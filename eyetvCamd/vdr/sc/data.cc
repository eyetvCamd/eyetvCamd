/*
 * Softcam plugin to VDR (C++)
 *
 * This code is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This code is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 * Or, point your browser to http://www.gnu.org/copyleft/gpl.html
 */

#include <ctype.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "data.h"
#include "misc.h"
#include "scsetup.h"
#include "log-core.h"
#include "i18n.h"

#define KEY_FILE     "SoftCam.Key"
#define EXT_AU_INT   (15*60*1000) // ms interval for external AU
#define EXT_AU_MIN   ( 2*60*1000) // ms min. interval for external AU

// -- cFileMap -----------------------------------------------------------------

cFileMap::cFileMap(const char *Filename, bool Rw)
{
  filename=strdup(Filename);
  rw=Rw;
  fd=-1; count=len=0; addr=0; failed=false;
}

cFileMap::~cFileMap()
{
  Clean();
  free(filename);
}

bool cFileMap::IsFileMap(const char *Name, bool Rw)
{
  return (!strcmp(Name,filename) && (!Rw || rw));
}

void cFileMap::Clean(void)
{
  if(addr) { munmap(addr,len); addr=0; len=0; }
  if(fd>=0) { close(fd); fd=-1; }
}

bool cFileMap::Map(void)
{
  cMutexLock lock(this);
  if(addr) { count++; return true; }
  if(!failed) {
    struct stat ds;
    if(!stat(filename,&ds)) {
      if(S_ISREG(ds.st_mode)) {
        fd=open(filename,rw ? O_RDWR : O_RDONLY);
        if(fd>=0) {
          unsigned char *map=(unsigned char *)mmap(0,ds.st_size,rw ? (PROT_READ|PROT_WRITE):(PROT_READ),MAP_SHARED,fd,0);
          if(map!=MAP_FAILED) {
            addr=map; len=ds.st_size; count=1;
            return true;
            }
          else PRINTF(L_GEN_ERROR,"mapping failed on %s: %s",filename,strerror(errno));
          close(fd); fd=-1;
          }
        else PRINTF(L_GEN_ERROR,"error opening filemap %s: %s",filename,strerror(errno));
        }
      else PRINTF(L_GEN_ERROR,"filemap %s is not a regular file",filename);
      }
    else PRINTF(L_GEN_ERROR,"can't stat filemap %s: %s",filename,strerror(errno));
    failed=true; // don't try this one over and over again
    }
  return false;
}

bool cFileMap::Unmap(void)
{
  cMutexLock lock(this);
  if(addr) {
    if(!(--count)) { Clean(); return true; }
    else Sync();
    }
  return false;
}

void cFileMap::Sync(void)
{
  cMutexLock lock(this);
  if(addr) msync(addr,len,MS_ASYNC);
}

// -- cFileMaps ----------------------------------------------------------------

cFileMaps filemaps;

cFileMaps::cFileMaps(void)
{
  cfgDir=0;
}

cFileMaps::~cFileMaps()
{
  Clear();
  free(cfgDir);
}

void cFileMaps::SetCfgDir(const char *CfgDir)
{
  free(cfgDir);
  cfgDir=strdup(CfgDir);
}

cFileMap *cFileMaps::GetFileMap(const char *name, const char *domain, bool rw)
{
  cMutexLock lock(this);
  char path[256];
  snprintf(path,sizeof(path),"%s/%s/%s",cfgDir,domain,name);
  cFileMap *fm=First();
  while(fm) {
    if(fm->IsFileMap(path,rw)) return fm;
    fm=Next(fm);
    }
  fm=new cFileMap(path,rw);
  Add(fm);
  return fm;  
}

// -- cStructItem --------------------------------------------------------------

cStructItem::cStructItem(void)
{
  comment=0; deleted=special=false;
}

cStructItem::~cStructItem()
{
  free(comment);
}

void cStructItem::SetComment(const char *com)
{
  free(comment);
  comment=com ? strdup(com):0;
}

bool cStructItem::Save(FILE *f)
{
  fprintf(f,"%s%s\n",*ToString(false),comment?comment:"");
  return ferror(f)==0;
}

// -- cCommentItem -------------------------------------------------------------

class cCommentItem : public cStructItem {
public:
  cCommentItem(void);
  };

cCommentItem::cCommentItem(void)
{
  SetSpecial();
}

// -- cStructLoader ------------------------------------------------------------

cStructLoader::cStructLoader(const char *Type, const char *Filename, int Flags)
:lock(true)
{
  path=0; mtime=0;
  type=Type; filename=Filename; flags=Flags&SL_CUSTOMMASK;
  cStructLoaders::Register(this);
}

cStructLoader::~cStructLoader()
{
  free(path);
}

void cStructLoader::AddItem(cStructItem *n, const char *com, cStructItem *ref)
{
  n->SetComment(com);
  ListLock(true);
  cStructItem *a=0;
  if(ref) { // insert before reference
    for(a=First(); a; a=Next(a))
      if(Next(a)==ref) break;
    }
  if(!a) { // insert before first non-special
    for(a=First(); a;) {
      cStructItem *nn=Next(a);
      if(nn && !nn->Special()) break;
      a=nn;
      }
    }
  Add(n,a);
  Modified();
  ListUnlock();
}

void cStructLoader::DelItem(cStructItem *d, bool keep)
{
  if(d) {
    d->Delete();
    if(keep) {
      cStructItem *n=new cCommentItem;
      n->SetComment(cString::sprintf(";%s%s",*d->ToString(false),d->Comment()?d->Comment():""));
      ListLock(true);
      Add(n,d);
      ListUnlock();
      }
    Modified();
    }
}

cStructItem *cStructLoader::NextValid(cStructItem *it) const
{
  while(it && !it->Valid()) it=Next(it);
  return it;
}

void cStructLoader::SetCfgDir(const char *cfgdir)
{
  free(path);
  path=strdup(AddDirectory(cfgdir,filename));
}

time_t cStructLoader::MTime(bool log)
{
  struct stat st;
  if(stat(path,&st)!=0) {
    if(log) {
      PRINTF(L_GEN_ERROR,"failed fstat %s: %s",path,strerror(errno));
      PRINTF(L_GEN_WARN,"automatic reload of %s disabled",path);
      }
    st.st_mtime=0;
    }
  return st.st_mtime;
}

void cStructLoader::CheckAccess(void)
{
  if(access(path,R_OK|W_OK)!=0) {
    if(errno!=EACCES)
      PRINTF(L_GEN_ERROR,"failed access %s: %s",path,strerror(errno));
    PRINTF(L_GEN_WARN,"no write permission on %s. Changes will not be saved!",path);
    SL_SETFLAG(SL_NOACCESS);
    }
  else SL_CLRFLAG(SL_NOACCESS);
}

bool cStructLoader::CheckUnmodified(void)
{
  time_t curr_mtime=MTime(false);
  if(mtime && mtime<curr_mtime && SL_TSTFLAG(SL_WATCH)) {
     PRINTF(L_CORE_LOAD,"abort save as file %s has been changed externaly",path);
     return false;
     }
  return true;
}

bool cStructLoader::CheckDoSave(void)
{
  return !SL_TSTFLAG(SL_DISABLED) && SL_TSTFLAG(SL_READWRITE)
         && !SL_TSTFLAG(SL_NOACCESS) && SL_TSTFLAG(SL_LOADED)
         && IsModified() && CheckUnmodified();
}

void cStructLoader::LoadFinished(void)
{
  SL_CLRFLAG(SL_SHUTUP);
  if(!SL_TSTFLAG(SL_LOADED))
    PRINTF(L_CORE_LOAD,"loading %s terminated with error. Changes will not be saved!",path);
}

void cStructLoader::OpenFailed(void)
{
  if(SL_TSTFLAG(SL_VERBOSE) && !SL_TSTFLAG(SL_SHUTUP)) {
    PRINTF(L_GEN_ERROR,"failed open %s: %s",path,strerror(errno));
    SL_SETFLAG(SL_SHUTUP);
    }
  if(SL_TSTFLAG(SL_MISSINGOK)) SL_SETFLAG(SL_LOADED);
  else SL_CLRFLAG(SL_LOADED);
}

void cStructLoader::Load(bool reload)
{
  if(SL_TSTFLAG(SL_DISABLED) || (reload && !SL_TSTFLAG(SL_WATCH))) return;
  FILE *f=fopen(path,"r");
  if(f) {
    int curr_mtime=MTime(true);
    ListLock(true);
    bool doload=false;
    if(!reload) {
      Clear(); Modified(false);
      mtime=curr_mtime;
      doload=true;
      }
    else if(mtime && mtime<curr_mtime) {
      PRINTF(L_CORE_LOAD,"detected change of %s",path);
      if(IsModified())
        PRINTF(L_CORE_LOAD,"discarding in-memory changes");
      for(cStructItem *a=First(); a; a=Next(a)) DelItem(a);
      Modified(false);
      mtime=curr_mtime;
      doload=true;
      }
    if(doload) {
      SL_SETFLAG(SL_LOADED);
      PRINTF(L_GEN_INFO,"loading %s from %s",type,path);
      CheckAccess();
      int lineNum=0, num=0;
      char buff[4096];
      while(fgets(buff,sizeof(buff),f)) {
        lineNum++;
        if(!index(buff,'\n') && !feof(f)) {
          PRINTF(L_GEN_ERROR,"file %s readbuffer overflow line#%d",path,lineNum);
          SL_CLRFLAG(SL_LOADED);
          break;
          }
        strreplace(buff,'\n',0); strreplace(buff,'\r',0); // chomp
        bool hasContent=false;
        char *ls;
        for(ls=buff; *ls; ls++) {
          if(*ls==';' || *ls=='#') {		  // comment
            if(hasContent)
              while(ls>buff && ls[-1]<=' ') ls--; // search back to non-whitespace
            break;
            }
          if(*ls>' ') hasContent=true;		  // line contains something usefull
          }
        cStructItem *it=0;
        if(hasContent) {
          char save=*ls;
          *ls=0; it=ParseLine(skipspace(buff)); *ls=save;
          if(!it) {
            PRINTF(L_GEN_ERROR,"file %s has error in line #%d",path,lineNum);
            ls=buff;
            }
          else num++;
          }
        else ls=buff;
        if(!it) it=new cCommentItem;
        if(it) {
          it->SetComment(ls);
          Add(it);
          }
        else {
          PRINTF(L_GEN_ERROR,"out of memory loading file %s",path);
          SL_CLRFLAG(SL_LOADED);
          break;
          }
        }
      ListUnlock();
      PRINTF(L_CORE_LOAD,"loaded %d %s from %s",num,type,path);
      PostLoad();
      }
    else ListUnlock();

    fclose(f);
    LoadFinished();
    }
  else
    OpenFailed();
}

void cStructLoader::Purge(void)
{
  if(!SL_TSTFLAG(SL_DISABLED) && !SL_TSTFLAG(SL_NOPURGE)) {
    ListLock(true);
    for(cStructItem *it=First(); it;) {
      cStructItem *n=Next(it);
      if(it->Deleted()) Del(it);
      it=n;
      }
    ListUnlock();
    }
}

void cStructLoader::Save(void)
{
  if(CheckDoSave()) {
    cSafeFile f(path);
    if(f.Open()) {
      ListLock(false);
      for(cStructItem *it=First(); it; it=Next(it))
        if(!it->Deleted() && !it->Save(f)) break;
      f.Close();
      mtime=MTime(true);
      Modified(false);
      ListUnlock();
      PRINTF(L_CORE_LOAD,"saved %s to %s",type,path);
      }
    }
}

// -- cStructLoaderPlain -------------------------------------------------------

cStructLoaderPlain::cStructLoaderPlain(const char *Type, const char *Filename, int Flags)
:cStructLoader(Type,Filename,Flags)
{}

void cStructLoaderPlain::PreLoad(void)
{
  ListLock(true);
  Clear(); Modified(false);
  ListUnlock();
}

void cStructLoaderPlain::Load(bool reload)
{
  if(SL_TSTFLAG(SL_DISABLED) || reload) return;
  FILE *f=fopen(path,"r");
  if(f) {
    PreLoad();
    ListLock(true);
    SL_SETFLAG(SL_LOADED);
    PRINTF(L_GEN_INFO,"loading %s from %s",type,path);
    CheckAccess();
    int lineNum=0;
    char buff[4096];
    while(fgets(buff,sizeof(buff),f)) {
      lineNum++;
      if(!index(buff,'\n') && !feof(f)) {
        PRINTF(L_GEN_ERROR,"file %s readbuffer overflow line#%d",path,lineNum);
        SL_CLRFLAG(SL_LOADED);
        break;
        }
      strreplace(buff,'\n',0); strreplace(buff,'\r',0); // chomp
      bool hasContent=false;
      char *ls;
      for(ls=buff; *ls; ls++) {
        if(*ls==';' || *ls=='#') break;
        if(*ls>' ') hasContent=true;
        }
      if(hasContent) {
        *ls=0;
        if(!ParseLinePlain(skipspace(buff)))
          PRINTF(L_GEN_ERROR,"file %s has error in line #%d",path,lineNum);
        }
      }
    ListUnlock();
    PostLoad();
    fclose(f);
    LoadFinished();
    }
  else
    OpenFailed();
}

void cStructLoaderPlain::PreSave(FILE *f)
{
  fprintf(f,"## This is a generated file. DO NOT EDIT!!\n"
            "## This file will be OVERWRITTEN WITHOUT WARNING!!\n");
}

void cStructLoaderPlain::Save(void)
{
  if(CheckDoSave()) {
    cSafeFile f(path);
    if(f.Open()) {
      ListLock(false);
      PreSave(f);
      for(cStructItem *it=First(); it; it=Next(it))
        if(!it->Deleted() && !it->Save(f)) break;
      PostSave(f);
      f.Close();
      Modified(false);
      ListUnlock();
      PRINTF(L_CORE_LOAD,"saved %s to %s",type,path);
      }
    }
}

// -- cStructLoaders -----------------------------------------------------------

#define RELOAD_TIMEOUT  20300 // ms
#define PURGE_TIMEOUT   60700 // ms
#define SAVE_TIMEOUT     5000 // ms

cStructLoader *cStructLoaders::first=0;
cTimeMs cStructLoaders::lastReload;
cTimeMs cStructLoaders::lastPurge;
cTimeMs cStructLoaders::lastSave;

void cStructLoaders::Register(cStructLoader *ld)
{
  PRINTF(L_CORE_DYN,"structloaders: registering loader %s",ld->type);
  ld->next=first;
  first=ld;
}

void cStructLoaders::SetCfgDir(const char *cfgdir)
{
  for(cStructLoader *ld=first; ld; ld=ld->next)
    ld->SetCfgDir(cfgdir);
}

void cStructLoaders::Load(bool reload)
{
  if(!reload || lastReload.TimedOut()) {
    for(cStructLoader *ld=first; ld; ld=ld->next) ld->Load(reload);
    lastReload.Set(RELOAD_TIMEOUT);
    }
}

void cStructLoaders::Save(bool force)
{
  if(force || lastSave.TimedOut()) {
    for(cStructLoader *ld=first; ld; ld=ld->next) ld->Save();
    lastSave.Set(SAVE_TIMEOUT);
    }
}

void cStructLoaders::Purge(void)
{
  if(lastPurge.TimedOut()) {
    for(cStructLoader *ld=first; ld; ld=ld->next) ld->Purge();
    lastPurge.Set(PURGE_TIMEOUT);
    }
}

// -- cPid ---------------------------------------------------------------------

cPid::cPid(int Pid, int Section, int Mask, int Mode)
{
  pid=Pid;
  sct=Section;
  mask=Mask;
  mode=Mode;
  filter=0;
}

// -- cPids --------------------------------------------------------------------

void cPids::AddPid(int Pid, int Section, int Mask, int Mode)
{
  if(!HasPid(Pid,Section,Mask,Mode)) {
    cPid *pid=new cPid(Pid,Section,Mask,Mode);
    Add(pid);
    }
}

bool cPids::HasPid(int Pid, int Section, int Mask, int Mode)
{
  for(cPid *pid=First(); pid; pid=Next(pid))
    if(pid->pid==Pid && pid->sct==Section && pid->mask==Mask && pid->mode==Mode)
      return true;
  return false;
}

// -- cEcmInfo -----------------------------------------------------------------

cEcmInfo::cEcmInfo(void)
{
  Setup();
}

cEcmInfo::cEcmInfo(const cEcmInfo *e)
{
  Setup(); SetName(e->name);
  ecm_pid=e->ecm_pid;
  ecm_table=e->ecm_table;
  caId=e->caId;
  emmCaId=e->emmCaId;
  provId=e->provId;
  Update(e);
  prgId=e->prgId;
  source=e->source;
  transponder=e->transponder;
}

cEcmInfo::cEcmInfo(const char *Name, int Pid, int CaId, int ProvId)
{
  Setup(); SetName(Name);
  ecm_pid=Pid;
  caId=CaId;
  provId=ProvId;
}

cEcmInfo::~cEcmInfo()
{
  ClearData();
  free(name);
}

void cEcmInfo::Setup(void)
{
  cached=failed=false;
  name=0; data=0;
  prgId=source=transponder=-1;
  ecm_table=0x80; emmCaId=0;
}

bool cEcmInfo::Compare(const cEcmInfo *e)
{
  return prgId==e->prgId && source==e->source && transponder==e->transponder &&
         caId==e->caId && ecm_pid==e->ecm_pid && provId==e->provId;
}

bool cEcmInfo::Update(const cEcmInfo *e)
{
  return (e->data && (!data || e->dataLen!=dataLen)) ? AddData(e->data,e->dataLen) : false;
}

void cEcmInfo::SetSource(int PrgId, int Source, int Transponder)
{
  prgId=PrgId;
  source=Source;
  transponder=Transponder;
}

void cEcmInfo::ClearData(void)
{
  free(data); data=0;
}

bool cEcmInfo::AddData(const unsigned char *Data, int DataLen)
{
  ClearData();
  data=MALLOC(unsigned char,DataLen);
  if(data) {
    memcpy(data,Data,DataLen);
    dataLen=DataLen;
    }
  else PRINTF(L_GEN_ERROR,"malloc failed in cEcmInfo::AddData()");
  return (data!=0);
}

void cEcmInfo::SetName(const char *Name)
{
  free(name);
  name=strdup(Name);
}

// -- cPlainKey ----------------------------------------------------------------

cPlainKey::cPlainKey(bool CanSupersede)
{
  super=CanSupersede;
}

bool cPlainKey::Set(int Type, int Id, int Keynr, void *Key, int Keylen)
{
  type=Type; id=Id; keynr=Keynr;
  return SetKey(Key,Keylen);
}

cString cPlainKey::PrintKeyNr(void)
{
  return cString::sprintf("%02X",keynr);
}

int cPlainKey::IdSize(void)
{
  return id>0xFF ? (id>0xFFFF ? 6 : 4) : 2;
}

cString cPlainKey::ToString(bool hide)
{
  return cString::sprintf(hide ? "%c %.*X %s %.4s..." : "%c %.*X %s %s",type,IdSize(),id,*PrintKeyNr(),*Print());
}

// -- cMutableKey --------------------------------------------------------------

cMutableKey::cMutableKey(bool Super)
:cPlainKey(Super)
{
  real=0;
}

cMutableKey::~cMutableKey()
{
  delete real;
}

bool cMutableKey::SetKey(void *Key, int Keylen)
{
  delete real;
  return (real=Alloc()) && real->SetKey(Key,Keylen);
}

bool cMutableKey::SetBinKey(unsigned char *Mem, int Keylen)
{
  delete real;
  return (real=Alloc()) && real->SetBinKey(Mem,Keylen);
}

int cMutableKey::Size(void)
{
  return real->Size();
}

bool cMutableKey::Cmp(void *Key, int Keylen)
{
  return real->Cmp(Key,Keylen);
}

bool cMutableKey::Cmp(cPlainKey *k)
{
  cMutableKey *mk=dynamic_cast<cMutableKey *>(k); // high magic ;)
  return real->Cmp(mk ? mk->real : k);
}

void cMutableKey::Get(void *mem)
{
  real->Get(mem);
}

cString cMutableKey::Print(void)
{
  return real->Print();
}

// ----------------------------------------------------------------

class cPlainKeyDummy : public cPlainKey {
private:
  char *str;
  int len;
protected:
  virtual cString Print(void);
  virtual cString PrintKeyNr(void) { return ""; }
public:
  cPlainKeyDummy(void);
  ~cPlainKeyDummy();
  virtual bool Parse(const char *line);
  virtual bool Cmp(void *Key, int Keylen) { return false; }
  virtual bool Cmp(cPlainKey *k) { return false; }
  virtual void Get(void *mem) {}
  virtual int Size(void) { return len; }
  virtual bool SetKey(void *Key, int Keylen);
  virtual bool SetBinKey(unsigned char *Mem, int Keylen);
  };

cPlainKeyDummy::cPlainKeyDummy(void):
cPlainKey(false)
{
  str=0;
}

cPlainKeyDummy::~cPlainKeyDummy()
{
  free(str);
}

bool cPlainKeyDummy::SetKey(void *Key, int Keylen)
{
  len=Keylen;
  free(str);
  str=MALLOC(char,len+1);
  if(str) strn0cpy(str,(char *)Key,len+1);
  return str!=0;
}

bool cPlainKeyDummy::SetBinKey(unsigned char *Mem, int Keylen)
{
  return SetKey(Mem,Keylen);
}

cString cPlainKeyDummy::Print(void)
{
  return str;
}

bool cPlainKeyDummy::Parse(const char *line)
{
  unsigned char sid[3];
  int len;
  if(GetChar(line,&type,1) && (len=GetHex(line,sid,3,false))) {
    type=toupper(type); id=Bin2Int(sid,len);
    char *l=strdup(line);
    line=skipspace(stripspace(l));
    SetKey((void *)line,strlen(line));
    free(l);
    return true;
    }
  return false;
}

// -- cPlainKeyTypeDummy -------------------------------------------------------

class cPlainKeyTypeDummy : public cPlainKeyType {
public:
  cPlainKeyTypeDummy(int Type):cPlainKeyType(Type,false) {}
  virtual cPlainKey *Create(void) { return new cPlainKeyDummy; }
  };

// -- cPlainKeyType ------------------------------------------------------------

cPlainKeyType::cPlainKeyType(int Type, bool Super)
{
  type=Type;
  cPlainKeys::Register(this,Super);
}

// -- cLastKey -----------------------------------------------------------------

cLastKey::cLastKey(void)
{
  lastType=lastId=lastKeynr=-1;
}

bool cLastKey::NotLast(int Type, int Id, int Keynr)
{
  if(lastType!=Type || lastId!=Id || lastKeynr!=Keynr) {
    lastType=Type; lastId=Id; lastKeynr=Keynr;
    return true;
    }
  return false;
}

// -- cPlainKeys ---------------------------------------------------------------

const char *externalAU=0;

cPlainKeys keys;

cPlainKeyType *cPlainKeys::first=0;

cPlainKeys::cPlainKeys(void)
:cStructList<cPlainKey>("keys",KEY_FILE,SL_READWRITE|SL_MISSINGOK|SL_WATCH|SL_VERBOSE)
{}

void cPlainKeys::Register(cPlainKeyType *pkt, bool Super)
{
  PRINTF(L_CORE_DYN,"registering key type %c%s",pkt->type,Super?" (super)":"");
  pkt->next=first;
  first=pkt;
}

void cPlainKeys::Trigger(int Type, int Id, int Keynr)
{
  if(lastkey.NotLast(Type,Id,Keynr))
    PRINTF(L_CORE_AUEXTERN,"triggered from findkey (%s)",*KeyString(Type,Id,Keynr));
  ExternalUpdate();
}

cPlainKey *cPlainKeys::FindKey(int Type, int Id, int Keynr, int Size, cPlainKey *key)
{
  key=FindKeyNoTrig(Type,Id,Keynr,Size,key);
  if(!key) Trigger(Type,Id,Keynr);
  return key;
}

cPlainKey *cPlainKeys::FindKeyNoTrig(int Type, int Id, int Keynr, int Size, cPlainKey *key)
{
  ListLock(false);
  for(key=key?Next(key):First(); key; key=Next(key))
    if(key->type==Type && key->id==Id && key->keynr==Keynr && (Size<0 || key->Size()==Size))
      break;
  ListUnlock();
  return key;
}

cPlainKey *cPlainKeys::NewFromType(int type)
{
  cPlainKeyType *pkt;
  for(pkt=first; pkt; pkt=pkt->next)
    if(pkt->type==type) return pkt->Create();
  PRINTF(L_CORE_LOAD,"unknown key type '%c', adding dummy",type);
  pkt=new cPlainKeyTypeDummy(type);
  return pkt->Create();
}

bool cPlainKeys::AddNewKey(cPlainKey *nk, const char *reason)
{
  cPlainKey *k;
  for(k=0; (k=FindKeyNoTrig(nk->type,nk->id,nk->keynr,-1,k)); )
    if(k->Cmp(nk)) return false;

  cPlainKey *ref=0;
  cString ks=nk->ToString(true);
  PRINTF(L_GEN_INFO,"key update for ID %s",*ks);
  ums.Queue("%s %s",tr("Key update"),*ks);
  for(k=0; (k=FindKeyNoTrig(nk->type,nk->id,nk->keynr,nk->Size(),k)); ) {
    if(nk->CanSupersede()) {
      PRINTF(L_GEN_INFO,"supersedes key: %s",*k->ToString(true));
      DelItem(k,ScSetup.SuperKeys==0);
      }
    if(!ref) ref=k;
    }
  char stamp[32], com[256];
  time_t tt=time(0);
  struct tm tm_r;
  strftime(stamp,sizeof(stamp),"%d.%m.%Y %T",localtime_r(&tt,&tm_r));
  snprintf(com,sizeof(com)," ; %s %s",reason,stamp);
  AddItem(nk,com,ref);
  return true;
}

bool cPlainKeys::NewKey(int Type, int Id, int Keynr, void *Key, int Keylen)
{
  cPlainKey *nk=NewFromType(Type);
  if(nk) {
    nk->Set(Type,Id,Keynr,Key,Keylen);
    return AddNewKey(nk,"from AU");
    }
  return false;
}

bool cPlainKeys::NewKeyParse(char *line, const char *reason)
{
  cPlainKey *nk=ParseLine(line);
  return nk && AddNewKey(nk,reason);
}

cPlainKey *cPlainKeys::ParseLine(char *line)
{
  char *s=skipspace(line);
  cPlainKey *k=NewFromType(toupper(*s));
  if(k && !k->Parse(line)) { delete k; k=0; }
  return k;
}

cString cPlainKeys::KeyString(int Type, int Id, int Keynr)
{
  cPlainKey *pk=NewFromType(Type);
  char *s;
  if(pk) {
    pk->type=Type; pk->id=Id; pk->keynr=Keynr;
    asprintf(&s,"%c %.*X %s",Type,pk->IdSize(),Id,*pk->PrintKeyNr());
    delete pk;
    }
  else s=strdup("unknown");
  return cString(s,true);
}

void cPlainKeys::PostLoad(void)
{
  ListLock(false);
  if(Count() && LOG(L_CORE_KEYS)) {
    for(cPlainKey *dat=First(); dat; dat=Next(dat))
      PRINTF(L_CORE_KEYS,"keys %s",*dat->ToString(false));
    }
  ListUnlock();
}

void cPlainKeys::HouseKeeping(void)
{
  if(trigger.TimedOut()) {
    trigger.Set(EXT_AU_INT);
    if(externalAU) PRINTF(L_CORE_AUEXTERN,"triggered from housekeeping");
    ExternalUpdate();
    }
}

void cPlainKeys::ExternalUpdate(void)
{
  if(externalAU && ScSetup.AutoUpdate>0) {
    if(last.TimedOut()) {
      Lock();
      if(!Active())
        Start();
      else PRINTF(L_CORE_AUEXTERN,"still running");
      Unlock();
      }
    else PRINTF(L_CORE_AUEXTERN,"denied, min. timeout not expired");
    }
}

void cPlainKeys::Action(void)
{
  last.Set(EXT_AU_MIN);
  PRINTF(L_CORE_AUEXTERN,"starting...");
  cTimeMs start;
  cPipe pipe;
  if(pipe.Open(externalAU,"r")) {
    char buff[1024];
    while(fgets(buff,sizeof(buff),pipe)) {
      char *line=skipspace(stripspace(buff));
      if(line[0]==0 || line[0]==';' || line[0]=='#') continue;
      NewKeyParse(line,"from ExtAU");
      }
    }
  pipe.Close();
  PRINTF(L_CORE_AUEXTERN,"done (elapsed %d)",(int)start.Elapsed());
}

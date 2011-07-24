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

#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "../tools.h"

#include "sc.h"
#include "scsetup.h"
#include "system.h"
#include "data.h"
#include "opts.h"
#include "log-core.h"
#include "i18n.h"

// --- cFeature ----------------------------------------------------------------

cFeature Feature;

bool cFeature::keyfile=false;
bool cFeature::smartcard=false;

void cFeature::NeedsKeyFile(void)
{
  if(!keyfile) PRINTF(L_CORE_DYN,"feature: using feature KEYFILE");
  keyfile=true;
}

void cFeature::NeedsSmartCard(void)
{
  if(!smartcard) PRINTF(L_CORE_DYN,"feature: using feature SMARTCARD");
  smartcard=true;
}

// -- cKeySnoop ----------------------------------------------------------------

cKeySnoop::cKeySnoop(cSystem *Sys, int Type, int Id, int Keynr)
{
  ok=false;
  sys=Sys; type=Type; id=Id; keynr=Keynr;
}

cKeySnoop::~cKeySnoop()
{
  if(!ok) sys->KeyFail(type,id,keynr);
}

void cKeySnoop::OK(cPlainKey *pk)
{
  sys->KeyOK(pk);
  ok=true;
}

// -- cLogHook -----------------------------------------------------------------

cLogHook::cLogHook(int Id, const char *Name)
{
  id=Id; name=Name;
  bailOut=false;
}

// -- cSystem ------------------------------------------------------------------

#define MAX_CHID 10

struct EcmCheck {
  union {
    struct {
      bool startFlag;
      int current, chids[MAX_CHID];
      } sys06;
    } caid;
  };

int cSystem::foundKeys=0;
int cSystem::newKeys=0;

cSystem::cSystem(const char *Name, int Pri)
{
  name=Name; pri=Pri;
  currentKeyStr[0]=0; doLog=true; cardNum=-1; logecm=0;
  check=new struct EcmCheck;
  memset(check,0,sizeof(struct EcmCheck));
  // default config
  maxEcmTry=2; // try to get a key X times from the same ECM pid (default)
  hasLogger=false;
  needsLogger=false;
  local=true;
  needsDescrData=false;
  constant=false;
}

cSystem::~cSystem()
{
  delete check;
  if(logecm) cSoftCAM::SetLogStatus(cardNum,logecm,false);
  delete logecm;
}

void cSystem::StartLog(const cEcmInfo *ecm, int caid)
{
  if(!logecm || logecm->caId!=caid) {
    if(logecm) cSoftCAM::SetLogStatus(cardNum,logecm,false);
    delete logecm;
    logecm=new cEcmInfo(ecm);
    logecm->caId=caid; logecm->emmCaId=0;
    cSoftCAM::SetLogStatus(cardNum,logecm,true);
    }
}

void cSystem::ParseCADescriptor(cSimpleList<cEcmInfo> *ecms, unsigned short sysId, const unsigned char *data, int len)
{
  const int pid=WORD(data,2,0x1FFF);
  switch(sysId>>8) {
    case 0x01: // Seca style
      for(int p=2; p<len; p+=15) {
        cEcmInfo *n=new cEcmInfo(name,WORD(data,p,0x1FFF),sysId,WORD(data,p+2,0xFFFF));
        if(data[p+4]==0xFF) n->AddData(&data[p+5],10);
        ecms->Add(n);
        }
      break;
    case 0x05: // Viaccess style
      for(int p=4; p<len; p+=2+data[p+1])
        if(data[p]==0x14)
          ecms->Add(new cEcmInfo(name,pid,sysId,(data[p+2]<<16)|(data[p+3]<<8)|(data[p+4]&0xF0)));
      break;
    default:   // default style
      {
      cEcmInfo *n=new cEcmInfo(name,pid,sysId,0);
      if(sysId==0x1234) { // BEV
        n->ecm_table=0x8e;
        n->emmCaId=0x1801;
        }
      ecms->Add(n);
      break;
      }
    }
}

void cSystem::ParseCAT(cPids *pids, const unsigned char *buffer)
{
  if(buffer[0]==0x09) {
    int caid=WORD(buffer,2,0xFFFF);
    int pid=WORD(buffer,4,0x1FFF);
    switch(caid>>8) {
      case 0x01: // Seca style (82/84)
        if(buffer[1]>4) {
          pids->AddPid(pid,0x82,0xFF); // Unique updates
          for(int i=7, nn=buffer[6] ; nn ; nn--,i+=4)
            pids->AddPid(WORD(buffer,i,0x1FFF),0x84,0xFF); // Shared updates
          }
        break;
      case 0x05: // Viaccess style (88/8c/8d/8e)
        pids->AddPid(pid,0x8B,0xFE,0x07); // mismatching 89/8f
        break;
      case 0x0d: // Cryptoworks style (82/84/86/88/89)
        pids->AddPid(pid,0x80,0xFF,0x06);
        pids->AddPid(pid,0x88,0xFE);
        break;
      case 0x18: // Nagra style, Nagra1(82) Nagra2(82/83) 
        pids->AddPid(pid,0x82,caid==0x1801 ? 0xFE:0xFF);
        break;
      default:   // default style (82)
        pids->AddPid(pid,0x82,0xFF);
        break;
      }
    }
}

int cSystem::CheckECM(const cEcmInfo *ecm, const unsigned char *data, bool sync)
{
  switch(ecm->caId>>8) {
    case 0x06: // Irdeto
      {
      const int cur=data[4];
      const int max=data[5];
      const int chid=WORD(data,6,0xFFFF);
      // if multiple channel id's, use current one only
      if(sync && ecm->caId==0x0604 && check->caid.sys06.current>0 && chid!=check->caid.sys06.current) {
        PRINTF(L_CORE_ECMPROC,"ecmcheck(%s): chid %04x != current %04x",name,chid,check->caid.sys06.current);
        return 2;
        }
      // search for fake channel id's on Stream
      if(!sync && max>0 && max<MAX_CHID) {
        if(cur==0) check->caid.sys06.startFlag=true;
        if(check->caid.sys06.startFlag) {
          if(cur<=max) check->caid.sys06.chids[cur]=chid;
          if(cur==max) {
            for(int i=0 ; i<max ; i++) {
              if(check->caid.sys06.chids[i]==0) {
                check->caid.sys06.startFlag=false;
                PRINTF(L_CORE_ECMPROC,"ecmcheck(%s): zero chid",name);
                return 1;
                }
              for(int j=i+1 ; j<=max ; j++) {
                if(check->caid.sys06.chids[i]==check->caid.sys06.chids[j]) {
                  check->caid.sys06.startFlag=false;
                  PRINTF(L_CORE_ECMPROC,"ecmcheck(%s): duplicate chid %04x",name,check->caid.sys06.chids[i]);
                  return 1;
                  }
                }
              }
            }
          }
        }
      break;
      }
    }
  return 0;
}

void cSystem::CheckECMResult(const cEcmInfo *ecm, const unsigned char *data, bool result)
{
  switch(ecm->caId>>8) {
    case 0x06: // Irdeto
      check->caid.sys06.current=result ? WORD(data,6,0xFFFF) : 0;
      break;
    }
}

void cSystem::KeyOK(cPlainKey *pk)
{
  if(lastkey.NotLast(pk->type,pk->id,pk->keynr)) {
    strn0cpy(currentKeyStr,pk->ToString(false),sizeof(currentKeyStr));
    PRINTF(L_CORE_ECM,"system: using key %s",*pk->ToString(true));
    doLog=true;
    }
}

void cSystem::KeyOK(const char *txt)
{
  snprintf(currentKeyStr,sizeof(currentKeyStr),"%s (%s)\n",txt?txt:name,tr("undisclosed key"));
  doLog=true;
}

void cSystem::KeyFail(int type, int id, int keynr)
{
  keys.Trigger(type,id,keynr);
  if(lastkey.NotLast(type,id,keynr) && doLog)
    PRINTF(L_CORE_ECM,"system: no key found for %s",*keys.KeyString(type,id,keynr));
}

// -- cSystemLink --------------------------------------------------------------

cSystemLink::cSystemLink(const char *Name, int Pri)
{
  name=Name; pri=Pri;
  opts=0; noFF=false;
  cSystems::Register(this);
}

cSystemLink::~cSystemLink()
{
  delete opts;
}

// -- cSystems -----------------------------------------------------------------

cSystemLink *cSystems::first=0;
int cSystems::nextSysIdent=0x1000;

void cSystems::Register(cSystemLink *sysLink)
{
  PRINTF(L_CORE_DYN,"systems: registering CA system %s, pri %d, ident %04X",sysLink->name,sysLink->pri,nextSysIdent);
  sysLink->next=first;
  sysLink->sysIdent=nextSysIdent++;
  first=sysLink;
}

int cSystems::Provides(const unsigned short *SysIds, bool ff)
{
  int n=0;
  for(; *SysIds; SysIds++) {
    if(ScSetup.Ignore(*SysIds)) continue;
    cSystemLink *sl=first;
    while(sl) {
      if(sl->CanHandle(*SysIds)) {
        if(sl->noFF && ff) return 0;
        n++;
        break;
        }
      sl=sl->next;
      }
    }
  return n;
}

cSystemLink *cSystems::FindByName(const char *Name)
{
  cSystemLink *sl=first;
  while(sl) {
    if(!strcasecmp(sl->name,Name)) return sl;
    sl=sl->next;
    }
  return 0;
}

cSystemLink *cSystems::FindById(unsigned short SysId, bool ff, int oldPri)
{
  // all pri's are negative!
  // oldPri = 0 -> get highest pri system
  // oldPri < 0 -> get highest pri system with pri<oldPri
  // oldPri > 0 -> get lowest pri system

  cSystemLink *sl=first, *csl=0;
  while(sl) {
    if((!ff || !sl->noFF) && sl->CanHandle(SysId)
       && sl->pri<oldPri
       && (!csl 
           || (oldPri<=0 && sl->pri>csl->pri)
           || (oldPri>0 && sl->pri<csl->pri)
          )
       ) csl=sl;
    sl=sl->next;
    }
  return csl;
}

cSystemLink *cSystems::FindByIdent(int ident)
{
  cSystemLink *sl=first;
  while(sl) {
    if(sl->sysIdent==ident) return sl;
    sl=sl->next;
    }
  return 0;
}

cSystem *cSystems::FindBySysName(unsigned short SysId, bool ff, const char *Name)
{
  cSystemLink *sl=FindByName(Name);
  if(sl && (!ff || !sl->noFF) && !ScSetup.Ignore(SysId) && sl->CanHandle(SysId)) return sl->Create();
  return 0;
}

cSystem *cSystems::FindBySysId(unsigned short SysId, bool ff, int oldPri)
{
  if(!ScSetup.Ignore(SysId)) {
    cSystemLink *csl=FindById(SysId,ff,oldPri);
    if(csl) return csl->Create();
    }
  return 0;
}

int cSystems::FindIdentBySysId(unsigned short SysId, bool ff, int &Pri)
{
  if(!ScSetup.Ignore(SysId)) {
    cSystemLink *csl=FindById(SysId,ff,Pri);
    if(csl) {
      Pri=csl->pri;
      return csl->sysIdent;
      }
    }
  return 0;
}

cSystem *cSystems::FindBySysIdent(int ident)
{
  cSystemLink *csl=FindByIdent(ident);
  return csl ? csl->Create() : 0;
}

bool cSystems::Init(const char *cfgdir)
{
  PRINTF(L_CORE_LOAD,"** registered systems:");
  for(cSystemLink *sl=first; sl; sl=sl->next)
    PRINTF(L_CORE_LOAD,"** %-16s  (pri %3d)",sl->name,sl->pri);
  for(cSystemLink *sl=first; sl; sl=sl->next)
    if(!sl->Init(cfgdir)) return false;
  return true;
}

void cSystems::Clean(void)
{
  for(cSystemLink *sl=first; sl; sl=sl->next) sl->Clean();
}

bool cSystems::ConfigParse(const char *Name, const char *Value)
{
  char sysName[32];
  unsigned int i;
  for(i=0; Name[i] && Name[i]!='.' && i<sizeof(sysName); i++)
    sysName[i]=Name[i];
  if(Name[i]) {
    sysName[i]=0; i++;
    cSystemLink *sl=FindByName(sysName);
    if(sl && sl->opts) return sl->opts->Parse(&Name[i],Value);
    }
  return false;
}

void cSystems::ConfigStore(bool AsIs)
{
  for(cSystemLink *sl=first; sl; sl=sl->next)
    if(sl->opts && sl->opts->Store(AsIs)) sl->NewConfig();
}

cOpts *cSystems::GetSystemOpts(bool start)
{
  static cSystemLink *sl=0;
  sl=(start || !sl) ? first : sl->next;
  while(sl) {
    if(sl->opts) return sl->opts;
    sl=sl->next;
    }
  return 0;
}

// -- cMsgCache ----------------------------------------------------------------

#define FREE   0x00 // modes
#define FAIL1  0x01
#define FAIL2  0x3D
#define FAILN  0x3E
#define GOOD   0x3F
#define MASK   0x3F
#define QUEUED 0x40
#define WAIT   0x80

struct Cache {
  int crc;
  int mode;
  };

cMsgCache::cMsgCache(int NumCache, int StoreSize)
{
  numCache=NumCache;
  storeSize=StoreSize;
  ptr=0; stores=0; maxFail=2;
  caches=MALLOC(struct Cache,numCache);
  if(caches) {
    Clear();
    if(storeSize>0) {
      stores=MALLOC(unsigned char,numCache*storeSize);
      if(!stores) PRINTF(L_GEN_ERROR,"msgcache: no memory for store area");
      }
    }
  else PRINTF(L_GEN_ERROR,"msgcache: no memory for cache");
}

cMsgCache::~cMsgCache()
{
  free(caches);
  free(stores);
}

void cMsgCache::SetMaxFail(int maxfail)
{
  maxFail=min(maxfail,FAILN);
  PRINTF(L_CORE_MSGCACHE,"%d/%p: maxFail set to %d",getpid(),this,maxFail);
}

void cMsgCache::Clear(void)
{
  cMutexLock lock(&mutex);
  memset(caches,0,sizeof(struct Cache)*numCache);
  ptr=0;
  PRINTF(L_CORE_MSGCACHE,"%d/%p: clear",getpid(),this);
}

struct Cache *cMsgCache::FindMsg(int crc)
{
  int i=ptr;
  while(1) {
    if(--i<0) i=numCache-1;
    struct Cache * const s=&caches[i];
    if(!s->mode) break;
    if(s->crc==crc) return s;
    if(i==ptr) break;
    }
  return 0;
}

// returns:
// -1 - msg cached as failed (max failed)
// 0  - msg cached as good, result stored
// >0 - msg not cached, queue id
int cMsgCache::Get(const unsigned char *msg, int len, unsigned char *store)
{
  int crc=crc32_le(0,msg,len);
  cMutexLock lock(&mutex);
  if(!caches || (storeSize>0 && !stores)) return -1; // sanity
  struct Cache *s;
  while((s=FindMsg(crc))) {
    if(!(s->mode&QUEUED)) break;
    s->mode|=WAIT;
    PRINTF(L_CORE_MSGCACHE,"%d/%p: msg already queued. waiting to complete",getpid(),this);
    wait.Wait(mutex);
    }
  int id;
  if(!s) {
    while(1) {
      s=&caches[ptr];
      if(!(s->mode&QUEUED)) break;
      s->mode|=WAIT;
      PRINTF(L_CORE_MSGCACHE,"%d/%p: queue overwrite protection id=%d",getpid(),this,ptr+1);
      wait.Wait(mutex); // don't overwrite queued msg's
      }
    id=ptr+1;
    s->crc=crc;
    s->mode=QUEUED;
    PRINTF(L_CORE_MSGCACHE,"%d/%p: queued msg with id=%d",getpid(),this,id);
    ptr++; if(ptr>=numCache) { ptr=0; PRINTF(L_CORE_MSGCACHE,"msgcache: roll-over (%d)",numCache); }
    return id;
    }
  else {
    id=(s-&caches[0])+1;
    if(s->mode==GOOD) {
      if(store && storeSize>0)
        memcpy(store,&stores[(id-1)*storeSize],storeSize);
      PRINTF(L_CORE_MSGCACHE,"%d/%p: msg is cached as GOOD (%d)",getpid(),this,id);
      return 0;
      }
    else if(s->mode>=FAIL1 && s->mode<=FAIL2) {
      PRINTF(L_CORE_MSGCACHE,"%d/%p: msg is cached as FAIL%d (%d)",getpid(),this,s->mode,id);
      s->mode|=QUEUED;
      return id;
      }
    else {
      PRINTF(L_CORE_MSGCACHE,"%d/%p: msg is cached as FAILN (%d)",getpid(),this,id);
      return -1;
      }
    }
}

int cMsgCache::Cache(int id, bool result, const unsigned char *store)
{
  cMutexLock lock(&mutex);
  if(id<1 || !caches || (storeSize>0 && !stores)) return 0; // sanity
  struct Cache *s=&caches[id-1];
  LBSTARTF(L_CORE_MSGCACHE);
  LBPUT("%d/%p: de-queued msg with id=%d ",getpid(),this,id);
  if(s->mode&WAIT) wait.Broadcast();
  if(result) {
    if(store && storeSize>0)
      memcpy(&stores[(id-1)*storeSize],store,storeSize);
    s->mode=GOOD;
    LBPUT("(GOOD)");
    return 0;
    }
  else {
    int m=s->mode&MASK;
    if(m==GOOD)
      m=s->mode=FREE;
    if(m==FREE || (m>=FAIL1 && m<=FAIL2)) {
      s->mode=(s->mode&MASK)+1;
      if(s->mode<maxFail)
        LBPUT("(FAIL%d)",s->mode);
      else
        m=s->mode=FAILN;
      }
    if(m==FAILN)
      LBPUT("(FAILN)");
    return s->mode;
    }
  LBEND();
}

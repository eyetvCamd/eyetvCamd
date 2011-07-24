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

#include "../../../libsi/section.h"

#include "viaccess.h"
#include "log-viaccess.h"
#include "tps.h"
#include "opentv.h"
#include "st20.h"

#include "sc.h"
#include "scsetup.h"
#include "system-common.h"
#include "filter.h"
#include "misc.h"
#include "helper.h"

#define CHECK_TIME    5*60*1000
#define LOADBIN_TIME 60*60*1000
#define TPSAU_TIME   30*60*1000

#define TRUST_BOXTIME // should we trust the local time on this machine?
//#define DUMP_TPSAU "/var/tmp"

#ifdef DUMP_TPSAU
#include <unistd.h>
#endif

// -- cRC6 ---------------------------------------------------------------------

/*
 * This code implements the RC6-32/20 block cipher.
 *
 * The algorithm is due to Ron Rivest and RSA Labs. This code is based on code
 * which was written by Martin Hinner <mhi@penguin.cz> in 1999, no copyright is
 * claimed.
 */

#define RC6_WORDSIZE	32
#define RC6_P32		0xB7E15163L
#define RC6_Q32		0x9E3779B9L

unsigned int cRC6::rol(unsigned int v, unsigned int cnt)
{
  cnt&=(RC6_WORDSIZE-1);
  return (v<<cnt) | (v>>(RC6_WORDSIZE-cnt));
}

unsigned int cRC6::ror(unsigned int v, unsigned int cnt)
{
  cnt&=(RC6_WORDSIZE-1);
  return (v>>cnt) | (v<<(RC6_WORDSIZE-cnt));
}

void cRC6::SetKey(const unsigned char *Key, int len)
{
  key[0]=RC6_P32;
  for(int v=1; v<RC6_MAX; v++) key[v]=key[v-1]+RC6_Q32;
  len/=4;
  unsigned int a=0, b=0, *l=AUTOARRAY(unsigned int,len);
  memcpy(l,Key,len*4);
  for(int i=0,j=0,v=3*(len>RC6_MAX ? len : RC6_MAX) ; v>0; v--) {
    a=key[i]=rol(key[i]+a+b,3);
    b=  l[j]=rol(  l[j]+a+b,a+b);
    i++; i%=RC6_MAX;
    j++; j%=len;
    }
}

void cRC6::Decrypt(unsigned char *data)
{
  unsigned int *l=(unsigned int *)data;
  unsigned int a, b, c, d;
  a=l[0]-key[RC6_MAX-2];
  b=l[1];
  c=l[2]-key[RC6_MAX-1];
  d=l[3];
  for(int i=RC6_ROUNDS; i>0; i--) {
    unsigned int t=a;
    unsigned int u=b;
    a=d; d=c; b=t; c=u;
    u=rol((d*(2*d+1)),5);
    t=rol((b*(2*b+1)),5);
    c=ror(c-key[2*i+1],t)^u;
    a=ror(a-key[2*i  ],u)^t;
    }
  l[0]=a;
  l[1]=b-key[0];
  l[2]=c;
  l[3]=d-key[1];
}

// -- cTransponderTime ---------------------------------------------------------

class cTransponderTime : public cSimpleItem {
private:
  time_t sattime;
  cTimeMs reftime;
  cMutex mutex;
  cCondVar wait;
  int source, transponder;
  bool hasHandler;
public:
  cTransponderTime(int Source, int Transponder);
  bool Is(int Source, int Transponder) const;
  void Set(time_t st);
  time_t Now(void);
  void SetHasHandler(bool on) { hasHandler=on; }
  bool HasHandler(void) const { return hasHandler; }
  };

cTransponderTime::cTransponderTime(int Source, int Transponder)
{
  source=Source; transponder=Transponder;
#ifdef TRUST_BOXTIME
  sattime=time(0);
#else
  sattime=-1;
#endif
  hasHandler=false;
}

bool cTransponderTime::Is(int Source, int Transponder) const
{
  return source==Source && transponder==Transponder;
}

void cTransponderTime::Set(time_t st)
{
  cMutexLock lock(&mutex);
  if(sattime<0 || reftime.Elapsed()>5*1000) {
    reftime.Set(-100);
    sattime=st;
    wait.Broadcast();

    char str[32];
    struct tm utm;
    st=Now();
    time_t now=time(0);
    gmtime_r(&st,&utm); asctime_r(&utm,str); stripspace(str);
    PRINTF(L_SYS_TIME,"%x:%x: %s (delta %ld)",source,transponder,str,st-now);
    }
}

time_t cTransponderTime::Now(void)
{
  cMutexLock lock(&mutex);
  if(sattime<0) {
    if(!wait.TimedWait(mutex,3*1000)) {
      PRINTF(L_SYS_TIME,"%x:%x: unsuccessfull wait",source,transponder);
      return -1;
      }
    }
  return sattime+(reftime.Elapsed()/1000);
}

// -- cSatTimeHook -------------------------------------------------------------

#ifndef TESTER

class cSatTimeHook : public cLogHook {
private:
  cTransponderTime *ttime;
public:
  cSatTimeHook(cTransponderTime *Ttime);
  ~cSatTimeHook();
  virtual void Process(int pid, unsigned char *data);
  };

cSatTimeHook::cSatTimeHook(cTransponderTime *Ttime)
:cLogHook(HOOK_SATTIME,"sattime")
{
  ttime=Ttime;
  pids.AddPid(0x14,0x71,0xff,0x03);
  ttime->SetHasHandler(true);
}

cSatTimeHook::~cSatTimeHook()
{
  ttime->SetHasHandler(false);
}

void cSatTimeHook::Process(int pid, unsigned char *data)
{
  if(data && ttime) {
    if(data[0]==0x70) { // TDT
      SI::TDT tdt(data,false);
      tdt.CheckParse();
      ttime->Set(tdt.getTime());
      }
    else if(data[0]==0x73) { // TOT
      SI::TOT tot(data,false);
      if(!tot.CheckCRCAndParse()) return;
      ttime->Set(tot.getTime());
      }
    }
}

#endif //TESTER

// -- cSatTime -----------------------------------------------------------------

class cSatTime {
private:
  static cMutex mutex;
  static cSimpleList<cTransponderTime> list;
  //
  int cardNum;
  cTransponderTime *ttime;
  //
  void CheckHandler(void);
public:
  cSatTime(int CardNum, int Source, int Transponder);
  time_t Now(void);
  };

cMutex cSatTime::mutex;
cSimpleList<cTransponderTime> cSatTime::list;

cSatTime::cSatTime(int CardNum, int Source, int Transponder)
{
  cardNum=CardNum;
  cMutexLock lock(&mutex);
  for(ttime=list.First(); ttime; ttime=list.Next(ttime))
    if(ttime->Is(Source,Transponder)) break;
  if(!ttime) {
    ttime=new cTransponderTime(Source,Transponder);
    if(ttime) list.Add(ttime);
    PRINTF(L_SYS_TIME,"%x:%x: created new transpondertime",Source,Transponder);
    }
  else PRINTF(L_SYS_TIME,"%x:%x: using existing transpondertime",Source,Transponder);
  CheckHandler();
}

time_t cSatTime::Now(void)
{
  CheckHandler();
  return ttime ? ttime->Now() : -1;
}

void cSatTime::CheckHandler(void)
{
  if(ttime) {
    if(!cSoftCAM::TriggerHook(cardNum,HOOK_SATTIME) && !ttime->HasHandler()) {
      cSatTimeHook *hook=new cSatTimeHook(ttime);
      cSoftCAM::AddHook(cardNum,hook);
      PRINTF(L_SYS_TIME,"added hook");
      }
    }
}

// -- cOpenTVModule ------------------------------------------------------------

class cOpenTVModule {
private:
  int id, modlen;
  unsigned char *mem;
  int received;
  info_header_t info;
  code_header_t code;
  data_header_t data;
  //
  bool Decompress(void);
  bool DoDecompress(unsigned char *out_ptr, const unsigned char *in_ptr, int dsize, int data_size, int usize);
  bool ParseSections(void);
  void Dump(void);
public:
  cOpenTVModule(const unsigned char *data);
  cOpenTVModule(int Id, const unsigned char *data, int len);
  ~cOpenTVModule();
  int AddPart(const unsigned char *data, int len);
  const info_header_t *InfoHdr(void) const { return &info; }
  const code_header_t *CodeHdr(void) const { return &code; }
  const data_header_t *DataHdr(void) const { return &data; }
  //
  inline static int Id(const unsigned char *data) { return UINT16_BE(&data[3]); }
  inline static int Offset(const unsigned char *data) { return UINT32_BE(&data[16]); }
  inline static int Length(const unsigned char *data) { return UINT32_BE(&data[20]); }
  };

cOpenTVModule::cOpenTVModule(const unsigned char *data)
{
  id=Id(data); modlen=Length(data);
  received=0;
  mem=MALLOC(unsigned char,modlen);
}

cOpenTVModule::cOpenTVModule(int Id, const unsigned char *data, int len)
{
  id=Id; modlen=received=len;
  mem=MALLOC(unsigned char,modlen);
  memcpy(mem,data,len);
  ParseSections();
}

cOpenTVModule::~cOpenTVModule()
{
  free(mem);
}

int cOpenTVModule::AddPart(const unsigned char *data, int slen)
{
  if(Id(data)==id) {
    int off=Offset(data);
    int mlen=Length(data);
    int rec=slen-28;
    if(mlen!=modlen || (off+rec)>mlen || !mem) {
      PRINTF(L_SYS_TPSAU,"length mismatch while adding to OpenTV module");
      return -1;
      }
    memcpy(&mem[off],data+24,rec);
    received+=rec;
    if(received==mlen) {
      if(!Decompress()) {
        PRINTF(L_SYS_TPSAU,"failed to decompress OpenTV module");
        return -1;
        }
      if(!ParseSections()) {
        PRINTF(L_SYS_TPSAU,"DATA & CODE section not located in OpenTV module");
        return -1;
        }
      Dump();
      return 1;
      }
    }
  return 0;
}

void cOpenTVModule::Dump(void)
{
#ifdef DUMP_TPSAU
#warning Dumping TPS AU data
  {
  char fname[32];
  snprintf(fname,sizeof(fname),"%s/decomp.bin.%d.%x",DUMP_TPSAU,getpid(),(int)time(0));
  int fd=open(fname,O_CREAT|O_TRUNC|O_WRONLY,DEFFILEMODE);
  if(fd>=0) {
    write(fd,mem,modlen);
    close(fd);
    PRINTF(L_SYS_TPSAU,"dumped to file '%s'",fname);
    }
  }
#endif
}

bool cOpenTVModule::ParseSections(void)
{
  int sections=0;
  for(int idx=0; idx<modlen;) {
    unsigned int hdr=UINT32_BE(mem+idx);
    unsigned int s_len=UINT32_BE(mem+idx+4);
    switch(hdr) {
      case INFO_SECTION_HDR:
	info.magic=hdr;
	info.bsssize=UINT32_BE(mem+idx+8);
	info.stacksize=UINT32_BE(mem+idx+12);
        sections|=1;
        break;
      case CODE_SECTION_HDR:
	code.magic=hdr;
	code.size=s_len-8;
	code.m_id=UINT32_BE(mem+idx+8);
	code.entry_point=UINT16_BE(mem+idx+12);
	code.end=UINT16_BE(mem+idx+14);
	code.code=mem+idx+8;
        sections|=2;
        break;
      case DATA_SECTION_HDR:
	data.magic=hdr;
	data.dlen=s_len-8;
	data.data=mem+idx+8;
        sections|=4;
        break;
      case SWAP_SECTION_HDR:
      case GDBO_SECTION_HDR:
        break;
      case LAST_SECTION_HDR:
      case 0:
        idx=modlen;
        break;
      case COMP_SECTION_HDR:
        PRINTF(L_GEN_DEBUG,"OpenTV module still compressed in ParseSections()");
        return 0;
      default:
        PRINTF(L_SYS_TPSAU,"unknown section hdr %08x (%c%c%c%c) in OpenTV module",hdr,(hdr>>24)&0xFF,(hdr>>16)&0xFF,(hdr>>8)&0xFF,hdr&0xFF);
        break;
      }
    idx+=s_len;
    }
  return sections>=6;
}

bool cOpenTVModule::Decompress(void)
{
  comp_header_t comp;
  comp.magic=UINT32_BE(mem);
  comp.csize=UINT32_BE(mem+4);
  comp.dsize=UINT32_BE(mem+8);
  comp.usize=UINT32_BE(mem+12);
  comp.end  =UINT8_BE(mem+16);
  if((COMP_SECTION_HDR!=comp.magic) || (comp.dsize<=comp.csize) || 
     (comp.usize>comp.dsize) || (comp.end>=1) || (comp.csize<=17))
    return true;
  unsigned char *decomp=MALLOC(unsigned char,comp.dsize);
  if(!decomp || !DoDecompress(decomp,mem+17,comp.dsize,comp.csize-17,comp.usize))
    return false;
  free(mem);
  mem=decomp; modlen=comp.dsize;
  return true;
}

#define BYTE() ((odd)?((in_ptr[0]&0x0F)|(in_ptr[1]&0xF0)):in_ptr[0])

#define NIBBLE(__v) \
  do { \
    odd^=1; \
    if(odd) __v=in_ptr[0]&0xF0; \
    else {  __v=(in_ptr[0]&0xF)<<4; in_ptr++; } \
    } while(0)

bool cOpenTVModule::DoDecompress(unsigned char *out_ptr, const unsigned char *in_ptr, int dsize, int data_size, int usize)
{
  if(usize==0) return false;
  const unsigned char *data_start=in_ptr;
  unsigned char *out_start=out_ptr;
  unsigned char *out_end=out_ptr+usize;
  int odd=0;
  while(1) {
    unsigned char mask=BYTE(); in_ptr++;
    for(int cnt=8; cnt>0; mask<<=1,cnt--) {
      if(mask&0x80) {
        out_ptr[0]=BYTE(); in_ptr++;
        out_ptr++;
        }
      else {
        int off=0, len=0;
        unsigned char cmd=BYTE(); in_ptr++;
        switch(cmd>>4) {
          case 0x0:
          case 0x1:
          case 0x2:
          case 0x3:
          case 0x4:
          case 0x5:
          case 0x6:
            off=((cmd&0xF)<<8)+BYTE(); in_ptr++;
            len=((cmd>>4)&0x7)+3;
            break;
          case 0x7:
            {
            unsigned char high=BYTE(); in_ptr++;
            off=((high&0x7F)<<8)+BYTE(); in_ptr++;
            if((cmd==0x7F) && (high&0x80)) {
              len=BYTE(); in_ptr++;
              if(len==0xFF) {
                len=BYTE(); in_ptr++;
                len=((len<<8)+BYTE()+0x121)&0xFFFF; in_ptr++;
                }
              else len+=0x22;
              }
            else {
              len=((cmd&0x0F)<<1)+3; if(high&0x80) len++;
              }
            break;
            }
          case 0x8:
          case 0x9:
          case 0xA:
          case 0xB:
            if(cmd&0x20) NIBBLE(off); else off=0;
            off=(off<<1)|(cmd&0x1F); len=2;
            break;
          case 0xC:
            off=cmd&0x0F; len=3;
            break;
          case 0xD:
          case 0xE:
          case 0xF:
            NIBBLE(off);
            off|=cmd&0x0F; len=((cmd>>4)&0x3)+2;
            break;
          }
        const unsigned char *from=out_ptr-(off+1);
        if(from<out_start || from>=out_ptr || len>(out_end-out_ptr)) {
          PRINTF(L_SYS_TPSAU,"length mismatch in OpenTV decompress");
          return false;
          }
        while(--len>=0) *out_ptr++=*from++;
        }
      if(out_end<=out_ptr) {
        if(out_end!=out_ptr) {
          PRINTF(L_SYS_TPSAU,"pointer mismatch in OpenTV decompress");
          return false;
          }
        int len=out_start+dsize-out_ptr;
        if(len>0) memmove(out_ptr,data_start+(data_size-(dsize-usize)),len);
        return true;
        }
      }
    }
}

#undef BYTE
#undef NIBBLE

// -- cTpsKey ------------------------------------------------------------------

cTpsKey::cTpsKey(void)
{
  timestamp=0;
}

void cTpsKey::Set(const cTpsKey *k)
{
  timestamp=k->timestamp;
  opmode=k->opmode;
  memcpy(step,k->step,sizeof(step));
}

void cTpsKey::Set(const unsigned char *mem)
{
  timestamp=*((unsigned int *)mem);
  opmode=mem[52+3];
  for(int i=0; i<3; i++) {
    memcpy(step[i].key,&mem[4+i*16],16);
    step[i].mode=mem[52+i];
    }
}

void cTpsKey::Put(unsigned char *mem) const
{
  *((unsigned int *)mem)=timestamp;
  mem[52+3]=opmode;
  for(int i=0; i<3; i++) {
    memcpy(&mem[4+i*16],step[i].key,16);
    mem[52+i]=step[i].mode;
    }
}

cString cTpsKey::ToString(bool hide)
{
  unsigned char tmp[60];
  Put(&tmp[4]);
  *((unsigned int *)tmp)=crc32_le(0,&tmp[4],sizeof(tmp)-4);
  char str[420];
  HexStr(str,tmp,sizeof(tmp));
  return str;
}

// -- cTpsAuHook ---------------------------------------------------------------

#ifndef TESTER

#define BUFF_SIZE 20000

class cTpsAuHook : public cLogHook {
private:
  cOpenTVModule *mod;
  int pmtpid, aupid;
public:
  cTpsAuHook(void);
  ~cTpsAuHook();
  virtual void Process(int pid, unsigned char *data);
  };

#endif //TESTER

// -- cTpsKeys -----------------------------------------------------------------

cTpsKeys tpskeys;

cTpsKeys::cTpsKeys(void)
:cStructListPlain<cTpsKey>("TPS keys","tps.cache",SL_READWRITE|SL_MISSINGOK|SL_WATCH|SL_NOPURGE)
,lastLoad()
,lastAu()
{
  lastLoad.Set(-LOADBIN_TIME);
  lastAu.Set(-TPSAU_TIME);
  first=last=0; algomem=0; loadlist=0;
}

cTpsKeys::~cTpsKeys()
{
  free(algomem);
  delete loadlist;
}

const cTpsKey *cTpsKeys::GetKey(time_t t)
{
  ListLock(false);
  cTpsKey *k;
  for(k=First(); k; k=Next(k)) if(t<k->Timestamp()) break;
  ListUnlock();
  return k;
}

const cTpsKey *cTpsKeys::GetV2Key(int id)
{
  unsigned char tmp[56];
  memset(tmp,0,sizeof(tmp));
  cPlainKey *pk=keys.FindKey('V',id,MBC3('T','P','S'),16,0);
  if(pk) {
    pk->Get(&tmp[4+2*16]);
    tmp[52+2]=1;
    tmp[52+3]=0x1C;
    cTpsKey *tk=new cTpsKey;
    if(tk) {
      tk->Set(tmp);
      return tk;
      }
    }
  else PRINTF(L_SYS_KEY,"missing %.4x TPS key\n",id);
  return 0;
}

void cTpsKeys::Check(time_t now, int cardnum)
{
  checkMutex.Lock();
  if(first==0 && last==0 && Count()>0)
    GetFirstLast();
  if(now>0 && lastCheck.Elapsed()>CHECK_TIME) {
    Purge(now);
    lastCheck.Set();
    }
  bool nokey=now+2*3600>last;
/*
  if(lastLoad.Elapsed()>(nokey ? LOADBIN_TIME/60 : LOADBIN_TIME)) {
    PRINTF(L_SYS_TPSAU,"loading "TPSBIN" triggered");
    LoadBin();
    if(now>0) Purge(now);
    lastLoad.Set();
    }
*/
  if(lastAu.Elapsed()>(nokey ? TPSAU_TIME/60 : TPSAU_TIME)) {
    if(ScSetup.AutoUpdate>0) {
      PRINTF(L_SYS_TPSAU,"TPS AU triggered");
      if(!cSoftCAM::TriggerHook(cardnum,HOOK_TPSAU)) {
        cTpsAuHook *hook=new cTpsAuHook;
        cSoftCAM::AddHook(cardnum,hook);
        PRINTF(L_SYS_TPSAU,"TPS AU hook added");
        }
      }
    lastAu.Set();
    }
  checkMutex.Unlock();
}

void cTpsKeys::Purge(time_t now)
{
  PRINTF(L_SYS_TPSAU,"purging TPS keylist");
  bool del=false;
  ListLock(true);
  for(cTpsKey *k=First(); k;) {
    cTpsKey *n=Next(k);
    if(k->Timestamp()<now-3600) { Del(k); del=true; }
    k=n;
    }
  ListUnlock();
  if(del) {
    GetFirstLast();
    Modified();
    }
}

void cTpsKeys::Join(cSimpleList<cTpsKey> *nlist)
{
  ListLock(true);
  cTpsKey *k;
  while((k=nlist->First())) {
    nlist->Del(k,false);
    cTpsKey *p=First();
    do {
      if(!p) {
        Add(k);
        Modified();
        break;
        }
      cTpsKey *n=Next(p);
      if(k->Timestamp()==p->Timestamp()) {
        p->Set(k);
        Modified();
        delete k;
        break;
        }
      if(k->Timestamp()>p->Timestamp() && (!n || k->Timestamp()<n->Timestamp())) {
        Add(k,p);
        Modified();
        break;
        }
      p=n;
      } while(p);
    }
  ListUnlock();
  delete nlist;
  GetFirstLast();
}

cString cTpsKeys::Time(time_t t)
{
  char str[32];
  struct tm tm_r;
  strftime(str,sizeof(str),"%b %e %T",localtime_r(&t,&tm_r));
  return str;
}

void cTpsKeys::GetFirstLast(void)
{
  if(Count()>0) {
    ListLock(false);
    cTpsKey *k=First();
    first=last=k->Timestamp();
    for(; k; k=Next(k)) {
      if(k->Timestamp()<last)
        PRINTF(L_SYS_TPSAU,"TPS keys not in accending order!");
      last=k->Timestamp();
      }
    PRINTF(L_SYS_TPS,"%d TPS keys available (from %s to %s)",Count(),*Time(first),*Time(last));
    ListUnlock();
    }
  else {
    last=first=0;
    PRINTF(L_SYS_TPS,"no TPS keys available");
    }
}

bool cTpsKeys::ProcessAu(const cOpenTVModule *mod)
{
  PRINTF(L_SYS_TPSAU,"processing TPS AU data");

  const code_header_t *codehdr=mod->CodeHdr();
  const data_header_t *datahdr=mod->DataHdr();
  const unsigned char *c=codehdr->code;
  const unsigned char *d=datahdr->data;
  unsigned int kd=0, cb1=0, cb2=0, cb3=0;
  for(unsigned int i=0; i<codehdr->size; i++) {
    if(c[i]==0x81) { // PushEA DS:$xxxx
      unsigned int addr=(c[i+1]<<8)|c[i+2];
      if(addr<(datahdr->dlen-3)) {
        if(d[addr]==0x79 && d[addr+1]==0x00 && d[addr+2]==0x79 && d[addr+3]==0x00)
          kd=addr;
        else if(d[addr]==0x73 && d[addr+1]==0x25 && d[addr+2]==0xFA)
          cb1=addr;
        else if(d[addr]==0x64 && (d[addr+1]&0xB0)==0xB0 && d[addr+2]==0x24)
          cb2=addr;
        else if((d[addr]&0x60)==0x60 && (d[addr+1]&0xB0)==0xB0 && (d[addr+2]&0x20)==0x20)
          cb3=addr;
/*
        else if(d[addr]==0x73 && d[addr+1]==0x25) {
          static const unsigned char scan1[] = { 0x28, 0x20, 0x20, 0xC0 };
          for(int j=2; j < 0xC; j++)
            if(!memcmp(&d[addr+j],scan1,sizeof(scan1))) { cb1=addr; break; }
          }
        else if(cb1 && !cb2) cb2=addr;
        else if(cb1 && cb2 && !cb3) cb3=addr;
*/
        }
      }
    }
  if(!kd || !cb1 || !cb2 || !cb3) {
    PRINTF(L_SYS_TPSAU,"couldn't locate all pointers in data section (%d,%d,%d,%d)",kd,cb1,cb2,cb3);
    return false;
    }

  unsigned int end=(kd>cb1 && kd>cb2 && kd>cb3) ? kd : datahdr->dlen;
  unsigned int off=min(cb1,min(cb2,cb3))-2;
  PRINTF(L_SYS_TPSAU,"pointers in data section kd=%d cb1=%d cb2=%d cb3=%d - dlen=%d off=%d end=%d - cb1=%d cb2=%d cb3=%d",kd,cb1,cb2,cb3,datahdr->dlen,off,end,cb1-off,cb2-off,cb3-off);
  RegisterAlgo3(d+off,cb1-off,cb2-off,cb3-off,end-off);

  const unsigned char *data=&d[kd];
  int seclen, numkeys;
  seclen=data[0] | (data[1]<<8);
  numkeys=data[2] | (data[3]<<8);
  int algo=data[4];
  int mkidx=data[5]&7;
  unsigned char *sec[7];
  sec[0]=(unsigned char *)data+6;
  for(int i=1; i<6; i++) sec[i]=sec[i-1]+seclen;
  sec[6]=sec[5]+numkeys;
  unsigned char key[16];
  cPlainKey *pk=keys.FindKey('V',0x7c00,MBC3('M','K',mkidx),16);
  if(!pk) {
    PRINTF(L_SYS_KEY,"missing V 7C00 TPSMK%d key",mkidx);
    return false;
    }
  pk->Get(key);

  if(sec[6]>=d+datahdr->dlen) {
    PRINTF(L_SYS_TPSAU,"section 5 exceeds buffer");
    return false;
    }
  int keylen=0;
  for(int i=0; i<numkeys; i++) keylen+=sec[5][i];
  keylen=(keylen+15)&~15;
  if(sec[6]+keylen>=d+datahdr->dlen) {
    PRINTF(L_SYS_TPSAU,"section 6 exceeds buffer");
    return false;
    }
  for(int i=0; i<keylen; i+=16) TpsDecrypt(&sec[6][i],algo,key);

  cSimpleList<cTpsKey> *nlist=new cSimpleList<cTpsKey>;
  for(int i=0; i<seclen; i++) {
    static const unsigned char startkey[] = { 0x01,0x01 };
    static const unsigned char startaes[] = { 0x09,0x10 };
    static const unsigned char startse[] = { 0x0a,0x10 };
    unsigned char tmp[56];
    tmp[0]=sec[0][i];
    tmp[1]=sec[1][i];
    tmp[2]=sec[2][i];
    tmp[3]=sec[3][i];
    if(CheckFF(tmp,4)) continue;
    int keyid=sec[4][i];
    int keylen=sec[5][keyid];
    if(keylen<32) continue;
    const unsigned char *tkey=sec[6];
    for(int j=0; j<keyid; j++) tkey+=sec[5][j];

    unsigned char ke[128];
    if(keylen!=45) {
      if(!Handle80008003(tkey,keylen,ke)) continue;
      tkey=ke;
      }

    if(memcmp(tkey,startkey,sizeof(startkey))) continue;
    tmp[52]=0;
    tmp[53]=tkey[5]; //tkey[4];
    tmp[54]=1;       //tkey[5];
    tmp[55]=0x1c;
    tkey+=9;
    if(memcmp(tkey,startaes,sizeof(startaes))) continue;
    memset(&tmp[4+ 0],0,16);
    memcpy(&tmp[4+16],&tkey[2],16);
    tkey+=18;
    if(memcmp(tkey,startse,sizeof(startse))) continue;
    memcpy(&tmp[4+32],&tkey[2],16);
    cTpsKey *k=new cTpsKey;
    if(k) { k->Set(tmp); nlist->Add(k); }
    }
  PRINTF(L_SYS_TPSAU,"got %d keys from AU data",nlist->Count());
  bool res=nlist->Count()>0;
  Join(nlist);
  return res;
}

/*
bool cTpsKeys::LoadBin(void)
{
  static const unsigned char mark[] = { 'T','P','S',0 };
  cFileMap *tpsbin=filemaps.GetFileMap(TPSBIN,FILEMAP_DOMAIN,false);
  if(!tpsbin) {
    PRINTF(L_SYS_TPS,"no filemap for "TPSBIN);
    return false;
    }
  else if(!tpsbin->Map()) {
    PRINTF(L_SYS_TPS,"mapping failed for "TPSBIN);
    return false;
    }
  else if(tpsbin->Size()<65536 || memcmp(mark,tpsbin->Addr()+32,sizeof(mark)) || memcmp(mark,tpsbin->Addr()+48,sizeof(mark))) {
    PRINTF(L_SYS_TPS,TPSBIN" format not recognised");
    tpsbin->Unmap();
    return false;
    }

  int size=tpsbin->Size()-56;
  cSimpleList<cTpsKey> *nlist=new cSimpleList<cTpsKey>;
  for(int i=68; i<size; i+=56) {
    const unsigned char *a=tpsbin->Addr()+i;
    if(*((const unsigned int *)a)==0x00000000L || *((const unsigned int *)a)==0xFFFFFFFFL)
      break;
    unsigned char tmp[56];
    DecryptBin(a,tmp);
    cTpsKey *k=new cTpsKey;
    if(k) { k->Set(tmp); nlist->Add(k); }
    }
  tpsbin->Unmap();
  PRINTF(L_SYS_TPSAU,"loaded %d keys from "TPSBIN" file",nlist->Count());
  Join(nlist);
  return true;
}

void cTpsKeys::DecryptBin(const unsigned char *in, unsigned char *out)
{
  unsigned int var2=*((unsigned int *)in);
  *((unsigned int *)out)=var2;
  for(int i=0; i<13; i++) {
    in+=4; out+=4;
    var2=(var2<<3) | (var2>>(32-3));
    unsigned int var1=*((unsigned int *)in) ^ var2;
    *((unsigned int *)out)=(var1<<(i+2)) | (var1>>(32-(i+2)));
    }
}
*/

void cTpsKeys::PreLoad(void)
{
  delete loadlist;
  loadlist=new cSimpleList<cTpsKey>;
  if(!loadlist) PRINTF(L_SYS_TPS,"no memory for loadlist");
}

void cTpsKeys::PostLoad(void)
{
  if(loadlist) { Join(loadlist); loadlist=0; }
}

bool cTpsKeys::ParseLinePlain(const char *line)
{
  unsigned char tmp[60];
  if(line[0]=='X') {
    if(line[1]=='S') {
      if(algomem) PRINTF(L_SYS_TPS,"multiple start extentions during cache load");
      if(sscanf(&line[2],"%x %x %x %x",&algolen,&cb1off,&cb2off,&cb3off)==4) {
        free(algomem);
        if((algomem=MALLOC(unsigned char,algolen))) {
          algoread=0;
          return true;
          }
        else PRINTF(L_SYS_TPS,"no memory for algo");
        }
      else PRINTF(L_SYS_TPS,"bad format in start extention");
      }
    else if(line[1]=='C') {
      if(algomem) {
        int off, len;
        unsigned int crc;
        if(sscanf(&line[2],"%x %x %n",&off,&crc,&len)==2) {
          line+=len+2;
          unsigned char buff[210];
          if((len=GetHex(line,buff,200,false))) {
            if(crc==crc32_le(0,buff,len) && off>=0 && off+len<=algolen) {
              memcpy(&algomem[off],buff,len);
              algoread+=len;
              if(algoread==algolen) {
                RegisterAlgo3(algomem,cb1off,cb2off,cb3off,algolen);
                free(algomem); algomem=0;
                }
              return true;
              }
            }
          }
        PRINTF(L_SYS_TPS,"bad format in code extention");
        }
      else PRINTF(L_SYS_TPS,"unexpected code extention");
      }
    else PRINTF(L_SYS_TPS,"unknown extention during cache load");
    }
  else {
    if(GetHex(line,tmp,sizeof(tmp))) {
      unsigned int crc=crc32_le(0,&tmp[4],sizeof(tmp)-4);
      if(*((unsigned int *)tmp)==crc) {
        cTpsKey *k=new cTpsKey;
        if(k) { k->Set(&tmp[4]); if(loadlist) loadlist->Add(k); }
        return true;
        }
      else PRINTF(L_SYS_TPS,"CRC failed during cache load");
      }
    }
  return false;
}

void cTpsKeys::PostSave(FILE *f)
{
  char str[420];
  unsigned char *mem;
  int len=0, cb1=0, cb2=0, cb3=0;
  if((mem=DumpAlgo3(len,cb1,cb2,cb3))) {
    fprintf(f,"XS %04X %04X %04X %04X\n",len,cb1,cb2,cb3);
    for(int i=0; i<len; i+=200) {
      int l=min(200,len-i);
      fprintf(f,"XC %04X %08X %s\n",i,crc32_le(0,&mem[i],l),HexStr(str,&mem[i],l));
      }
    free(mem);
    }
}

// -- cTpsAuHook ---------------------------------------------------------------

#ifndef TESTER

#define AUSID 0x12C0

cTpsAuHook::cTpsAuHook(void)
:cLogHook(HOOK_TPSAU,"tpsau")
{
  mod=0; pmtpid=aupid=-1;
  pids.AddPid(0x0000,0x00,0xff); // PAT
}

cTpsAuHook::~cTpsAuHook()
{
  delete mod;
}

void cTpsAuHook::Process(int pid, unsigned char *data)
{
  if(data) {
    if(pid==0) { // PAT
      SI::PAT pat(data,false);
      if(pat.CheckCRCAndParse()) {
        SI::PAT::Association assoc;
        for(SI::Loop::Iterator it; pat.associationLoop.getNext(assoc,it);) {
          if(!assoc.isNITPid() && assoc.getServiceId()==AUSID) {
            pmtpid=assoc.getPid();
            PRINTF(L_SYS_TPSAU,"got PMT pid %04x for SID %04x",pmtpid,AUSID);
            cPid *pid=pids.First();
            if(pid && pid->filter) {
              pid->filter->Start(pmtpid,0x02,0xFF,0x00,false);
              pid->filter->Flush();
              }
            else PRINTF(L_GEN_DEBUG,"internal: no pid/filter in cTpsAuHook/pat");
            return;
            }
          }
        PRINTF(L_SYS_TPSAU,"no PMT pid found for SID %04x",AUSID);
        BailOut();
        }
      }
    else if(pid==pmtpid) { // PMT
      SI::PMT pmt(data,false);
      if(pmt.CheckCRCAndParse() && pmt.getServiceId()==AUSID) {
        SI::PMT::Stream stream;
        for(SI::Loop::Iterator it; pmt.streamLoop.getNext(stream,it); ) {
          if(stream.getStreamType()==0x05) {
            aupid=stream.getPid();
            PRINTF(L_SYS_TPSAU,"got AU pid %04x",aupid);
            cPid *pid=pids.First();
            if(pid && pid->filter) {
              pid->filter->Start(aupid,0x87,0xFF,0x00,false);
              pid->filter->Flush();
              }
            else PRINTF(L_GEN_DEBUG,"internal: no pid/filter in cTpsAuHook/pmt");
            return;
            }
          }
        PRINTF(L_SYS_TPSAU,"could not locate AU pid in PMT %04x data",pmtpid);
        BailOut();
        }
      }
    else if(pid==aupid) {
      if(cOpenTVModule::Id(data)==2) {
        if(!mod) mod=new cOpenTVModule(data);
        if(mod) {
          int r=mod->AddPart(data,SCT_LEN(data));
          if(r>0) {
            PRINTF(L_SYS_TPSAU,"received complete OpenTV module ID 2");
            if(tpskeys.ProcessAu(mod)) BailOut();
            r=-1;
            }            
          if(r<0) { delete mod; mod=0; }
          }
        }
      }
    }
}

#endif //TESTER

// -- cTPSDecrypt --------------------------------------------------------------

unsigned char *cTPSDecrypt::mem=0;
int cTPSDecrypt::memLen=0;
int cTPSDecrypt::cb1off=0;
int cTPSDecrypt::cb2off=0;
int cTPSDecrypt::cb3off=0;
cMutex cTPSDecrypt::st20Mutex;
cST20 cTPSDecrypt::st20;
bool cTPSDecrypt::st20Inited=false;

void cTPSDecrypt::TpsDecrypt(unsigned char *data, short mode, const unsigned char *key)
{
  switch(mode) {
    case 0: break;
    case 1: cAES::SetKey(key); cAES::Decrypt(data,16); break;
    case 2: cRC6::SetKey(key,16); cRC6::Decrypt(data); break;
    case 3: if(mem) {
              if(!DecryptAlgo3(key,data))
                PRINTF(L_SYS_TPS,"decrypt failed in algo 3");
              }
            else PRINTF(L_SYS_TPS,"no callbacks for algo 3 registered");
            break;
    default: PRINTF(L_SYS_TPS,"unknown TPS decryption algo %d",mode); break;
    }
}

bool cTPSDecrypt::RegisterAlgo3(const unsigned char *data, int cb1, int cb2, int cb3, int len)
{
  cMutexLock lock(&st20Mutex);
  free(mem);
  if(!(mem=MALLOC(unsigned char,len))) return false;
  memcpy(mem,data,len);
  memLen=len; cb1off=cb1; cb2off=cb2; cb3off=cb3;
  st20Inited=false;
  PRINTF(L_SYS_TPSAU,"registered callbacks for algo 3");
  return true;
}

unsigned char *cTPSDecrypt::DumpAlgo3(int &len, int &cb1, int &cb2, int &cb3)
{
  cMutexLock lock(&st20Mutex);
  if(!mem) return 0;
  unsigned char *buff=MALLOC(unsigned char,memLen);
  if(!buff) return 0;
  memcpy(buff,mem,memLen);
  len=memLen; cb1=cb1off; cb2=cb2off; cb3=cb3off;
  return buff;
}

bool cTPSDecrypt::InitST20(void)
{
  if(!mem) return false;
  if(!st20Inited) {
    st20.SetFlash(mem,memLen);
    st20.SetRam(NULL,0x10000);
    st20Inited=true;
    }
  return true;
}

bool cTPSDecrypt::Handle80008003(const unsigned char *src, int len, unsigned char *dest)
{
  cMutexLock lock(&st20Mutex);
  if(cb1off && InitST20()) {
    for(int i=0; i<len; i++) st20.WriteByte(RAMS+0x400+i,src[i]);
    st20.WriteShort(RAMS+0x0,0x8000);
    st20.WriteShort(RAMS+0x2,0x8003);
    st20.WriteWord(RAMS+0x8,RAMS+0x400);
    st20.Init(FLASHS+cb1off,RAMS+0xF000);
    st20.SetCallFrame(0,RAMS,0,0);
    int err=st20.Decode(1000);
    if(err<0) {
      PRINTF(L_SYS_TPSAU,"ST20 processing failed in callback1 (%d)",err);
      return false;
      }
    for(int i=0; i<0x2D; i++) dest[i]=st20.ReadByte(RAMS+0x400+i);
    return true;
    }
  return false;
}

bool cTPSDecrypt::DecryptAlgo3(const unsigned char *key, unsigned char *data)
{
  cMutexLock lock(&st20Mutex);
  if(cb2off && cb3off && InitST20()) {
    for(int i=0; i<16; i++) st20.WriteByte(RAMS+0x400+i,key[i]);
    st20.Init(FLASHS+cb2off,RAMS+0xF000);
    st20.SetCallFrame(0,RAMS+0x400,RAMS+0x800,0);
    int err=st20.Decode(30000);
    if(err<0) {
      PRINTF(L_SYS_TPS,"ST20 processing failed in callback2 (%d)",err);
      return false;
      }

    for(int i=0; i<16; i++) st20.WriteByte(RAMS+0x400+i,data[i]);
    st20.Init(FLASHS+cb3off,RAMS+0xF000);
    st20.SetCallFrame(0,RAMS+0x400,RAMS+0x1000,RAMS+0x800);
    err=st20.Decode(40000);
    if(err<0) {
      PRINTF(L_SYS_TPS,"ST20 processing failed in callback3 (%d)",err);
      return false;
      }
    for(int i=0; i<16; i++) data[i]=st20.ReadByte(RAMS+0x1000+i);
    return true;
    }
  return false;
}

// -- cTPS ---------------------------------------------------------------------

cTPS::cTPS(void)
{
  sattime=0;
}

cTPS::~cTPS()
{
  delete sattime;
}

int cTPS::Decrypt(int cardNum, int Source, int Transponder, unsigned char *data, int len)
{
  if(!sattime) {
    sattime=new cSatTime(cardNum,Source,Transponder);
    if(!sattime) {
      PRINTF(L_SYS_TPS,"failed to create time class");
      return -1;
      }
    }
  time_t now=sattime->Now();
  tpskeys.Check(now,cardNum);
  if(now<0) return -1;

  const cTpsKey *k=tpskeys.GetKey(now);
  doPost=0;
  int opmode=k?k->Opmode():4, doTPS=0, doPre=0, hasDF=0, ret=0;
  if((opmode&4) && data[0]==0xD2 && data[1]==0x01 && data[2]==0x01) {
    data+=3; len-=3; ret=3;
    if(data[0]==0x90) PRINTF(L_SYS_TPS,"TPS v1 is no longer supported");
    if(data[0]==0x40) data[0]=0x90;
    doTPS=1;
    if(!k) k=tpskeys.GetV2Key(0x7c00);
    }
  for(int i=0; i<len; i+=data[i+1]+2) {
    if(!k && (doTPS || doPre || doPost)) {
      PRINTF(L_SYS_TPS,"no TPS key available for current time of day");
      return -1;
      }
    switch(data[i]) {
      case 0xDF:
        if(!(opmode&4)) doTPS =(0x6996>>((data[i+2]&0xF)^(data[i+2]>>4)))&1;
        if(opmode&8)    doPre =(0x6996>>((data[i+3]&0xF)^(data[i+3]>>4)))&1;
        if(opmode&16)   doPost=(0x6996>>((data[i+4]&0xF)^(data[i+4]>>4)))&1;
        hasDF=1;
        break;
      case 0xEA:
        if(doPre) TpsDecrypt(&data[i+2],k->Mode(0),k->Key(0));
        if(doTPS) TpsDecrypt(&data[i+2],(hasDF)?k->Mode(2):1,k->Key(2));
        if(doPost) { postMode=k->Mode(1); memcpy(postKey,k->Key(1),sizeof(postKey)); }
        break;
      }
    }
  return ret;
}

void cTPS::PostProc(unsigned char *cw)
{
  if(doPost) TpsDecrypt(cw,postMode,postKey);
}

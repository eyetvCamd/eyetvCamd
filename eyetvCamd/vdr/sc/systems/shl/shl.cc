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

#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <malloc.h>
#include <sys/ioctl.h>
#include <sys/poll.h>
#include <linux/dvb/dmx.h>

#include <openssl/des.h>

#include "system-common.h"
#include "data.h"
#include "filter.h"
#include "misc.h"
#include "log-sys.h"

#define SYSTEM_SHL           0x4A60

#define SYSTEM_NAME          "@SHL"
#define SYSTEM_PRI           -10
#define SYSTEM_CAN_HANDLE(x) ((x)==SYSTEM_SHL)

#define L_SYS        15
#define L_SYS_HOPPER LCLASS(L_SYS,L_SYS_LASTDEF<<1)
#define L_SYS_HSTATS LCLASS(L_SYS,L_SYS_LASTDEF<<2)
#define L_SYS_ALL    LALL(L_SYS_HSTATS)

static const struct LogModule lm_sys = {
  (LMOD_ENABLE|L_SYS_ALL)&LOPT_MASK,
  (LMOD_ENABLE|L_SYS_DEFDEF)&LOPT_MASK,
  "shl",
  { L_SYS_DEFNAMES,"pidHopper","hopperStats" }
  };
ADD_MODULE(L_SYS,lm_sys)

// evil hack, but it's not worth to put much effort in a dead system ...
static cTimeMs __time;
#define time_ms() ((int)__time.Elapsed())

static cPlainKeyTypeReg<cPlainKeyStd,'Z',false> KeyReg;

// -- cPesFilter ---------------------------------------------------------------

class cPesFilter : public cPidFilter {
public:
  cPesFilter(const char *Id, int Num, int DvbNum, int IdleTime);
  virtual void Start(int Pid, int Section, int Mask, int Mode, bool Crc);
  };

cPesFilter::cPesFilter(const char *Id, int Num, int DvbNum, int IdleTime)
:cPidFilter(Id,Num,DvbNum,IdleTime)
{}

void cPesFilter::Start(int Pid, int Section, int Mask, int Mode, bool Crc)
{
  if(fd>=0) {
    Stop();
    struct dmx_pes_filter_params FilterParams;
    FilterParams.input=DMX_IN_FRONTEND;
    FilterParams.output=DMX_OUT_TAP;
    FilterParams.pes_type=DMX_PES_OTHER;
    FilterParams.flags=DMX_IMMEDIATE_START;
    FilterParams.pid=Pid;
    CHECK(ioctl(fd,DMX_SET_PES_FILTER,&FilterParams));
    pid=Pid;
    active=true;
    }
}

// -- cPidHopper ---------------------------------------------------------------

struct FilterPacket {
  struct FilterPacket *next;
  int lastTime, dataLen;
  unsigned char lastData[1];
  };
  
struct FilterData {
  cPidFilter *filter;
  int lastTime, lastRead, createTime, readCount;
  struct FilterPacket *first, *last;
  };

class cPidHopper : public cAction {
private:
  struct FilterData *d;
  int numFilters;
  cMutex sleepMutex;
  cCondVar sleep;
  //
  void PurgeFilter(struct FilterData *dd);
  void PurgePacket(struct FilterData *dd, struct FilterPacket *p);
protected:
  virtual void Process(cPidFilter *filter, unsigned char *data, int len);
  virtual cPidFilter *CreateFilter(const char *Id, int Num, int DvbNum, int IdleTime);
public:
  cPidHopper(int DvbNum, int NumFilters);
  virtual ~cPidHopper();
  int Read(int pid, int & now, unsigned char *buff, int len, int timeout);
  bool AddPid(int pid);
  void CutOff(int now);
  };

cPidHopper::cPidHopper(int DvbNum, int NumFilters)
:cAction("pidhopper",DvbNum)
{
  numFilters=NumFilters;
  d=MALLOC(struct FilterData,NumFilters);
  memset(d,0,sizeof(struct FilterData)*NumFilters);
  PRINTF(L_SYS_HOPPER,"pid hopper started (%d filters)",numFilters);
}

cPidHopper::~cPidHopper()
{
  Lock();
  int now=time_ms(), num=1;
  PRINTF(L_SYS_HSTATS,"active pids: (now=%d)\n",now);
  for(int i=0; i<numFilters; i++) {
    struct FilterData *dd=&d[i];
    if(dd->filter) {
      if(dd->filter->Pid()>=0)
        PRINTF(L_SYS_HSTATS,"%2d. pid %04x lastTime=%4d lastRead=%4d created=%4d count=%4d",
                num++,
                dd->filter->Pid(),
                dd->lastTime>0 ? (now-dd->lastTime)/1000 : -1,
                dd->lastRead>0 ? (now-dd->lastRead)/1000 : -1,
                dd->createTime>0 ? (now-dd->createTime)/1000 : -1,
                dd->readCount);
      DelFilter(dd->filter);
      dd->filter=0;
      }
    PurgeFilter(dd);
    }
  Unlock();
  free(d);
}

void cPidHopper::PurgeFilter(struct FilterData *dd)
{
  if(dd->first && dd->last) PurgePacket(dd,dd->last);
}

void cPidHopper::PurgePacket(struct FilterData *dd, struct FilterPacket *p)
{
  while(dd->first) {
    struct FilterPacket *del=dd->first;
    dd->first=del->next;
    if(dd->filter) PRINTF(L_SYS_HOPPER,"PID %04x purging packet %p (time=%d)",dd->filter->Pid(),del,del->lastTime);
    free(del);
    if(del==p) break;
    }
  if(!dd->first) dd->last=0;
}

void cPidHopper::CutOff(int now)
{
  Lock();
  PRINTF(L_SYS_HOPPER,"cutoff (now=%d)",now);
  for(int i=0; i<numFilters; i++) {
    struct FilterData *dd=&d[i];
    struct FilterPacket *p=dd->first, *del=0;
    while(p) {
      if(p->lastTime>=now) break;
      del=p;
      p=p->next;
      }
    if(del) {
      PurgePacket(dd,del);
      dd->lastTime=dd->first ? dd->first->lastTime : -1;
      }
    }
  Unlock();
}

cPidFilter *cPidHopper::CreateFilter(const char *Id, int Num, int DvbNum, int IdleTime)
{
  cPidFilter *filter=new cPesFilter(Id,Num,DvbNum,IdleTime);
  d[Num].filter=filter;
  d[Num].lastTime=-1;
  d[Num].lastRead=-1;
  d[Num].createTime=time_ms();
  d[Num].readCount=0;
  d[Num].first=0;
  d[Num].last=0;
  return filter;
}

void cPidHopper::Process(cPidFilter *filter, unsigned char *data, int len)
{
  if(data && len>0) {
    int now=time_ms();
    for(int i=0; i<numFilters; i++)
      if(d[i].filter==filter) {
        do {
          int l=min(len,184);
          struct FilterPacket *p=(struct FilterPacket *)malloc(sizeof(struct FilterPacket)+l-1);
          if(p) {
            p->next=0;
            memcpy(p->lastData,data,l);
            p->dataLen=l;
            p->lastTime=now;
            if(d[i].first)
              d[i].last->next=p;
            else {
              d[i].first=p;
              d[i].lastTime=now;
              }
            d[i].last=p;
            PRINTF(L_SYS_HOPPER,"PID %04x queued packet %p (time=%d len=%d) (%s)",d[i].filter->Pid(),p,p->lastTime,l,d[i].first==p?"first":"queue");
            }
          len-=l; data+=l;
          } while(len>0);
        sleep.Broadcast();
        break;
        }
    }
}

bool cPidHopper::AddPid(int pid)
{
  cPidFilter *filter=NewFilter(0);
  if(!filter) return false;
  filter->Start(pid,0,0,0,false);
  return true;
}

int cPidHopper::Read(int pid, int & now, unsigned char *buff, int len, int timeout)
{
  Lock();
  timeout+=time_ms();
  PRINTF(L_SYS_HOPPER,"PID %04x read (now=%d)",pid,now);
  int n=-1;
  while(time_ms()<timeout) {
    struct FilterData *dd=0, *old=0;
    for(int i=0; i<numFilters; i++)
      if(d[i].filter) {
        if(d[i].filter->Pid()==pid) { dd=&d[i]; break; }
        if(!old ||
           (d[i].lastRead<0 && d[i].createTime<old->createTime) ||
           (d[i].lastRead>=0 && d[i].lastRead<old->lastRead))
          old=&d[i];
        }
    if(!dd) {
      LBSTARTF(L_SYS_VERBOSE);
      LBPUT("NEW ");
      if(!AddPid(pid)) {
        if(!old) break;
        LBPUT("{0x%04x/%d/%d} ",old->filter->Pid(),old->lastRead>0 ? (time_ms()-old->lastRead)/1000:-1,old->lastTime>0 ? (time_ms()-old->lastTime)/1000:-1);
        old->filter->Stop(); // purge LRU filter
        PurgeFilter(old);
        old->lastTime=-1;
        old->lastRead=-1;
        old->createTime=time_ms();
        old->readCount=0;
        old->filter->Start(pid,0,0,0,false);
        }
      LBEND();
      continue;
      }
    for(struct FilterPacket *p=dd->first; p; p=p->next) {
      if(p->lastTime>=now) {
        n=min(len,p->dataLen);
        memcpy(buff,p->lastData,n);
        now=p->lastTime;
        PRINTF(L_SYS_HOPPER,"PID %04x returning packet %p (time=%d)",dd->filter->Pid(),p,p->lastTime);
        PurgePacket(dd,p);
        dd->lastRead=time_ms();
        dd->readCount++;
        dd->lastTime=dd->first ? dd->first->lastTime : -1;
        break;
        }
      else {
        PRINTF(L_SYS_HOPPER,"PID %04x skipping packet %p (time=%d)",dd->filter->Pid(),p,p->lastTime);
        if(!p->next) {
          PRINTF(L_SYS_HOPPER,"PID %04x queue empty",dd->filter->Pid());
          PurgePacket(dd,p);
          break;
          }
        }
      }
    if(n>0) break;
    Unlock();
    sleepMutex.Lock();
    sleep.TimedWait(sleepMutex,200);
    sleepMutex.Unlock();
    Lock();
    }
  Unlock();
  return n;
}

// -- cShl ---------------------------------------------------------------------

class cShl {
private:
  DES_key_schedule sched;
public:
  void SetKey( const unsigned char *key);
  void Decode(unsigned char *data, int n);
  };

void cShl::SetKey(const unsigned char *key)
{
  DES_key_sched((DES_cblock *)key,&sched);
}

void cShl::Decode(unsigned char *data, int n)
{
  int r=n&7;
  n-=r;
  for(int i=0; i<n; i+=8)
    DES_ecb_encrypt((DES_cblock *)&data[i],(DES_cblock *)&data[i],&sched,DES_DECRYPT);
  if(r) { // possible last incomplete block
    DES_cblock out;
    DES_ecb_encrypt((DES_cblock *)&data[n],&out,&sched,DES_DECRYPT);
    memcpy(&data[n],out,r);
    }
}

// -- SHL provider -------------------------------------------------------------

struct ShlProv {
  const char *name;
  unsigned short id;
  bool hasXor;
  int filters, pidRange;
  const unsigned short pids[128];
  const unsigned char xorTab[16];
  };

static const struct ShlProv prov[] = {
/*
  { "FreeX",0x4A90,false,40,0x1300,
    { 0x1389,0x138a,0x138b,0x138c,0x138d,0x138e,0x138f,0x1390,
      0x1391,0x1392,0x1393,0x1394,0x1395,0x1396,0x1397,0x1398,
      0x1399,0x139a,0x139b,0x139c,0x13a4,0x13a5,0x13a6,0x13a7,
      0x13a8,0x13a9,0x13bf,0x13c0,0x13c1,0x13c2,0x13c3,0x13c4,
      0x13c5,0x13c6,0x13c7,0x13c8,0x13c9,0x13ca,0x13cb,0x13cc },
    {} },
*/
  { "FullX-2",0x4A90,false,128,0x0000,
    { 0x0bb9,0x0bba,0x0bbc,0x0bbf,0x0bc0,0x0bc2,0x0bc3,0x0bc4,
      0x0bc7,0x0bc8,0x0bc9,0x0bcc,0x0bcd,0x0bce,0x0bcf,0x0bd0,
      0x0bd3,0x0bd5,0x0bd6,0x0bd7,0x0bd8,0x0bd9,0x0bdb,0x0bdd,
      0x0bdf,0x0be0,0x0be1,0x0be2,0x0be3,0x0be4,0x0be5,0x0be6,
      0x0be7,0x0be8,0x0be9,0x0bea,0x1784,0x177c,0x178a,0x1796,
      0x1774,0x1772,0x17a2,0x1783,0x1792,0x177b,0x1775,0x179b,
      0x1794,0x1789,0x1778,0x1799,0x1793,0x1797,0x1782,0x1779,
      0x179e,0x1787,0x178d,0x177d,0x1771,0x1791,0x1776,0x1788,
      0x178f,0x179c,0x177e,0x178b,0x1785,0x1781,0x0bc5,0x17a1,
      0x0bbe,0x179f,0x0bdc,0x1780,0x0bca,0x0bd2,0x0bbd,0x179a,
      0x1773,0x178e,0x1795 },
    {} },
  { "FullX",0x6997,true,60,0x1700,
    { 0x178e,0x178f,0x1783,0x1780,0x1776,0x177b,0x1772,0x1779,
      0x1785,0x179e,0x1791,0x1771,0x177e,0x1793,0x1778,0x179a,
      0x17a2,0x17a1,0x178d,0x1787,0x1797,0x1796,0x177d,0x1781,
      0x1799,0x1794,0x179b,0x1784,0x1795,0x177c,0x1782,0x179c,
      0x1773,0x1788,0x1789,0x178b,0x179f,0x1775,0x1792,0x1774,
      0x178a },
    { 0x4E,0x9E,0x5C,0xFA,0x62,0x80,0x4C,0x86,0x56,0xDF,0x7E,0x03,0x9B,0x05,0xB2,0xE7 } },
  { "DP",0x6996,true,48,0x1300,
    { 0x1389,0x138a,0x138b,0x138c,0x138d,0x138e,0x138f,0x1390,
      0x1391,0x1392,0x1393,0x1394,0x1395,0x1396,0x1397,0x1398,
      0x1399,0x139a,0x139b,0x139c,0x13a4,0x13a5,0x13a6,0x13a7,
      0x13a8,0x13a9,0x13bf,0x13c0,0x13c1,0x13c2,0x13c3,0x13c4,
      0x13c5,0x13c6,0x13c7,0x13c8,0x13c9,0x13ca,0x13cb,0x13cc },
    { 0x30,0x61,0xD7,0xC0,0x8E,0x41,0x2C,0xB9,0xAA,0x50,0xB2,0xF4,0x5B,0x2E,0x84,0xCF } }
  };

static const struct ShlProv *FindProv(unsigned short id)
{
  for(unsigned int i=0; i<(sizeof(prov)/sizeof(struct ShlProv)); i++)
    if(prov[i].id==id) return &prov[i];
  return 0;
}

// -- cSystemShl ---------------------------------------------------------------

#define ENC_OFF 12 // encrypted bytes offset
#define ENC_LEN 24 // encrypted bytes length

#define KEY_OFF   7
#define PID_OFF   15
#define EMASK_OFF 8
#define OMASK_OFF 12
#define CW_OFF    17

class cSystemShl : public cSystem, private cShl {
private:
  cPidHopper *ph;
  int prvId;
  bool hasPrvId;
  cMutex mutex;
  cCondVar wait;
  //
  void ProcessCw(const unsigned char *data, int prvId, const unsigned char *xorTable);
public:
  cSystemShl(void);
  virtual ~cSystemShl();
  virtual bool ProcessECM(const cEcmInfo *ecm, unsigned char *source);
  virtual void ProcessEMM(int pid, int caid, unsigned char *buffer);
  };

cSystemShl::cSystemShl(void)
:cSystem(SYSTEM_NAME,SYSTEM_PRI)
{
  ph=0; hasPrvId=false;
  hasLogger=needsLogger=true; maxEcmTry=20;
}

cSystemShl::~cSystemShl()
{
  delete ph;
}

bool cSystemShl::ProcessECM(const cEcmInfo *ecm, unsigned char *source)
{
  static const unsigned char tester[] = { 0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF };
  // in old SHL algo ECM starts with 12 0xFF
  if(SCT_LEN(source)<(ENC_OFF+ENC_LEN+3)) {
    PRINTF(L_SYS_VERBOSE,"short ecm");
    return false;
    }
  if(memcmp(tester,&source[3],sizeof(tester))) { // new SHL
    mutex.Lock();
    if(!hasPrvId) {
      wait.TimedWait(mutex,500); // wait for provider ID
      if(!hasPrvId) {
        PRINTF(L_SYS_ECM,"no provider ID (logger enabled?)");
        mutex.Unlock();
        return false;
        }
      }
    const struct ShlProv *prv=FindProv(prvId);
    if(!prv) {
      PRINTF(L_SYS_ECM,"provider %04x not supported",prvId);
      mutex.Unlock();
      return false;
      }
    mutex.Unlock();
    if(!ph) {
      PRINTF(L_SYS_ECM,"using card %d",CardNum());
      ph=new cPidHopper(CardNum(),prv->filters);
      if(!ph) {
        PRINTF(L_SYS_ECM,"no pid hopper");
        return false;
        }
#if 1
      unsigned int l=min((int)(sizeof(prv->pids)/sizeof(unsigned short)),prv->filters);
      for(unsigned int i=0; i<l; i++) {
        if(prv->pids[i]<=0) break;
        ph->AddPid(prv->pids[i]);
        }
#endif
      }

    int pid=ecm->ecm_pid;
    int now=time_ms();
    unsigned char buff[4096];
    unsigned char emask=0, omask=0;
    memcpy(buff+1,source,SCT_LEN(source)); buff[0]=0;
    LBSTARTF(L_SYS_VERBOSE);
    LBPUT("chain 0x%04x [%d] ",pid,SCT_LEN(source)+1);
    while(1) {
      unsigned char *ptr=buff;
      if(ptr[0]!=0) { // decrypt whole payload
        LBPUT("PES");
        unsigned char test[8];
        memcpy(test,ptr,sizeof(test)); Decode(test,sizeof(test));
        if(test[0]!=source[0] && (ptr[0]==0x11 || ptr[0]==0x12)) {     // Section is available at section_pointer offset.
          SetKey(&ptr[KEY_OFF+1]);             // Use the same key and pid offsets which you use
          Decode(&ptr[PID_OFF+1],183-PID_OFF); // for normal SHL section data decoding.
          ptr+=ptr[0]+1;
          if(ptr[0]==0x91 && SCT_LEN(ptr)==11) ptr+=11;
          LBPUT("s");
          }
        else if(test[0]!=source[0] && ptr[0]==0x0a) {
          SetKey(&ptr[1]);
          Decode(&ptr[9],183-9);
          ptr+=ptr[0]+1;
          LBPUT("a");
          }
        else {
          Decode(ptr,184);
          LBPUT("n");
          }
        LBPUT(" ");
        }
      else {          // decrypt data only
        LBPUT("SCT ");
        ptr++;
        SetKey(&ptr[KEY_OFF]);
        Decode(&ptr[PID_OFF],SCT_LEN(ptr)-PID_OFF);
        }
      if(ptr[0]!=source[0]) {
        LBPUT("wrong section %02x != %02x",ptr[0],source[0]);
        return false;
        }
      pid=(ptr[PID_OFF]<<8)+ptr[PID_OFF+1];
      if(pid==0x1FFF) {     // finished
        if(prv->hasXor) ProcessCw(ptr,prv->id,prv->xorTab);
        else {
          for(int i=0; i<8; i++) {
            const int m=1<<(7-i);
            if(!(emask & m)) cw[i  ]=ptr[CW_OFF+i];
            if(!(omask & m)) cw[i+8]=ptr[CW_OFF+i+8];
            }
          }
        LBPUT("done");
        ph->CutOff(now);
        return true;
        }
      else {                // next hop
        if(prv->hasXor) ProcessCw(ptr,prv->id,prv->xorTab);
        else {
          emask=ptr[EMASK_OFF];
          omask=ptr[OMASK_OFF];
          memcpy(cw,&ptr[CW_OFF],16);
          }
        LBPUT("0x%04x ",pid);
#if 1
        if(prv->pidRange && (pid&0xFF00)!=prv->pidRange) {
          LBPUT("wrong pid range");
          return false;
          }
#endif
        int n=ph->Read(pid,now,buff,sizeof(buff),1000);
        if(n<CW_OFF+16+1) {
          LBPUT("read failed");
          return false;
          }
        LBPUT("[%d] ",n);
        }
      }
    LBEND();
    }
  else { // old SHL
    static const unsigned char signature[] = { 0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF };
    source+=ENC_OFF+3; // here's the DES crypto content

    cPlainKey *pk=0;
    cKeySnoop ks(this,'Z',ecm->provId,0x00);
    unsigned char key[8];
    while((pk=keys.FindKey('Z',ecm->provId,0x00,sizeof(key),pk))) {
      pk->Get(key);
      SetKey(key);
      Decode(source,ENC_LEN);
      unsigned char sigbuf[8];
      memcpy(&sigbuf[0],&source[0],2);
      memcpy(&sigbuf[2],&source[18],6);
      if(!memcmp(sigbuf,signature,sizeof(sigbuf))) {
        memcpy(cw,&source[2],16);
        ks.OK(pk);
        return true;
        }
      }
    }
  return false;
}

void cSystemShl::ProcessCw(const unsigned char *data, int prvId, const unsigned char *xorTable)
{
  if(SCT_LEN(data)<150) {
    PRINTF(L_SYS_VERBOSE,"data is too short");
    return;
    }
  const unsigned int dataStart=data[33];
  if((dataStart+35)>184) {
    PRINTF(L_SYS_VERBOSE,"data broken (%d)",dataStart);
    return;
    }
  const unsigned int xorVal=data[dataStart+35];
  const unsigned int emask =data[dataStart+34];
  const unsigned int omask =data[dataStart+33];
  for(int i=0; i<8; i++) {
    const unsigned int m=1<<i;
    if(emask & m) cw[i  ]=data[CW_OFF+i  ]^xorVal^xorTable[i  ];
    if(omask & m) cw[i+8]=data[CW_OFF+8+i]^xorVal^xorTable[i+8];
    }
}

void cSystemShl::ProcessEMM(int pid, int caid, unsigned char *buffer)
{
  int n=SCT_LEN(buffer)-3;
  buffer+=3;
  for(int i=0; i<n;) {
    switch(buffer[i]) {
      case 0x0a:
        mutex.Lock();
        prvId=WORD(buffer,i+2,0xFFFF);
        if(!hasPrvId) {
          const struct ShlProv *prv=FindProv(prvId);
          PRINTF(L_SYS_EMM,"provider id %04x (%s)\n",prvId,prv ? prv->name:"unknown");
          wait.Broadcast();
          }
        hasPrvId=true;
        mutex.Unlock();
        break;
      case 0x08:
      case 0x0b:
      case 0x09:
      case 0x06:
      case 0x02:
      case 0x30:
      case 0x07:
        break;
      default:
        HEXDUMP(L_SYS_EMM,&buffer[i+2],buffer[i+1],"unknown nano: %02x:",buffer[i]);
        break;
      }
    i+=buffer[i+1]+2;
    }
}

// -- cSystemLinkShl -----------------------------------------------------------

class cSystemLinkShl : public cSystemLink {
public:
  cSystemLinkShl(void);
  virtual bool CanHandle(unsigned short SysId);
  virtual cSystem *Create(void) { return new cSystemShl; }
  };

static cSystemLinkShl staticInit;

cSystemLinkShl::cSystemLinkShl(void)
:cSystemLink(SYSTEM_NAME,SYSTEM_PRI)
{
  Feature.NeedsKeyFile();
}

bool cSystemLinkShl::CanHandle(unsigned short SysId)
{
  SysId&=0xFFF0;
  return SYSTEM_CAN_HANDLE(SysId);
}

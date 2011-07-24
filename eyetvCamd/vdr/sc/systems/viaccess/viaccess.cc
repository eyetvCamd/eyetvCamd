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
#include <ctype.h>

#include "system-common.h"
#include "misc.h"
#include "parse.h"
#include "log-core.h"

#include "viaccess.h"
#include "tps.h"
#include "log-viaccess.h"

#define SYSTEM_NAME          "Viaccess"
#define SYSTEM_PRI           -10
#define SYSTEM_CAN_HANDLE(x) ((x)==SYSTEM_VIACCESS)

static const struct LogModule lm_sys = {
  (LMOD_ENABLE|L_SYS_ALL)&LOPT_MASK,
  (LMOD_ENABLE|L_SYS_DEFDEF|L_SYS_TPS|L_SYS_TPSAU|L_SYS_TIME|L_SYS_ST20)&LOPT_MASK,
  "viaccess",
  { L_SYS_DEFNAMES,"tps","tpsau","time","st20","disasm" }
  };
ADD_MODULE(L_SYS,lm_sys)

// -- cPlainKeyVia -------------------------------------------------------------

#define VIA1_KEYLEN 8
#define VIA2_KEYLEN 16
#define VIATPS_KEYLEN 16

class cPlainKeyVia : public cPlainKeyStd {
protected:
  virtual int IdSize(void) { return 6; }
  virtual cString PrintKeyNr(void);
public:
  cPlainKeyVia(bool Super);
  virtual bool Parse(const char *line);
  virtual bool SetKey(void *Key, int Keylen);
  virtual bool SetBinKey(unsigned char *Mem, int Keylen);
  };

static cPlainKeyTypeReg<cPlainKeyVia,'V'> KeyReg;

cPlainKeyVia::cPlainKeyVia(bool Super)
:cPlainKeyStd(Super)
{}

bool cPlainKeyVia::SetKey(void *Key, int Keylen)
{
  if(keynr==MBC3('T','P','S')) SetSupersede(false);
  return cPlainKeyStd::SetKey(Key,Keylen);
}

bool cPlainKeyVia::SetBinKey(unsigned char *Mem, int Keylen)
{
  if(keynr==MBC3('T','P','S')) SetSupersede(false);
  return cPlainKeyStd::SetBinKey(Mem,Keylen);
}

bool cPlainKeyVia::Parse(const char *line)
{
  unsigned char sid[3], skeynr, skey[VIA2_KEYLEN];
  int len;
  if(GetChar(line,&type,1) && (len=GetHex(line,sid,3,false))) {
     type=toupper(type); id=Bin2Int(sid,len);
     line=skipspace(line);
     bool ok=false;
     if(!strncasecmp(line,"TPSMK",5) && line[5]>='0' && line[5]<='9') {
       keynr=MBC3('M','K',line[5]-'0');
       line+=6;
       ok=(len=GetHex(line,skey,VIATPS_KEYLEN));
       }
     else if(!strncasecmp(line,"TPS ",4)) {
       line+=4;
       keynr=MBC3('T','P','S');
       ok=(len=GetHex(line,skey,VIATPS_KEYLEN));
       }
     else if(GetHex(line,&skeynr,1)) {
       keynr=skeynr;
       ok=((len=GetHex(line,skey,VIA2_KEYLEN,false)) && (len==VIA1_KEYLEN || len==VIA2_KEYLEN));
       }
     if(ok) {
       SetBinKey(skey,len);
       return true;
       }
    }
  return false;
}

cString cPlainKeyVia::PrintKeyNr(void)
{
  char tmp[12];
  const char *kn=tmp;
  switch(keynr) {
    case MBC3('T','P','S'):
      kn="TPS"; break;
    case MBC3('M','K',0): case MBC3('M','K',1): case MBC3('M','K',2):
    case MBC3('M','K',3): case MBC3('M','K',4): case MBC3('M','K',5):
    case MBC3('M','K',6): case MBC3('M','K',7): case MBC3('M','K',8):
    case MBC3('M','K',9):
      snprintf(tmp,sizeof(tmp),"TPSMK%d",C3(keynr)); break;
    default:
      snprintf(tmp,sizeof(tmp),"%02X",keynr); break;
    }
  return kn;
}

// -- cViaccessCardInfo --------------------------------------------------------

class cViaccessCardInfo : public cStructItem, public cProviderViaccess, public cCardViaccess {
public:
  unsigned char keyno, key[8];
  //
  bool Parse(const char *line);
  };

bool cViaccessCardInfo::Parse(const char *line)
{
  return GetHex(line,ident,sizeof(ident)) && 
         GetHex(line,ua,sizeof(ua)) &&
         GetHex(line,sa,sizeof(sa)) &&
         GetHex(line,&keyno,1) && 
         GetHex(line,key,sizeof(key));
}

// -- cViaccessCardInfos -------------------------------------------------------

class cViaccessCardInfos : public cCardInfos<cViaccessCardInfo> {
public:
  cViaccessCardInfos(void):cCardInfos<cViaccessCardInfo>("Viaccess cards","Viaccess.KID",0) {}
  };

static cViaccessCardInfos Vcards;

// -- cViaccess ----------------------------------------------------------------

class cViaccess : protected cDes {
private:
  unsigned char v2key[8];
  bool v2mode;
  //
  int HashNanos(const unsigned char *data, int len);
  void Via2Mod(const unsigned char *key2, unsigned char *data);
protected:
  unsigned char hbuff[8], hkey[8];
  int pH;
  //
  void SetV2Mode(const unsigned char *key2);
  void SetHashKey(const unsigned char *key);
  void HashByte(unsigned char c);
  void HashClear(void);
  void Hash(void);
  //
  void Decode(unsigned char *data, const unsigned char *key);
  bool Decrypt(const unsigned char *work_key, const unsigned char *data, int len, unsigned char *des_data1, unsigned char *des_data2);
  //
  virtual unsigned int Mod(unsigned int R, unsigned int key7) const;
public:
  cViaccess(void);
  };

cViaccess::cViaccess(void)
:cDes()
{
  v2mode=false;
}

/* viaccess DES modification */

unsigned int cViaccess::Mod(unsigned int R, unsigned int key7) const
{
  if(key7!=0) {
    const unsigned int key5=(R>>24)&0xff;
    unsigned int al=key7*key5 + key7 + key5;
    al=(al&0xff)-((al>>8)&0xff);
    if(al&0x100) al++;
    R=(R&0x00ffffffL) + (al<<24);
    }
  return R;
}

/* viaccess2 modification. Extracted from "Russian wafer" card.
   A lot of thanks to it's author :) */

void cViaccess::Via2Mod(const unsigned char *key2, unsigned char *data)
{
  int kb, db;
  for(db=7; db>=0; db--) {
    for(kb=7; kb>3; kb--) {
      int a0=kb^db;
      int pos=7;
      if(a0&4) { a0^=7; pos^=7; }
      a0=(a0^(kb&3)) + (kb&3);
      if(!(a0&4)) data[db]^=(key2[kb] ^ ((data[kb^pos]*key2[kb^4]) & 0xFF));
      }
    }
  for(db=0; db<8; db++) {
    for(kb=0; kb<4; kb++) {
      int a0=kb^db;
      int pos=7;
      if(a0&4) { a0^=7; pos^=7; }
      a0=(a0^(kb&3)) + (kb&3);
      if(!(a0&4)) data[db]^=(key2[kb] ^ ((data[kb^pos]*key2[kb^4]) & 0xFF));
      }
    }
}

void cViaccess::Decode(unsigned char *data, const unsigned char *key)
{
  if(v2mode) Via2Mod(v2key,data);
  Des(data,key,VIA_DES);
  if(v2mode) Via2Mod(v2key,data);
}

void cViaccess::SetV2Mode(const unsigned char *key2)
{
  if(key2) {
    memcpy(v2key,key2,sizeof(v2key));
    v2mode=true;
    }
  else v2mode=false;
}


void cViaccess::SetHashKey(const unsigned char *key)
{
  memcpy(hkey,key,sizeof(hkey));
}

void cViaccess::HashByte(unsigned char c)
{
  hbuff[pH++]^=c;
  if(pH==8) { pH=0; Hash(); }
}

void cViaccess::HashClear(void)
{
  memset(hbuff,0,sizeof(hbuff));
  pH=0;
}

void cViaccess::Hash(void)
{
  if(v2mode) Via2Mod(v2key,hbuff);
  Des(hbuff,hkey,VIA_DES_HASH);
  if(v2mode) Via2Mod(v2key,hbuff);
}

int cViaccess::HashNanos(const unsigned char *data, int len)
{
  int i=0;
  pH=0;
  if(data[0]==0x9f) {
    HashByte(data[i++]);
    HashByte(data[i++]);
    for(int j=0; j<data[1]; j++) HashByte(data[i++]);
    while(pH!=0) HashByte(0);
    }
  for(; i<len; i++) HashByte(data[i]);
  return i;
}

bool cViaccess::Decrypt(const unsigned char *work_key, const unsigned char *data, int len, unsigned char *des_data1, unsigned char *des_data2)
{
  int pos=0, encStart=0;
  unsigned char signatur[8];
  while(pos<len) {
    switch(data[pos]) {
      case 0xea:                 // encrypted bytes
        encStart = pos + 2;
        memcpy(des_data1,&data[pos+2],8);
        memcpy(des_data2,&data[pos+2+8],8);
        break;
      case 0xf0:                 // signature
        memcpy(signatur,&data[pos+2],8);
        break;
      }
    pos += data[pos+1]+2;
    }
  HashClear();
  SetHashKey(work_key);
  // key preparation
  unsigned char prepared_key[8];
  if(work_key[7]==0) {
    // 8th key-byte = 0 then like Eurocrypt-M but with viaccess mods
    HashNanos(data,encStart+16);
    memcpy(prepared_key,work_key,sizeof(prepared_key));
    }
  else { // key8 not zero
    // rotate the key 2x left
    prepared_key[0]=work_key[2];
    prepared_key[1]=work_key[3];
    prepared_key[2]=work_key[4];
    prepared_key[3]=work_key[5];
    prepared_key[4]=work_key[6];
    prepared_key[5]=work_key[0];
    prepared_key[6]=work_key[1];
    prepared_key[7]=work_key[7];
    // test if key8 odd
    if(work_key[7]&1) {
      HashNanos(data,encStart);
      // test if low nibble zero
      unsigned char k = ((work_key[7] & 0xf0) == 0) ? 0x5a : 0xa5;
      for(int i=0; i<8; i++) {
        unsigned char tmp=des_data1[i];
        des_data1[i]=(k & hbuff[pH]) ^ tmp;
        HashByte(tmp);
        }
      for(int i=0; i<8; i++) {
        unsigned char tmp=des_data2[i];
        des_data2[i]=(k & hbuff[pH]) ^ tmp;
        HashByte(tmp);
        }
      }
    else {
      HashNanos(data,encStart+16);
      }
    }
  Decode(des_data1,prepared_key);
  Decode(des_data2,prepared_key);
  Hash();
  return (memcmp(signatur,hbuff,8)==0);
}

// -- cSystemViaccess ----------------------------------------------------------

#define MAX_NEW_KEYS 5

class cSystemViaccess : public cSystem, private cViaccess {
private:
  cTPS tps;
public:
  cSystemViaccess(void);
  virtual bool ProcessECM(const cEcmInfo *ecm, unsigned char *data);
  virtual void ProcessEMM(int pid, int caid, unsigned char *data);
  virtual void ParseCADescriptor(cSimpleList<cEcmInfo> *ecms, unsigned short sysId, const unsigned char *data, int len);
  };

cSystemViaccess::cSystemViaccess(void)
:cSystem(SYSTEM_NAME,SYSTEM_PRI)
{
  hasLogger=true;
}

void cSystemViaccess::ParseCADescriptor(cSimpleList<cEcmInfo> *ecms, unsigned short sysId, const unsigned char *data, int len)
{
  const int pid=WORD(data,2,0x1FFF);
  if(pid>=0xAA && pid<=0xCF) {
    PRINTF(L_CORE_ECMPROC,"viaccess: dropped \"fake\" ecm pid 0x%04x",pid);
    return;
    }
  cSystem::ParseCADescriptor(ecms,sysId,data,len);
}

bool cSystemViaccess::ProcessECM(const cEcmInfo *ecm, unsigned char *data)
{
  unsigned char *nanos=(unsigned char *)cParseViaccess::NanoStart(data);
  int len=SCT_LEN(data)-(nanos-data);

  bool mayHaveTps=false;
  if(ecm->provId==0x007c00) { // TPS
    maxEcmTry=4;
    mayHaveTps=true;
    int num=tps.Decrypt(CardNum(),ecm->source,ecm->transponder,nanos,len);
    if(num<0) return false;
    nanos+=num; len-=num;
    }

  if(cParseViaccess::CheckNano90FromNano(nanos)) {
    int keynr=cParseViaccess::KeyNrFromNano(nanos);
    cKeySnoop ks(this,'V',ecm->provId,keynr);
    cPlainKey *pk=0;
    while((pk=keys.FindKey('V',ecm->provId,keynr,-1,pk))) {
      unsigned char key[16];
      if(pk->Size()<=(int)sizeof(key)) {
        pk->Get(key);
        SetV2Mode(pk->Size()==VIA2_KEYLEN ? &key[VIA1_KEYLEN] : 0);
        if(cViaccess::Decrypt(key,&nanos[5],len-5,&cw[0],&cw[8])) {
          if(mayHaveTps) tps.PostProc(cw);
          ks.OK(pk);
          return true;
          }
        }
      }
    }
  return false;
}

void cSystemViaccess::ProcessEMM(int pid, int caid, unsigned char *data)
{
  for(cViaccessCardInfo *mkey=Vcards.First(); mkey; mkey=Vcards.Next(mkey)) {
    int updtype;
    cAssembleData ad(data);
    if(mkey->cCardViaccess::MatchEMM(data)) {
      updtype=3;
      HashClear();
      memcpy(hbuff+3,mkey->ua,sizeof(mkey->ua));
      }
    else if(mkey->cProviderViaccess::MatchEMM(data)) {
      if(mkey->cProviderViaccess::Assemble(&ad)<0) continue;
      updtype=2;
      HashClear();
      memcpy(hbuff+5,mkey->sa,sizeof(mkey->sa)-1);
      }
    else continue;

    const unsigned char *buff;
    if((buff=ad.Assembled())) {
      const unsigned char *scan=cParseViaccess::NanoStart(buff);
      unsigned int scanlen=SCT_LEN(buff)-(scan-buff);

      if(scanlen>=5 && mkey->cProviderViaccess::MatchID(buff) &&
         cParseViaccess::KeyNrFromNano(scan)==mkey->keyno) {
        scan+=5; scanlen-=5;
        SetHashKey(mkey->key);
        Hash();

        unsigned int n;
        if(scan[0]==0x9e && scanlen>=(n=scan[1]+2)) {
          for(unsigned int i=0; i<n; i++) HashByte(scan[i]);
          Hash(); pH=0;
          scan+=n; scanlen-=5;
          }
        if(scanlen>0) {
          unsigned char newKey[MAX_NEW_KEYS][8];
          int numKeys=0, updPrv[MAX_NEW_KEYS]={}, updKey[MAX_NEW_KEYS]={};

          for(unsigned int cnt=0; cnt<scanlen && numKeys<MAX_NEW_KEYS;) {
            const unsigned int parm=scan[cnt++];
            unsigned int plen=scan[cnt++];

            switch(parm) {
              case 0x90:
              case 0x9E:
                cnt+=plen;
                break;
              case 0xA1: // keyupdate
                updPrv[numKeys]=(scan[cnt]<<16)+(scan[cnt+1]<<8)+(scan[cnt+2]&0xF0);
                updKey[numKeys]=scan[cnt+2]&0x0F;
                // fall through
              default:
                HashByte(parm); HashByte(plen);
                while(plen--) HashByte(scan[cnt++]);
                break;
              case 0xEF: // crypted key(s)
                HashByte(parm); HashByte(plen);
                if(plen==sizeof(newKey[0])) {
                  const unsigned char k7=mkey->key[7];
                  for(unsigned int kc=0 ; kc<sizeof(newKey[0]) ; kc++) {
                    const unsigned char b=scan[cnt++];
                    if(k7&1) newKey[numKeys][kc]=b^(hbuff[pH]&(k7<0x10 ? 0x5a : 0xa5));
                    else     newKey[numKeys][kc]=b;
                    HashByte(b);
                    }
                  numKeys++;
                  }
                else {
                  PRINTF(L_SYS_EMM,"%d: key length mismatch %d!=%d",CardNum(),plen,(int)sizeof(newKey[0]));
                  cnt=scanlen;
                  }
                break;
              case 0xF0: // signature
                {
                char str[20], str2[20];
                static const char *ptext[] = { 0,0,"SHARED","UNIQUE" };
                const char *addr = (updtype==2) ? HexStr(str,mkey->sa,sizeof(mkey->sa)) : HexStr(str,mkey->ua,sizeof(mkey->ua));

                Hash();
                if(!memcmp(&scan[cnt],hbuff,sizeof(hbuff))) {
                  unsigned char key[8];
                  memcpy(key,mkey->key,sizeof(key));
                  if(key[7]) { // Rotate key
                    const unsigned char t1=key[0], t2=key[1];
                    key[0]=key[2]; key[1]=key[3]; key[2]=key[4]; key[3]=key[5]; key[4]=key[6];
                    key[5]=t1; key[6]=t2;
                    }

                  while(numKeys--) {
                    Decode(newKey[numKeys],key);
                    PRINTF(L_SYS_EMM,"%02X%02X %02X %s %s - KEY %06X.%02X -> %s",
                        mkey->ident[0],mkey->ident[1],mkey->keyno,addr,
                        ptext[updtype],updPrv[numKeys],updKey[numKeys],
                        HexStr(str2,newKey[numKeys],sizeof(newKey[numKeys])));
                    FoundKey();
                    if(keys.NewKey('V',updPrv[numKeys],updKey[numKeys],newKey[numKeys],8)) NewKey();
                    }
                  }
                else
                  PRINTF(L_SYS_EMM,"%02X%02X %02X %s %s - FAIL",mkey->ident[0],mkey->ident[1],mkey->keyno,addr,ptext[updtype]);
                cnt=scanlen;
                break;
                }
              }
            }
          }
        }
      }
    }
}

// -- cSystemLinkViaccess ------------------------------------------------------

class cSystemLinkViaccess : public cSystemLink {
public:
  cSystemLinkViaccess(void);
  virtual bool CanHandle(unsigned short SysId);
  virtual cSystem *Create(void) { return new cSystemViaccess; }
  };

static cSystemLinkViaccess staticInit;

cSystemLinkViaccess::cSystemLinkViaccess(void)
:cSystemLink(SYSTEM_NAME,SYSTEM_PRI)
{
  Feature.NeedsKeyFile();
}

bool cSystemLinkViaccess::CanHandle(unsigned short SysId)
{
  SysId&=SYSTEM_MASK;
  return SYSTEM_CAN_HANDLE(SysId);
}

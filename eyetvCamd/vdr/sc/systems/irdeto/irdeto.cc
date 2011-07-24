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

#include "system-common.h"
#include "data.h"
#include "parse.h"
#include "misc.h"
#include "log-sys.h"

#define SYSTEM_IRDETO        0x0600
#define SYSTEM_BETA          0x1700

#define SYSTEM_NAME          "Irdeto"
#define SYSTEM_PRI           -10
#define SYSTEM_CAN_HANDLE(x) ((x)==SYSTEM_IRDETO || (x)==SYSTEM_BETA)

#define L_SYS        5
#define L_SYS_ALL    LALL(L_SYS_LASTDEF)

static const struct LogModule lm_sys = {
  (LMOD_ENABLE|L_SYS_ALL)&LOPT_MASK,
  (LMOD_ENABLE|L_SYS_DEFDEF)&LOPT_MASK,
  "irdeto",
  { L_SYS_DEFNAMES }
  };
ADD_MODULE(L_SYS,lm_sys)

static cPlainKeyTypeReg<cPlainKeyStd,'I',false> KeyReg;

// -- cIrdCardInfo -------------------------------------------------------------

class cIrdCardInfo : public cStructItem, public cProviderIrdeto, public cCardIrdeto {
public:
  unsigned char PMK[8], HMK[10];
  bool haveHMK;
  //
  bool Parse(const char *line);
  virtual cString ToString(bool hide);
  };

bool cIrdCardInfo::Parse(const char *line)
{
  haveHMK=false;
  if(GetHex(line,hexSer,sizeof(hexSer)) && GetHex(line,HMK,sizeof(HMK))) haveHMK=true;
  return GetHex(line,&provBase,sizeof(provBase)) &&
         GetHex(line,provId,sizeof(provId)) &&
         GetHex(line,PMK,sizeof(PMK));
}

cString cIrdCardInfo::ToString(bool hide)
{
  char s1[20], s2[20], s3[20], s4[20];
  return cString::sprintf("%s %s %02x %s %s",
     HexStr(s1,hexSer,sizeof(hexSer)),HexStr(s2,HMK,sizeof(HMK)),provBase,
     HexStr(s3,provId,sizeof(provId)),HexStr(s4,PMK,sizeof(PMK)));
}

// -- cIrdCardInfos ------------------------------------------------------------

class cIrdCardInfos : public cCardInfos<cIrdCardInfo> {
public:
  cIrdCardInfos(void);
  bool Update(cIrdCardInfo *ci, const unsigned char *pmk, const unsigned char *id);
  };

static cIrdCardInfos Icards;

cIrdCardInfos::cIrdCardInfos(void)
:cCardInfos<cIrdCardInfo>("Irdeto cards","Ird-Beta.KID",SL_READWRITE)
{}

bool cIrdCardInfos::Update(cIrdCardInfo *ci, const unsigned char *pmk, const unsigned char *id)
{
  bool res=false;
  char str[20], str2[12];
  if(memcmp(ci->PMK,pmk,sizeof(ci->PMK))) {
    PRINTF(L_GEN_INFO,"new PMK for I card %s: %s",HexStr(str2,ci->hexSer,sizeof(ci->hexSer)),KeyStr(str,pmk));
    memcpy(ci->PMK,pmk,sizeof(ci->PMK));
    res=true;
    }
  if(id && memcmp(ci->provId,id,sizeof(ci->provId))) {
    PRINTF(L_GEN_INFO,"new PrvID for I card %s: %s",HexStr(str2,ci->hexSer,sizeof(ci->hexSer)),HexStr(str,id,sizeof(ci->provId)));
    memcpy(ci->provId,id,sizeof(ci->provId));
    res=true;
    }
  if(res) Modified();
  return res;
}

// -- cIrdeto ------------------------------------------------------------------

class cIrdeto {
private:
  static const unsigned char T0[], T1[], T2[];
  unsigned char tempKey[10];
  //
  void Rotate(unsigned char *p);
  void MakeTempKey(const unsigned char *key, int date);
protected:
  void SessionKeyCrypt(unsigned char *data, const unsigned char *key, int date);
  bool SignatureCheck(const unsigned char *data, int length, const unsigned char *key, int date, const unsigned char *signature, int keylen);
  void DecryptIrd(unsigned char *data, unsigned char *key, int rounds, int offset);
  };

const unsigned char cIrdeto::T0[32] = {
  0,1,2,3,4,5,6,7, 0,1,2,3,4,5,6,7,
  0,1,2,3,4,5,6,7, 0,3,6,1,4,7,2,5
  };

const unsigned char cIrdeto::T1[256] = {
  0xDA,0x26,0xE8,0x72,0x11,0x52,0x3E,0x46, 0x32,0xFF,0x8C,0x1E,0xA7,0xBE,0x2C,0x29,
  0x5F,0x86,0x7E,0x75,0x0A,0x08,0xA5,0x21, 0x61,0xFB,0x7A,0x58,0x60,0xF7,0x81,0x4F,
  0xE4,0xFC,0xDF,0xB1,0xBB,0x6A,0x02,0xB3, 0x0B,0x6E,0x5D,0x5C,0xD5,0xCF,0xCA,0x2A,
  0x14,0xB7,0x90,0xF3,0xD9,0x37,0x3A,0x59, 0x44,0x69,0xC9,0x78,0x30,0x16,0x39,0x9A,
  0x0D,0x05,0x1F,0x8B,0x5E,0xEE,0x1B,0xC4, 0x76,0x43,0xBD,0xEB,0x42,0xEF,0xF9,0xD0,
  0x4D,0xE3,0xF4,0x57,0x56,0xA3,0x0F,0xA6, 0x50,0xFD,0xDE,0xD2,0x80,0x4C,0xD3,0xCB,
  0xF8,0x49,0x8F,0x22,0x71,0x84,0x33,0xE0, 0x47,0xC2,0x93,0xBC,0x7C,0x3B,0x9C,0x7D,
  0xEC,0xC3,0xF1,0x89,0xCE,0x98,0xA2,0xE1, 0xC1,0xF2,0x27,0x12,0x01,0xEA,0xE5,0x9B,
  0x25,0x87,0x96,0x7B,0x34,0x45,0xAD,0xD1, 0xB5,0xDB,0x83,0x55,0xB0,0x9E,0x19,0xD7,
  0x17,0xC6,0x35,0xD8,0xF0,0xAE,0xD4,0x2B, 0x1D,0xA0,0x99,0x8A,0x15,0x00,0xAF,0x2D,
  0x09,0xA8,0xF5,0x6C,0xA1,0x63,0x67,0x51, 0x3C,0xB2,0xC0,0xED,0x94,0x03,0x6F,0xBA,
  0x3F,0x4E,0x62,0x92,0x85,0xDD,0xAB,0xFE, 0x10,0x2E,0x68,0x65,0xE7,0x04,0xF6,0x0C,
  0x20,0x1C,0xA9,0x53,0x40,0x77,0x2F,0xA4, 0xFA,0x6D,0x73,0x28,0xE2,0xCD,0x79,0xC8,
  0x97,0x66,0x8E,0x82,0x74,0x06,0xC7,0x88, 0x1A,0x4A,0x6B,0xCC,0x41,0xE9,0x9D,0xB8,
  0x23,0x9F,0x3D,0xBF,0x8D,0x95,0xC5,0x13, 0xB9,0x24,0x5A,0xDC,0x64,0x18,0x38,0x91,
  0x7F,0x5B,0x70,0x54,0x07,0xB6,0x4B,0x0E, 0x36,0xAC,0x31,0xE6,0xD6,0x48,0xAA,0xB4
  };

const unsigned char cIrdeto::T2[256] = {
  0x8E,0xD5,0x32,0x53,0x4B,0x18,0x7F,0x95, 0xBE,0x30,0xF3,0xE0,0x22,0xE1,0x68,0x90,
  0x82,0xC8,0xA8,0x57,0x21,0xC5,0x38,0x73, 0x61,0x5D,0x5A,0xD6,0x60,0xB7,0x48,0x70,
  0x2B,0x7A,0x1D,0xD1,0xB1,0xEC,0x7C,0xAA, 0x2F,0x1F,0x37,0x58,0x72,0x88,0xFF,0x87,
  0x1C,0xCB,0x00,0xE6,0x4E,0xAB,0xEB,0xB3, 0xF7,0x59,0x71,0x6A,0x64,0x2A,0x55,0x4D,
  0xFC,0xC0,0x51,0x01,0x2D,0xC4,0x54,0xE2, 0x9F,0x26,0x16,0x27,0xF2,0x9C,0x86,0x11,
  0x05,0x29,0xA2,0x78,0x49,0xB2,0xA6,0xCA, 0x96,0xE5,0x33,0x3F,0x46,0xBA,0xD0,0xBB,
  0x5F,0x84,0x98,0xE4,0xF9,0x0A,0x62,0xEE, 0xF6,0xCF,0x94,0xF0,0xEA,0x1E,0xBF,0x07,
  0x9B,0xD9,0xE9,0x74,0xC6,0xA4,0xB9,0x56, 0x3E,0xDB,0xC7,0x15,0xE3,0x80,0xD7,0xED,
  0xEF,0x13,0xAC,0xA1,0x91,0xC2,0x89,0x5B, 0x08,0x0B,0x4C,0x02,0x3A,0x5C,0xA9,0x3B,
  0xCE,0x6B,0xA7,0xE7,0xCD,0x7B,0xA0,0x47, 0x09,0x6D,0xF8,0xF1,0x8B,0xB0,0x12,0x42,
  0x4A,0x9A,0x17,0xB4,0x7E,0xAD,0xFE,0xFD, 0x2C,0xD3,0xF4,0xB6,0xA3,0xFA,0xDF,0xB8,
  0xD4,0xDA,0x0F,0x50,0x93,0x66,0x6C,0x20, 0xD8,0x8A,0xDD,0x31,0x1A,0x8C,0x06,0xD2,
  0x44,0xE8,0x23,0x43,0x6E,0x10,0x69,0x36, 0xBC,0x19,0x8D,0x24,0x81,0x14,0x40,0xC9,
  0x6F,0x2E,0x45,0x52,0x41,0x92,0x34,0xFB, 0x5E,0x0D,0xF5,0x76,0x25,0x77,0x63,0x65,
  0xAF,0x4F,0xCC,0x03,0x9D,0x0C,0x28,0x39, 0x85,0xDE,0xB5,0x7D,0x67,0x83,0xBD,0xC3,
  0xDC,0x3C,0xAE,0x99,0x04,0x75,0x8F,0x97, 0xC1,0xA5,0x9E,0x35,0x0E,0x3D,0x1B,0x79
  };

void cIrdeto::Rotate(unsigned char *p)
{
  unsigned char c, t;
  c=p[9]<<7;
  t=*p; *p++=(t>>1) | c; c=t<<7;
  t=*p; *p++=(t>>1) | c; c=t<<7;
  t=*p; *p++=(t>>1) | c; c=t<<7;
  t=*p; *p++=(t>>1) | c; c=t<<7;
  t=*p; *p++=(t>>1) | c; c=t<<7;
  t=*p; *p++=(t>>1) | c; c=t<<7;
  t=*p; *p++=(t>>1) | c; c=t<<7;
  t=*p; *p++=(t>>1) | c; c=t<<7;
  t=*p; *p++=(t>>1) | c; c=t<<7;
  t=*p; *p++=(t>>1) | c; c=t<<7;
}

void cIrdeto::MakeTempKey(const unsigned char *key, int date)
{
  memcpy(tempKey,key,8);
  tempKey[8]=(unsigned char)(date >> 8) ^ key[0];
  tempKey[9]=(unsigned char)date ^ key[1];
}

void cIrdeto::DecryptIrd(unsigned char *data, unsigned char *key, int rounds, int offset)
{
  for(int count=0; count<rounds; count++) {
    int index=count % 10;
    if(index==0) Rotate(key);
    unsigned char k=key[index];
    unsigned char d=data[T0[(count & 0x0f) + offset]] ^ k;
    d=(k&1) ? T1[d] : T2[d];
    data[T0[((count+1) & 0x0f) + offset]] ^= d;
    }
}

void cIrdeto::SessionKeyCrypt(unsigned char *data, const unsigned char *key, int date)
{
  MakeTempKey(key,date);
  DecryptIrd(data,tempKey,128,16);
}

bool cIrdeto::SignatureCheck(const unsigned char *data, int length, const unsigned char *key, int date, const unsigned char *signature, int keylen)
{
  unsigned char buffer[256];
  memcpy(buffer,data,length);
  int rounds;
  if((length%8)==0) rounds=40;
  else {
    int i=0x61;
    while((length%8)!=0) buffer[length++]=i++;
    rounds=104;
    }

  if(keylen==10) memcpy(tempKey,key,10);
  else MakeTempKey(key,date);

  int i=0;
  while(i<length-8) {
    DecryptIrd(buffer,tempKey,40,0);
    i+=8;
    for(int j=0; j<8; j++) buffer[j] ^= buffer[i+j];
    Rotate(tempKey);
    }
  DecryptIrd(buffer,tempKey,rounds,0);
  return memcmp(buffer,signature,5)==0;
}

// -- cSystemIrd ---------------------------------------------------------------

class cSystemIrd : public cSystem, private cIrdeto {
private:
  unsigned char lastKey;
public:
  cSystemIrd(void);
  virtual bool ProcessECM(const cEcmInfo *ecm, unsigned char *source);
  virtual void ProcessEMM(int pid, int caid, unsigned char *buffer);
  };

cSystemIrd::cSystemIrd(void)
:cSystem(SYSTEM_NAME,SYSTEM_PRI)
{
  lastKey=0;
  hasLogger=true;
}

bool cSystemIrd::ProcessECM(const cEcmInfo *ecm, unsigned char *source)
{
  unsigned char *data=0;
  int date=-1, keynr=0;
  source+=6;
  int length=source[5]+6-5; // 6 header bytes - 5 signature bytes
  for(int index=6 ; index<length && (data==0 || date==-1) ;) {
    int param=source[index++];
    int len  =source[index++] & 0x3f;

    switch(param) {
      case 0x78:
        keynr = source[index];
        data = &source[index+2];
        break;
      case 0x00:
      case 0x40:
        date = (source[index]<<8) | source[index+1];
        break;
      }
    index+=len;
    }
  if(data==0 || date==-1) {
    PRINTF(L_SYS_ECM,"incomplete ECM structure");
    return false;
    }

  cKeySnoop ks(this,'I',source[2],keynr);
  cPlainKey *pk=0;
  unsigned char key[8];
  while((pk=keys.FindKey('I',source[2],keynr,sizeof(key),pk))) {
    unsigned char save[16];
    memcpy(save,data,16); // save the encrypted data
    pk->Get(key);
    SessionKeyCrypt(&data[0],key,date);
    SessionKeyCrypt(&data[8],key,date);
    if(SignatureCheck(source,length,key,date,&source[length],8)) {
      memcpy(cw,&data[0],16);
      ks.OK(pk);
      return true;
      }
    memcpy(data,save,16); // put back the encrypted data if it didn't works
    }
  return false;
}

void cSystemIrd::ProcessEMM(int pid, int caid, unsigned char *buffer)
{
  int i, numKeys=0, date=0;
  unsigned char adr[10], id[4], *pk[4], prov, *mk=0, prvId[3]={0,0,0};

  int n=SCT_LEN(buffer);
  unsigned char savebuf[4096];
  if(n>(int)sizeof(savebuf)) {
    PRINTF(L_SYS_EMM,"%d: paket size %d to big for savebuffer in IrdetoLog",CardNum(),n);
    return;
    }

  int index=cParseIrdeto::AddrLen(buffer);
  memset(adr,0,sizeof(adr));
  memcpy(adr,&buffer[3],index+1);
  index+=4+4; // 3 header + adr type + 4 {0x01 0x00 0x00 len}
  
  int sindex=index;           // save index for sig. check
  int slen=buffer[index-1]-5; // save packet length, 5 signature bytes
  int maxIndex=index+slen;
  if(maxIndex>n-5) {
    PRINTF(L_SYS_EMM,"%d: bad packet length (%d > %d)",CardNum(),maxIndex,n-5);
    maxIndex=n-5;
    }
  bool badnano=false;
  while(!badnano && index<maxIndex) {
    unsigned char nlen=buffer[index+1] & 0x3F;
    //unsigned char prio=buffer[index] & 0x40;
    unsigned char nano=buffer[index] & ~0x40;
    switch(nano) {
      case 0x10: // key update
        {
        int k=(buffer[index+1]>>6)+1; // key counter
        if(nlen!=k*9) { badnano=true; break; }
        for(i=0 ; i<k ; i++) {
          id[i]= buffer[index+2+0+i*9];
          pk[i]=&buffer[index+2+1+i*9];
          numKeys++;
          }
        }
	break;
      case 0x00: // date
        if(nlen<2) { badnano=true; break; }
	date=WORD(buffer,index+2,0xFFFF);
	break;
      case 0x28: // pmk & provid update
        if(nlen!=13) { badnano=true; break; }
        prov= buffer[index+2+0];
        mk=  &buffer[index+2+2];
        prvId[0]=buffer[index+2+10];
        prvId[1]=buffer[index+2+11];
        prvId[2]=buffer[index+2+12];
        break;
      case 0x29: // pmk update
        if(nlen!=10) { badnano=true; break; }
        prov= buffer[index+2+0];
        mk=  &buffer[index+2+2];
        break;
      case 0x11: // channel id
        if(nlen!=6) { badnano=true; break; }
        //chId[0]=buffer[index+2+0];
        //chId[1]=buffer[index+2+1];
        break;
      case 0x91: // erase channel id
        if(nlen!=6) { badnano=true; break; }
        //eraseChId[0]=buffer[index+2+0];
        //eraseChId[1]=buffer[index+2+1];
        break;
      case 0x8B: // CB20-matrix
        if(nlen!=0x20) { badnano=true; break; }
        //cb20ptr=&buffer[index+2];
        break;
      case 0x22: // set country code
        if(nlen!=3) { badnano=true; break; }
        //
        break;
      case 0x95: // unknown
        if(nlen!=2) { badnano=true; break; }
        //
        break;
      case 0x1E: // unknown
        if(nlen!=15) { badnano=true; break; }
        //
        break;
      case 0x1F: // unknown
        if(nlen!=3) { badnano=true; break; }
        //
        break;
      case 0x16: // unknown
        if(nlen!=2) { badnano=true; break; }
        //
        break;
      case 0x12: // unknown
        if(nlen!=6) { badnano=true; break; }
        //
        break;

      default:
        PRINTF(L_SYS_EMM,"%d: unhandled nano 0x%02x",CardNum(),nano);
        break;
      }
    index+=nlen+2;
    }
  if(badnano || index!=maxIndex) {
    PRINTF(L_SYS_EMM,"%d: bad nano/bad paket",CardNum());
    return;
    }

  // lastKey: save cpu time if we get bursts of the same key
  if((numKeys>0 && (id[0]!=lastKey || numKeys>1)) || mk) {
    memcpy(savebuf,buffer,n); // save the buffer
    cIrdCardInfo *ci=Icards.First();
    unsigned char *chkkey=AUTOMEM(max(sizeof(ci->PMK),sizeof(ci->HMK)));
    while(ci) {
      ci->hexBase=cParseIrdeto::AddrBase(buffer);
      if((numKeys>0         && (ci->cProviderIrdeto::MatchEMM(buffer) || CheckNull(ci->provId,sizeof(ci->provId)) )) ||
         (mk && ci->haveHMK && (ci->cCardIrdeto::MatchEMM(buffer)))) {
        LBSTARTF(L_SYS_EMM);
        LBPUT("%02x %02x%02x%02x",buffer[3],buffer[4],buffer[5],buffer[6]);

        for(i=0 ; i<numKeys ; i++) {
          lastKey=id[i];
          SessionKeyCrypt(pk[i],ci->PMK,date);
          }
        int keylen;
        if(mk) {
          memcpy(chkkey,ci->HMK,sizeof(ci->HMK)); // key is modified in decryptIrd()
          DecryptIrd(mk,chkkey,128,16);
          keylen=sizeof(ci->HMK);
          memcpy(chkkey,ci->HMK,sizeof(ci->HMK)); // signature check with HMK
          }
       else {
          keylen=sizeof(ci->PMK);
          memcpy(chkkey,ci->PMK,sizeof(ci->PMK)); // signature check with PMK
          }

        memcpy(&buffer[sindex-6],adr,5);
        if(SignatureCheck(&buffer[sindex-6],slen+6,chkkey,date,&buffer[sindex+slen],keylen))
	  {
          char str[20];
          LBPUT(" - OK");
          for(i=0 ; i<numKeys ; i++) LBPUT(" - PK %02x %s",id[i],KeyStr(str,pk[i]));
          if(mk) LBPUT(" - PMK %s",KeyStr(str,mk));

          for(i=0 ; i<numKeys ; i++) {
            FoundKey();
            if(keys.NewKey('I',ci->provBase,id[i],pk[i],8)) NewKey();
            }
          if(mk) {
            FoundKey();
            if(Icards.Update(ci,mk,prvId[0]?prvId:0)) NewKey();
            }
          break;
          }
        else {
          LBPUT(" - FAIL");
          }
        LBEND();

        memcpy(buffer,savebuf,n); // restore the buffer
        }
      ci=Icards.Next(ci);
      }
    }
}

// -- cSystemLinkIrd -----------------------------------------------------------

class cSystemLinkIrd : public cSystemLink {
public:
  cSystemLinkIrd(void);
  virtual bool CanHandle(unsigned short SysId);
  virtual cSystem *Create(void) { return new cSystemIrd; }
  };

static cSystemLinkIrd staticInit;

cSystemLinkIrd::cSystemLinkIrd(void)
:cSystemLink(SYSTEM_NAME,SYSTEM_PRI)
{
  Feature.NeedsKeyFile();
}

bool cSystemLinkIrd::CanHandle(unsigned short SysId)
{
  SysId&=SYSTEM_MASK;
  return SYSTEM_CAN_HANDLE(SysId);
}

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
//#include <byteswap.h>

#include <openssl/sha.h>

#include "system-common.h"
#include "data.h"
#include "helper.h"
#include "crypto.h"
#include "parse.h"
#include "misc.h"
#include "log-sys.h"
#include "log-core.h"

#define SYSTEM_SECA          0x0100

#define SYSTEM_NAME          "Seca"
#define SYSTEM_PRI           -10
#define SYSTEM_CAN_HANDLE(x) ((x)==SYSTEM_SECA)

#define FILEMAP_DOMAIN       "seca"

#define L_SYS        14
#define L_SYS_ALL    LALL(L_SYS_LASTDEF)

static const struct LogModule lm_sys = {
  (LMOD_ENABLE|L_SYS_ALL)&LOPT_MASK,
  (LMOD_ENABLE|L_SYS_DEFDEF)&LOPT_MASK,
  "seca",
  { L_SYS_DEFNAMES }
  };
ADD_MODULE(L_SYS,lm_sys)

// -- cPlainKeySeca ------------------------------------------------------------

#define PLAINLEN_SECA_H 8
#define PLAINLEN_SECA_D 16
#define PLAINLEN_SECA_B 90
#define PLAINLEN_SECA_E 6  // exponent key len
#define EMM_MAGIC       1
#define N51_MAGIC       2

class cPlainKeySeca : public cDualKey {
protected:
  virtual bool IsBNKey(void) const;
  virtual int IdSize(void) { return 4; }
  virtual cString PrintKeyNr(void);
public:
  cPlainKeySeca(bool Super);
  virtual bool Parse(const char *line);
  };

static cPlainKeyTypeReg<cPlainKeySeca,'S'> KeyReg;

cPlainKeySeca::cPlainKeySeca(bool Super)
:cDualKey(Super,true)
{}

bool cPlainKeySeca::IsBNKey(void) const
{
  switch(keynr&C2MASK) {
    case MBC('E','1'):
    case MBC('M','1'):
    case MBC('E','3'):
    case MBC('M','3'):
    case MBC('E','5'):
    case MBC('M','5'):
    case MBC('E','9'):
    case MBC('M','9'): return true;
    }
  return false;
}

bool cPlainKeySeca::Parse(const char *line)
{
  unsigned char sid[2];
  int len;
  if(GetChar(line,&type,1) && (len=GetHex(line,sid,2,false))) {
    type=toupper(type); id=Bin2Int(sid,len);
    line=skipspace(line);
    int emmnr=0, keylen=PLAINLEN_SECA_B;
    if(!strncasecmp(line,"EMM",3)) { // EMM RSA key
      emmnr=EMM_MAGIC;
      line+=3;
      line=skipspace(line);
      }
    else if(!strncasecmp(line,"N51",3)) { // Nano 51 RSA key
      emmnr=N51_MAGIC;
      line+=3;
      line=skipspace(line);
      keylen=toupper(*line)=='E'?1:129;
      }
    bool ok;
    switch(toupper(*line)) {
      case 'E':
      case 'M': ok=GetChar(line,&keynr,2);
                keynr=ADDC3(keynr,emmnr);
                break;
      default:  ok=emmnr ? false : GetHex(line,sid,1);
                keynr=sid[0];
                break;
      }
    if(ok) {
      unsigned char *skey=AUTOMEM(keylen);
      len=GetHex(line,skey,keylen,false);
      if(IsBNKey()) {
        if(C2(keynr)=='E' && len==PLAINLEN_SECA_E) {
          // support short exponent keys
          keylen=len;
          }
        }
      else {
        if(len==PLAINLEN_SECA_H || len==PLAINLEN_SECA_D) {
          // support for 16 & 8 byte keys
          keylen=len;
          }
        }
      if(len==keylen) {
        SetBinKey(skey,keylen);
        return true;
        }
      }
    }
  return false;
}

cString cPlainKeySeca::PrintKeyNr(void)
{
  char nr[32];
  int q=0;
  switch(C3(keynr)) {
    case EMM_MAGIC: q+=snprintf(nr+q,sizeof(nr)-q,"EMM "); break;
    case N51_MAGIC: q+=snprintf(nr+q,sizeof(nr)-q,"N51 "); break;
    }
  if(IsBNKey()) {
    nr[q  ]=(keynr>>8) & 0xff;
    nr[q+1]= keynr     & 0xff;
    nr[q+2]=0;
    q+=2;
    }
  else q+=snprintf(nr+q,sizeof(nr)-q,"%.2X",keynr);
  return nr;
}

// -- cSecaCardInfo ------------------------------------------------------------

class cSecaCardInfo : public cStructItem, public cProviderSeca {
private:
  int len;
public:
  unsigned char key[16];
  //
  bool Parse(const char *line);
  int KeySize(void) { return len; }
  };

bool cSecaCardInfo::Parse(const char *line)
{
  return GetHex(line,provId,sizeof(provId)) &&
         GetHex(line,sa,sizeof(sa)) &&
         (len=GetHex(line,key,sizeof(key),false)) && 
         (len==PLAINLEN_SECA_H || len==PLAINLEN_SECA_D);
}

// -- cSecaCardInfos -----------------------------------------------------------

class cSecaCardInfos : public cCardInfos<cSecaCardInfo> {
public:
  cSecaCardInfos(void):cCardInfos<cSecaCardInfo>("Seca cards","Seca.KID",0) {}
  };

static cSecaCardInfos Scards;

// -- cSeca --------------------------------------------------------------------

static const unsigned char secaPC1[] = {
  42,57,29,34,  41,53,30,15,
  19,36,23,14,  43,61,12, 3,
  51,49,5,  6,  45,54,52,47,
  63,38,58,22,  60,33,10,26,
  37,35,44, 1,  20,62,28,18,
  46, 9,39, 4,  27,11,21,50,
  31,25, 2, 7,  13,55,59,17
  };

static const unsigned char secaPC2[] = {
  18, 3,21,15,  42,35,37, 8,
  49,41,30,55,  56,29,12,23,
  43,14,7 ,27,  13, 2,11,45,
   4,34,54,51,  22,40,16,25,
  26,48,53,28,   1,17, 5,31,
  50, 6,39,24,  33,47,38,32
  };

static inline void __swap8_4(unsigned char *data)
{
  unsigned char temp[4];
  memcpy(temp,data,4);
  memcpy(data,&data[4],4);
  memcpy(&data[4],temp,4);
}
#define swap8_4(d) __swap8_4(d)

class cSeca {
private:
  static const unsigned char TD[];
  //
  void Fase(unsigned char *data, const unsigned char *key, const unsigned char *T1, const unsigned char *T2);
protected:
  static const unsigned char T1_S1[], T2_S1[];
  //
  void Decrypt(unsigned char *data, const unsigned char *key, const unsigned char *T1, const unsigned char *T2);
  void Encrypt(unsigned char *data, const unsigned char *key, const unsigned char *T1, const unsigned char *T2);
  void CalcSignature(const unsigned char *buff, int len, unsigned char *signature, const unsigned char *k, const unsigned char *T1, const unsigned char *T2);
  // Seca2 functions
  void AdditionalAlgo(unsigned char *data, const unsigned char *key, const int mode);
  };

const unsigned char cSeca::TD[4] = { 1,3,0,2 };

const unsigned char cSeca::T1_S1[256] = { // Original Tables
  0x2a,0xe1,0x0b,0x13,0x3e,0x6e,0x32,0x48, 0xd3,0x31,0x08,0x8c,0x8f,0x95,0xbd,0xd0,
  0xe4,0x6d,0x50,0x81,0x20,0x30,0xbb,0x75, 0xf5,0xd4,0x7c,0x87,0x2c,0x4e,0xe8,0xf4,
  0xbe,0x24,0x9e,0x4d,0x80,0x37,0xd2,0x5f, 0xdb,0x04,0x7a,0x3f,0x14,0x72,0x67,0x2d,
  0xcd,0x15,0xa6,0x4c,0x2e,0x3b,0x0c,0x41, 0x62,0xfa,0xee,0x83,0x1e,0xa2,0x01,0x0e,//8
  0x7f,0x59,0xc9,0xb9,0xc4,0x9d,0x9b,0x1b, 0x9c,0xca,0xaf,0x3c,0x73,0x1a,0x65,0xb1,
  0x76,0x84,0x39,0x98,0xe9,0x53,0x94,0xba, 0x1d,0x29,0xcf,0xb4,0x0d,0x05,0x7d,0xd1,
  0xd7,0x0a,0xa0,0x5c,0x91,0x71,0x92,0x88, 0xab,0x93,0x11,0x8a,0xd6,0x5a,0x77,0xb5,
  0xc3,0x19,0xc1,0xc7,0x8e,0xf9,0xec,0x35, 0x4b,0xcc,0xd9,0x4a,0x18,0x23,0x9f,0x52,//16
  0xdd,0xe3,0xad,0x7b,0x47,0x97,0x60,0x10, 0x43,0xef,0x07,0xa5,0x49,0xc6,0xb3,0x55,
  0x28,0x51,0x5d,0x64,0x66,0xfc,0x44,0x42, 0xbc,0x26,0x09,0x74,0x6f,0xf7,0x6b,0x4f,
  0x2f,0xf0,0xea,0xb8,0xae,0xf3,0x63,0x6a, 0x56,0xb2,0x02,0xd8,0x34,0xa4,0x00,0xe6,
  0x58,0xeb,0xa3,0x82,0x85,0x45,0xe0,0x89, 0x7e,0xfd,0xf2,0x3a,0x36,0x57,0xff,0x06,//24
  0x69,0x54,0x79,0x9a,0xb6,0x6c,0xdc,0x8b, 0xa7,0x1f,0x90,0x03,0x17,0x1c,0xed,0xd5,
  0xaa,0x5e,0xfe,0xda,0x78,0xb0,0xbf,0x12, 0xa8,0x22,0x21,0x3d,0xc2,0xc0,0xb7,0xa9,
  0xe7,0x33,0xfb,0xf1,0x70,0xe5,0x17,0x96, 0xf8,0x8d,0x46,0xa1,0x86,0xe2,0x40,0x38,
  0xf6,0x68,0x25,0x16,0xac,0x61,0x27,0xcb, 0x5b,0xc8,0x2b,0x0f,0x99,0xde,0xce,0xc5
  };

const unsigned char cSeca::T2_S1[256] = {
  0xbf,0x11,0x6d,0xfa,0x26,0x7f,0xf3,0xc8, 0x9e,0xdd,0x3f,0x16,0x97,0xbd,0x08,0x80,
  0x51,0x42,0x93,0x49,0x5b,0x64,0x9b,0x25, 0xf5,0x0f,0x24,0x34,0x44,0xb8,0xee,0x2e,
  0xda,0x8f,0x31,0xcc,0xc0,0x5e,0x8a,0x61, 0xa1,0x63,0xc7,0xb2,0x58,0x09,0x4d,0x46,
  0x81,0x82,0x68,0x4b,0xf6,0xbc,0x9d,0x03, 0xac,0x91,0xe8,0x3d,0x94,0x37,0xa0,0xbb, //8
  0xce,0xeb,0x98,0xd8,0x38,0x56,0xe9,0x6b, 0x28,0xfd,0x84,0xc6,0xcd,0x5f,0x6e,0xb6,
  0x32,0xf7,0x0e,0xf1,0xf8,0x54,0xc1,0x53, 0xf0,0xa7,0x95,0x7b,0x19,0x21,0x23,0x7d,
  0xe1,0xa9,0x75,0x3e,0xd6,0xed,0x8e,0x6f, 0xdb,0xb7,0x07,0x41,0x05,0x77,0xb4,0x2d,
  0x45,0xdf,0x29,0x22,0x43,0x89,0x83,0xfc, 0xd5,0xa4,0x88,0xd1,0xf4,0x55,0x4f,0x78,//16
  0x62,0x1e,0x1d,0xb9,0xe0,0x2f,0x01,0x13, 0x15,0xe6,0x17,0x6a,0x8d,0x0c,0x96,0x7e,
  0x86,0x27,0xa6,0x0d,0xb5,0x73,0x71,0xaa, 0x36,0xd0,0x06,0x66,0xdc,0xb1,0x2a,0x5a,
  0x72,0xbe,0x3a,0xc5,0x40,0x65,0x1b,0x02, 0x10,0x9f,0x3b,0xf9,0x2b,0x18,0x5c,0xd7,
  0x12,0x47,0xef,0x1a,0x87,0xd2,0xc2,0x8b, 0x99,0x9c,0xd3,0x57,0xe4,0x76,0x67,0xca,//24
  0x3c,0xfb,0x90,0x20,0x14,0x48,0xc9,0x60, 0xb0,0x70,0x4e,0xa2,0xad,0x35,0xea,0xc4,
  0x74,0xcb,0x39,0xde,0xe7,0xd4,0xa3,0xa5, 0x04,0x92,0x8c,0xd9,0x7c,0x1c,0x7a,0xa8,
  0x52,0x79,0xf2,0x33,0xba,0x1f,0x30,0x9a, 0x00,0x50,0x4c,0xff,0xe5,0xcf,0x59,0xc3,
  0xe3,0x0a,0x85,0xb3,0xae,0xec,0x0b,0xfe, 0xe2,0xab,0x4a,0xaf,0x69,0x6c,0x2c,0x5d
  };

void cSeca::Fase(unsigned char *D, const unsigned char *key, const unsigned char *T1, const unsigned char *T2)
{
  for(int l=0; l<4; ++l) { D[l]^=key[l]; D[l]=T1[D[l]&0xff]; }
  for(int l=6; l>3; --l) {
    D[(l+2)&3]^=D[(l+1)&3];
    D[l&3]=T2[(sn8(D[(l+1)&3])+D[l&3])&0xff];
    } 
  for(int l=3; l>0; --l) {
    D[(l+2)&3]^=D[(l+1)&3];
    D[l&3]=T1[(sn8(D[(l+1)&3])+D[l&3])&0xff];
    }
  D[2]^=D[1];
  D[1]^=D[0];
}

void cSeca::Decrypt(unsigned char *d, const unsigned char *key, const unsigned char *T1, const unsigned char *T2)
{
  unsigned char k[16],D[4];
  memcpy(k,key,sizeof(k));
  unsigned char C=0xff;
  for(int j=0; j<4; ++j) {
    for(int i=0; i<16; ++i) {
      if((i&3)==0) ++C;
      k[i]^=T1[(k[(15+i)&0xf]^k[(i+1)&0xf]^C)&0xff]; 
      } 
    }
  int j=0;
  for(int i=0; i<16; ++i) {
    for(int l=0; l<4; ++l) D[l]=d[l+4];
    j=(j+12)&0xf;
    Fase(D,&k[j&0xc],T1,T2);
    for(int l=0; l<4; ++l) d[l]^=T2[D[TD[l]]&0xff];
    for(int l=3; l>=0; --l) k[(j+l)&0xf]^=T1[(k[(j+l+1)&0xf]^k[(j+l+15)&0xf]^(15-i))&0xff];
    if(i<15) swap8_4(d);
    }
}

void cSeca::Encrypt(unsigned char *d, const unsigned char *key, const unsigned char *T1, const unsigned char *T2)
{
  unsigned char D[4],kk[16];
  memcpy(kk, key, sizeof(kk));
  for(int j=0, i=0; i<16; ++i, j=(j+4)&0xf) {
    for(int l=0; l<4; ++l) kk[(j+l)&0xf] ^= T1[(kk[(j+l+1)&0xf]^kk[(j+l+15)&0xf]^i)&0xff]; 
    if(i>0) swap8_4(d);
    for(int l=0; l<4; ++l) D[l]=d[l+4];
    Fase(D,&kk[j&0xc],T1,T2);
    for(int l=0; l<4; ++l) d[l]^=T2[D[TD[l]]&0xff];
    } 
}

void cSeca::CalcSignature(const unsigned char *buff, int len, unsigned char *signature, const unsigned char *k, const unsigned char *T1, const unsigned char *T2)
{
  memset(signature,0,8);
  for(int i=0 ; i<len ; i+=8) {
    for(int j=0 ; j<8 && i+j<len; j++) signature[j]^=buff[i+j];
    Encrypt(signature,k,T1,T2);
    }
}

void cSeca::AdditionalAlgo(unsigned char *data, const unsigned char *key, const int mode)
{
  static const unsigned int adders[] = {
    0x0000,0xe555,0xafff,0x5ffe,0xf552,0x6ffb,0xcff9,0x154c,
    0x3ff4,0x4ff1,0x4543,0x1fea,0xdfe6,0x8537,0x0fdd,0x7fd8
    };
  const unsigned short * const k = (const unsigned short *)key;
  unsigned short * const dd = (unsigned short *)data;
  unsigned int d1=dd[0], d2=dd[1], d3=dd[2], d4=dd[3];
  PRINTF(L_SYS_VERBOSE,"additional algo: %s",!mode?"encrypt":"decrypt");
  if(!mode) {
    for(int i=0; i<0x10; i++) {
      const unsigned int adder = adders[i];
      d1 += (k[0] + d3 + adder) ^ (k[1] + d4 + adder);
      d2 += (k[2] + d3 + adder) ^ (k[3] + d4 + adder);
      d1 = ror16(d1,5);
      d2 = rol16(d2,3);
      d3 += (k[0] + d1 + adder) ^ (k[1] + d2 + adder);
      d4 += (k[2] + d1 + adder) ^ (k[3] + d2 + adder);
      d3 = rol16(d3,3);
      d4 = ror16(d4,5);
      }
    }
  else {
    for(int i=0xf; i>=0; i--) {
      const unsigned int adder = adders[i];
      d4 = rol16(d4,5);
      d3 = ror16(d3,3);
      d4 -= (k[2] + d1 + adder) ^ (k[3] + d2 + adder);
      d3 -= (k[0] + d1 + adder) ^ (k[1] + d2 + adder);
      d2 = ror16(d2,3);
      d1 = rol16(d1,5);
      d2 -= (k[2] + d3 + adder) ^ (k[3] + d4 + adder);
      d1 -= (k[0] + d3 + adder) ^ (k[1] + d4 + adder);
      }
    }
  dd[0]=d1; dd[1]=d2; dd[2]=d3; dd[3]=d4;
}

// -- cSeca2Prov ---------------------------------------------------------------

#define MAX_PERMS    10
#define KEY2INDEX(x) (((x)>>5)&0x03)

struct Perm {
  unsigned char P1[8], P2[8], P3[8], P4[8];
  };

struct ProvData {
  unsigned short LoadId, MTLoadSize, MTSize;
  unsigned char SHA1Pad, SHA1End;
  unsigned char FP[8];
  };

class cSeca2Prov: protected cSeca {
private:
  cFileMap *hash, *mask;
  unsigned short id;
  //
  unsigned short Sum(const unsigned char *data, int n) const;
protected:
  const struct ProvData *pData;
  const struct Perm *perm;
  //
  cFileMap *GetMap(const char *type, int size, bool generic) const;
public:
  cSeca2Prov(unsigned short Id);
  virtual ~cSeca2Prov();
  // provider specific mods
  virtual void PreSSE(unsigned char *data, int pos) {}
  virtual void PostSSE(unsigned char *data, int pos) {}
  virtual void SignatureMod(unsigned char *MD, const unsigned char *PK) {}
  virtual void PreCW(unsigned char *data) {}
  virtual void PostCW(unsigned char *data) {}
  virtual void ChainTableXor(unsigned char *data, unsigned short index)=0;
  virtual bool DoSigCheck(void) { return true; }
  //
  virtual bool Init(void);
  void CalcSHASignature(const unsigned char *data, int n, unsigned char *signature, bool PadMode=true);
  bool DecryptSE(unsigned char *data, const unsigned char *key, int n, int start, const unsigned char *T1, const unsigned char *T2);
  bool Matches(unsigned short Id) const { return Id==id; }
  //
  const unsigned char *T1(int index) const;
  const unsigned char *T2(int index) const;
  const struct Perm *P(int index) const;
  inline const unsigned char *MT(void) const { return mask->Addr(); }
  inline int MTSize(void) const { return pData->MTSize; }
  inline const unsigned char *FP(void) const { return pData->FP; }
  };

cSeca2Prov::cSeca2Prov(unsigned short Id)
{
  id=Id;
  pData=0; perm=0; hash=0; mask=0;
}

cSeca2Prov::~cSeca2Prov()
{
  if(hash) hash->Unmap();
  if(mask) mask->Unmap();
}

bool cSeca2Prov::Init(void)
{
  hash=GetMap("hash",1536,false);
  mask=GetMap("mt",pData->MTLoadSize,false);
  if(!hash || !mask) return false;
  return true;
}

cFileMap *cSeca2Prov::GetMap(const char *type, int size, bool generic) const
{
  char name[32];
  snprintf(name,sizeof(name),generic ? "s2_%s.bin" : "s2_%s_%04x.bin",type,pData->LoadId);
  cFileMap *map=filemaps.GetFileMap(name,FILEMAP_DOMAIN,false);
  if(!map)
    PRINTF(L_SYS_CRYPTO,"no filemap for %s",name);
  else if(!map->Map()) {
    PRINTF(L_SYS_CRYPTO,"mapping failed for %s",name);
    map=0;
    }
  else if(map->Size()<size) {
    PRINTF(L_SYS_CRYPTO,"%s file %s has wrong size (has=%d wanted=%d)",type,name,map->Size(),size);
    map->Unmap();
    map=0;
    }
  return map;
}

// Hash file layout is (Yankse style):
//  Table T1 for 9x, 
//  Table T1 for Bx,
//  Table T1 for FX,
//  Table T2 for 9x, 
//  Table T2 for Bx,
//  Table T2 for FX  (total: 1536 bytes)

const unsigned char *cSeca2Prov::T1(int index) const
{
  static int idxTrans[] = { 0,1,2,2 };
  if(index>=0 && index<=3) {
    return hash->Addr()+(idxTrans[index]*256);
    }
  PRINTF(L_SYS_CRYPTO,"bad T1 table index %d",index);
  return 0;
}

const unsigned char *cSeca2Prov::T2(int index) const
{
  static int idxTrans[] = { 0,1,2,2 };
  if(index>=0 && index<=3) {
    return hash->Addr()+(idxTrans[index]*256)+768;
    }
  PRINTF(L_SYS_CRYPTO,"bad T2 table index %d",index);
  return 0;
}

const struct Perm *cSeca2Prov::P(int index) const
{
  if(index>=0 && index<MAX_PERMS && perm) {
    return &perm[index];
    }
  PRINTF(L_SYS_CRYPTO,"no perm table or bad index %d",index);
  return 0;
}

unsigned short cSeca2Prov::Sum(const unsigned char *data, int n) const
{
  unsigned int sum=0;
  for(int i=1; i<n; i+=2) sum+=((data[i-1]<<8)+data[i]);
  if(n&1) sum+=(data[n-1]<<4);
  return sum&0x3FFF;
}

bool cSeca2Prov::DecryptSE(unsigned char *data, const unsigned char *key, int n, int start, const unsigned char *T1, const unsigned char *T2)
{
  const int sigStart=n-data[n-1]-9; // 82 <8 bytes> P5(sigStart)
  const int encrLen=sigStart-16;

#ifdef DEBUG_SECA_EXTRA
  if(encrLen>=n || encrLen<0) printf("encrLen error in SE\n");
  if(sigStart<16) printf("sigStart error in SE\n");
#endif
  if(encrLen>=n || encrLen<0 || sigStart<16) return false;

  Decrypt(data+sigStart-8,key,T1,T2);
  ChainTableXor(data+sigStart-8,Sum(data+start,sigStart-8-start));
  int i;

  for(i=start; i<encrLen; i+=8) {
    Decrypt(data+i,key,T1,T2);
    ChainTableXor(data+i,Sum(&data[i+8],8));
    }

  int restBytes=sigStart&0x07;
  if(!restBytes) restBytes=8;

  // Last Block
  unsigned char lastBlock[8];
  memset(lastBlock,0,sizeof(lastBlock));
  memcpy(lastBlock,data+sigStart-restBytes,restBytes);
  Decrypt(lastBlock,key,T1,T2);

  Decrypt(data+i,key,T1,T2);
  ChainTableXor(data+i,Sum(lastBlock,sizeof(lastBlock)));
  return true;
}

void cSeca2Prov::CalcSHASignature(const unsigned char *data, int n, unsigned char *signature, bool PadMode)
{
  SHA_CTX ctx;
  SHA1_Init(&ctx);
  SHA1_Update(&ctx,data,n);
  unsigned char Pad=0;
  if(PadMode) {
    unsigned char End=pData->SHA1End;
    SHA1_Update(&ctx,&End,1);
    int l=(n&63)+1;
    Pad=pData->SHA1Pad;
    if(l>62) {
      for(; l<64; l++) SHA1_Update(&ctx,&Pad,1);
      l=0;
      }
    for(; l<62; l++) SHA1_Update(&ctx,&Pad,1);
    unsigned short s=NXSwapShort(n);
    SHA1_Update(&ctx,&s,2);
    }
  else for(; n&63; n++) SHA1_Update(&ctx,&Pad,1);

  *((unsigned int *)(signature   ))=NXSwapInt(ctx.h0);
  *((unsigned int *)(signature+ 4))=NXSwapInt(ctx.h1);
  *((unsigned int *)(signature+ 8))=NXSwapInt(ctx.h2);
  *((unsigned int *)(signature+12))=NXSwapInt(ctx.h3);
  *((unsigned int *)(signature+16))=NXSwapInt(ctx.h4);
}

// -- cSecaProviders & cSecaProviderLink ---------------------------------------

class cSeca2ProvLink {
friend class cSeca2Providers;
private:
  cSeca2ProvLink *next;
  const unsigned short *ids;
public:
  cSeca2ProvLink(const unsigned short *Ids);
  bool CanHandle(unsigned short Id);
  virtual cSeca2Prov *Create(unsigned short Id)=0;
  virtual ~cSeca2ProvLink() {};
  };

template<class CC> class cSeca2ProvLinkReg : public cSeca2ProvLink {
public:
  cSeca2ProvLinkReg(const unsigned short *Ids):cSeca2ProvLink(Ids) {}
  virtual cSeca2Prov *Create(unsigned short Id) { return new CC(Id); }
  };

class cSeca2Providers {
friend class cSeca2ProvLink;
private:
  static cSeca2ProvLink *first;
  //
  static void Register(cSeca2ProvLink *spl);
public:
  static cSeca2Prov *GetProv(unsigned short id);
  };

cSeca2ProvLink *cSeca2Providers::first=0;

void cSeca2Providers::Register(cSeca2ProvLink *spl)
{
  LBSTART(L_CORE_DYN);
  LBPUT("seca2prov: registering Seca2 provider");
  for(int i=0; spl->ids[i]; i++) LBPUT(" %.4x",spl->ids[i]);
  LBEND();
  spl->next=first;
  first=spl;
}

cSeca2Prov *cSeca2Providers::GetProv(unsigned short id)
{
  cSeca2ProvLink *spl=first;
  while(spl) {
    if(spl->CanHandle(id)) {
      cSeca2Prov *sp=spl->Create(id);
      if(sp && sp->Init()) return sp;
      delete sp;
      }
    spl=spl->next;
    }
  return 0;
}

cSeca2ProvLink::cSeca2ProvLink(const unsigned short *Ids)
{
  ids=Ids;
  cSeca2Providers::Register(this);
}

bool cSeca2ProvLink::CanHandle(unsigned short Id)
{
  for(int i=0; ids[i]; i++) if(ids[i]==Id) return true;
  return false;
}

// -- cSeca2ProvSSE ------------------------------------------------------------

#define beforeSig (pos-8)
#define afterSig  (pos+1)

class cSeca2ProvSSE : public cSeca2Prov {
private:
  virtual void PreSSECore(unsigned char *buf, const unsigned char *data, int i)=0;
  virtual void PostSSECore1(unsigned char *data, int pos)=0;
  virtual void PostSSECore2(unsigned char *buf, const unsigned char *data, int pos)=0;
  virtual void PostSSECore3(unsigned char *data, const unsigned char *buf, int pos)=0;
  virtual void PostSSECore4(unsigned char *data, int pos)=0;
protected:
  cDes des;
  cFileMap *sse, *sseP, *cw;
  //
  virtual bool InitSSE(void)=0;
  virtual void PreSSE(unsigned char *data, int pos);
  virtual void PostSSE(unsigned char *data, int pos);
  //
  inline const unsigned char *SSET1(void) const { return sse->Addr(); }
  inline const unsigned char *SSET2(void) const { return sse->Addr()+3072; }
  inline const unsigned char *SSEPT1(void) const { return sseP->Addr(); }
  inline const unsigned char *SSEPT2(void) const { return sseP->Addr()+80; }
  inline const unsigned char *CWT1(void) const { return cw->Addr(); }
public:
  cSeca2ProvSSE(unsigned short Id);
  virtual ~cSeca2ProvSSE();
  virtual bool Init(void);
  };

cSeca2ProvSSE::cSeca2ProvSSE(unsigned short Id)
:cSeca2Prov(Id)
,des(secaPC1,secaPC2)
{
  sse=0; sseP=0; cw=0;
}

cSeca2ProvSSE::~cSeca2ProvSSE()
{
  if(sse)  sse->Unmap();
  if(sseP) sseP->Unmap();
  if(cw)   cw->Unmap();
}

bool cSeca2ProvSSE::Init(void)
{
  return InitSSE() && cSeca2Prov::Init();
}

void cSeca2ProvSSE::PreSSE(unsigned char *data, int pos)
{
  const unsigned char *T1=SSEPT1();
  unsigned char tmpBuf[80];

  data+=pos+5; // Start at offset 5
  for(int i=4; i>=0; i--) {
    int j=i*16;
    PreSSECore(&tmpBuf[j],&data[j],i);
    }
  for(int i=79; i>=0; i--) data[i]=tmpBuf[i]^T1[i];
}

void cSeca2ProvSSE::PostSSE(unsigned char *data, int pos)
{
  PostSSECore1(data,pos);
  // create the SHA hash buffer
  unsigned char tmpBuf[64];
  memcpy(tmpBuf+0,&data[beforeSig+0],4);
  memcpy(tmpBuf+4,&data[afterSig +4],4);
  PostSSECore2(tmpBuf+8,data,pos);
  // Calc Signature of the generated buffer here
  unsigned char MD[20];
  CalcSHASignature(tmpBuf,sizeof(tmpBuf),MD,false);
  // Prepare DES data
  memcpy(tmpBuf+0,&data[beforeSig+4],4);
  memcpy(tmpBuf+4,&data[afterSig +0],4);
  // DES Enrypt
  des.Des(tmpBuf,MD,SECA_DES_ENCR);
  // modify data with encrypted DES data
  PostSSECore3(data,tmpBuf,pos);
  // save the signature
  memcpy(tmpBuf,data+afterSig,8);
  PostSSECore4(data,pos);
  // put the signature in the data
  memcpy(data+beforeSig,tmpBuf,8);
}

// -- France -------------------------------------------------------------------

class cSeca2ProvFR : public cSeca2ProvSSE {
private:
  virtual void PreSSECore(unsigned char *buf, const unsigned char *data, int i);
  virtual void PostSSECore1(unsigned char *data, int pos);
  virtual void PostSSECore2(unsigned char *buf, const unsigned char *data, int pos);
  virtual void PostSSECore3(unsigned char *data, const unsigned char *buf, int pos);
  virtual void PostSSECore4(unsigned char *data, int pos);
protected:
  virtual bool InitSSE(void);
  virtual void PostCW(unsigned char *data);
  virtual void ChainTableXor(unsigned char *data, unsigned short index);
  virtual void SignatureMod(unsigned char *MD, const unsigned char *PK);
public:
  cSeca2ProvFR(unsigned short Id);
  };

static const unsigned short IdsFR[] = { 0x80,0x81,0 };

static cSeca2ProvLinkReg<cSeca2ProvFR> staticLinkFR(IdsFR);

static const struct ProvData provDataFR = {
  0x80,16895,16895,0x7F,0xF7, { 3, 0, 0, 0, 2, 4, 3, 0 }
  };

cSeca2ProvFR::cSeca2ProvFR(unsigned short Id)
:cSeca2ProvSSE(Id)
{
  pData=&provDataFR;
}

bool cSeca2ProvFR::InitSSE(void)
{
  sse=GetMap("sse",5120,true);
  sseP=GetMap("sse",1104,false);
  cw=GetMap("cw",512,false);
  return sse && sseP && cw;
}

void cSeca2ProvFR::PreSSECore(unsigned char *buf, const unsigned char *data, int i)
{
  const unsigned char *T1=SSET1(), *T2=SSET2();

  buf[ 0]=data[0x06]^T2[i+0x0a];
  buf[ 1]=data[0x01]^T2[i+0x00];
  buf[ 2]=data[0x02]^T1[i+0x00];
  buf[ 3]=data[0x03]^T2[i+0x04];
  buf[ 4]=~data[0x04];
  buf[ 5]=data[0x05]^T1[i+0x01];
  buf[ 6]=data[0x00]^T2[i+0x0f];
  buf[ 7]=data[0x07]^T1[i+0x02];
  buf[ 8]=data[0x08]^T1[i+0x00];
  buf[ 9]=data[0x0d]^T2[i+0x14];
  buf[10]=data[0x0f]^T2[i+0x07];
  buf[11]=~data[0x0b];
  buf[12]=data[0x0c]^T1[i+0x00];
  buf[13]=data[0x09]^T2[i+0x19];
  buf[14]=data[0x0e]^T2[i+0x1e];
  buf[15]=data[0x0a]^T1[i+0x01];
}

void cSeca2ProvFR::PostSSECore1(unsigned char *data, int pos)
{
  data[beforeSig+0]^=-(0x2d);
  data[beforeSig+1]-=0x22;
  data[beforeSig+2]^=0x1d;
  data[beforeSig+3]^=-(0x68);
  data[beforeSig+4] =~data[beforeSig + 4];
  data[beforeSig+5]^=0x26;
  data[beforeSig+6]^=0x09;
  data[beforeSig+7]-=0x3e;
  data[afterSig +0]^=-(0x5d);
  data[afterSig +1]-=0x74;
  data[afterSig +2]^=0x2d;
  data[afterSig +3]^=-(0x2a);
  data[afterSig +4]+=0x0d;
  data[afterSig +5]^=-(0x6c);
  data[afterSig +6]^=-(0x76);
  data[afterSig +7]+=0x31;
}

void cSeca2ProvFR::PostSSECore2(unsigned char *buf, const unsigned char *data, int pos)
{
  const unsigned char *T1=SSEPT2();
  memcpy(buf, &T1[data[afterSig+4]+0x48], 56);
}

void cSeca2ProvFR::PostSSECore3(unsigned char *data, const unsigned char *buf, int pos)
{
  data[beforeSig+4]=buf[5];
  data[beforeSig+5]=buf[4];
  data[beforeSig+6]=buf[7];
  data[beforeSig+7]=buf[2];
  data[afterSig +0]=buf[3];
  data[afterSig +1]=buf[1];
  data[afterSig +2]=buf[0];
  data[afterSig +3]=buf[6];
}

void cSeca2ProvFR::PostSSECore4(unsigned char *data, int pos)
{
  data[afterSig+0]=data[beforeSig+3]^0x3e;
  data[afterSig+1]=data[beforeSig+1]^0x5e;
  data[afterSig+2]=data[beforeSig+5]^0x2f;
  data[afterSig+3]=data[beforeSig+0]^0x77;
  data[afterSig+4]=data[beforeSig+6]^-(0x4b);
  data[afterSig+5]=data[beforeSig+2]^-(0x38);
  data[afterSig+6]=data[beforeSig+7]^0x29;
  data[afterSig+7]=data[beforeSig+4]^0x2b;
}

void cSeca2ProvFR::PostCW(unsigned char *data)
{
  const unsigned char *T1=SSET1(), *T2=SSET2(), *T3=SSEPT2(), *T4=CWT1();
  unsigned int idx;
  unsigned char key[8];
  idx=((data[0]<<8)|data[1]);     key[0]=T3[idx & 0x3FF];
  idx=(idx + key[0]);             key[1]=T3[idx & 0x3FF];
  idx=((data[2]<<8)|data[3]);     key[2]=T4[idx & 0x1FF];
  idx=idx + key[2];               key[3]=T4[idx & 0x1FF];
  idx=((data[8+4]<<8)|data[8+5]); key[4]=T2[idx & 0x7FF];
  idx=idx + key[4];               key[5]=T2[idx & 0x7FF];
  idx=((data[8+6]<<8)|data[8+7]); key[6]=T1[idx & 0xBFF];
  idx=idx + key[6];               key[7]=T1[idx & 0xBFF];
  des.Des(data+4,key,SECA_DES_ENCR);
}

void cSeca2ProvFR::ChainTableXor(unsigned char *data, unsigned short index)
{
  static const unsigned char tabIdx[] = { 0x00, 0x08, 0x03, 0x1F, 0x06, 0x32, 0x12, 0x0C };
  static const unsigned char tabXor[] = { 0x77, 0x2B, 0xC8, 0xEE, 0x2F, 0xD3, 0x22, 0x29 };
  static const unsigned char tabPos[] = {    0,    2,    1,    6,    4,    5,    7,    3 };

  unsigned int idx1, idx2;
  unsigned char xorVal=0;
  const unsigned char *T1=MT(), *T2=SSET1();

  idx1 = (index^0x17AC) & 0x3FFF;
  idx2 = idx1 & 0xBFF;
  for(int i=0; i<8; i++) {
    idx1 = (idx1 + tabIdx[i]) & 0x3FFF;
    idx2 = (idx2 + xorVal) & 0xBFF;
    xorVal ^= T1[idx1] ^ tabXor[i];
    data[tabPos[i]] ^= xorVal ^ T2[idx2];
    }
}

void cSeca2ProvFR::SignatureMod(unsigned char *MD, const unsigned char *PK)
{
  const unsigned char *T1=SSEPT2();
  unsigned int idx;

  idx = ((MD[18] << 8) | MD[19]) & 0x3FF;
  MD[10]  = T1[idx];
  MD[13] ^= PK[6];
  idx = ((MD[15] << 8) | MD[13]) & 0x3FF;
  MD[16]  = T1[idx];
  des.Des(MD,MD+10,SECA_DES_ENCR);
}

// -- Netherlands --------------------------------------------------------------

class cSeca2ProvNL : public cSeca2ProvSSE {
private:
  virtual void PreSSECore(unsigned char *buf, const unsigned char *data, int i);
  virtual void PostSSECore1(unsigned char *data, int pos);
  virtual void PostSSECore2(unsigned char *buf, const unsigned char *data, int pos);
  virtual void PostSSECore3(unsigned char *data, const unsigned char *buf, int pos);
  virtual void PostSSECore4(unsigned char *data, int pos);
protected:
  virtual bool InitSSE(void);
  virtual void PostCW(unsigned char *data);
  virtual void ChainTableXor(unsigned char *data, unsigned short index);
  virtual bool DoSigCheck(void) { return false; }
public:
  cSeca2ProvNL(unsigned short Id);
  };

static const unsigned short IdsNL[] = { 0x6a,0 };

static cSeca2ProvLinkReg<cSeca2ProvNL> staticLinkNL(IdsNL);

static const struct ProvData provDataNL = {
  0x6a,16895,16895,0xa3,0x3a, { 3, 0, 0, 0, 2, 4, 3, 0 }
  };

cSeca2ProvNL::cSeca2ProvNL(unsigned short Id)
:cSeca2ProvSSE(Id)
{
  pData=&provDataNL;
}

bool cSeca2ProvNL::InitSSE(void)
{
  sse=GetMap("sse",5120,true);
  sseP=GetMap("sse",336,false);
  cw=GetMap("cw",512,false);
  return sse && sseP && cw;
}

void cSeca2ProvNL::PreSSECore(unsigned char *buf, const unsigned char *data, int i)
{
  const unsigned char *T1=SSET1(), *T2=SSET2();

  buf[0] =data[7] ^T1[i+0x17];
  buf[1] =data[1] ^T2[i+0x07];
  buf[2] =data[2] ^T1[i+0x03];
  buf[3] =data[3] ^T2[i+0x0C];
  buf[4] =~data[4];
  buf[5] =data[5] ^T1[i+0x1E];
  buf[6] =data[6] ^T1[i+0x11];
  buf[7] =data[0] ^T2[i+0x0F];
  buf[8] =data[9] ^T1[i+0x15];
  buf[9] =data[8] ^T2[i+0x04];
  buf[10]=data[12]^T2[i+0x07];
  buf[11]=data[11]^T1[i+0x16];
  buf[12]=data[10]^T1[i+0x01];
  buf[13]=data[13]^T1[i+0x0F];
  buf[14]=~data[14];
  buf[15]=data[15]^T2[i+0x0B];
}

void cSeca2ProvNL::PostSSECore1(unsigned char *data, int pos)
{
  // modify 8 bytes before signature byte (0x82)
  data[beforeSig+0] ^= 0xd6; data[beforeSig+1] ^= 0x96;
  data[beforeSig+2] -= 0x51; data[beforeSig+3] ^= 0x3a;
  data[beforeSig+4] -= 0x8d; data[beforeSig+5] ^= 0xf1;
  data[beforeSig+6] -= 0xc2; data[beforeSig+7] ^= 0xb1;

  data[afterSig+0] ^= 0x84; data[afterSig+1] ^= 0xf8;
  data[afterSig+2] -= 0x7d; data[afterSig+3] = ~(data[afterSig+3]);
  data[afterSig+4] ^= 0xfd; data[afterSig+5] ^= 0xd0;
  data[afterSig+6] ^= 0x77; data[afterSig+7] ^= 0x25;
}

void cSeca2ProvNL::PostSSECore2(unsigned char *buf, const unsigned char *data, int pos)
{
  const unsigned char *T1=SSEPT2();
  memcpy(buf,&T1[(data[afterSig+6]+0x19)&0x7F],56);
}

void cSeca2ProvNL::PostSSECore3(unsigned char *data, const unsigned char *buf, int pos)
{
  data[beforeSig+4]=buf[7];
  data[beforeSig+5]=buf[0];
  data[beforeSig+6]=buf[3];
  data[beforeSig+7]=buf[5];
  data[afterSig+0]=buf[6];
  data[afterSig+1]=buf[2];
  data[afterSig+2]=buf[1];
  data[afterSig+3]=buf[4];
}

void cSeca2ProvNL::PostSSECore4(unsigned char *data, int pos)
{
  data[afterSig+0]=data[beforeSig+1] ^ 0x65; data[afterSig+1]=data[beforeSig+0] ^ 0x75;
  data[afterSig+2]=data[beforeSig+5] ^ 0x35; data[afterSig+3]=data[beforeSig+3] ^ 0xd9;
  data[afterSig+4]=data[beforeSig+6] ^ 0xb7; data[afterSig+5]=data[beforeSig+7] ^ 0x9a;
  data[afterSig+6]=data[beforeSig+4] ^ 0xc7; data[afterSig+7]=data[beforeSig+2] ^ 0x1f;
}

void cSeca2ProvNL::PostCW(unsigned char *data)
{
  const unsigned char *T1=SSET1(), *T2=SSET2(), *T3=CWT1(), *T4=SSEPT2();
  unsigned char key[8];
  unsigned int off2;

  off2=data[2]^data[8+6];
  key[2]=T4[off2]; key[5]=T4[(off2+key[2])&0xff];

  off2=(data[3]<<8)|data[8+4]; key[0]=T3[off2&0x1ff];
  off2+=key[2]; key[4]=T3[off2&0x1ff];

  off2=(data[0]<<8)|data[8+7]; key[7]=T2[off2&0x7ff];
  off2+=key[0]; key[1]=T2[off2&0x7ff];

  off2=(data[1]<<8)|data[8+5]; key[3]=T1[off2&0xbff];
  off2+=key[4]; key[6]=T1[off2&0xbff];

  des.Des(data+4,key,SECA_DES_ENCR);
}

void cSeca2ProvNL::ChainTableXor(unsigned char *data, unsigned short index)
{
  static const unsigned short tabIdx[] = { 0x000, 0x4C3, 0x5D8, 0x63A, 0x471, 0x639, 0x417, 0x6CD };
  static const unsigned char  tabXor[] = {  0x84,  0xD6,  0x3A,  0x1F,  0x25,  0xB1,  0x7D,  0xF7 };
  static const unsigned char tabPos1[] = {     2,     4,     3,     4,     5,     7,     6,     7 };
  static const unsigned char tabPos2[] = {     3,     1,     5,     6,     0,     1,     0,     2 };

  unsigned int idx1, idx2;
  unsigned char xorVal=0;
  const unsigned char *T1=MT(), *T2=SSET1();

  idx1=(index^0x2B36) & 0x3FFF;
  idx2=idx1 & 0xBFF;
  for(int i=0; i<8; i++) {
    idx1=(idx1 + tabIdx[i]) & 0x3FFF;
    idx2=(idx2 + xorVal) & 0xBFF;
    xorVal ^= T1[idx1] ^ tabXor[i];
    data[tabPos1[i]]^=xorVal;
    data[tabPos2[i]]^=T2[idx2];
    }
}

#undef beforeSig
#undef afterSig

// -- Poland -------------------------------------------------------------------

class cSeca2ProvPL : public cSeca2Prov {
protected:
  virtual void PreCW(unsigned char *data);
  virtual void ChainTableXor(unsigned char *data, unsigned short index);
public:
  cSeca2ProvPL(unsigned short Id);
  };

static const unsigned short IdsPL[] = { 0x65,0 };

static cSeca2ProvLinkReg<cSeca2ProvPL> staticLinkPL(IdsPL);

static const struct ProvData provDataPL = {
  0x65,16895,16895,0x96,0x69, { 3, 0, 0, 0, 2, 4, 3, 0 }
  };

static const struct Perm ptPL[MAX_PERMS] = {
    { // 1
    { 2, 4, 4, 3, 2, 2, 4, 4 },
    { 3, 2, 4, 3, 4, 4, 2, 3 },
    { 0, 0, 0, 0, 0, 0, 0, 0 },
    { 0, 0, 0, 0, 0, 0, 0, 0 }
  },{ // 2
    { 4, 4, 3, 2, 2, 3, 3, 4 },
    { 4, 2, 3, 4, 3, 2, 4, 2 },
    { 0, 0, 0, 0, 0, 0, 0, 0 },
    { 0, 0, 0, 0, 0, 0, 0, 0 }
  },{ // 3
  },{ // 4
  },{ // 5
  },{ // 6
  },{ // 7
  },{ // 8
  },{ // 9
    { 4, 4, 3, 2, 3, 4, 3, 2 },
    { 2, 4, 3, 2, 2, 4, 4, 3 },
    { 2, 3, 4, 1, 0, 0, 0, 0 },
    { 0, 0, 0, 0, 0, 0, 0, 0 },
  },{ // 10
  } };

cSeca2ProvPL::cSeca2ProvPL(unsigned short Id)
:cSeca2Prov(Id)
{
  pData=&provDataPL; perm=ptPL;
}

void cSeca2ProvPL::PreCW(unsigned char *data)
{
  static const unsigned char XT[]={ 0xA0,0x12,0x23,0x35,0x46,0xB0,0xDF,0xCA, 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00 };
  static const unsigned char PT[]={ 0x05,0x04,0x06,0x07,0x03,0x00,0x01,0x02, 0x08,0x09,0x0A,0x0B,0x0C,0x0D,0x0E,0x0F };
  unsigned char temp[16];

  xxor(temp,16,data,XT);
  for(int i=15; i>=0; i--) data[i]=temp[PT[i]];
}

void cSeca2ProvPL::ChainTableXor(unsigned char *data, unsigned short index)
{
  const unsigned char *T1=MT();
  for(int i=0; i<8; i++) data[i]^=T1[index++];
}

// -- cSystemSeca ---------------------------------------------------------------

class cSystemSeca : public cSystem, private cSeca {
private:
  cSeca2Prov *sp, *spL;
  int emmLenCnt;
  cDes des;
  cRSA rsa;
  //
  bool GetSeca2Prov(int provId, cSeca2Prov * &sp);
  bool Process0FNano(int count, unsigned char *hashDW, unsigned char *PK, const unsigned char *T1, const unsigned char *T2);
  bool Process51Nano(unsigned char *CW, const unsigned char *PermData, const unsigned char *hashDW, const unsigned char *T1, const unsigned char *T2, int provId);
  void Crypto51Nano(unsigned char *data, const unsigned char *key, const unsigned char crypto, const unsigned char mode);
  void Permute(unsigned char *data, const unsigned char *pdata, const unsigned char *P);
public:
  cSystemSeca(void);
  virtual ~cSystemSeca();
  virtual bool ProcessECM(const cEcmInfo *ecm, unsigned char *data);
  virtual void ProcessEMM(int pid, int caid, unsigned char *buffer);
  };

cSystemSeca::cSystemSeca(void)
:cSystem(SYSTEM_NAME,SYSTEM_PRI),
des(secaPC1,secaPC2)
{
  sp=spL=0; emmLenCnt=0;
  hasLogger=true;
}

cSystemSeca::~cSystemSeca()
{
  delete sp;
  delete spL;
}

bool cSystemSeca::GetSeca2Prov(int provId, cSeca2Prov * &sp)
{
  if(!sp || !sp->Matches(provId)) {
    delete sp;
    sp=cSeca2Providers::GetProv(provId);
    }
  return sp!=0;
}

bool cSystemSeca::ProcessECM(const cEcmInfo *ecmD, unsigned char *data)
{
  bool SE=false;
  unsigned char *ecm;
  const int msgLen=cParseSeca::Payload(data,(const unsigned char **)&ecm);
  const int keyNr=cParseSeca::KeyNr(data);

  if(keyNr&0x80) {
    PRINTF(L_SYS_VERBOSE,"Super Encryption enabled");
    SE=true;
    if(!GetSeca2Prov(ecmD->provId,sp)) {
      if(doLog) PRINTF(L_SYS_ECM,"Seca2 provider %04x not supported",ecmD->provId);
      return false;
      }
    }
  HEXDUMP(L_SYS_VERBOSE,data,SCT_LEN(data),"ECM:");
  if(ecm[0]==0x10) {
    if(msgLen<0x5c) {
      if(doLog) PRINTF(L_SYS_ECM,"ECM length too small (%d)",msgLen);
      return false;
      }

    if(SE) {
      HEXDUMP(L_SYS_VERBOSE,ecm+(msgLen-0x5a),0x5a,"ECM before SSE data modification :");
      sp->PreSSE(ecm,msgLen-0x5a);
      HEXDUMP(L_SYS_VERBOSE,ecm+(msgLen-0x5a),0x5a,"ECM after SSE data modification :");
      }

    const int rsaKeynr=(ecm[1]&0x0F)+'0';
    cBN exp, mod;
    cPlainKey *rsaKey;
    if(!(rsaKey=keys.FindKey('S',ecmD->provId,MBC('E',rsaKeynr),-1))) {
      if(doLog) PRINTF(L_SYS_KEY,"missing %04x E%c key",ecmD->provId,rsaKeynr);
      return false;
      }
    rsaKey->Get(exp);
    if(!(rsaKey=keys.FindKey('S',ecmD->provId,MBC('M',rsaKeynr),-1))) {
      if(doLog) PRINTF(L_SYS_KEY,"missing %04x M%c key",ecmD->provId,rsaKeynr);
      return false;
      }
    rsaKey->Get(mod);
    HEXDUMP(L_SYS_VERBOSE,ecm+(msgLen-0x5a),0x5a,"ECM before RSA decrypt :");
    if(rsa.RSA(ecm+(msgLen-0x5a),ecm+(msgLen-0x5a),0x5a,exp,mod)!=0x5a) {
      PRINTF(L_SYS_CRYPTO,"RSA decrypt failed (ECM)");
      return false;
      }
    HEXDUMP(L_SYS_VERBOSE,ecm+(msgLen-0x5a),0x5a,"ECM after RSA decrypt :");

    if(SE) { // PostSSE needed
      const int hashPtr=msgLen-ecm[msgLen-1]-9;
      if(hashPtr>=msgLen-9 || ecm[hashPtr]!=0x82) {
        PRINTF(L_SYS_CRYPTO,"RSA decrypt failed, signature pointer not found");
        return false;
        }
      HEXDUMP(L_SYS_VERBOSE,ecm+(msgLen-0x5a),0x5a,"ECM before SSE result modification :");
      sp->PostSSE(ecm,hashPtr);
      HEXDUMP(L_SYS_VERBOSE,ecm+(msgLen-0x5a),0x5a,"ECM after SSE result modification :");
      }
    }

  const unsigned char *T1=T1_S1, *T2=T2_S1;
  int decrLen=msgLen;
  if(SE) {
    if(!(T1=sp->T1(KEY2INDEX(keyNr))) ||
       !(T2=sp->T2(KEY2INDEX(keyNr)))) return false;
    decrLen-=ecm[msgLen-1]; if(decrLen<8) return false;
    }

  bool key8=!(cParseSeca::SysMode(data)&0x10);
  cPlainKey *pk=0;
  cKeySnoop ks(this,'S',ecmD->provId,keyNr&0x0F);
  unsigned char *buff=AUTOMEM(msgLen);
  while((pk=keys.FindKey('S',ecmD->provId,keyNr&0x0F,key8?8:16,pk))) {
    memcpy(buff,ecm,msgLen); // if decoding fails we need the original data
    
    unsigned char PK[16], signature[20];
    pk->Get(PK);
    if(key8) memcpy(&PK[8],&PK[0],8); // duplicate key

    if(SE) {
      sp->CalcSHASignature(buff,decrLen-8,signature);
      LDUMP(L_SYS_VERBOSE,signature,8,"signature before encrypt :");
      sp->SignatureMod(signature,PK);
      Encrypt(signature,PK,T1,T2);
      LDUMP(L_SYS_VERBOSE,signature,8,"signature after encrypt :");

      if(sp->DoSigCheck() && memcmp(signature,&buff[decrLen-8],8)) {
        PRINTF(L_SYS_VERBOSE,"signature check failed before SE decrypt");
        continue;
        }
      HEXDUMP(L_SYS_VERBOSE,buff+2,msgLen-2,"ECM before SE decrypt :");
      if(!sp->DecryptSE(buff+2,PK,msgLen-2,8,T1,T2)) {
        PRINTF(L_SYS_VERBOSE,"SE decrypt failed");
        continue;
        }
      HEXDUMP(L_SYS_VERBOSE,buff+2,msgLen-2,"ECM after SE decrypt :");
      }
    else {
      CalcSignature(buff,decrLen-8,signature,PK,T1,T2);
      }
    unsigned char hashDW[28]; // actually 16 bytes used for processing, but SHA needs more
    unsigned char *cCW=0;
    const unsigned char *nano51Data=0;
    int nr0F=0;

    for(int i=0 ; i<decrLen; ) {
      int param=buff[i++];
      int extra=(param >> 4) & 0x0f;
      switch(extra) {
        case 0x0d: extra=0x10; break;
        case 0x0e: extra=0x18; break;
        case 0x0f: extra=0x20; break;
        }
      switch(param) {
        case 0x0f:
          if(SE) nr0F++;
          break;
        case 0x51:
          if(SE) nano51Data=&buff[i];
          break;
        case 0xd1:
          cCW=&buff[i];
          if(SE) {
            sp->CalcSHASignature(buff,i,hashDW,false);
            memcpy(&hashDW[20],hashDW,8);
            Decrypt(&hashDW[20],PK,sp->T1(0),sp->T2(0));
            sp->CalcSHASignature(hashDW,28,hashDW,false);
            }
          break;
        case 0x82:
          if((!SE || sp->DoSigCheck()) && memcmp(&buff[i],signature,8)) { 
            PRINTF(L_SYS_VERBOSE,"signature check failed after decrypt");
            cCW=0; i=decrLen;
            }
          break;
        }
      i+=extra;
      }
    if(cCW) {
      if(!nr0F || Process0FNano(nr0F,hashDW,PK,T1,T2)) {
        if(SE) sp->PreCW(cCW);
        if(!nano51Data || Process51Nano(cCW,nano51Data,hashDW,T1,T2,ecmD->provId)) {
          LDUMP(L_SYS_VERBOSE,cCW,16,"crypted CW :");
          Decrypt(&cCW[0],PK,T1,T2); Decrypt(&cCW[8],PK,T1,T2);
          LDUMP(L_SYS_VERBOSE,cCW,16,"decrypted CW :");
          if(SE) {
            LDUMP(L_SYS_VERBOSE,cCW,16,"decrypted CW before modification :");
            sp->PostCW(cCW);
            LDUMP(L_SYS_VERBOSE,cCW,16,"decrypted CW after modification :");
            }
          memcpy(cw,cCW,16);
          ks.OK(pk);
          return true;
          }
        }
      }
    }
  return false;
}

bool cSystemSeca::Process0FNano(int count, unsigned char *hashDW, unsigned char *PK, const unsigned char *T1, const unsigned char *T2)
{
  const unsigned char *MT=sp->MT();
  unsigned char buffB[8];

  LDUMP(L_SYS_VERBOSE,hashDW,16,"0f: initial hashDW:");
  LDUMP(L_SYS_VERBOSE,PK,16,"0f: PK:");
  while(count--) {
    unsigned char buffA[8];
    swap8_4(hashDW);
    memcpy(buffA,hashDW,8);
    Decrypt(hashDW,PK,sp->T1(0),sp->T2(0));
    LDUMP(L_SYS_VERBOSE,hashDW,16,"0f: hashDW:");

    unsigned char buffE[8];
    int off=WORD(hashDW,0,0x3FFF);
    xxor(buffE,8,&MT[off],&MT[0x3FFF-off]);
    xxor(buffE,8,&MT[( ((buffE[0] ^ hashDW[6])<<8)
                     +  buffE[7] ^ hashDW[7]     )&0x3FFF],buffE);
    LDUMP(L_SYS_VERBOSE,buffE,8,"0f: buffE:");

    unsigned char buffC[16];
    off=WORD(hashDW,2,0x3FFF);
    xxor(buffC,16,&MT[off],&MT[0x3FFF-off]);
    xxor(buffC,16,&MT[( ((hashDW[6] ^ buffC[0])<<8)
                      + (hashDW[7] ^ buffC[15])   )&0x3FFF],buffC);
    LDUMP(L_SYS_VERBOSE,buffC,16,"0f: buffC:");

    off=WORD(hashDW,4,0x3FFF);
    for(int i=0; i<16; ++i) off=WORD(MT,off,0x3FFF);
    if(off+512 >= sp->MTSize()) {
      printf("BUG!!!: error: masking table overflow\n");
      return false;
      }
    PRINTF(L_SYS_VERBOSE,"0f: off=%04x",off);

    memcpy(buffB,buffE,8);
    Decrypt(buffB,buffC,&MT[off],&MT[off+256]);
    LDUMP(L_SYS_VERBOSE,buffB,8,"0f: buffB after 1st decr:");
    xxor(buffB,8,buffB,buffE);
    LDUMP(L_SYS_VERBOSE,buffB,8,"0f: buffB after xor:");
    Decrypt(buffB,PK,T1,T2);
    LDUMP(L_SYS_VERBOSE,buffB,8,"0f: buffB after 2nd decr:");
    xxor(hashDW,8,buffA,buffB);
    }

  for(int i=0; i<8; ++i) {
    PK[i] = buffB[i];
    PK[i+8] = ~sn8(buffB[i]);
    }
  LDUMP(L_SYS_VERBOSE,hashDW,16,"0f: hashDW:");
  LDUMP(L_SYS_VERBOSE,PK,16,"0f: new PK:");
  return true;
}

void cSystemSeca::Crypto51Nano(unsigned char *data, const unsigned char *key, const unsigned char crypto, const unsigned char mode)
{
  switch(crypto) {
    case 0: // Xor
      PRINTF(L_SYS_VERBOSE,"51: XOR crypto selected");
      xxor(data+0,8,key,data+0); xxor(data+8,8,key,data+8);
      break;
    case 1: // Seca crypto, with 9x table, always!
      {
      unsigned char PK[16];
      const unsigned char *T1 = sp->T1(0);
      const unsigned char *T2 = sp->T2(0);
      memcpy(PK+0,key,8); memcpy(PK+8,key,8);
      PRINTF(L_SYS_VERBOSE,"51: SECA crypto selected: %s",mode?"decrypt":"encrypt");
      if(mode) { Decrypt(data,PK,T1,T2); Decrypt(data+8,PK,T1,T2); }
      else     { Encrypt(data,PK,T1,T2); Encrypt(data+8,PK,T1,T2); }
      break;
      }
    case 2: // DES crypto (Modified PC1,PC2)
      PRINTF(L_SYS_VERBOSE,"51: DES crypto selected: %s",mode?"decrypt":"encrypt");
      if(mode) { des.Des(data,key,SECA_DES_DECR); des.Des(data+8,key,SECA_DES_DECR); }
      else     { des.Des(data,key,SECA_DES_ENCR); des.Des(data+8,key,SECA_DES_ENCR); }
      break;
    case 3: // Additional Algo
      PRINTF(L_SYS_VERBOSE,"51: Additional crypto selected: %s",mode?"decrypt":"encrypt");
      AdditionalAlgo(data,key,mode); AdditionalAlgo(data+8,key,mode);
      break;
    }
}

bool cSystemSeca::Process51Nano(unsigned char *CW, const unsigned char *PermData, const unsigned char *hashDW, const unsigned char *T1, const unsigned char *T2, int provId)
{
  const int pMode=(PermData[0]&0x3F)-1;
  const struct Perm *PT=sp->P(pMode);
  if(!PT) {
    PRINTF(L_SYS_VERBOSE,"51: failed to get permutation table");
    return false;
    }
  unsigned char buffF[8], buffG[8];
  switch(pMode) {
    case 0x00:
    case 0x01:
      {
      // Permutation 1
      Permute(buffF,PermData,PT->P1);
      LDUMP(L_SYS_VERBOSE,buffF,8,"51: buffF:");
      xxor(buffG,8,buffF,hashDW);
      const int addAlgoMode=(PermData[0]&1)^1;
      for(int i=(PermData[1]&0x0f); i>0; i--) AdditionalAlgo(buffG,hashDW+8,addAlgoMode);
      // Permutation 2
      Permute(buffF,PermData,PT->P2);
      xxor(buffG,8,buffF,buffG);
      LDUMP(L_SYS_VERBOSE,buffG,8,"51: buffG:");
      // Permutation 3
      Permute(buffF,PermData,PT->P3);
      xxor(buffG,8,buffF,buffG);
      // Permutation 4
      Permute(buffF,PermData,PT->P4);
      xxor(buffG,8,buffF,buffG);
      break;
      }
    case 0x08:
      {
      unsigned char buff1[128], buff2[64], buff3[9];
      // prepare buffers for rsa data
      for(int i=0; i<20; i++)
        buff1[i]=buff2[i]=buff2[i+20]=buff2[i+40]=hashDW[i];
      memcpy(buff3,PermData+2,3);
      for(int i=0; i<8; i++) buff1[i] = buff3[PT->P1[i]-2] ^= buff1[i];
      LDUMP(L_SYS_VERBOSE,buff1,20,"51: permuted hashDW:");
      for(int i=0; i<20; i++)
        buff1[i+24]=buff1[i+44]=buff1[i+68]=buff1[i+88]=buff1[i+108]=buff1[i];
      LBSTARTF(L_SYS_VERBOSE);
      LBPUT("51: nano51Data:");
      for(int i=0; i<4; i++) {
        unsigned char t=PermData[PT->P3[i]];
        if(PT->P3[i]==1) t&=0x0F;
        buff1[i+20]=buff1[i+64]=buff2[i+60]=t;
        LBPUT(" %02x",t);
        }
      LBEND();
      HEXDUMP(L_SYS_VERBOSE,buff1,128,"51: buff1:");
      HEXDUMP(L_SYS_VERBOSE,buff2,64,"51: buff2:");

      memcpy(buff3,buff1,9);
      for(int j=0; j<64; j++) { // XOR even with buff2, odd with 0xFF
        buff1[ j*2   ] ^= buff2[j];
        buff1[(j*2)+1] ^= 0xFF;
        }
      HEXDUMP(L_SYS_VERBOSE,buff1,128,"51: buff1 rsa prepped:");

      // RSA decrypt
      {
      cBN exp, mod;
      cPlainKey *pk;
      if(!(pk=keys.FindKey('S',provId,MBC3('E','9',N51_MAGIC),-1))) {
        if(doLog) PRINTF(L_SYS_KEY,"missing %04x N51 E9 key",provId);
        return false;
        }
      pk->Get(exp);
      if(!(pk=keys.FindKey('S',provId,MBC3('M','9',N51_MAGIC),-1))) {
        if(doLog) PRINTF(L_SYS_KEY,"missing %04x N51 M9 key",provId);
        return false;
        }
      pk->Get(mod);
      if(rsa.RSA(buff1,buff1,128,exp,mod)<9) return false;
      HEXDUMP(L_SYS_VERBOSE,buff1,128,"51: buff1 rsa decrypted:");
      }

      unsigned int sum=0;
      for(int j=0; j<9; j++) {
        unsigned int nextSum=(buff1[j]&0x80) ? 1:0;
        buff1[j]=(sum+2*buff1[j])^buff3[j];
        sum=nextSum;        
        }
      LDUMP(L_SYS_VERBOSE,buff1,9,"51: buff1 after sumalgo:");

      memcpy(buffG,buff1,8);
      memcpy(buff3,PermData+2,3);
      for(int i=0; i<8; i++) buffG[i] = buff3[PT->P2[i]-2] ^= buffG[i];
      break;
      }

    default:
      PRINTF(L_SYS_VERBOSE,"51: data incorrect or proccessing not needed");
      return false;
    }
  LDUMP(L_SYS_VERBOSE,buffG,8,"51: cryptokey:");
  Crypto51Nano(CW,buffG,(PermData[1]>>6)&3,PermData[0]&0x80);
  // Final Permutation
  Permute(buffF,PermData,sp->FP());
  xxor(CW+0,8,CW+0,buffF); xxor(CW+8,8,CW+8,buffF);
  LDUMP(L_SYS_VERBOSE,hashDW,8,"51: cryptokey:");
  Crypto51Nano(CW,hashDW,(PermData[1]>>4)&0x03,PermData[0]&0x40);
  LDUMP(L_SYS_VERBOSE,CW,16,"51: new encrypted CW:");
  return true;
}

void cSystemSeca::Permute(unsigned char *data, const unsigned char *pdata, const unsigned char *P)
{
  for(int i=7; i>=0; i--) data[i] = P[i] ? pdata[P[i]] : 0;
}

void cSystemSeca::ProcessEMM(int pid, int caid, unsigned char *buffer)
{
  if(buffer[0]!=0x84) return; // we only support shared updates

  int msgLen=cParseSeca::CmdLen(buffer);
  if(msgLen!=0x55 && msgLen!=0x63) {
    if(++emmLenCnt<=5)
      PRINTF(L_SYS_EMM,"length of EMM does not match (%02x <> 55/63)%s",msgLen,emmLenCnt==5 ? " ....":"");
    return;
    }
  emmLenCnt=0;
  unsigned char *emm;
  msgLen=cParseSeca::Payload(buffer,(const unsigned char **)&emm);
  int provId=cParseSeca::ProvId(buffer);
  bool SE=false;
  const unsigned char *T1=T1_S1, *T2=T2_S1;
  int decrLen=msgLen;
  if(emm[0]==0x10) { // de-SSE
    const int rsaKeynr = emm[1]+'0';
    cPlainKey *pk;
    cBN exp, mod;
    
    if(!(pk=keys.FindKeyNoTrig('S',provId,MBC3('E',rsaKeynr,EMM_MAGIC),-1))) return;
    pk->Get(exp);
    if(!(pk=keys.FindKeyNoTrig('S',provId,MBC3('M',rsaKeynr,EMM_MAGIC),-1))) return;
    pk->Get(mod);
 
    if(rsa.RSA(emm+2,emm+2,msgLen-2,exp,mod)!=msgLen-2) return;
    const int keyNr=cParseSeca::KeyNr(buffer);
    if(keyNr&0x80) {
      SE=true;
      decrLen-=emm[msgLen-1]; 
      if(!GetSeca2Prov(provId,spL) ||
         !(T1=spL->T1(KEY2INDEX(keyNr))) ||
         !(T2=spL->T2(KEY2INDEX(keyNr))) ||
         decrLen<8) return;
      }
    }

  unsigned char *buff=AUTOMEM(msgLen);
  for(cSecaCardInfo *ci=Scards.First(); ci; ci=Scards.Next(ci)) {
    if(ci->MatchEMM(buffer) || (CheckNull(ci->sa,sizeof(ci->sa)) && ci->MatchID(buffer))) {
      unsigned char MK[16];
      if(cParseSeca::SysMode(buffer)&0x10) {
        if(ci->KeySize()!=16) continue; // don't bother
        memcpy(&MK[0],ci->key,16);
        }
      else {
        memcpy(&MK[0],ci->key,8);
        memcpy(&MK[8],ci->key,8);
        }

      unsigned char signature[20];
      memcpy(buff,emm,msgLen); // if decoding fails we need the original de-sse'd data

      if(!SE) {
        for(int i=0 ; i<=64 && i<msgLen-8; i+=8) Decrypt(&buff[i],MK,T1,T2);
        CalcSignature(buff,decrLen-8,signature,MK,T1,T2);
        }
      else {
        spL->CalcSHASignature(buff,decrLen-8,signature);
        Encrypt(signature,MK,T1,T2);
        if(memcmp(signature,&buff[decrLen-8],8) ||
           !spL->DecryptSE(buff+2,MK,msgLen-2,0,T1,T2)) continue;
        }

      unsigned char keyN[4], *key[4];
      int pos, len;
      unsigned int numKeys=0;
      bool sigOk=false;
      for(pos=0 ; pos<decrLen ; pos+=len) {
        int cmd=buff[pos++];
        len=(cmd>>4)&0x0f;
        switch(len) {
          case 0x0b: len=0x0e; break;
          case 0x0d: len=0x10; break;
          case 0x0e: len=0x18; break;
          case 0x0f: len=0x20; break;
          }
        switch(cmd) {
          case 0x82: // signature
            if(!memcmp(signature,&buff[pos],8)) sigOk=true;
            pos=decrLen;
            break;
          case 0x90: // new primary key
            if(numKeys<sizeof(keyN)) {
              keyN[numKeys]=buff[pos] & 0x0f;
              key[numKeys]=&buff[pos+1];
              numKeys++;
              }
            break;
          }
        }
      char str[20],str2[20],str3[20];
      if(sigOk) {
        LBSTARTF(L_SYS_EMM);
        LBPUT("ID %s SA %s",HexStr(str,SID(buffer),2),HexStr(str2,SA(buffer),3));
        LBPUT(" - OK (CI ID %s SA %s KEY01 %s)",HexStr(str,ci->provId,2),HexStr(str2,ci->sa,3),HexStr(str3,ci->key,8));
        for(unsigned int i=0 ; i<numKeys ; i++) {
          Decrypt(key[i],MK,T1,T2);
          LBPUT(" KEY %02x %s",keyN[i],HexStr(str,key[i],8));
          FoundKey();
          if(keys.NewKey('S',provId,keyN[i],key[i],8)) NewKey();
         }
        LBEND();
        break;
        }
      else if(!CheckNull(ci->sa,sizeof(ci->sa)))
        PRINTF(L_SYS_EMM,"ID %s SA %s - FAIL",HexStr(str,SID(buffer),2),HexStr(str2,SA(buffer),3));
      }
    }
  return;
}

// -- cSystemLinkSeca ----------------------------------------------------------

class cSystemLinkSeca : public cSystemLink {
public:
  cSystemLinkSeca(void);
  virtual bool CanHandle(unsigned short SysId);
  virtual cSystem *Create(void) { return new cSystemSeca; }
  };

static cSystemLinkSeca staticInit;

cSystemLinkSeca::cSystemLinkSeca(void)
:cSystemLink(SYSTEM_NAME,SYSTEM_PRI)
{
  Feature.NeedsKeyFile();
}

bool cSystemLinkSeca::CanHandle(unsigned short SysId)
{
  SysId&=SYSTEM_MASK;
  return SYSTEM_CAN_HANDLE(SysId);
}

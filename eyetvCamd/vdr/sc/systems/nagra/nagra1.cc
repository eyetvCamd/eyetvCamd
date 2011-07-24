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

#include "system.h"
#include "misc.h"
#include "opts.h"
#include "helper.h"

#include "nagra.h"
#include "nagra-def.h"
#include "cpu.h"
#include "log-nagra.h"

#define SYSTEM_NAME          "Nagra"
#define SYSTEM_PRI           -10

// -- cEmu ---------------------------------------------------------------------

#define MAX_COUNT 200000

class cEmu : public c6805 {
protected:
  int romNr, id;
  char *romName, *romExtName, *eepromName;
  //
  int InitStart, InitEnd;
  int EmmStart, EmmEnd, EmmKey0, EmmKey1;
  int FindKeyStart, FindKeyEnd;
  int MapAddr;
  int Rc1H, Rc1L;
  int EnsIrdChk, Cmd83Chk;
  int SoftInt, StackHigh;
  //
  bool AddRom(unsigned short addr, unsigned short size, const char *name);
  bool AddEeprom(unsigned short addr, unsigned short size, unsigned short otpSize, const char *name);
  //
  virtual bool InitSetup(void) { return true; }
  virtual bool UpdateSetup(const unsigned char *emm) { return true; }
  virtual bool MathMapHandler(void);
public:
  cEmu(void);
  virtual ~cEmu();
  bool Init(int RomNr, int Id);
  bool GetOpKeys(const unsigned char *Emm, unsigned char *id, unsigned char *key0, unsigned char *key1);
  bool GetPkKeys(const unsigned char *select, unsigned char *pkset);
  bool Matches(int RomNr, int Id);
  };

cEmu::cEmu(void)
{
  romName=romExtName=eepromName=0;
}

cEmu::~cEmu()
{
  free(romName);
  free(romExtName);
  free(eepromName);
}

bool cEmu::AddRom(unsigned short addr, unsigned short size, const char *name)
{
  cMap *map=new cMapRom(addr,name);
  return AddMapper(map,addr,size);
}

bool cEmu::AddEeprom(unsigned short addr, unsigned short size, unsigned short otpSize, const char *name)
{
  cMap *map=new cMapEeprom(addr,name,otpSize);
  return AddMapper(map,addr,size);
}

bool cEmu::Matches(int RomNr, int Id)
{
  return (romNr==RomNr && id==Id);
}

bool cEmu::Init(int RomNr, int Id)
{
  romNr=RomNr; id=Id;
  asprintf(&romName,"ROM%d.bin",romNr);
  asprintf(&romExtName,"ROM%dext.bin",romNr);
  asprintf(&eepromName,"eep%i_%02x.bin",romNr,(id&0xff00)>>8);
  if(InitSetup()) {
    ForceSet(EnsIrdChk, 0x81,true);
    ForceSet(Cmd83Chk+0,0x98,true);
    ForceSet(Cmd83Chk+1,0x9d,true);
    if(SoftInt) {
      Set(0x1ffc,SoftInt>>8);
      Set(0x1ffd,SoftInt&0xff);
      }
    SetSp(StackHigh,0);
    SetPc(InitStart);
    ClearBreakpoints();
    AddBreakpoint(InitEnd);
    if(!Run(MAX_COUNT)) return true;
    }
  return false;
}

bool cEmu::GetOpKeys(const unsigned char *Emm, unsigned char *id, unsigned char *key0, unsigned char *key1)
{
  int keys=0;
  if(UpdateSetup(Emm)) {
    SetMem(0x0080,&Emm[0],64);
    SetMem(0x00F8,&Emm[1],2);
    SetPc(EmmStart);
    ClearBreakpoints();
    AddBreakpoint(EmmEnd);
    AddBreakpoint(MapAddr);
    AddBreakpoint(EmmKey0);
    AddBreakpoint(EmmKey1);
    while(!Run(MAX_COUNT)) {
      unsigned short pc=GetPc();
      if(pc==EmmKey0) {
        GetMem(0x82,key0,8);
        keys++;
        }
      if(pc==EmmKey1) {
        GetMem(0x82,key1,8);
        keys++;
        }
      if(pc==MapAddr) {
        if(!MathMapHandler()) break;
        PopPc(); // remove return address from stack
        }
      if(pc==EmmEnd) {
        GetMem(0x00F8,id,2);
        break;
        }
      }
    }
  return (keys==2);
}

bool cEmu::GetPkKeys(const unsigned char *select, unsigned char *pkset)
{
  Set(0x0081,select[2]<<7);
  SetMem(0x00F8,select,3);
  SetPc(FindKeyStart);
  ClearBreakpoints();
  AddBreakpoint(FindKeyEnd);
  while(!Run(MAX_COUNT)) {
    unsigned short pc=GetPc();
    if(pc==FindKeyEnd) {
      if(!cc.c) {
        PRINTF(L_SYS_EMU,"Updating PK keys");
        unsigned short loc=(Get(Rc1H)<<8)+Get(Rc1L);
        for(int i=0; i<45; i+=15) GetMem(loc+4+i,pkset+i,15);
        return true;
        }
      else {
        PRINTF(L_SYS_EMU,"Updating PK keys failed. Used a correct EEPROM image for provider %04x ?",((select[0]<<8)|select[1]));
        break;
        }
      }
    }
  return false;
}

bool cEmu::MathMapHandler(void)
{
  PRINTF(L_SYS_EMU,"Unsupported math call $%02x in ROM %d, please report",a,romNr);
  return false;
}

// -- cEmuRom3Core -------------------------------------------------------------

class cEmuRom3Core : public cEmu {
private:
  bool special05;
protected:
  virtual void Stepper(void);
  virtual void WriteHandler(unsigned char seg, unsigned short ea, unsigned char &op);
  virtual void ReadHandler(unsigned char seg, unsigned short ea, unsigned char &op);
  bool CoreInitSetup(void);
  bool DoMaps(bool hasExt);
public:
  cEmuRom3Core(void);
  };

cEmuRom3Core::cEmuRom3Core(void)
{
  special05=false;
}

void cEmuRom3Core::WriteHandler(unsigned char seg, unsigned short ea, unsigned char &op)
{
  if(ea==0x05) special05=(op&0x40)!=0;
}

void cEmuRom3Core::ReadHandler(unsigned char seg, unsigned short ea, unsigned char &op)
{
  if(special05) {
    special05=false; // prevent loop
    unsigned short start=Get(0x30C0);
    unsigned short end=Get(0x30C1);
    if(((ea>>8)>=start) && ((ea>>8)<=end)) op=0x00; // dataspace
    else op=0x01;                                   // codespace
    special05=true;
    }
  else
    switch(ea) {
      case 0x06:
      case 0x07:
        if(!(Get(0x04)&4)) op=random()&0xFF;
        break;
      }
}

void cEmuRom3Core::Stepper(void)
{}

bool cEmuRom3Core::CoreInitSetup(void)
{
  ForceSet(0x0001,1<<3,true);
  Set(0x0002,0x03);
  Set(0x004e,0x4B);
  return true;
}

bool cEmuRom3Core::DoMaps(bool hasExt)
{
  // Eeprom & ROMext are non-fatal also they are required for some providers
  if(hasExt) AddRom(0x2000,0x2000,romExtName);
  AddEeprom(0xE000,0x1000,0x20,eepromName);
  return AddRom(0x4000,0x4000,romName);
}

// -- cEmuRom3 -----------------------------------------------------------------

class cEmuRom3 : public cEmuRom3Core {
protected:
  virtual bool InitSetup(void);
  virtual bool UpdateSetup(const unsigned char *emm);
public:
  cEmuRom3(void);
  };

cEmuRom3::cEmuRom3(void)
{
  InitStart=0x4000;
  InitEnd  =0x734b;
  EmmStart =0x5a82;
  EmmEnd   =0x67db;
  EmmKey0  =0x617c;
  EmmKey1  =0x6184;
  FindKeyStart=0x6133;
  FindKeyEnd  =0x6147;
  MapAddr  =0x2800;
  Rc1H     =0x0024;
  Rc1L     =0x0025;
  EnsIrdChk=0x6437;
  Cmd83Chk =0x6431;
  SoftInt  =0x0000;
  StackHigh=0x7f;
}

bool cEmuRom3::InitSetup(void)
{
  return DoMaps(true) && CoreInitSetup();
}

bool cEmuRom3::UpdateSetup(const unsigned char *emm)
{
  return CoreInitSetup();
}

// -- cEmuRom7 -----------------------------------------------------------------

class cEmuRom7 : public cEmuRom3Core {
protected:
  virtual bool InitSetup(void);
  virtual bool UpdateSetup(const unsigned char *emm);
public:
  cEmuRom7(void);
  };

cEmuRom7::cEmuRom7(void)
{
  InitStart=0x4000;
  InitEnd  =0x7b68;
  EmmStart =0x482b;
  EmmEnd   =0x6146;
  EmmKey0  =0x4f2a;
  EmmKey1  =0x4f32;
  FindKeyStart=0x4ee4;
  FindKeyEnd  =0x4ef5;
  MapAddr  =0x200f;
  Rc1H     =0x0028;
  Rc1L     =0x0029;
  EnsIrdChk=0x51df;
  Cmd83Chk =0x51d9;
  SoftInt  =0x4008;
  StackHigh=0x7f;
}

bool cEmuRom7::InitSetup(void)
{
  return DoMaps(false) && CoreInitSetup();
}

bool cEmuRom7::UpdateSetup(const unsigned char *emm)
{
  SetMem(0x01A2,&emm[1],2);
  return true;
}

// -- cEmuRom10Core ------------------------------------------------------------

class cEmuRom10Core : public cEmu {
protected:
  struct Map { unsigned char A[64], B[64], C[64], D[4], opSize; } map;
  //
  virtual void Stepper(void);
  bool DoMaps(bool hasExt, int romSize);
  bool CoreInitSetup(void);
  bool CoreUpdateSetup(const unsigned char *emm);
  };

void cEmuRom10Core::Stepper(void)
{
  int rnd=random();
  unsigned char mem7=Get(0x07);
  if(cc.i) mem7&=0xFD; else mem7|=0x02;
  Set(0x07,mem7);
  if(bitset(mem7,1) && bitset(mem7,7)) {
    Set(0x05,(rnd&0xFF00)>>8);
    Set(0x06,rnd&0xFF);
    }
}

bool cEmuRom10Core::DoMaps(bool hasExt, int romSize)
{
  // Eeprom & ROMext are non-fatal also they are required for some providers
  if(hasExt) AddRom(0x2000,0x2000,romExtName);
  AddEeprom(0xC000,0x2000,0x40,eepromName);
  return AddRom(0x4000,romSize,romName);
}

bool cEmuRom10Core::CoreInitSetup(void)
{
  ForceSet(0x01,0x13,true);
  Set(0x02,0x3);
  Set(0x07,0xFF);
  return true;
}

bool cEmuRom10Core::CoreUpdateSetup(const unsigned char *emm)
{
  SetMem(0x01A2,&emm[1],2);
  SetMem(0x0307,&emm[1],2);
  Set(0x0300,emm[5]);
  Set(0x0301,emm[2]);
  return true;
}

// -- cEmuRom10 ----------------------------------------------------------------

class cEmuRom10 : public cEmuRom10Core {
protected:
  virtual bool InitSetup(void);
  virtual bool UpdateSetup(const unsigned char *emm);
  virtual bool MathMapHandler(void);
public:
  cEmuRom10(void);
  };

cEmuRom10::cEmuRom10(void)
{
  InitStart=0x4000;
  InitEnd  =0x81ca;
  EmmStart =0x6a71;
  EmmEnd   =0x81f7;
  EmmKey0  =0x7172;
  EmmKey1  =0x717a;
  FindKeyStart=0x712c;
  FindKeyEnd  =0x713d;
  MapAddr  =0x2020;
  Rc1H     =0x004a;
  Rc1L     =0x004b;
  EnsIrdChk=0x7427;
  Cmd83Chk =0x7421;
  SoftInt  =0x4004;
  StackHigh=0x3ff;
}

bool cEmuRom10::InitSetup(void)
{
  return DoMaps(false,0x5A00) && CoreInitSetup();
}

bool cEmuRom10::UpdateSetup(const unsigned char *emm)
{
  Set(0x006e,0x4B);
  return CoreUpdateSetup(emm);
}

bool cEmuRom10::MathMapHandler(void)
{
  PRINTF(L_SYS_EMU,"math call: $%02x",a);
  switch(a) {
    case 0x02:
      switch(Get(0x41)) {
        case 0: map.opSize=0x04; break;
        case 1: map.opSize=0x20; break;
        case 2: map.opSize=0x24; break;
        case 3: map.opSize=0x30; break;
        case 4: map.opSize=0x34; break;
        case 5:
        default: map.opSize=0x40; break;
        }
      return true;

    case 0x0e:
    case 0x0F:
    case 0x10:
      {
      const unsigned short addr=(Get(0x4d)<<8)|Get(0x4e);
      unsigned char tmp[64];
      GetMem(addr,tmp,map.opSize);
      unsigned char *reg;
      switch(a) {
        case 0x0e: reg=map.A; break;
        case 0x0f: reg=map.B; break;
        case 0x10: reg=map.C; break;
        default: return false;
        }
      SetMem(addr,reg,map.opSize);
      memcpy(reg,tmp,map.opSize);
      Set(0x41,map.opSize);
      return true;
      }

    case 0x29:
      Set(0x120,1);
      return true;
    }
  return cEmu::MathMapHandler();
}

// -- cEmuRom11 ----------------------------------------------------------------

class cEmuRom11 : public cEmuRom10Core {
protected:
  virtual bool InitSetup(void);
  virtual bool UpdateSetup(const unsigned char *emm);
public:
  cEmuRom11(void);
  };

cEmuRom11::cEmuRom11(void)
{
  InitStart=0x4000;
  InitEnd  =0x405b;
  EmmStart =0x5865;
  EmmEnd   =0x9990;
  EmmKey0  =0x5f66;
  EmmKey1  =0x5f6e;
  FindKeyStart=0x5f20;
  FindKeyEnd  =0x5f31;
  MapAddr  =0x2020;
  Rc1H     =0x004a;
  Rc1L     =0x004b;
  EnsIrdChk=0x621b;
  Cmd83Chk =0x6215;
  SoftInt  =0x4004;
  StackHigh=0x3ff;
}

bool cEmuRom11::InitSetup(void)
{
  return DoMaps(false,0x8000) && CoreInitSetup();
}

bool cEmuRom11::UpdateSetup(const unsigned char *emm)
{
  Set(0x0068,0x4B);
  return CoreUpdateSetup(emm);
}

// -- cNagraDES ----------------------------------------------------------------

class cNagraDES {
private:
  cDes des;
protected:
  void Decrypt(const unsigned char *data, const unsigned char *key, unsigned char *out, bool desmod=false);
  void Crypt(const unsigned char *data, const unsigned char *key, unsigned char *out);
  bool SigCheck(const unsigned char *block, const unsigned char *sig, const unsigned char *vkey, const int rounds);
};

void cNagraDES::Decrypt(const unsigned char *data, const unsigned char *key, unsigned char *out, bool desmod)
{
  unsigned char cardkey[8];
  memcpy(cardkey,key,8);
  RotateBytes(cardkey,8);
  memcpy(out,data,8);
  if(!desmod) RotateBytes(out,8);
  des.Des(out,cardkey,NAGRA_DES_DECR);
  if(!desmod) RotateBytes(out,8);
}

void cNagraDES::Crypt(const unsigned char *data, const unsigned char *key, unsigned char *out)
{
  unsigned char cardkey[8];
  memcpy(cardkey,key,8);
  RotateBytes(cardkey,8);
  memcpy(out,data,8);
  RotateBytes(out,8);
  des.Des(out,cardkey,NAGRA_DES_ENCR);
  RotateBytes(out,8);
}

bool cNagraDES::SigCheck(const unsigned char *block, const unsigned char *sig, const unsigned char *vkey, const int rounds)
{
  unsigned char hash[8];
  memcpy(hash,vkey,8);
  for(int j=0; j<rounds; j++) {
    unsigned char cr[8];
    Crypt(block+j*8,hash,cr);
    xxor(hash,8,cr,&block[j*8]);
    }
  return (0==memcmp(hash,sig,8));
}

// -- cNagra1 ------------------------------------------------------------------

class cNagra1 : public cNagra, public cNagraDES {
protected:
  virtual void CreatePQ(const unsigned char *key, BIGNUM *p, BIGNUM *q);
  bool DecryptECM(const unsigned char *in, unsigned char *out, const unsigned char *vkey, int len, BIGNUM *e1, BIGNUM *n1, BIGNUM *n2);
  bool DecryptEMM(const unsigned char *in, unsigned char *out, const unsigned char *vkey, int len, BIGNUM *e1, BIGNUM *n1, BIGNUM *n2);
  };

bool cNagra1::DecryptECM(const unsigned char *in, unsigned char *out, const unsigned char *vkey, int len, BIGNUM *e1, BIGNUM *n1, BIGNUM *n2)
{
  cBN result;
  if(rsa.RSA(result,&in[2],len,pubExp,n2)<=0) {
    PRINTF(L_SYS_CRYPTO,"error decrypting ECM (round 1)");
    return false;;
    }
  cBN mod;
  BN_set_word(mod,in[0]>>4);
  BN_lshift(mod,mod,508);
  BN_add(result,result,mod);
  if(rsa.RSA(out,64,result,e1,n1,false)!=64) {
    PRINTF(L_SYS_CRYPTO,"error: result of ECM decryption is not 64 bytes");
    return false;
    }
  if(vkey && !SigCheck(out,&out[56],vkey,7)) {
    PRINTF(L_SYS_CRYPTO,"ECM decryption failed");
    return false;
    }
  return true;
}

bool cNagra1::DecryptEMM(const unsigned char *in, unsigned char *out, const unsigned char *vkey, int len, BIGNUM *e1, BIGNUM *n1, BIGNUM *n2)
{
  cBN result;
  if(rsa.RSA(result,&in[9],len,pubExp,n2)<=0) {
    PRINTF(L_SYS_CRYPTO,"error decrypting EMM (round 1)");
    return false;;
    }
  cBN mod;
  BN_set_word(mod,in[0]>>6);
  BN_lshift(mod,mod,510);
  BN_add(result,result,mod);
  if(rsa.RSA(out,64,result,e1,n1,false)!=64) {
    PRINTF(L_SYS_CRYPTO,"error: result of EMM decryption is not 64 bytes");
    return false;
    }
  if(!vkey || SigCheck(out,&in[1],vkey,8))
    return true;
    
  // We need to use signature exchange method (7blocks emm,1 block signature)
  PRINTF(L_SYS_CRYPTO,"warning: signature failed,trying signature exchange method");
  unsigned char buf[8];
  RotateBytes(buf,&in[1],sizeof(buf));
  cBN newdata, newsig;
  BN_bin2bn(buf,sizeof(buf),newdata);
  BN_copy(newsig,result);
  BN_mask_bits(newsig,64);
  BN_bn2bin(newsig,buf);
  RotateBytes(buf,sizeof(buf));

  BN_rshift(result,result,64); // delete the lower 64 bits,we saved it
  BN_lshift(result,result,64); // before as the new 64 bit signature
  BN_add(result,result,newdata);
  
  if(rsa.RSA(out,64,result,e1,n1)!=64) {
    PRINTF(L_SYS_CRYPTO,"error: result of EMM decryption is not 64 bytes");
    return false;
    }
  if(vkey && !SigCheck(out,buf,vkey,8)) {
    PRINTF(L_SYS_CRYPTO,"fatal: signature failed in signature exchange method");
    return false;
    }
  return true;
}

void cNagra1::CreatePQ(const unsigned char *key, BIGNUM *p, BIGNUM *q)
{
  // Make des_key
  unsigned char des_data[32];
  unsigned char des_key[8], des_tmp[8];
  memcpy(des_data,key,8);
  RotateBytes(des_tmp,key,8);
  des_tmp[7]=0x00;
  Decrypt(des_data,des_tmp,des_key,true);
  RotateBytes(des_key,8);
  des_key[7]=0x00;

  // Calculate P
  for(int i=0; i<4; i++) {
    const int off=i*8;
    memcpy(&des_data[off],&key[4],8);
    des_data[off]^=i;
    Decrypt(&des_data[off],des_key,des_tmp,true);
    memcpy(&des_data[off],des_tmp,8);
    }
  BN_bin2bn(des_data,32,p);
  BN_add_word(p,(key[12]<<4)|((key[13]>>4)&0x0f));
  BN_set_bit(p,(BN_num_bytes(p)*8)-1);

  // Calculate Q
  for(int i=0; i<4; i++) {
    const int off=i*8;
    memcpy(&des_data[off],&key[4],8);
    des_data[off]^=(i+4);
    Decrypt(&des_data[off],des_key,des_tmp,true);
    memcpy(&des_data[off],des_tmp,8);
    }
  BN_bin2bn(des_data,32,q);
  BN_add_word(q,((key[13]&0x0f)<<8)|key[14]);
  BN_set_bit(q,(BN_num_bytes(q)*8)-1);
}

#ifndef TESTER

// -- cSystemNagra -------------------------------------------------------------

class cSystemNagra : public cSystem, public cNagra1 {
private:
  unsigned char mecmTable[256];
  cEmu *emu;
  //
  void WriteTable(unsigned char *from, int off);
public:
  cSystemNagra(void);
  virtual ~cSystemNagra();
  virtual bool ProcessECM(const cEcmInfo *ecm, unsigned char *data);
  virtual void ProcessEMM(int pid, int caid, unsigned char *buffer);
  };

cSystemNagra::cSystemNagra(void)
:cSystem(SYSTEM_NAME,SYSTEM_PRI)
{
  emu=0;
  memset(mecmTable,0,sizeof(mecmTable));
  hasLogger=true;
}

cSystemNagra::~cSystemNagra()
{
  delete emu;
}

void cSystemNagra::WriteTable(unsigned char *from, int off)
{
  off&=0xFF;
  if(off+16<256) memcpy(mecmTable+off,from,16);
  else {
    int l=256-off;
    memcpy(mecmTable+off,from,l);
    memcpy(mecmTable,from+l,16-l);
    }
}

bool cSystemNagra::ProcessECM(const cEcmInfo *ecm, unsigned char *data)
{
  int cmdLen=data[4];
  int id=(data[5]*256)+data[6];
  cTimeMs minTime;
  if(data[3]!=0x03) {
    PRINTF(L_SYS_ECM,"invalid ECM");
    return false;
    }
  data+=7;
  if(data[0]!=0x10) {
    PRINTF(L_SYS_ECM,"no ECM data available");
    return false;
    }
  const int ecmLen=data[1];
  const int verType=(data[2]>>3)&1;
  const int keynr=(data[2]>>4)&1;
  const int ecmParm=data[2]&7;
  bool decrypt;
  if(ecmParm==7) decrypt=false;
  else if(ecmParm==5) decrypt=true;
  else {
    PRINTF(L_SYS_ECM,"unknown ecmParm, ignoring ECM");
    return false;
    }

  cKeySnoop ks(this,'N',id,keynr);
  unsigned char sessionKey[8], verifyKey[8];
  bool hasVerifyKey=false;
  cPlainKey *pk;
  if((pk=keys.FindKey('N',id,ADDC3('V',KEYSET(0,0,verType)),sizeof(verifyKey)))) {
    pk->Get(verifyKey);
    hasVerifyKey=true;
    }
  else if(doLog) PRINTF(L_SYS_KEY,"missing %04X V TYP%d key (non-fatal)",id,verType);
  if(!(pk=keys.FindKey('N',id,keynr,sizeof(sessionKey)))) return false;
  pk->Get(sessionKey);

  const int desLen=(ecmLen-9) & ~7; // datalen - signature - ks byte
  unsigned char *decrypted=AUTOMEM(desLen);
  for(int i=(desLen/8)-1; decrypt && i>=0; i--) {
    const int off=i*8;
    Decrypt(data+11+off,sessionKey,decrypted+off);
    }
  if(decrypt && hasVerifyKey && !SigCheck(decrypted,data+3,verifyKey,desLen/8)) {
    PRINTF(L_SYS_ECM,"signature check failed in DES decrypt");
    return false;
    }
  int cwEvenMecmIndex=-1, cwOddMecmIndex=-1;
  switch(decrypted[0]) {
    case 0x10:  // Whole CW
      cwOddMecmIndex=decrypted[1];
      memcpy(cw+8,&decrypted[2],8);
      cwEvenMecmIndex=decrypted[10];
      memcpy(cw,&decrypted[11],8);
      break;
    case 0x11: // Odd CW
      cwOddMecmIndex=decrypted[1];
      memcpy(cw+8, &decrypted[2],8);
      break;
    case 0x12: // Even CW
      cwEvenMecmIndex=decrypted[1];
      memcpy(cw,&decrypted[2],8);
      break;
    default: PRINTF(L_SYS_ECM,"failed to get CW"); return false;
    }

  const unsigned char * const mecmData=data+(ecmLen+2);
  const int mecmRSALen=mecmData[1]-4;
  if((cmdLen-(ecmLen+2))>64 && (*mecmData==0x20 || *mecmData==0x39)) {
    if(mecmRSALen!=64) {
      if(mecmRSALen>64 && doLog)
        PRINTF(L_SYS_ECM,"ECM too big (len: %d)",mecmRSALen);
      return false;
      }

    const int mecmProvId=mecmData[2]*256+mecmData[3];
    const int keyType=(mecmData[4]>>3)&1;
    const int pkey=mecmData[4]&3;

    cBN e1, n1, n2;
    cPlainKey *pk;
    if(!(pk=keys.FindKey('N',mecmProvId,MBC3('E','1',KEYSET(0,pkey,keyType)),-1))) {
      if(doLog) PRINTF(L_SYS_KEY,"missing %04x E1 PK%d TYP%d key",mecmProvId,pkey,keyType);
      return false;
      }
    pk->Get(e1);
    if(!(pk=keys.FindKey('N',mecmProvId,MBC3('N','1',KEYSET(0,pkey,keyType)),-1)))  {
      if(doLog) PRINTF(L_SYS_KEY,"missing %04x N1 PK%d TYP%d key",mecmProvId,pkey,keyType);
      return false;
      }
    pk->Get(n1);
    if(!(pk=keys.FindKey('N',mecmProvId,MBC3('N','2',KEYSET(0,0,keyType)),-1)))  {
      if(doLog) PRINTF(L_SYS_KEY,"missing %04x N2 TYP%d key",mecmProvId,keyType);
      return false;
      }
    pk->Get(n2);
    hasVerifyKey=false;
    if((pk=keys.FindKey('N',mecmProvId,ADDC3('V',KEYSET(0,0,keyType)),sizeof(verifyKey))))  {
      pk->Get(verifyKey);
      hasVerifyKey=true;
      }
    else if(doLog) PRINTF(L_SYS_KEY,"missing %04x V TYP%d key (non-fatal)",mecmProvId,keyType);
    unsigned char *decrMecmData=AUTOMEM(mecmRSALen);
    if(!DecryptECM(&mecmData[4],decrMecmData,hasVerifyKey?verifyKey:0,mecmRSALen,e1,n1,n2))
      return false;

    if(*mecmData==0x39 || (*mecmData==0x20 && (mecmProvId&0xFE00)==0x4800)) {
      unsigned char xor_table[64];
      for(int i=sizeof(xor_table)-1; i>=0; i--) xor_table[i]=63-i;
      CreateRSAPair(&decrMecmData[24],xor_table,e1,n1);

      // new XOR table for MECM data
      cBNctx ctx;
      cBN x;
      BN_mod_exp(x,e1,pubExp,n1,ctx);
      int l=sizeof(xor_table)-BN_num_bytes(x);
      memset(xor_table,0,l);
      BN_bn2bin(x,xor_table+l);
      RotateBytes(xor_table,sizeof(xor_table));

      // And finally, new MECM table
      for(int i=39; i<mecmRSALen; i++) decrMecmData[i]^=xor_table[i-39];
      memcpy(&decrMecmData[3],&decrMecmData[39],mecmRSALen-39);
      }

    if(decrMecmData[0]==0x2F && decrMecmData[1]==mecmData[2] && decrMecmData[2]==mecmData[3])
      WriteTable(decrMecmData+4,decrMecmData[3]*2);
    }

  if(cwOddMecmIndex>=0 && cwOddMecmIndex<0x80) {
    const int d=cwOddMecmIndex*2;
    for(int i=0 ; i<8 ; i++) cw[i+8]^=mecmTable[(d+i)&0xFF]; // odd
    }
  if(cwEvenMecmIndex>=0 && cwEvenMecmIndex<0x80) {
    const int d=cwEvenMecmIndex*2;
    for(int i=0 ; i<8 ; i++) cw[i]^=mecmTable[(d+i)&0xFF]; // even
    }
  ks.OK(pk);
  int i=minEcmTime-minTime.Elapsed();
  if(i>0) cCondWait::SleepMs(i);

  return true;
}
  
void cSystemNagra::ProcessEMM(int pid, int caid, unsigned char *buffer)
{
  const int id=buffer[10]*256+buffer[11];
  static const unsigned char tester[] = { 0x00,0x00,0x00,0x00,0x00,0x00,0x4B };
  if(memcmp(&buffer[3],tester,sizeof(tester)) || SCT_LEN(buffer)<(12+9+64)) {
    PRINTF(L_SYS_EMM,"%d: not a valid EMM structure",CardNum());
    return;
    }
  const int pkey=(buffer[12]&3);
  cPlainKey *pk;
  cBN e1, n1, n2;
  if(!(pk=keys.FindKey('N',id,MBC3('E','1',KEYSET(0,pkey,0)),-1))) return;
  pk->Get(e1);
  if(!(pk=keys.FindKey('N',id,MBC3('N','1',KEYSET(0,pkey,0)),-1))) return;
  pk->Get(n1);
  unsigned char v[8];
  bool hasVerifyKey=false;
  if((pk=keys.FindKey('N',id,ADDC3('V',KEYSET(0,0,0)),sizeof(v)))) {
    pk->Get(v);
    hasVerifyKey=true;
    }
  int mode=(buffer[12]&8) ? 0:2;
  unsigned char emmdata[64];
  bool r;
  do {
    switch(mode) {
      case 0: // get the ROM10 specific management key
        if(!(pk=keys.FindKey('N',id,MBC3('N','2',KEYSET(10,0,1)),-1))) return;
        pk->Get(n2);
        mode=1; break;
      case 1: // get the ROM11 specific management keys
        if(!(pk=keys.FindKey('N',id,MBC3('E','1',KEYSET(11,pkey,1)),-1))) return;
        pk->Get(e1);
        if(!(pk=keys.FindKey('N',id,MBC3('N','1',KEYSET(11,pkey,1)),-1))) return;
        pk->Get(n1);
        if(!(pk=keys.FindKey('N',id,MBC3('N','2',KEYSET(11,0,1)),-1))) return;
        pk->Get(n2);
        hasVerifyKey=false;
        if((pk=keys.FindKey('N',id,ADDC3('V',KEYSET(11,0,1)),sizeof(v)))) {
          pk->Get(v);
          hasVerifyKey=true;
          }
        mode=-1; break;
      case 2: // get the universal management key
        if(!(pk=keys.FindKey('N',id,MBC3('N','2',KEYSET(0,0,0)),-1))) return;
        pk->Get(n2);
        mode=-1; break;
      }
    r=DecryptEMM(&buffer[12],emmdata,hasVerifyKey?v:0,64,e1,n1,n2);
    } while(!r && mode>=0);
  if(r) {
    HEXDUMP(L_SYS_RAWEMM,emmdata,sizeof(emmdata),"Nagra1 RAWEMM");
    unsigned int i=0;
    bool gotKeys=false;
    unsigned char key0[8], key1[8];
    int keyId=(emmdata[i+1]<<8)+emmdata[i+2];
    switch(emmdata[i]) { // Check filter type
      case 0x00: // One card
        i+=7; break;
      case 0x01: case 0x02: case 0x03: case 0x04: // Group of cards
      case 0x05: case 0x06: case 0x07: case 0x08:
      case 0x09: case 0x0A: case 0x0B: case 0x0C:
      case 0x0D: case 0x0E: case 0x0F: case 0x10:
      case 0x11: case 0x12: case 0x13: case 0x14:
      case 0x15: case 0x16: case 0x17: case 0x18:
      case 0x19: case 0x1A: case 0x1B: case 0x1C:
      case 0x1D: case 0x1E: case 0x1F: case 0x20:
        i+=emmdata[i]+0x7; break;
      case 0x3e:
        i+=6; break;
      case 0x3d: // All cards with same system ID
      case 0x3f:
        i+=3; break;
      }
    int nrKeys=0;
    while(i<sizeof(emmdata)) {
      if((emmdata[i]&0xF0)==0xF0) { // Update with CPU code
        const int romNr=emmdata[i]&0x0F;
        if(!emu || !emu->Matches(romNr,id)) {
          delete emu; emu=0;
          PRINTF(L_SYS_EMM,"%d: setting defaults for ROM %d",CardNum(),romNr);
          switch(romNr) {
            case 3:  emu=new cEmuRom3;  break;
            case 7:  emu=new cEmuRom7;  break;
            case 10: emu=new cEmuRom10; break;
            case 11: emu=new cEmuRom11; break;
            default: PRINTF(L_SYS_EMM,"%d: unsupported ROM",CardNum()); return;
            }
          if(!emu || !emu->Init(romNr,id)) {
            delete emu; emu=0;
            PRINTF(L_SYS_EMM,"%d: initialization failed for ROM %d",CardNum(),romNr);
            return;
            }
          }
        unsigned char ki[2];
        if((gotKeys=emu->GetOpKeys(emmdata,ki,key0,key1))) {
          keyId=(ki[0]<<8)+ki[1];
          PRINTF(L_SYS_EMM,"%d: got keys for %04X (ROM %d)",CardNum(),keyId,romNr);
          }
        unsigned char select[3], pkset[3][15];
        select[0]=(keyId>>8)|0x01; // always high id for ECM RSA keys
        select[1]=keyId&0xFF;
        select[2]=0; // type 0
        if(emu->GetPkKeys(&select[0],&pkset[0][0])) {
          int pkKeyId=((select[0]<<8)+select[1]);
          PRINTF(L_SYS_EMM,"%d: got PK keys for %04X (ROM %d)",CardNum(),pkKeyId,romNr);
          for(int i=0; i<3; i++) {
            CreateRSAPair(pkset[i],0,e1,n1);
            FoundKey();
            if(keys.NewKey('N',pkKeyId,ADDC3(MBC('N','1'),KEYSET(0,i,0)),n1,64)) NewKey();
            FoundKey();
            if(keys.NewKey('N',pkKeyId,ADDC3(MBC('E','1'),KEYSET(0,i,0)),e1,64)) NewKey();
            }
          }
        break; // don't process other nanos
        }
      else if(emmdata[i]==0x60) { // NULL nano
        i+=2;
        }
      else if(emmdata[i]==0x00) {
        i++;
        }
      else if(emmdata[i]==0x81) {
        i++;
        }
      else if(emmdata[i]==0x83) {
        keyId=(emmdata[i+1]<<8)+emmdata[i+2];
        i+=3;
        }
      else if(emmdata[i]==0x42) { // plain Key
        if(emmdata[i+1]==0x05) memcpy(key0,&emmdata[i+2],sizeof(key0));
        else                   memcpy(key1,&emmdata[i+2],sizeof(key1));
        i+=10;
        if(++nrKeys==2) {
          gotKeys=true;
          PRINTF(L_SYS_EMM,"%d: got keys for %04X (plain)",CardNum(),keyId);
          break;
          }
        }
      else {
        PRINTF(L_SYS_EMM,"%d: ignored nano %02x",CardNum(),emmdata[i]);
        break;
        }
      }
    if(gotKeys) {
      FoundKey();
      if(keys.NewKey('N',keyId,00,key0,8)) NewKey();
      FoundKey();
      if(keys.NewKey('N',keyId,01,key1,8)) NewKey();
      }
    }
}

// -- cSystemLinkNagra ---------------------------------------------------------

class cSystemLinkNagra : public cSystemLink {
public:
  cSystemLinkNagra(void);
  virtual bool CanHandle(unsigned short SysId);
  virtual cSystem *Create(void) { return new cSystemNagra; }
  };

static cSystemLinkNagra staticInitN1;

cSystemLinkNagra::cSystemLinkNagra(void)
:cSystemLink(SYSTEM_NAME,SYSTEM_PRI)
{
  opts=new cOpts(SYSTEM_NAME,1);
  opts->Add(new cOptInt("MinEcmTime",trNOOP("Nagra: min. ECM processing time"),&minEcmTime,0,5000));
  Feature.NeedsKeyFile();
}

bool cSystemLinkNagra::CanHandle(unsigned short SysId)
{
  return SysId==SYSTEM_NAGRA; // || SysId==SYSTEM_NAGRA_BEV;
}

#endif //TESTER

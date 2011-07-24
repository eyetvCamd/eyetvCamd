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
#include <stdlib.h>
#include <string.h>

#include "nagra2.h"

// -- cMap4101 -----------------------------------------------------------------

class cMap4101 : public cMapCore {
private:
  int mId;
protected:
  void DoMap(int f, unsigned char *data=0, int l=0);
public:
  cMap4101(int Id);
  };

cMap4101::cMap4101(int Id)
{
  mId=Id|0x100;
}

void cMap4101::DoMap(int f, unsigned char *data, int l)
{
  PRINTF(L_SYS_MAP,"%04x: calling function %02X",mId,f);
  switch(f) {
    case 0x58:
      {
      cBN a, b, x, y, scalar;
      D.GetLE(data+0x00,16);
      x.GetLE(data+0x10,16);
      y.GetLE(data+0x20,16);
      b.GetLE(data+0x30,16);
      a.GetLE(data+0x40,16);
      scalar.GetLE(data+0x50,16);
      int scalarbits=BN_num_bits(scalar);
      if(scalarbits>=2 && !BN_is_zero(x) && !BN_is_zero(y) && !BN_is_zero(b)) {
        CurveInit(a);
        ToProjective(0,x,y);
        BN_copy(Qx,Px);
        BN_copy(Qy,Py);
        for(int i=scalarbits-2; i>=0; i--) {
          DoubleP(0);
          if(BN_is_bit_set(scalar,i)) AddP(0);
          }
        ToAffine();
        }
      memset(data,0,64);
      Px.PutLE(&data[0],16);
      Py.PutLE(&data[32],16);
      break;
      }
    case 0x44:
      sctx.h4=UINT32_LE(data);
      sctx.h3=UINT32_LE(data+4);
      sctx.h2=UINT32_LE(data+8);
      sctx.h1=UINT32_LE(data+12);
      sctx.h0=UINT32_LE(data+16);
      cMapCore::DoMap(0x44,data);
      break;
    default:
      if(!cMapCore::DoMap(f,data,l))
        PRINTF(L_SYS_MAP,"%04x: unsupported call %02x",mId,f);
      break;
    }
}

// -- cN2Prov4101 ----------------------------------------------------------------

class cN2Prov4101 : public cN2Prov, public cMap4101, public cN2Emu {
protected:
  virtual void WriteHandler(unsigned char seg, unsigned short ea, unsigned char &op);
public:
  cN2Prov4101(int Id, int Flags);
  virtual bool PostProcAU(int id, unsigned char *data);
  virtual int ProcessBx(unsigned char *data, int len, int pos);
  };

static cN2ProvLinkReg<cN2Prov4101,0x4101,(N2FLAG_POSTAU|N2FLAG_Bx)> staticPL4101;

cN2Prov4101::cN2Prov4101(int Id, int Flags)
:cN2Prov(Id,Flags)
,cMap4101(Id)
{
  hasWriteHandler=true;
}

bool cN2Prov4101::PostProcAU(int id, unsigned char *data)
{
  if(data[1]==0x01) {
    cPlainKey *pk;
    if(!(pk=keys.FindKeyNoTrig('N',id,MBC(N2_MAGIC,0x30),16))) {
      PRINTF(L_SYS_EMM,"missing %04x NN 30 3DES key (16 bytes)",id);
      return false;
      }
    unsigned char dkey[16];
    pk->Get(dkey);
    DES_key_schedule ks1, ks2;
    DES_key_sched((DES_cblock *)&dkey[0],&ks1);
    DES_key_sched((DES_cblock *)&dkey[8],&ks2);
    DES_ecb2_encrypt(DES_CAST(&data[7]),DES_CAST(&data[7]),&ks1,&ks2,DES_DECRYPT);
    DES_ecb2_encrypt(DES_CAST(&data[7+8]),DES_CAST(&data[7+8]),&ks1,&ks2,DES_DECRYPT);
    }
  return true;
}

int cN2Prov4101::ProcessBx(unsigned char *data, int len, int pos)
{
  if(Init(id,110)) {
    SetMem(0x80,data,len);
    SetPc(0x80+pos);
    SetSp(0x0FFF,0x0FE0);
    ClearBreakpoints();
    AddBreakpoint(0xACDD);
    AddBreakpoint(0x9BDD);
    AddBreakpoint(0x7EC5);
    while(!Run(5000)) {
      switch(GetPc()) {
        case 0xACDD:
          return -1;
        case 0x9BDD:
          GetMem(0x80,data,len);
          return a;
        case 0x7EC5:
          {
          PopPc();
          unsigned short args=GetPc();
          int clen=Get(args+4);
          unsigned short ptr=(Get(args)<<8)+Get(args+1);
          if(clen+pos-1>len) return -1; // sanity
          GetMem(ptr,&data[pos-1],clen);
          ptr=(Get(args+2)<<8)+Get(args+3)-0x0CED+0x00B4-0x0080; // wild hack
          if(ptr<7 || ptr+8>len || data[ptr-1]!=0x10 || data[ptr-7]!=0x42) // sanity
            return -1;
          unsigned char tmp[96];
          memcpy(&tmp[0],&data[ptr],8);
          ptr=(((data[ptr]&0x3F)|0x40)<<8)+(data[ptr+1]&0x7F);
          GetMem(ptr,&tmp[8],sizeof(tmp)-8);
          DoMap(0x02,0,2);
          DoMap(0x58,tmp);
          DoMap(0x43);
          DoMap(0x44,tmp);
          SetMem(0x0440,tmp,20);
          SetMem(0x80,data,len);
          SetPc(0x80+pos);
          break;
          }
        }
      }
    }
  return -1;
}

void cN2Prov4101::WriteHandler(unsigned char seg, unsigned short ea, unsigned char &op)
{
  if(cr==0x00) {
    if(ea==0x0a || ea==0x12 || ea==0x16) {
      unsigned char old=Get(ea);
      if(old&2) op=(old&~0x02) | (op&0x02);
      }
    }
}

// -- cN2Prov7101 ----------------------------------------------------------------

static cN2ProvLinkReg<cN2Prov4101,0x7101,(N2FLAG_POSTAU)> staticPL7101;

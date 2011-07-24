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

#include "nagra.h"
#include "nagra-def.h"
#include "log-nagra.h"
#include "log-core.h"

static const struct LogModule lm_sys = {
  (LMOD_ENABLE|L_SYS_ALL)&LOPT_MASK,
  (LMOD_ENABLE|L_SYS_DEFDEF|L_SYS_EMU|L_SYS_DISASM80|L_SYS_MAP)&LOPT_MASK,
  "nagra",
  { L_SYS_DEFNAMES,"emu","disasm","disasm80","cpuStats","map","rawemm","rawecm" }
  };
ADD_MODULE(L_SYS,lm_sys)

int minEcmTime=400; // ms

// -- cPlainKeyNagra -----------------------------------------------------------

#define PLAINLEN_NAGRA_H 8
#define PLAINLEN_NAGRA_I 16
#define PLAINLEN_NAGRA_B 64
#define PLAINLEN_NAGRA_IE 24
#define PLAINLEN_NAGRA_BE 96

static cPlainKeyTypeReg<cPlainKeyNagra,'N'> KeyReg;

cPlainKeyNagra::cPlainKeyNagra(bool Super)
:cDualKey(Super,true)
{}

bool cPlainKeyNagra::IsBNKey(int kn)
{
  switch(kn & C2MASK) {
    // Nagra1
    case MBC('M','1'):
    case MBC('E','1'):
    case MBC('N','1'):
    case MBC('N','2'):
    // Nagra2
    case MBC(N2_MAGIC,N2_EMM_G_R):
    case MBC(N2_MAGIC,N2_EMM_S_R):
    case MBC(N2_MAGIC,N2_EMM_G_R|N2_EMM_SEL):
    case MBC(N2_MAGIC,N2_EMM_S_R|N2_EMM_SEL):
      return true;
    }
  return false;
}

bool cPlainKeyNagra::IsBNKey(void) const
{
  return IsBNKey(keynr);
}

int cPlainKeyNagra::ParseTag(const char *tag, const char * &line)
{
  int r=-1;
  const int l=strlen(tag);
  if(!strncasecmp(line,tag,l)) {
    line+=l; r=0;
    while(*line!=0 && isdigit(*line)) {
      r=r*10 + *line-'0';
      line++;
      }
    line=skipspace(line);
    }
  return r;
}

void cPlainKeyNagra::GetTagDef(int nr, int &romnr, int &pk, int &keytype)
{
  romnr  =DEF_ROM;
  pk     =DEF_PK;
  keytype=DEF_TYPE;
  nr&=C2MASK;
  if(nr=='V' || nr==MBC('N','2') || nr==MBC('M','1'))
    pk=0; // different default
}

bool cPlainKeyNagra::Parse(const char *line)
{
  unsigned char sid[2];
  int len;
  if(GetChar(line,&type,1) && (len=GetHex(line,sid,2,false))) {
    type=toupper(type); id=Bin2Int(sid,len); 
    line=skipspace(line);
    bool ok;
    if(!strncasecmp(line,"NN",2)) {
      line=skipspace(line+2);
      unsigned char skeynr;
      if((ok=GetHex(line,&skeynr,1)))
        keynr=MBC(N2_MAGIC,skeynr);
      }
    else {
      bool tags;
      switch(toupper(*line)) {
        case 'E':
        case 'N': ok=GetChar(line,&keynr,2); tags=true; break;
        case 'M': ok=GetChar(line,&keynr,2); tags=false; break;
        case 'V': ok=GetChar(line,&keynr,1); tags=true; break;
        default:  {
                  unsigned char skeynr;
                  ok=GetHex(line,&skeynr,1); keynr=skeynr; tags=false; break;
                  }
        }
      if(ok) {
        int romnr, pk, keytype;
        GetTagDef(keynr,romnr,pk,keytype);
        line=skipspace(line);
        while(tags) {
          int r;
          if((r=ParseTag("ROM",line))>=0) {
            if(r>0 && r<=15) romnr=r;
            else PRINTF(L_CORE_LOAD,"nagrakey: ignoring bad ROM number %d",r);
            }
          else if((r=ParseTag("TYP",line))>=0) {
            if(r>=0 && r<=1) keytype=r;
            else PRINTF(L_CORE_LOAD,"nagrakey: ignoring bad key type %d",r);
            }
          else if((r=ParseTag("PK",line))>=0) {
            if(r>=0 && r<=2) pk=r;
            else PRINTF(L_CORE_LOAD,"nagrakey: ignoring bad PK key number %d",pk);
            }
          else {
            tags=false;
            if(pk!=0 && (keynr=='V' || keynr==MBC('N','2'))) {
              pk=0;
              PRINTF(L_CORE_LOAD,"nagrakey: auto-adjusting to PK0 for N2/V key");
              }
            if(romnr!=0 && keytype!=1) {
              keytype=1;
              PRINTF(L_CORE_LOAD,"nagrakey: auto-adjusting to TYP1 for ROM key");
              }
            }
          }
        if(IsBNKey() || keynr=='V') keynr=ADDC3(keynr,KEYSET(romnr,pk,keytype));
        }
      }

    unsigned char skey[PLAINLEN_NAGRA_BE];
    int keylen=PLAINLEN_NAGRA_BE;
    len=GetHex(line,skey,keylen,false);
    if(   (!IsBNKey() && (len==PLAINLEN_NAGRA_H || len==PLAINLEN_NAGRA_I || len==PLAINLEN_NAGRA_IE))
       || ( IsBNKey() && (len==PLAINLEN_NAGRA_B || len==PLAINLEN_NAGRA_BE)))
      keylen=len;
    if(len==keylen) {
      if((keynr=='V' || keynr==MBC(N2_MAGIC,0x03) || keynr==MBC(N2_MAGIC,(0x03|N2_EMM_SEL))) && CheckNull(skey,len))
        PRINTF(L_GEN_WARN,"nagrakey: FAKE verify keys will cause problems. Please remove them!");
      SetBinKey(skey,len);
      return true;
      }
    }
  return false;
}

cString cPlainKeyNagra::PrintKeyNr(void)
{
  if(C2(keynr)==N2_MAGIC)
    return cString::sprintf("NN %.2X",keynr&0xFF);
  int k=keynr & C2MASK;
  if(!IsBNKey() && k!='V')
    return cString::sprintf("%.2X",keynr);
  char nr[32];
  int q=0;
  if(IsBNKey()) nr[q++]=(keynr>>8) & 0xff;
  nr[q++]=keynr & 0xff;
  nr[q]=0;
  int romnr, pk, keytype;
  GetTagDef(keynr,romnr,pk,keytype);
  if(ROM(keynr) !=romnr  ) q+=snprintf(nr+q,sizeof(nr)-q," ROM%d",ROM(keynr));
  if(PK(keynr)  !=pk     ) q+=snprintf(nr+q,sizeof(nr)-q," PK%d",PK(keynr));
  if(TYPE(keynr)!=keytype) q+=snprintf(nr+q,sizeof(nr)-q," TYP%d",TYPE(keynr));
  return nr;
}

// -- cNagra -------------------------------------------------------------------

cNagra::cNagra(void)
{
  BN_set_word(pubExp,3);
}

void cNagra::CreateRSAPair(const unsigned char *key, const unsigned char *data, BIGNUM *e, BIGNUM *m)
{
  // Calculate P and Q from data
  cBN p,q;
  CreatePQ(key,p,q);
  ExpandPQ(p,q,data,e,m);
}

void cNagra::ExpandPQ(BIGNUM *p, BIGNUM *q, const unsigned char *data, BIGNUM *e, BIGNUM *m)
{
  // Calculate N=P*Q (modulus)
  cBNctx ctx;
  BN_mul(m,p,q,ctx);
  if(data) BN_bin2bn(data,64,e); // set provided data as E1
  else {                         // else calculate the 'official' one
    // E = ( ( ( (P-1) * (Q-1) * 2) + 1) / 3)
    BN_sub_word(p,1);
    BN_sub_word(q,1);
    BN_mul(e,p,q,ctx);
    BN_mul_word(e,2);
    BN_add_word(e,1);
    BN_div_word(e,3);
    }
}

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
#include <stdlib.h>
#include <ctype.h>

#include "../tools.h"

#include "system-common.h"
#include "data.h"
#include "crypto-bn.h"

// -- cHexKey ------------------------------------------------------------------

cHexKey::cHexKey(bool Super)
:cPlainKey(Super)
{
  key=0;
}

cHexKey::~cHexKey()
{
  free(key);
}

bool cHexKey::SetKey(void *Key, int Keylen)
{
  keylen=Keylen;
  free(key); key=MALLOC(unsigned char,keylen);
  if(key) memcpy(key,Key,keylen);
  return key!=0;
}

bool cHexKey::SetBinKey(unsigned char *Mem, int Keylen)
{
  return SetKey(Mem,Keylen);
}

bool cHexKey::Cmp(void *Key, int Keylen)
{
  return keylen==Keylen && memcmp(key,Key,keylen)==0;
}

bool cHexKey::Cmp(cPlainKey *k)
{
  cHexKey *hk=dynamic_cast<cHexKey *>(k); // downcast
  return hk && Cmp(hk->key,hk->keylen);
}

void cHexKey::Get(void *mem)
{
  memcpy(mem,key,keylen);
}

cString cHexKey::Print(void)
{
  char *str=(char *)malloc(keylen*2+2);
  if(str) HexStr(str,key,keylen);
  return cString(str,true);
}

// -- cBNKey -------------------------------------------------------------------

cBNKey::cBNKey(bool Super, bool Rotate)
:cPlainKey(Super)
{
  key=new cBN; rotate=Rotate;
}

cBNKey::~cBNKey()
{
  delete key;
}

bool cBNKey::SetKey(void *Key ,int Keylen)
{
  keylen=Keylen;
  return BN_copy(*key,(BIGNUM *)Key)!=0;
}

bool cBNKey::SetBinKey(unsigned char *Mem, int Keylen)
{
  keylen=Keylen;
  return rotate ? key->GetLE(Mem,Keylen) : key->Get(Mem,Keylen);
}

bool cBNKey::Cmp(void *Key, int Keylen)
{
  return BN_cmp(*key,(BIGNUM *)Key)==0;
}

bool cBNKey::Cmp(cPlainKey *k)
{
  cBNKey *bk=dynamic_cast<cBNKey *>(k); // downcast
  return bk && BN_cmp(*key,*bk->key)==0;
}

void cBNKey::Get(void *mem)
{
  BN_copy((BIGNUM *)mem,*key);
}

cString cBNKey::Print(void)
{
  unsigned char *mem=AUTOMEM(keylen);
  if(rotate) key->PutLE(mem,keylen); else key->Put(mem,keylen);
  char *str=(char *)malloc(keylen*2+2);
  if(str) HexStr(str,mem,keylen);
  return cString(str,true);
}

// -- cDualKey -----------------------------------------------------------------

cDualKey::cDualKey(bool Super, bool Rotate)
:cMutableKey(Super)
{
  rotate=Rotate;
}

cPlainKey *cDualKey::Alloc(void) const
{
  if(IsBNKey()) return new cBNKey(CanSupersede(),rotate);
  else          return new cHexKey(CanSupersede());
}

// -- cPlainKeyStd ---------------------------------------------------------------

#define PLAINLEN_STD 8

cPlainKeyStd::cPlainKeyStd(bool Super)
:cHexKey(Super)
{}

bool cPlainKeyStd::Parse(const char *line)
{
  unsigned char sid[3], skeynr, skey[PLAINLEN_STD];
  int len;
  if(GetChar(line,&type,1) && (len=GetHex(line,sid,3,false)) &&
     GetHex(line,&skeynr,1) && GetHex(line,skey,PLAINLEN_STD)) {
    type=toupper(type); id=Bin2Int(sid,len); keynr=skeynr;
    SetBinKey(skey,PLAINLEN_STD);
    return true;
    }
  return false;
}

// -- cSystemScCore ------------------------------------------------------------
/*
cSystemScCore::cSystemScCore(const char *Name, int Pri, int ScId, const char *ScName)
:cSystem(Name,Pri)
{
  scId=ScId; scName=ScName;
}

bool cSystemScCore::ProcessECM(const cEcmInfo *ecm, unsigned char *source)
{
  bool res=false;
  cSmartCard *card=smartcards.LockCard(scId);
  if(card) {
    res=card->Decode(ecm,source,cw);
    smartcards.ReleaseCard(card);
    if(res) KeyOK(scName);
    }
  return res;
}

void cSystemScCore::ProcessEMM(int pid, int caid, unsigned char *buffer)
{
  cSmartCard *card=smartcards.LockCard(scId);
  if(card) {
    card->Update(pid,caid,buffer);
    smartcards.ReleaseCard(card);
    }
}
*/

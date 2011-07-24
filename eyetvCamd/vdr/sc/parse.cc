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
#include <stdio.h>
#include <string.h>

#include "parse.h"
#include "misc.h"
#include "log.h"

// -----------------------------------------------------------------------------

void SetSctLen(unsigned char *data, int len)
{
  data[1]=(len>>8) | 0x70;
  data[2]=len & 0xFF;
}

static void SortNanos(unsigned char *dest, const unsigned char *src, int len)
{
  int w=0, c=-1;
  while(1) {
    int n=0x100;
    for(int j=0; j<len;) {
      int l=src[j+1]+2;
      if(src[j]==c) {
        if(w+l>len) {
          PRINTF(L_GEN_ERROR,"sortnanos: sanity check failed. Exceeding memory area. Probably corrupted nanos!");
          memset(dest,0,len); // zero out everything
          return;
          }
        memcpy(&dest[w],&src[j],l);
        w+=l;
        }
      else if(src[j]>c && src[j]<n)
        n=src[j];
      j+=l;
      }
    if(n==0x100) break;
    c=n;
    }
}

// -- cProviders ---------------------------------------------------------------

void cProviders::AddProv(cProvider *p)
{
  if(p) Add(p);
}

cProvider *cProviders::FindProv(const unsigned char *data)
{
  cProvider *p=First();
  while(p) {
    if(p->MatchID(data)) break;
    p=Next(p);
    }
  return p;
}

cProvider *cProviders::MatchProv(const unsigned char *data)
{
  cProvider *p=First();
  while(p) {
    if(p->MatchEMM(data)) break;
    p=Next(p);
    }
  return p;
}

// -- cAssSct ------------------------------------------------------------------

cAssSct::cAssSct(const unsigned char *Data)
{
  data=Data;
}

// -- cAssembleData ------------------------------------------------------------

cAssembleData::cAssembleData(const unsigned char *Data)
{
  data=Data;
  curr=0;
}

void cAssembleData::SetAssembled(const unsigned char *Data)
{
  Add(new cAssSct(Data));
  curr=First();
}

const unsigned char *cAssembleData::Assembled(void)
{
  const unsigned char *ret=0;
  if(First()) {
    if(curr) {
      ret=curr->Data();
      curr=Next(curr);
      }
    }
  else {
    ret=data;
    data=0;
    }
  return ret;
}

// -- cIdSet -------------------------------------------------------------------

cIdSet::cIdSet(void)
{
  card=0;
}

cIdSet::~cIdSet()
{
  delete card;
}

void cIdSet::ResetIdSet(void)
{
  delete card; card=0;
  Clear();
}

void cIdSet::SetCard(cCard *c)
{
  delete card;
  card=c;
}

bool cIdSet::MatchEMM(const unsigned char *data)
{
  return (card && card->MatchEMM(data)) || MatchProv(data);
}

bool cIdSet::MatchAndAssemble(cAssembleData *ad, int *updtype, cProvider **prov)
{
  const unsigned char *data=ad->Data();
  cProvider *p;
  if(card && card->MatchEMM(data)) {
    if(updtype) *updtype=card->UpdateType(data);
    if(prov) *prov=0;
    if(card->Assemble(ad)>=0) return true;
    }
  else if((p=MatchProv(data))) {
    if(updtype) *updtype=p->UpdateType(data);
    if(prov) *prov=p;
    if(p->Assemble(ad)>=0) return true;
    }
  return false;
}

// -- cProviderIrdeto ----------------------------------------------------------

cProviderIrdeto::cProviderIrdeto(unsigned char pb, const unsigned char *pi)
{
  provBase=pb;
  memcpy(provId,pi,sizeof(provId));
}

bool cProviderIrdeto::MatchID(const unsigned char *data)
{
  const unsigned int mode=cParseIrdeto::AddrBase(data);
  return (mode<0x10 && mode==provBase);
}

bool cProviderIrdeto::MatchEMM(const unsigned char *data)
{
  const unsigned int len=cParseIrdeto::AddrLen(data);
  const unsigned int mode=cParseIrdeto::AddrBase(data);
  return (   mode<0x10 && len<=sizeof(provId)
          && mode==provBase && (!len || !memcmp(&data[4],provId,len)));
}

unsigned long long cProviderIrdeto::ProvId(void)
{
  return (provId[0]<<24)+(provId[1]<<16)+(provId[2]<<8)+provBase;
}

// -- cCardIrdeto --------------------------------------------------------------

cCardIrdeto::cCardIrdeto(unsigned char hb, const unsigned char *hs)
{
  hexBase=hb;
  memcpy(hexSer,hs,sizeof(hexSer));
}

bool cCardIrdeto::MatchEMM(const unsigned char *data)
{
  const unsigned int len=cParseIrdeto::AddrLen(data);
  const unsigned int mode=cParseIrdeto::AddrBase(data);
  return (   mode>=0x10 && len<=sizeof(hexSer)
          && mode==hexBase && (!len || !memcmp(&data[4],hexSer,len)));
}

int cCardIrdeto::UpdateType(const unsigned char *data)
{
  switch(cParseIrdeto::AddrLen(data)) {
    case 0:  return 0; // global
    case 1:
    case 2:  return 2; // shared
    default: return 3; // unique
    }
}

// -- cParseIrdeto -------------------------------------------------------------

unsigned int cParseIrdeto::AddrLen(const unsigned char *data)
{
  return data[3] & 0x07;
}

unsigned int cParseIrdeto::AddrBase(const unsigned char *data)
{
  return data[3] >> 3;
}

// -- cProviderSeca ------------------------------------------------------------

cProviderSeca::cProviderSeca(const unsigned char *pi, const unsigned char *s)
{
  memcpy(provId,pi,sizeof(provId));
  memcpy(sa,s,sizeof(sa));
}

bool cProviderSeca::MatchID(const unsigned char *data)
{
  const unsigned char *id=cParseSeca::ProvIdPtr(data);
  return id && !memcmp(id,provId,sizeof(provId));
}

bool cProviderSeca::MatchEMM(const unsigned char *data)
{
  return TID(data)==0x84 &&
         !memcmp(SID(data),provId,sizeof(provId)) && !memcmp(SA(data),sa,sizeof(sa));
}

unsigned long long cProviderSeca::ProvId(void)
{
  return (provId[0]<<8)+provId[1];
}

// -- cCardSeca ----------------------------------------------------------------

cCardSeca::cCardSeca(const unsigned char *u)
{
  memcpy(ua,u,sizeof(ua));
}

bool cCardSeca::MatchEMM(const unsigned char *data)
{
  return TID(data)==0x82 &&
         !memcmp(UA(data),ua,sizeof(ua));
}

// -- cParseSeca ---------------------------------------------------------------

int cParseSeca::CmdLen(const unsigned char *data)
{
  struct SecaCmd *s=(struct SecaCmd *)data;
  return ((s->cmdLen1&0x0f)<<8) | s->cmdLen2;
}

int cParseSeca::Payload(const unsigned char *data, const unsigned char **payload)
{
  int l;
  switch(TID(data)) {
    case 0x80:
    case 0x81: l=sizeof(struct SecaEcm); break;
    case 0x82: l=sizeof(struct SecaEmmUnique); break;
    case 0x84: l=sizeof(struct SecaEmmShared); break;
    default: return -1;
    }
  if(payload) *payload=&data[l];
  return CmdLen(data)-l+sizeof(struct SecaCmd);
}

int cParseSeca::SysMode(const unsigned char *data)
{
  switch(TID(data)) {
    case 0x80:
    case 0x81: return ((struct SecaEcm *)data)->sm; break;
    case 0x82: return ((struct SecaEmmUnique *)data)->sm; break;
    case 0x84: return ((struct SecaEmmShared *)data)->sm; break;
    default: return -1;
    }
}

int cParseSeca::KeyNr(const unsigned char *data)
{
  switch(TID(data)) {
    case 0x80:
    case 0x81: return ((struct SecaEcm *)data)->keyNr; break;
    case 0x82: return ((struct SecaEmmUnique *)data)->keyNr; break;
    case 0x84: return ((struct SecaEmmShared *)data)->keyNr; break;
    default: return -1;
    }
}

const unsigned char *cParseSeca::ProvIdPtr(const unsigned char *data)
{
  switch(TID(data)) {
    case 0x80:
    case 0x81: return ((struct SecaEcm *)data)->id;
    case 0x82: return ((struct SecaEmmUnique *)data)->id;
    case 0x84: return ((struct SecaEmmShared *)data)->id;
    }
  return 0;
}

int cParseSeca::ProvId(const unsigned char *data)
{
  const unsigned char *id=cParseSeca::ProvIdPtr(data);
  return id ? (id[0]<<8)+id[1] : -1;
}

// -- cProviderViaccess --------------------------------------------------------

cProviderViaccess::cProviderViaccess(void)
{
  sharedEmm=0; sharedLen=sharedToggle=0;
}

cProviderViaccess::cProviderViaccess(const unsigned char *id, const unsigned char *s)
{
  sharedEmm=0; sharedLen=sharedToggle=0;
  memcpy(ident,id,sizeof(ident));
  ident[2]&=0xf0;
  memcpy(sa,s,sizeof(sa));
}

cProviderViaccess::~cProviderViaccess()
{
  free(sharedEmm);
}

bool cProviderViaccess::MatchID(const unsigned char *data)
{
  const unsigned char *id=cParseViaccess::ProvIdPtr(data);
  return id && id[0]==ident[0] && id[1]==ident[1] && (id[2]&0xf0)==ident[2];
}

bool cProviderViaccess::MatchEMM(const unsigned char *data)
{
  switch(data[0]) {
    case 0x8e: 
      if(memcmp(&data[3],sa,sizeof(sa)-1)) break;
      if((data[6]&2)==0)
        return sharedEmm && MatchID(sharedEmm);
      // fall through
    case 0x8c:
    case 0x8d:
      return MatchID(data);
    }
  return false;
}

unsigned long long cProviderViaccess::ProvId(void)
{
  return (ident[0]<<16)+(ident[1]<<8)+ident[2];
}

int cProviderViaccess::UpdateType(const unsigned char *data)
{
  switch(data[0]) {
    case 0x8e: return 2; // shared
    case 0x8c:
    case 0x8d:
    default:   return 0; // global
    }
}

int cProviderViaccess::Assemble(cAssembleData *ad)
{
  const unsigned char *data=ad->Data();
  int len=SCT_LEN(data);
  switch(data[0]) {
    case 0x8C:
    case 0x8D:
      if(data[0]!=sharedToggle) {
        free(sharedEmm);
        sharedLen=len;
        sharedEmm=(unsigned char *)malloc(len);
        if(sharedEmm) memcpy(sharedEmm,data,sharedLen);
        sharedToggle=data[0];
        }
      break;

    case 0x8E:
      if(sharedEmm) {
        unsigned char *tmp=AUTOMEM(len+sharedLen);
        unsigned char *ass=(unsigned char *)cParseViaccess::NanoStart(data);
        len-=(ass-data);
        if((data[6]&2)==0) {
          const int addrlen=len-8;
          len=0;
          tmp[len++]=0x9e;
          tmp[len++]=addrlen;
          memcpy(&tmp[len],&ass[0],addrlen); len+=addrlen;
          tmp[len++]=0xf0;
          tmp[len++]=0x08;
          memcpy(&tmp[len],&ass[addrlen],8); len+=8;
          }
        else {
          memcpy(tmp,ass,len);
          }
        ass=(unsigned char *)cParseViaccess::NanoStart(sharedEmm);
        int l=sharedLen-(ass-sharedEmm);
        memcpy(&tmp[len],ass,l); len+=l;

        ass=(unsigned char *)malloc(len+7);
        if(ass) {
          memcpy(ass,data,7);
          SortNanos(ass+7,tmp,len);
          SetSctLen(ass,len+4);
          ad->SetAssembled(ass);
          return 1; // assembled
          }
        }
      break;
    }
  return -1; // ignore
}

// -- cCardViaccess ------------------------------------------------------------

cCardViaccess::cCardViaccess(const unsigned char *u)
{
  memcpy(ua,u,sizeof(ua));
}

bool cCardViaccess::MatchEMM(const unsigned char *data)
{
  return data[0]==0x88 && !memcmp(&data[3],ua,sizeof(ua));
}

// -- cParseViaccess -----------------------------------------------------------

const unsigned char *cParseViaccess::NanoStart(const unsigned char *data)
{
  switch(data[0]) {
    case 0x88: return &data[8];
    case 0x8e: return &data[7];
    case 0x8c:
    case 0x8d: return &data[3];
    case 0x80:
    case 0x81: return &data[4];
    }
  return 0;
}

const unsigned char *cParseViaccess::CheckNano90(const unsigned char *data)
{
  return CheckNano90FromNano(NanoStart(data));
}

const unsigned char *cParseViaccess::CheckNano90FromNano(const unsigned char *data)
{
  if(data && data[0]==0x90 && data[1]==0x03) return data;
  return 0;
}

int cParseViaccess::KeyNr(const unsigned char *data)
{
  return KeyNrFromNano(CheckNano90(data));
}

int cParseViaccess::KeyNrFromNano(const unsigned char *data)
{
  return data ? data[4]&0x0f : -1;
}

const unsigned char *cParseViaccess::ProvIdPtr(const unsigned char *data)
{
  data=CheckNano90(data);
  return data ? &data[2] : 0;
}

int cParseViaccess::ProvId(const unsigned char *data)
{
  const unsigned char *id=cParseViaccess::ProvIdPtr(data);
  return id ? (id[0]<<16)+(id[1]<<8)+(id[2]&0xf0) : -1;
}

// -- cCardNagra2 --------------------------------------------------------------

cCardNagra2::cCardNagra2(const unsigned char *a)
{
  addr[0]=a[2];
  addr[1]=a[1];
  addr[2]=a[0];
  addr[3]=a[3];
}

bool cCardNagra2::MatchEMM(const unsigned char *data)
{
  return data[0]==0x82 ||
        (data[0]==0x83 && !memcmp(&data[3],addr,(data[7]==0x10)?3:4));
}

int cCardNagra2::UpdateType(const unsigned char *data)
{
  if(data[0]==0x83) return (data[7]==0x10) ? 2:3;
  return 0;
}

// -- cProviderConax -----------------------------------------------------------

cProviderConax::cProviderConax(const unsigned char *a)
{
  memcpy(addr,a,sizeof(addr));
}

bool cProviderConax::MatchID(const unsigned char *data)
{
  return MatchEMM(data);
}

bool cProviderConax::MatchEMM(const unsigned char *data)
{
  return !memcmp(&data[3],addr,sizeof(addr));
}

unsigned long long cProviderConax::ProvId(void)
{
  return Bin2LongLong(addr,sizeof(addr));
}

// -- cCardConax ---------------------------------------------------------------

cCardConax::cCardConax(const unsigned char *a)
{
  memcpy(addr,a,sizeof(addr));
}

bool cCardConax::MatchEMM(const unsigned char *data)
{
  return !memcmp(&data[3],addr,sizeof(addr));
}

// -- cProviderCryptoworks -----------------------------------------------------

cProviderCryptoworks::cProviderCryptoworks(const unsigned char *a)
{
  memcpy(addr,a,sizeof(addr));
}

bool cProviderCryptoworks::MatchID(const unsigned char *data)
{
  return MatchEMM(data);
}

bool cProviderCryptoworks::MatchEMM(const unsigned char *data)
{
  return false;
}

unsigned long long cProviderCryptoworks::ProvId(void)
{
  return Bin2LongLong(addr,sizeof(addr));
}

// -- cCardCryptoworks----------------------------------------------------------

cCardCryptoworks::cCardCryptoworks(const unsigned char *a)
{
  memcpy(addr,a,sizeof(addr));
  sharedEmm=0; sharedLen=globalToggle=0; globalCrc=0;
}

cCardCryptoworks::~cCardCryptoworks()
{
  free(sharedEmm);
}

bool cCardCryptoworks::MatchEMM(const unsigned char *data)
{
  if((data[1]&0xF0)==0x70 && data[3]==0xA9 && data[4]==0xff) {
    switch(data[0]) {
      case 0x82: return !memcmp(&data[5],addr,sizeof(addr));
      case 0x84: return !memcmp(&data[5],addr,sizeof(addr)-1);
      case 0x86:
      case 0x88:
      case 0x89: return true;
      }
    }
  return false;
}

int cCardCryptoworks::UpdateType(const unsigned char *data)
{
  switch(data[0]) {
    case 0x82: return 3; // unique
    case 0x84:
    case 0x86: return 2; // shared
    default:   return 0; // global
    }
}

int cCardCryptoworks::Assemble(cAssembleData *ad)
{
  const unsigned char *data=ad->Data();
  int len=SCT_LEN(data);
  switch(data[0]) {
    case 0x82:
      return 0; // no assemble needed

    case 0x84:
      free(sharedEmm);
      sharedEmm=(unsigned char *)malloc(len);
      if(sharedEmm) {
        memcpy(sharedEmm,data,len);
        sharedLen=len;
        }
      break;

    case 0x86:
      if(sharedEmm) {
        int alen=len-5 + sharedLen-12;
        unsigned char *tmp=AUTOMEM(alen);
        memcpy(tmp,&data[5],len-5);
        memcpy(tmp+len-5,&sharedEmm[12],sharedLen-12);

        unsigned char *ass=(unsigned char *)malloc(alen+12);
        if(!ass) return -1; // ignore
        memcpy(ass,sharedEmm,12);
        SortNanos(ass+12,tmp,alen);
        SetSctLen(ass,alen+9);
        free(sharedEmm); sharedEmm=0;
        if(ass[11]==alen) { // sanity check
          ad->SetAssembled(ass);
          return 1; // assembled
          }
        }
      break;

    case 0x88:
    case 0x89:
      if(data[0]!=globalToggle) {
        globalToggle=data[0];
        unsigned int crc=crc32_le(0,data+1,len-1);
        if(crc!=globalCrc) {
          globalCrc=crc;
          return 0; // no assemble needed
          }
        }
      break;
    }
  return -1; // ignore
}

// -- cProviderNDS -------------------------------------------------------------

cProviderNDS::cProviderNDS(const unsigned char *s)
{
  memcpy(sa,s,sizeof(sa));
}

bool cProviderNDS::MatchID(const unsigned char *data)
{
  return MatchEMM(data);
}

bool cProviderNDS::MatchEMM(const unsigned char *data)
{
  return cParseNDS::HasAddr(data,sa);
}

unsigned long long cProviderNDS::ProvId(void)
{
  return Bin2LongLong(sa,sizeof(sa));
}

int cProviderNDS::Assemble(cAssembleData *ad)
{
  return cParseNDS::Assemble(ad,sa);
}

// -- cCardNDS -----------------------------------------------------------------

cCardNDS::cCardNDS(const unsigned char *u)
{
  memcpy(ua,u,sizeof(ua));
}

bool cCardNDS::MatchEMM(const unsigned char *data)
{
  return cParseNDS::HasAddr(data,ua);
}

int cCardNDS::Assemble(cAssembleData *ad)
{
  return cParseNDS::Assemble(ad,ua);
}

// -- cParseNDS ----------------------------------------------------------------

unsigned int cParseNDS::NumAddr(const unsigned char *data)
{
  return ((data[3]&0x30)>>4)+1;
}

int cParseNDS::AddrMode(const unsigned char *data)
{
  switch(data[3]&0xC0) {
    case 0x40: return 3;
    case 0x80: return 2;
    default:   return 0;
    }
}

bool cParseNDS::HasAddr(const unsigned char *data, const unsigned char *a)
{
  int s;
  switch(AddrMode(data)) {
    case 2: s=3; break;
    case 3: s=4; break;
    default: return true;
    }
  for(int l=NumAddr(data)-1; l>=0; l--) {
    if(!memcmp(&data[l*4+4],a,s)) return true;
    }
  return false;
}

const unsigned char *cParseNDS::PayloadStart(const unsigned char *data)
{
  //return &data[4 + NumAddr(data)*4 + 2];
  if(AddrMode(data)==0) return &data[4];
  else                  return &data[4+NumAddr(data)*4];
}

int cParseNDS::PayloadSize(const unsigned char *data)
{
  //return emm[2]+emm[3]+4-1+5;
  int l=SCT_LEN(data);
  if(AddrMode(data)==0) return l-(4);
  else                  return l-(4+NumAddr(data)*4);
}

int cParseNDS::Assemble(cAssembleData *ad, const unsigned char *a)
{
  const unsigned char *data=ad->Data();
  int len=cParseNDS::PayloadSize(data);
  const unsigned char *pl=cParseNDS::PayloadStart(data);
  switch(cParseNDS::AddrMode(data)) {
    case 0:
     {
     int n=*(pl-1) & 0x01;
     for(int i=cParseNDS::NumAddr(data); i>0 && len>0; i--) {
       int l;
       if(n) { 
         l=(((*pl & 0x3F)<<8) + *(pl+1)) << 1;
         pl+=2; len-=2;
         }
       else {
         l=(*pl & 0x3F) << 1;
         pl++; len--;
         }
       if(l>0 && l<=len) {
         unsigned char *ass=(unsigned char *)malloc(len+4); if(!ass) return -1; // ignore
         ass[0]=data[0];
         ass[3]=data[3]&0x0F;
         memcpy(&ass[4],pl,l);
         SetSctLen(ass,l+1);
         ad->SetAssembled(ass);
         pl+=l; len-=l;
         }
       }
     return 1; // assembled
     }

    case 2:
    case 3:
      {
      unsigned char *ass=(unsigned char *)malloc(len+8); if(!ass) return -1; // ignore
      ass[0]=data[0];
      ass[3]=data[3]&0x0F;
      memcpy(&ass[4],a,4);
      memcpy(&ass[8],pl,len);
      SetSctLen(ass,len+5);
      ad->SetAssembled(ass);
      return 1; // assembled
      }
    }
  return -1; // ignore
}

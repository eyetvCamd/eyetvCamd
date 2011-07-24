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
#include <stdio.h>

#include "crypto.h"
#include "helper.h"
#include "log.h"

// -----------------------------------------------------------------------------

void RotateBytes(unsigned char *out, const unsigned char *in, int n)
{
  if(n>0) {
    out+=n;
    do { *(--out)=*(in++); } while(--n);
    }
}

void RotateBytes(unsigned char *in, int n)
{
  if(n>1) {
    unsigned char *e=in+n-1;
    do {
      unsigned char temp=*in;
      *in++=*e;
      *e-- =temp;
      } while(in<e);
    }
}

// -- cBN ----------------------------------------------------------------------

bool cBN::Get(const unsigned char *in, int n)
{
  return BN_bin2bn(in,n,&big)!=0;
}

int cBN::Put(unsigned char *out, int n) const
{
  int s=BN_num_bytes(&big);
  if(s>n) {
    unsigned char *buff=AUTOMEM(s);
    BN_bn2bin(&big,buff);
    memcpy(out,buff+s-n,n);
    }
  else if(s<n) {
    int l=n-s;
    memset(out,0,l);
    BN_bn2bin(&big,out+l);
    }
  else BN_bn2bin(&big,out);
  return s;
}

bool cBN::GetLE(const unsigned char *in, int n)
{
  unsigned char *tmp=AUTOMEM(n);
  RotateBytes(tmp,in,n);
  return BN_bin2bn(tmp,n,&big)!=0;
}

int cBN::PutLE(unsigned char *out, int n) const
{
  int s=Put(out,n);
  RotateBytes(out,n);
  return s;
}

// -- cIDEA --------------------------------------------------------------------

#ifdef OPENSSL_NO_IDEA
#warning ** openssl lacks IDEA support. Using deprecated static support code. Update your openssl package.
#include "support/i_cbc.c"
#include "support/i_skey.c"
#endif

void cIDEA::Decrypt(unsigned char *data, int len, const unsigned char *key, unsigned char *iv) const
{
  unsigned char v[8];
  if(!iv) { memset(v,0,sizeof(v)); iv=v; }
  IDEA_KEY_SCHEDULE ks;
  idea_set_encrypt_key(key,&ks);
  idea_cbc_encrypt(data,data,len&~7,&ks,iv,IDEA_DECRYPT);
}

int cIDEA::Encrypt(const unsigned char *data, int len, unsigned char *crypt, const unsigned char *key, unsigned char *iv) const
{
  unsigned char v[8];
  if(!iv) { memset(v,0,sizeof(v)); iv=v; }
  len&=~7;
  IDEA_KEY_SCHEDULE ks;
  idea_set_encrypt_key(key,&ks);
  idea_cbc_encrypt(data,crypt,len,&ks,iv,IDEA_ENCRYPT);
  return len;
}

void cIDEA::SetEncKey(const unsigned char *key, IdeaKS *ks) const
{
  idea_set_encrypt_key(key,ks);
}

void cIDEA::SetDecKey(const unsigned char *key, IdeaKS *ks) const
{
  IDEA_KEY_SCHEDULE tmp;
  idea_set_encrypt_key(key,&tmp);
  idea_set_decrypt_key(&tmp,ks);
}

void cIDEA::Decrypt(unsigned char *data, int len, IdeaKS *ks, unsigned char *iv) const
{
  unsigned char v[8];
  if(!iv) { memset(v,0,sizeof(v)); iv=v; }
  idea_cbc_encrypt(data,data,len&~7,ks,iv,IDEA_DECRYPT);
}

int cIDEA::Encrypt(const unsigned char *data, int len, unsigned char *crypt, IdeaKS *ks, unsigned char *iv) const
{
  unsigned char v[8];
  if(!iv) { memset(v,0,sizeof(v)); iv=v; }
  idea_cbc_encrypt(data,crypt,len&~7,ks,iv,IDEA_ENCRYPT);
  return len;
}

// -- cRSA ---------------------------------------------------------------------

bool cRSA::Input(cBN *d, const unsigned char *in, int n, bool LE) const
{
  if(LE) return d->GetLE(in,n); 
  else   return d->Get(in,n);
}

int cRSA::Output(unsigned char *out, int n, cBN *r, bool LE) const
{
  if(LE) return r->PutLE(out,n);
  else   return r->Put(out,n);
}

int cRSA::RSA(unsigned char *out, const unsigned char *in, int n, const BIGNUM *exp, const BIGNUM *mod, bool LE) const
{
  cBNctx ctx;
  cBN r, d;
  if(Input(&d,in,n,LE)) {
    if(BN_mod_exp(r,d,exp,mod,ctx)) return Output(out,n,&r,LE);
    PRINTF(L_GEN_ERROR,"rsa: mod-exp failed");
    }
  return 0;
}

int cRSA::RSA(BIGNUM *out, const unsigned char *in, int n, const BIGNUM *exp, const BIGNUM *mod, bool LE) const
{
  cBNctx ctx;
  cBN d;
  if(Input(&d,in,n,LE)) {
    if(BN_mod_exp(out,d,exp,mod,ctx)) return BN_num_bytes(out);
    PRINTF(L_GEN_ERROR,"rsa: mod-exp failed");
    }
  return 0;
}

int cRSA::RSA(unsigned char *out, int n, BIGNUM *in, const BIGNUM *exp, const BIGNUM *mod, bool LE) const
{
  cBNctx ctx;
  cBN r;
  if(BN_mod_exp(r,in,exp,mod,ctx)) return Output(out,n,&r,LE);
  PRINTF(L_GEN_ERROR,"rsa: mod-exp failed");
  return 0;
}

// -- cDes ---------------------------------------------------------------------

const unsigned char cDes::_PC1[] = {
   57,49,41,33,25,17, 9, 1, 
   58,50,42,34,26,18,10, 2,
   59,51,43,35,27,19,11, 3,
   60,52,44,36,63,55,47,39,
   31,23,15, 7,62,54,46,38,
   30,22,14, 6,61,53,45,37,
   29,21,13, 5,28,20,12, 4
  };

const unsigned char cDes::_PC2[] = {
  14,17,11,24, 1, 5,   3,28,15, 6,21,10,
  23,19,12, 4,26, 8,  16, 7,27,20,13, 2,
  41,52,31,37,47,55,  30,40,51,45,33,48,
  44,49,39,56,34,53,  46,42,50,36,29,32
  };

const unsigned char cDes::_E[] = {
  32, 1, 2, 3, 4, 5,   4, 5, 6, 7, 8, 9,
   8, 9,10,11,12,13,  12,13,14,15,16,17,
  16,17,18,19,20,21,  20,21,22,23,24,25,
  24,25,26,27,28,29,  28,29,30,31,32, 1
  };

const unsigned char cDes::_P[] = {
  16, 7,20,21,  29,12,28,17,
   1,15,23,26,   5,18,31,10,
   2, 8,24,14,  32,27, 3, 9,
  19,13,30, 6,  22,11, 4,25
  };

const unsigned char cDes::IP[] = {
  58,50,42,34,26,18,10,2,  60,52,44,36,28,20,12,4,
  62,54,46,38,30,22,14,6,  64,56,48,40,32,24,16,8,
  57,49,41,33,25,17, 9,1,  59,51,43,35,27,19,11,3,
  61,53,45,37,29,21,13,5,  63,55,47,39,31,23,15,7
  };

const unsigned char cDes::FP[] = {
  40,8,48,16,56,24,64,32,  39,7,47,15,55,23,63,31,
  38,6,46,14,54,22,62,30,  37,5,45,13,53,21,61,29,
  36,4,44,12,52,20,60,28,  35,3,43,11,51,19,59,27,
  34,2,42,10,50,18,58,26,  33,1,41, 9,49,17,57,25
  };

const unsigned char cDes::S[][64] = {
  { 0xe,0x0,0x4,0xf,0xd,0x7,0x1,0x4,  0x2,0xe,0xf,0x2,0xb,0xd,0x8,0x1,
    0x3,0xa,0xa,0x6,0x6,0xc,0xc,0xb,  0x5,0x9,0x9,0x5,0x0,0x3,0x7,0x8,
    0x4,0xf,0x1,0xc,0xe,0x8,0x8,0x2,  0xd,0x4,0x6,0x9,0x2,0x1,0xb,0x7,
    0xf,0x5,0xc,0xb,0x9,0x3,0x7,0xe,  0x3,0xa,0xa,0x0,0x5,0x6,0x0,0xd
  },
  { 0xf,0x3,0x1,0xd,0x8,0x4,0xe,0x7,  0x6,0xf,0xb,0x2,0x3,0x8,0x4,0xe,
    0x9,0xc,0x7,0x0,0x2,0x1,0xd,0xa,  0xc,0x6,0x0,0x9,0x5,0xb,0xa,0x5,
    0x0,0xd,0xe,0x8,0x7,0xa,0xb,0x1,  0xa,0x3,0x4,0xf,0xd,0x4,0x1,0x2,
    0x5,0xb,0x8,0x6,0xc,0x7,0x6,0xc,  0x9,0x0,0x3,0x5,0x2,0xe,0xf,0x9
  },
  { 0xa,0xd,0x0,0x7,0x9,0x0,0xe,0x9,  0x6,0x3,0x3,0x4,0xf,0x6,0x5,0xa,
    0x1,0x2,0xd,0x8,0xc,0x5,0x7,0xe,  0xb,0xc,0x4,0xb,0x2,0xf,0x8,0x1,
    0xd,0x1,0x6,0xa,0x4,0xd,0x9,0x0,  0x8,0x6,0xf,0x9,0x3,0x8,0x0,0x7,
    0xb,0x4,0x1,0xf,0x2,0xe,0xc,0x3,  0x5,0xb,0xa,0x5,0xe,0x2,0x7,0xc
  },
  { 0x7,0xd,0xd,0x8,0xe,0xb,0x3,0x5,  0x0,0x6,0x6,0xf,0x9,0x0,0xa,0x3,
    0x1,0x4,0x2,0x7,0x8,0x2,0x5,0xc,  0xb,0x1,0xc,0xa,0x4,0xe,0xf,0x9,
    0xa,0x3,0x6,0xf,0x9,0x0,0x0,0x6,  0xc,0xa,0xb,0x1,0x7,0xd,0xd,0x8,
    0xf,0x9,0x1,0x4,0x3,0x5,0xe,0xb,  0x5,0xc,0x2,0x7,0x8,0x2,0x4,0xe
  },
  { 0x2,0xe,0xc,0xb,0x4,0x2,0x1,0xc,  0x7,0x4,0xa,0x7,0xb,0xd,0x6,0x1,
    0x8,0x5,0x5,0x0,0x3,0xf,0xf,0xa,  0xd,0x3,0x0,0x9,0xe,0x8,0x9,0x6,
    0x4,0xb,0x2,0x8,0x1,0xc,0xb,0x7,  0xa,0x1,0xd,0xe,0x7,0x2,0x8,0xd,
    0xf,0x6,0x9,0xf,0xc,0x0,0x5,0x9,  0x6,0xa,0x3,0x4,0x0,0x5,0xe,0x3
  },
  { 0xc,0xa,0x1,0xf,0xa,0x4,0xf,0x2,  0x9,0x7,0x2,0xc,0x6,0x9,0x8,0x5,
    0x0,0x6,0xd,0x1,0x3,0xd,0x4,0xe,  0xe,0x0,0x7,0xb,0x5,0x3,0xb,0x8,
    0x9,0x4,0xe,0x3,0xf,0x2,0x5,0xc,  0x2,0x9,0x8,0x5,0xc,0xf,0x3,0xa,
    0x7,0xb,0x0,0xe,0x4,0x1,0xa,0x7,  0x1,0x6,0xd,0x0,0xb,0x8,0x6,0xd
  },
  { 0x4,0xd,0xb,0x0,0x2,0xb,0xe,0x7,  0xf,0x4,0x0,0x9,0x8,0x1,0xd,0xa,
    0x3,0xe,0xc,0x3,0x9,0x5,0x7,0xc,  0x5,0x2,0xa,0xf,0x6,0x8,0x1,0x6,
    0x1,0x6,0x4,0xb,0xb,0xd,0xd,0x8,  0xc,0x1,0x3,0x4,0x7,0xa,0xe,0x7,
    0xa,0x9,0xf,0x5,0x6,0x0,0x8,0xf,  0x0,0xe,0x5,0x2,0x9,0x3,0x2,0xc
  },
  { 0xd,0x1,0x2,0xf,0x8,0xd,0x4,0x8,  0x6,0xa,0xf,0x3,0xb,0x7,0x1,0x4,
    0xa,0xc,0x9,0x5,0x3,0x6,0xe,0xb,  0x5,0x0,0x0,0xe,0xc,0x9,0x7,0x2,
    0x7,0x2,0xb,0x1,0x4,0xe,0x1,0x7,  0x9,0x4,0xc,0xa,0xe,0x8,0x2,0xd,
    0x0,0xf,0x6,0xc,0xa,0x9,0xd,0x0,  0xf,0x3,0x3,0x5,0x5,0x6,0x8,0xb
  } };

const unsigned char cDes::LS[] = { 1,1,2,2,2,2,2,2,1,2,2,2,2,2,2,1 };

/* actualy slower ... :-(
#if defined __GNUC__ && __GNUC__ >= 2
#define shiftin(V,R,n) ({unsigned int VV; \
                         __asm__ ("btl %3,%2\n\tadcl %0,%0" : "=g" (VV) : "0" (V), "g" (R), "r" (n) : "cc"); \
                         VV; })
#else
*/
#define shiftin(V,R,n) ((V<<1)+(((R)>>(n))&1))
#define rol28(V,n) ((V<<(n) ^ V>>(28-(n)))&0xfffffffL)
#define ror28(V,n) ((V>>(n) ^ V<<(28-(n)))&0xfffffffL)

#if defined __GNUC__ && __GNUC__ >= 2
#if defined __i486__ || defined __pentium__ || defined __pentiumpro__ || defined __amd64__
#define hash(T) ({unsigned int TT; \
                 __asm__("bswapl %k0\n\trorw $8,%w0\n\tbswapl %k0" : "=g" (TT) : "0" (T) : "cc"); \
                 TT; })
#elif defined __i386__
#define hash(T) ({unsigned int TT; \
                 __asm__("rorl $16,%k0\n\trorw $8,%w0\n\troll $16,%k0" : "=g" (TT) : "0" (T) : "cc"); \
                 TT; })
#endif
#endif
#ifndef hash // fallback default
#define hash(T) ((T&0x0000ffffL) | ((T>>8)&0x00ff0000L) | ((T<<8)&0xff000000L))
#endif

#define DESROUND(C,D,T) { \
   unsigned int s=0; \
   for(int j=7, k=0; j>=0; j--) { \
     unsigned int v=0, K=0; \
     for(int t=5; t>=0; t--, k++) { \
       v=shiftin(v,T,E[k]); \
       if(PC2[k]<29) K=shiftin(K,C,28-PC2[k]); \
       else          K=shiftin(K,D,56-PC2[k]); \
       } \
     s=(s<<4) + S[7-j][v^K]; \
     } \
   T=0; \
   for(int j=31; j>=0; j--) T=shiftin(T,s,P[j]); \
   }

#define DESHASH(T) { if(mode&DES_HASH) T=hash(T); }

cDes::cDes(const unsigned char *pc1, const unsigned char *pc2)
{
  PC1=pc1 ? pc1 : _PC1;
  PC2=pc2 ? pc2 : _PC2;
  for(int i=0; i<48; i++) E[i]=32-_E[i];
  for(int i=0; i<32; i++) P[i]=32-_P[31-i];
}

void cDes::Permute(unsigned char *data, const unsigned char *P, int n) const
{
  unsigned char pin[8];
  for(int i=0, k=0; k<n; i++) {
    int p=0;
    for(int j=7; j>=0; j--,k++) {
      const int t=P[k]-1;
      p=shiftin(p,data[t>>3],7-(t&7));
      }
    pin[i]=p;
    }
  memcpy(data,pin,8);
}

void cDes::Des(unsigned char *data, const unsigned char *key, int mode) const
{
  unsigned char mkey[8];
  if(mode&DES_PC1) {
    memcpy(mkey,key,sizeof(mkey));
    Permute(mkey,PC1,56);
    key=mkey;
    }
  if(mode&DES_IP) Permute(data,IP,64);
  unsigned int C=UINT32_BE(key  ) >> 4;
  unsigned int D=UINT32_BE(key+3) & 0xfffffffL;
  unsigned int L=UINT32_BE(data  );
  unsigned int R=UINT32_BE(data+4);
  if(!(mode&DES_RIGHT)) {
    for(int i=15; i>=0; i--) {
      C=rol28(C,LS[15-i]); D=rol28(D,LS[15-i]);
      unsigned int T=R;
      if(mode&DES_MOD) T=Mod(T,key[7]); // apply costum mod e.g. Viaccess
      DESROUND(C,D,T);
      DESHASH(T);
      T^=L; L=R; R=T;
      }
    }
  else {
    for(int i=15; i>=0; i--) {
      unsigned int T=R;
      if(mode&DES_MOD) T=Mod(T,key[7]); // apply costum mod e.g. Viaccess
      DESROUND(C,D,T);
      DESHASH(T);
      T^=L; L=R; R=T;
      C=ror28(C,LS[i]); D=ror28(D,LS[i]);
      }
    }
  BYTE4_BE(data  ,R);
  BYTE4_BE(data+4,L);
  if(mode&DES_FP) Permute(data,FP,64);
}

// -- cAES ---------------------------------------------------------------------

#ifndef OPENSSL_HAS_AES
#warning ** openssl lacks AES support. Using deprecated static support code. Update your openssl package.
#include "support/aes_core.c"
#endif

cAES::cAES(void)
{
  active=false;
}

void cAES::SetKey(const unsigned char *key)
{
  AES_set_decrypt_key(key,128,&dkey);
  AES_set_encrypt_key(key,128,&ekey);
  active=true;
}

int cAES::Encrypt(const unsigned char *data, int len, unsigned char *crypt) const
{
  if(active) {
    len=(len+15)&(~15); // pad up to a multiple of 16
    for(int i=0; i<len; i+=16) AES_encrypt(data+i,crypt+i,(const AES_KEY *)&ekey);
    return len;
    }
  return -1;
}

void cAES::Decrypt(unsigned char *data, int len) const
{
  if(active)
    for(int i=0; i<len; i+=16) AES_decrypt(data+i,data+i,(const AES_KEY *)&dkey);
}

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

#include "system-common.h"
#include "data.h"
#include "helper.h"
#include "crypto.h"
#include "misc.h"
#include "log-sys.h"
#define SYSTEM_CRYPTOWORKS   0x0D00

#define SYSTEM_NAME          "Cryptoworks"
#define SYSTEM_PRI           -10
#define SYSTEM_CAN_HANDLE(x) ((x)==SYSTEM_CRYPTOWORKS)

#define L_SYS        4
#define L_SYS_ALL    LALL(L_SYS_LASTDEF)

static const struct LogModule lm_sys = {
  (LMOD_ENABLE|L_SYS_ALL)&LOPT_MASK,
  (LMOD_ENABLE|L_SYS_DEFDEF)&LOPT_MASK,
  "cryptoworks",
  { L_SYS_DEFNAMES }
  };
ADD_MODULE(L_SYS,lm_sys)

// -- cCryptoworks -------------------------------------------------------------
typedef unsigned char byte;

class cCryptoworks {
private:
protected:
  int curCaId;
  int curProv;
public:
  cCryptoworks(void) {};
  void CW_SWAP_KEY(byte *key);
  void CW_SWAP_DATA(byte *k);
  void CW_DES_ROUND(byte *d,byte *k);
  void CW_48_Key(byte *inkey, byte *outkey, byte algotype);
  void CW_LS_DES_KEY(byte *key, byte Rotate_Counter);
  void CW_RS_DES_KEY(byte *k, byte Rotate_Counter);
  void CW_RS_DES_SUBKEY(byte *k, byte Rotate_Counter);
  void CW_PREP_KEY(byte *key );
  void CW_L2DES(byte *data, byte *key, byte algo);
  void CW_R2DES(byte *data, byte *key, byte algo);
  void CW_DES(byte *data, byte *inkey, byte m);
  void *CW_SWAP(byte *dest, const byte *src, int len);
  void CW_XOR(byte *d, byte *k, byte l);
  void CW_DEC_ENC(byte *d, byte *k, byte a,byte m);
  void CW_DEC(byte *d, byte *key, byte algo);
  void CW_ENC(byte *d, byte *key, byte algo);
  void CW_PREP_RSA_KEY(byte *d, byte *k, byte algo);
  void CW_PREP_RSA_DAT(byte *data);
  void CW_SIGN(byte *d, byte l, byte *key, byte *h);
  byte CW_SIGCHK(byte *d, byte  l, byte *key );
  byte CW_PDUSDEC(byte* d, int l, byte* k );
  void CW_PDUDEC(byte* d, int l, byte* k );
  void CW_PDUENC(byte* d, int l, byte* k );
  void CW_DCW(byte *d ,byte l , byte *dcw);
  void CW_PREP_RSA(byte *d, byte *k, byte algo, byte len);
  void DecryptRSA(byte *msg, byte *mod, byte *exp, int lenMsg, int lenExp);
  
  static byte cw_sbox1[64];
  static byte cw_sbox2[64];
  static byte cw_sbox3[64];
  static byte cw_sbox4[64];
  static byte AND_bit1[8];
  static byte AND_bit2[8];
  static byte AND_bit3[8];
  static byte AND_bit4[8];
};

byte cCryptoworks::cw_sbox1[64] =
{	
  0xD8,0xD7,0x83,0x3D,0x1C,0x8A,0xF0,0xCF,0x72,0x4C,0x4D,0xF2,0xED,0x33,0x16,0xE0, 
  0x8F,0x28,0x7C,0x82,0x62,0x37,0xAF,0x59,0xB7,0xE0,0x00,0x3F,0x09,0x4D,0xF3,0x94,
  0x16,0xA5,0x58,0x83,0xF2,0x4F,0x67,0x30,0x49,0x72,0xBF,0xCD,0xBE,0x98,0x81,0x7F,
  0xA5,0xDA,0xA7,0x7F,0x89,0xC8,0x78,0xA7,0x8C,0x05,0x72,0x84,0x52,0x72,0x4D,0x38
};

byte cCryptoworks::cw_sbox2[64] =
{
  0xD8,0x35,0x06,0xAB,0xEC,0x40,0x79,0x34,0x17,0xFE,0xEA,0x47,0xA3,0x8F,0xD5,0x48,
  0x0A,0xBC,0xD5,0x40,0x23,0xD7,0x9F,0xBB,0x7C,0x81,0xA1,0x7A,0x14,0x69,0x6A,0x96,
  0x47,0xDA,0x7B,0xE8,0xA1,0xBF,0x98,0x46,0xB8,0x41,0x45,0x9E,0x5E,0x20,0xB2,0x35,
  0xE4,0x2F,0x9A,0xB5,0xDE,0x01,0x65,0xF8,0x0F,0xB2,0xD2,0x45,0x21,0x4E,0x2D,0xDB
};
	
byte cCryptoworks::cw_sbox3[64] =
{	
  0xDB,0x59,0xF4,0xEA,0x95,0x8E,0x25,0xD5,0x26,0xF2,0xDA,0x1A,0x4B,0xA8,0x08,0x25,
  0x46,0x16,0x6B,0xBF,0xAB,0xE0,0xD4,0x1B,0x89,0x05,0x34,0xE5,0x74,0x7B,0xBB,0x44,
  0xA9,0xC6,0x18,0xBD,0xE6,0x01,0x69,0x5A,0x99,0xE0,0x87,0x61,0x56,0x35,0x76,0x8E,
  0xF7,0xE8,0x84,0x13,0x04,0x7B,0x9B,0xA6,0x7A,0x1F,0x6B,0x5C,0xA9,0x86,0x54,0xF9
};

byte cCryptoworks::cw_sbox4[64] =
{	
  0xBC,0xC1,0x41,0xFE,0x42,0xFB,0x3F,0x10,0xB5,0x1C,0xA6,0xC9,0xCF,0x26,0xD1,0x3F,
  0x02,0x3D,0x19,0x20,0xC1,0xA8,0xBC,0xCF,0x7E,0x92,0x4B,0x67,0xBC,0x47,0x62,0xD0,
  0x60,0x9A,0x9E,0x45,0x79,0x21,0x89,0xA9,0xC3,0x64,0x74,0x9A,0xBC,0xDB,0x43,0x66,
  0xDF,0xE3,0x21,0xBE,0x1E,0x16,0x73,0x5D,0xA2,0xCD,0x8C,0x30,0x67,0x34,0x9C,0xCB
};

byte cCryptoworks::AND_bit1[8] = {0x00,0x40,0x04,0x80,0x21,0x10,0x02,0x08};
byte cCryptoworks::AND_bit2[8] = {0x80,0x08,0x01,0x40,0x04,0x20,0x10,0x02};
byte cCryptoworks::AND_bit3[8] = {0x82,0x40,0x01,0x10,0x00,0x20,0x04,0x08};
byte cCryptoworks::AND_bit4[8] = {0x02,0x10,0x04,0x40,0x80,0x08,0x01,0x20};

void cCryptoworks::DecryptRSA(byte *msg, byte *mod, byte *exp, int lenMsg, int lenExp)
{
  cBNctx ctx;
  cBN Mod, Exp, Msg, result;
  if( NXHostByteOrder() == NX_LittleEndian )
  {
    Mod.GetLE(mod, 64);
    Exp.GetLE(exp, lenExp);
    Msg.GetLE(msg, lenMsg);
  }
  else
  {
    Mod.Get(mod, 64);
    Exp.Get(exp, lenExp);
    Msg.Get(msg, lenMsg);
  }
  BN_mod_exp(result, Msg, Exp, Mod, ctx);
  if( NXHostByteOrder() == NX_LittleEndian )
  {
    result.PutLE(msg, lenMsg);
  }
  else
  {
    result.Put(msg, lenMsg);
  }
}

void cCryptoworks::CW_SWAP_KEY(byte *key)
{
  byte k[8];
  
  memcpy(k, key, 8);
  memcpy(key, key + 8, 8);
  memcpy(key + 8, k, 8);
}

void cCryptoworks::CW_SWAP_DATA(byte *k) 
{
  byte d[4];

  memcpy(d, k + 4, 4);
  memcpy(k + 4 ,k ,4);
  memcpy(k, d, 4);
}

void cCryptoworks::CW_DES_ROUND( byte *d,byte *k)
{
  byte	aa[44]   = {1,0,3,1,2,2,3,2,1,3,1,1,3,0,1,2,3,1,3,2,2,0,7,6,5,4,7,6,5,7,6,5,6,7,5,7,5,7,6,6,7,5,4,4},
	bb[44]   = {0x80,0x08,0x10,0x02,0x08,0x40,0x01,0x20,0x40,0x80,0x04,0x10,0x04,0x01,0x01,0x02,0x20,0x20,0x02,0x01,0x80,0x04,0x02,0x02,0x08,0x02,0x10,0x80,0x01,0x20,0x08,0x80,0x01,0x08,0x40,0x01,0x02,0x80,0x10,0x40,0x40,0x10,0x08,0x01},
	ff[4]    = {0x02,0x10,0x04,0x04},
	l[24]    = {0,2,4,6,7,5,3,1,4,5,6,7,7,6,5,4,7,4,5,6,4,7,6,5};
					  
  byte des_td[8], i, o, n, c = 1, m = 0, r = 0, *a = aa,*b = bb, *f = ff, *p1 = l,*p2 = l+8,*p3 = l+16;
	
  for (m = 0; m < 2; m++)
    for(i = 0; i < 4; i++)
      des_td[*p1++] = (m) ?  ((d[*p2++]*2) & 0x3F) | ((d[*p3++] & 0x80) ? 0x01 : 0x00): 
						      (d[*p2++]/2) | ((d[*p3++] & 0x01) ? 0x80 : 0x00); 
  for (i = 0; i < 8; i++)
  {
    c = (c) ? 0 : 1; r = (c) ? 6 : 7; n = (i) ? i-1 : 1;  
    o = (c) ? ((k[n] & *f++) ? 1 : 0) : des_td[n];
    for (m = 1; m < r; m++) 
      o = (c) ? (o*2) | ((k[*a++] & *b++) ? 0x01 : 0x00) : 
		  (o/2) | ((k[*a++] & *b++) ? 0x80 : 0x00);
    n = (i) ? n+1 : 0;   
    des_td[n] = (c) ?  des_td[n] ^ o : (o ^ des_td[n] )/4;
  }
  
  for( i = 0; i < 8; i++)
  {
    d[0] ^= (AND_bit1[i]  & cw_sbox1[des_td[i]]);
    d[1] ^= (AND_bit2[i]  & cw_sbox2[des_td[i]]);
    d[2] ^= (AND_bit3[i]  & cw_sbox3[des_td[i]]);
    d[3] ^= (AND_bit4[i]  & cw_sbox4[des_td[i]]);
  }
  CW_SWAP_DATA(d);
}

void cCryptoworks::CW_48_Key(byte *inkey, byte *outkey, byte algotype)
{
  byte Round_Counter ,i = 8,*key128 = inkey, *key48 = inkey + 0x10; 

  Round_Counter = 7 - (algotype & 7);
  memset (outkey, 0, 16);
  memcpy(outkey, key48, 6);
  for( ; i >  Round_Counter;i--) 
    if (i != 0)
      outkey[i-2] = key128[i];
}

void cCryptoworks::CW_LS_DES_KEY(byte *key,byte Rotate_Counter)
{
  byte round[] = {1,2,2,2,2,2,2,1,2,2,2,2,2,2,1,1}, i, n;
  unsigned short k[8];

  n = round[Rotate_Counter];

  for (i = 0; i < 8; i++) k[i] = key[i];
  for (i = 1; i < n + 1; i++)
  {
    k[7] =  (k[7]*2) |  ((k[4] & 0x008) ? 1 : 0); 
    k[6] =  (k[6]*2) |  ((k[7] & 0xF00) ? 1 : 0);	  k[7] &=0xff;
    k[5] =  (k[5]*2) |  ((k[6] & 0xF00) ? 1 : 0);	  k[6] &=0xff;
    k[4] = ((k[4]*2) |  ((k[5] & 0xF00) ? 1 : 0)) & 0xFF; k[5] &= 0xff; 
    k[3] =  (k[3]*2) |  ((k[0] & 0x008) ? 1 : 0); 
    k[2] =  (k[2]*2) |  ((k[3] & 0xF00) ? 1 : 0);	  k[3] &= 0xff;
    k[1] =  (k[1]*2) |  ((k[2] & 0xF00) ? 1 : 0);	  k[2] &= 0xff; 
    k[0] = ((k[0]*2) |  ((k[1] & 0xF00) ? 1 : 0)) & 0xFF; k[1] &= 0xff;
  }
  for (i = 0; i < 8; i++) key[i] = (byte) k[i];
}

void cCryptoworks::CW_RS_DES_KEY(byte *k, byte Rotate_Counter)
{
  byte i,c;

  for (i = 1; i < Rotate_Counter+1; i++)
  {
    c = (k[3] & 0x10) ? 0x80 : 0;
    //c = 0; if (k[3] & 0x10) c = 0x80;
    k[3] /= 2;	if (k[2] & 1) k[3] |= 0x80;
    k[2] /= 2;	if (k[1] & 1) k[2] |= 0x80;
    k[1] /= 2;	if (k[0] & 1) k[1] |= 0x80;
    k[0] /= 2;  k[0] |= c ; 
    c = (k[7] & 0x10) ? 0x80 : 0;
    //c = 0; if (k[7] & 0x10) c = 0x80;
    k[7] /= 2;	if (k[6] & 1) k[7] |= 0x80;
    k[6] /= 2;	if (k[5] & 1) k[6] |= 0x80;
    k[5] /= 2;	if (k[4] & 1) k[5] |= 0x80;
    k[4] /= 2; 	k[4] |= c; 
    c=0;
  }
}

void cCryptoworks::CW_RS_DES_SUBKEY(byte *k, byte Rotate_Counter)
{ 
  byte round[] = {1,1,2,2,2,2,2,2,1,2,2,2,2,2,2,1};

  CW_RS_DES_KEY(k, round[Rotate_Counter]);
}

void cCryptoworks::CW_PREP_KEY(byte *key )
{
  byte DES_key[8],j;
  int Round_Counter = 6,i,a;
  
  key[7] = 6;
  memset(DES_key, 0 , 8);
  do
  {
    a = 7; 
    i = key[7]; 
    j = key[Round_Counter];
    do
    {
      DES_key[i] = ( (DES_key[i] * 2) | ((j & 1) ? 1: 0) ) & 0xFF;
      j /=2;
      i--;
      if (i < 0) i = 6;
      a--;
    } while (a >= 0);
    key[7] = i;
    Round_Counter--;
  } while ( Round_Counter >= 0 );
  a = DES_key[4];
  DES_key[4] = DES_key[6];
  DES_key[6] = a;
  DES_key[7] = (DES_key[3] * 16) & 0xFF;
  memcpy(key,DES_key,8);
  CW_RS_DES_KEY(key,4);
}

void cCryptoworks::CW_L2DES(byte *data, byte *key, byte algo)
{
  byte i, k0[22], k1[22];
  
  memcpy(k0,key,22);
  memcpy(k1,key,22);
  
  CW_48_Key(k0, k1,algo);
  CW_PREP_KEY(k1);

  for (i = 0; i< 2; i++)
  {
    CW_LS_DES_KEY( k1,15);
    CW_DES_ROUND( data ,k1);
  }
}

void cCryptoworks::CW_R2DES(byte *data, byte *key, byte algo)
{
  byte i, k0[22],k1[22];
  
  memcpy(k0,key,22);
  memcpy(k1,key,22);
  
  CW_48_Key(k0, k1, algo);
  CW_PREP_KEY(k1);
  
  for (i = 0;i< 2; i++)
    CW_LS_DES_KEY(k1,15);
  
  for (i = 0;i< 2; i++)
  {
    CW_DES_ROUND( data ,k1);
    CW_RS_DES_SUBKEY(k1,1);
  }
  CW_SWAP_DATA(data);
}

void cCryptoworks::CW_DES(byte *data, byte *inkey, byte m)
{
  byte key[22],  i; 
  
  memcpy(key, inkey + 9, 8);
  CW_PREP_KEY( key );
  
  for (i = 16; i > 0; i--)
  {
    if (m == 1) CW_LS_DES_KEY(key, (byte) (i-1));
    CW_DES_ROUND( data ,key);
    if (m == 0) CW_RS_DES_SUBKEY(key, (byte) (i-1));
  } 
}

void *cCryptoworks::CW_SWAP(byte *dest, const byte *src, int len)
{
  byte tmp[128];
  byte *ret = dest;

  memcpy(tmp, src, len);

  while (len--)
    *dest++ = tmp[len];

  return ret;
}


void cCryptoworks::CW_XOR(byte *d, byte *k, byte l) 
{
  byte i;
	
  for (i = 0; i < l; i++) d[i] ^= k[i];	
}


void cCryptoworks::CW_DEC_ENC(byte *d, byte *k, byte a,byte m)
{
  byte n = m & 1;
	
  CW_L2DES(d , k, a);
  CW_DES  (d , k, n);
  CW_R2DES(d , k, a);
  if (m & 2) CW_SWAP_KEY(k);
}

void cCryptoworks::CW_DEC(byte *d, byte *key, byte algo)
{
  byte k0[22], algo_type = algo & 7,mode = 0, i;
	
  memcpy(k0,key,22);

  if (algo_type < 7) 
    CW_DEC_ENC(d , k0, algo_type, mode);
  else 
    for (i = 0; i < 3; i++)
    {
      mode = (mode == 1) ? 0 : 1;	
      CW_DEC_ENC(d , k0, algo_type ,(byte) (mode | 2));
    }
}

void cCryptoworks::CW_ENC(byte *d, byte *key, byte algo)
{
  byte k0[22], algo_type = algo & 7,mode = 1, i;
	
  memcpy(k0,key,22);

  if (algo_type < 7) 
    CW_DEC_ENC(d , k0, algo_type, mode);
  else 
    for (i = 0; i < 3; i++)
    {
      mode = (mode == 1) ? 0 : 1;	
      CW_DEC_ENC(d , k0, algo_type , (byte) (mode | 2));
    }
}

void cCryptoworks::CW_PREP_RSA_KEY(byte *d, byte *k, byte algo)
{
  CW_DEC_ENC(d, k, algo,1);
  d[0] |= 0x80;
  if ((algo & 0x18) < 0x18) d[0] = 0xFF;
  if (algo & 8) d[1] = 0xFF;
}

void cCryptoworks::CW_PREP_RSA_DAT(byte *data)
{
	
  byte d[0x20],d1[0x20],i,t1[] = { 0x0E,0x03,0x05,0x08,0x09,0x04,0x02,0x0F,0x00,0x0D,0x0B,0x06,0x07,0x0A,0x0C,0x01};

  memcpy(d+8,data,8);

#if 1

  for (i = 0; i < 8; i++ ) {
	  d[(i*2)+1] = d1[i*2+1] = d[i+8];
	  d[i*2]     = d1[i*2] = ((t1[d[i*2+1] /16] *16) & 0xFF) | (t1[d[i+8] & 0x0F]);
  }
#else
  for (i = 0; i < 8; i++ ) {
	  d[(i*2)+1] = d[i+8];
	  d[i*2]     = ((t1[d[i*2+1] /16] *16) & 0xFF) | (t1[d[i+8] & 0x0F]);
  }
#endif
  for (i = 0; i < 4; i++)
	  memcpy(&data[i*16],d,16);
  data[31] = ((data[15] * 16) & 0xFF) | 6;
  data[16] = data[0] ^ 1;

  data[32] &= 0x7F;
  data[32] |= 0x40;
  CW_SWAP(data,data,0x20);
  CW_SWAP(data+0x20,data+0x20,0x20);
}

#define KEYSET(prov,typ,id) ((((prov)&0xFF)<<16)|(((typ)&0xFF)<<8)|((id)&0xFF))
void cCryptoworks::CW_PREP_RSA(byte *d, byte *k, byte algo, byte len)
{
  byte c_mod[64];
  byte mask[128], key[22], exp[] ={0x02}, i = 0, l = len;
  cPlainKey *pk;

  if(!(pk=keys.FindKey('W', curCaId, KEYSET(curProv, 0x10, 0x00), 64))) {
    PRINTF(L_SYS_KEY,"missing %04X %02X 10 00 key", curCaId, curProv);
    return;
  }
  pk->Get(c_mod);
  
  memcpy(key,k,22);
  CW_PREP_RSA_KEY(d, key, algo);	
  while ( i < len )
  {
    CW_PREP_RSA_DAT(d);
    DecryptRSA(d, c_mod, exp, 0x40, 1);
    CW_SWAP(d,d,8);
    memcpy (mask + i,d + 8,(l > 0x20) ? 0x20 : l);
    CW_SWAP(mask + i,mask + i,(l > 0x20) ? 0x20 : l);
    i += 0x20;
    l -=0x20;
  }
  memcpy(d,mask,len);
}

void cCryptoworks::CW_SIGN(byte *d, byte l, byte *key, byte *h)
{
  byte n = 1, i, j, k[22], algo = d[0] & 7;
  
  memcpy(k,key,22);
  
  if (algo == 7) algo = 6;
  
  j = 0;
  for ( i = 0; i < l; i++ )
  {
    h[j] ^= d[i];
    j++;
    if ( j > 7 )
    {
      if (n) CW_L2DES(h, key, algo);
      CW_DES( h, k, 1 );
      j = n = 0;
    }
  }
  if (j > 0) CW_DES( h, k, 1 );
  CW_R2DES(h, k, algo);
}

byte cCryptoworks::CW_SIGCHK(byte *d, byte  l, byte *key )
{
  byte k[22], h[8];
  
  memcpy(k,key,22);	
  memset(h, 0, 8);
  CW_SIGN(d, (byte)(l - 10), k, h);
  return  (memcmp(d + l -8,h,8) == 0 );
}

byte cCryptoworks::CW_PDUSDEC(byte* d, int l, byte* k )
{
  byte i = 0, pi, li,/* algo = d[0] & 7,*/ buf[128];

  memset(buf,0,sizeof(buf));
	  
  while ( i < l )
  {
    pi = d[i];
    li = d[i + 1];
    //printf("pi %02X\n",pi);
    if (i == 0) li = 1;	
    switch (pi)
    {
      case 0x85 :
      {
	memcpy(buf,&d[i+2+li-8],8);
	CW_PREP_RSA(buf, k,d[0],li-8);
	CW_XOR(&d[i+2]    , buf    , (byte) (li - 8));
	memcpy(&d[i]      , &d[i+2], li - 8);
	memcpy(&d[i+li-8] , &d[i+li + 2], l-i-li);	
	d[2] -= 10;
	li = d[i+1];
	l -= 10;
      } break;

      case 0x86 :
      {
	memcpy(&d[i], &d[i + 2 + li],l-i-li+2);
	l	-= (li +2);
	d[2]	-= (li +2);
	continue;
      }

      case 0xdf  : return l; 

    }
    i += ( 2 + li );
  }
  return l;
}

void cCryptoworks::CW_PDUDEC(byte* d, int l, byte* k )
{
  byte	pi, li, i = 0, j,key[22], algo = d[0], buf[250];
  
  memcpy(key,k,22);
  memcpy(buf, d, l);
  memset(d,0,l);
  
  l = CW_PDUSDEC(buf,l,k); 
  memcpy(d,buf,l);
  
  while ( i < l )
  {
    pi = d[i];
    li = d[i + 1];
    if (i == 0 && li == 0) li = 1;
    if(pi == 0xdf) return;
    if ( ( pi == 0xda ) || ( pi == 0xdb ) || ( pi == 0xdc ) )
      for ( j = 0; j < ( li / 8 ); j++ )
	CW_DEC(&d[i + 2 + ( j * 8 )], key, algo);   
    i += ( 2 + li );
  }
  return;
}

void cCryptoworks::CW_PDUENC(byte* d, int l, byte* k )
{
  byte	pi, li, i = 0, j, key[22], algo = d[0] & 7;
  
  memcpy(key,k,22);
  
  while ( i < l )
  {
    pi = d[i];
    li = d[i + 1];
    if (i == 0 && li == 0) li = 1;
    if(pi == 0xdf) return;
    if ( ( pi == 0xda ) || ( pi == 0xdb ) || ( pi == 0xdc ) )
      for ( j = 0; j < ( li / 8 ); j++ )
	CW_ENC(&d[i + 2 + ( j * 8 )], key, algo);   
    i += ( 2 + li );
  }
  return;
}

void cCryptoworks::CW_DCW(byte *d ,byte l , byte *dcw)
{
  byte i = 0, pi, li;
  
  while ( i < l )
  {
    pi = d[i];
    li = d[i + 1];
    if (i == 0) li = 1;	
    if ( pi == 0xdf ) return;
    if ( pi == 0xdb ) {
      memcpy(dcw,d+i+2, li);
      return;
    }
    i += ( 2 + li );
  }			
}


// -- cPlainKeyCryptoworks -----------------------------------------------------

#define PLAINLEN_CW_D  16
#define PLAINLEN_CW_CC  6
#define PLAINLEN_CW_R  64

#define CCTYP               0x00
#define CCID                0xFF

#define PROV(keynr)         (((keynr)>>16)&0xFF)
#define TYPE(keynr)         (((keynr)>> 8)&0xFF)
#define ID(keynr)           (((keynr)   )&0xFF)


class cPlainKeyCryptoworks : public cDualKey {
protected:
  virtual bool IsBNKey(void) const;
  virtual int IdSize(void) { return 4; }
  virtual cString PrintKeyNr(void);
public:
  cPlainKeyCryptoworks(bool Super);
  virtual bool Parse(const char *line);
  };

static cPlainKeyTypeReg<cPlainKeyCryptoworks,'W'> KeyReg;

cPlainKeyCryptoworks::cPlainKeyCryptoworks(bool Super)
:cDualKey(Super,true)
{}

bool cPlainKeyCryptoworks::IsBNKey(void) const
{
  return TYPE(keynr)==0x10;
}

bool cPlainKeyCryptoworks::Parse(const char *line)
{
  unsigned char sid[2], sprov;
  if(GetChar(line,&type,1) && GetHex(line,sid,2) && GetHex(line,&sprov,1)) {
    int keylen, prov;
    type=toupper(type); id=Bin2Int(sid,2); prov=sprov;
    line=skipspace(line);
    bool ok;
    if(!strncasecmp(line,"CC",2) || !strncasecmp(line,"06",2)) { // cardkey
      keynr=KEYSET(prov,CCTYP,CCID);
      keylen=PLAINLEN_CW_CC;
      line+=2;
      ok=true;
      }
    else {
      unsigned char sid, styp;
      ok=GetHex(line,&styp,1) && GetHex(line,&sid,1);
      keynr=KEYSET(prov,styp,sid);
      keylen=IsBNKey() ? PLAINLEN_CW_R : PLAINLEN_CW_D;
      }
    if(ok) {
      unsigned char *skey=AUTOMEM(keylen);
      if(GetHex(line,skey,keylen)) {
        SetBinKey(skey,keylen);
        return true;
        }
      }
    }
  return false;
}

cString cPlainKeyCryptoworks::PrintKeyNr(void)
{
  int prov=PROV(keynr);
  int keytyp=TYPE(keynr);
  int keyid=ID(keynr);
  return cString::sprintf(keytyp==CCTYP && keyid==CCID ? "%02X CC":"%02X %02X %02X",prov,keytyp,keyid);
}

// -- cSystemCryptoworks -------------------------------------------------------

#define ECM_ALGO_TYP   5
#define ECM_DATA_START ECM_ALGO_TYP
#define ECM_NANO_LEN   7
#define ECM_NANO_START 8

class cSystemCryptoworks : public cSystem, private cCryptoworks {
private:
public:
  cSystemCryptoworks(void);
  virtual bool ProcessECM(const cEcmInfo *ecmInfo, unsigned char *data);
//  virtual void ProcessEMM(int pid, int caid, unsigned char *data);
  };

cSystemCryptoworks::cSystemCryptoworks(void)
:cSystem(SYSTEM_NAME,SYSTEM_PRI)
{
//  hasLogger=true;
}

bool cSystemCryptoworks::ProcessECM(const cEcmInfo *ecmInfo, byte *ecm)
{
  byte key[22];
  int len, prov, keyid;
  ecm += 5;
  
  len = ecm[2] + 3;
  curCaId = ecmInfo->caId;

  for (int i = 0; i < len; i++) 
  {
    switch( ecm[i] )
    {
      case 0x83:	// KEY
      case 0x84:
	curProv = prov = ecm[i + 2] & 0xFC;
	keyid = ecm[ i + 2] & 3;
	i += ecm[i + 1] + 1;
	break;
	
      case 0xDF:
      case 0x85:
      case 0x80:
      case 0x86:
      case 0x8A:
      case 0x8B:
      case 0x8C:
      case 0x8E:
      case 0x90:
      case 0x91:
      case 0xD7:
      case 0xDB:	// CWS
	i += ecm[i + 1] + 1;
	break;
	
      default: break;
    }
  }
  
  cPlainKey *pk;
  if(!(pk=keys.FindKey('W',ecmInfo->caId,KEYSET(prov,CCTYP,CCID),6))) {
    if(doLog) PRINTF(L_SYS_KEY,"missing %04X %02X CC key",ecmInfo->caId,prov);
    return false;
  }
  pk->Get(key+16);

  if(!(pk=keys.FindKey('W',ecmInfo->caId,KEYSET(prov,0x20,keyid),16))) {
    if(doLog) PRINTF(L_SYS_KEY,"missing %04X %02X 20 %02X key",ecmInfo->caId,prov,keyid);
    return false;
  }
  pk->Get(key);
  
  if ( CW_SIGCHK(ecm, len, key ) == 0 ) return false;
  CW_PDUDEC (ecm, len -10, key);
  CW_DCW(ecm, (byte)(len -10), cw);
  
  return true;
}

/*bool cSystemCryptoworks::ProcessECM(const cEcmInfo *ecmInfo, unsigned char *data)
{
  int len=SCT_LEN(data);
  if(data[ECM_NANO_LEN]!=len-ECM_NANO_START) {
    PRINTF(L_SYS_ECM,"invalid ECM structure");
    return false;
    }

  int prov=-1, keyid=0;
  for(int i=ECM_NANO_START; i<len; i+=data[i+1]+2) {
    if(data[i]==0x83) {
      prov =data[i+2]&0xFC;
      keyid=data[i+2]&0x03;
      break;
      }
    }
  if(prov<0) {
    PRINTF(L_SYS_ECM,"provider ID not located in ECM");
    return false;
    }

  unsigned char key[22];
  cPlainKey *pk;
  if(!(pk=keys.FindKey('W',ecmInfo->caId,KEYSET(prov,CCTYP,CCID),6))) {
    if(doLog) PRINTF(L_SYS_KEY,"missing %04X %02X CC key",ecmInfo->caId,prov);
    return false;
    }
  pk->Get(key+16);

  // RSA stage
  for(int i=ECM_NANO_START; i<len; i+=data[i+1]+2) {
    int l=data[i+1]+2;
    switch(data[i]) {
      case 0x85:
        {
        if(!(pk=keys.FindKey('W',ecmInfo->caId,KEYSET(prov,0x31,keyid),16))) {
          if(doLog) PRINTF(L_SYS_KEY,"missing %04X %02X 31 %02X key",ecmInfo->caId,prov,keyid);
          return false;
          }
        pk->Get(key);
        cBN mod;
        if(!(pk=keys.FindKey('W',ecmInfo->caId,KEYSET(prov,0x10,0x00),64))) {
          if(doLog) PRINTF(L_SYS_KEY,"missing %04X %02X 10 00 key",ecmInfo->caId,prov);
          return false;
          }
        pk->Get(mod);

        l-=10;
    //    if(!DecryptRSA(&data[i+2],l,data[ECM_ALGO_TYP],key,mod))
    //      return false;
        memmove(&data[i],&data[i+2],l);
        memmove(&data[i+l],&data[i+l+10],len-i-l);
        len-=10;
        break;
        }
      case 0x86:
        memmove(&data[i],&data[i+l],len-i-l);
        len-=l;
        continue;
      }
    }

  cKeySnoop ks(this,'W',ecmInfo->caId,KEYSET(prov,0x20,keyid));
  if(!(pk=keys.FindKey('W',ecmInfo->caId,KEYSET(prov,0x20,keyid),16))) {
    if(doLog) PRINTF(L_SYS_KEY,"missing %04X %02X 20 %02X key",ecmInfo->caId,prov,keyid);
    return false;
    }
  pk->Get(key);

  // DES stage
  unsigned char sig[8];
  LDUMP(L_SYS_VERBOSE,&data[len-8],8,"sig org:");
  data[ECM_NANO_LEN]=len-ECM_NANO_START;
//  Signature(&data[ECM_DATA_START],len-ECM_DATA_START-10,key,sig);
  for(int i=ECM_NANO_START; i<len; i+=data[i+1]+2) {
    switch(data[i]) {
      case 0xDA:
      case 0xDB:
      case 0xDC:
        for(int j=0; j<data[i+1]; j+=8)
//          DecryptDES(&data[i+2+j],data[ECM_ALGO_TYP],key);
        break;
      case 0xDF:
        if(memcmp(&data[i+2],sig,8)) {
          PRINTF(L_SYS_ECM,"signature failed in ECM");
          return false;
          }
        break;
      }
    }
  
  // CW stage
  for(int i=ECM_NANO_START; i<len; i+=data[i+1]+2) {
    switch(data[i]) {
      case 0xDB:
        if(data[i+1]==0x10) {
          memcpy(cw,&data[i+2],16);
          LDUMP(L_SYS_VERBOSE,cw,16,"cw:");
          ks.OK(pk);
          return true;
          }
        break;
      }
    }

  return false;
}
*/
/*
void cSystemCryptoworks::ProcessEMM(int pid, int caid, unsigned char *data)
{
}
*/

// -- cSystemLinkCryptoworks ---------------------------------------------------

class cSystemLinkCryptoworks : public cSystemLink {
public:
  cSystemLinkCryptoworks(void);
  virtual bool CanHandle(unsigned short SysId);
  virtual cSystem *Create(void) { return new cSystemCryptoworks; }
  };

static cSystemLinkCryptoworks staticInit;

cSystemLinkCryptoworks::cSystemLinkCryptoworks(void)
:cSystemLink(SYSTEM_NAME,SYSTEM_PRI)
{
  Feature.NeedsKeyFile();
}

bool cSystemLinkCryptoworks::CanHandle(unsigned short SysId)
{
  SysId&=SYSTEM_MASK;
  return SYSTEM_CAN_HANDLE(SysId);
}


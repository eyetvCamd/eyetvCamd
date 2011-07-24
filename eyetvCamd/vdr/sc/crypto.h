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

#ifndef ___CRYPTO_H
#define ___CRYPTO_H

//
// NOTE: all crypto classes SHOULD be fully reentrant.
//       They may be called from different threads concurrently.
//
// NOTE: cAES is not fully reentrant. Encrypt/Decrypt are reentrant
//       (as AES_KEY is const there), but SetKey is not. Be carefull.
//

#define OPENSSL_ALGORITHM_DEFINES
#include <openssl/opensslconf.h>
#include <openssl/opensslv.h>

#if OPENSSL_VERSION_NUMBER < 0x0090700fL
#error Openssl version 0.9.7 or newer is strongly recomended
#endif

#include "crypto-bn.h"

// ----------------------------------------------------------------

#define DES_LEFT    0
#define DES_RIGHT   1

#define DES_HASH    8
#define DES_PC1    16
#define DES_IP     32
#define DES_FP     64
#define DES_MOD   128

// common DES modes

#define PRV_DES_ENCRYPT (DES_LEFT|DES_PC1|DES_IP|DES_FP)
#define PRV_DES_DECRYPT (DES_RIGHT|DES_PC1|DES_IP|DES_FP)

#define VIA_DES       (DES_LEFT|DES_MOD)
#define VIA_DES_HASH  (DES_LEFT|DES_HASH|DES_MOD)
#define SECA_DES_ENCR  PRV_DES_ENCRYPT
#define SECA_DES_DECR  PRV_DES_DECRYPT
#define NAGRA_DES_ENCR PRV_DES_ENCRYPT
#define NAGRA_DES_DECR PRV_DES_DECRYPT

class cDes {
private:
  static const unsigned char _E[], _P[], _PC1[], _PC2[], IP[], FP[];
protected:
  static const unsigned char S[][64], LS[];
  const unsigned char *PC1, *PC2;
  unsigned char E[48], P[32];
  //
  virtual unsigned int Mod(unsigned int R, unsigned int key7) const { return R; }
  void Permute(unsigned char *data, const unsigned char *P, int n) const;
public:
  cDes(const unsigned char *pc1=0, const unsigned char *pc2=0);
  virtual ~cDes() {}
  void Des(unsigned char *data, const unsigned char *key, int mode) const;
  };

// ----------------------------------------------------------------

#if defined(OPENSSL_NO_AES) | defined(NO_AES) | OPENSSL_VERSION_NUMBER<0x0090700fL
#include "support/aes.h"
#else
#define OPENSSL_HAS_AES
#include <openssl/aes.h>
#endif

class cAES {
private:
  bool active;
  AES_KEY dkey, ekey;
protected:
  void SetKey(const unsigned char *key);
  void Decrypt(unsigned char *data, int len) const ;
  int Encrypt(const unsigned char *data, int len, unsigned char *crypt) const;
public:
  cAES(void);
  };

// ----------------------------------------------------------------

#if defined(OPENSSL_NO_IDEA) | defined(NO_IDEA) | OPENSSL_VERSION_NUMBER<0x0090700fL
#include "support/idea.h"
#else
#define OPENSSL_HAS_IDEA
#include <openssl/idea.h>
#endif

typedef IDEA_KEY_SCHEDULE IdeaKS;

class cIDEA {
public:
  // single shot API
  void Decrypt(unsigned char *data, int len, const unsigned char *key, unsigned char *iv) const;
  int Encrypt(const unsigned char *data, int len, unsigned char *crypt, const unsigned char *key, unsigned char *iv) const;
  // multi shot API
  void SetEncKey(const unsigned char *key, IdeaKS *ks) const;
  void SetDecKey(const unsigned char *key, IdeaKS *ks) const;
  void Decrypt(unsigned char *data, int len, IdeaKS *ks, unsigned char *iv) const;
  int Encrypt(const unsigned char *data, int len, unsigned char *crypt, IdeaKS *ks, unsigned char *iv) const;
  };

// ----------------------------------------------------------------

class cRSA {
private:
  bool Input(cBN *d, const unsigned char *in, int n, bool LE) const;
  int Output(unsigned char *out, int n, cBN *r, bool LE) const;
public:
  int RSA(unsigned char *out, const unsigned char *in, int n, const BIGNUM *exp, const BIGNUM *mod, bool LE=true) const;
  int RSA(BIGNUM *out, const unsigned char *in, int n, const BIGNUM *exp, const BIGNUM *mod, bool LE=true) const;
  int RSA(unsigned char *out, int len, BIGNUM *in, const BIGNUM *exp, const BIGNUM *mod, bool LE=true) const;
  };

#endif //___CRYPTO_H

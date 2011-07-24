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

#ifndef ___CRYPTO_BN_H
#define ___CRYPTO_BN_H

#include <openssl/bn.h>

// ----------------------------------------------------------------

void RotateBytes(unsigned char *in, int n);
void RotateBytes(unsigned char *out, const unsigned char *in, int n);

// ----------------------------------------------------------------

class cBN {
private:
  BIGNUM big;
public:
  cBN(void) { BN_init(&big); }
  ~cBN() { BN_free(&big); }
  operator BIGNUM* () { return &big; }
  BIGNUM *operator->() { return &big; }
  bool Get(const unsigned char *in, int n);
  bool GetLE(const unsigned char *in, int n);
  int Put(unsigned char *out, int n) const;
  int PutLE(unsigned char *out, int n) const;
  };

class cBNctx {
private:
  BN_CTX *ctx;
public:
  cBNctx(void) { ctx=BN_CTX_new(); }
  ~cBNctx() { BN_CTX_free(ctx); }
  operator BN_CTX* () { return ctx; }
  };

#endif //___CRYPTO_BN_H

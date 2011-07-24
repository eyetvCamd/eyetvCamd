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

#ifndef __NAGRA_NAGRA_H
#define __NAGRA_NAGRA_H

#include "system-common.h"
#include "crypto.h"

// ----------------------------------------------------------------

extern int minEcmTime;

// ----------------------------------------------------------------

class cPlainKeyNagra : public cDualKey {
private:
  int ParseTag(const char *tag, const char * &line);
  void GetTagDef(int nr, int &romnr, int &pk, int &keytype);
protected:
  virtual bool IsBNKey(void) const;
  virtual int IdSize(void) { return 4; }
  virtual cString PrintKeyNr(void);
public:
  cPlainKeyNagra(bool Super);
  virtual bool Parse(const char *line);
  static bool IsBNKey(int kn);
  };

// ----------------------------------------------------------------

class cNagra {
protected:
  cRSA rsa;
  cBN pubExp;
  //
  virtual void CreatePQ(const unsigned char *pk, BIGNUM *p, BIGNUM *q)=0;
  void ExpandPQ(BIGNUM *p, BIGNUM *q, const unsigned char *data, BIGNUM *e, BIGNUM *m);
  void CreateRSAPair(const unsigned char *key, const unsigned char *data, BIGNUM *e, BIGNUM *m);
public:
  cNagra(void);
  virtual ~cNagra() {};
  };

#endif

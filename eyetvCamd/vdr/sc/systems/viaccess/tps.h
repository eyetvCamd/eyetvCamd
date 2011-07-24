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

#ifndef __VIACCESS_TPS_H
#define __VIACCESS_TPS_H

#include <time.h>
#include "../../../tools.h"
#include "../../../thread.h"
#include "data.h"
#include "crypto.h"

class cSatTime;
class cTpsAuHook;
class cOpenTVModule;
class cST20;

// ----------------------------------------------------------------

#define RC6_ROUNDS	20
#define RC6_MAX		(RC6_ROUNDS*2+4)

class cRC6 {
private:
  unsigned int key[RC6_MAX];
  //
  unsigned int rol(unsigned int v, unsigned int cnt);
  unsigned int ror(unsigned int v, unsigned int cnt);
public:
  void SetKey(const unsigned char *Key, int len);
  void Decrypt(unsigned char *data);
  };

// ----------------------------------------------------------------

class cTPSDecrypt : private cAES, private cRC6 {
private:
  static unsigned char *mem;
  static int memLen, cb1off, cb2off, cb3off;
  static cMutex st20Mutex;
  static cST20 st20;
  static bool st20Inited;
  //
  static bool InitST20(void);
  static bool DecryptAlgo3(const unsigned char *key, unsigned char *data);
protected:
  void TpsDecrypt(unsigned char *data, short mode, const unsigned char *key);
  static bool RegisterAlgo3(const unsigned char *data, int cb1, int cb2, int cb3, int len);
  static unsigned char *DumpAlgo3(int &len, int &cb1, int &cb2, int &cb3);
  static bool Handle80008003(const unsigned char *src, int len, unsigned char *dest);
  };

// ----------------------------------------------------------------

class cTPS : private cTPSDecrypt {
private:
  cSatTime *sattime;
  int doPost;
  short postMode;
  unsigned char postKey[16];
public:
  cTPS(void);
  ~cTPS();
  int Decrypt(int cardNum, int Source, int Transponder, unsigned char *data, int len);
  void PostProc(unsigned char *cw);
  };

// ----------------------------------------------------------------

class cTpsKey : public cStructItem {
private:
  time_t timestamp;
  int opmode;
  struct {
    unsigned char mode;
    unsigned char key[16];
    } step[3];
  //
  void Put(unsigned char *mem) const;
public:
  cTpsKey(void);
  const unsigned char *Key(int st) const { return step[st].key; }
  int Mode(int st) const { return step[st].mode; }
  int Opmode(void) const { return opmode; }
  time_t Timestamp(void) const { return timestamp; }
  void Set(const cTpsKey *k);
  void Set(const unsigned char *mem);
  virtual cString ToString(bool hide=false);
  };

// ----------------------------------------------------------------

class cTpsKeys : public cStructListPlain<cTpsKey>, private cTPSDecrypt {
friend class cTpsAuHook;
private:
  time_t first, last;
  cSimpleList<cTpsKey> *loadlist;
  //
  cTimeMs lastCheck, lastLoad, lastAu;
  cMutex checkMutex;
  //
  unsigned char *algomem;
  int algolen, algoread, cb1off, cb2off, cb3off;
  //
  void Join(cSimpleList<cTpsKey> *nlist);
  void Purge(time_t now);
  void GetFirstLast(void);
//  bool LoadBin(void);
//  void DecryptBin(const unsigned char *in, unsigned char *out);
  cString Time(time_t t);
  bool ProcessAu(const cOpenTVModule *mod);
protected:
  virtual bool ParseLinePlain(const char *line);
  virtual void PreLoad(void);
  virtual void PostLoad(void);
  virtual void PostSave(FILE *f);
public:
  cTpsKeys(void);
  ~cTpsKeys();
  const cTpsKey *GetKey(time_t t);
  const cTpsKey *GetV2Key(int id);
  void Check(time_t now, int cardnum);
  };

extern cTpsKeys tpskeys;

#endif

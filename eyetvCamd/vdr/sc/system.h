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

#ifndef ___SYSTEM_H
#define ___SYSTEM_H

#include "../tools.h"

#include "data.h"
#include "misc.h"

// ----------------------------------------------------------------

#define SYSTEM_MASK 0xFF00

// ----------------------------------------------------------------

class cEcmInfo;
class cPlainKey;
class cSystem;
class cSystems;
class cLogger;
class cOpts;
class cHookManager;

// ----------------------------------------------------------------

class cFeature {
private:
  static bool keyfile, smartcard;
public:
  void NeedsKeyFile(void);
  void NeedsSmartCard(void);
  bool KeyFile(void) const { return keyfile; }
  bool SmartCard(void) const { return smartcard; }
  };

extern cFeature Feature;

// ----------------------------------------------------------------

class cKeySnoop {
private:
  cSystem *sys;
  int type, id, keynr;
  bool ok;
public:
  cKeySnoop(cSystem *Sys, int Type, int Id, int Keynr);
  ~cKeySnoop();
  void OK(cPlainKey *pk);
  };

// ----------------------------------------------------------------

class cLogHook : public cSimpleItem {
friend class cHookManager;
private:
  int cardNum, id;
  const char *name;
  cTimeMs delay;
  bool bailOut;
protected:
  cPids pids;
public:
  cLogHook(int Id, const char *Name);
  virtual ~cLogHook() {}
  virtual void Process(int pid, unsigned char *data)=0;
  void BailOut(void) { bailOut=true; }
  };

// ----------------------------------------------------------------

struct EcmCheck;

class cSystem : public cSimpleItem {
friend class cKeySnoop;
private:
  static int foundKeys, newKeys;
  int pri, cardNum;
  cLastKey lastkey;
  char *lastTxt;
  char currentKeyStr[48];
  struct EcmCheck *check;
  cEcmInfo *logecm;
protected:
  const char *name;
  unsigned char cw[16];
  bool doLog;
  // config details
  int maxEcmTry;
  bool local, hasLogger, needsLogger, needsDescrData, constant;
  //
  void KeyOK(cPlainKey *pk);
  void KeyOK(const char *txt);
  void KeyFail(int type, int id, int keynr);
  void StartLog(const cEcmInfo *ecm, int caid);
public:
  cSystem(const char *Name, int Pri);
  virtual ~cSystem();
  virtual int CheckECM(const cEcmInfo *ecm, const unsigned char *data, bool sync);
  virtual void CheckECMResult(const cEcmInfo *ecm, const unsigned char *data, bool result);
  virtual bool ProcessECM(const cEcmInfo *ecm, unsigned char *buffer)=0;
  virtual void ProcessEMM(int pid, int caid, unsigned char *buffer) {};
  virtual void ParseCADescriptor(cSimpleList<cEcmInfo> *ecms, unsigned short sysId, const unsigned char *data, int len);
  virtual void ParseCAT(cPids *pids, const unsigned char *buffer);
  unsigned char *CW(void) { return cw; }
  void DoLog(bool Log) { doLog=Log; }
  int Pri(void) { return pri; }
  const char *CurrentKeyStr(void) { return currentKeyStr; }
  const char *Name(void) { return name; }
  int CardNum(void) { return cardNum; }
  void CardNum(int CardNum) { cardNum=CardNum; }
  int MaxEcmTry(void) { return maxEcmTry; }
  bool HasLogger(void) { return hasLogger; }
  bool NeedsLogger(void) { return needsLogger; }
  bool Local(void) { return local; }
  bool NeedsData(void) { return needsDescrData; }
  bool Constant(void) { return constant; }
  static void FoundKey(void) { foundKeys++; }
  static void NewKey(void) { newKeys++; }
  static void KeyStats(int &fk, int &nk) { fk=foundKeys; nk=newKeys; }
  };

// ----------------------------------------------------------------

class cSystemLink {
friend class cSystems;
private:
  int sysIdent;
protected:
  cSystemLink *next;
  const char *name;
  int pri;
  cOpts *opts;
  bool noFF;
public:
  cSystemLink(const char *Name, int Pri);
  virtual ~cSystemLink();
  virtual bool CanHandle(unsigned short SysId)=0;
  virtual cSystem *Create(void)=0;
  virtual bool Init(const char *cfgdir) { return true; }
  virtual void Clean(void) {}
  virtual void NewConfig(void) {}
  };

// ----------------------------------------------------------------

class cSystems {
friend class cSystemLink;
private:
  static cSystemLink *first;
  static int nextSysIdent;
  //
  static void Register(cSystemLink *sysLink);
  static cSystemLink *FindByName(const char *Name);
  static cSystemLink *FindById(unsigned short SysId, bool ff, int oldPri);
  static cSystemLink *FindByIdent(int ident);
public:
  static int Provides(const unsigned short *SysIds, bool ff);
  static cSystem *FindBySysId(unsigned short SysId, bool ff, int oldPri);
  static cSystem *FindBySysName(unsigned short SysId, bool ff, const char *Name);
  static int FindIdentBySysId(unsigned short SysId, bool ff, int &Pri);
  static cSystem *FindBySysIdent(int ident);
  static bool Init(const char *cfgdir);
  static void Clean(void);
  //
  static bool ConfigParse(const char *Name, const char *Value);
  static void ConfigStore(bool AsIs);
  static cOpts *GetSystemOpts(bool start);
  };

// ----------------------------------------------------------------

struct Cache;

class cMsgCache {
private:
  struct Cache *caches;
  unsigned char *stores;
  int numCache, ptr, storeSize, maxFail;
  cMutex mutex;
  cCondVar wait;
  //
  struct Cache *FindMsg(int crc);
public:
  cMsgCache(int NumCache, int StoreSize);
  ~cMsgCache();
  int Cache(int id, bool result, const unsigned char *store);
  int Get(const unsigned char *msg, int len, unsigned char *store);
  void Clear(void);
  void SetMaxFail(int max);
  };

#endif //___SYSTEM_H

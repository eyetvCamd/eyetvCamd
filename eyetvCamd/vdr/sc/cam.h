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

#ifndef ___CAM_H
#define ___CAM_H

#include <linux/dvb/ca.h>
#include <vdr/dvbdevice.h>
#include <vdr/thread.h>
#include "data.h"
#include "misc.h"

class cChannel;

class cEcmHandler;
class cEcmData;
class cLogger;
class cHookManager;
class cLogHook;
class cDeCSA;
class cDeCsaTSBuffer;
class cScCiAdapter;
class cScDvbDevice;
class cPrg;

// ----------------------------------------------------------------

class cEcmCache : public cStructListPlain<cEcmData> {
private:
  cEcmData *Exists(cEcmInfo *e);
protected:
  virtual bool ParseLinePlain(const char *line);
public:
  cEcmCache(void);
  void New(cEcmInfo *e);
  int GetCached(cSimpleList<cEcmInfo> *list, int sid, int Source, int Transponder);
  void Delete(cEcmInfo *e);
  void Flush(void);
  };

extern cEcmCache ecmcache;

// ----------------------------------------------------------------

class cPrgPid : public cSimpleItem {
private:
  int type, pid;
  bool proc;
public:
  cPrgPid(int Type, int Pid) { type=Type; pid=Pid; proc=false; }
  int Pid(void) { return pid; }
  int Type(void) { return type; }
  bool Proc(void) { return proc; }
  void Proc(bool is) { proc=is; };
  };

// ----------------------------------------------------------------

class cPrg : public cSimpleItem {
private:
  int prg;
  bool isUpdate;
public:
  cSimpleList<cPrgPid> pids;
  //
  cPrg(int Prg, bool IsUpdate) { prg=Prg; isUpdate=IsUpdate; }
  int Prg(void) { return prg; }
  bool IsUpdate(void) { return isUpdate; }
  };

// ----------------------------------------------------------------

#if APIVERSNUM >= 10500
typedef int caid_t;
#else
typedef unsigned short caid_t;
#endif

#define MAX_CW_IDX        16
#define MAX_CI_SLOTS      8
#ifdef VDR_MAXCAID
#define MAX_CI_SLOT_CAIDS VDR_MAXCAID
#else
#define MAX_CI_SLOT_CAIDS 16
#endif

class cCam : private cMutex {
private:
  int cardNum;
  cScDvbDevice *device;
  cSimpleList<cEcmHandler> handlerList;
  cLogger *logger;
  cHookManager *hookman;
  int source, transponder, liveVpid, liveApid;
  unsigned char indexMap[MAX_CW_IDX], lastCW[MAX_CW_IDX][2*8];
  //
  cEcmHandler *GetHandler(int sid, bool needZero, bool noshift);
  void RemHandler(cEcmHandler *handler);
  int GetFreeIndex(void);
  void LogStartup(void);
public:
  cCam(cScDvbDevice *dev, int CardNum);
  virtual ~cCam();
  // EcmHandler API
  void WriteCW(int index, unsigned char *cw, bool force);
  void SetCWIndex(int pid, int index);
  void DumpAV7110(void);
  void LogEcmStatus(const cEcmInfo *ecm, bool on);
  bool GetPrgCaids(int source, int transponder, int prg, caid_t *c);
  void AddHook(cLogHook *hook);
  bool TriggerHook(int id);
  // Plugin API
  bool Active(bool log);
  void HouseKeeping(void);
  void Tune(const cChannel *channel);
  void PostTune(void);
  void SetPid(int type, int pid, bool on);
  void Stop(void);
  void AddPrg(cPrg *prg);
  bool HasPrg(int prg);
  char *CurrentKeyStr(int num);
  //
  bool IsSoftCSA(void);
  int Source(void) { return source; }
  int Transponder(void) { return transponder; }
  };

void LogStatsDown(void);

// ----------------------------------------------------------------

#define MAX_LRU_CAID 10

class cScDvbDevice : public cDvbDevice {
private:
  cDeCSA *decsa;
  cDeCsaTSBuffer *tsBuffer;
#if APIVERSNUM >= 10500
  cScCiAdapter *ciadapter;
  cCiAdapter *hwciadapter;
#endif
  cCam *cam;
  int fd_dvr, fd_ca, fd_ca2;
  bool softcsa;
  cMutex cafdMutex;
  cTimeMs lastDump;
  struct LruCaid {
    int src, tr, prg;
    caid_t caids[MAXCAIDS+1];
    } lrucaid[MAX_LRU_CAID];
  cMutex lruMutex;
  static int budget;
  //
#ifndef SASC
  void LateInit(void);
  void EarlyShutdown(void);
  int FindLRUPrg(int source, int transponder, int prg);
#endif //SASC
protected:
#ifndef SASC
#if APIVERSNUM >= 10500
  virtual bool Ready(void);
#else
  virtual void CiStartDecrypting(void);
  virtual bool CiAllowConcurrent(void) const;
#endif
  virtual bool SetPid(cPidHandle *Handle, int Type, bool On);
  virtual bool SetChannelDevice(const cChannel *Channel, bool LiveView);
  virtual bool OpenDvr(void);
  virtual void CloseDvr(void);
  virtual bool GetTSPacket(uchar *&Data);
#endif //SASC
public:
  cScDvbDevice(int n, int cafd);
  ~cScDvbDevice();
#ifndef SASC
#if APIVERSNUM >= 10501
  virtual bool HasCi(void);
#endif
#if APIVERSNUM < 10500
  virtual int ProvidesCa(const cChannel *Channel) const;
#endif
#endif //SASC
  static void Capture(void);
  static bool Initialize(void);
  static void Startup(void);
  static void Shutdown(void);
  static void SetForceBudget(int n);
  static bool ForceBudget(int n);
  virtual bool SetCaDescr(ca_descr_t *ca_descr, bool initial);
  virtual bool SetCaPid(ca_pid_t *ca_pid);
  void DumpAV7110(void);
  cCam *Cam(void) { return cam; }
  bool SoftCSA(void) { return softcsa; }
  virtual bool GetPrgCaids(int source, int transponder, int prg, caid_t *c);
  };

#endif // ___CAM_H

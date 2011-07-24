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

#ifndef ___LOG_H
#define ___LOG_H

#include "misc.h"

class cMutex;

// ----------------------------------------------------------------

#define LOPT_NUM    24
#define LOPT_MAX    (1<<LOPT_NUM)
#define LOPT_MASK   (LOPT_MAX-1)
#define LMOD_MAX    (1<<(32-LOPT_NUM))
#define LMOD_MASK   (LMOD_MAX-1)
#define LMOD_ENABLE 1

#define LCLASS(m,o) ((((m)&LMOD_MASK)<<LOPT_NUM)|((o)&LOPT_MASK))
#define LMOD(c)     (((c)>>LOPT_NUM)&LMOD_MASK)
#define LOPT(o)     ((o)&LOPT_MASK)
#define LALL(o)     (((o)<<1)-1)

#define ADD_MODULE(MOD,NAME) static bool __add_mod_ ## NAME = cLogging::AddModule(MOD,&NAME);

#define PRINTF(c,...)       cLogging::Printf((c),__VA_ARGS__)
#define PUTS(c,t)           cLogging::Puts((c),(t))
#define PUTLB(c,lb)         cLogging::PutLB((c),(lb))
#define HEXDUMP(c,d,n,...)  cLogging::Dump((c),(d),(n),__VA_ARGS__)
#define LDUMP(c,d,n,...)    cLogging::LineDump((c),(d),(n),__VA_ARGS__)
#define LOG(c)              cLogging::Enabled((c))

// backward compatibility
#define HexDump(d,n)        do { int __n=(n); HEXDUMP(L_GEN_MISC,(d),__n,"dump: n=%d/0x%04x",__n,__n); } while(0)

#define LBSTART(c)          do { int __c=(c); if(LOG(__c)) { cLogLineBuff __llb(__c)
#define LBSTARTF(c)         do { int __c=(c); { cLogLineBuff __llb(__c)
#define LBPUT(...)          __llb.Printf(__VA_ARGS__)
#define LBFLUSH()           __llb.Flush()
#define LBEND( )            } } while(0)

// ----------------------------------------------------------------

#define L_GEN       0
#define L_GEN_ERROR LCLASS(L_GEN,0x2)
#define L_GEN_WARN  LCLASS(L_GEN,0x4)
#define L_GEN_INFO  LCLASS(L_GEN,0x8)
#define L_GEN_DEBUG LCLASS(L_GEN,0x10)
#define L_GEN_MISC  LCLASS(L_GEN,0x20)

#define L_GEN_ALL   LALL(L_GEN_MISC)

// ----------------------------------------------------------------

struct LogConfig {
  int logCon, logFile, logSys, logUser, noTimestamp;
  int maxFilesize;
  char logFilename[128];
  };

extern struct LogConfig logcfg;

struct LogModule {
  int OptSupported, OptDefault;
  const char *Name, *OptName[LOPT_NUM];
  };

struct LogHeader {
  int c;
  char stamp[32];
  char tag[64];
  };

class cLogging {
private:
  static void (*LogPrint)(const struct LogHeader *lh, const char *txt);
  //
  static void PrivateLogPrint(const struct LogHeader *lh, const char *txt);
  static bool GetHeader(int c, struct LogHeader *lh);
  static void LogLine(const struct LogHeader *lh, const char *txt, bool doCrc=true);
  static const struct LogModule *GetModule(int c);
  static void UpgradeOptions(int m);
public:
  static bool AddModule(int m, const struct LogModule *dm);
  static void SetLogPrint(void (*lp)(const struct LogHeader *lh, const char *txt));
  //
  static void Printf(int c, const char *format, ...) __attribute__ ((format (printf,2,3)));
  static void Puts(int c, const char *txt);
  static void PutLB(int c, cLineBuff *lb);
  static void Dump(int c, const void *data, int n, const char *format, ...) __attribute__ ((format (printf,4,5)));
  static void LineDump(int c, const void *data, int n, const char *format, ...) __attribute__ ((format (printf,4,5)));
  static bool Enabled(int c);
  //
  static void SetModuleOptions(int c);
  static int GetModuleOptions(int c);
  static void SetModuleOption(int c, bool on);
  static void SetModuleDefault(int c);
  static const char *GetModuleName(int c);
  static const char *GetOptionName(int c);
  static void ParseConfig(const char *txt);
  static bool GetConfig(cLineBuff *lb);
  static void ReopenLogfile(void);
  static int GetClassByName(const char *name);
  };

// ----------------------------------------------------------------

class cUserMsg : public cSimpleItem {
private:
  char *msg;
public:
  cUserMsg(const char *m);
  ~cUserMsg();
  const char *Message(void) { return msg; };
  };

// ----------------------------------------------------------------

class cUserMsgs : public cSimpleList<cUserMsg> {
private:
  cMutex *mutex;
public:
  cUserMsgs(void);
  ~cUserMsgs();
  void Queue(const char *fmt, ...) __attribute__ ((format (printf,2,3)));
  cUserMsg *GetQueuedMsg(void);
  };

extern cUserMsgs ums;

// ----------------------------------------------------------------

class cLogLineBuff : public cLineBuff {
private:
  int c;
public:
  cLogLineBuff(int C);
  ~cLogLineBuff();
  void Flush(void);
  };

#endif //___LOG_H

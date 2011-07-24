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
#include <stdarg.h>
#include <time.h>

#include "../tools.h"
#include "../thread.h"

#include "log.h"
#include "misc.h"

#define LMOD_SUP 32
#define LMOD_CFG_VALID  0x80000000

struct LogConfig logcfg = {
  1,0,0,0,0,
  0,
  "/var/log/cwdwEmu"
  };

static const struct LogModule lm_general = {
  (LMOD_ENABLE|L_GEN_ALL)&LOPT_MASK,
  (LMOD_ENABLE|L_GEN_ALL)&LOPT_MASK,
  "general", { "error","warn","info","debug","misc" }
  };

static const struct LogModule *mods[LMOD_SUP] = { &lm_general };
static int config[LMOD_SUP] = { LMOD_ENABLE|L_GEN_ALL|LMOD_CFG_VALID };
static int cmask[LMOD_SUP]  = { LMOD_ENABLE|L_GEN_ALL };
static cMutex logfileMutex;
static FILE *logfile=0;
static bool logfileShutup=false, logfileReopen=false;
static long long logfileSize;

static unsigned int lastCrc=0;
static int lastC=0, lastCount=0;
static cTimeMs lastTime;
static cMutex lastMutex;

// -- cLogging -----------------------------------------------------------------

void (*cLogging::LogPrint)(const struct LogHeader *lh, const char *txt)=cLogging::PrivateLogPrint;

bool cLogging::AddModule(int m, const struct LogModule *lm)
{
  if(m<LMOD_SUP) {
    if(mods[m]) Printf(L_GEN_DEBUG,"duplicate logging module id %d (%s & %s)",m,mods[m]->Name,lm->Name);
    mods[m]=lm;
    if(config[m]&LMOD_CFG_VALID) UpgradeOptions(m);
    else SetModuleDefault(LCLASS(m,0));
//printf("module %-16s added. mod=%x supp=%x def=%x cfg=%x\n",lm->Name,m,lm->OptSupported,lm->OptDefault,config[m]);
    }
  else Printf(L_GEN_DEBUG,"failed to add logging module %d (%s)",m,lm->Name);
  return true;
}

void cLogging::SetLogPrint(void (*lp)(const struct LogHeader *lh, const char *txt))
{
  LogPrint=lp;
}

const struct LogModule *cLogging::GetModule(int c)
{
  int m=LMOD(c);
  return m<LMOD_SUP ? mods[m] : 0;
}

bool cLogging::GetHeader(int c, struct LogHeader *lh)
{
  const struct LogModule *lm=GetModule(c);
  if(lm) {
    if(!logcfg.noTimestamp) {
      time_t tt=time(0);
      struct tm tm_r;
      strftime(lh->stamp,sizeof(lh->stamp),"%b %e %T",localtime_r(&tt,&tm_r));
      }
    else lh->stamp[0]=0;
    int i, o=LOPT(c)&~LMOD_ENABLE;
    for(i=0; i<LOPT_NUM; i++,o>>=1) if(o&1) break;
    snprintf(lh->tag,sizeof(lh->tag),"%s.%s",lm->Name,(i>=1 && i<LOPT_NUM && lm->OptName[i-1])?lm->OptName[i-1]:"unknown");
    lh->c=c;
    return true;
    }
  return false;  
}

void cLogging::LogLine(const struct LogHeader *lh, const char *txt, bool doCrc)
{
  if(doCrc) {
    unsigned int crc=crc32_le(0,(const unsigned char *)txt,strlen(txt)+1);
    lastMutex.Lock();
    if(lastCrc==crc && lastC==lh->c && ++lastCount<4000000 && lastTime.Elapsed()<=30*1000) {
      lastMutex.Unlock();
      return;
      }
    else {
      unsigned int saveCrc=lastCrc;
      int saveCount=lastCount, saveC=lastC;
      lastTime.Set();
      lastCrc=crc; lastC=lh->c; lastCount=0;
      lastMutex.Unlock();
      if(saveCount>1) {
        struct LogHeader lh2;
        if(GetHeader(saveC,&lh2)) {
          char buff[128];
          snprintf(buff,sizeof(buff),"last message repeated %d times",saveCount);
          LogLine(&lh2,buff,false);
          }
        if(saveCrc==crc && saveC==lh->c) return;
        }
      }
    }

  LogPrint(lh,txt);
}

void cLogging::PrivateLogPrint(const struct LogHeader *lh, const char *txt)
{
  if(logcfg.logFile) {
    logfileMutex.Lock();
    if(logfileReopen) {
      logfileReopen=false; logfileShutup=false;
      if(logfile) {
        PRINTF(L_GEN_DEBUG,"logfile closed, reopen as '%s'",logcfg.logFilename);
        fclose(logfile);
        logfile=0;
        }
      }
    if(!logfile && !logfileShutup) {
      logfile=fopen(logcfg.logFilename,"a");
      if(logfile) {
        setlinebuf(logfile);
        logfileSize=ftell(logfile);
        if(logfileSize<0) {
          logfileSize=0;
          PRINTF(L_GEN_ERROR,"cannot determine size of logfile '%s', assuming zero",logcfg.logFilename);
          }
        PRINTF(L_GEN_DEBUG,"logfile '%s' opened",logcfg.logFilename);
        }
      else {
        logfileShutup=true;
        PRINTF(L_GEN_ERROR,"failed to open logfile '%s': %s",logcfg.logFilename,strerror(errno));
        }
      }
    if(logfile) {
      int q=fprintf(logfile,"%s [%s] %s\n",lh->stamp,lh->tag,txt);
      if(q>0) logfileSize+=q;

      if(logcfg.maxFilesize>0 && logfileSize>((long long)logcfg.maxFilesize*1024)) {
        fprintf(logfile,"%s [%s] %s\n",lh->stamp,lh->tag,"logfile closed, filesize limit reached");
        fclose(logfile);
        logfile=0; logfileShutup=false;
        char *name;
        asprintf(&name,"%s.old",logcfg.logFilename);
        if(rename(logcfg.logFilename,name)) {
          logfileShutup=true;
          PRINTF(L_GEN_ERROR,"failed to rotate logfile: %s",strerror(errno));
          PRINTF(L_GEN_ERROR,"logging to file disabled!");
          }
        free(name);
        }
      }
    logfileMutex.Unlock();
    }

  if(logcfg.logCon)
  {
//    printf("%s [%s] %s\n",lh->stamp,lh->tag,txt);
    void ControllerLog(const char *format, ...);
    ControllerLog("%s [%s] %s\n",lh->stamp,lh->tag,txt);
  }
  if(logcfg.logSys || LMOD(lh->c)==L_GEN) {
    int pri=-1;
    switch(lh->c) {
      case L_GEN_ERROR:
      case L_GEN_WARN:  if(SysLogLevel>0) pri=LOG_ERR; break;
      case L_GEN_INFO:  if(SysLogLevel>1) pri=LOG_INFO; break;
      case L_GEN_DEBUG: if(SysLogLevel>2) pri=LOG_DEBUG; break;
      case L_GEN_MISC:  if(logcfg.logSys) pri=LOG_DEBUG; break;
      default:          pri=LOG_DEBUG; break;
      }
    if(pri>=0)
      syslog(pri,"[%d] [%s] %s",cThread::ThreadId(),lh->tag,txt);
    }
}

bool cLogging::Enabled(int c)
{
  int m=LMOD(c);
  int o=LOPT(c)|LMOD_CFG_VALID|LMOD_ENABLE;
  return m<LMOD_SUP && (config[m] & o)==o;
}

void cLogging::Printf(int c, const char *format, ...)
{
  if(Enabled(c)) {
    struct LogHeader lh;
    if(GetHeader(c,&lh)) {
      char buff[1024];
      va_list ap;
      va_start(ap,format);
      vsnprintf(buff,sizeof(buff),format,ap);
      va_end(ap);
      LogLine(&lh,buff);
      }
    }
}

void cLogging::Puts(int c, const char *txt)
{
  if(txt && Enabled(c)) {
    struct LogHeader lh;
    if(GetHeader(c,&lh)) {
      LogLine(&lh,txt);
      }
    }
}

void cLogging::PutLB(int c, cLineBuff *lb)
{
  if(lb->Length()>0) {
    Puts(c,lb->Line());
    lb->Flush();
    }
}

void cLogging::Dump(int c, const void *data, int n, const char *format, ...)
{
  if(Enabled(c)) {
    struct LogHeader lh;
    if(GetHeader(c,&lh)) {
      char buff[1024];
      va_list ap;
      va_start(ap,format);
      vsnprintf(buff,sizeof(buff),format,ap);
      va_end(ap);
      LogLine(&lh,buff);
      const unsigned char *d=(const unsigned char *)data;
      for(int i=0; i<n;) {
        int q=sprintf(buff,"%04x:",i);
        for(int l=0 ; l<16 && i<n ; l++) q+=sprintf(&buff[q]," %02x",d[i++]);
        LogLine(&lh,buff);
        }
      }
    }
}

void cLogging::LineDump(int c, const void *data, int n, const char *format, ...)
{
  if(Enabled(c)) {
    struct LogHeader lh;
    if(GetHeader(c,&lh)) {
      char buff[4096];
      va_list ap;
      va_start(ap,format);
      unsigned int q=vsnprintf(buff,sizeof(buff),format,ap);
      va_end(ap);
      const unsigned char *d=(const unsigned char *)data;
      for(int i=0; i<n; i++) {
        if(q>=sizeof(buff)-8) {
          q+=snprintf(&buff[q],sizeof(buff)-q,"....");
          break;
          }
        q+=snprintf(&buff[q],sizeof(buff)-q," %02x",d[i]);
        }
      LogLine(&lh,buff);
      }
    }
}

void cLogging::SetModuleDefault(int c)
{
  const struct LogModule *lm=GetModule(c);
  if(lm) {
    int m=LMOD(c);
    config[m]=(lm->OptDefault&LOPT_MASK)|LMOD_CFG_VALID;
    cmask[m] = lm->OptSupported&LOPT_MASK;
    }
}

void cLogging::SetModuleOptions(int c)
{
  const struct LogModule *lm=GetModule(c);
  if(lm) config[LMOD(c)]=(LOPT(c)&(lm->OptSupported&LOPT_MASK))|LMOD_CFG_VALID;
}

int cLogging::GetModuleOptions(int c)
{
  const struct LogModule *lm=GetModule(c);
  return lm ? LOPT(config[LMOD(c)]) : -1;
}

void cLogging::SetModuleOption(int c, bool on)
{
  const struct LogModule *lm=GetModule(c);
  if(lm) {
    int m=LMOD(c);
    int o=LOPT(c)&(lm->OptSupported&LOPT_MASK);
    if(o) {
      if(on) config[m]|=o; else config[m]&=(~o);
      config[m]|=LMOD_CFG_VALID;
      }
    }
}

void cLogging::UpgradeOptions(int m)
{
  const struct LogModule *lm=mods[m];
  if(lm) {
    config[m]&=(lm->OptSupported&LOPT_MASK);
    config[m]|=(lm->OptDefault&LOPT_MASK)&(~cmask[m]);
    cmask[m]  =lm->OptSupported&LOPT_MASK;
    config[m]|=LMOD_CFG_VALID;
    }
}

const char *cLogging::GetModuleName(int c)
{
  const struct LogModule *lm=GetModule(c);
  return lm ? lm->Name : 0;
}

const char *cLogging::GetOptionName(int c)
{
  const struct LogModule *lm=GetModule(c);
  if(lm) {
    int o=LOPT(c)&~LMOD_ENABLE;
    if(lm->OptSupported & o) {
      int i;
      for(i=0; i<LOPT_NUM; i++,o>>=1) if(o&1) break;
      return (i>=1 && i<LOPT_NUM && lm->OptName[i-1]) ? lm->OptName[i-1] : 0;
      }
    }
  return 0;
}

bool cLogging::GetConfig(cLineBuff *lb)
{
  lb->Flush();
  bool cont=false;
  for(int m=1; m<LMOD_SUP; m++)
    if(config[m]&LMOD_CFG_VALID) {
      lb->Printf("%s%d:%x:%x",cont?",":"",m,config[m]&~LMOD_CFG_VALID,cmask[m]);
      cont=true;
      }
  return cont;
}

void cLogging::ParseConfig(const char *txt)
{
  int n=0, l, m, o, d;
  while(sscanf(&txt[n],"%d:%x:%x%n",&m,&o,&d,&l)==3) {
    if(m>=1 && m<LMOD_SUP) {
      config[m]=(o&LOPT_MASK)|LMOD_CFG_VALID;
      cmask[m] =(d&LOPT_MASK);
      UpgradeOptions(m);
      }
    n+=l;
    if(txt[n]!=',') break;
    n++;
    }
}

void cLogging::ReopenLogfile(void)
{
  logfileMutex.Lock();
  logfileReopen=true;
  logfileMutex.Unlock();
}

int cLogging::GetClassByName(const char *name)
{
  const char *s=index(name,'.');
  if(s) {
    unsigned int l=s-name;
    for(int m=1; m<LMOD_SUP; m++) {
      const struct LogModule *lm=mods[m];
      if(lm && strlen(lm->Name)==l && !strncasecmp(name,lm->Name,l)) {
        s++;
        if(!strcasecmp(s,"enable"))
           return LCLASS(m,1);
        for(int o=1; o<LOPT_NUM; o++)
          if((lm->OptSupported&(1<<o)) && lm->OptName[o-1] && !strcasecmp(s,lm->OptName[o-1]))
            return LCLASS(m,1<<o);
        break;
        }
      }
    }
  return -1;  
}

// -- cUserMsg -----------------------------------------------------------------

cUserMsg::cUserMsg(const char *m)
{
  msg=strdup(m);
}

cUserMsg::~cUserMsg()
{
  free(msg);
}

// -- cUserMsgs ----------------------------------------------------------------

cUserMsgs ums;

cUserMsgs::cUserMsgs(void)
{
  mutex=new cMutex;
}

cUserMsgs::~cUserMsgs()
{
  delete mutex;
}

void cUserMsgs::Queue(const char *fmt, ...)
{
  if(logcfg.logUser) {
    char buff[1024];
    va_list ap;
    va_start(ap,fmt);
    vsnprintf(buff,sizeof(buff),fmt,ap);
    va_end(ap);
    mutex->Lock();
    Add(new cUserMsg(buff));
    mutex->Unlock();
    }
}

cUserMsg *cUserMsgs::GetQueuedMsg(void)
{
  mutex->Lock();
  cUserMsg *um=First();
  if(um) Del(um,false);
  mutex->Unlock();
  return um;
}

// -- cLogLineBuff -------------------------------------------------------------

cLogLineBuff::cLogLineBuff(int C)
:cLineBuff(256)
{
  c=C;
}

cLogLineBuff::~cLogLineBuff()
{
  Flush();
}

void cLogLineBuff::Flush(void)
{
  cLogging::PutLB(c,this);
}


#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <dlfcn.h>
#include <dirent.h>
#include <fnmatch.h>

#include "vdr/sc/data.h"
#include "vdr/sc/sc.h"
#include "vdr/sc/scsetup.h"
#include "vdr/sc/opts.h"
#include "vdr/sc/misc.h"
#include "vdr/sc/system.h"
//#include "vdr/sc/cam.h"
#include "vdr/sc/log-core.h"
//#include "version.h"


void InitAll(const char *cfgdir)
{
  logcfg.logCon=1;
  logcfg.noTimestamp=1;
  cSystems::ConfigParse("Cardclient.Immediate","0");

  filemaps.SetCfgDir(cfgdir);
  cStructLoaders::SetCfgDir(cfgdir);

  if(!Feature.KeyFile()) keys.Disable();
//  if(!Feature.SmartCard()) smartcards.Disable();
  cStructLoaders::Load(false);
  if(Feature.KeyFile() && keys.Count()<1)
    PRINTF(L_GEN_ERROR,"no keys loaded for softcam!");
  if(!cSystems::Init(cfgdir)) exit(2);

#ifdef DEFAULT_PORT
//  smartcards.AddPort(DEFAULT_PORT);
#endif
//  smartcards.LaunchWatcher();
}

void LogAll(void)
{
  for(int i=0; i<32; i++)
    cLogging::SetModuleOptions(LCLASS(i,0xFFFFFFFF));
}

void SDump(const unsigned char *buffer, int n)
{
  for(int l=0 ; l<n ; l++) printf("%02x ",buffer[l]);
  printf("\n");
}

int ReadRaw(const char *name, unsigned char *buff, int maxlen)
{
  FILE *f=fopen(name,"r");
  if(f) {
    char line[512];
    int len=0;
    while(len<maxlen && fgets(line,sizeof(line),f)) {
      if(strstr(line,"dump: n=")) continue;
      char *p=index(line,'#');
      if(p) *p=0;
      p=index(line,'[');
      if(p) {
        p=index(p,']');
        if(p) {
          strcpy(line,p+1);
          }
        }
      p=index(line,':');
      if(p) p++; else p=line;
      while(1) {
        char *np;
        int val=strtol(p,&np,16);
        if(p!=np) {
          buff[len++]=val;
          p=np;
          }
        else break;
        }
      }
    fclose(f);
    printf("using raw from %s:\n",name);
    HexDump(buff,len);
    return len;
    }
  printf("failed to open raw file %s: %s\n",name,strerror(errno));
  exit(1);
}

//
//

static const struct LogModule lm_core = {
  (LMOD_ENABLE|L_CORE_ALL)&LOPT_MASK,
  (LMOD_ENABLE|L_CORE_LOAD|L_CORE_ECM|L_CORE_PIDS|L_CORE_AU|L_CORE_AUSTATS|L_CORE_CAIDS|L_CORE_NET|L_CORE_CI|L_CORE_SC|L_CORE_HOOK)&LOPT_MASK,
  "core",
  { "load","action","ecm","ecmProc","pids","au","auStats","auExtra","auExtern",
    "caids","keys","dynamic","csa","ci","av7110","net","netData","msgcache",
    "serial","smartcard","hook","ciFull" }
  };
ADD_MODULE(L_CORE,lm_core)

//
//

cScSetup ScSetup;

cScSetup::cScSetup(void)
{
  AutoUpdate = 1;
  memset(ScCaps,0,sizeof(ScCaps));
  ScCaps[0] = 1;
  ScCaps[1] = 2;
  ConcurrentFF = 0;
  memset(CaIgnore,0,sizeof(CaIgnore));
  LocalPriority = 0;
  ForceTransfer = 1;
  PrestartAU = 0;
}

void cScSetup::Check(void) {}
bool cScSetup::Ignore(unsigned short caid) { return false; }

//
//

void cSoftCAM::SetLogStatus(int CardNum, const cEcmInfo *ecm, bool on) {}
void cSoftCAM::AddHook(int CardNum, cLogHook *hook) {}
bool cSoftCAM::TriggerHook(int CardNum, int id) { return true; }

//
//

// --- cOpt --------------------------------------------------------------------

cOpt::cOpt(const char *Name, const char *Title)
{
  name=Name; title=Title;
  fullname=0; persistant=true;
}

cOpt::~cOpt()
{
  free(fullname);
}

const char *cOpt::FullName(const char *PreStr)
{
  if(PreStr) {
    free(fullname);
    asprintf(&fullname,"%s.%s",PreStr,name);
    return fullname;
    }
  else return name;
}

// --- cOptInt -----------------------------------------------------------------

cOptInt::cOptInt(const char *Name, const char *Title, int *Storage, int Min, int Max)
:cOpt(Name,Title)
{
  storage=Storage; min=Min; max=Max;
}

void cOptInt::Parse(const char *Value)
{
  *storage=atoi(Value);
}

void cOptInt::Backup(void)
{
}

bool cOptInt::Set(void)
{
  if(value!=*storage) { *storage=value; return true; }
  return false;
}

void cOptInt::Store(const char *PreStr)
{
}

void cOptInt::Create(cOsdMenu *menu)
{
}

// --- cOptSel -----------------------------------------------------------------

cOptSel::cOptSel(const char *Name, const char *Title, int *Storage, int NumStr, const char * const *Strings)
:cOptInt(Name,Title,Storage,0,NumStr)
{
  strings=Strings;
  trStrings=0;
}

cOptSel::~cOptSel()
{
  free(trStrings);
}

void cOptSel::Create(cOsdMenu *menu)
{
}

// --- cOptBool -----------------------------------------------------------------

cOptBool::cOptBool(const char *Name, const char *Title, int *Storage)
:cOptInt(Name,Title,Storage,0,1)
{}

void cOptBool::Create(cOsdMenu *menu)
{
}

// --- cOptStr -----------------------------------------------------------------

cOptStr::cOptStr(const char *Name, const char *Title, char *Storage, int Size, const char *Allowed)
:cOpt(Name,Title)
{
  storage=Storage; size=Size; allowed=Allowed;
  value=MALLOC(char,size);
}

cOptStr::~cOptStr()
{
  free(value);
}

void cOptStr::Parse(const char *Value)
{
  strn0cpy(storage,Value,size);
}

void cOptStr::Backup(void)
{
}

bool cOptStr::Set(void)
{
  if(strcmp(value,storage)) { strn0cpy(storage,value,size); return true; }
  return false;
}

void cOptStr::Store(const char *PreStr)
{
}

void cOptStr::Create(cOsdMenu *menu)
{
}

// --- cOptCap -----------------------------------------------------------------

class cOptCap : public cOpt {
protected:
  int *storage, *value;
  int size, disp, len;
public:
  cOptCap(const char *Name, const char *Title, int *Storage, int Size, int Disp);
  virtual ~cOptCap();
  virtual void Parse(const char *Value);
  virtual void Backup(void);
  virtual bool Set(void);
  virtual void Store(const char *PreStr);
  virtual void Create(cOsdMenu *menu);
  };

cOptCap::cOptCap(const char *Name, const char *Title, int *Storage, int Size, int Disp)
:cOpt(Name,Title)
{
  storage=Storage; size=Size; disp=Disp; len=sizeof(int)*size;
  value=MALLOC(int,size);
}

cOptCap::~cOptCap()
{
  free(value);
}

void cOptCap::Parse(const char *Value)
{
  memset(storage,0,len);
  int i=0;
  while(1) {
    char *p;
    const int c=strtol(Value,&p,10);
    if(c<=0 || p==Value || i>=size) return;
    storage[i++]=c;
    Value=p;
    }
}

void cOptCap::Backup(void)
{
}

bool cOptCap::Set(void)
{
  if(memcmp(value,storage,len)) { memcpy(storage,value,len); return true; }
  return false;
}

void cOptCap::Store(const char *PreStr)
{
}

void cOptCap::Create(cOsdMenu *menu)
{
}

// --- cOpts -------------------------------------------------------------------

cOpts::cOpts(const char *PreStr, int NumOpts)
{
  preStr=PreStr;
  numOpts=NumOpts; numAdd=0;
  if((opts=MALLOC(cOpt *,numOpts))) memset(opts,0,sizeof(cOpt *)*numOpts);
}

cOpts::~cOpts()
{
  if(opts) {
    for(int i=0; i<numOpts; i++) delete opts[i];
    free(opts);
    }
}

void cOpts::Add(cOpt *opt)
{
  if(opts && numAdd<numOpts) opts[numAdd++]=opt;
}

bool cOpts::Parse(const char *Name, const char *Value)
{
  if(opts) {
    for(int i=0; i<numAdd; i++)
      if(opts[i] && opts[i]->Persistant() && !strcasecmp(Name,opts[i]->Name())) {
        opts[i]->Parse(Value);
        return true;
        }
    }
  return false;
}

bool cOpts::Store(bool AsIs)
{
  return false;
}

void cOpts::Backup(void)
{
}

void cOpts::Create(cOsdMenu *menu)
{
}

//
//
// VDR
//
//

#include "vdr/sources.h"

cString cSource::ToString(int Code)
{
  char buffer[16];
  char *q = buffer;
  switch (Code & st_Mask) {
    case stCable: *q++ = 'C'; break;
    case stSat:   *q++ = 'S';
                  {
                    int pos = Code & ~st_Mask;
                    q += snprintf(q, sizeof(buffer) - 2, "%u.%u", (pos & ~st_Neg) / 10, (pos & ~st_Neg) % 10); // can't simply use "%g" here since the silly 'locale' messes up the decimal point
                    *q++ = (Code & st_Neg) ? 'E' : 'W';
                  }
                  break;
    case stTerr:  *q++ = 'T'; break;
    default:      *q++ = Code + '0'; // backward compatibility
    }
  *q = 0;
  return buffer;
}

int cSource::FromString(const char *s)
{
  int type = stNone;
  switch (toupper(*s)) {
    case 'C': type = stCable; break;
    case 'S': type = stSat;   break;
    case 'T': type = stTerr;  break;
    case '0' ... '9': type = *s - '0'; break; // backward compatibility
    default: esyslog("ERROR: unknown source key '%c'", *s);
             return stNone;
    }
  int code = type;
  if (type == stSat) {
     int pos = 0;
     bool dot = false;
     bool neg = false;
     while (*++s) {
           switch (toupper(*s)) {
             case '0' ... '9': pos *= 10;
                               pos += *s - '0';
                               break;
             case '.':         dot = true;
                               break;
             case 'E':         neg = true; // fall through to 'W'
             case 'W':         if (!dot)
                                  pos *= 10;
                               break;
             default: esyslog("ERROR: unknown source character '%c'", *s);
                      return stNone;
             }
           }
     if (neg)
        pos |= st_Neg;
     code |= pos;
     }
  return code;
}

//
//

#include "vdr/channels.h"

int cChannel::Transponder(int Frequency, char Polarization)
{
  // some satellites have transponders at the same frequency, just with different polarization:
  switch (tolower(Polarization)) {
    case 'h': Frequency += 100000; break;
    case 'v': Frequency += 200000; break;
    case 'l': Frequency += 300000; break;
    case 'r': Frequency += 400000; break;
    }
  return Frequency;
}

/*
 * config.c: Configuration file handling
 *
 * See the main source file 'vdr.c' for copyright information and
 * how to reach the author.
 *
 * $Id: config.c 1.147 2007/01/26 13:32:19 kls Exp $
 */

#include "config.h"
#include <ctype.h>
#include <stdlib.h>
#include "device.h"
#include "i18n.h"
#include "interface.h"
#include "plugin.h"
#include "recording.h"

// IMPORTANT NOTE: in the 'sscanf()' calls there is a blank after the '%d'
// format characters in order to allow any number of blanks after a numeric
// value!

// --- cCommand --------------------------------------------------------------

char *cCommand::result = NULL;

cCommand::cCommand(void)
{
  title = command = NULL;
  confirm = false;
}

cCommand::~cCommand()
{
  free(title);
  free(command);
}

bool cCommand::Parse(const char *s)
{
  const char *p = strchr(s, ':');
  if (p) {
     int l = p - s;
     if (l > 0) {
        title = MALLOC(char, l + 1);
        stripspace(strn0cpy(title, s, l + 1));
        if (!isempty(title)) {
           int l = strlen(title);
           if (l > 1 && title[l - 1] == '?') {
              confirm = true;
              title[l - 1] = 0;
              }
           command = stripspace(strdup(skipspace(p + 1)));
           return !isempty(command);
           }
        }
     }
  return false;
}

const char *cCommand::Execute(const char *Parameters)
{
  free(result);
  result = NULL;
  char *cmdbuf = NULL;
  if (Parameters)
     asprintf(&cmdbuf, "%s %s", command, Parameters);
  const char *cmd = cmdbuf ? cmdbuf : command;
  dsyslog("executing command '%s'", cmd);
  cPipe p;
  if (p.Open(cmd, "r")) {
     int l = 0;
     int c;
     while ((c = fgetc(p)) != EOF) {
           if (l % 20 == 0)
              result = (char *)realloc(result, l + 21);
           result[l++] = c;
           }
     if (result)
        result[l] = 0;
     p.Close();
     }
  else
     esyslog("ERROR: can't open pipe for command '%s'", cmd);
  free(cmdbuf);
  return result;
}

// -- cSVDRPhost -------------------------------------------------------------

cSVDRPhost::cSVDRPhost(void)
{
  addr.s_addr = 0;
  mask = 0;
}

bool cSVDRPhost::Parse(const char *s)
{
  mask = 0xFFFFFFFF;
  const char *p = strchr(s, '/');
  if (p) {
     char *error = NULL;
     int m = strtoul(p + 1, &error, 10);
     if (error && *error && !isspace(*error) || m > 32)
        return false;
     *(char *)p = 0; // yes, we know it's 'const' - will be restored!
     if (m == 0)
        mask = 0;
     else {
        mask <<= (32 - m);
        mask = htonl(mask);
        }
     }
  int result = inet_aton(s, &addr);
  if (p)
     *(char *)p = '/'; // there it is again
  return result != 0 && (mask != 0 || addr.s_addr == 0);
}

bool cSVDRPhost::Accepts(in_addr_t Address)
{
  return (Address & mask) == addr.s_addr;
}

// -- cCommands --------------------------------------------------------------

cCommands Commands;
cCommands RecordingCommands;

// -- cSVDRPhosts ------------------------------------------------------------

cSVDRPhosts SVDRPhosts;

bool cSVDRPhosts::Acceptable(in_addr_t Address)
{
  cSVDRPhost *h = First();
  while (h) {
        if (h->Accepts(Address))
           return true;
        h = (cSVDRPhost *)h->Next();
        }
  return false;
}

// -- cSetupLine -------------------------------------------------------------

cSetupLine::cSetupLine(void)
{
  plugin = name = value = NULL;
}

cSetupLine::cSetupLine(const char *Name, const char *Value, const char *Plugin)
{
  name = strdup(Name);
  value = strdup(Value);
  plugin = Plugin ? strdup(Plugin) : NULL;
}

cSetupLine::~cSetupLine()
{
  free(plugin);
  free(name);
  free(value);
}

int cSetupLine::Compare(const cListObject &ListObject) const
{
  const cSetupLine *sl = (cSetupLine *)&ListObject;
  if (!plugin && !sl->plugin)
     return strcasecmp(name, sl->name);
  if (!plugin)
     return -1;
  if (!sl->plugin)
     return 1;
  int result = strcasecmp(plugin, sl->plugin);
  if (result == 0)
     result = strcasecmp(name, sl->name);
  return result;
}

bool cSetupLine::Parse(char *s)
{
  char *p = strchr(s, '=');
  if (p) {
     *p = 0;
     char *Name  = compactspace(s);
     char *Value = compactspace(p + 1);
     if (*Name) { // value may be an empty string
        p = strchr(Name, '.');
        if (p) {
           *p = 0;
           char *Plugin = compactspace(Name);
           Name = compactspace(p + 1);
           if (!(*Plugin && *Name))
              return false;
           plugin = strdup(Plugin);
           }
        name = strdup(Name);
        value = strdup(Value);
        return true;
        }
     }
  return false;
}

bool cSetupLine::Save(FILE *f)
{
  return fprintf(f, "%s%s%s = %s\n", plugin ? plugin : "", plugin ? "." : "", name, value) > 0;
}

// -- cSetup -----------------------------------------------------------------

cSetup Setup;

cSetup::cSetup(void)
{
  OSDLanguage = 0;
  strcpy(OSDSkin, "sttng");
  strcpy(OSDTheme, "default");
  PrimaryDVB = 1;
  ShowInfoOnChSwitch = 1;
  TimeoutRequChInfo = 1;
  MenuScrollPage = 1;
  MenuScrollWrap = 0;
  MenuButtonCloses = 0;
  MarkInstantRecord = 1;
  strcpy(NameInstantRecord, "TITLE EPISODE");
  InstantRecordTime = 180;
  LnbSLOF    = 11700;
  LnbFrequLo =  9750;
  LnbFrequHi = 10600;
  DiSEqC = 0;
  SetSystemTime = 0;
  TimeSource = 0;
  TimeTransponder = 0;
  MarginStart = 2;
  MarginStop = 10;
  AudioLanguages[0] = -1;
  EPGLanguages[0] = -1;
  EPGScanTimeout = 5;
  EPGBugfixLevel = 3;
  EPGLinger = 0;
  SVDRPTimeout = 300;
  ZapTimeout = 3;
  PrimaryLimit = 0;
  DefaultPriority = 50;
  DefaultLifetime = 99;
  PausePriority = 10;
  PauseLifetime = 1;
  UseSubtitle = 1;
  UseVps = 0;
  VpsMargin = 120;
  RecordingDirs = 1;
  VideoDisplayFormat = 1;
  VideoFormat = 0;
  UpdateChannels = 5;
  UseDolbyDigital = 1;
  ChannelInfoPos = 0;
  ChannelInfoTime = 5;
  OSDLeft = 54;
  OSDTop = 45;
  OSDWidth = 624;
  OSDHeight = 486;
  OSDMessageTime = 1;
  UseSmallFont = 1;
  MaxVideoFileSize = MAXVIDEOFILESIZE;
  SplitEditedFiles = 0;
  MinEventTimeout = 30;
  MinUserInactivity = 300;
  MultiSpeedMode = 0;
  ShowReplayMode = 0;
  ResumeID = 0;
  CurrentChannel = -1;
  CurrentVolume = MAXVOLUME;
  CurrentDolby = 0;
  InitialChannel = 0;
  InitialVolume = -1;
}

cSetup& cSetup::operator= (const cSetup &s)
{
  memcpy(&__BeginData__, &s.__BeginData__, (char *)&s.__EndData__ - (char *)&s.__BeginData__);
  return *this;
}

cSetupLine *cSetup::Get(const char *Name, const char *Plugin)
{
  for (cSetupLine *l = First(); l; l = Next(l)) {
      if ((l->Plugin() == NULL) == (Plugin == NULL)) {
         if ((!Plugin || strcasecmp(l->Plugin(), Plugin) == 0) && strcasecmp(l->Name(), Name) == 0)
            return l;
         }
      }
  return NULL;
}

void cSetup::Store(const char *Name, const char *Value, const char *Plugin, bool AllowMultiple)
{
  if (Name && *Name) {
     cSetupLine *l = Get(Name, Plugin);
     if (l && !AllowMultiple)
        Del(l);
     if (Value)
        Add(new cSetupLine(Name, Value, Plugin));
     }
}

void cSetup::Store(const char *Name, int Value, const char *Plugin)
{
  char *buffer = NULL;
  asprintf(&buffer, "%d", Value);
  Store(Name, buffer, Plugin);
  free(buffer);
}

bool cSetup::Load(const char *FileName)
{
  if (cConfig<cSetupLine>::Load(FileName, true)) {
     bool result = true;
     for (cSetupLine *l = First(); l; l = Next(l)) {
         bool error = false;
         if (l->Plugin()) {
            cPlugin *p = cPluginManager::GetPlugin(l->Plugin());
            if (p && !p->SetupParse(l->Name(), l->Value()))
               error = true;
            }
         else {
            if (!Parse(l->Name(), l->Value()))
               error = true;
            }
         if (error) {
            esyslog("ERROR: unknown config parameter: %s%s%s = %s", l->Plugin() ? l->Plugin() : "", l->Plugin() ? "." : "", l->Name(), l->Value());
            result = false;
            }
         }
     return result;
     }
  return false;
}

void cSetup::StoreLanguages(const char *Name, int *Values)
{
  char buffer[I18nNumLanguages * 4];
  char *q = buffer;
  for (int i = 0; i < I18nNumLanguages; i++) {
      if (Values[i] < 0)
         break;
      const char *s = I18nLanguageCode(Values[i]);
      if (s) {
         if (q > buffer)
            *q++ = ' ';
         strncpy(q, s, 3);
         q += 3;
         }
      }
  *q = 0;
  Store(Name, buffer);
}

bool cSetup::ParseLanguages(const char *Value, int *Values)
{
  int n = 0;
  while (Value && *Value && n < I18nNumLanguages) {
        char buffer[4];
        strn0cpy(buffer, Value, sizeof(buffer));
        int i = I18nLanguageIndex(buffer);
        if (i >= 0)
           Values[n++] = i;
        if ((Value = strchr(Value, ' ')) != NULL)
           Value++;
        }
  Values[n] = -1;
  return true;
}

bool cSetup::Parse(const char *Name, const char *Value)
{
  if      (!strcasecmp(Name, "OSDLanguage"))         OSDLanguage        = atoi(Value);
  else if (!strcasecmp(Name, "OSDSkin"))             strn0cpy(OSDSkin, Value, MaxSkinName);
  else if (!strcasecmp(Name, "OSDTheme"))            strn0cpy(OSDTheme, Value, MaxThemeName);
  else if (!strcasecmp(Name, "PrimaryDVB"))          PrimaryDVB         = atoi(Value);
  else if (!strcasecmp(Name, "ShowInfoOnChSwitch"))  ShowInfoOnChSwitch = atoi(Value);
  else if (!strcasecmp(Name, "TimeoutRequChInfo"))   TimeoutRequChInfo  = atoi(Value);
  else if (!strcasecmp(Name, "MenuScrollPage"))      MenuScrollPage     = atoi(Value);
  else if (!strcasecmp(Name, "MenuScrollWrap"))      MenuScrollWrap     = atoi(Value);
  else if (!strcasecmp(Name, "MenuButtonCloses"))    MenuButtonCloses   = atoi(Value);
  else if (!strcasecmp(Name, "MarkInstantRecord"))   MarkInstantRecord  = atoi(Value);
  else if (!strcasecmp(Name, "NameInstantRecord"))   strn0cpy(NameInstantRecord, Value, MaxFileName);
  else if (!strcasecmp(Name, "InstantRecordTime"))   InstantRecordTime  = atoi(Value);
  else if (!strcasecmp(Name, "LnbSLOF"))             LnbSLOF            = atoi(Value);
  else if (!strcasecmp(Name, "LnbFrequLo"))          LnbFrequLo         = atoi(Value);
  else if (!strcasecmp(Name, "LnbFrequHi"))          LnbFrequHi         = atoi(Value);
  else if (!strcasecmp(Name, "DiSEqC"))              DiSEqC             = atoi(Value);
  else if (!strcasecmp(Name, "SetSystemTime"))       SetSystemTime      = atoi(Value);
  else if (!strcasecmp(Name, "TimeSource"))          TimeSource         = cSource::FromString(Value);
  else if (!strcasecmp(Name, "TimeTransponder"))     TimeTransponder    = atoi(Value);
  else if (!strcasecmp(Name, "MarginStart"))         MarginStart        = atoi(Value);
  else if (!strcasecmp(Name, "MarginStop"))          MarginStop         = atoi(Value);
  else if (!strcasecmp(Name, "AudioLanguages"))      return ParseLanguages(Value, AudioLanguages);
  else if (!strcasecmp(Name, "EPGLanguages"))        return ParseLanguages(Value, EPGLanguages);
  else if (!strcasecmp(Name, "EPGScanTimeout"))      EPGScanTimeout     = atoi(Value);
  else if (!strcasecmp(Name, "EPGBugfixLevel"))      EPGBugfixLevel     = atoi(Value);
  else if (!strcasecmp(Name, "EPGLinger"))           EPGLinger          = atoi(Value);
  else if (!strcasecmp(Name, "SVDRPTimeout"))        SVDRPTimeout       = atoi(Value);
  else if (!strcasecmp(Name, "ZapTimeout"))          ZapTimeout         = atoi(Value);
  else if (!strcasecmp(Name, "PrimaryLimit"))        PrimaryLimit       = atoi(Value);
  else if (!strcasecmp(Name, "DefaultPriority"))     DefaultPriority    = atoi(Value);
  else if (!strcasecmp(Name, "DefaultLifetime"))     DefaultLifetime    = atoi(Value);
  else if (!strcasecmp(Name, "PausePriority"))       PausePriority      = atoi(Value);
  else if (!strcasecmp(Name, "PauseLifetime"))       PauseLifetime      = atoi(Value);
  else if (!strcasecmp(Name, "UseSubtitle"))         UseSubtitle        = atoi(Value);
  else if (!strcasecmp(Name, "UseVps"))              UseVps             = atoi(Value);
  else if (!strcasecmp(Name, "VpsMargin"))           VpsMargin          = atoi(Value);
  else if (!strcasecmp(Name, "RecordingDirs"))       RecordingDirs      = atoi(Value);
  else if (!strcasecmp(Name, "VideoDisplayFormat"))  VideoDisplayFormat = atoi(Value);
  else if (!strcasecmp(Name, "VideoFormat"))         VideoFormat        = atoi(Value);
  else if (!strcasecmp(Name, "UpdateChannels"))      UpdateChannels     = atoi(Value);
  else if (!strcasecmp(Name, "UseDolbyDigital"))     UseDolbyDigital    = atoi(Value);
  else if (!strcasecmp(Name, "ChannelInfoPos"))      ChannelInfoPos     = atoi(Value);
  else if (!strcasecmp(Name, "ChannelInfoTime"))     ChannelInfoTime    = atoi(Value);
  else if (!strcasecmp(Name, "OSDLeft"))             OSDLeft            = atoi(Value);
  else if (!strcasecmp(Name, "OSDTop"))              OSDTop             = atoi(Value);
  else if (!strcasecmp(Name, "OSDWidth"))          { OSDWidth           = atoi(Value); if (OSDWidth  < 100) OSDWidth  *= 12; OSDWidth &= ~0x07; } // OSD width must be a multiple of 8
  else if (!strcasecmp(Name, "OSDHeight"))         { OSDHeight          = atoi(Value); if (OSDHeight < 100) OSDHeight *= 27; }
  else if (!strcasecmp(Name, "OSDMessageTime"))      OSDMessageTime     = atoi(Value);
  else if (!strcasecmp(Name, "UseSmallFont"))        UseSmallFont       = atoi(Value);
  else if (!strcasecmp(Name, "MaxVideoFileSize"))    MaxVideoFileSize   = atoi(Value);
  else if (!strcasecmp(Name, "SplitEditedFiles"))    SplitEditedFiles   = atoi(Value);
  else if (!strcasecmp(Name, "MinEventTimeout"))     MinEventTimeout    = atoi(Value);
  else if (!strcasecmp(Name, "MinUserInactivity"))   MinUserInactivity  = atoi(Value);
  else if (!strcasecmp(Name, "MultiSpeedMode"))      MultiSpeedMode     = atoi(Value);
  else if (!strcasecmp(Name, "ShowReplayMode"))      ShowReplayMode     = atoi(Value);
  else if (!strcasecmp(Name, "ResumeID"))            ResumeID           = atoi(Value);
  else if (!strcasecmp(Name, "CurrentChannel"))      CurrentChannel     = atoi(Value);
  else if (!strcasecmp(Name, "CurrentVolume"))       CurrentVolume      = atoi(Value);
  else if (!strcasecmp(Name, "CurrentDolby"))        CurrentDolby       = atoi(Value);
  else if (!strcasecmp(Name, "InitialChannel"))      InitialChannel     = atoi(Value);
  else if (!strcasecmp(Name, "InitialVolume"))       InitialVolume      = atoi(Value);
  else
     return false;
  return true;
}

bool cSetup::Save(void)
{
  Store("OSDLanguage",        OSDLanguage);
  Store("OSDSkin",            OSDSkin);
  Store("OSDTheme",           OSDTheme);
  Store("PrimaryDVB",         PrimaryDVB);
  Store("ShowInfoOnChSwitch", ShowInfoOnChSwitch);
  Store("TimeoutRequChInfo",  TimeoutRequChInfo);
  Store("MenuScrollPage",     MenuScrollPage);
  Store("MenuScrollWrap",     MenuScrollWrap);
  Store("MenuButtonCloses",   MenuButtonCloses);
  Store("MarkInstantRecord",  MarkInstantRecord);
  Store("NameInstantRecord",  NameInstantRecord);
  Store("InstantRecordTime",  InstantRecordTime);
  Store("LnbSLOF",            LnbSLOF);
  Store("LnbFrequLo",         LnbFrequLo);
  Store("LnbFrequHi",         LnbFrequHi);
  Store("DiSEqC",             DiSEqC);
  Store("SetSystemTime",      SetSystemTime);
  Store("TimeSource",         cSource::ToString(TimeSource));
  Store("TimeTransponder",    TimeTransponder);
  Store("MarginStart",        MarginStart);
  Store("MarginStop",         MarginStop);
  StoreLanguages("AudioLanguages", AudioLanguages);
  StoreLanguages("EPGLanguages", EPGLanguages);
  Store("EPGScanTimeout",     EPGScanTimeout);
  Store("EPGBugfixLevel",     EPGBugfixLevel);
  Store("EPGLinger",          EPGLinger);
  Store("SVDRPTimeout",       SVDRPTimeout);
  Store("ZapTimeout",         ZapTimeout);
  Store("PrimaryLimit",       PrimaryLimit);
  Store("DefaultPriority",    DefaultPriority);
  Store("DefaultLifetime",    DefaultLifetime);
  Store("PausePriority",      PausePriority);
  Store("PauseLifetime",      PauseLifetime);
  Store("UseSubtitle",        UseSubtitle);
  Store("UseVps",             UseVps);
  Store("VpsMargin",          VpsMargin);
  Store("RecordingDirs",      RecordingDirs);
  Store("VideoDisplayFormat", VideoDisplayFormat);
  Store("VideoFormat",        VideoFormat);
  Store("UpdateChannels",     UpdateChannels);
  Store("UseDolbyDigital",    UseDolbyDigital);
  Store("ChannelInfoPos",     ChannelInfoPos);
  Store("ChannelInfoTime",    ChannelInfoTime);
  Store("OSDLeft",            OSDLeft);
  Store("OSDTop",             OSDTop);
  Store("OSDWidth",           OSDWidth);
  Store("OSDHeight",          OSDHeight);
  Store("OSDMessageTime",     OSDMessageTime);
  Store("UseSmallFont",       UseSmallFont);
  Store("MaxVideoFileSize",   MaxVideoFileSize);
  Store("SplitEditedFiles",   SplitEditedFiles);
  Store("MinEventTimeout",    MinEventTimeout);
  Store("MinUserInactivity",  MinUserInactivity);
  Store("MultiSpeedMode",     MultiSpeedMode);
  Store("ShowReplayMode",     ShowReplayMode);
  Store("ResumeID",           ResumeID);
  Store("CurrentChannel",     CurrentChannel);
  Store("CurrentVolume",      CurrentVolume);
  Store("CurrentDolby",       CurrentDolby);
  Store("InitialChannel",     InitialChannel);
  Store("InitialVolume",      InitialVolume);

  Sort();

  if (cConfig<cSetupLine>::Save()) {
     isyslog("saved setup to %s", FileName());
     return true;
     }
  return false;
}

extern BOOL showCwDw;
extern BOOL showRequests;
extern BOOL showEmmDebug;
extern BOOL doEcmEmmRecording;
extern BOOL isEmmEnable;

static inline BOOL getShowCwDw()
{
  return showCwDw;
}

static inline BOOL getShowRequests()
{
  return showRequests;
}

static inline void setShowCwDw(BOOL action)
{
  showCwDw = action;
}

static inline void setShowRequests(BOOL action)
{
  showRequests = action;
}

static inline void setEmmDebug(BOOL action)
{
  showEmmDebug = action;
}

static inline BOOL getEmmDebug()
{
  return showEmmDebug;
}

static inline BOOL getRawRecordState()
{
  return doEcmEmmRecording;
}

static void setRawRecording(BOOL action)
{
  doEcmEmmRecording = action;
}

static inline BOOL getEmmEnable()
{
  return isEmmEnable;
}

static inline void setEmmEnable(BOOL action)
{
  isEmmEnable = action;
}

void ControllerDump(NSData *buf);
void ControllerDump(unsigned char *buf, int len);
void ControllerLog(const char *format, ...);

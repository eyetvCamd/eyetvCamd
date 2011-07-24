/* Controller */

#import <Cocoa/Cocoa.h>
#import "pmt.h"
#import "emm.h"
#import "SrvController.h"
#import "sectionFilter.h"

#define NUM_DEVS 12
typedef struct
{
  section *sPMT;
  section *sECM;
//  section *sEMM;
  section *sCAT;
  unsigned long curPmtPid;
  unsigned long curServiceId;
  unsigned long curTransponderId;
  unsigned long curEcmPid;
  int selected;
  pmt * pmtSet;
  cat * catSet;
  caDescriptor *curCa;
  unsigned char lastDW[16];
//  emmDataState emmBufferState;
//  NSMutableData *emmBuffer;
  NSMutableDictionary *emmSectionFilter;
  NSMutableDictionary *emmBuffer;
  NSMutableDictionary *emmState;
} devCtrl;

@interface Controller : NSObject
{
  IBOutlet NSTextView *gdbView;
  IBOutlet NSWindow *myWindow;
  IBOutlet NSTableView *caDescList;
  IBOutlet SrvController *srvListCtl;
  IBOutlet NSPopUpButton *currentDevice;
  IBOutlet IrdController *irdCtl;
  IBOutlet NSButton *edEmmProcessing;
  
  @private
    devCtrl devs[NUM_DEVS];
    int selectedDevice;
    bool doCaChange;
    bool pmtChanged;
    NSMutableSet *caCache;
    NSString *docPath;
    NSString *cacacheFile;
    NSString *configFile;
    NSOutputStream *mpgFile;
    NSMutableSet *emmReaders;
    NSMutableDictionary *recordPids;
}

- (IBAction)selectedDevs:(id)sender;
- (IBAction)saveCacacheAction:(id)sender;
- (IBAction)cwdwAction:(id)sender;
- (IBAction)requestsAction:(id)sender;
- (IBAction)clearLogAction:(id)sender;
- (IBAction)emmAndEcmRawRecord:(id)sender;
- (IBAction)enableEmm:(id)sender;

- (id)init;
- (void)dealloc;
- (void)caCacheSave;
- (void)saveCfgParams;
- (void)applicationDidFinishLaunching:(NSNotification *)notification;
- (void)handlePortMessage:(NSPortMessage *)portMessage;
- (void)rcvNotifications:(NSNotification *)obj;
- (void)rcvPidsAndControls:(NSNotification *)obj;
- (void)sendData:(NSData *)pdata dev:(int)devIndex;
- (void)textToLog:(NSString *)logtext;
- (void)awakeFromNib;
- (void)tableViewSelectionDidChange:(NSNotification *)notification;
- (void)writeDwToDescrambler:(unsigned char *)dw caDesc:(caDescriptor *)ca sid:(unsigned long)sid;
- (void)emmAddParams:(unsigned char *)serial provData:(unsigned char *)bytes caid:(unsigned int)casys ident:(unsigned int)provid;
- (void)emmRmParams:(unsigned char *)serial provData:(unsigned char *)bytes caid:(unsigned int)casys ident:(unsigned int)provid;
- (void)irdetoChannelChange:(unsigned int)irdchn;
- (void)srvListLoadConfig;
- (void)decodePluginMessage:(NSArray *)msgData;
- (unsigned long)getNagraIdent:(NSData *)pEcm caDesc:(caDescriptor *)ca;
- (unsigned long)getCworksIdent:(NSData *)pEcm caDesc:(caDescriptor *)ca;
- (void)clearAllEmm:(int)devno;
- (void)addEmmPid:(unsigned int)pid toDevice:(int)devno;
- (void)stopAllEmmPids;
- (void)startAllEmmPids;

@end

static inline void pluginLog(const char *format, ...)
{
  va_list ap;
  va_start(ap, format);
  NSString *str = [[NSString alloc] initWithFormat:[NSString stringWithCString:format encoding:NSASCIIStringEncoding] arguments:ap];
  [[NSDistributedNotificationCenter defaultCenter] postNotificationName:@"cwdwgwLogObserver" object:str userInfo:nil deliverImmediately:true];
  va_end(ap);
  [str release];
}

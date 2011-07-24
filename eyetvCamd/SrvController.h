/* SrvController */

#import <Cocoa/Cocoa.h>
#import "pmt.h"
#import "IrdController.h"
#import "emm.h"

@interface SrvController : NSObject
{
    IBOutlet NSTableView *filerListView;
    IBOutlet NSTableView *srvListView;
    IBOutlet IrdController *irdCtl;
    IBOutlet NSButton *emuBtn;
    NSMutableArray *dSource;
    NSMutableArray *csList;
    id delegateObj;
    NSString *docPath;
    NSString *serversFile;
    NSString *cacacheFile;
    unsigned char ecmParity[16];
    BOOL enableEmu;
}

- (IBAction)insertFilterForServer:(id)sender;
- (IBAction)insertServer:(id)sender;
- (IBAction)removeFilterForServer:(id)sender;
- (IBAction)removeServer:(id)sender;
- (IBAction)saveServerList:(id)sender;
- (IBAction)enableOrDisableEmu:(id)sender;
- (bool)hasCasys:(unsigned int)Casys Ident:(unsigned int)Ident;
- (void)sendEcmPacket:(NSData *)Packet Cadesc:(caDescriptor *)desc Ssid:(unsigned int)ssid devIndex:(unsigned int)index;
- (void)sendEmmPacket:(NSData *)Packet Params:(emmParams *)params;
- (void)writeDwToDescrambler:(unsigned char *)dw caDesc:(caDescriptor *)ca sid:(unsigned long)sid;
- (void)emmAddParams:(unsigned char *)serial provData:(unsigned char *)bytes caid:(unsigned int)casys ident:(unsigned int)provid;
- (void)emmRmParams:(unsigned char *)serial provData:(unsigned char *)bytes caid:(unsigned int)casys ident:(unsigned int)provid;
- (void)setDelegateForController:(id)obj;
- (void)initEmu;
- (void)loadServerConfigAndCache;
- (void)enableEmu:(bool)action;
- (bool)getEnableEmu;
- (id)init;
- (void)dealloc;
@end

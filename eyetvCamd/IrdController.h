/* IrdController */

#import <Cocoa/Cocoa.h>

enum
{
  MAX_IRD_CHANNELS = 10
};

typedef struct
{
  NSData *irdEcm[MAX_IRD_CHANNELS];
  unsigned int chids[MAX_IRD_CHANNELS];
  BOOL startSync;
  BOOL validFlag;
  unsigned int curSelectedIndex;
  unsigned int curMaxChn;
  unsigned int curSetChn;
} irdetoDevice;

@interface IrdController : NSObject
{
  IBOutlet NSTableView *chanView;

@private
  irdetoDevice devs[12];
  unsigned int selectedDevice;
  id delegated;
}

- (void) clearValid:(unsigned int)index;
- (void) setSelectedDevice:(unsigned int)index;
- (void) clear:(unsigned int)devIndex;
- (NSData *) getEcm:(NSData *)Packet dev:(unsigned int)devIndex;
- (void) setDelegateObj:(id)anobj;
- (void)setIrdetoChannel:(unsigned int)chn forDev:(unsigned int)index;
- (id) init;
- (void) dealloc;

@end

@interface NSObject (IrdControllerDelegate) 
- (void)irdetoChannelChange:(unsigned int)chn;
@end
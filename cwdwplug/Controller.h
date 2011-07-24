/* Controller */

#import <Cocoa/Cocoa.h>
#include <Foundation/NSLock.h>

#define MAX_DEVICES 16

//#define SHOW_SPEED_STATISTICS

/*
@interface NSCondition : NSObject <NSLocking> {                                                   
@private                                                                                          
  void *_priv;                                                                                  
}                                                                                                 

- (void)wait;                                                                                     
- (BOOL)waitUntilDate:(NSDate *)limit;                                                            
- (void)signal;                                                                                   
- (void)broadcast;                                                                                

- (void)setName:(NSString *)n AVAILABLE_MAC_OS_X_VERSION_10_5_AND_LATER;                          
- (NSString *)name AVAILABLE_MAC_OS_X_VERSION_10_5_AND_LATER;                                     

@end 
 */

@interface Controller : NSObject
{
	@private
	NSMutableIndexSet *pidList;
	void *keys;
	bool doDescramble;
	NSMutableArray *tsPackets;
	NSMutableArray *cleanPackets;
	NSLock *dataLock;
	NSLock *descramblerLock;
	NSTimeInterval lastDwTime;
	NSMutableIndexSet *decryptPids;
	int deviceIndex;
	NSDate *lastCheck;
	unsigned int tspc;

	int packetsBlockCount;
	unsigned char even_dw[8];
	unsigned char odd_dw[8];
	
#ifdef SHOW_SPEED_STATISTICS
	CFAbsoluteTime		speedStatisticsLastOutput;
	CFTimeInterval		speedStatisticsDuration;
	long				speedStatisticsPackets;
	long				speedStatisticsClusters;
#endif

}

+ (void)LaunchThread:(id)obj;
- (void)descramblerEvent:(NSTimer *)timerObj;
- (void)pidFilterEvent:(NSTimer *)timerObj;
- (id)init;
- (void)dealloc;
- (void)logMessage:( NSString *)msg;
- (void)sendData:(NSArray *)parray;
- (void)clearPidList;
- (void)clearDescramblerPidList;
- (bool)isPidInList:(unsigned int)pid;
- (bool)isPidInDescramblerList:(unsigned int)pid;
- (void)addPid:(unsigned int)pid;
- (void)addPidToDescrambler:(unsigned int)pid;
- (void)removePid:(unsigned int)pid;
- (bool)descramble;
- (void)setDescramble:(bool)action;
- (void *)getKeys;
- (void)appendTsPacket:(unsigned char *)pbytes length:(int)len;
- (void)getCleanTsPacket:(unsigned char *)buffer;
- (void)channelChange;
- (void)setDevIndex:(int)index;
- (int)getDevIndex;
- (void)rcvPidsAndControls:(NSNotification *)obj;
- (void)decodeCwdwgwMessage:(NSData *)pmsg;
@end

static inline void pluginLog(const char *format, ...)
{
  va_list ap;
  va_start(ap, format);
  NSString *str = [[NSString alloc] initWithFormat:[NSString stringWithCString:format] arguments:ap];
  [[NSDistributedNotificationCenter defaultCenter] postNotificationName:@"cwdwgwLogObserver" object:str];
  va_end(ap);
  [str release];
}


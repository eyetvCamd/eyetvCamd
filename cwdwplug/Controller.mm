#import "Controller.h"
#include "messages.h"
#include "FFdecsa/FFdecsa.h"
#include "EyeTVPluginDefs.h"
#include <stdlib.h>
typedef struct
{                                                                                                                                          
#if defined(__BIG_ENDIAN__)                                                                                                                
  unsigned int  syncByte:8,                                                                                                                 
	    transportErr:1,                                                                                                             
	    payloadStart:1,                                                                                                             
	    priority:1,                                                                                                                 
	    pid:13,                                                                                                                     
	    scrambling:2,                                                                                                               
	    adaption:2,                                                                                                                 
	    continuity:4;                                                                                                               
#endif                                                                                                                                     
#if defined(__LITTLE_ENDIAN__)                                                                                                             
  unsigned int syncByte:8,
		  transportErr:1,                                                                                                             
		  payloadStart:1,                                                                                                             
		  priority:1,                                                                                                                 
		  pid:13,                                                                                                                     
		  scrambling:2,                                                                                                               
		  adaption:2,                                                                                                                 
          continuity:4;                                                                                                                 
#endif                                                                                                                                     
} TSFields;                                                                                                                                

typedef union                                                                                                                              
{                                                                                                                                          
  TSFields ts;                                                                                                                             
  unsigned int word;                                                                                                                       
} TSHeader;
static bool doExit = NO;
extern NSCondition *pidFilter[MAX_DEVICES];

@implementation Controller

- (id) init 
{
	self = [super init];
	if (self != nil)
	{
		lastDwTime = [[NSDate date] timeIntervalSince1970];
		tsPackets = [[NSMutableArray alloc] init];
		cleanPackets = [[NSMutableArray alloc] init];
		dataLock = [[NSLock alloc] init];
		descramblerLock = [[NSLock alloc] init];
		decryptPids = [[NSMutableIndexSet alloc] init];
		doDescramble = NO;
		keys = get_key_struct();
		pidList = [[NSMutableIndexSet alloc] init];
		lastCheck = [[NSDate alloc] init];
		tspc = 0;
		packetsBlockCount = get_internal_parallelism();
		
		memset(even_dw, 0, sizeof(even_dw));
		memset(odd_dw, 0, sizeof(odd_dw));
		
#ifdef SHOW_SPEED_STATISTICS
		speedStatisticsLastOutput = 0.;
		speedStatisticsDuration = 0.;
		speedStatisticsPackets = 0;
		speedStatisticsClusters = 0;
#endif
	}
	return self;
}

- (void) dealloc 
{
  [dataLock release];
  [descramblerLock release];
  [cleanPackets release];
  [tsPackets release];
  [pidList release];
  [decryptPids release];
  free_key_struct(keys);
  [super dealloc];
}

- (bool)descramble
{
  NSTimeInterval curInterval = [[NSDate date] timeIntervalSince1970];
  if( (curInterval - lastDwTime) >= 60 )
  {
    doDescramble = NO;
  }
  return doDescramble;
}

- (void)setDescramble:(bool)action
{
  doDescramble = action;
}

- (void *)getKeys
{
  return keys;
}

- (void)decodeCwdwgwMessage:(NSData *)pmsg
{
  if( pmsg != nil )
  {
    unsigned long mType = ntohl(*(unsigned long *)[pmsg bytes]);
    switch(mType)
    {
      case msg_add_pid:
      case msg_remove_pid:
      {
	unsigned int rcvPid = ntohl(((msgPid *)[pmsg bytes])->mPid);
	if( mType == msg_add_pid )
	{
	  [self addPid:rcvPid];
	}
	else
	{
	  [self removePid:rcvPid];
	}
      } break;
      case msg_dw:
      {
	static unsigned char zero_dw[8] = {0,0,0,0,0,0,0,0};
	
	newDw *dw = (newDw *)[pmsg bytes];
	for(int i = 0; i < 16; i += 4)
	{
	  dw->dw[i+3] = dw->dw[i] + dw->dw[i + 1] + dw->dw[i + 2];
	}
	if( memcmp(&dw->dw[0], even_dw, 8 ) != 0 && memcmp(&dw->dw[0], zero_dw, 8) != 0 )
	{
	  memcpy(even_dw, &dw->dw[0], 8);
	  set_even_control_word(keys, &dw->dw[0]);
	}
	if( memcmp(&dw->dw[8], odd_dw, 8 ) != 0 && memcmp(&dw->dw[8], zero_dw, 8) != 0 )
	{
	  memcpy(odd_dw, &dw->dw[8], 8);
	  set_odd_control_word(keys, &dw->dw[8]);
	  
	}
	doDescramble = YES;
	lastDwTime = [[NSDate date] timeIntervalSince1970];
      } break;
	
      case msg_ca_change:
	doDescramble = NO;
	break;
	
      case msg_camd_online:
      {
	extern VLCEyeTVPluginGlobals_t *lGlobals;
	if( lGlobals != 0 )
	{
	  DeviceInfo *deviceInfo = &lGlobals->devices[deviceIndex];
	  if( deviceInfo != 0 )
	  {
	    unsigned long pmtpid = 0;
	    [self logMessage:@"ServiceChanged:\n"];
	    for(int j = 0; j < deviceInfo->activePIDsCount; j++ )
	    {
	      [self logMessage:[NSString stringWithFormat:@"Active PID: %lx, type: %lx\n",
				deviceInfo->activePIDs[j].pid, deviceInfo->activePIDs[j].pidType]];
	      if( deviceInfo->activePIDs[j].pidType == kEyeTVPIDType_PMT )
	      {
		pmtpid = deviceInfo->activePIDs[j].pid;
	      }
	    }
	    
	    msgNewChannel message;
	    fillNewChannelMessage(&message, deviceInfo->transponderID, deviceInfo->serviceID, pmtpid);
	    NSArray *ncMsg = [NSArray arrayWithObjects:[NSData dataWithBytes:&message length:sizeof(message)],
			      [NSNumber numberWithInt:deviceIndex], nil]; 
	    [self sendData:[NSArray arrayWithObjects:ncMsg,nil]];
	  }
	}
      }	break;
	
      default:
      {
	[self logMessage:[NSString stringWithFormat:@"unknown message type %d",mType]];
      }
    }
  }
}

- (void)handlePortMessage:(NSPortMessage *)portMessage
{
  NSData *pmsg = [[portMessage components] objectAtIndex:0];
  [self decodeCwdwgwMessage:pmsg];
}

- (void)logMessage:(NSString *)msg
{
  [[NSDistributedNotificationCenter defaultCenter] postNotificationName:@"cwdwgwLogObserver" object:msg userInfo:nil deliverImmediately:true];
}

- (void)sendData:(NSArray *)parray
{
/*
  id remote = [[NSMessagePortNameServer sharedInstance] portForName:@"cwdwgwECMData"];
  if( remote != nil )
  {
    NSPortMessage *msg = [[NSPortMessage alloc] initWithSendPort:remote receivePort:nil components:parray];
    [msg sendBeforeDate:[NSDate date]];
    [msg release];
  } */
  NSMutableData *dmsg = [[NSMutableData alloc] init];
  NSKeyedArchiver *arch = [[NSKeyedArchiver alloc] initForWritingWithMutableData:dmsg];
  [arch setOutputFormat:NSPropertyListXMLFormat_v1_0];
  [arch encodeObject:parray forKey:@"DArray"];
  [arch finishEncoding];
  NSString *msg = [[NSString alloc] initWithData:dmsg encoding:NSASCIIStringEncoding];
  [[NSDistributedNotificationCenter defaultCenter] postNotificationName:@"cwdwgwPIDandControlObserver" object:msg userInfo:nil deliverImmediately:true];
  [arch release];
  [msg release];
  [dmsg release];
}

- (void)clearPidList
{
  [dataLock lock];
  [pidList removeAllIndexes];
  [dataLock unlock];
}

- (bool)isPidInList:(unsigned int)pid
{
  [dataLock lock];
  bool result =  [pidList containsIndex:pid];
  [dataLock unlock];
  return result;
}

- (bool)isPidInDescramblerList:(unsigned int)pid
{
  [dataLock lock];
  bool result =  [decryptPids containsIndex:pid];
  [dataLock unlock];
  return result;
}

- (void)addPid:(unsigned int)pid
{
  [dataLock lock];
  if( [pidList containsIndex:pid] != YES )
  {
    [pidList addIndex:pid];
  }
  [dataLock unlock];
}

- (void)addPidToDescrambler:(unsigned int)pid
{
  [dataLock lock];
  [decryptPids addIndex:pid];
  [dataLock unlock];
}

- (void)removePid:(unsigned int)pid;
{
  [dataLock lock];
  [pidList removeIndex:pid];
  [dataLock unlock];
}

- (void)clearDescramblerPidList
{
  [dataLock lock];
  [decryptPids removeAllIndexes];
  [dataLock unlock];
}

- (void)setDevIndex:(int)index
{
  deviceIndex = index;
}

- (int)getDevIndex
{
  return deviceIndex;
}


+ (void)LaunchThread:(id)obj
{
  NSAutoreleasePool*  pool = [[NSAutoreleasePool alloc] init];
  
  // Let the run loop process things.
  // [obj messagePortInit:obj];
  NSString *observerName = [NSString stringWithFormat:@"PluginPIDFilter%d", [obj getDevIndex]];
  [[NSDistributedNotificationCenter defaultCenter] addObserver:obj
						      selector:@selector(rcvPidsAndControls:) 
							  name:observerName object:nil];
  [NSTimer scheduledTimerWithTimeInterval:10 target:obj selector:@selector(pidFilterEvent:) userInfo:nil repeats:YES];  
  do
  {
    [[NSRunLoop currentRunLoop] run];
  }
  while ( doExit != YES );
  [pool release];
}

/* Structure for TS-Packets */
typedef struct 
{
  unsigned char header[4];
  unsigned char	data[188-4];
} TransportStreamPacket;

- (void)pidFilterEvent:(NSTimer *)timerObj
{
}

- (int)blockDecrypt:(int)blockSize
{
  NSArray *packets = [tsPackets objectsAtIndexes:[NSIndexSet indexSetWithIndexesInRange:NSMakeRange(0,blockSize)]];
  int elements = ([packets count] * 2) + 1;
  unsigned char *dp[elements];
  for(int i = 0, j = 0;  i < [packets count]; i++, j += 2 )
  {
    dp[j] = (unsigned char *)[[packets objectAtIndex:i] bytes];
    dp[j + 1] = dp[j] + 188;
  }
  dp[elements - 1] = 0;
#ifdef SHOW_SPEED_STATISTICS
	CFAbsoluteTime		startTime = CFAbsoluteTimeGetCurrent();
#endif

  int descrambled = decrypt_packets(keys, dp);
#ifdef SHOW_SPEED_STATISTICS
	CFAbsoluteTime		endTime = CFAbsoluteTimeGetCurrent();
	speedStatisticsDuration += endTime - startTime;
	speedStatisticsPackets += descrambled;
	speedStatisticsClusters++;
	if ((speedStatisticsDuration > 0.) && ((endTime - speedStatisticsLastOutput) >= 1.0))
	{
		double		netBits = (double)speedStatisticsPackets * 188. * 8.;
		double		netSpeed = netBits / speedStatisticsDuration;
		double		efficiency = (double)speedStatisticsPackets * 100. / ((double)speedStatisticsClusters * (double)packetsBlockCount);
		
		[self logMessage:[NSString stringWithFormat:@"packetsBlockCount:%d, packets:%ld, clusters:%ld, duration:%.6f, efficiency:%6.2f%%, net speed:%.3f Mbps\n", packetsBlockCount, speedStatisticsPackets, speedStatisticsClusters, speedStatisticsDuration, efficiency, netSpeed / 1000000.]];
		
		speedStatisticsLastOutput = endTime;
		speedStatisticsDuration = 0.;
		speedStatisticsPackets = 0;
		speedStatisticsClusters = 0;
	}
#endif
  for(int i = 0; i < descrambled; i++)
  {
    [cleanPackets addObject:[packets objectAtIndex:i]];
  }
  [tsPackets removeObjectsInRange:NSMakeRange(0, descrambled)];
  return descrambled;
}

- (void)descramblerEvent:(NSTimer *)timerObj
{
//  NSAutoreleasePool*  pool = [[NSAutoreleasePool alloc] init];
//  [descrambler wait];
  [descramblerLock lock];
  if( [tsPackets count] >= packetsBlockCount )
  {
    int more = packetsBlockCount - [self blockDecrypt:packetsBlockCount];
    while( more != 0 )
    {
      more = more - [self blockDecrypt:more];
    }
  }
  [descramblerLock unlock];
//  [pool release];
}

- (void)appendTsPacket:(unsigned char *)pbytes length:(int)len
{
  bool doSignal = NO;
  NSData *pd = [[NSData alloc] initWithBytes:pbytes length:len];
  [descramblerLock lock];
  [tsPackets addObject:pd];
  if( [tsPackets count] >= packetsBlockCount )
    doSignal = YES;
  [descramblerLock unlock];
  if( doSignal == YES )
    [self descramblerEvent:nil];
  [pd release];
}

static inline int payload(const unsigned char *tsp)
{
  TSHeader hdr;
  hdr.word = ntohl(*(unsigned int *)tsp);
  int pOffset = 0, pLen = 184;
  
  if( hdr.ts.transportErr != 0 ) return -1;
  switch( hdr.ts.adaption )
  {                                                                                                                                        
    case 1:
      break;    // payload only
    case 2:
      return 0; // no payload
    case 3:
      pOffset = 1 + tsp[4];
      pLen -= pOffset;
      break;
    default:
      return -1;
  }
  if( hdr.ts.payloadStart != 0 )
  {
    int additional = tsp[4 + pOffset] + 1;
    pOffset += additional;
    pLen -= additional;
  }
  if( pOffset < 0 || pLen < 0 ) return -1;
  if( (pOffset + pLen) != 184) return -1;
  return pLen;
}

- (void)getCleanTsPacket:(unsigned char *)buffer
{
  [descramblerLock lock];
  NSData *obj = nil;
  if( [cleanPackets count] > 0 )
  {
    obj = [[cleanPackets objectAtIndex:0] retain];
    [cleanPackets removeObjectAtIndex:0];
  }
  [descramblerLock unlock];
  if( obj != nil )
  {
    [obj getBytes:buffer length:188];
    [obj release];
  }
}

- (void)channelChange
{
  [dataLock lock];
  [cleanPackets removeAllObjects];
  [dataLock unlock];
  [descramblerLock lock];
  [tsPackets removeAllObjects];
  [descramblerLock unlock];
}

- (void)rcvPidsAndControls:(NSNotification *)obj
{
  NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
  NSString *p = [obj object];
	NSData *dmsg = [[NSData alloc] initWithBytes:[p cStringUsingEncoding:NSASCIIStringEncoding] length:[p lengthOfBytesUsingEncoding:NSASCIIStringEncoding]];
  NSKeyedUnarchiver *amsg = [[NSKeyedUnarchiver alloc] initForReadingWithData:dmsg];
  NSData *rcvd = [amsg decodeObjectForKey:@"DMessage"];
  [self decodeCwdwgwMessage:rcvd];
  [amsg release];
  [dmsg release];
  [pool release];
}

@end

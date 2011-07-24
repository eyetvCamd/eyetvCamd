#import "sectionFilter.h"
#include "dvbheader.h"
#include "globals.h"

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
  if( pOffset < 0 || pLen < 0 ) 
  {
    return -1;
  }
  if( (pOffset + pLen) != 184) 
  {
    return -1;
  }
  return pLen;
}

@implementation section

- (unsigned int)hash
{
  return pid;
}

- (bool)isEqual:(id)anObj
{
  return pid == [anObj getPid];
}

- (payloadStates) toStream:(unsigned char *)buffer
{
  if( state == statePayloadFull )
  {
    return stateNone;
  }
  TSHeader rcv;
  int payloadLen = payload(buffer);
  unsigned int offset = 188 - payloadLen;
  rcv.word = ntohl(*(unsigned int *)buffer);
  if( rcv.ts.pid != pid )
  {
    pid = rcv.ts.pid;
    [self reset];
  }
  if( state == stateBeginPayloadFound && rcv.ts.payloadStart != 0 )
  {
    [self reset];
  }
  if( state == stateNone && rcv.ts.payloadStart != 0 )
  {
    if( payloadLen > 0 )
    {
      CC = rcv.ts.continuity & 0xf;
      state = stateBeginPayloadFound;
      [pidBytes setLength:0];
      [pidBytes appendBytes:(buffer + offset) length:payloadLen];
    }
  } 
  else if( state == stateBeginPayloadFound && rcv.ts.payloadStart == 0 )
  {
    if( payloadLen > 0 )
    {
      unsigned int nextCC = rcv.ts.continuity & 0xf;
      int expectedCC = CC + 1;
      if( expectedCC == 16 )
	expectedCC = 0;
      if( nextCC == expectedCC )
      {
	CC = expectedCC;
	[pidBytes appendBytes:(buffer + offset) length:payloadLen];
      }
      else 
      {
	[self reset];
	ControllerLog("section filter: PID 0x%x: Continuity Indicator failed: expected %d, received %d\n", pid, CC+1, nextCC);
      }
    }
    else
    {
      [self reset];
    }
  }
  if( state == stateBeginPayloadFound )
  {
    unsigned char *sectionData = (unsigned char *)[pidBytes bytes];
    unsigned int sectionLen = (((sectionData[1] & 0xf) << 8) | sectionData[2]) + 3;
    if( sectionLen <= [pidBytes length] )
    {
      sectionBytes = [[NSData alloc] initWithBytes:sectionData length:sectionLen];
      state = statePayloadFull;
    }
  }
  return state;
}

- (BOOL) nextSection
{
  if( sectionBytes != 0 )
  {
    sectionOffset += [sectionBytes length];
    unsigned char *sectionData = (unsigned char *)[pidBytes bytes] + sectionOffset;
    unsigned int sectionLen = (((sectionData[1] & 0xf) << 8) | sectionData[2]) + 3;
    if( sectionData[0] != 0xff && ( sectionOffset + sectionLen ) <= [pidBytes length] )
    {
      [sectionBytes release];
      sectionBytes = [[NSData alloc] initWithBytes:sectionData length:sectionLen];
      return YES;
    }
  }
  return NO;
}

- (id) initWithPid:(unsigned int)_pid
{
  self = [super init];
  if( self != nil )
  {
    state = stateNone;
    CC = 0;
    pidBytes = [[NSMutableData alloc] initWithCapacity:512];
    sectionBytes = nil;
    sectionOffset = 0;
	pid = _pid;
  }
  return self;
}

- (void) dealloc
{
  [pidBytes release];
  [super dealloc];
}

- (unsigned int) getPid
{
  return pid;
}

- (void) setPid:(unsigned int) _pid
{
  pid = _pid;
}

- (unsigned char *) getBuffer
{
  return (unsigned char *)[sectionBytes bytes];
}

- (NSData *) getData
{
  return sectionBytes;
}

- (void) reset
{
  state = stateNone;
  sectionOffset = 0;
  if( sectionBytes != 0 )
  {
    [sectionBytes release];
    sectionBytes = 0;
  }
}

@end

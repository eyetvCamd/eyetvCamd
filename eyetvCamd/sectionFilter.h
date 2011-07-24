#import <Cocoa/Cocoa.h>

typedef enum
{
  stateNone,
  stateBeginPayloadFound,
  statePayloadFull
} payloadStates;

@interface section: NSObject
{
@protected
  unsigned int pid;
  unsigned int CC;
  NSMutableData *pidBytes;
  NSData *sectionBytes;
  payloadStates state;
  int sectionOffset;
}

- (unsigned int)hash;
- (bool)isEqual:(id)anObj;
- (payloadStates) toStream:(unsigned char *)buffer;
- (id) initWithPid:(unsigned int)_pid;
- (void) dealloc;
- (unsigned int) getPid;
- (void) setPid:(unsigned int) _pid;
- (unsigned char *) getBuffer;
- (NSData *) getData;
- (void) reset;
- (BOOL) nextSection;

@end


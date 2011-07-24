#include <sys/time.h>
#import "uniproto.h"
#include <openssl/md5.h>

extern "C" unsigned long crc32(unsigned long, void *, unsigned int );

@implementation caFilterEntry

- (unsigned int)caid
{
  return caid;
}

- (unsigned int)ident
{
  return ident;
}

- (void)updateCaid:(int)_caid
{
  caid = _caid;
}

- (void)updateIdent:(int)_ident
{
  ident = _ident;
}

- (unsigned)hash
{
  unsigned long crc = 0;
  crc = crc32(crc, &caid, sizeof(caid));
  return crc32(crc, &ident, sizeof(ident));
}

- (bool)isEqual:(id)anObj
{
  return ( caid == [anObj caid] && ident == [anObj ident] );
}

- (id)initWithCaid:(unsigned int)_caid Ident:(unsigned int)_ident
{
  self = [super init];
  if( self != nil )
  {
    caid = _caid;
    ident = _ident;
  }
  return self;
}

- (void) dealloc 
{
  [super dealloc];
}

@end 

@implementation uniproto

- (BOOL)isEqual:(id)anObj
{
  if( memcmp(objSign, [anObj getSignature], 16) != 0 )
    return NO;
  return YES;
}

- (unsigned)hash
{
  return crc32(0, objSign, 16);
}

- (id)initWithProto:(NSString *)_proto User:(NSString *)_user Password:(NSString *)_password Host:(NSString *)_host Port:(NSString *)_port NcdKey:(NSString *)_key
{
  self = [super init];
  if( self != nil )
  {
    isEnabled = NO;
    filterSet = [[NSMutableArray alloc] init];
    protoStr = [[NSString alloc] initWithString:_proto];
    userStr = [[NSString alloc] initWithString:_user];
    passStr = [[NSString alloc] initWithString:_password];
    hostStr = [[NSString alloc] initWithString:_host];
    portStr = [[NSString alloc] initWithString:_port];
    keyStr = [[NSString alloc] initWithString:_key];
    emmAllowed = [[NSMutableSet alloc] init];
    [self updateSignature];
    [self updateFilterSignature];
    struct timeval tv;
    gettimeofday(&tv, NULL);
    for(int i = 0; i < 16; i++ )
    {
      last.lastActive[i] = tv;
    }
  }
  return self;
}

- (void)addFilterEntry:(caFilterEntry *)entry
{
  [filterSet addObject:entry];
  [self updateFilterSignature];
}

- (void)removeFilterAtIndex:(int)index
{
  [filterSet removeObjectAtIndex:index];
  [self updateFilterSignature];
}

- (unsigned char *)getSignature
{
  return objSign;
}

- (unsigned char *)getFilterSignature
{
  return filterSign;
}

- (void)sendEcmPacket:(NSData *)Packet Cadesc:(caDescriptor *)desc Ssid:(unsigned int)ssid devIndex:(unsigned int)index
{
  NSException* exc = [NSException exceptionWithName:@"MethodNotImplemented"
		   reason:@"Subclass of uniproto must override virtual method \"sendEcmPacket:\""
		 userInfo:nil];
  [exc raise];
}

- (void)sendEmmPacket:(NSData *)Packet Params:(emmParams *)param
{
}

- (void)enable:(BOOL)action
{
  isEnabled = action;
}

- (void)setDelegate:(id)obj
{
  delegateObj = obj;
}

- (BOOL)isEnabled
{
  return isEnabled;
}

- (NSString *)getProto
{
  return protoStr;
}

- (NSString *)getUser
{
  return userStr;
}

- (NSString *)getPassword
{
  return passStr;
}

- (NSString *)getHost
{
  return hostStr;
}

- (NSString *)getPort
{
  return portStr;
}

- (NSString *)getKey
{
  return keyStr;
}

- (caFilterEntry *)getFilterEntryAtIndex:(int)index
{
  return [filterSet objectAtIndex:index];
}

- (int)getCaidAtIndex:(int)index
{
  caFilterEntry *ca = [filterSet objectAtIndex:index];
  return [ca caid];
}

- (int)getIdentAtIndex:(int)index
{
  caFilterEntry *ca = [filterSet objectAtIndex:index];
  return [ca ident];
}

- (int)getFilterCount
{
  return [filterSet count];
}

- (void)updateFilterSignature
{
  MD5_CTX context;
  MD5_Init(&context);
  for(int i = 0; i < [filterSet count]; i ++)
  {
    caFilterEntry *ca = [filterSet objectAtIndex:i];
    int caid = [ca caid];
    int ident = [ca ident];
    MD5_Update(&context, &caid, sizeof(caid));
    MD5_Update(&context, &ident, sizeof(ident));
  }
  MD5_Final(filterSign, &context);
}

- (void)updateSignature
{
  MD5_CTX context;
  MD5_Init(&context);
  MD5_Update(&context, [protoStr cStringUsingEncoding:NSASCIIStringEncoding], [protoStr lengthOfBytesUsingEncoding:NSASCIIStringEncoding]);
  MD5_Update(&context, [userStr cStringUsingEncoding:NSASCIIStringEncoding], [userStr lengthOfBytesUsingEncoding:NSASCIIStringEncoding]);
  MD5_Update(&context, [passStr cStringUsingEncoding:NSASCIIStringEncoding], [passStr lengthOfBytesUsingEncoding:NSASCIIStringEncoding]);
  MD5_Update(&context, [hostStr cStringUsingEncoding:NSASCIIStringEncoding], [hostStr lengthOfBytesUsingEncoding:NSASCIIStringEncoding]);
  MD5_Update(&context, [portStr cStringUsingEncoding:NSASCIIStringEncoding], [portStr lengthOfBytesUsingEncoding:NSASCIIStringEncoding]);
  MD5_Update(&context, [keyStr cStringUsingEncoding:NSASCIIStringEncoding], [keyStr lengthOfBytesUsingEncoding:NSASCIIStringEncoding]);
  MD5_Final(objSign, &context);
}

- (void)updateProto:(NSString *)proto
{
  if( [protoStr compare:proto] != NSOrderedSame )
  {
    NSString *pstr = [[NSString alloc] initWithString:proto];
    [protoStr release];
    protoStr = pstr; 
    [self updateSignature];
  }
}

- (void)updateUser:(NSString *)user
{
  if( [userStr compare:user] != NSOrderedSame )
  {
    NSString *pstr = [[NSString alloc] initWithString:user];
    [userStr release];
    userStr = pstr; 
    [self updateSignature];
  }
}
- (void)updatePassword:(NSString *)password
{
  if( [passStr compare:password] != NSOrderedSame )
  {
    NSString *pstr = [[NSString alloc] initWithString:password];
    [passStr release];
    passStr = pstr; 
    [self updateSignature];
  }
}

- (void)updateHost:(NSString *)host
{
  if( [hostStr compare:host] != NSOrderedSame )
  {
    NSString *pstr = [[NSString alloc] initWithString:host];
    [hostStr release];
    hostStr = pstr; 
    [self updateSignature];
  }
}

- (void)updatePort:(NSString *)port
{
  if( [portStr compare:port] != NSOrderedSame )
  {
    NSString *pstr = [[NSString alloc] initWithString:port];
    [portStr release];
    portStr = pstr; 
    [self updateSignature];
  }
}

- (void)updateKey:(NSString *)ncdkey;
{
  if( [keyStr compare:ncdkey] != NSOrderedSame )
  {
    NSString *pstr = [[NSString alloc] initWithString:ncdkey];
    [keyStr release];
    keyStr = pstr; 
    [self updateSignature];
  }
}

- (void)updateCaid:(int)caid atIndex:(int)index
{
  caFilterEntry *ca = [filterSet objectAtIndex:index];
  [ca updateCaid:caid];
  [self updateFilterSignature];
}

- (void)updateIdent:(int)ident atIndex:(int)index
{
  caFilterEntry *ca = [filterSet objectAtIndex:index];
  [ca updateIdent:ident];
  [self updateFilterSignature];
}

- (BOOL)isAllowdCaid:(int)caid Ident:(int)ident
{
  if( [filterSet count] == 0 )
  {
    return YES;
  }
  caFilterEntry *ca = [[caFilterEntry alloc] initWithCaid:caid Ident:ident];
  BOOL result = [filterSet containsObject:ca];
  [ca release];
  return result;
}

- (void)dealloc
{
  [emmAllowed release];
  [filterSet removeAllObjects];
  [filterSet release];
  [protoStr release];
  [userStr release];
  [passStr release];
  [hostStr release];
  [portStr release];
  [keyStr release];
  [super dealloc];
}

@end

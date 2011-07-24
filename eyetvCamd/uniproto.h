#import <Cocoa/Cocoa.h>
#import "pmt.h"
#import "emm.h"

void ControllerLog(const char *format, ...);

@interface caFilterEntry : NSObject
{
  @private
  unsigned int caid;
  unsigned int ident;
}

- (unsigned int)caid;
- (unsigned int)ident;
- (void)updateCaid:(int)_caid;
- (void)updateIdent:(int)_ident;
- (unsigned)hash;
- (bool)isEqual:(id)anObj;
- (id)initWithCaid:(unsigned int)_caid Ident:(unsigned int)_ident;
- (void)dealloc;
@end

typedef struct
{
  unsigned char lastSign[16][16];
  struct timeval lastActive[16];
} lastParams;

@interface uniproto : NSObject 
{
  @protected
  NSString *protoStr;
  NSString *userStr;
  NSString *passStr;
  NSString *hostStr;
  NSString *portStr;
  NSString *keyStr;
  NSMutableArray *filterSet;
  unsigned char objSign[16];
  unsigned char filterSign[16];
  bool isEnabled;
  id delegateObj;
  struct timeval lastDwTime;
  NSMutableSet *emmAllowed;
  lastParams last;
}

- (void)dealloc;
- (id)initWithProto:(NSString *)_proto User:(NSString *)_user Password:(NSString *)_password
	       Host:(NSString *)_host Port:(NSString *)_port NcdKey:(NSString *)_key;

- (BOOL)isEqual:(id)anObj;
- (unsigned)hash;

- (void)addFilterEntry:(caFilterEntry *)entry;
- (caFilterEntry *)getFilterEntryAtIndex:(int)index;
- (void)removeFilterAtIndex:(int)index;
- (BOOL)isAllowdCaid:(int)caid Ident:(int)ident;

- (BOOL)isEnabled;
- (void)sendEcmPacket:(NSData *)Packet Cadesc:(caDescriptor *)desc Ssid:(unsigned int)ssid devIndex:(unsigned int)index;
- (void)sendEmmPacket:(NSData *)Packet Params:(emmParams *)param;
- (void)enable:(BOOL)action;
- (void)setDelegate:(id)obj;
- (unsigned char *)getSignature;
- (unsigned char *)getFilterSignature;
- (void)updateSignature;
- (void)updateFilterSignature;

- (void)updateProto:(NSString *)proto;
- (void)updateUser:(NSString *)user;
- (void)updatePassword:(NSString *)password;
- (void)updateHost:(NSString *)host;
- (void)updatePort:(NSString *)port;
- (void)updateKey:(NSString *)ncdkey;
- (void)updateCaid:(int)caid atIndex:(int)index;
- (void)updateIdent:(int)ident atIndex:(int)index;

- (NSString *)getProto;
- (NSString *)getUser;
- (NSString *)getPassword;
- (NSString *)getHost;
- (NSString *)getPort;
- (NSString *)getKey;
- (int)getCaidAtIndex:(int)index;
- (int)getIdentAtIndex:(int)index;
- (int)getFilterCount;

@end

#import <Cocoa/Cocoa.h>
#import "pmt.h"

typedef enum
{
  emmStateNone = 0,
  emmStateReady,
  emmStateReadyUseParams,
  via8cdReceived,
  cryptoworks84Received
} emmDataState;

@interface cat : NSObject
{
@private
  NSMutableSet *caList;
}

- (id)init;
- (void)dealloc;
- (void)parseCATPayload:(NSData *)packet;
- (bool)parseCaDescriptor:(Descriptor *)caDesc;
- (NSArray *)getCaDescriptors;
- (int)caDescCount;
- (void)clearList;
@end

@interface emmParams : NSObject
{
@protected
  unsigned int caid;
  unsigned int ident;
  unsigned char cardSerial[8];
  unsigned char providerData[8];
  unsigned int globalCrc;
  int globalToggle;
}

//- (id)initWithSerial:(unsigned char *)sn caid:(unsigned int)casys ident:(unsigned int)provid;
- (id)initWithSerial:(unsigned char *)sn provData:(unsigned char *)pd caid:(unsigned int)casys ident:(unsigned int)provid;
- (void)dealloc;
- (unsigned)hash;
- (bool)isEqual:(id)anObj;
- (unsigned int)getCaid;
- (unsigned int)getIdent;
- (unsigned char *)getCardSerial;
- (unsigned char *)getProviderData;
- (unsigned int)getGlobalCrc;
- (void)setGlobalCrc:(unsigned int)crc;
- (int)getGlobalToggle;
- (void)setGlobalToggle:(int)togle;
@end

emmDataState processEmmData(unsigned char *emmPacket, emmParams *params, NSMutableData *emmBuffer, emmDataState currentState);

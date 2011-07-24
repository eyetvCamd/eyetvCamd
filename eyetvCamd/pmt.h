#import <Cocoa/Cocoa.h>

typedef enum
{
  CA_FLAG_DYNAMIC = 0,
  CA_FLAG_STATIC = 1
} releaseFlag;

typedef enum
{
  DECRYPT_MODE_NONE = 0,
  DECRYPT_MODE_TPS_DW_ENCRYPTED = 1
} decryptFlag;

enum
{
  SECA_CA_SYSTEM	= 0x100,
  VIACCESS_CA_SYSTEM	= 0x500,
  IRDETO_CA_SYSTEM	= 0x600,
  BETA_CA_SYSTEM	= 0x1700,
  NAGRA_CA_SYSTEM	= 0x1800,
  CONAX_CA_SYSTEM	= 0x0b00,
  CRYPTOWORKS_CA_SYSTEM	= 0xd00,
  NDS_CA_SYSTEM		= 0x900
};

typedef struct descr Descriptor;
struct descr
{
  unsigned char descriptor_tag;
  unsigned char descriptor_length;
  
  Descriptor * next() { return (Descriptor*)( ((unsigned char*)(this+1)) + descriptor_length ); }
  unsigned char *Data() { return ((unsigned char *)this + 2); }
  unsigned int descSize() { return ( descriptor_length + 2 ); }
};

typedef struct esdescr ESDescriptor;
struct esdescr
{
  unsigned char streamType;
  unsigned char esPidHi;
  unsigned char esPidLow;
  unsigned char esInfoLenHi;
  unsigned char esInfoLow;
  unsigned int esInfoLen() { return ( (((esInfoLenHi & 0xf) << 8) | (esInfoLow & 0xff)) ); }
  unsigned int descSize() { return ( esInfoLen() + 5 ); }
  ESDescriptor * next() { return (ESDescriptor*)( (unsigned char*)this + descSize() ); }
  unsigned char *Data() { return ((unsigned char *)this + 5); }
};

@interface caDescriptor : NSObject
{
  @private
  unsigned int ecmpid;
  unsigned int casys;
  unsigned int ident;
  unsigned int messageId;
  unsigned int irdetoChannel;
  unsigned int ssid;
  decryptFlag dMode;
  releaseFlag flag;
}

- (id) initStaticWithEcmpid:(unsigned int)pid casys:(unsigned int)ca ident:(unsigned int)provid;
- (id) initDynamicWithEcmpid:(unsigned int)pid casys:(unsigned int)ca ident:(unsigned int)provid;
- (void) setEcmpid:(unsigned int)pid casys:(unsigned int)ca ident:(unsigned int)provid;
- (unsigned)hash;
- (bool)isEqual:(id)anObj;
- (unsigned int)getEcmpid;
- (unsigned int)getCasys;
- (unsigned int)getIdent;
- (unsigned int)getMessageId;
- (unsigned int)getIrdetoChannel;
- (void)setMessageId:(unsigned int)msgId;
- (void)setIrdetoChannel:(unsigned int)chn;
- (decryptFlag)getDmode;
- (void)setDmode:(decryptFlag)mode;
- (unsigned int)getSsid;
- (void)setSsid:(unsigned int)sid;

@end

@interface pmt : NSObject
{
@private
  NSMutableSet *caList;
}

- (id)init;
- (void)dealloc;
- (void)parsePmtPayload:(NSData *)packet;
- (void)parseCaDescriptor:(Descriptor *)caDesc;
- (NSArray *)getCaDescriptors;
- (int)caDescCount;
- (void)clearList;
@end

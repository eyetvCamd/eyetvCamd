#import <Cocoa/Cocoa.h>
#import "pmt.h"
#import "AGSocket/AGSocket.h"
#import "uniproto.h"
#include "aes.h"


typedef struct 
{
  unsigned char cmd; // 0x01 - CW, 0x00 - ECM, 0x02 - EMM
  unsigned char len;
  unsigned char pad[2];
  unsigned long crc;
} udp_header_s;

typedef struct 
{
  unsigned short srvID;
  unsigned short casID;
  unsigned int prvID;
  unsigned short pinID;
} service_s;

typedef struct 
{
  udp_header_s udp;
  service_s service;
} camd35xHeader;

static inline unsigned long getcrc(unsigned char *buf) { return ((buf[0] << 24) | (buf[1] << 16) | (buf[2] << 8) | buf[3]); }

@interface camd3Client : uniproto
{
@private
  AGSocket *sock;
  bool connected;
  unsigned long userCrc;
  AES_KEY  decrypt_key;
  AES_KEY  encrypt_key;
  int socketType;
}

- (void)dealloc;
- (void)sendEcmPacket:(NSData *)Packet Cadesc:(caDescriptor *)desc Ssid:(unsigned int)ssid devIndex:(unsigned int)index;
- (void)socketBecameReadable:(AGSocket *)rdSock;
- (void)socketConnected:(AGSocket *)cnSock;
- (id)initWithProto:(NSString *)_proto User:(NSString *)_user Password:(NSString *)_password
	       Host:(NSString *)_host Port:(NSString *)_port NcdKey:(NSString *)_key;
@end

@interface NSObject (camd3ClientDelegate) 
- (void)writeDwToDescrambler:(unsigned char *)dw caDesc:(caDescriptor *)ca sid:(unsigned long)sid;
@end
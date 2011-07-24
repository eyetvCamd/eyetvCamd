#import <Cocoa/Cocoa.h>
#import "pmt.h"
#import "AGSocket/AGSocket.h"
#import "uniproto.h"

#define CWS_FIRSTCMDNO 0xe0
#define CWS_NETMSGSIZE 2400
#define DES_ENCRYPT (0) /* Encrypt */
#define DES_DECRYPT (1) /* Decrypt */

typedef enum
{
  MSG_CLIENT_2_SERVER_LOGIN = CWS_FIRSTCMDNO,
  MSG_CLIENT_2_SERVER_LOGIN_ACK,
  MSG_CLIENT_2_SERVER_LOGIN_NAK,
  MSG_CARD_DATA_REQ,
  MSG_CARD_DATA,
  MSG_SERVER_2_CLIENT_NAME,
  MSG_SERVER_2_CLIENT_NAME_ACK,
  MSG_SERVER_2_CLIENT_NAME_NAK,
  MSG_SERVER_2_CLIENT_LOGIN,
  MSG_SERVER_2_CLIENT_LOGIN_ACK,
  MSG_SERVER_2_CLIENT_LOGIN_NAK,
  MSG_ADMIN,
  MSG_ADMIN_ACK,
  MSG_ADMIN_LOGIN,
  MSG_ADMIN_LOGIN_ACK,
  MSG_ADMIN_LOGIN_NAK,
  MSG_ADMIN_COMMAND,
  MSG_ADMIN_COMMAND_ACK,
  MSG_ADMIN_COMMAND_NAK,
  MSG_KEEPALIVE = CWS_FIRSTCMDNO + 0x1d
} netMsgType;

@interface ncdClient : uniproto
{
@private
  unsigned int protocol_version;
  unsigned char desKey[14];
  unsigned char workKey[14];
  unsigned char sessionKey[16];
  unsigned char loginKey[16];
  unsigned char spread[16];
  unsigned short rcvMsgId;
  unsigned short sndMsgId;
  unsigned long rcvSsid;
  char passwd[120];
  
  AGSocket *sock;
  bool connected;
  bool authenticated;
  bool randomBytesReceived;
  bool phase2;
  NSMutableSet *requested;
}

- (void)dealloc;
- (void)sendEcmPacket:(NSData *)Packet Cadesc:(caDescriptor *)desc Ssid:(unsigned int)ssid devIndex:(unsigned int)index;
- (void)socketBecameReadable:(AGSocket *)rdSock;
- (void)socketConnected:(AGSocket *)cnSock;
- (id)initWithProto:(NSString *)_proto User:(NSString *)_user Password:(NSString *)_password
	       Host:(NSString *)_host Port:(NSString *)_port NcdKey:(NSString *)_key;
- (BOOL)serverSend:(NSMutableData *)stream key:(unsigned char *)deskey ssid:(unsigned long)ssid;
- (int)desEncrypt:(NSMutableData *)stream key:(unsigned char *)deskey;
- (int)desDecrypt:(NSMutableData *)stream key:(unsigned char *)deskey;
- (void)desRandomGet:(unsigned char *)padBytes length:(int)noPadBytes;
- (void)desKeyParityAdjust:(unsigned char *)key length:(int)len;
- (void)desKeySpread:(unsigned char *)normal;
- (void)md5Crypt:(unsigned char *)pw salt:(const char *)salt;
- (void)tryToServerConnect:(NSTimer *)obj;
- (void)lateSend:(NSTimer *)obj;
@end

@interface NSObject (ncdClientDelegate) 
- (void)writeDwToDescrambler:(unsigned char *)dw caDesc:(caDescriptor *)ca sid:(unsigned long)sid;
- (void)emmAddParams:(unsigned char *)sn provData:(unsigned char *)pd caid:(unsigned int)casys ident:(unsigned int)provid;
- (void)emmRmParams:(unsigned char *)sn provData:(unsigned char *)pd caid:(unsigned int)casys ident:(unsigned int)provid;
@end
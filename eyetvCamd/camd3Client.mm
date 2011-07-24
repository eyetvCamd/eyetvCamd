#include <sys/time.h>
#import "camd3Client.h"
#import "pmt.h"
#include <openssl/md5.h>
#include "globals.h"

extern "C" unsigned long crc32(unsigned long, void *, unsigned int );

@implementation camd3Client

- (void)sendEcmPacket:(NSData *)Packet Cadesc:(caDescriptor *)desc Ssid:(unsigned int)ssid devIndex:(unsigned int)index
{
  unsigned char ecmSign[16];
  unsigned char *buffer = (unsigned char *)[Packet bytes];
  int len = [Packet length]; 
  MD5(buffer + 3, len - 3, ecmSign);
  bool doSendRequest = false;
  struct timeval curTime;
  gettimeofday(&curTime, NULL);
  if( memcmp(ecmSign, last.lastSign[index], 16) != 0 )
    doSendRequest = true;
  if( last.lastActive[index].tv_sec + 30 <= curTime.tv_sec )
    doSendRequest = true;
  if( doSendRequest == true )
  {
    last.lastActive[index] = curTime;
    memcpy(last.lastSign[index], ecmSign, 16);
    if( connected == true )
    {
      if( getShowRequests() == YES )
      {
	ControllerLog("camd3: %s:%s Send Ecm (%04x, %06x)\n", 
			  [hostStr cStringUsingEncoding:NSASCIIStringEncoding],
			    [portStr cStringUsingEncoding:NSASCIIStringEncoding],
			      [desc getCasys], [desc getIdent]);
      }
      lastDwTime = curTime;
      unsigned char b[512], encBuf[512];
      camd35xHeader *cHeader = (camd35xHeader *)b;
      
      memset(b, 0xff ,512);
      cHeader->udp.cmd = 0;
      cHeader->udp.len = len;
      memcpy(b + 20, buffer, len);
      cHeader->udp.crc = htonl(crc32(0, b + 20, len));
      cHeader->service.srvID = htons(ssid & 0xffff);
      cHeader->service.pinID = htons([desc getEcmpid] & 0xffff);
      cHeader->service.casID = htons([desc getCasys] & 0xffff);
      cHeader->service.prvID = htonl([desc getIdent]);
      len += 20;
      len = (((len - 1) >> 4) + 1 ) << 4;
      *(u_long *)encBuf = htonl(userCrc);
      for( int i = 0; i < len; i += 16)
      {
	AES_encrypt(&b[i], &encBuf[4 + i], &encrypt_key);
      }
      [sock writeBytes:encBuf ofLength:(len + 4)];
    }
  }
}

- (void)socketBecameReadable:(AGSocket *)rdSock
{
  unsigned char buf[512], decBuf[512], *b;
  int len = [rdSock bytesAvailable];
  len = [rdSock readBytes:buf ofLength:len fromAddress:nil withFlags:0];
  b = buf;
  if(len > 0)
  {
    b = b + 4; len -= 4;
    for(int i = 0; i < len; i += 16)
    {
      AES_decrypt(&b[i], &decBuf[i], &decrypt_key); 
    }
    if( userCrc == getcrc(buf) )
    {
      camd35xHeader *hdr = (camd35xHeader *)decBuf;
      if( hdr->udp.cmd == 1 && hdr->udp.len == 16 && len >= (36))
      {
	if ([delegateObj respondsToSelector:@selector(writeDwToDescrambler:caDesc:sid:)])
	{
	  caDescriptor *desc = [[caDescriptor alloc] initStaticWithEcmpid:ntohs(hdr->service.pinID) 
								    casys:ntohs(hdr->service.casID)
								    ident:ntohl(hdr->service.prvID)];
	  if( getShowRequests() == YES )
	  {
	    struct timeval dwt;
	    gettimeofday(&dwt, NULL);
	    double rcvTime = dwt.tv_sec * 1000000 + dwt.tv_usec;
	    double sndTime = lastDwTime.tv_sec * 1000000 + lastDwTime.tv_usec;
	    int rspTime = (rcvTime - sndTime)/1000;
	    ControllerLog("camd3: %s:%s Receive DW (%04x, %06x), took %d ms\n", 
			  [hostStr cStringUsingEncoding:NSASCIIStringEncoding],
			    [portStr cStringUsingEncoding:NSASCIIStringEncoding],
			      [desc getCasys], [desc getIdent], rspTime);
	  }
	  [delegateObj writeDwToDescrambler:(decBuf + 20) caDesc:desc sid:ntohs(hdr->service.srvID)];
	  [desc release];
	}
      }
    }
  }
  else if( len <= 0 )
  {
  }
}

- (void)tryToServerConnect:(NSTimer *)tObj
{
  if( isEnabled == NO )
  {
    return;
  }
  
  if( connected == NO )
  {
    if( sock != 0 )
    {
      [sock close];
      [sock release];
    }
    unsigned short srvPort = strtol([portStr cStringUsingEncoding:NSASCIIStringEncoding],0,0) & 0xffff;
    AGInetSocketAddress *srvAddr = [AGInetSocketAddress addressWithHostname:hostStr port:htons(srvPort)];
    sock = [[AGSocket alloc] initWithFamily:PF_INET type:socketType protocol:0];
    if( sock != nil )
    {
      [sock setDelegate:self];
      [sock connectToAddressInBackground:srvAddr];
      connected = true;
      ControllerLog("camd3: connected to server %s:%s\n",
		      [hostStr cStringUsingEncoding:NSASCIIStringEncoding],
			[portStr cStringUsingEncoding:NSASCIIStringEncoding]);
    }
  }
  
  [NSTimer scheduledTimerWithTimeInterval:5 target:self selector:@selector(tryToServerConnect:)
				 userInfo:nil repeats:NO];
}


- (void)socketConnected:(AGSocket *)cnSock; 
{
  connected = true;
}

- (void)setDelegate:(id)obj
{
  delegateObj = obj;
}

- (void)enable:(BOOL)action
{
  isEnabled = action;
  if( action == YES )
  {
    connected = NO;
    [self tryToServerConnect:nil];
  }
  else
  {
    connected = NO;
    [sock close];
    [sock release];
    sock = nil;
  }
}

- (id)initWithProto:(NSString *)_proto 
	       User:(NSString *)_user
	   Password:(NSString *)_password
	       Host:(NSString *)_host
	       Port:(NSString *)_port
	     NcdKey:(NSString *)_key
{
  self = [super initWithProto:_proto User:_user Password:_password Host:_host Port:_port NcdKey:_key];
  if (self != nil)
  {
    if( [protoStr isEqualToString:@"cd3tcp"] )
    {
      socketType = SOCK_STREAM;
    }
    else
    {
      socketType = SOCK_DGRAM;
    }
    unsigned char keyDig[16];
    userCrc = crc32(0, MD5((const unsigned char *)[_user cStringUsingEncoding:NSASCIIStringEncoding], [_user length], keyDig), 16);
    MD5((const unsigned char *)[_password cStringUsingEncoding:NSASCIIStringEncoding], [_password length], keyDig);
    AES_set_decrypt_key(keyDig, 128, &decrypt_key);
    AES_set_encrypt_key(keyDig, 128, &encrypt_key); 
    connected = false;
  }
  return self;
}

- (void) dealloc
{
  if( sock != nil )
  {
    [sock release];
  }
  [super dealloc];
}

@end

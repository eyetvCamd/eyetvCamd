#include <sys/time.h>
#import "ncdClient.h"
#include <openssl/des.h>
#include <openssl/md5.h>
#include "globals.h"

void ecb_crypt (void *key, void *buf, unsigned int len, unsigned int mode)
{
  des_key_schedule shed;
  des_set_key_unchecked((const_des_cblock *)key, shed);
  des_ecb_encrypt((const_des_cblock *)buf, (des_cblock *)buf, shed, mode);
}

void cbc_crypt (void *key, void *buf, unsigned int len, unsigned int mode, void *ivec)
{
  des_key_schedule shed;
  des_set_key_unchecked((const_des_cblock *)key, shed);
  des_cbc_encrypt((const unsigned char *)buf, (unsigned char *)buf, len, shed, (des_cblock *)ivec, mode);	
}

static const char *itoa64 = "./0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";
void to64(char *s, unsigned long v, int n)
{
  while (--n >= 0)
  {
    *s++ = itoa64[v & 0x3f];
    v >>= 6;
  }
}

@implementation ncdClient

- (BOOL)serverReceive:(NSMutableData *)stream key:(unsigned char *)deskey
{
  int len = [stream length];
  unsigned char *data = (unsigned char *)[stream mutableBytes];
  if ( ((data[0] << 8) | data[1]) != len - 2 )
  {
    return NO;
  }
  int protoAdd = (protocol_version < 525) ? 11:15;
  if ((len = [self desDecrypt:stream key:deskey]) < protoAdd)
  {
    return NO;
  }
  
  data = (unsigned char *)[stream mutableBytes];
  rcvMsgId = ( (data[2] << 8) | data[3] );
//  ControllerLog("msgid = 0x%04x\n", rcvMsgId & 0xffff); 
  if( protocol_version < 525 )
  {
    rcvSsid = (data[4] << 24) | (data[5] << 16) | (data[6] << 8) | data[7];
    unsigned newLen = [stream length] - 8;
    bcopy(data + 8, data, newLen);
    [stream setLength:newLen];
  }
  else
  {
    rcvSsid = (data[4] << 8) | data[5];
    unsigned newLen = [stream length] - 12;
    bcopy(data + 12, data, newLen);
    [stream setLength:newLen];
//    ControllerLog("ssid = 0x%04x\n", rcvSsid & 0xffff);
  }
  int rlength = [stream length];
  uint msglen = ((((data[1] & 0x0f) << 8) | data[2]) + 3);
  if ( msglen > rlength ) 
  {
    return NO;
  }
  return YES;
}

- (BOOL)serverSend:(NSMutableData *)stream key:(unsigned char *)deskey ssid:(unsigned long)ssid
{
  unsigned char tmpByte;
  unsigned char tmpBuffer[2];
  unsigned char clientID[2]={0x45,0x43}; // EC = EyetvCamd
  int protoAdd = (protocol_version < 525) ? 8:12;
  int streamLength = [stream length];
  if ( streamLength < 0 || (streamLength + protoAdd) > CWS_NETMSGSIZE ) 
  {
    return NO;
  }
  
  const unsigned char *buffer = (const unsigned char *)[stream bytes];
  tmpBuffer[0] = (buffer[1] & 0xf0) | (((streamLength - 3) >> 8) & 0x0f);
  tmpBuffer[1] = (streamLength - 3) & 0xff;
  [stream replaceBytesInRange:NSMakeRange(1, 2) withBytes:tmpBuffer];
  NSMutableData *netbuf = [[NSMutableData alloc] initWithCapacity:512];
  [netbuf increaseLengthBy:2];
  
  tmpByte = (sndMsgId >> 8);
  [netbuf appendBytes:&tmpByte length:1];
  tmpByte = (sndMsgId & 0xff);
  [netbuf appendBytes:&tmpByte length:1];
//  ControllerLog("sndid = 0x%04x\n", sndMsgId & 0xffff);
  if ( ssid != 0 )
  {
    if( protocol_version < 525 )
    {
      tmpByte = ((ssid >> 24) & 0xff);
      [netbuf appendBytes:&tmpByte length:1];
      tmpByte = ((ssid >> 16) & 0xff);
      [netbuf appendBytes:&tmpByte length:1];
      tmpByte = ((ssid >>  8) & 0xff);
      [netbuf appendBytes:&tmpByte length:1];
      tmpByte = (ssid & 0xff);
      [netbuf appendBytes:&tmpByte length:1];
    }
    else
    {
//      ControllerLog("sndssid = 0x%04x\n", ssid & 0xffff);
      tmpByte = ((ssid >>  8) & 0xff);
      [netbuf appendBytes:&tmpByte length:1];
      tmpByte = (ssid & 0xff);
      [netbuf appendBytes:&tmpByte length:1];
      [netbuf increaseLengthBy:6];
    }
  }
  else 
  {
	  [netbuf appendBytes:clientID length:2];
	  [netbuf increaseLengthBy:2];
    if( protocol_version >= 525)
    {
      [netbuf increaseLengthBy:4];
    }
  }
  [netbuf appendData:stream];
  int len = [self desEncrypt:netbuf key:deskey];
  if (len < 0) return false;
  unsigned char *buf = (unsigned char *)[netbuf mutableBytes];
  buf[0] = (len - 2) >> 8;
  buf[1] = (len - 2) & 0xff;
  [sock writeBytes:[netbuf bytes] ofLength:[netbuf length]];
  [netbuf release];
  return true;
}

- (int)desEncrypt:(NSMutableData *)stream  key:(unsigned char *)deskey
{
  unsigned char checksum = 0;
  int noPadBytes;
  unsigned char padBytes[7]/* = {2,2,2,2,2,2,2}*/;
  unsigned char ivec[8]/* = {1,1,1,1,1,1,1,1}*/;
  unsigned short i;
  int len = [stream length];
  
  if ( !deskey ) return len;
  
  noPadBytes = (8 - ((len - 1) % 8)) % 8;
  [stream increaseLengthBy:noPadBytes];
  if ( len + noPadBytes + 1 >= CWS_NETMSGSIZE - 8 )
    return -1;
  unsigned char *buffer = (unsigned char *)[stream mutableBytes];
  [self desRandomGet:padBytes length:noPadBytes];
  for (i = 0; i < noPadBytes; i++)
    buffer[len++] = padBytes[i];
  for (i = 2; i < len; i++)
    checksum ^= buffer[i];
  
  [stream increaseLengthBy:1];
  buffer[len++] = checksum;
  
  [self desRandomGet:ivec length:8];
  [stream increaseLengthBy:8];
  memcpy(buffer + len, ivec, 8);
  for (i = 2; i < len; i += 8)
  {
    cbc_crypt(deskey , (buffer + i), 8, DES_ENCRYPT, ivec);
    ecb_crypt((deskey + 8), (buffer + i), 8, DES_DECRYPT);
    ecb_crypt(deskey  , (buffer + i), 8, DES_ENCRYPT);
    memcpy(ivec, buffer+i, 8);
  }
  len += 8;
  return len;
}

- (int)desDecrypt:(NSMutableData *)stream key:(unsigned char *)deskey
{
  unsigned char ivec[8];
  unsigned char nextIvec[8];
  int i;
  unsigned char checksum = 0;
  int len = [stream length];
  unsigned char *buffer = (unsigned char *)[stream mutableBytes];
  
  if ( !deskey )
    return len;
  
  if ( (len - 2) % 8 || (len - 2) < 16 ) 
    return -1;
  
  len -= 8;
  memcpy(nextIvec, buffer + len, 8);
  for (i = 2; i < len; i += 8)
  {
    memcpy(ivec, nextIvec, 8);
    memcpy(nextIvec, buffer+i, 8);
    ecb_crypt(deskey , (buffer + i), 8, DES_DECRYPT);
    ecb_crypt((deskey + 8), (buffer + i), 8, DES_ENCRYPT);
    cbc_crypt(deskey , (buffer + i), 8, DES_DECRYPT, ivec);
  } 
  for (i = 2; i < len; i++) 
  {
    checksum ^= buffer[i];
  }
  if ( checksum != 0 )
    return -1;
  return len;
}

- (void)desLoginKeyGet:(unsigned char *)key1 key:(unsigned char *)key2
{
  unsigned char des14[14];
  int i;
  for (i = 0; i < 14; i++)
  {
    des14[i] = key1[i] ^ key2[i];
  }
  [self desKeySpread:des14];
  memcpy(loginKey, spread, 16);
}

- (void) desSessionKeyGet:(unsigned char *)key1 pw:(unsigned char *)cryptPw
{
  unsigned char des14[14];
  memcpy(des14, key1, 14);
  for(uint i = 0; i < strlen((const char *)(void *)cryptPw); i++)
  {
    des14[i % 14] ^= cryptPw[i];
  }
  [self desKeySpread:des14];
  memcpy(sessionKey, spread, 16);
}

- (void)desKeyParityAdjust:(unsigned char *)key length:(int)len
{
  unsigned char i, j, parity;
  
  for (i = 0; i < len; i++)
  {
    parity = 1;
    for (j = 1; j < 8; j++) if ((key[i] >> j) & 0x1) parity = ~parity & 0x01;
    key[i] |= parity;
  }
}

- (void) desKeySpread:(unsigned char *)normal
{
  spread[ 0] = normal[ 0] & 0xfe;
  spread[ 1] = ((normal[ 0] << 7) | (normal[ 1] >> 1)) & 0xfe;
  spread[ 2] = ((normal[ 1] << 6) | (normal[ 2] >> 2)) & 0xfe;
  spread[ 3] = ((normal[ 2] << 5) | (normal[ 3] >> 3)) & 0xfe;
  spread[ 4] = ((normal[ 3] << 4) | (normal[ 4] >> 4)) & 0xfe;
  spread[ 5] = ((normal[ 4] << 3) | (normal[ 5] >> 5)) & 0xfe;
  spread[ 6] = ((normal[ 5] << 2) | (normal[ 6] >> 6)) & 0xfe;
  spread[ 7] = normal[ 6] << 1;
  spread[ 8] = normal[ 7] & 0xfe;
  spread[ 9] = ((normal[ 7] << 7) | (normal[ 8] >> 1)) & 0xfe;
  spread[10] = ((normal[ 8] << 6) | (normal[ 9] >> 2)) & 0xfe;
  spread[11] = ((normal[ 9] << 5) | (normal[10] >> 3)) & 0xfe;
  spread[12] = ((normal[10] << 4) | (normal[11] >> 4)) & 0xfe;
  spread[13] = ((normal[11] << 3) | (normal[12] >> 5)) & 0xfe;
  spread[14] = ((normal[12] << 2) | (normal[13] >> 6)) & 0xfe;
  spread[15] = normal[13] << 1;
  [self desKeyParityAdjust:spread length:16];
}

- (void)desRandomGet:(unsigned char *)buffer length:(int)len
{
  unsigned char idx = 0;
  int randomNo = 0;
  
  for (idx = 0; idx < len; idx++)
  {
    if (!(idx % 3)) randomNo = rand();
    buffer[idx] = (randomNo >> ((idx % 3) << 3)) & 0xff;
  }
}

- (void)md5Crypt:(unsigned char *)pw salt:(const char *)salt
{
  const char *magic = "$1$";
  const int magic_len = 3;
  char *p, *sp, *ep;
  unsigned char final[16];
  int sl,pl,i,j,pw_len = strlen((const char *)pw);
  MD5_CTX	ctx,ctx1;
  unsigned long l;
  
  /* Refine the Salt first */
  sp = (char *)salt;
  
  /* If it starts with the magic string, then skip that */
  if(!strncmp(sp, magic, magic_len))
    sp += magic_len;
  
  /* It stops at the first '$', max 8 chars */
  for(ep = sp; *ep && *ep != '$' && ep < (sp + 8); ep++)
    continue;
  
  /* get the length of the true salt */
  sl = ep - sp;
  
  MD5_Init(&ctx);
  
  /* The password first, since that is what is most unknown */
  MD5_Update(&ctx, pw, pw_len);
  
  /* Then our magic string */
  MD5_Update(&ctx, (const void *)magic, magic_len);
  
  /* Then the raw salt */
  MD5_Update(&ctx, (unsigned char *) sp, sl);
  
  /* Then just as many characters of the MD5(pw, salt, pw) */
  MD5_Init(&ctx1);
  MD5_Update(&ctx1, pw, pw_len);
  MD5_Update(&ctx1, (unsigned char *) sp, sl);
  MD5_Update(&ctx1, pw, pw_len);
  MD5_Final(final, &ctx1);
  
  for(pl = pw_len; pl > 0; pl -= 16)
    MD5_Update(&ctx, final, pl > 16 ? 16 : pl);
  
  /* Don't leave anything around in vm they could use. */
  memset(final, 0, sizeof(final));
  
  /* Then something really weird... */
  for (j = 0, i = pw_len; i ; i >>= 1)
    if(i & 1)
      MD5_Update(&ctx, final + j, 1);
    else
      MD5_Update(&ctx, pw + j, 1);
  
  /* Now make the output string */
  strcpy(passwd, magic);
  strncat(passwd, sp, sl);
  strcat(passwd, "$");
  
  MD5_Final(final, &ctx);
  
  /*
   * and now, just to make sure things don't run too fast
   * On a 60 Mhz Pentium this takes 34 msec, so you would
   * need 30 seconds to build a 1000 entry dictionary...
   */
  for(i = 0; i < 1000; i++) {
    MD5_Init(&ctx1);
    
    if(i & 1)
      MD5_Update(&ctx1, pw, pw_len);
    else
      MD5_Update(&ctx1, final, 16);
    
    if(i % 3)
      MD5_Update(&ctx1, (unsigned char *) sp, sl);
    
    if(i % 7)
      MD5_Update(&ctx1, pw, pw_len);
    
    if(i & 1)
      MD5_Update(&ctx1, final, 16);
    else
      MD5_Update(&ctx1, pw, pw_len);
    
    MD5_Final(final, &ctx1);
  }
  
  p = passwd + strlen(passwd);
  
  l = (final[ 0]<<16) | (final[ 6]<<8) | final[12]; to64(p,l,4); p += 4;
  l = (final[ 1]<<16) | (final[ 7]<<8) | final[13]; to64(p,l,4); p += 4;
  l = (final[ 2]<<16) | (final[ 8]<<8) | final[14]; to64(p,l,4); p += 4;
  l = (final[ 3]<<16) | (final[ 9]<<8) | final[15]; to64(p,l,4); p += 4;
  l = (final[ 4]<<16) | (final[10]<<8) | final[ 5]; to64(p,l,4); p += 4;
  l =                    final[11]                ; to64(p,l,2); p += 2;
  *p = '\0';
  
  /* Don't leave anything around in vm they could use. */
  memset(final, 0, sizeof(final));
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
    randomBytesReceived = NO;
    phase2 = NO;
    unsigned short srvPort = strtol([portStr cStringUsingEncoding:NSASCIIStringEncoding],0,0) & 0xffff;
    AGInetSocketAddress *srvAddr = [AGInetSocketAddress addressWithHostname:hostStr port:htons(srvPort)];
    sock = [[AGSocket alloc] initWithFamily:PF_INET type:SOCK_STREAM protocol:0];
    if( sock != nil )
    {
      ControllerLog("newcamd: connecting to server %s:%s...\n",
		    [hostStr cStringUsingEncoding:NSASCIIStringEncoding],
		      [portStr cStringUsingEncoding:NSASCIIStringEncoding]);
      [sock setDelegate:self];
      [sock connectToAddressInBackground:srvAddr];
    }
  }
  
  [NSTimer scheduledTimerWithTimeInterval:5 target:self selector:@selector(tryToServerConnect:)
				 userInfo:nil repeats:NO];
}


- (void)enable:(BOOL)action
{
  isEnabled = action;
  if( action == YES )
  {
    connected = NO;
    authenticated = NO;
    [requested removeAllObjects];
    [self tryToServerConnect:nil];
  }
  else
  {
    if( sock != nil )
    {
      connected = NO;
      authenticated = NO;
      
      [sock close];
      [sock release];
      sock = nil;
    }
  }
}

- (void)socketConnectFailed:(AGSocket *)cnSock
{
  ControllerLog("newcamd: connect to server %s:%s failed: %s.\n",
		 [hostStr cStringUsingEncoding:NSASCIIStringEncoding],
		   [portStr cStringUsingEncoding:NSASCIIStringEncoding],
		     strerror([cnSock error]));
}

- (void)socketConnected:(AGSocket *)cnSock
{
  connected = YES;
  ControllerLog("newcamd: connected to server %s:%s.\n",
		[hostStr cStringUsingEncoding:NSASCIIStringEncoding],
		  [portStr cStringUsingEncoding:NSASCIIStringEncoding]);
}

- (void)newcamdLoginProcedure:(NSMutableData *)pPacket
{
  if( randomBytesReceived == YES )
  {
    if( phase2 == NO )
    {
      [self desLoginKeyGet:desKey key:workKey];
      [self serverReceive:pPacket key:loginKey];
    }
    else
    {
      [self serverReceive:pPacket key:sessionKey];
      phase2 = NO;
    }
  }
  unsigned char *pBuffer = (unsigned char *)[pPacket mutableBytes];
  if( [pPacket length] == 14 && randomBytesReceived == NO )
  {
    // Receive 14 Random Bytes... 
    randomBytesReceived = YES;
    ControllerLog("newcamd: %s:%s: random bytes received...\n",
		  [hostStr cStringUsingEncoding:NSASCIIStringEncoding],
		    [portStr cStringUsingEncoding:NSASCIIStringEncoding]);
    const unsigned char *wk = (const unsigned char *)[pPacket mutableBytes];
    for(int i = 0; i < 14; i++)
    {
      workKey[i] = wk[i];
    }
    [self md5Crypt:(unsigned char *)[passStr cStringUsingEncoding:NSASCIIStringEncoding] salt:"$1$abcdefgh$"];
    [self desSessionKeyGet:desKey pw:(unsigned char *)passwd];
    NSMutableData *css = [[NSMutableData alloc] initWithCapacity:256];
    unsigned char *cssBuf = (unsigned char *)[css mutableBytes];
    [css increaseLengthBy:3];
    cssBuf[0] = MSG_CLIENT_2_SERVER_LOGIN;
    [css appendBytes:[userStr cStringUsingEncoding:NSASCIIStringEncoding]
			length:[userStr lengthOfBytesUsingEncoding:NSASCIIStringEncoding]];
    [css increaseLengthBy:1];
    [css appendBytes:passwd length:strlen(passwd)];
    [css increaseLengthBy:1];
    cssBuf[2] = [css length] - 3;

    [self desLoginKeyGet:desKey key:workKey];
    [self serverSend:css key:loginKey ssid:0];
    [css release];
  }
  else if(pBuffer[0] == MSG_CLIENT_2_SERVER_LOGIN_ACK)
  {
    // cardserver login phase 2
    ControllerLog("newcamd: %s:%s: send DATA_REQ...\n",
		  [hostStr cStringUsingEncoding:NSASCIIStringEncoding],
		    [portStr cStringUsingEncoding:NSASCIIStringEncoding]);
    NSMutableData *css = [[NSMutableData alloc] initWithCapacity:256];
    unsigned char *cssBuf = (unsigned char *)[css mutableBytes];
    [css increaseLengthBy:3];
    cssBuf[0] = MSG_CARD_DATA_REQ;
    phase2 = YES;
    [self serverSend:css key:sessionKey ssid:0];
    [css release];
  }
  else if( pBuffer[0] == MSG_CARD_DATA )
  {
    // Received data from cardserver....
    unsigned char bytes[8];
    int advanced = 0;
    authenticated = true;
    ControllerLog("newcamd: %s:%s: Authorization done.\n",
		  [hostStr cStringUsingEncoding:NSASCIIStringEncoding],
		    [portStr cStringUsingEncoding:NSASCIIStringEncoding]);
    ControllerLog("newcamd: %s:%s: Cards present:\n",
		  [hostStr cStringUsingEncoding:NSASCIIStringEncoding],
		    [portStr cStringUsingEncoding:NSASCIIStringEncoding]);
    unsigned char *pCardData = (unsigned char *)[pPacket mutableBytes];
	  // pCardData+3 as a byte that tells us if AU is enabled or not.
    pCardData += 4;
    advanced += 4;
    unsigned int caid = (pCardData[0] << 8) | pCardData[1];
    int idents = pCardData[10] & 0xff;

    pCardData += 2;
    advanced += 2;
	  // card Serial
    [pPacket getBytes:bytes range:NSMakeRange(advanced, 8)];

    pCardData += 9;
    advanced += 9;
    for(int i = 0; i < idents; i++)
    {
      unsigned char ident[3];
      unsigned char number[8];
      [pPacket getBytes:ident range:NSMakeRange(advanced, 3)];
      advanced += 3;
		// card SA
      [pPacket getBytes:number range:NSMakeRange(advanced, 8)];
      advanced += 8;
      unsigned int provId = ((ident[0] << 16) | (ident[1] << 8) | ident[2]);
      caFilterEntry *desc = [[caFilterEntry alloc] initWithCaid:caid Ident:provId];
      [self addFilterEntry:desc];
      [desc release];
		// both need to be null , if we only test for the SA (aka provider data , aka number)
		// it doesn't work as some card don't have a SA.
      if( memcmp(number, "\000\000\000\000\000\000\000\000", sizeof(number)) == 0 && memcmp(bytes, "\000\000\000\000\000\000\000\000", sizeof(bytes))==0)
      {
			ControllerLog("newcamd: %s:%s:   CAID %04x, IDENT %06x\n",
						  [hostStr cStringUsingEncoding:NSASCIIStringEncoding],
						  [portStr cStringUsingEncoding:NSASCIIStringEncoding],
						  caid & 0xffff, provId & 0xffffff);
      }
      else
      {
			emmParams *emmp = [[emmParams alloc] initWithSerial:bytes provData:number caid:caid ident:provId];
			if( [emmAllowed member:emmp] == nil )
			{
				[emmAllowed addObject:emmp];
				if ( [delegateObj respondsToSelector:@selector(emmAddParams:provData:caid:ident:)] == YES )
				{
					[delegateObj emmAddParams:bytes provData:number caid:caid ident:provId];
				}
				[emmp release];
			}
			ControllerLog("newcamd: %s:%s:   CAID %04x, IDENT %06x,  PROVIDER DATA %02X%02X%02X%02X%02X%02X%02X%02X\n",
						  [hostStr cStringUsingEncoding:NSASCIIStringEncoding],
						  [portStr cStringUsingEncoding:NSASCIIStringEncoding],
						  caid & 0xffff, provId & 0xffffff,
						  number[0], number[1], number[2], 
						  number[3], number[4], number[5],
						  number[6], number[7]);
	  }
    }
  }
  else if( pBuffer[0] == MSG_CLIENT_2_SERVER_LOGIN_NAK )
  {
    //	 cardserver login failed
    ControllerLog("newcamd: Login to server %s:%s failed.\n",
		  [hostStr cStringUsingEncoding:NSASCIIStringEncoding],
		    [portStr cStringUsingEncoding:NSASCIIStringEncoding]);
  }
  else 
  {
    // Unknown cardserver protocol, can't access CA cards 
    ControllerLog("newcamd: %s:%s: protocol broken!\n",
		  [hostStr cStringUsingEncoding:NSASCIIStringEncoding],
		    [portStr cStringUsingEncoding:NSASCIIStringEncoding]);
  }	
}


- (void)socketBecameReadable:(AGSocket *)rdSock
{
  int length = [rdSock bytesAvailable];
  if( length <= 0 )
  {
    connected = false;
    authenticated = false;
    ControllerLog("newcamd: server %s:%s closed connection.\n", 
		  [[self getHost] cStringUsingEncoding:NSASCIIStringEncoding],
		    [[self getPort] cStringUsingEncoding:NSASCIIStringEncoding]);
    int count = [emmAllowed count];
    for( int i = 0; i < count; i++ )
    {
      NSEnumerator *emms = [emmAllowed objectEnumerator];
      for( id obj = [emms nextObject]; obj != nil; obj = [emms nextObject] )
      {
	if ( [delegateObj respondsToSelector:@selector(emmRmParams:caid:ident:)] == YES )
	{
	  [delegateObj emmRmParams:[obj getCardSerial] provData:[obj getProviderData] caid:[obj getCaid] ident:[obj getIdent]];
	}
      }
    }
    [emmAllowed removeAllObjects];
    return;
  }
  NSMutableData *pPacket = [[NSMutableData alloc] initWithLength:((length > 512)? length:512)];
  [pPacket setLength:length];
  length = [rdSock readBytes:[pPacket mutableBytes] ofLength:length fromAddress:nil withFlags:0];
  if( authenticated == NO )
  {
    [self newcamdLoginProcedure:pPacket];
  }
  else
  {
    int advanced = 0;
    while(advanced < length)
    {
      unsigned char *pBuf = (unsigned char *)[pPacket mutableBytes] + advanced;
      int packetLength = (pBuf[0] << 8) | pBuf[1];
      packetLength += 2;
      if( packetLength <= length )
      {
	if( packetLength <= 256 )
	{
	  NSMutableData *pOnePacket = [[NSMutableData alloc] initWithBytesNoCopy:(pBuf + advanced)
						  length:packetLength freeWhenDone:NO];
	  [self serverReceive:pOnePacket key:sessionKey];
	  caDescriptor *ncdDesc = [[caDescriptor alloc] initStaticWithEcmpid:0 casys:0 ident:0];
	  [ncdDesc setMessageId:rcvMsgId];
	  caDescriptor *desc = [requested member:ncdDesc];
	  [ncdDesc release];

	  if( desc != nil )
	  {
	    [desc retain];
	    [requested removeObject:desc];
	    [desc setMessageId:0];
	  }

	  unsigned char *pOneBuf = (unsigned char *)[pOnePacket mutableBytes];
	  if( (pOneBuf[0] == 0x80 || pOneBuf[0] == 0x81) && pOneBuf[2] == 0x10)
	  {
	    if ( [delegateObj respondsToSelector:@selector(writeDwToDescrambler:caDesc:sid:)] == YES 
		  && desc != nil )
	    {
	      if( getShowRequests() == YES )
	      {
		struct timeval dwt;
		gettimeofday(&dwt, NULL);
		double rcvTime = dwt.tv_sec * 1000000 + dwt.tv_usec;
		double sndTime = lastDwTime.tv_sec * 1000000 + lastDwTime.tv_usec;
		int rspTime = (rcvTime - sndTime)/1000;
		ControllerLog("newcamd: %s:%s Receive DW (%04x, %06x), took %d ms\n",
			      [hostStr cStringUsingEncoding:NSASCIIStringEncoding],
				[portStr cStringUsingEncoding:NSASCIIStringEncoding],
				  [desc getCasys], [desc getIdent], rspTime);
	      }
	      [delegateObj writeDwToDescrambler:&pOneBuf[3] caDesc:desc sid:[desc getSsid]];
	    }
	  }

	  if( desc != nil )
	  {
	    [desc release];
	  }
	  [pOnePacket release];
	}
	advanced += packetLength;
      }
      else
      {
	break;
      }
    }
  }
  [pPacket release];
}

- (void)lateSend:(NSTimer *)tobj
{
  NSArray *args = [tobj userInfo];
  unsigned int ssid = [[args objectAtIndex:0] unsignedIntValue];
  NSMutableData *packet = [args objectAtIndex:1];
  [self serverSend:packet key:sessionKey ssid:ssid];
}

- (void)sendEcmPacket:(NSData *)Packet Cadesc:(caDescriptor *)desc Ssid:(unsigned int)ssid devIndex:(unsigned int)index;
{
  if( connected != YES || authenticated != YES )
  {
    return;
  }
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
    if( connected == YES && authenticated == YES )
    {
      if( getShowRequests() == YES )
      {
	ControllerLog("newcamd: %s:%s Send Ecm (%04x, %06x)\n", 
		      [hostStr cStringUsingEncoding:NSASCIIStringEncoding],
			[portStr cStringUsingEncoding:NSASCIIStringEncoding],
			  [desc getCasys], [desc getIdent]);
      }
      lastDwTime = curTime;
      sndMsgId = (++sndMsgId & 0xffff);
      if( sndMsgId == 0 ) 
      {
	sndMsgId =1;
      }
      caDescriptor *ncdDesc = [[caDescriptor alloc] initStaticWithEcmpid:[desc getEcmpid]
						   casys:[desc getCasys] ident:[desc getIdent]];
      [ncdDesc setMessageId:sndMsgId];
      [ncdDesc setSsid:ssid];
      [requested addObject:ncdDesc];
      [ncdDesc release];
      NSMutableData *ncdPacket = [[NSMutableData alloc] initWithCapacity:512];
      [ncdPacket appendData:Packet];
/*      unsigned char *ecm = (unsigned char *)[ncdPacket mutableBytes];
      unsigned int lid = (ecm[6] << 16) | (ecm[7] << 8) | (ecm[8] & 0xf0);
      if(lid == 0x020710)
      {
	ecm[8] = 0x14;
	ControllerDump(ncdPacket);
      }*/
      [self serverSend:ncdPacket key:sessionKey ssid:ssid];
//      NSArray *args = [NSArray arrayWithObjects:[NSNumber numberWithUnsignedInt:ssid], ncdPacket, nil];
//      [NSTimer scheduledTimerWithTimeInterval:2 target:self selector:@selector(lateSend:)
//				     userInfo:args repeats:NO];
      [ncdPacket release];
    }
  }
}

- (void)sendEmmPacket:(NSData *)Packet Params:(emmParams *)param;
{
  if( connected != YES || authenticated != YES )
  {
    return;
  }
  if( [emmAllowed member:param] == nil )
  {
    return;
  }
  sndMsgId = (++sndMsgId & 0xffff);
  if( sndMsgId == 0 ) 
  {
    sndMsgId = 1;
  }
  if( getEmmDebug() == TRUE )
  {
    ControllerLog("newcamd: %s:%s Send EMM (%04x, %06x)\n", 
		  [hostStr cStringUsingEncoding:NSASCIIStringEncoding],
		    [portStr cStringUsingEncoding:NSASCIIStringEncoding],
		      [param getCaid], [param getIdent]);
    ControllerDump(Packet);
  }
  NSMutableData *ncdPacket = [[NSMutableData alloc] initWithCapacity:512];
  [ncdPacket appendData:Packet];
  [self serverSend:ncdPacket key:sessionKey ssid:0];
  [ncdPacket release];
}

- (id)initWithProto:(NSString *)_proto User:(NSString *)_user Password:(NSString *)_password
	       Host:(NSString *)_host Port:(NSString *)_port NcdKey:(NSString *)_key
{
  self = [super initWithProto:_proto User:_user Password:_password Host:_host Port:_port NcdKey:_key];
  if (self != nil)
  {
    connected = false;
    authenticated = false;
    unsigned int tmpKey[14];
    int i = sscanf( [keyStr cStringUsingEncoding:NSASCIIStringEncoding],
		    "%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x",
		      &tmpKey[0], &tmpKey[1], &tmpKey[2], &tmpKey[3], &tmpKey[4], &tmpKey[5],
			&tmpKey[6], &tmpKey[7], &tmpKey[8], &tmpKey[9], &tmpKey[10], &tmpKey[11],
			  &tmpKey[12], &tmpKey[13]) ;
    if( i == 14 )
    {
      desKey[0] = tmpKey[0] & 0xff;
      desKey[1] = tmpKey[1] & 0xff;
      desKey[2] = tmpKey[2] & 0xff;
      desKey[3] = tmpKey[3] & 0xff;
      desKey[4] = tmpKey[4] & 0xff;
      desKey[5] = tmpKey[5] & 0xff;
      desKey[6] = tmpKey[6] & 0xff;
      desKey[7] = tmpKey[7] & 0xff;
      desKey[8] = tmpKey[8] & 0xff;
      desKey[9] = tmpKey[9] & 0xff;
      desKey[10] = tmpKey[10] & 0xff;
      desKey[11] = tmpKey[11] & 0xff;
      desKey[12] = tmpKey[12] & 0xff;
      desKey[13] = tmpKey[13] & 0xff;
    }
    else
    {
      bzero(desKey, sizeof(desKey));
    }
    protocol_version = 525;
    sndMsgId = 1;
    requested = [[NSMutableSet alloc] init];
  }
  return self;
}

- (void)dealloc
{
  [requested release];
  if( sock != nil )
  {
    [sock release];
    sock = nil;
  }
  [super dealloc];
}

@end 

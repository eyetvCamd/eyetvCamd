#import "pmt.h"
extern "C" unsigned long crc32(unsigned long, void *, unsigned int );
@implementation caDescriptor
- (id) initStaticWithEcmpid:(unsigned int)pid casys:(unsigned int)ca ident:(unsigned int)provid
{
  self = [super init];
  if( self != nil )
  {
    ecmpid = pid;
    casys = ca;
    ident = provid;
    messageId = 0;
    flag = CA_FLAG_STATIC;
  }
  return self;
}

- (id) initDynamicWithEcmpid:(unsigned int)pid casys:(unsigned int)ca ident:(unsigned int)provid
{
  self = [super init];
  if( self != nil )
  {
    ecmpid = pid;
    casys = ca;
    ident = provid;
    messageId = 0;
    irdetoChannel = 0;
    flag = CA_FLAG_DYNAMIC;
    dMode = DECRYPT_MODE_NONE;
  }
  return self;
}

- (void) setEcmpid:(unsigned int)pid casys:(unsigned int)ca ident:(unsigned int)provid
{
  ecmpid = pid;
  casys = ca;
  ident = provid;
}

- (void)setMessageId:(unsigned int)msgId
{
  messageId = msgId;
}

- (void)setIrdetoChannel:(unsigned int)chn
{
  irdetoChannel = chn;
}

- (unsigned int)getEcmpid
{
  return ecmpid;
}

- (unsigned int)getCasys
{
  return casys;
}

- (unsigned int)getIdent
{
  return ident;
}

- (unsigned int)getMessageId
{
  return messageId;
}

- (unsigned int)getIrdetoChannel
{
  return irdetoChannel;
}

- (unsigned)hash
{
  unsigned long crc = 0;
  if( messageId == 0 )
  {
    crc = crc32(crc, &casys, sizeof(casys));
    crc = crc32(crc, &ident, sizeof(ident));
    crc = crc32(crc, &ecmpid, sizeof(ecmpid));
  }
  else
  {
    crc = messageId;
  }
  return crc;
}

- (bool)isEqual:(id)anObj
{
  if( messageId != 0 || [anObj getMessageId] != 0 )
  {
    return ( messageId == [anObj getMessageId] );
  }
  return ( (casys == [anObj getCasys]) && (ident == [anObj getIdent]) && (ecmpid == [anObj getEcmpid]) );
}

- (decryptFlag)getDmode
{
  return dMode;
}

- (void)setDmode:(decryptFlag)mode
{
  dMode = mode;
}

- (unsigned int)getSsid
{
  return ssid;
}

- (void)setSsid:(unsigned int)sid
{
  ssid = sid;
}


@end

@implementation pmt
- (id) init
{
  self = [super init];
  if (self != nil)
  {
    caList = [[NSMutableSet alloc] init];
  }
  return self;
}

- (void) dealloc
{
  [caList removeAllObjects];
  [caList release];
  [super dealloc];
}

- (void)parseCaDescriptor:(Descriptor *)caDesc
{
  if( caDesc->descriptor_tag != 0x9 ) return;
  unsigned char *caData = caDesc->Data();
  unsigned int caSysytem = ((caData[0] & 0xff) << 8) | (caData[1] & 0xff);
  unsigned int caOffset = 2;
  switch( caSysytem & 0xff00 )
  {
    case SECA_CA_SYSTEM:
    {
        while( caOffset <= ( caDesc->descriptor_length - 4 ) )
        {
            unsigned char *secaCa = caData + caOffset;
            unsigned int ecmPid = ((secaCa[0] & 0x1f) << 8) | secaCa[1];
            unsigned int ident =  ((secaCa[2] & 0xff) << 8) | secaCa[3];
            caDescriptor *cdNew = [[caDescriptor alloc] initStaticWithEcmpid:ecmPid casys:caSysytem ident:ident];
            if ( [caList containsObject:cdNew] == NO )
            {
                [caList addObject:cdNew];
            }
            [cdNew release];
            caOffset += 15;
        }
    } break;

    case VIACCESS_CA_SYSTEM:
    {
        unsigned char *viaCa = caData + caOffset;
        unsigned int ecmPid = ((viaCa[0] & 0x1f) << 8) | viaCa[1];
        caOffset += 2;
        while( caOffset <= ( caDesc->descriptor_length - 5 ))
        {
            unsigned char *viaCa = caData + caOffset;
            if( viaCa[0] == 0x14 )
            {
                uint ident = (viaCa[2] << 16) | (viaCa[3] << 8) | (viaCa[4] & 0xf0);
                caDescriptor *cdNew = [[caDescriptor alloc] initStaticWithEcmpid:ecmPid casys:caSysytem ident:ident];
                if ( [caList containsObject:cdNew] == NO )
                {
                    [caList addObject:cdNew];
                }
                [cdNew release];
            }
            caOffset += viaCa[1] + 2;
        }
    } break;
          
    default:
    {
        unsigned char *defCa = caData + caOffset;
        uint ecmPid = ((defCa[0] & 0x1f) << 8) | defCa[1];
        caDescriptor *cdNew = [[caDescriptor alloc] initStaticWithEcmpid:ecmPid casys:caSysytem ident:0];
        if ( [caList containsObject:cdNew] == NO )
        {
            [caList addObject:cdNew];
        }
        [cdNew release];
    }
  }
}

- (void)parsePmtPayload:(NSData *)packet
{
  [caList removeAllObjects];
  unsigned int curOffset = 0;
  unsigned int curMaxLen = [packet length]  - 4 /* CRC32 */;
  unsigned char *pData = (unsigned char *)[packet bytes];
  
  int cahdLen = ((pData[10] & 0xf) << 8) | (pData[11] & 0xff);
  curOffset += 12;
  
  if( cahdLen != 0 )
  {
    int curCahdLen = 0;
    Descriptor *descs = (Descriptor *)(pData + curOffset);
    do
    {
      if( descs->descriptor_tag == 0x9 ) // found CA descriptor, parse it
      {
	[self parseCaDescriptor:descs];
      }
      curCahdLen += descs->descSize();
      descs = descs->next();
    } while( curCahdLen < cahdLen );
  }
  curOffset += cahdLen;
  ESDescriptor *esdescs = (ESDescriptor *)(pData + curOffset); 
  while(curOffset < curMaxLen)
  {
    Descriptor *curDesc = (Descriptor *)esdescs->Data();
    unsigned int dLength = esdescs->esInfoLen();
    int dOffset = 0;
    do
    {
      if(curDesc->descriptor_tag == 0x9 ) // found CA descriptor, parse it
      {
	[self parseCaDescriptor:curDesc];
      }
      dOffset += curDesc->descSize();
      curDesc = curDesc->next();
    } while ( dOffset < dLength ) ;
    
    curOffset += esdescs->descSize();
    esdescs = esdescs->next();
  }
}

- (NSArray *)getCaDescriptors
{
  return [caList allObjects];
}

- (int)caDescCount
{
  return [caList count];
}

- (void)clearList
{
  [caList removeAllObjects];
}


@end

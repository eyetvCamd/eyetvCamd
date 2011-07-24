#import "emm.h"
#import "globals.h"

@implementation cat

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

- (void)parseCATPayload:(NSData *)packet
{
    [caList removeAllObjects];
    if( getEmmDebug() == TRUE )
    {
        ControllerLog("CAT Received:------------\n");
        ControllerDump(packet);
        ControllerLog("--------------------------------\n");
    }
    unsigned int curOffset = 8; // skip header 
    unsigned int curMaxLen = [packet length]  - 4 /* CRC32 */;
    unsigned char *pData = (unsigned char *)[packet bytes];
    Descriptor *descs = (Descriptor *)(pData + curOffset);
    do
    {
        if( descs->descriptor_tag == 0x9 ) // found CA descriptor, parse it
        {
            if( [self parseCaDescriptor:descs] == NO )
            {
                ControllerLog("CAT: Unknown CA Descriptor:\n");
                ControllerDump([NSData dataWithBytes:(pData + curOffset) length:descs->descSize()]);
            }
        }
        curOffset += descs->descSize();
        descs = descs->next();
    } while( curOffset < curMaxLen );
}

- (bool)parseCaDescriptor:(Descriptor *)caDesc
{
    if( caDesc->descriptor_tag != 0x9 ) 
    {
        return NO;
    }
    unsigned char *caData = caDesc->Data();
    unsigned int caSysytem = ((caData[0] & 0xff) << 8) | (caData[1] & 0xff);
    unsigned int caOffset = 2;
    switch( caSysytem & 0xff00 )
    {
        case SECA_CA_SYSTEM:
        {
            unsigned char *secaCa = caData + caOffset;
            unsigned int emmPid = ((secaCa[0] & 0x1f) << 8) | secaCa[1];
            caDescriptor *cdNew = [[caDescriptor alloc] initStaticWithEcmpid:emmPid casys:caSysytem ident:0];
            if ( [caList containsObject:cdNew] == NO )
            {
                [caList addObject:cdNew];
            }
            [cdNew release];
            if( caDesc->descriptor_length > 4 )
            {
                for(int i = 3, nn = secaCa[2]; nn != 0; nn--, i += 4 )
                {
                    emmPid = ((secaCa[i] & 0x1f) << 8) | secaCa[i + 1];
                    caDescriptor *cdNew = [[caDescriptor alloc] initStaticWithEcmpid:emmPid casys:caSysytem ident:0];
                    if ( [caList containsObject:cdNew] == NO )
                    {
                        [caList addObject:cdNew];
                    }
                    [cdNew release];
                }
            }
        } break;
        case VIACCESS_CA_SYSTEM:
        {
            unsigned char *viaCa = caData + caOffset;
            unsigned int emmPid = ((viaCa[0] & 0x1f) << 8) | viaCa[1];
            caDescriptor *cdNew = [[caDescriptor alloc] initStaticWithEcmpid:emmPid casys:caSysytem ident:0];
            if ( [caList containsObject:cdNew] == NO )
            {
                [caList addObject:cdNew];
            }
            [cdNew release];
        } break;
        default:
        {
            unsigned char *defCa = caData + caOffset;
            uint emmPid = ((defCa[0] & 0x1f) << 8) | defCa[1];
            caDescriptor *cdNew = [[caDescriptor alloc] initStaticWithEcmpid:emmPid casys:caSysytem ident:0];
            if ( [caList containsObject:cdNew] == NO )
            {
                [caList addObject:cdNew];
            }
            [cdNew release];
        }
    }
    return YES;
}

@end

extern "C" unsigned long crc32(unsigned long, void *, unsigned int );
@implementation emmParams

- (id)initWithSerial:(unsigned char *)sn provData:(unsigned char *)pd caid:(unsigned int)casys ident:(unsigned int)provid
{
    self = [super init];
    if( self != nil )
    {
        memcpy(cardSerial, sn, sizeof(cardSerial));
        memcpy(providerData, pd, sizeof(providerData));
        caid = casys;
        ident = provid;
        globalCrc=0;
        globalToggle=0;
    }
    return self;
}

/*- (id)initWithSerial:(unsigned char *)sn caid:(unsigned int)casys ident:(unsigned int)provid
 {
 self = [super init];
 if( self != nil )
 {
 memcpy(cardSerial, sn, sizeof(cardSerial));
 memcpy(providerData, sn, sizeof(providerData));
 caid = casys;
 ident = provid;
 }
 return self;
 }*/

- (void)dealloc
{
    [super dealloc];
}

- (unsigned)hash
{
    unsigned long crc = crc32(0, &caid, sizeof(caid)); 
    crc = crc32(crc, &ident, sizeof(ident));
    crc = crc32(crc, cardSerial, sizeof(cardSerial));
    crc = crc32(crc, providerData, sizeof(providerData));
    return crc;
}

- (bool)isEqual:(id)anObj
{
    if( caid == [anObj getCaid] && 
       ident == [anObj getIdent] && 
       memcmp(cardSerial, [anObj getCardSerial], sizeof(cardSerial)) == 0 &&
       memcmp(providerData, [anObj getProviderData], sizeof(providerData)) == 0 )
        return YES;
    return NO;
}

- (unsigned int)getCaid
{
    return caid;
}

- (unsigned int)getIdent
{
    return ident;
}

- (unsigned char *)getCardSerial
{
    return cardSerial;
}

- (unsigned char *)getProviderData
{
    return providerData;
}
- (unsigned int)getGlobalCrc
{
    return globalCrc;
}

- (void)setGlobalCrc:(unsigned int)crc
{
    globalCrc=crc;
}

- (int)getGlobalToggle
{
    return globalToggle;
}

- (void)setGlobalToggle:(int)togle
{
    globalToggle=togle;
}

@end


static emmDataState processViaccessEmm(unsigned char *emm, emmParams *params, NSMutableData *emmBuffer, emmDataState curState)
{
    unsigned int emmPktLen = (((emm[1] & 0xf) << 8) | emm[2]) + 3;
    unsigned char *serial = [params getCardSerial];
    unsigned int ident = [params getIdent];
    
    if( emm[0] != 0x88 && emm[0] != 0x8c && emm[0] != 0x8d && emm[0] != 0x8e && emm[0] != 0x8a && emm[0] != 0x8b )
    {
        if( getEmmDebug() == TRUE )
        {
            ControllerLog("Unsupported Viaccess EMM table number 0x%x\n", emm[0] & 0xff);
        }
    }
    if( emm[0] == 0x88 )
    {
        if( emm[4] == serial[4] &&
           emm[5] == serial[5] && 
           emm[6] == serial[6] &&
           emm[7] == serial[7] )
        {
            if( getEmmDebug() == TRUE )
            {
                ControllerLog("Viaccess EMM-U:\n");
                ControllerDump( [NSData dataWithBytes:emm length:emmPktLen] );
            }
            return emmStateReady;
        }
    }
    else if( emm[0] == 0x8e && curState == via8cdReceived )
    {
        if( emm[3] == serial[4] && 
           emm[4] == serial[5] && 
           emm[5] == serial[6] )
        {
            // assemble EMM packet from 8c/8d and 8e
            unsigned char nano9e[] = {0x9e, 0x20};
            unsigned char nanof0[] = {0xf0, 0x08};
            unsigned char * pEmmData = (unsigned char *)[emmBuffer mutableBytes];
            int emmLen = (((pEmmData[4 + 1] & 0xf) << 8) | pEmmData[4 + 2]) + 3 + 4; // 4 - offset to store SA
            if( getEmmDebug() == TRUE )
            {
                ControllerLog("Viaccess : assemble EMM-S from\n");
                ControllerDump( [NSData dataWithBytes:((char *)[emmBuffer bytes] + 4) length:([emmBuffer length] - 4)] );
                ControllerLog("and\n");
                ControllerDump([NSData dataWithBytes:emm length:emmPktLen]);
                ControllerLog("\n");
            }
            // write table header
            pEmmData[0] = 0x8e;
            pEmmData[1] = 0x70;
            pEmmData[2] = 0;
            // write SA
            pEmmData[3] = serial[4];
            pEmmData[4] = serial[5];
            pEmmData[5] = serial[6];
            pEmmData[6] = 1;
            // write ADF
            [emmBuffer appendBytes:nano9e length:2];
            emmLen += 2;
            [emmBuffer appendBytes:(emm + 7) length:32];
            emmLen += 32;
            // write signature 
            [emmBuffer appendBytes:nanof0 length:2];
            emmLen += 2;
            [emmBuffer appendBytes:(emm + 7 + 32) length:8];
            emmLen += 8;
            // update len
            pEmmData[1] |= ((emmLen - 3) >> 8) & 0xf;
            pEmmData[2] = (emmLen - 3) & 0xff;
            
            return emmStateReadyUseParams;
        }
    }
    
    else if( emm[0] == 0x8a || emm[0] == 0x8b)
    {
        if( getEmmDebug() == TRUE )
        {
            ControllerLog("Viaccess EMM-G :\n");
            ControllerDump([NSData dataWithBytes:emm length:emmPktLen]);
            ControllerLog("\n");
        }
        return emmStateReady;
    }
    
    else if( emm[0] == 0x8c || emm[0] == 0x8d ) 
    {
        if( emm[3] == 0x90 && emm[4] == 3 && 
           emm[5] == ((ident >> 16) & 0xff) &&
           emm[6] == ((ident >>  8) & 0xff) &&
           (emm[7] & 0xf0) == (ident & 0xff) )
        {
            [emmBuffer setLength:4];
            [emmBuffer appendBytes:emm length:emmPktLen];
            return via8cdReceived;
        }
    }
    
    if( curState == via8cdReceived )
        return via8cdReceived;
    return emmStateNone;
}

static emmDataState processIrdetoEmm(unsigned char *emm, emmParams *params, NSMutableData *emmBuffer, emmDataState curState)
{
    if( emm[0] == 0x82 )
    {
        unsigned int emmPktLen = (((emm[1] & 0xf) << 8) | emm[2]) + 3;
        int serialLen = emm[3] & 0x7;
        int mode = (emm[3] >> 3);
        bool ok = NO;
        
        if( getEmmDebug() == TRUE )
        {
            if( emm[4] == 4 && emm[5] == 3 )
            {
                ControllerLog("--------------------------------------------\n");
                ControllerDump( [NSData dataWithBytes:emm length:emmPktLen] );
            }
        }
        
        if( mode & 0x10 )
        {
            unsigned char *serial = [params getCardSerial];
            if( mode == serial[4] )
            {
                if( serialLen == 0 || memcmp(&emm[4], &serial[5], serialLen) == 0 )
                {
                    ok = YES;
                    if( getEmmDebug() == TRUE )
                    {
                        ControllerLog("Irdeto EMM-HEXSERIAL\n");
                    }
                }
            }
        }
        else
        {
            unsigned char *prov = [params getProviderData];
            if( mode == prov[4] )
            {
                if( serialLen == 0 || memcmp(&emm[4], &prov[5], serialLen) == 0 )
                {
                    ok = YES;
                    if( getEmmDebug() == TRUE )
                    {
                        ControllerLog("Irdeto EMM-PROVIDER\n");
                    }
                }
            }
        }
        if( ok == YES )
        {
            //      if( getEmmDebug() == TRUE )
            //      {
            //	ControllerDump( [NSData dataWithBytes:emm length:emmPktLen] );
            //      }
            return emmStateReady;
        }
    }
    return emmStateNone;
}


static emmDataState processConaxEmm(unsigned char *emm, emmParams *params, NSMutableData *emmBuffer, emmDataState curState)
{
    if( emm[0] == 0x82 )
    {
        unsigned char *providerData = [params getProviderData]+2;
        unsigned char *emmProviderData = &emm[4];
        
        if(memcmp(providerData,emmProviderData,6)==0) {
            if( getEmmDebug() == TRUE ) {
                ControllerLog("Conax EMM\n");
                ControllerLog("provider data:\n");
                ControllerDump( [NSData dataWithBytes:providerData length:6 ]);
                ControllerLog("emm provider data:\n");
                ControllerDump( [NSData dataWithBytes:emmProviderData length:6] );
            }
            return emmStateReady;
        }
    }
    return emmStateNone;
}

static void SortNanos(unsigned char *dest, const unsigned char *src, int len)
{
    int w=0, c=-1;
    while(1) {
        int n=0x100;
        for(int j=0; j<len;) {
            int l=src[j+1]+2;
            if(src[j]==c) {
                if(w+l>len) {
                    if( getEmmDebug() == TRUE ) {
                    ControllerLog("sortnanos: sanity check failed. Exceeding memory area. Probably corrupted nanos!");
                    }
                    memset(dest,0,len); // zero out everything
                    return;
                }
                memcpy(&dest[w],&src[j],l);
                w+=l;
            }
            else if(src[j]>c && src[j]<n)
                n=src[j];
            j+=l;
        }
        if(n==0x100) break;
        c=n;
    }
}


static emmDataState processCryptoworksEmm(unsigned char *emm, emmParams *params, NSMutableData *emmBuffer, emmDataState curState)
{
    unsigned int emmPktLen = (((emm[1] & 0xf) << 8) | emm[2]) + 3;
    unsigned char *serial = [params getCardSerial]+3;

    if((emm[1]&0xF0)!=0x70 || emm[3]!=0xA9 || emm[4]!=0xff)
        return emmStateNone;

    if(emm[0]== 0x82) {
        // need to check UA
        if(memcmp(&emm[5],serial,5)!=0)
            return emmStateNone;
        if( getEmmDebug() == TRUE ) {
            ControllerLog("Cryptoworks EMM-U:\n");
            ControllerDump( [NSData dataWithBytes:emm length:emmPktLen] );
        }
        return emmStateReady; // no assemble needed
    }

    else if(emm[0]==0x84) {
        // need to check SA
        if(memcmp(&emm[5],serial,4)!=0)
            return emmStateNone;
        [emmBuffer setLength:0];
        [emmBuffer appendBytes:emm length:emmPktLen];
        return cryptoworks84Received;
    }

    else if( emm[0] == 0x86 && curState == cryptoworks84Received ) {
        unsigned char * pEmmData = (unsigned char *)[emmBuffer mutableBytes];
        int sharedLen=[emmBuffer length];
        int emmLen=emmPktLen-5 + sharedLen-12;
        if( getEmmDebug() == TRUE ) {
            ControllerLog("Cryptoworks : assemble EMM-S from\n");
            ControllerDump( [NSData dataWithBytes:((char *)[emmBuffer bytes]) length:([emmBuffer length])] );
            ControllerLog("and\n");
            ControllerDump([NSData dataWithBytes:emm length:emmPktLen]);
            ControllerLog("\n");
        }

        NSMutableData *tmp = [[NSMutableData alloc] init];
        [tmp appendBytes:&emm[5] length:emmPktLen-5];
        [tmp appendBytes:&pEmmData[12] length:sharedLen-12];

        NSMutableData *assemble = [[NSMutableData alloc] initWithLength:emmLen+12];
        [assemble appendBytes:pEmmData length:12];
        
        unsigned char * pAssembledData = (unsigned char *)[assemble mutableBytes];
        unsigned char * pTmp = (unsigned char *)[tmp mutableBytes];
        SortNanos(pAssembledData+12,pTmp,emmLen);

        pAssembledData[1]=((emmLen+9)>>8) | 0x70;
        pAssembledData[2]=(emmLen+9) & 0xFF;
        
        if(pAssembledData[11]==emmLen) { // sanity check
            [emmBuffer setLength:0];
            [emmBuffer appendBytes:pAssembledData length:emmLen+12];
            [tmp release];
            [assemble release];
            return emmStateReadyUseParams; // assembled
        }
        [tmp release];
        [assemble release];
        return emmStateNone;
    }

    if( emm[0] == 0x88 || emm[0]== 0x89) {
      if(emm[0]!=[params getGlobalToggle]) {
        if( getEmmDebug() == TRUE ) {
            ControllerLog("Cryptoworks EMM-G:\n");
            ControllerDump( [NSData dataWithBytes:emm length:emmPktLen] );
        }
        [params setGlobalToggle:emm[0]];
        //unsigned int crc=crc32_le(0,emm+1,emmPktLen-1);
        unsigned int crc=crc32(0,emm+1,emmPktLen-1);
        if(crc!=[params getGlobalCrc]) {
          [params setGlobalCrc:crc];
          return emmStateReady; // no assemble needed
          }
        }
    }

    
    
      return emmStateNone;
}

/*
 Example of GLOBAL EMM's
 This one has IRD-EMM + Card-EMM
 82 70 20 00 02 06 02 7D 0E 89 53 71 16 90 14 40
 01 ED 17 7D 9E 1F 28 CF 09 97 54 F1 8E 72 06 E7
 51 AF F5

 82 70 20 -> emm len = 0x23 (35)
 00 -> numAddr = 1  ->  ((emm[3]&0x30)>>4)+1
    |-> emm[3]&0xC0 = 0 -> s=0, address_len=4

 IRD-EMM part, 02 00 or 02 06 xx aabbccdd yy
 num_filter = 0 (as s=0)  
 payloadStart = emm+4+(address_len*num_filter) = emm+4
 02 06 ->ird emm, 6 bytes long
 02 = xx
 7D 0E 89 53 = aabbccdd
 71 = yy

 payloadStart += 2 + payloadStart[1]; = emm + 4 + 2 + 6 = emm+12
 payloadSize = emmLen - payloadStart = emm[3]-(payloadStart-emm) = 35 - 12 = 23
 payload = 16 90 14 40 01 ED 17 7D 9E 1F 28 CF 09 97 54 F1 8E 72 06 E7 51 AF F5
 
 16 90 14 40  -> is that a card address ?
 01 ED 17 7D 9E 1F 28 CF 09 97 54 F1 8E 72 06 E7 51 AF F5
 
 This one has only IRD-EMM
 82 70 6D 00 07 69 01 30 07 14 5E 0F FF FF 00 06 
 00 0D 01 00 03 01 00 00 00 0F 00 00 00 5E 01 00 
 01 0C 2E 70 E4 55 B6 D2 34 F7 44 86 9E 5C 91 14
 81 FC DF CB D0 86 65 77 DF A9 E1 6B A8 9F 9B DE
 90 92 B9 AA 6C B3 4E 87 D2 EC 92 DA FC 71 EF 27 
 B3 C3 D0 17 CF 0B D6 5E 8C DB EB B3 37 55 6E 09 
 7F 27 3C F1 85 29 C9 4E 0B EE DF 68 BE 00 C9 00

 82 70 6D ->  emm len = 0x70 (112)
 00 -> numAddr = 1  ->  ((emm[3]&0x30)>>4)+1
    |-> emm[3]&0xC0 = 0 -> s=0, address_len=4

 num_filter = 0 (as s=0)  
 payloadStart = emm+4+(address_len*num_filter) = emm+4
 07 69 ->  ird emm, 0x69 bytes long (105 bytes)
 01 30 07 14 5E 0F FF FF 00 06 
 00 0D 01 00 03 01 00 00 00 0F 00 00 00 5E 01 00 
 01 0C 2E 70 E4 55 B6 D2 34 F7 44 86 9E 5C 91 14
 81 FC DF CB D0 86 65 77 DF A9 E1 6B A8 9F 9B DE
 90 92 B9 AA 6C B3 4E 87 D2 EC 92 DA FC 71 EF 27 
 B3 C3 D0 17 CF 0B D6 5E 8C DB EB B3 37 55 6E 09 
 7F 27 3C F1 85 29 C9 4E 0B EE DF 68 BE 00 C9 00
 I guess the last 00 is for pading
 
 Multipart EMM
 
 82 30 ad 70 00 XX XX XX 00 XX XX XX 00 XX XX XX 00 XX XX XX 00 00 
 d3 02 00 22 90 20 44 02 4a 50 1d 88 ab 02 ac 79 16 6c df a1 b1 b7 77 00 ba eb 63 b5 c9 a9 30 2b 43 e9 16 a9 d5 14 00 
 d3 02 00 22 90 20 44 02 13 e3 40 bd 29 e4 90 97 c3 aa 93 db 8d f5 6b e4 92 dd 00 9b 51 03 c9 3d d0 e2 37 44 d3 bf 00
 d3 02 00 22 90 20 44 02 97 79 5d 18 96 5f 3a 67 70 55 bb b9 d2 49 31 bd 18 17 2a e9 6f eb d8 76 ec c3 c9 cc 53 39 00 
 d2 02 00 21 90 1f 44 02 99 6d df 36 54 9c 7c 78 1b 21 54 d9 d4 9f c1 80 3c 46 10 76 aa 75 ef d6 82 27 2e 44 7b 00
 */

static emmDataState processNDSEmm(unsigned char *emm, emmParams *params, NSMutableData *emmBuffer, emmDataState curState)
{
    int payloadSize;
    unsigned char *payloadStart;
    int i,s;
    bool processEmm;
    bool assembled;
    unsigned char *serial = [params getCardSerial]+4;
    
    unsigned int emmPktLen = (((emm[1] & 0xf) << 8) | emm[2]) + 3;
    unsigned int numAddrs = ((emm[3]&0x30)>>4)+1;
    unsigned char addresses[numAddrs][4];
    NSMutableData *data;
    unsigned char *pEmmData;
    int address_len = 0;
    int word_block;
    int block_len;
    int position=-1;
    
    switch(emm[3]&0xC0) {
        case 0x80:
            s=3;
            address_len=4;
            break;
        case 0x40:
            s=4;
            address_len=4;
            break;
        default:
            s=0;
            address_len=4;
    }
    
    

    if(s>0) {
        for(i=0;i<numAddrs; i++) {
            if(memcmp(&emm[i*4+4],serial,s)==0) {
                position++;
                break;
            }
        }
    }

    if( getEmmDebug() == TRUE ) {
        ControllerLog("NDS card serial :\n");
        ControllerDump( [NSData dataWithBytes:serial length:4] );
        
        ControllerLog("NDS emm packet :\n");
        ControllerDump( [NSData dataWithBytes:emm length:emmPktLen] );
        
        ControllerLog("NDS emm packet numAddrs = %d\n",numAddrs);


        ControllerLog("NDS emm packet addresses:\n");
        ControllerDump( [NSData dataWithBytes:emm+4 length:numAddrs*address_len] );
    }

    if(emm[1]==0x70) {
        // check that this emm is for us
        if(position!=-1)
            processEmm=TRUE;
        else {
            processEmm=FALSE;
            if( getEmmDebug() == TRUE ) {
                ControllerLog("NDS emm not matching card UA or SA\n");
            }
        }

        if(processEmm) {
            if( getEmmDebug() == TRUE ) {
                if(s==3)
                    ControllerLog("NDS EMM-S\n");
                if(s==4)
                    ControllerLog("NDS EMM-U\n");
            }
            return emmStateReady;
        }
        
        return emmStateNone;
    }

    
    // let just check the 1 of the serial matches
    //if(position!=-1) {
    //    if( getEmmDebug() == TRUE ) {
            ControllerLog("NDS EMM-G, Multipart EMM or IRD-EMM with card payload\n");
    //    }
        return emmStateReady;
    //}
    //else
    //    return emmStateNone;
    
    // this might not be needed so let's not go there for now and see if this help
    // R.
    payloadStart = emm + 4 + (numAddrs * address_len);
    payloadSize  = emmPktLen - 4 -(numAddrs * address_len) - 1;
    ControllerLog("NDS emm payloadSize = %d\n",payloadSize);
    
    
    //
    // process multi block emm or card data from emm with ird+card payload
    //
    memcpy(addresses, emm + 4, numAddrs * 4);

    if( getEmmDebug() == TRUE ) {
        ControllerLog("NDS EMM-G, Multipart EMM or IRD-EMM with card payload\n");
    }

    [emmBuffer setLength:0 ];
    assembled = FALSE;
    word_block = payloadStart[0] & 0x01;    // we might be off by 1 here, to be verified with an actual emm-g
    payloadStart++;
    payloadSize--;

    for(i=0; i<numAddrs && payloadSize>0; i++) {
        if(word_block) {
            block_len=(((payloadStart[0] & 0x3F)<<8) | payloadStart[1]) << 1;
            payloadStart+=2;
            payloadSize-=2;
        }
        else {
            block_len=(payloadStart[0] & 0x3F) << 1;
            payloadStart++;
            payloadSize--;
        }
        if(block_len>0 && block_len<=payloadSize) {
            assembled=TRUE;
            data=[[NSMutableData alloc] init];
            [data setLength:payloadSize+4];
            pEmmData = (unsigned char *)[data mutableBytes];
            pEmmData[0] = emm[0];
            pEmmData[1] = 0x70;
            pEmmData[2] = (address_len + block_len + 1) & 0xFF;
            pEmmData[3] = emm[3]&0x0CF;
            memcpy(pEmmData+4,             addresses[i-numAddrs], address_len); // Address
            memcpy(pEmmData+4+address_len, payloadStart,         block_len);   // Data
            [emmBuffer appendBytes:pEmmData length:4+address_len+block_len];
            
            payloadStart+=block_len;
            payloadSize-=block_len;
            [data release];

        }
    }
    if(assembled)
        return emmStateReadyUseParams;
    else
        return emmStateNone;

}

static emmDataState processNagraEmm(unsigned char *emm, emmParams *params, NSMutableData *emmBuffer, emmDataState curState)
{
    
    unsigned char *serial = [params getCardSerial]+4;
    unsigned char swappedSerial[4];
    unsigned int emmPktLen = emm[9]+2;

    // EMM-G
    if( emm[0] == 0x82 || emm[0] == 0x84) {
        if( getEmmDebug() == TRUE ) {
            ControllerLog("Nagra EMM-G\n");
        }
        return emmStateReady;
    }
    // EMM-S and EMM-U
    else if( emm[0] == 0x83 ) {
        if( getEmmDebug() == TRUE ) {
            if(emm[7]==0x10)
                ControllerLog("Nagra EMM-S\n");
            else
                ControllerLog("Nagra EMM-U\n");
            ControllerLog("Nagra emm packet :\n");
            ControllerDump( [NSData dataWithBytes:emm length:emmPktLen] );
        }
        swappedSerial[0]=serial[2];
        swappedSerial[1]=serial[1];
        swappedSerial[2]=serial[0];
        swappedSerial[3]=serial[3];

        if( getEmmDebug() == TRUE ) {
            ControllerLog("[processNagraEmm] swappedSerial=%02X %02X %02X %02X \n",swappedSerial[0],swappedSerial[1],swappedSerial[2],swappedSerial[3]);
            ControllerLog("[processNagraEmm] emm serial=%02X %02X %02X %02X \n",emm[3],emm[4],emm[5],emm[6]);
        }
        
        if(memcmp(&emm[3],swappedSerial,(emm[7]==0x10)?3:4)==0)
            return emmStateReady;   
    }

    return emmStateNone;
}

static emmDataState processBetacryptEmm(unsigned char *emm, emmParams *params, NSMutableData *emmBuffer, emmDataState curState)
{
    return emmStateNone;
}

static emmDataState processSecaEmm(unsigned char *emm, emmParams *params, NSMutableData *emmBuffer, emmDataState curState)
{
    unsigned char *sa = [params getProviderData]+5; // seca SA is 4 byte, of which only 3 byte are used
    unsigned char *emmSA = emm+5;    
    
    unsigned char *serial = [params getCardSerial]+2;
    unsigned char *emmUA= emm+3;
    
    switch (emm[0]) {
        case 0x82:
        {            
            if(memcmp(serial,emmUA,6)==0) {
                if( getEmmDebug() == TRUE ) {
                    
                    ControllerLog("[unique] Card serial:\n");
                    ControllerDump( [NSData dataWithBytes:serial length:6 ] );
                    ControllerLog("[unique] EMM UA data:\n");
                    ControllerDump( [NSData dataWithBytes:emmUA length:6] );
                }
                return emmStateReady;
            }
            break;
        }
            
        case 0x83:
        case 0x84:
        {
            if (memcmp(sa,emmSA,3)==0) {
                if( getEmmDebug() == TRUE ) {
                    ControllerLog("[shared] Card SA :\n");
                    ControllerDump( [NSData dataWithBytes:sa length:3 ] );
                    ControllerLog("[shared] EMM SA data:\n");
                    ControllerDump( [NSData dataWithBytes:emmSA length:3] );
                }
                return emmStateReady;
            }            
            break;
        }
            
    }
    return emmStateNone;
}




emmDataState processEmmData(unsigned char *emmPacket, emmParams *params, NSMutableData *emmBuffer, emmDataState currentState)
{
    switch( [params getCaid] & 0xff00 )
    {
        case VIACCESS_CA_SYSTEM:
            return processViaccessEmm(emmPacket, params, emmBuffer, currentState);
            break;
            
        case IRDETO_CA_SYSTEM:
            return processIrdetoEmm(emmPacket, params, emmBuffer, currentState);
            
        case CONAX_CA_SYSTEM:
            return processConaxEmm(emmPacket, params, emmBuffer, currentState);
            
        case CRYPTOWORKS_CA_SYSTEM:
            return processCryptoworksEmm(emmPacket, params, emmBuffer, currentState);
            
        case SECA_CA_SYSTEM:
            return processSecaEmm(emmPacket, params, emmBuffer, currentState);
            
        case NDS_CA_SYSTEM:
            return processNDSEmm(emmPacket, params, emmBuffer, currentState);
            
        case NAGRA_CA_SYSTEM:
            return processNagraEmm(emmPacket, params, emmBuffer, currentState);
            
        case BETA_CA_SYSTEM: // betacrypt and irdeto have the same emm processing
            return processIrdetoEmm(emmPacket, params, emmBuffer, currentState);
            
        default:
            ControllerLog("[processEmmData] Unknown CAID\n");
            break;
    }
    return emmStateNone;
}

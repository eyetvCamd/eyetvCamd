#import "SrvController.h"
#import "camd3client.h"
#import "ncdClient.h"
#include "globals.h"
#include "vdrScCompat.h"
#include "vdr/sc/system.h"
#include "vdr/sc/log.h"
#include "vdr/sources.h"
#include <openssl/md5.h>

@implementation SrvController

- (IBAction)insertFilterForServer:(id)sender
{
  int row = [srvListView selectedRow];
  if( row != -1 )
  {
    uniproto *obj = [dSource objectAtIndex:row];
    caFilterEntry *ca = [[caFilterEntry alloc] initWithCaid:0 Ident:0];
    [obj addFilterEntry:ca];
    [ca release];
    [filerListView reloadData];
  }
}

- (IBAction)insertServer:(id)sender
{
  uniproto *obj = [[uniproto alloc] initWithProto:@"camd3" User:@"" Password:@"" Host:@"" Port:@"" NcdKey:@""];
  uniproto *srv = [[camd3Client alloc] initWithProto:@"camd3" User:@"" Password:@"" Host:@"" Port:@"" NcdKey:@""]; 
  [dSource addObject:obj];
  [csList addObject:srv];
  [obj release];
  [srv release];
  [srvListView reloadData];
}

- (IBAction)removeFilterForServer:(id)sender
{
  int srvrow = [srvListView selectedRow];
  int fltrow = [filerListView selectedRow];
  if( srvrow != -1 && fltrow != -1 )
  {
    uniproto *obj = [dSource objectAtIndex:srvrow];
    uniproto *server = [csList objectAtIndex:srvrow];
    [obj removeFilterAtIndex:fltrow];
    [server removeFilterAtIndex:fltrow];
    [filerListView reloadData];
  }
}

- (IBAction)removeServer:(id)sender
{
  int srvrow = [srvListView selectedRow];
  if( srvrow != -1 )
  {
    [csList removeObjectAtIndex:srvrow];
    [dSource removeObjectAtIndex:srvrow];
    [srvListView reloadData];
  }
}

- (IBAction)saveServerList:(id)sender
{
  //encode and save list
  NSMutableArray *file = [[NSMutableArray alloc] init];
  NSEnumerator *iter = [dSource objectEnumerator];
  id enObject;
  while( (enObject = [iter nextObject]) != 0 )
  {
    NSMutableArray *caFilter = [[NSMutableArray alloc] init];
    for(int index = 0; index < [enObject getFilterCount]; index++)
    {
      NSMutableDictionary *flt = [[NSMutableDictionary alloc] init];
      [flt setObject:[NSString stringWithFormat:@"%04x", [enObject getCaidAtIndex:index]] forKey:@"caid"];
      [flt setObject:[NSString stringWithFormat:@"%06x", [enObject getIdentAtIndex:index]] forKey:@"ident"];
      [caFilter addObject:flt];
      [flt release];
    }
    NSMutableDictionary *record = [[NSMutableDictionary alloc] init];
    [record setObject:[enObject getProto] forKey:@"proto"];
    [record setObject:[enObject getUser] forKey:@"user"];
    [record setObject:[enObject getPassword] forKey:@"password"];
    [record setObject:[enObject getHost] forKey:@"host"];
    [record setObject:[enObject getPort] forKey:@"port"];
    [record setObject:[enObject getKey] forKey:@"ncdkey"];
    if( [enObject isEnabled] == YES )
    {
      [record setObject:@"YES" forKey:@"enable"];
    }
    else
    {
      [record setObject:@"NO" forKey:@"enable"];
    }
    [record setObject:caFilter forKey:@"filter"];
    [file addObject:record];
    [record release];
    [caFilter release];
  }
  if( [file writeToFile:[[docPath stringByExpandingTildeInPath] stringByAppendingPathComponent:serversFile] atomically:NO] != YES )
  {
    NSFileManager *dir = [NSFileManager defaultManager];
    [dir createDirectoryAtPath:[docPath stringByExpandingTildeInPath] attributes:nil];
    [file writeToFile:[[docPath stringByExpandingTildeInPath] stringByAppendingPathComponent:serversFile] atomically:NO];
  }
  [file release];
}

- (IBAction)enableOrDisableEmu:(id)sender
{
  bool action = (enableEmu == YES)? NO:YES;
  enableEmu = action;
  if( enableEmu == NO )
  {
    [sender setTitle:@"Enable EMU"];
  }
  else
  {
    [sender setTitle:@"Disable EMU"];
  }
}

- (void)enableEmu:(bool)action
{
  enableEmu = action;
}

- (bool)getEnableEmu
{
  return enableEmu;
}

- (int)numberOfRowsInTableView:(NSTableView *)tableView
{
  int retval = 0;
  if( tableView == srvListView )
  {
    retval = [dSource count];
  }
  else if( tableView == filerListView )
  {
    if( [dSource count] != 0 )
    {
      int row = [srvListView selectedRow];
      if( row != -1 )
      {
	retval = [[dSource objectAtIndex:row] getFilterCount];
      }
    }
  }
  return retval;
}

- (id)tableView:(NSTableView *)tableView
      objectValueForTableColumn:(NSTableColumn *)tableColumn
	    row:(int)row
{
  if( tableView == srvListView )
  {
    uniproto *obj = [dSource objectAtIndex:row];
    NSString *column = [tableColumn identifier];
    if( [column isEqualToString:@"proto"] )
    {
      return [NSString stringWithString:[obj getProto]];
    }
    else if( [column isEqualToString:@"user"] )
    {
      return [NSString stringWithString:[obj getUser]];
    }
    else if( [column isEqualToString:@"pass"] )
    {
      return [NSString stringWithString:[obj getPassword]];
    }
    else if( [column isEqualToString:@"host"] )
    {
      return [NSString stringWithString:[obj getHost]];
    }
    else if( [column isEqualToString:@"port"] )
    {
      return [NSString stringWithString:[obj getPort]];
    }
    else if( [column isEqualToString:@"ncdkey"] )
    {
      return [NSString stringWithString:[obj getKey]];
    }
    else if( [column isEqualToString:@"ena"] )
    {
      return [NSNumber numberWithBool:[obj isEnabled]];
    }
  }
  if( tableView == filerListView )
  {
    int srvrow = [srvListView selectedRow];
    if( row != -1 && srvrow != -1 )
    {
      uniproto *obj = [dSource objectAtIndex:srvrow];
      NSString *column = [tableColumn identifier];
      if( [obj getFilterCount] > row )
      {
	if( [column isEqualToString:@"caid"] )
	{
	  return [NSString stringWithFormat:@"%04x", [obj getCaidAtIndex:row]];
	}
	if( [column isEqualToString:@"ident"] )
	{
	  return [NSString stringWithFormat:@"%06x", [obj getIdentAtIndex:row]];
	}
      }
    }
  }
  return @"";
}

- (void)tableView:(NSTableView *)tableView 
      setObjectValue:(id)anObject 
	forTableColumn:(NSTableColumn *)tableColumn 
	      row:(int)row
{
  if( tableView == srvListView && row != -1 )
  {
    uniproto *obj = [dSource objectAtIndex:row];
    uniproto *server = [csList objectAtIndex:row];
    NSString *column = [tableColumn identifier];
    if( [column isEqualToString:@"proto"] )
    {
      [obj updateProto:anObject];
    }
    else if( [column isEqualToString:@"user"] )
    {
      [obj updateUser:anObject];
    }
    else if( [column isEqualToString:@"pass"] )
    {
      [obj updatePassword:anObject];
    }
    else if( [column isEqualToString:@"host"] )
    {
      [obj updateHost:anObject];
    }
    else if( [column isEqualToString:@"port"] )
    {
      [obj updatePort:anObject];
    }
    else if( [column isEqualToString:@"ncdkey"] )
    {
      [obj updateKey:anObject];
    }
    else if( [column isEqualToString:@"ena"] )
    {
      bool doEnable = [anObject boolValue];
      if( memcmp([obj getSignature], [server getSignature], 16) != 0 )
      {
	uniproto *srv = nil;
	if( [[obj getProto] isEqualToString:@"camd3"] )
	{
	  srv = [[camd3Client alloc] initWithProto:@"camd3" 
					     User:[obj getUser] 
					 Password:[obj getPassword]
					     Host:[obj getHost] 
					     Port:[obj getPort]
					   NcdKey:[obj getKey] ];
	}
	if( [[obj getProto] isEqualToString:@"cd3tcp"] )
	{
	  srv = [[camd3Client alloc] initWithProto:@"cd3tcp" 
					      User:[obj getUser] 
					  Password:[obj getPassword]
					      Host:[obj getHost] 
					      Port:[obj getPort]
					    NcdKey:[obj getKey] ];
	}
	if( [[obj getProto] isEqualToString:@"ncd525"] )
	{
	  srv = [[ncdClient alloc] initWithProto:@"ncd525" 
					      User:[obj getUser] 
					  Password:[obj getPassword]
					      Host:[obj getHost] 
					      Port:[obj getPort]
					    NcdKey:[obj getKey] ];
	}
	if( srv != nil )
	{
	  for(int i = 0; i < [obj getFilterCount]; i++)
	  {
	    [srv addFilterEntry:[obj getFilterEntryAtIndex:i]];
	  }
	  [srv setDelegate:self];
	  [csList replaceObjectAtIndex:row withObject:srv];
	  [srv release];
	}
      }
      [obj enable:doEnable];
      [[csList objectAtIndex:row] enable:doEnable];
    }
  }
  if( tableView == filerListView && row != -1 )
  {
    int srvrow = [srvListView selectedRow];
    if( row != -1 && srvrow != -1 )
    {
      uniproto *obj = [dSource objectAtIndex:srvrow];
      uniproto *server = [csList objectAtIndex:srvrow];
      NSString *column = [tableColumn identifier];
      if( [column isEqualToString:@"caid"] )
      {
	int caid = strtol([anObject cString], 0, 16);
	[obj updateCaid:caid atIndex:row];
      }
      if( [column isEqualToString:@"ident"] )
      {
	int ident = strtol([anObject cString], 0, 16);
	[obj updateIdent:ident atIndex:row];
      }
      if( memcmp([obj getFilterSignature], [server getFilterSignature], 16) != 0 )
      {
	for(int i = 0; i < [server getFilterCount]; i++)
	{
	  [server removeFilterAtIndex:i];
	}
	for(int i = 0; i < [obj getFilterCount]; i++)
	{
	  [server addFilterEntry:[obj getFilterEntryAtIndex:i]];
	}
      }
    }
  }
}

- (void)tableViewSelectionDidChange:(NSNotification *)notification
{
  [filerListView reloadData];
}


- (void)writeDwToDescrambler:(unsigned char *)dw caDesc:(caDescriptor *)ca sid:(unsigned long)sid
{
  if ([delegateObj respondsToSelector:@selector(writeDwToDescrambler:caDesc:sid:)])
  {
    if( getShowCwDw() == YES )
    {
      NSData *DW = [[NSData alloc] initWithBytes:dw length:16];
      ControllerLog("Receive DW:\n");
      ControllerDump(DW);
      [DW release];
    }
    [delegateObj writeDwToDescrambler:dw caDesc:ca sid:sid];
  }
}

- (void)emmAddParams:(unsigned char *)serial provData:(unsigned char *)bytes caid:(unsigned int)casys ident:(unsigned int)provid
{
  if ([delegateObj respondsToSelector:@selector(emmAddParams:provData:caid:ident:)])
  {
    [delegateObj emmAddParams:serial provData:bytes caid:casys ident:provid];
  }
}

- (void)emmRmParams:(unsigned char *)serial provData:(unsigned char *)bytes caid:(unsigned int)casys ident:(unsigned int)provid
{
  if ([delegateObj respondsToSelector:@selector(emmAddParams:provData:caid:ident:)])
  {
    [delegateObj emmRmParams:serial provData:bytes caid:casys ident:provid];
  }
}

- (NSDragOperation) tableView: (NSTableView *) view
                    validateDrop: (id <NSDraggingInfo>) info
                    proposedRow: (int) row
                    proposedDropOperation: (NSTableViewDropOperation) operation
{
  int column = [view columnAtPoint:[info draggingLocation]];
  if( view == srvListView && column != -1 && column != 0 && column != 6 )
  {
    return NSDragOperationCopy;
  }
  return NSDragOperationNone;
}

- (BOOL) tableView: (NSTableView *) view
         acceptDrop: (id <NSDraggingInfo>) info
         row: (int) row
         dropOperation: (NSTableViewDropOperation) operation
{
  int column = [view columnAtPoint:[info draggingLocation]];
  if( view == srvListView && column != -1 && column != 0 && column != 6 )
  {
    return YES;
  }
  return NO;
}

- (BOOL) tableView: (NSTableView *) view
         writeRows: (NSArray *) rows
         toPasteboard: (NSPasteboard *) pboard
{
  if( view == srvListView )
  {
//    [view mouseU]
    return YES;
  }
  return NO;
}

- (void)setDelegateForController:(id)obj;
{
  delegateObj = obj;
  [srvListView registerForDraggedTypes: [NSArray arrayWithObjects: NSStringPboardType, nil]];
}

- (bool)hasCasys:(unsigned int)Casys Ident:(unsigned int)Ident
{
  NSEnumerator *iter = [csList objectEnumerator];
  id sender;
  while( sender = [iter nextObject] )
  {
    if( [sender isEnabled] == YES && [sender isAllowdCaid:Casys Ident:Ident] == YES)
    {
	  return true;
    }
  }
  
  return false;
}

- (void)sendEcmPacket:(NSData *)Packet Cadesc:(caDescriptor *)desc Ssid:(unsigned int)ssid  devIndex:(unsigned int)index
{
  NSData *ecmPacket = nil;
  unsigned int casysBase = [desc getCasys] & 0xff00;
  if( casysBase == 0x600 ) // Irdeto ECM
  {
    ecmPacket = [irdCtl getEcm:Packet dev:index];
  }
  else
  {
    ecmPacket = Packet;
  }
  if(ecmPacket != nil && index >= 0 && index < 16)
  {
    unsigned char parity;
    [ecmPacket getBytes:&parity length:1];
    if( ecmParity[index] != parity )
    {
      ecmParity[index] = parity;
      if( getShowCwDw() == YES )
      {
	ControllerLog("Send ECM:\n");
	ControllerDump(ecmPacket);
      }
    }
  }

  NSEnumerator *iter = [csList objectEnumerator];
  id sender;
  while( sender = [iter nextObject] )
  {
    if( [sender isEnabled] == YES && [sender isAllowdCaid:[desc getCasys] Ident:[desc getIdent]] == YES)
    {
      if( ecmPacket != nil ) [sender sendEcmPacket:ecmPacket Cadesc:desc Ssid:ssid devIndex:index];
    }
  }

	if( ecmPacket != nil && enableEmu == YES )
	{
		static unsigned char emuLastSign[11][16]; //HACK: max 11 devices...
		unsigned char ecmSign[16];
		unsigned char *buffer = (unsigned char *)[Packet bytes];
		int len = [Packet length]; 
		MD5(buffer + 3, len - 3, ecmSign);
		if( memcmp(ecmSign, emuLastSign[index], 16) != 0 )
		{
			memcpy(emuLastSign[index], ecmSign, 16);
			cEcmInfo ecmD("dummy", 0x123, [desc getCasys], [desc getIdent]);
			ecmD.SetSource(10, cSource::stSat, 120);
			cSystem *sys = 0;
			int lastPri = 0 ;
			while( (sys = cSystems::FindBySysId([desc getCasys], false, lastPri)) != 0 )
			{
				lastPri=sys->Pri();
				if( sys->ProcessECM(&ecmD, (unsigned char *)[Packet bytes]) != false )
				{
					[self writeDwToDescrambler:sys->CW() caDesc:desc sid:ssid];
					delete sys;
					return;
				}
				delete sys;
			}
		}
	}
}

- (void)sendEmmPacket:(NSData *)Packet Params:(emmParams *)params
{
  NSEnumerator *iter = [csList objectEnumerator];
  id sender;
  while( sender = [iter nextObject] )
  {
    if( [sender isEnabled] == YES /*&& [sender isEmmEnabled] == YES*/)
    {
      [sender sendEmmPacket:Packet Params:params];
    }
  }
}

- (void)loadServerConfigAndCache
{
  NSArray *file = [[NSArray alloc] initWithContentsOfFile:[[docPath stringByExpandingTildeInPath] stringByAppendingPathComponent:serversFile]];
  if( file != nil)
  {
    NSEnumerator *iter = [file objectEnumerator];
    id enObject;
    while( (enObject = [iter nextObject]) != nil )
    {
      uniproto *uobj = [[uniproto alloc] initWithProto:[enObject objectForKey:@"proto"] 
						  User:[enObject objectForKey:@"user"]
					      Password:[enObject objectForKey:@"password"]
						  Host:[enObject objectForKey:@"host"]
						  Port:[enObject objectForKey:@"port"]
						NcdKey:[enObject objectForKey:@"ncdkey"] ];
      BOOL doEnable = NO;
      if( [[enObject objectForKey:@"enable"] isEqualToString:@"YES"] )
      {
	doEnable = YES;
      }
      [uobj enable:doEnable];
      NSArray *caFilter = [enObject objectForKey:@"filter"];
      if( [caFilter count] != 0 )
      {
	NSEnumerator *caIter = [caFilter objectEnumerator];
	id caEntry;
	while( (caEntry = [caIter nextObject]) != 0 )
	{
	  int caid = strtol([[caEntry objectForKey:@"caid"] cString], 0, 16);
	  int ident = strtol([[caEntry objectForKey:@"ident"] cString], 0, 16);
	  caFilterEntry *ca = [[caFilterEntry alloc] initWithCaid:caid Ident:ident];
	  [uobj addFilterEntry:ca];
	  [ca release];
	}
      }
      [dSource addObject:uobj];
      uniproto *srv = nil;
      if( [[uobj getProto] isEqualToString:@"camd3"] )
      {
	srv = [[camd3Client alloc] initWithProto:@"camd3" 
					    User:[uobj getUser] 
					Password:[uobj getPassword]
					    Host:[uobj getHost] 
					    Port:[uobj getPort]
					  NcdKey:[uobj getKey] ];
      }
      
      if( [[uobj getProto] isEqualToString:@"cd3tcp"] )
      {
	srv = [[camd3Client alloc] initWithProto:@"cd3tcp" 
					    User:[uobj getUser] 
					Password:[uobj getPassword]
					    Host:[uobj getHost] 
					    Port:[uobj getPort]
					  NcdKey:[uobj getKey] ];
      }

      if( [[uobj getProto] isEqualToString:@"ncd525"] )
      {
	srv = [[ncdClient alloc] initWithProto:@"ncd525" 
					  User:[uobj getUser] 
				      Password:[uobj getPassword]
					  Host:[uobj getHost] 
					  Port:[uobj getPort]
					NcdKey:[uobj getKey] ];
      }
      
      if( srv != nil )
      {
	for(int i = 0; i < [uobj getFilterCount]; i++)
	{
	  [srv addFilterEntry:[uobj getFilterEntryAtIndex:i]];
	}
	[srv enable:doEnable];
	[srv setDelegate:self];
	[csList addObject:srv];
	[srv release];
      }
      [uobj release];
    }
    [file release];
  }
}

- (id) init
{
  self = [super init];
  if (self != nil)
  {
    dSource = [[NSMutableArray alloc] init];
    csList = [[NSMutableArray alloc] init];
    docPath = [[NSString alloc] initWithCString:"~/Documents/eyetvCamd"];
    serversFile = [[NSString alloc] initWithCString:"servers.plist"];
    cacacheFile = [[NSString alloc] initWithCString:"cacache.plist"];
  }
  return self;
}

- (void)initEmu
{
  InitAll([[docPath stringByExpandingTildeInPath] cStringUsingEncoding:NSASCIIStringEncoding]);
  LogAll();
  cLogging::SetModuleOption(LCLASS(7,0x20<<2),false); // Nagra L_SYS_DISASM
  cLogging::SetModuleOption(LCLASS(7,0x20<<4),false); // Nagra L_SYS_CPUSTATS
  int state = (enableEmu == NO)? NSOffState:NSOnState;
  [emuBtn setState:state];
  if( enableEmu == NO )
  {
    [emuBtn setTitle:@"Enable EMU"];
  }
  else
  {
    [emuBtn setTitle:@"Disable EMU"];
  }
}

- (void) dealloc 
{
  [serversFile release];
  [cacacheFile release];
  [docPath release];
  [csList removeAllObjects];
  [dSource removeAllObjects];
  [dSource release];
  [csList release];
  [super dealloc];
}


@end

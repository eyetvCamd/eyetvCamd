#import "Controller.h"
#include "messages.h"
#include "dvbheader.h"
#include "globals.h"
#include "aes1.h"

Controller *ctrl = nil;
NSLock *logLocker = nil;
NSLock *dumpLocker = nil;

void ControllerLog(const char *format, ...)
{
	if ( logLocker == nil ) return;
	[logLocker lock];
	if( ctrl != nil )
	{
		va_list ap;
		va_start(ap, format);
		NSString *str = [[NSString alloc] initWithFormat:[NSString stringWithCString:format encoding:NSASCIIStringEncoding] arguments:ap];
		[ctrl textToLog:str];
		[str release];
		va_end(ap);
	}
	[logLocker unlock];
}

void ControllerDump(NSData *buf)
{
	if ( dumpLocker == nil ) return;
	[dumpLocker lock];
	if( ctrl != nil )
	{
		NSMutableString *dumpStr = [[NSMutableString alloc] init];
		unsigned char *bufBytes = (unsigned char *)[buf bytes];
		for( int i = 0; i < [buf length]; i++ )
		{
			int offset = (i % 16);
			
			if( offset == 0 )
			{
				if(i != 0) [dumpStr appendString:@"\n"];
				[dumpStr appendFormat:@"%04x: ", i];
			}
			else if ( offset == 4 || offset == 8 || offset == 12 )
			{
				[dumpStr appendString:@"  "];
			}
			[dumpStr appendFormat:@"%02x ", (bufBytes[i] & 0xff) ];
		}
		
		if( [dumpStr length] > 0 )
		{
			ControllerLog("%s\n", [dumpStr cStringUsingEncoding:NSASCIIStringEncoding]);
		}
		[dumpStr release];
	}
	[dumpLocker unlock];
}

void ControllerDump(unsigned char *buf, int len)
{
	if ( logLocker == nil ) return;
	NSData *pData = [[NSData alloc] initWithBytesNoCopy:buf length:len freeWhenDone:NO];
	ControllerDump(pData);
	[pData release];
}

@implementation Controller
- (void)textToLog:(NSString *)logtext
{
	float doScroll = [[[gdbView enclosingScrollView] verticalScroller] floatValue];
	[gdbView replaceCharactersInRange:NSMakeRange([[gdbView textStorage] length], 0) withString:logtext];
	if( doScroll == 1.0 )
	{
		[gdbView scrollRangeToVisible:NSMakeRange([[gdbView textStorage] length], 0)];
	}
}

- (void)decodePluginMessage:(NSArray *)msgData
{
	for(int ac = 0; ac < [msgData count]; ac++)
	{ 
		if( [[msgData objectAtIndex:ac] count]!= 2 )
			continue;
		NSData *pmsg = [[msgData objectAtIndex:ac] objectAtIndex:0];
		NSNumber *devIndex = [[msgData objectAtIndex:ac] objectAtIndex:1];
		int idx = 0;
		if( devIndex != nil )
		{
			idx = [devIndex intValue];
			if(idx < 0 || idx > 11)
			{
				return;
			}
		}
		devCtrl *pDev = &devs[idx];
		if( pmsg != nil )
		{
			unsigned char *tsPacket = (unsigned char *)[pmsg bytes]; 
			TSHeader rcv;
			rcv.word = ntohl(*(unsigned int *)tsPacket); 
			
			switch( rcv.word )
			{
				case msg_new_channel:
				{
					[pDev->pmtSet clearList];
					if( selectedDevice == idx )
					{
						pDev->selected = -1;
						[caDescList reloadData];
						[irdCtl clearValid:idx];
					}
					msgNewChannel *pch = (msgNewChannel *)[pmsg bytes];
					unsigned long newTransponderId = ntohl(pch->mTransponder);
					if( newTransponderId != pDev->curTransponderId )
					{
						[pDev->sCAT reset];
						msgPid filterPid;
						filterPid.id = msg_add_pid;
						filterPid.mPid = htonl(1); // CAT pid is always 1
						NSMutableData *pmsg = [[NSMutableData alloc] init];
						[pmsg appendBytes:&filterPid length:sizeof(filterPid)];
						[self sendData:pmsg dev:idx];
						[pmsg release];
					}
					pDev->curTransponderId = newTransponderId;
					pDev->curServiceId = ntohl(pch->mService);
					pDev->curPmtPid = ntohl(pch->mPmt);
					[pDev->sPMT reset];
					[pDev->sECM reset];
					msgPid filterPid;
					filterPid.id = msg_add_pid;
					filterPid.mPid = htonl(pDev->curPmtPid);
					NSMutableData *pmsg = [[NSMutableData alloc] init];
					[pmsg appendBytes:&filterPid length:sizeof(filterPid)];
					[self sendData:pmsg dev:idx];
					[pmsg release];
				} break;
				case msg_initialized:
					pDev->curTransponderId = 0;
					pDev->curServiceId = 0;
					pDev->curPmtPid = 0;
					pDev->curEcmPid = 0;
					break;
				case msg_termitate:
					pDev->curTransponderId = 0;
					pDev->curServiceId = 0;
					pDev->curPmtPid = 0;
					pDev->curEcmPid = 0;
					break;
					
				default:
				{
					if( rcv.ts.syncByte == 0x47 && rcv.ts.transportErr == 0 ) // DVB Packet w/o ts error
					{
						unsigned int pid = rcv.ts.pid;
						if( pid == pDev->curPmtPid )
						{
							if( [pDev->sPMT toStream:tsPacket] == statePayloadFull )
							{
								do
								{
									unsigned char *pmtPacket = (unsigned char *)[pDev->sPMT getBuffer];
									if( pmtPacket[0] == 0x2 ) // PMT Table ID
									{
										unsigned int pmtLen = (((pmtPacket[1] & 0xf) << 8) | (pmtPacket[2] & 0xff)) + 3;
										unsigned int pmtSid = (pmtPacket[3] << 8) | pmtPacket[4];
										if( pmtLen > 16 && pmtSid == pDev->curServiceId )
										{
											if( getShowCwDw() == YES )
											{
												ControllerLog("Received PMT:\n");
												ControllerDump([pDev->sPMT getData]);
											}
											[pDev->pmtSet parsePmtPayload:[pDev->sPMT getData]];
											caDescriptor *ca = [[caDescriptor alloc] initStaticWithEcmpid:0 casys:0 ident:0];
											int msgid = (pDev->curPmtPid << 16) | (pDev->curServiceId & 0xffff);
											[ca setMessageId:msgid];
											caDescriptor *found = [caCache member:ca];
											pDev->selected = 0;
											
											if ((found == nil) && (srvListCtl != nil))
											{
												NSArray *List = [pDev->pmtSet getCaDescriptors];
												unsigned int caCount = [List count];
												if( caCount > 0 )
												{
													for( int i = 0; i < caCount; i++ )
													{
														caDescriptor *desc = [List objectAtIndex:i];
														if ([srvListCtl hasCasys:[desc getCasys] Ident:[desc getIdent]])
														{
															found = desc;
															break;
														}
													}
												}
											}
											
											if( found != nil )
											{
												[ca setMessageId:0];
												[ca setEcmpid:[found getEcmpid] casys:[found getCasys] ident:[found getIdent]];
												[irdCtl setIrdetoChannel:[found getIrdetoChannel] forDev:idx];
												pDev->selected = [[pDev->pmtSet getCaDescriptors] indexOfObject:ca];
												if( pDev->selected == NSNotFound || pDev->selected >= [pDev->pmtSet caDescCount] )
												{
													pDev->selected = 0;
												}
												else
												{
													pDev->curEcmPid = [ca getEcmpid];
													if( selectedDevice != idx )
													{
														msgPid filterPid;
														filterPid.id = msg_add_pid;
														filterPid.mPid = htonl(pDev->curEcmPid);
														NSMutableData *pmsg = [[NSMutableData alloc] init];
														[pmsg appendBytes:&filterPid length:sizeof(filterPid)];
														[self sendData:pmsg dev:idx];
														[pmsg release];
													}
												}
											}
											if( selectedDevice == idx )
											{
												pmtChanged = YES;
												[caDescList reloadData];
												[caDescList selectRowIndexes:[NSIndexSet indexSetWithIndex:pDev->selected] byExtendingSelection:NO]; 
												pmtChanged = NO;
											}
											[ca release];
											[self clearAllEmm:idx];
											unsigned int emmCaCount = [pDev->catSet caDescCount];
											if( emmCaCount != 0 )
											{
												caDescriptor *desc = [[pDev->pmtSet getCaDescriptors] objectAtIndex:pDev->selected];
												NSArray *List = [pDev->catSet getCaDescriptors];
												for( int i = 0; i < emmCaCount; i++ )
												{
													id obj = [List objectAtIndex:i];
													if( [obj getCasys] == [desc getCasys] )
													{
														unsigned int emmPid = [obj getEcmpid];
														[self addEmmPid:emmPid toDevice:idx];
														if( getEmmEnable() == YES )
														{
															msgPid filterPid;
															filterPid.id = msg_add_pid;
															filterPid.mPid = htonl(emmPid);
															NSMutableData *pmsg = [[NSMutableData alloc] init];
															[pmsg appendBytes:&filterPid length:sizeof(filterPid)];
															[self sendData:pmsg dev:idx];
															ControllerLog("EMM Processing: caid:%x emmpid:%x\n",[desc getCasys], emmPid);
														}
													}
												}
											}
										}
										else
										{
											[pDev->sPMT reset];
										}
									}
								} while( [pDev->sPMT nextSection] == YES );
							}
						}
						else if( pid == pDev->curEcmPid )
						{
							if( [pDev->sECM toStream:tsPacket] == statePayloadFull )
							{
								int row = pDev->selected;
								int rowCount = [[pDev->pmtSet getCaDescriptors] count];
								decryptFlag dmode = DECRYPT_MODE_NONE;
								do
								{
									unsigned char *ecmPacket = [pDev->sECM getBuffer];
									if( ecmPacket[0] == 0x80 || ecmPacket[0] == 0x81 ) // ECM Table ID
									{
										unsigned int pLen = (((ecmPacket[1] & 0xf) << 8) | (ecmPacket[2] & 0xff)) + 3;
										NSData *showEcm = [[NSData alloc] initWithBytes:ecmPacket length:pLen];
										unsigned int ecmLen = (((ecmPacket[1] & 0xf) << 8) | (ecmPacket[2] & 0xff)) + 3;
										if( ecmLen <= [[pDev->sECM getData] length] )
										{
											NSData *pEcm = [[NSData alloc] initWithBytes:ecmPacket length:ecmLen];
											if( row !=  -1 && rowCount > row )
											{
												caDescriptor *desc = [[pDev->pmtSet getCaDescriptors] objectAtIndex:row];
												unsigned long ident = 0;
												unsigned int casysBase = [pDev->curCa getCasys] & 0xff00;
												if( casysBase == 0x1800 )
												{
													ident = [self getNagraIdent:pEcm caDesc:desc];
												}
												if( casysBase == 0x0d00 )
												{
													ident = [self getCworksIdent:pEcm caDesc:desc];
												}
												if( casysBase == 0x0600 )
												{
													
												}
												if( ident != 0 )
												{
													[desc setEcmpid:[desc getEcmpid] casys:[desc getCasys] ident:ident];
												}
												[pDev->curCa setEcmpid:[desc getEcmpid] casys:[desc getCasys] ident:[desc getIdent]];
												
												[desc setDmode:dmode];
												[pDev->curCa setDmode:dmode];
												[srvListCtl sendEcmPacket:pEcm Cadesc:desc Ssid:pDev->curServiceId devIndex:idx];
											}
											[pEcm release];
										}
										[showEcm release];
										
										if( getRawRecordState() == YES )
										{
											NSString *key = [[NSString alloc] initWithFormat:@"dev%dECMPid.0x%x",idx, [pDev->sECM getPid]];
											BOOL fileRelease = NO;
											NSOutputStream *file = [recordPids objectForKey:key];
											if( file == 0 )
											{
												NSString *path = [[[docPath stringByExpandingTildeInPath] stringByAppendingPathComponent:@"record"] 
																  stringByAppendingPathComponent:key];
												file = [[NSOutputStream alloc] initToFileAtPath:path append:YES];
												[file open];
												[recordPids setObject:file forKey:key];
												fileRelease = YES;
											}
											[file write:[pDev->sECM getBuffer] maxLength:[[pDev->sECM getData] length]];
											if( fileRelease == YES )
											{
												[file release];
											}
											[key release];
										}
									}
								} while( [pDev->sECM nextSection] == YES );
								[pDev->sECM reset];
							}
						}
						else if( pid == 1 )  // CAT received
						{
							if( [pDev->sCAT toStream:tsPacket] == statePayloadFull )
							{
								[pDev->catSet parseCATPayload:[pDev->sCAT getData]];
								[self clearAllEmm:idx];
								int emmCaCount = [pDev->catSet caDescCount];
								if( emmCaCount != 0 && [pDev->pmtSet caDescCount] != 0 )
								{
									caDescriptor *desc = [[pDev->pmtSet getCaDescriptors] objectAtIndex:pDev->selected];
									NSArray *List = [pDev->catSet getCaDescriptors];
									for( int i = 0; i < emmCaCount; i++ )
									{
										id obj = [List objectAtIndex:i];
										if( [obj getCasys] == [desc getCasys] )
										{
											unsigned int emmPid = [obj getEcmpid];
											[self addEmmPid:emmPid toDevice:idx];
											if( getEmmEnable() == YES )
											{
												msgPid filterPid;
												filterPid.id = msg_add_pid;
												filterPid.mPid = htonl(emmPid);
												NSMutableData *pmsg = [[NSMutableData alloc] init];
												[pmsg appendBytes:&filterPid length:sizeof(filterPid)];
												[self sendData:pmsg dev:idx];
												ControllerLog("EMM Processing: caid:%x emmpid:%x\n",[desc getCasys], emmPid);
											}
										}
									}
								}
							}
						}
						else // emm received
						{
							NSNumber *key = [[NSNumber alloc] initWithUnsignedInt:pid];
							section *sEmm = [pDev->emmSectionFilter objectForKey:key];
							NSNumber *emmStateRef = [pDev->emmState objectForKey:key];
							NSMutableData *emmAssembleBuffer = [pDev->emmBuffer objectForKey:key];
							if( getEmmEnable() == YES && sEmm != nil && emmStateRef != nil && emmAssembleBuffer != nil )
							{
								if( [sEmm toStream:tsPacket] == statePayloadFull )
								{
									do
									{
										unsigned int curEmmPid = [sEmm getPid];
										unsigned int curCaid = [[[pDev->pmtSet getCaDescriptors] objectAtIndex:pDev->selected] getCasys];
										NSEnumerator *emmDescriptors = [[pDev->catSet getCaDescriptors] objectEnumerator];
										caDescriptor *emmDesc = nil;
										while( ( emmDesc = [emmDescriptors nextObject] ) != nil )
										{	// we need to match both the emmPid and the curent CAID as some feed have 3 or more CAID using the same emmPid
											// if we only match on the emmPid we only get the 1st caid and it might not be the right one.
											if( [emmDesc getEcmpid] == curEmmPid && [emmDesc getCasys] == curCaid)
											{
												break;
											}
										}

										if( emmDesc != nil )
										{
											caDescriptor *ecmDesc = [[pDev->pmtSet getCaDescriptors] objectAtIndex:pDev->selected];
											NSEnumerator *emms = [emmReaders objectEnumerator];
											emmParams *paramsObj = nil;
											while( (paramsObj = [emms nextObject]) != 0 )
											{
												// test that the emm is for the current provider except for nagra.. where apaprently the emm
												// can be on a different provid than the one selected.
												if( ([paramsObj getCaid] == [emmDesc getCasys] && [paramsObj getIdent] == [ecmDesc getIdent]) ||
													([paramsObj getCaid] == [emmDesc getCasys] && ([paramsObj getCaid] & 0xff00) == NAGRA_CA_SYSTEM) )
												{
													if( ([paramsObj getCaid] & 0xff00) != IRDETO_CA_SYSTEM && ([paramsObj getCaid] & 0xff00) != BETA_CA_SYSTEM )
													{
														break;
													}
													else
													{
														unsigned char *provData = [paramsObj getProviderData];
														if( provData[5] != 0x1f && provData[6] != 0xff && provData[7] != 0xff )
														{
															break;
														}
													}
												}
											}

											if( paramsObj != nil )
											{
												emmDataState currentState = (emmDataState)[emmStateRef intValue];
												emmDataState emmBufferState = processEmmData([sEmm getBuffer], paramsObj, 
																							 emmAssembleBuffer, currentState);
												switch(emmBufferState)
												{
													case emmStateReady:
														[srvListCtl sendEmmPacket:[sEmm getData] Params:paramsObj];
														break;
													case emmStateReadyUseParams:
														[srvListCtl sendEmmPacket:emmAssembleBuffer Params:paramsObj];
														break;
												}
												if( currentState == via8cdReceived && emmBufferState == emmStateReady )
												{
													emmBufferState = via8cdReceived;
												}
												if( currentState == cryptoworks84Received && emmBufferState == emmStateReady )
												{
													emmBufferState = cryptoworks84Received;
												}
												NSNumber *newState = [[NSNumber alloc] initWithInt:(int)emmBufferState];
												[pDev->emmState setObject:newState forKey:key];
												[newState release];
											}
										}
										if( getRawRecordState() == YES )
										{
											NSString *fkey = [[NSString alloc] initWithFormat:@"dev%dEMMPid.0x%x",idx, [sEmm getPid]];
											BOOL fileRelease = NO;
											NSOutputStream *file = [recordPids objectForKey:fkey];
											if( file == 0 )
											{ 
												NSString *path = [[[docPath stringByExpandingTildeInPath] stringByAppendingPathComponent:@"record"] 
																  stringByAppendingPathComponent:fkey];
												file = [[NSOutputStream alloc] initToFileAtPath:path append:YES];
												[file open];
												[recordPids setObject:file forKey:fkey];
												fileRelease = YES;
											}
											[file write:[sEmm getBuffer] maxLength:[[sEmm getData] length]];
											if( fileRelease == YES )
											{
												[file release];
											}
											[fkey release];
										}
									} while( [sEmm nextSection] == YES );
								} 
								[sEmm reset];
								
								[key release];
							}
						}
					}
				}
			}
		}
	}
}

- (void)handlePortMessage:(NSPortMessage *)portMessage

{
	NSArray *msgData = [portMessage components];
	[self decodePluginMessage:msgData];
}

+ (void)dataThreadStart:(id)obj
{
	NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
	NSMessagePort *localPort = [[NSMessagePort alloc] init];
	[localPort setDelegate:obj];
	
	// Register the port using a specific name. The name must be unique.
	[[NSMessagePortNameServer sharedInstance] registerPort:localPort name:@"cwdwgwECMData"];
	
	// Configure the object and add it to the current run loop.
	[[NSRunLoop currentRunLoop] addPort:localPort forMode:NSDefaultRunLoopMode];
	[obj srvListLoadConfig];
	while(1)
	{
		[[NSRunLoop currentRunLoop] run];
	}
	[pool release];
}

- (void)srvListLoadConfig
{
	[srvListCtl loadServerConfigAndCache];
}

- (void)applicationDidFinishLaunching:(NSNotification *)notification
{
	[[NSDistributedNotificationCenter defaultCenter] addObserver:self
														selector:@selector(rcvNotifications:) 
															name:@"cwdwgwLogObserver"
														  object:nil];
#if !defined(USE_NSPORT_IPC)
	[[NSDistributedNotificationCenter defaultCenter] addObserver:self
														selector:@selector(rcvPidsAndControls:) 
															name:@"cwdwgwPIDandControlObserver"
														  object:nil];
#endif
	[irdCtl setDelegateObj:self];
	// send cwdwplug alive message
	typeOnlyMessage ca;
	ca.id = msg_camd_online;
	NSData *pmsg = [[NSData alloc] initWithBytes:&ca length:sizeof(ca)];
	for( int i = 0; i < NUM_DEVS; i++)
	{
		[self sendData:pmsg dev:i];
	}
	[pmsg release];
	[currentDevice addItemsWithTitles:[NSArray arrayWithObjects:@"device 0", @"device 1", @"device 2",
									   @"device 3", @"device 4", @"device 5", @"device 6", @"device 7",
									   @"device 8", @"device 9", @"device 10", @"device 11", nil]];
	[currentDevice setTitle:@"device 0"];
#if defined(USE_NSPORT_IPC)
	NSMessagePort *localPort = [[NSMessagePort alloc] init];
	[localPort setDelegate:self];
	
	// Register the port using a specific name. The name must be unique.
	[[NSMessagePortNameServer sharedInstance] registerPort:localPort name:@"cwdwgwECMData"];
	
	// Configure the object and add it to the current run loop.
	[[NSRunLoop currentRunLoop] addPort:localPort forMode:NSDefaultRunLoopMode];
#endif
	[self srvListLoadConfig];
	[srvListCtl enableEmu:NO];
	NSDictionary *gcfg = [[NSDictionary alloc] initWithContentsOfFile:[[docPath stringByExpandingTildeInPath]
																	   stringByAppendingPathComponent:configFile]];
	if( gcfg != nil )
	{
		NSNumber *emmflag = [gcfg objectForKey:@"enableEmm"];
		if( emmflag != nil )
		{
			bool action = [emmflag boolValue];
			setEmmEnable(action);
		}
		NSNumber *emuFlag = [gcfg objectForKey:@"enableEmulation"];
		if( emuFlag != nil )
		{
			bool action = [emuFlag boolValue];
			[srvListCtl enableEmu:action];
		}
		[gcfg release];
	}
	[srvListCtl initEmu];
	int state = (getEmmEnable() == NO)? NSOffState:NSOnState;
	[edEmmProcessing setState:state];
	if( getEmmEnable() == NO )
	{
		[edEmmProcessing setTitle:@"Enable EMM"];
	}
	else
	{
		[edEmmProcessing setTitle:@"Disable EMM"];
	}
	
}

- (int)numberOfRowsInTableView:(NSTableView *)tableView
{
	return [devs[selectedDevice].pmtSet caDescCount];
}

- (id)tableView:(NSTableView *)tableView
objectValueForTableColumn:(NSTableColumn *)tableColumn
			row:(int)row
{
	if( [[devs[selectedDevice].pmtSet getCaDescriptors] count] > row )
	{
		caDescriptor *desc = [[devs[selectedDevice].pmtSet getCaDescriptors] objectAtIndex:row];
		NSString *column = [tableColumn identifier];
		if( [column isEqualToString:@"caid"] )
		{
			return [NSString stringWithFormat:@"%04x", [desc getCasys]];
		}
		else if( [column isEqualToString:@"ident"] )
		{
			return [NSString stringWithFormat:@"%06x", [desc getIdent]];
		}
		else if( [column isEqualToString:@"ecmpid"] )
		{
			return [NSString stringWithFormat:@"%04x", [desc getEcmpid]];
		}
	}
	return @"";
}

- (void)irdetoChannelChange:(unsigned int)irdchn
{
	devCtrl *pDev = &devs[selectedDevice];
	int row = [caDescList selectedRow];
	int rowCount = [[pDev->pmtSet getCaDescriptors] count];
	if( row == -1 || rowCount <= row ) 
		return;
	caDescriptor *ca = [[caDescriptor alloc] initStaticWithEcmpid:0 casys:0 ident:0];
	unsigned int msgid = (pDev->curPmtPid << 16) | (pDev->curServiceId & 0xffff);
	[ca setMessageId:msgid];
	caDescriptor *found = [caCache member:ca];
	[ca release];
	if( found != nil )
	{
		[found setIrdetoChannel:irdchn];
	}
}

- (void)tableViewSelectionDidChange:(NSNotification *)notification
{
	int row = [caDescList selectedRow];
	int rowCount = [[devs[selectedDevice].pmtSet getCaDescriptors] count];
	if( row == -1 || rowCount <= row ) 
		return;
	if( row == devs[selectedDevice].selected && pmtChanged == NO ) 
	{
		return;
	}
	caDescriptor *prevDesc = [[devs[selectedDevice].pmtSet getCaDescriptors] objectAtIndex:devs[selectedDevice].selected];
	devs[selectedDevice].selected = row;
	caDescriptor *desc = [[devs[selectedDevice].pmtSet getCaDescriptors] objectAtIndex:row];
	if( desc != 0 )
	{
		devs[selectedDevice].curEcmPid = [desc getEcmpid];
		msgPid filterPid;
		filterPid.id = msg_add_pid;
		filterPid.mPid = htonl(devs[selectedDevice].curEcmPid);
		NSMutableData *pmsg = [[NSMutableData alloc] init];
		[pmsg appendBytes:&filterPid length:sizeof(filterPid)];
		[self sendData:pmsg dev:selectedDevice];
		if( doCaChange == YES )
		{
			[devs[selectedDevice].sECM reset];
			typeOnlyMessage ca;
			ca.id = msg_ca_change;
			[pmsg setLength:0];
			[pmsg appendBytes:&ca length:sizeof(ca)];
			[self sendData:pmsg dev:selectedDevice];
		}
		caDescriptor *cache = [[caDescriptor alloc] initStaticWithEcmpid:[desc getEcmpid]
																   casys:[desc getCasys] ident:[desc getIdent]];
		int msgid = (devs[selectedDevice].curPmtPid << 16) | (devs[selectedDevice].curServiceId & 0xffff);
		[cache setMessageId:msgid];
		caDescriptor *incache = [caCache member:cache];
		if( incache == nil )
		{
			[caCache addObject:cache];
		}
		else
		{
			[incache setEcmpid:[desc getEcmpid] casys:[desc getCasys] ident:[desc getIdent]];
		}
		[cache release];
		[pmsg release];
		devCtrl *pDev = &devs[selectedDevice];
		unsigned int emmCaCount = [pDev->catSet caDescCount];
		if( emmCaCount != 0 && ( [prevDesc getCasys] != [desc getCasys] ) )
		{
			[self clearAllEmm:selectedDevice];
			NSArray *List = [pDev->catSet getCaDescriptors];
			for( int i = 0; i < emmCaCount; i++ )
			{
				id obj = [List objectAtIndex:i];
				if( [obj getCasys] == [desc getCasys] )
				{
					unsigned int emmPid = [obj getEcmpid];
					[self addEmmPid:emmPid toDevice:selectedDevice];
					if( getEmmEnable() == YES )
					{
						msgPid filterPid;
						filterPid.id = msg_add_pid;
						filterPid.mPid = htonl(emmPid);
						NSMutableData *pmsg = [[NSMutableData alloc] init];
						[pmsg appendBytes:&filterPid length:sizeof(filterPid)];
						[self sendData:pmsg dev:selectedDevice];
						ControllerLog("EMM Processing: caid:%x emmpid:%x\n",[desc getCasys], emmPid);
					}
				}
			}
		}
	}
}

- (void)rcvNotifications:(NSNotification *)obj
{
	NSString *p = [obj object];
	if( p != NULL )
	{
		ControllerLog("cwdwplug: %s", [p cStringUsingEncoding:NSASCIIStringEncoding]);
	}
}

- (void)rcvPidsAndControls:(NSNotification *)obj
{
	NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
	NSString *p = [obj object];
	NSData *dmsg = [[NSData alloc] initWithBytes:[p cStringUsingEncoding:NSASCIIStringEncoding] 
										  length:[p lengthOfBytesUsingEncoding:NSASCIIStringEncoding]];
	NSKeyedUnarchiver *amsg = [[NSKeyedUnarchiver alloc] initForReadingWithData:dmsg];
	NSArray *rcvd = [amsg decodeObjectForKey:@"DArray"];
	[self decodePluginMessage:rcvd];
	[amsg release];
	[dmsg release];
	[pool release];
}

- (void)sendData:(NSData *)pdata dev:(int)devIndex
{
#if defined(USE_NSPORT_IPC)
	NSString *pname = [NSString stringWithFormat:@"eyetvPluginPIDFilter%d", devIndex];
	NSMessagePort *remote = [[NSMessagePortNameServer sharedInstance] portForName:pname];
	if( remote != 0 )
	{
		NSArray *parray = [NSMutableArray arrayWithObjects:pdata, nil];
		NSPortMessage *msg = [[NSPortMessage alloc] initWithSendPort:remote receivePort:nil components:parray];
		[msg sendBeforeDate:[NSDate date]];
		[msg release];
	}
#else
	NSString *pname = [NSString stringWithFormat:@"PluginPIDFilter%d", devIndex];
	NSMutableData *dmsg = [[NSMutableData alloc] init];
	NSKeyedArchiver *arch = [[NSKeyedArchiver alloc] initForWritingWithMutableData:dmsg];
	[arch setOutputFormat:NSPropertyListXMLFormat_v1_0];
	[arch encodeObject:pdata forKey:@"DMessage"];
	[arch finishEncoding];
	NSString *msg = [[NSString alloc] initWithData:dmsg encoding:NSASCIIStringEncoding];
	[[NSDistributedNotificationCenter defaultCenter] postNotificationName:pname object:msg];
	[arch release];
	[msg release];
	[dmsg release];
#endif  
}

- (void)writeDwToDescrambler:(unsigned char *)dw caDesc:(caDescriptor *)ca sid:(unsigned long)sid
{
	unsigned int msgid = 0x11111111;
	bool legacyDW = false;
	if ( dw[3] == (unsigned char)(dw[0] + dw[1] + dw[2]) &&
		dw[7] == (unsigned char)(dw[4] + dw[5] + dw[6]) &&
		dw[11] == (unsigned char)(dw[8] + dw[9] + dw[10]) &&
		dw[15] == (unsigned char)(dw[12] + dw[13] + dw[14]) )
	{
		legacyDW = true;
	}
	for(int i = 0; i < NUM_DEVS; i++)
	{
		
		//    if( memcmp(dw, devs[i].lastDW, 16) != 0)
		//    {
		//      memcpy(devs[i].lastDW, dw, 16);
		//    }
		//    else break;
		
		if( sid == devs[i].curServiceId && [devs[i].curCa isEqual:ca] == YES )
		{
			/*
			 ControllerLog("dev[%d]: needed sid = 0x%x, current sid = 0x%x\n",i, sid, devs[i].curServiceId);
			 ControllerLog("dev[%d]: needed CA = [0x%x,0x%x,0x%x,0x%x], CurCA = [0x%x,0x%x,0x%x,0x%x] \n",i,
			 [ca getEcmpid], [ca getCasys], [ca getIdent], [ca getMessageId], 
			 [devs[i].curCa getEcmpid], [devs[i].curCa getCasys], 
			 [devs[i].curCa getIdent], [devs[i].curCa getMessageId] );
			 */
			if( [devs[i].curCa getDmode] == DECRYPT_MODE_TPS_DW_ENCRYPTED && legacyDW == false )
			{
				// need additional DW decription 
			}
			NSMutableData *pmsg = [[NSMutableData alloc] init];
			[pmsg setLength:0];
			[pmsg appendBytes:&msgid length:4];
			if( legacyDW == false )
			{
				dw[3] = dw[0] + dw[1] + dw[2];
				dw[7] = dw[4] + dw[5] + dw[6];
				dw[11] = dw[8] + dw[9] + dw[10];
				dw[15] = dw[12] + dw[13] + dw[14];
			}
			[pmsg appendBytes:dw length:16];
			[self sendData:pmsg dev:i];
			[pmsg release];
		}
	}
}

- (void)emmAddParams:(unsigned char *)serial provData:(unsigned char *)bytes caid:(unsigned int)casys ident:(unsigned int)provid
{
	emmParams *emmp = [[emmParams alloc] initWithSerial:serial provData:bytes caid:casys ident:provid];
	emmParams *reader = [emmReaders member:emmp];
	if( reader == nil )
	{
		[emmReaders addObject:emmp];
	}
	[emmp release];
}

- (void)emmRmParams:(unsigned char *)serial provData:(unsigned char *)bytes caid:(unsigned int)casys ident:(unsigned int)provid
{
	emmParams *emmp = [[emmParams alloc] initWithSerial:serial provData:bytes caid:casys ident:provid];
	emmParams *reader = [emmReaders member:emmp];
	if( reader != nil )
	{
		[emmReaders removeObject:reader];
	}
	[emmp release];
}

- (IBAction)selectedDevs:(id)sender
{
	doCaChange = NO;
	int newIndex =  [sender indexOfSelectedItem] - 1;
	[sender setTitle:[NSString stringWithFormat:@"device %d", newIndex]];
	[irdCtl setSelectedDevice:newIndex];
	int newca = devs[newIndex].selected;
	if ( selectedDevice != newIndex )
	{
		selectedDevice = newIndex;
		[caDescList reloadData];
		if( newca != -1 && newca < [devs[newIndex].pmtSet caDescCount] )
		{
			[caDescList selectRowIndexes:[NSIndexSet indexSetWithIndex:newca] byExtendingSelection:NO]; 
		}
	}
	doCaChange = YES;
}

- (IBAction)cwdwAction:(id)sender
{
	BOOL action = (getShowCwDw() == YES)? NO:YES;
	setShowCwDw(action);
}

- (IBAction)requestsAction:(id)sender
{
	BOOL action = (getShowRequests() == YES)? NO:YES;
	setShowRequests(action);
}

- (IBAction)clearLogAction:(id)sender
{
	[gdbView replaceCharactersInRange:NSMakeRange(0, [[gdbView textStorage] length]) withString:@""];
}

- (IBAction)emmAndEcmRawRecord:(id)sender
{
	BOOL action = (getRawRecordState() == YES)? NO:YES;
	if( action == YES )
	{
		[sender setTitle:@"Stop recording....."];
	}
	else
	{
		[sender setTitle:@"Start ECM & EMM rec"];
		[recordPids removeAllObjects];
	}
	setRawRecording(action);
}

- (IBAction)enableEmm:(id)sender
{
	BOOL action = (getEmmEnable() == YES)? NO:YES;
	if( action == YES )
	{
		[sender setTitle:@"Disable EMM"];
		[self startAllEmmPids];
	}
	else
	{
		[sender setTitle:@"Enable EMM"];
		[self stopAllEmmPids];
	}
	setEmmEnable(action);
}

- (void)awakeFromNib
{
	[srvListCtl setDelegateForController:self];
}

- (void)caCacheSave
{
	NSMutableArray *file = [[NSMutableArray alloc] initWithCapacity:[caCache count]];
	NSEnumerator *items = [caCache objectEnumerator];
	caDescriptor *ca;
	while (  (ca = [items nextObject]) != nil )
	{
		NSMutableDictionary *caItem = [[NSMutableDictionary alloc] init];
		[caItem setObject:[NSNumber numberWithUnsignedInt:[ca getEcmpid]] forKey:@"ecmpid"];
		[caItem setObject:[NSNumber numberWithUnsignedInt:[ca getCasys]] forKey:@"casys"];
		[caItem setObject:[NSNumber numberWithUnsignedInt:[ca getIdent]] forKey:@"ident"];
		[caItem setObject:[NSNumber numberWithUnsignedInt:[ca getMessageId]] forKey:@"msgid"];
		[caItem setObject:[NSNumber numberWithUnsignedInt:[ca getIrdetoChannel]] forKey:@"irdchn"];
		[file addObject:caItem];
		[caItem release];
	}
	if( [file count] != 0 )
	{
		if( [file writeToFile:[[docPath stringByExpandingTildeInPath] stringByAppendingPathComponent:cacacheFile] 
				   atomically:NO] != YES )
		{
			NSFileManager *dir = [NSFileManager defaultManager];
			[dir createDirectoryAtPath:[docPath stringByExpandingTildeInPath] attributes:nil];
			[file writeToFile:[[docPath stringByExpandingTildeInPath] stringByAppendingPathComponent:cacacheFile]
				   atomically:NO];
		}
	}
	[file release];
}

- (void)saveCfgParams
{
	NSMutableDictionary *cfg = [[NSMutableDictionary alloc] init];
	[cfg setObject:[NSNumber numberWithBool:getEmmEnable()] forKey:@"enableEmm"];
	[cfg setObject:[NSNumber numberWithBool:[srvListCtl getEnableEmu]] forKey:@"enableEmulation"];
	
	if( [cfg writeToFile:[[docPath stringByExpandingTildeInPath] 
						  stringByAppendingPathComponent:configFile] atomically:NO] != YES )
	{
		NSFileManager *dir = [NSFileManager defaultManager];
		[dir createDirectoryAtPath:[docPath stringByExpandingTildeInPath] attributes:nil];
		[cfg writeToFile:[[docPath stringByExpandingTildeInPath] stringByAppendingPathComponent:configFile] atomically:NO];
	}
	[cfg release];
}

- (IBAction)saveCacacheAction:(id)sender
{
	[self caCacheSave];
}

- (unsigned long)getNagraIdent:(NSData *)pEcm caDesc:(caDescriptor *)ca
{
	if( [ca getCasys] >= 0x1800 && [ca getCasys] < 0x1803 )
	{
		unsigned char *pbEcm = (unsigned char *)[pEcm bytes];
		return ( ((pbEcm[5] << 8) | pbEcm[6]) & 0xffff );
	}
	return 0;
}

- (unsigned long)getCworksIdent:(NSData *)pEcm caDesc:(caDescriptor *)ca
{
	unsigned char *pbEcm = (unsigned char *)[pEcm bytes];
	for( int i = 8; i < [pEcm length]; )
	{
		if( pbEcm[i] == 0x83 && pbEcm[i + 1] == 0x1 )
		{
			return (unsigned long)pbEcm[i + 2] & 0xff; 
		}
		if( pbEcm[i] == 0x84 && pbEcm[i + 1] == 0x2 )
		{
			return (unsigned long)pbEcm[i + 2] & 0xff; 
		}
		i += pbEcm[i + 1] + 2;
	}
	return 0;
}

- (void)applicationWillTerminate:(NSNotification *)aNotification
{
	[self caCacheSave];
	[self saveCfgParams];
}

- (BOOL)windowShouldClose:(id)sender
{
	[NSApp terminate:NSApp];
	return YES;
}

- (void)clearAllEmm:(int)devno
{
	[devs[devno].emmSectionFilter removeAllObjects];
	[devs[devno].emmBuffer removeAllObjects];
	[devs[devno].emmState removeAllObjects];
}

- (void)startAllEmmPids
{
	for( int i = 0; i < NUM_DEVS; i++ )
	{
		[devs[i].sCAT reset];
	}
}

- (void)stopAllEmmPids
{
	for( int i = 0; i < NUM_DEVS; i++ )
	{
		devCtrl *pDev = &devs[i];
		if( [pDev->emmSectionFilter count] != 0 )
		{
			NSEnumerator *emms = [pDev->emmSectionFilter objectEnumerator];
			section *sEmm;
			while ( (sEmm = [emms nextObject]) != nil )
			{
				unsigned long emmPid = [sEmm getPid];
				msgPid filterPid;
				filterPid.id = msg_remove_pid;
				filterPid.mPid = htonl(emmPid);
				NSMutableData *pmsg = [[NSMutableData alloc] init];
				[pmsg appendBytes:&filterPid length:sizeof(filterPid)];
				[self sendData:pmsg dev:i];
				ControllerLog("EMM Processing: stop emmpid:%x\n", emmPid);
			}
		}
	}
}

- (void)addEmmPid:(unsigned int)pid toDevice:(int)devno
{
	devCtrl *pDev = &devs[devno];
	NSNumber *key = [[NSNumber alloc] initWithUnsignedInt:pid];
	NSNumber *state = [[NSNumber alloc] initWithInt:(int)emmStateNone];
	NSMutableData *buffer = [[NSMutableData alloc] initWithCapacity:1024];
	section *sEmm = [[section alloc] initWithPid:pid];
	[pDev->emmSectionFilter setObject:sEmm forKey:key];
	[pDev->emmBuffer setObject:buffer forKey:key];
	[pDev->emmState setObject:state forKey:key];
	[sEmm release];
	[buffer release];
	[state release];
	[key release];
}

- (id) init 
{
	logLocker = [[NSLock alloc] init];
	dumpLocker = [[NSLock alloc] init]; 
	self = [super init];
	if( self != nil )
	{
		//    mpgFile = [[NSOutputStream alloc] initToFileAtPath:@"/Users/igor/eyetvcamd.ts" append:NO];
		//    [mpgFile open];
		
		selectedDevice = 0;
		doCaChange = YES;
		pmtChanged = NO;
		emmReaders = [[NSMutableSet alloc] init];
		caCache = [[NSMutableSet alloc] init];
		docPath = [[NSString alloc] initWithCString:"~/Documents/eyetvCamd"];
		cacacheFile = [[NSString alloc] initWithCString:"cacache.plist"];
		configFile = [[NSString alloc] initWithCString:"gcfg.plist"];
		recordPids = [[NSMutableDictionary alloc] init];
		NSArray *file = [[NSArray alloc] initWithContentsOfFile:[[docPath stringByExpandingTildeInPath]
																 stringByAppendingPathComponent:cacacheFile]];
		{
			if( file != nil )
			{
				for(int i = 0; i < [file count]; i++)
				{
					NSDictionary *obj = [file objectAtIndex:i];
					caDescriptor *ca = [[caDescriptor alloc] initStaticWithEcmpid:[[obj objectForKey:@"ecmpid"] unsignedIntValue]
																			casys:[[obj objectForKey:@"casys"] unsignedIntValue]
																			ident:[[obj objectForKey:@"ident"] unsignedIntValue]];
					[ca setMessageId:[[obj objectForKey:@"msgid"] unsignedIntValue]];
					[ca setIrdetoChannel:[[obj objectForKey:@"irdchn"] unsignedIntValue]];
					[caCache addObject:ca];
					[ca release];
				}
			}
			[file release];
		}
		for(int i = 0; i < NUM_DEVS; i++)
		{
			devs[i].sPMT = [[section alloc] initWithPid:0];
			devs[i].sECM = [[section alloc] initWithPid:0];
			devs[i].sCAT = [[section alloc] initWithPid:0];
			//      devs[i].sEMM = [[section alloc] initWithPid:0];
			devs[i].curTransponderId = 0;
			devs[i].curServiceId = 0;
			devs[i].curPmtPid = 0;
			devs[i].curEcmPid = 0;
			devs[i].pmtSet = [[pmt alloc] init];
			devs[i].catSet = [[cat alloc] init];
			devs[i].selected = -1;
			devs[i].curCa = [[caDescriptor alloc] initStaticWithEcmpid:0 casys:0 ident:0];
			//      devs[i].emmBufferState = emmStateNone;
			//      devs[i].emmBuffer = [[NSMutableData alloc] initWithCapacity:1024];
			devs[i].emmSectionFilter = [[NSMutableDictionary alloc] init];
			devs[i].emmBuffer = [[NSMutableDictionary alloc] init];
			devs[i].emmState = [[NSMutableDictionary alloc] init];
		}
		NSString *teststr = [[NSString alloc] initWithCString:"test"];
		NSFileManager *dir = [NSFileManager defaultManager];
		NSString *path = [[docPath stringByExpandingTildeInPath] stringByAppendingPathComponent:@".tstfile"];
		if( [teststr writeToFile:path atomically:NO encoding:NSUnicodeStringEncoding error:NULL] != YES )
		{
			[dir createDirectoryAtPath:[docPath stringByExpandingTildeInPath] attributes:nil];
		}
		else
		{
			[dir removeFileAtPath:path handler:nil];
		}
		path = [[docPath stringByExpandingTildeInPath] stringByAppendingPathComponent:@"record/.tstfile"];
		if( [teststr writeToFile:path atomically:NO encoding:NSUnicodeStringEncoding error:NULL] != YES )
		{
			[dir createDirectoryAtPath:[[docPath stringByExpandingTildeInPath] stringByAppendingPathComponent:@"record"] attributes:nil];
		}
		else
		{
			[dir removeFileAtPath:path handler:nil];
		}
		[teststr release];
		ctrl = [self retain];
	}
	return self;
}

- (void) dealloc
{
	//  [mpgFile release];
	[pmt release];
	[emmReaders release];
	[caCache release];
	[cacacheFile release];
	[configFile release];
	[docPath release];
	[recordPids release];
	for(int i = 0; i < NUM_DEVS; i++)
	{
		[devs[i].sPMT release];
		[devs[i].sECM release];
		[devs[i].sCAT release];
		//    [devs[i].sEMM release];
		[devs[i].pmtSet release];
		[devs[i].catSet release];
		[devs[i].curCa release];
		//    [devs[i].emmBuffer release];
		[devs[i].emmSectionFilter release];
		[devs[i].emmBuffer release];
		[devs[i].emmState release];
	}
	[dumpLocker release];
	[logLocker release];
	[super dealloc];
}

@end


#import "IrdController.h"
#include "globals.h"

@implementation IrdController

- (int)numberOfRowsInTableView:(NSTableView *)tableView
{
  if( devs[selectedDevice].validFlag != YES )
  {
    return 0;
  }
  return devs[selectedDevice].curMaxChn;
}

- (id)tableView:(NSTableView *)tableView
      objectValueForTableColumn:(NSTableColumn *)tableColumn
	    row:(int)row
{
  if( devs[selectedDevice].curMaxChn > row )
  {
    NSString *column = [tableColumn identifier];
    if( [column isEqualToString:@"chanIndex"] )
    {
      return [NSString stringWithFormat:@"%d", row];
    }
    else if( [column isEqualToString:@"chanIdentifier"] )
    {
      return [NSString stringWithFormat:@"%04x", devs[selectedDevice].chids[row]];
    }
  }
  return @"";
}

- (void)tableViewSelectionDidChange:(NSNotification *)notification
{
  int row = [chanView selectedRow];
  int rowCount = devs[selectedDevice].curMaxChn;
  if( row == -1 || rowCount <= row ) 
    return;
  devs[selectedDevice].curSelectedIndex = row;
  devs[selectedDevice].curSetChn = devs[selectedDevice].chids[row];
  if( [delegated respondsToSelector:@selector(irdetoChannelChange:)] )
  {
    [delegated irdetoChannelChange:devs[selectedDevice].curSetChn];
  }
}

- (void)setIrdetoChannel:(unsigned int)chn forDev:(unsigned int)index
{
  devs[index].curSetChn = chn;
}

- (void) clearValid:(unsigned int)index
{
  devs[index].validFlag = NO;
  if(index == selectedDevice)
  {
    [chanView reloadData];
//    [chanView selectRowIndexes:[NSIndexSet indexSetWithIndex:devs[index].curSelectedIndex] byExtendingSelection:NO];
  }
}

- (void) setSelectedDevice:(unsigned int)index
{
  selectedDevice = index;
}

- (NSData *) getEcm:(NSData *)Packet dev:(unsigned int)devIndex
{
  unsigned char *pBuf = (unsigned char *)[Packet bytes];
  irdetoDevice *pDev = &devs[devIndex];
  const int cur = pBuf[4];
  const int max = pBuf[5];
  const int chid = ((pBuf[6] << 8) & 0xff00) | (pBuf[7] & 0xff);
  if( max >= 0 && max < MAX_IRD_CHANNELS )
  {
    devs[devIndex].curMaxChn = max+1;
    if( cur == 0 )
    {
      pDev->startSync = YES;
      [self clear:devIndex];
    }
    if( pDev->startSync == YES )
    {
      if( cur <= max )
      {
	pDev->chids[cur] = chid;
	pDev->irdEcm[cur] = [Packet retain];
      }
      if( cur == max )
      {
	for( int i = 0; i <= max; i++ )
	{
	  if( pDev->chids[i] == 0 )
	  {
	    pDev->startSync = false;
	    ControllerLog("irdeto ecm: zero chid\n");
	    return nil;
	  }
	  for(int j = i + 1; j <= max; j++)
	  {
	    if( pDev->chids[i] == pDev->chids[j] )
	    {
	      pDev->startSync = false;
	      ControllerLog("irdeto ecm: duplicate chid %04x\n",  chid);
	      return nil;
	    }
	  }
	  if( pDev->curSetChn != 0 && pDev->curSetChn == pDev->chids[i] )
	  {
	    pDev->curSelectedIndex = i;
	  }
	}
	if( pDev->validFlag == NO )
	{
	  pDev->validFlag = YES;
	  if( devIndex == selectedDevice )
	  {
	    [chanView reloadData];
	    [chanView selectRowIndexes:[NSIndexSet indexSetWithIndex:pDev->curSelectedIndex] byExtendingSelection:NO];
	  }
	}
	return pDev->irdEcm[pDev->curSelectedIndex];
      }
    }
  }
  return nil;
}

- (void) clear:(unsigned int)devIndex
{
  irdetoDevice *pDev = &devs[devIndex];
  pDev->startSync = YES;
//  pDev->validFlag = NO;
//  pDev->curSelectedIndex = 1;
  for( int i=0; i < MAX_IRD_CHANNELS; i++ )
  {
    if( pDev->irdEcm[i] != nil ) [pDev->irdEcm[i] release];
    pDev->irdEcm[i] = nil;
    pDev->chids[i] = 0;
  }
}

- (void) setDelegateObj:(id)anobj
{
  delegated = anobj;
}

- (id) init
{
  self = [super init];
  if (self != nil)
  {
    selectedDevice = 0;
    for(int dev = 0; dev < 12; dev++)
    {
      devs[dev].startSync = NO;
      devs[dev].validFlag = NO;
      devs[dev].curSelectedIndex = 0;
      devs[dev].curMaxChn = 0;
      devs[dev].curSetChn = 0;
      for( int i=0; i < MAX_IRD_CHANNELS; i++ )
      {
	devs[dev].irdEcm[i] = nil;
	devs[dev].chids[i] = 0;
      }
    }
  }
  return self;
}

- (void) dealloc 
{
  for(int dev = 0; dev < 12; dev++)
  {
    irdetoDevice *pDev = &devs[dev];
    for( int i=0; i < MAX_IRD_CHANNELS; i++ )
    {
      if( pDev->irdEcm[i] != nil ) [pDev->irdEcm[i] release];
      pDev->irdEcm[i] = nil;
      pDev->chids[i] = 0;
    }
  }
  [super dealloc];
}


@end

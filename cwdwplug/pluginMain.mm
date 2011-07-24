/*****************************************************************************
* eyetvplugin.c: Plug-In for the EyeTV software to connect to VLC
*****************************************************************************
* Copyright (C) 2006-2007 the VideoLAN team
* $Id: eyetvplugin.c 19206 2007-03-05 18:15:27Z fkuehne $
*
* Authors: Felix K√ºhne <fkuehne at videolan dot org>
*
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation; either version 2 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program; if not, write to the Free Software
* Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston MA 02110-1301, USA.
*****************************************************************************/
#import <Cocoa/Cocoa.h>
#import "Controller.h"
#include "messages.h"
#include "EyeTVPluginDefs.h"
#include "FFdecsa/FFdecsa.h"

#define PLUGIN_NOTIFICATION_OBJECT	"cwdwgw"

#pragma push
#pragma pack(1)


/* Structure for TS-Packets */
typedef struct 
{
  unsigned char header[4];
  unsigned char	data[188-4];
} TransportStreamPacket;

#pragma pop

VLCEyeTVPluginGlobals_t *lGlobals = 0;

//static Controller *ctrl = 0;
//static NSMutableIndexSet *decryptPids;

Controller *ctrls[MAX_DEVICES];
static bool doSendTsPackets = NO;
static int GetDeviceInfoIndex(VLCEyeTVPluginGlobals_t *globals, EyeTVPluginDeviceID deviceID)
{
  int i;
  if( globals ) 
  {
//    if(globals->deviceCount == 1)
//      return 0;
    
    for( i=0; i<globals->deviceCount; i++) 
    {
      if( globals->devices[i].deviceID == deviceID ) 
      {
	return i;
      }
    }
  }
  return -1;
}

#pragma mark -

/* initialise the plug-in */
static long VLCEyeTVPluginInitialize(VLCEyeTVPluginGlobals_t** globals, long apiVersion, EyeTVPluginCallbackProc callback)
{
  long result = 0;
  lGlobals = *globals = (VLCEyeTVPluginGlobals_t *) calloc(1, sizeof( VLCEyeTVPluginGlobals_t ) );
  ( *globals )->callback = callback;
  
  for(int i = 0; i < MAX_DEVICES; i++)
  {
    ctrls[i] = nil;
  }
  pluginLog("%s\n", "plugin initialized");
  return result;
}

/* we will be terminated soon, clean up */
static long VLCEyeTVPluginTerminate(VLCEyeTVPluginGlobals_t *globals)
{
  pluginLog("%s\n", "plugin terminated");
  
  long result = 0;
  
  /* invalidate and free msg port */
  
  if( globals ) 
  {
    free( globals );
  }
  
  lGlobals = 0;
  
  for(int i = 0; i < MAX_DEVICES; i++)
  {
    [ctrls[i] release];
    ctrls[i] = nil;
  }
  return result;
}

/* called when EyeTV asks various stuff about us */
static long VLCEyeTVPluginGetInformation(VLCEyeTVPluginGlobals_t *globals, long* outAPIVersion, char* outName, char *outDescription)
{
  pluginLog("GetInfo\n");
  long result = 0;
  if( globals ) 
  {
    if( outAPIVersion )
    {
      *outAPIVersion = EYETV_PLUGIN_API_VERSION;
    }
    
    if( outName )
    {
      char* name = "cwdwgw Plug-In";
      strcpy( &outName[0], name);
    }
    
    if( outDescription )
    {
      char* desc = "This Plug-In connects EyeTV to the cwdwgw.";
      strcpy( &outDescription[0], desc);
    }
  }
  return result;
}

/* called if a device is added */
static long VLCEyeTVPluginDeviceAdded(VLCEyeTVPluginGlobals_t *globals, EyeTVPluginDeviceID deviceID, EyeTVPluginDeviceType deviceType)
{
  long result = 0;
  DeviceInfo *deviceInfo;
  if( deviceID == 100 ) deviceID = 101;
  if( globals ) 
  {
    if( globals->deviceCount < MAX_DEVICES )  
    {
      int devIndex = globals->deviceCount;
      pluginLog("Device with id %i and Index = %d added\n", deviceID, devIndex);
      deviceInfo = &( globals->devices[devIndex] );
      memset(deviceInfo, 0, sizeof(DeviceInfo));
      
      deviceInfo->deviceID = deviceID;
      deviceInfo->deviceType = deviceType;
      ctrls[devIndex] = [[Controller alloc] init];
      [ctrls[devIndex] setDevIndex:devIndex];
      [NSThread detachNewThreadSelector:@selector(LaunchThread:) toTarget:[Controller class] withObject:ctrls[devIndex]];
      globals->deviceCount++;
    }
  }
  return result;
}

/* called if a device is removed */
static long VLCEyeTVPluginDeviceRemoved(VLCEyeTVPluginGlobals_t *globals, EyeTVPluginDeviceID deviceID)
{
  pluginLog("Device ID = %016qx removed\n", deviceID);
  
  long result = 0;
  int i;
  
  if( globals ) 
  {
    for( i = 0; i < globals->deviceCount; i++ )
    {
      if ( globals->devices[i].deviceID == deviceID ) 
      {
	globals->deviceCount--;
	
	if( i<globals->deviceCount )
	{
	  globals->devices[i] = globals->devices[globals->deviceCount];
	}
      }
    }
  }
  return result;
}

/* This function is called, whenever packets are received by EyeTV. For reasons of performance,
* the data is the original data, not a copy. That means, EyeTV waits until this method is 
* finished. Therefore all in this method should be as fast as possible. */
static long VLCEyeTVPluginPacketsArrived(VLCEyeTVPluginGlobals_t *globals, EyeTVPluginDeviceID deviceID, long **packets, long packetsCount)
{
  if( globals != 0 )
  {
    int devIndex = GetDeviceInfoIndex(globals, deviceID);
    if( devIndex != -1 )
    {
      DeviceInfo *deviceInfo = &globals->devices[devIndex];
      
      if( deviceInfo != 0 && ctrls[devIndex] != 0 ) 
      {
	bool needSendPids = NO;
	for(int i = 0; i < packetsCount; i++ ) 
	{
	  TransportStreamPacket *packet = ( TransportStreamPacket* )packets[i];
	  
	  // search for PID 
	  unsigned int PID = ((packet->header[1] & 0x1fL) << 8) | (packet->header[2] & 0xffL);
	  
	  if( [ctrls[devIndex] isPidInList:PID] == YES ) 
	  {
	    NSArray *sndData = [NSArray arrayWithObjects:[NSData dataWithBytes:&packet->header[0] length:188], [NSNumber numberWithInt:devIndex], nil];
	    [ctrls[devIndex] sendData:[NSArray arrayWithObjects:sndData, nil]];
	    globals->packetCount++;
	    needSendPids = YES;
	  }
	  
	  if( doSendTsPackets == YES && (PID == 0 || PID == 0x11 || PID == 0x12) )
	  {
	    NSArray *sndData = [NSArray arrayWithObjects:[NSData dataWithBytes:&packet->header[0] length:188], [NSNumber numberWithInt:devIndex], nil];
	    [ctrls[devIndex] sendData:[NSArray arrayWithObjects:sndData, nil]];
	    needSendPids = YES;
	  }
	  
	  if( [ctrls[devIndex] descramble] == YES )
	  {
	    if( [ctrls[devIndex] isPidInDescramblerList:PID] == YES )
	    {
	      [ctrls[devIndex] appendTsPacket:&packet->header[0] length:188];
	      [ctrls[devIndex] getCleanTsPacket:&packet->header[0]];
	    // DEBUG
	      if( doSendTsPackets == YES )
	      {
		NSArray *sndData = [NSArray arrayWithObjects:[NSData dataWithBytes:&packet->header[0] length:188], [NSNumber numberWithInt:devIndex], nil];
		[ctrls[devIndex] sendData:[NSArray arrayWithObjects:sndData, nil]];
		needSendPids = YES;
	      }
	    // DEBUG  
	    }
	  }
	}
      }
    }
  }
  return 0;
}

/*	VLCEyeTVPluginServiceChanged,
*
*	- *globals		: The plug-in Globals
*	- deviceID		: Identifies the active Device
*   - headendID		: The HeadendID, for e300 it's the orbital position of the satelite in 
*					  tenth degrees east
*   - transponderID : The Frequency in kHz
*   - serviceID		: original ServiceID from the DVB-Stream (e300, e400)
*	- pidList		: List of active PIDs	
*
*	Whenever a service changes, this function is called. Service-related plug-in data should be updated here.
*/
static long VLCEyeTVPluginServiceChanged(VLCEyeTVPluginGlobals_t *globals, 
					 EyeTVPluginDeviceID deviceID, 
					 long headendID, 
					 long transponderID, 
					 long serviceID, 
					 EyeTVPluginPIDInfo *pidList, 
					 long pidsCount)
{
  long result = 0;
  int	i;
  pluginLog("service change API called: globals = 0x%x, deviceid = %d\n", globals, deviceID);
  if( globals ) 
  {
    int devIndex = GetDeviceInfoIndex(globals, deviceID);
    if( devIndex != -1 )
    {
      DeviceInfo *deviceInfo = &globals->devices[devIndex];
      if( deviceInfo ) 
      {
		bool		pidsChanged = false;
		
		if (deviceInfo->pidsCount != pidsCount)
		{
			pluginLog("pidsCount changed from %d to %d\n", deviceInfo->pidsCount, pidsCount);
			pidsChanged = true;
		}
		else
		{
		  for( i = 0; i < pidsCount; i++ )
		  {
			if (![ctrls[devIndex] isPidInList:pidList[i].pid])
			{
				pluginLog("pid %d is new\n", pidList[i].pid);
				pidsChanged = true;
			}
		  }
		}
		
	if( (deviceInfo->headendID != headendID) || (deviceInfo->transponderID != transponderID)  || (deviceInfo->serviceID != serviceID) || pidsChanged )
	{
	  [ctrls[devIndex] clearPidList];
	  [ctrls[devIndex] setDescramble:NO];
	  [ctrls[devIndex] clearDescramblerPidList];
	  [ctrls[devIndex] channelChange];
	  pluginLog("ServiceChanged:\n");
	  deviceInfo->headendID = headendID;
	  deviceInfo->transponderID = transponderID;
	  deviceInfo->serviceID = serviceID;
	  deviceInfo->activePIDsCount = pidsCount;
	  unsigned long pmtpid = 0;

	  for( i = 0; i < pidsCount; i++ )
	  {
	    deviceInfo->activePIDs[i] = pidList[i];
	    pluginLog("Active PID: %lx, type: %lx\n", pidList[i].pid, pidList[i].pidType);
	    if(pidList[i].pidType == kEyeTVPIDType_PMT) pmtpid = pidList[i].pid;
	    if( pidList[i].pidType == kEyeTVPIDType_Video || 
		pidList[i].pidType == kEyeTVPIDType_AC3Audio || 
		pidList[i].pidType == kEyeTVPIDType_MPEGAudio ||
		pidList[i].pidType == 0xd )
	    {
	      [ctrls[devIndex] addPidToDescrambler:pidList[i].pid];
	    }
	  }

	  msgNewChannel message;
	  fillNewChannelMessage(&message,transponderID,serviceID,pmtpid);
	  pluginLog("HeadendID: %lx, TransponderID: %lx, ServiceID: %lx, PMT pid: %lx\n" , headendID, transponderID, serviceID, pmtpid);
	  NSArray *ncMsg = [NSArray arrayWithObjects:[NSData dataWithBytes:&message length:sizeof(message)], [NSNumber numberWithInt:devIndex], nil];
	  [ctrls[devIndex] sendData:[NSArray arrayWithObjects:ncMsg, nil]];
	}      
	deviceInfo->pidsCount = 0;
	
      }
    }
  }
  return result;
}


#pragma mark -
/* EyeTVPluginDispatcher,
*
*	- selector : See 'EyeTVPluginDefs.h'
*	- *refCon :  The RefCon to the plug-in-related Data
*	- deviceID : Identifies the Device
*	- params : Parameters for functioncall
*
* This function is a part of the interface for the communication with EyeTV. If something happens,
* EyeTV thinks, we should know of, it calls this function with the corresponding selector. */

#pragma export on

extern "C" long EyeTVPluginDispatcher( EyeTVPluginParams* params )
{
  long result = 0;
  NSAutoreleasePool *arPool = [[NSAutoreleasePool alloc] init];
  
  switch( params->selector ) 
  {
    case kEyeTVPluginSelector_Initialize:
      result = VLCEyeTVPluginInitialize((VLCEyeTVPluginGlobals_t**)params->refCon, 
					params->initialize.apiVersion, params->initialize.callback);
      break;
      
    case kEyeTVPluginSelector_Terminate:
      result = VLCEyeTVPluginTerminate((VLCEyeTVPluginGlobals_t*)params->refCon);
      break;
      
    case kEyeTVPluginSelector_GetInfo:
      result = VLCEyeTVPluginGetInformation((VLCEyeTVPluginGlobals_t*)params->refCon, 
					    params->info.pluginAPIVersion, params->info.pluginName, params->info.description);
      break;
      
    case kEyeTVPluginSelector_DeviceAdded:
      result = VLCEyeTVPluginDeviceAdded((VLCEyeTVPluginGlobals_t*)params->refCon, 
					 params->deviceID, params->deviceAdded.deviceType);
      break;
      
    case kEyeTVPluginSelector_DeviceRemoved:
      result = VLCEyeTVPluginDeviceRemoved((VLCEyeTVPluginGlobals_t*)params->refCon, params->deviceID);
      break;
      
    case kEyeTVPluginSelector_PacketsArrived:
      result = VLCEyeTVPluginPacketsArrived((VLCEyeTVPluginGlobals_t*)params->refCon, params->deviceID, 
					    params->packetsArrived.packets, params->packetsArrived.packetCount);
      break;
      
    case kEyeTVPluginSelector_ServiceChanged:
      result = VLCEyeTVPluginServiceChanged((VLCEyeTVPluginGlobals_t*)params->refCon, 
					    params->deviceID, params->serviceChanged.headendID, 
					    params->serviceChanged.transponderID, params->serviceChanged.serviceID, 
					    params->serviceChanged.pidList, params->serviceChanged.pidCount);
      break;
  }
  [arPool release];
  return result;
}

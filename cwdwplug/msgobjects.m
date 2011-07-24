//
//  msgobjects.m
//  cwdwplug
//
//  Created by igor on 5/4/07.
//  Copyright 2007 __MyCompanyName__. All rights reserved.
//

#import "msgobjects.h"


@implementation PLChanChange

- (id)initWithTransponder:(unsigned int)transponder service:(unsigned int)srv pmtpid:(unsigned int)pid
{
  self = [super init];
  if( self != nil )
  {
    transponderId = transponder;
    serviceId = srv;
    pmtPid = pid;
  }
  return self;
}

- (unsigned long)transponder
{
  return transponderId;
}

- (unsigned long)service
{
  return serviceId;
}

- (unsigned long)pmtpid;
{
  return pmtPid;
}

- (void) dealloc 
{
  [super dealloc];
}

@end

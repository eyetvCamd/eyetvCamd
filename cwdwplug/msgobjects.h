//
//  msgobjects.h
//  cwdwplug
//
//  Created by igor on 5/4/07.
//  Copyright 2007 __MyCompanyName__. All rights reserved.
//

#import <Cocoa/Cocoa.h>


@interface PLChanChange : NSObject
{
  @private
  unsigned long transponderId;
  unsigned long serviceId;
  unsigned long pmtPid;
}

- (unsigned long)transponder;
- (unsigned long)service;
- (unsigned long)pmtpid;
- (id)initWithTransponder:(unsigned int)transponder service:(unsigned int)srv pmtpid:(unsigned int)pid;
- (void) dealloc;
@end

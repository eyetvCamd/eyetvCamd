// AGSocketAddress.h
//
// Copyright (c) 2002 Aram Greenman. All rights reserved.
//
// Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:
//
// 1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
// 2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.
// 3. The name of the author may not be used to endorse or promote products derived from this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE AUTHOR "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

#import <Foundation/NSObject.h>
#include <sys/types.h>
#include <sys/socket.h>

/*!
@class AGSocketAddress
@abstract AGSocketAddress is a wrapper for a sockaddr structure. 
@discussion AGSocketAddress is a wrapper for a sockaddr structure. Usually you don't instatiate AGSocketAddress directly but one of its subclasses: AGInetSocketAddress, for AF_INET (IPv4) addresses, AGInet6SocketAddress, for AF_INET6 (IPv6) addresses, and AGUnixSocketAddress, for AF_UNIX addresses. 

AGSocketAddress is a class cluster. Concrete subclasses must implement the primitive methods -length, -setSocketAddress:length:, and -socketAddress. Additionally they should probably override any initializers declared here.
*/
@interface AGSocketAddress : NSObject <NSCopying, NSMutableCopying> {
}

/*!
@method addressWithAddress:
Creates a new address with <i>addr</i>. Useful for converting an AGSocketAddress to one of its subclasses, e.g.:

<code>&nbsp;&nbsp;&nbsp;&nbsp;AGSocketAddress *sa;<br>
&nbsp;&nbsp;&nbsp;&nbsp;AGInetSocketAddress *ia;<br><br>
&nbsp;&nbsp;&nbsp;&nbsp;...<br><br>
&nbsp;&nbsp;&nbsp;&nbsp;if ([sa family] == AF_INET)<br>
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;ia = [AGInetSocketAddress addressWithAddress:sa];</code> */
+ (id)addressWithAddress:(AGSocketAddress *)addr;

/*!
@method addressWithSocketAddress:length:
Creates a new address initialized with <i>addr</i> and <i>len</i>. */
+ (id)addressWithSocketAddress:(const struct sockaddr *)addr length:(unsigned)len;

/*!
@method initWithAddress:
Initializes an address with <i>addr</i>. Useful for converting an AGSocketAddress to one of its subclasses, e.g.:

<code>&nbsp;&nbsp;&nbsp;&nbsp;AGSocketAddress *sa;<br>
&nbsp;&nbsp;&nbsp;&nbsp;AGInetSocketAddress *ia;<br><br>
&nbsp;&nbsp;&nbsp;&nbsp;...<br><br>
&nbsp;&nbsp;&nbsp;&nbsp;if ([sa family] == AF_INET)<br>
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;ia = [[AGInetSocketAddress alloc] initWithAddress:sa];</code> */
- (id)initWithAddress:(AGSocketAddress *)addr;

/*!
@method initWithSocketAddress:length:
Initializes an address with sockaddr structure <i>addr</i> of length <i>len</i>. */
- (id)initWithSocketAddress:(const struct sockaddr *)addr length:(unsigned)len;

/*!
@method family
Returns the address family of the receiver, i.e., AF_INET, AF_UNIX, etc. */
- (int)family;

/*!
@method isEqual:
Compares receiver to <i>obj</i> and returns YES if they represent equivalent socket addresses. This implementation just compares the sockaddr structure representations of the two addresses for bit equality, subclasses should override this method if they allow for less strict comparison. */
- (BOOL)isEqual:(id)obj;

/*!
@method setSocketAddress:length:
Sets the address to sockaddr structure <i>addr</i> of length <i>len</i>. */
- (void)setSocketAddress:(const struct sockaddr *)addr length:(unsigned)len;


/*!
@method socketAddress
Returns a pointer to the sockaddr structure representation of the receiver. */
- (const struct sockaddr *)socketAddress;

/*!
@method length
Returns the length of the sockaddr structure representation of the receiver. */
- (unsigned)length;

@end
// AGUnixSocketAddress.h
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

#import <AGSocketAddress.h>
#include <sys/un.h>

/*!
@class AGUnixSocketAddress
@abstract AGUnixSocketAddress represents Unix local domain addresses.
@discussion AGUnixSocketAddress represents Unix local domain addresses.  
*/
@interface AGUnixSocketAddress : AGSocketAddress {
	struct sockaddr_un address;
}

/*!
@method addressWithPath:
Creates a new address with the given path. */
+ (id)addressWithPath:(NSString *)path;

/*!
@method addressWithUnixSocketAddress:
Create a new address with <i>addr</i>. */
+ (id)addressWithUnixSocketAddress:(const struct sockaddr_un *)addr;

/*!
@method initWithPath:
Initializes an address with the given path. */
- (id)initWithPath:(NSString *)path;

/*!
@method initWithUnixSocketAddress:
Initializes an address with <i>addr</i>. */
- (id)initWithUnixSocketAddress:(const struct sockaddr_un *)addr;

/*!
@method path
Returns the path of the receiver. */
- (NSString *)path;

/*!
@method unixSocketAddress
Returns a pointer to the sockaddr_un structure representation of the receiver. */
- (const struct sockaddr_un *)unixSocketAddress;

/*!
@method setPath:
Sets the path of the receiver to <i>path</i>. */
- (void)setPath:(NSString *)path;

/*!
@method setUnixSocketAddress:
Sets the address to sockaddr_un structure <i>addr</i>. */
- (void)setUnixSocketAddress:(const struct sockaddr_un *)addr;

@end
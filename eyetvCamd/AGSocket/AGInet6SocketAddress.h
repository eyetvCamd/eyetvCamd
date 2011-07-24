// AGInet6SocketAddress.h
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
#include <netinet/in.h>

@class NSArray, NSString;

/*!
@class AGInet6SocketAddress
@abstract AGInet6SocketAddress is a subclass of AGSocketAddress for AF_INET6 (IPv6) addresses.
@discussion AGInet6SocketAddress is a wrapper for a sockaddr_in6 structure. As a subclass of AGSocketAddress, it is suitable for use in AGSocket methods such as -readDataFromAddress:. 

Host addresses must be a pointer to a 128-bit IPv6 address. Port numbers are always in network (big-endian) byte-order. 
*/
@interface AGInet6SocketAddress : AGSocketAddress {
	struct sockaddr_in6 address;
}

/*!
@method stringForHostAddress:
Returns the string representation of <i>host</i>. */
+ (NSString *)stringForHostAddress:(const uint8_t *)host;

/*!
@method addressesForHostname:port:
Returns an NSArray containing all addresses for <i>name</i>, with the port number for each set to <i>port</i>. Returns an empty array if <i>name</i> can't be resolved. */
+ (NSArray *)addressesForHostname:(NSString *)name port:(uint16_t)port;

/*!
@method addressWithHostname:port:
Creates a new address with hostname <i>name</i> and port number <i>port</i>. <i>name</i> may be either a hostname or the string representation of a host address. Returns nil if <i>name</i> can't be resolved. */
+ (id)addressWithHostname:(NSString *)name port:(uint16_t)port;

/*!
@method addressWithHostAddress:port:
Creates a new address with host address <i>host</i> and port number <i>port</i>. */
+ (id)addressWithHostAddress:(const uint8_t *)host port:(uint16_t)port;

/*!
@method addressWithInet6SocketAddress:
Creates a new address with <i>addr</i>. */
+ (id)addressWithInet6SocketAddress:(const struct sockaddr_in6 *)addr;

/*!
@method initWithHostname:port:
Initializes an address with hostname <i>name</i> and port number <i>port</i>. <i>name</i> may be either a hostname or the string representation of a host address. Returns nil if <i>name</i> can't be resolved. */
- (id)initWithHostname:(NSString *)name port:(uint16_t)port;

/*!
@method initWithHostAddress:port:
Initializes an address with host address <i>host</i> and port number <i>port</i>. */
- (id)initWithHostAddress:(const uint8_t *)host port:(uint16_t)port;

/*!
@method initWithInet6SocketAddress:
Initializes an address with <i>addr</i>. */
- (id)initWithInet6SocketAddress:(const struct sockaddr_in6 *)addr;

/*!
@method hostAddress
Returns a pointer to the 128-bit representation of the host address of the receiver. */
- (const uint8_t *)hostAddress;

/*!
@method inet6SocketAddress
Returns a pointer to the sockaddr_in6 structure representation of the receiver. */
- (const struct sockaddr_in6 *)inet6SocketAddress;

/*!
@method port
Returns the port number of the receiver. */
- (uint16_t)port;

/*!
@method setHostAddress:
Sets the host address of the receiver to <i>host</i>. */
- (void)setHostAddress:(const uint8_t *)host;

/*!
@method setInet6SocketAddress:
Sets the address to sockaddr_in6 structure <i>addr</i>. */
- (void)setInet6SocketAddress:(const struct sockaddr_in6 *)addr;

/*!
@method setPort:
Sets the port number of the receiver to <i>port</i>. */
- (void)setPort:(uint16_t)port;

/*!
@method hostname
Returns the hostname of the receiver, or nil if the host address can't be resolved. */
- (NSString *)hostname;

/*!
@method aliases
Returns an NSArray containing all aliases for the receiver. Returns an empty array if no aliases are found, or if the host address can't be resolved. */
- (NSArray *)aliases;

@end

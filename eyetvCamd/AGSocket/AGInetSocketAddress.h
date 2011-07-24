// AGInetSocketAddress.h
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
@class AGInetSocketAddress
@abstract AGInetSocketAddress is a subclass of AGSocketAddress for AF_INET (IPv4) addresses.
@discussion AGInetSocketAddress is a wrapper for a sockaddr_in structure. As a subclass of AGSocketAddress, it is suitable for use in AGSocket methods such as -readDataFromAddress:. 

Host addresses and port numbers are always in network (big-endian) byte-order. 
*/
@interface AGInetSocketAddress : AGSocketAddress {
	struct sockaddr_in address;
}

/*!
@method stringForHostAddress:
Returns the dotted quad string representation of <i>host</i>. */
+ (NSString *)stringForHostAddress:(uint32_t)host;

/*!
@method addressesForHostname:port:
Returns an NSArray containing all addresses for <i>name</i>, with the port number for each set to <i>port</i>. Returns an empty array if <i>name</i> can't be resolved. */
+ (NSArray *)addressesForHostname:(NSString *)name port:(uint16_t)port;

/*!
@method addressWithHostname:port:
Creates a new address with hostname <i>name</i> and port number <i>port</i>. <i>name</i> may be either a hostname or a dotted quad representation of a host address. Returns nil if <i>name</i> can't be resolved. */
+ (id)addressWithHostname:(NSString *)name port:(uint16_t)port;

/*!
@method addressWithHostAddress:port:
Creates a new address with host address <i>host</i> and port number <i>port</i>. */
+ (id)addressWithHostAddress:(uint32_t)host port:(uint16_t)port;

/*!
@method addressWithInetSocketAddress:
Creates a new address with <i>addr</i>. */
+ (id)addressWithInetSocketAddress:(const struct sockaddr_in *)addr;

/*!
@method initWithHostname:port:
Initializes an address with hostname <i>name</i> and port number <i>port</i>. <i>name</i> may be either a hostname or a dotted quad representation of a host address. Returns nil if <i>name</i> can't be resolved. */
- (id)initWithHostname:(NSString *)name port:(uint16_t)port;

/*!
@method initWithHostAddress:port:
Initializes an address with host address <i>host</i> and port number <i>port</i>. */
- (id)initWithHostAddress:(uint32_t)host port:(uint16_t)port;

/*!
@method initWithInetSocketAddress:
Initializes an address with <i>addr</i>. */
- (id)initWithInetSocketAddress:(const struct sockaddr_in *)addr;

/*!
@method hostAddress
Returns the binary representation of the host address of the receiver. */
- (uint32_t)hostAddress;

/*!
@method inetSocketAddress
Returns a pointer to the sockaddr_in structure representation of the receiver. */
- (const struct sockaddr_in *)inetSocketAddress;

/*!
@method port
Returns the port number of the receiver. */
- (uint16_t)port;

/*!
@method setHostAddress:
Sets the host address of the receiver to <i>host</i>. */
- (void)setHostAddress:(uint32_t)host;

/*!
@method setInetSocketAddress:
Sets the address to sockaddr_in structure <i>addr</i>. */
- (void)setInetSocketAddress:(const struct sockaddr_in *)addr;

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
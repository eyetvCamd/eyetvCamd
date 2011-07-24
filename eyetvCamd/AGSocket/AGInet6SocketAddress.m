// AGInet6SocketAddress.m
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

#import <AGInet6SocketAddress.h>
#import <Foundation/Foundation.h>
#include <arpa/inet.h>
#include <netdb.h>

@implementation AGInet6SocketAddress

+ (NSString *)stringForHostAddress:(const uint8_t *)host {
	char buf[INET6_ADDRSTRLEN];
	struct in6_addr addr;
	memcpy(addr.s6_addr, host, sizeof(addr.s6_addr));
	if (!inet_ntop(AF_INET6, &addr, buf, INET6_ADDRSTRLEN))
		return nil;
	return [NSString stringWithUTF8String:buf];
}

+ (NSArray *)addressesForHostname:(NSString *)name port:(uint16_t)port {
	NSMutableArray *addrs = [NSMutableArray array];
	struct hostent *hent;
	if (!(hent = gethostbyname2([name UTF8String], AF_INET6)))
		return addrs;
	int i;
	for (i = 0; hent->h_addr_list[i]; i++)
		[addrs addObject:[self addressWithHostAddress:((struct in6_addr *)hent->h_addr_list[i])->s6_addr port:port]];
	return addrs;
}

+ (id)addressWithHostname:(NSString *)name port:(uint16_t)port {
	return [[[self alloc] initWithHostname:name port:port] autorelease];
}

+ (id)addressWithHostAddress:(const uint8_t *)host port:(uint16_t)port {
	return [[[self alloc] initWithHostAddress:host port:port] autorelease];
}

+ (id)addressWithInet6SocketAddress:(const struct sockaddr_in6 *)addr {
	return [[[self alloc] initWithInet6SocketAddress:addr] autorelease];
}

- (id)initWithHostname:(NSString *)name port:(uint16_t)port {
	struct hostent *hent;
	if (!(hent = gethostbyname2([name UTF8String], AF_INET6))) {
		[self release];
		return nil;
	}
	return [self initWithHostAddress:((struct in6_addr *)hent->h_addr)->s6_addr port:port];
}

- (id)initWithHostAddress:(const uint8_t *)host port:(uint16_t)port {
	struct sockaddr_in6 addr;
	memset(&addr, 0, sizeof(struct sockaddr_in6));
	addr.sin6_family = AF_INET6;
	addr.sin6_port = port;
	memcpy(addr.sin6_addr.s6_addr, host, sizeof(addr.sin6_addr.s6_addr));
	return [self initWithInet6SocketAddress:&addr];
}

- (id)initWithInet6SocketAddress:(const struct sockaddr_in6 *)addr {
	if (self = [super init])
		[self setInet6SocketAddress:addr];
	return self;
}

- (id)init {
	return [self initWithHostAddress:in6addr_any.s6_addr port:0];
}

- (const uint8_t *)hostAddress {
	return address.sin6_addr.s6_addr;
}

- (const struct sockaddr_in6 *)inet6SocketAddress {
	return &address;
}

- (uint16_t)port {
	return address.sin6_port;
}

- (void)setHostAddress:(const uint8_t *)host {
	memcpy(address.sin6_addr.s6_addr, host, sizeof(address.sin6_addr.s6_addr));
}

- (void)setInet6SocketAddress:(const struct sockaddr_in6 *)addr {
	memcpy(&address, addr, sizeof(struct sockaddr_in6));
}

- (void)setPort:(uint16_t)port {
	address.sin6_port = port;
}

- (NSString *)hostname {
	struct hostent *hent;
	struct in6_addr addr;
	memcpy(addr.s6_addr, [self hostAddress], sizeof(addr.s6_addr));
	if (!(hent = gethostbyaddr((char *)&addr, sizeof(addr), AF_INET6)))
		return nil;
	return [NSString stringWithUTF8String:hent->h_name];
}

- (NSArray *)aliases {
	NSMutableArray *aliases = [NSMutableArray array];
	struct hostent *hent;
	struct in6_addr addr;
	memcpy(addr.s6_addr, [self hostAddress], sizeof(addr.s6_addr));
	if (!(hent = gethostbyaddr((char *)&addr, sizeof(addr), AF_INET6)))
		return aliases;
	int i;
	for (i = 0; hent->h_aliases[i]; i++)
		[aliases addObject:[NSString stringWithUTF8String:hent->h_aliases[i]]];
	return aliases;
}

- (void)setSocketAddress:(const struct sockaddr *)addr length:(unsigned)len {
	if (addr->sa_family != AF_INET6)
		[NSException raise:NSInvalidArgumentException format:@"invalid address family"];
	[self setInet6SocketAddress:(struct sockaddr_in6 *)addr];
}

- (const struct sockaddr *)socketAddress {
	return (struct sockaddr *)&address;
}

- (unsigned)length {
	return sizeof(struct sockaddr_in6);
}

- (NSString *)description {
	return [NSString stringWithFormat:@"%@ {\n\
	address = %@,\n\
	port = %u,\n\
	hostname = %@,\n\
	aliases = %@\n\
}", 
	[super description], 
	[[self class] stringForHostAddress:[self hostAddress]], ntohs([self port]), 
	[self hostname], 
	[self aliases]];
}

@end

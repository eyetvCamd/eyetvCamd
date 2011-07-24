// AGInetSocketAddress.m
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

#import <AGInetSocketAddress.h>
#import <Foundation/Foundation.h>
#include <arpa/inet.h>
#include <netdb.h>

@implementation AGInetSocketAddress

+ (NSString *)stringForHostAddress:(uint32_t)host {
	char buf[INET_ADDRSTRLEN];
	struct in_addr addr = { host };
	if (!inet_ntop(AF_INET, &addr, buf, INET_ADDRSTRLEN))
		return nil;
	return [NSString stringWithUTF8String:buf];
}

+ (NSArray *)addressesForHostname:(NSString *)name port:(uint16_t)port {
	NSMutableArray *addrs = [NSMutableArray array];
	struct hostent *hent;
	if (!(hent = gethostbyname([name UTF8String])))
		return addrs;
	int i;
	for (i = 0; hent->h_addr_list[i]; i++)
		[addrs addObject:[self addressWithHostAddress:*(uint32_t *)hent->h_addr_list[i] port:port]];
	return addrs;
}

+ (id)addressWithHostname:(NSString *)name port:(uint16_t)port {
	return [[[self alloc] initWithHostname:name port:port] autorelease];
}

+ (id)addressWithHostAddress:(uint32_t)host port:(uint16_t)port {
	return [[[self alloc] initWithHostAddress:host port:port] autorelease];
}

+ (id)addressWithInetSocketAddress:(const struct sockaddr_in *)addr {
	return [[[self alloc] initWithInetSocketAddress:addr] autorelease];
}

- (id)initWithHostname:(NSString *)name port:(uint16_t)port {
	struct hostent *hent;
	if (!(hent = gethostbyname([name UTF8String]))) {
		[self release];
		return nil;
	}
	uint32_t host = *(uint32_t *)hent->h_addr;
	return [self initWithHostAddress:host port:port];
}

- (id)initWithHostAddress:(uint32_t)host port:(uint16_t)port {
	struct sockaddr_in addr;
	memset(&addr, 0, sizeof(struct sockaddr_in));
	addr.sin_family = AF_INET;
	addr.sin_port = port;
	addr.sin_addr.s_addr = host;
	return [self initWithInetSocketAddress:&addr];
}

- (id)initWithInetSocketAddress:(const struct sockaddr_in *)addr {
	if (self = [super init])
		[self setInetSocketAddress:addr];
	return self;
}

- (id)init {
	return [self initWithHostAddress:INADDR_ANY port:0];
}

- (void)setInetSocketAddress:(const struct sockaddr_in *)addr {
	memcpy(&address, addr, sizeof(struct sockaddr_in));
}

- (const struct sockaddr_in *)inetSocketAddress {
	return &address;
}

- (void)setHostAddress:(uint32_t)host {
	address.sin_addr.s_addr = host;
}

- (uint32_t)hostAddress {
	return address.sin_addr.s_addr;
}

- (void)setPort:(uint16_t)port {
	address.sin_port = port;
}

- (uint16_t)port {
	return address.sin_port;
}

- (NSString *)hostname {
	struct hostent *hent;
	uint32_t host = [self hostAddress];
	if (!(hent = gethostbyaddr((char *)&host, sizeof(uint32_t), AF_INET)))
		return nil;
	return [NSString stringWithUTF8String:hent->h_name];
}

- (NSArray *)aliases {
	NSMutableArray *aliases = [NSMutableArray array];
	struct hostent *hent;
	uint32_t host = [self hostAddress];
	if (!(hent = gethostbyaddr((char *)&host, sizeof(uint32_t), AF_INET)))
		return aliases;
	int i;
	for (i = 0; hent->h_aliases[i]; i++)
		[aliases addObject:[NSString stringWithUTF8String:hent->h_aliases[i]]];
	return aliases;
}

- (void)setSocketAddress:(const struct sockaddr *)addr length:(unsigned)len {
	if (addr->sa_family != AF_INET)
		[NSException raise:NSInvalidArgumentException format:@"invalid address family"];
	[self setInetSocketAddress:(struct sockaddr_in *)addr];
}

- (const struct sockaddr *)socketAddress {
	return (struct sockaddr *)&address;
}

- (unsigned)length {
	return sizeof(struct sockaddr_in);
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
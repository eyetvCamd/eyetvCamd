// AGUnixSocketAddress.m
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

#import <AGUnixSocketAddress.h>
#import <Foundation/Foundation.h>

@implementation AGUnixSocketAddress

+ (id)addressWithPath:(NSString *)path {
	return [[[self alloc] initWithPath:path] autorelease];
}

+ (id)addressWithUnixSocketAddress:(const struct sockaddr_un *)addr {
	return [[[self alloc] initWithUnixSocketAddress:addr] autorelease];
}

- (id)init {
	return [self initWithPath:@""];
}

- (id)initWithPath:(NSString *)path {
	struct sockaddr_un addr;
	memset(&addr, 0, sizeof(struct sockaddr_un));
	addr.sun_family = AF_UNIX;
	strncpy(address.sun_path, [path UTF8String], sizeof(address.sun_path) - 1);
	return [self initWithUnixSocketAddress:&addr];
}

- (id)initWithUnixSocketAddress:(const struct sockaddr_un *)addr {
	if (self = [super init])
		[self setUnixSocketAddress:addr];
	return self;
}

- (const struct sockaddr_un *)unixSocketAddress {
	return &address;
}

- (void)setUnixSocketAddress:(const struct sockaddr_un *)addr {
	memcpy(&address, addr, SUN_LEN(addr));
}

- (NSString *)path {
	return [NSString stringWithUTF8String:address.sun_path];
}

- (void)setPath:(NSString *)path {
	strncpy(address.sun_path, [path UTF8String], sizeof(address.sun_path) - 1);
}

- (void)setSocketAddress:(const struct sockaddr *)addr length:(unsigned)len {
	if (addr->sa_family != AF_UNIX)
		[NSException raise:NSInvalidArgumentException format:@"invalid address family"];
	[self setUnixSocketAddress:(struct sockaddr_un *)addr];
}

- (const struct sockaddr *)socketAddress {
	return (struct sockaddr *)&address;
}

- (unsigned)length {
	return SUN_LEN(&address);
}

- (NSString *)description {
	return [NSString stringWithFormat:@"%@ %@", [super description], [self path]];
}

@end

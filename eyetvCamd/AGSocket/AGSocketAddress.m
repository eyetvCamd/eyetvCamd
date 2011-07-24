// AGSocketAddress.m
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
#import <Foundation/Foundation.h>

@interface AGConcreteSocketAddress : AGSocketAddress {
	struct sockaddr *address;
	int length;
}

@end

@implementation AGSocketAddress

+ (id)allocWithZone:(NSZone *)zone {
	return NSAllocateObject(self == [AGSocketAddress class] ? [AGConcreteSocketAddress class] : self, 0, zone);
}

- (id)copyWithZone:(NSZone *)zone {
	return [self mutableCopyWithZone:zone];
}

- (id)mutableCopyWithZone:(NSZone *)zone {
	return [[[self class] alloc] initWithAddress:self];
}

+ (id)addressWithAddress:(AGSocketAddress *)addr {
	return [[[self alloc] initWithAddress:addr] autorelease];
}
	
+ (id)addressWithSocketAddress:(const struct sockaddr *)addr length:(unsigned)len {
	return [[[self alloc] initWithSocketAddress:addr length:len] autorelease];
}

- (id)initWithAddress:(AGSocketAddress *)addr {
	return [self initWithSocketAddress:[addr socketAddress] length:[addr length]];
}

- (id)initWithSocketAddress:(const struct sockaddr *)addr length:(unsigned)len {
	if (self = [super init])
		[self setSocketAddress:addr length:len];
	return self;
}

- (int)family {
	return [self socketAddress]->sa_family;
}

- (BOOL)isEqual:(id)obj {
	if (![obj isKindOfClass:[AGSocketAddress class]])
		return NO;
	if ([self length] != [obj length])
		return NO;
	const uint8_t *selfp = (uint8_t *)[self socketAddress], *otherp = (uint8_t *)[obj socketAddress];
	int len = [self length];
	int i;
	for (i = sizeof([self socketAddress]->sa_len); i < len; i++) // skip sa_len
		if (selfp[i] != otherp[i])
			return NO;
	return YES;
}

- (void)setSocketAddress:(const struct sockaddr *)addr length:(unsigned)len
{
}

- (const struct sockaddr *)socketAddress
{
	return Nil;
}


- (unsigned)length
{
	return 0;
}


@end

@implementation AGConcreteSocketAddress

- (id)init {
	struct sockaddr addr;
	memset(&addr, 0, sizeof(struct sockaddr));
	return [self initWithSocketAddress:&addr length:sizeof(struct sockaddr)];
}

- (id)initWithSocketAddress:(const struct sockaddr *)addr length:(unsigned)len {
	if (self = [super init]) {
		if (!(address = malloc(SOCK_MAXADDRLEN))) {
			[self release];
			return nil;
		}
		[self setSocketAddress:addr length:len];
	}
	return self;
}

- (void)dealloc {
	free(address);
	[super dealloc];
}

- (void)setSocketAddress:(const struct sockaddr *)addr length:(unsigned)len {
	memcpy(address, addr, len);
	length = len;
}

- (const struct sockaddr *)socketAddress {
	return address;
}

- (unsigned)length {
	return length;
}

@end
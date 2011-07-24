// AGSocket.m
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

#import <AGSocket.h>
#import <AGSocketAddress.h>
#import <Foundation/Foundation.h>
#include <sys/ioctl.h>
#include <sys/signal.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <unistd.h>

NSString *const AGSocketOperationException = @"AGSocketOperationException";
NSString *const AGSocketBrokenPipeException = @"AGSocketBrokenPipeException";

static void AGSocketCallBack(CFSocketRef native, CFSocketCallBackType type, CFDataRef addr, const void *data, void *info);

@interface AGSocket (Private)
// designated initializer
- (id)initWithSocket:(int)sock;
// CFSocket callbacks
- (void)socketConnectCallBack:(const void *)data;
- (void)socketReadCallBack:(const void *)data;
- (void)socketWriteCallBack:(const void *)data;
@end

@implementation AGSocket (Private)

- (id)initWithSocket:(int)sock {
	if (self = [super init]) {
		// assign socket
		if ((native = sock) < 0) {
			[self release];
			return nil;
		}
		// create local and remote address
		localAddress = [[AGSocketAddress alloc] init];
		remoteAddress = [[AGSocketAddress alloc] init];
		// create CFSocket
		CFSocketContext context = { 0, self, NULL, NULL, NULL };
		if (!(cfSocket = CFSocketCreateWithNative(NULL, native, kCFSocketReadCallBack | kCFSocketWriteCallBack | kCFSocketConnectCallBack, AGSocketCallBack, &context))) {
			[self release];
			return nil;
		}
		// turn off kCFSocketAutomaticallyReenableRead/WriteCallBack and kCFSocketCloseNativeOnInvalidate
		CFSocketSetSocketFlags(cfSocket, 0);
		// add to run loop
		CFRunLoopSourceRef source;
		if (!(source = CFSocketCreateRunLoopSource(NULL, cfSocket, 0))) {
			[self release];
			return nil;
		}
		CFRunLoopAddSource(CFRunLoopGetCurrent(), source, kCFRunLoopDefaultMode);
		CFRelease(source);
	}
	return self;
}

- (void)socketConnectCallBack:(const void *)data {
	if (data) { // connect failed
		state = AGSocketStateNone;
		error = *(int *)data;
		if ([delegate respondsToSelector:@selector(socketConnectFailed:)])
			[delegate socketConnectFailed:self];
	} else {
		state = AGSocketStateConnected;
		error = 0;
		if ([delegate respondsToSelector:@selector(socketConnected:)])
			[delegate socketConnected:self];
	}
}

- (void)socketReadCallBack:(const void *)data {
	if (state == AGSocketStateListening) {
		// accept the incoming connection
		AGSocket *child;
		if (!(child = [[AGSocket alloc] initWithSocket:accept(native, NULL, NULL)]))
			[NSException raise:AGSocketOperationException format:@"error accept(): %s", strerror(errno)];
		child->state = AGSocketStateConnected;
		if ([delegate respondsToSelector:@selector(socket:acceptedChild:)])
			[delegate socket:self acceptedChild:child];
		[child release];
		// reenable callback
		CFSocketEnableCallBacks(cfSocket, kCFSocketReadCallBack);
	} else
		if ([delegate respondsToSelector:@selector(socketBecameReadable:)])
			[delegate socketBecameReadable:self];
}

- (void)socketWriteCallBack:(const void *)data {
	if ([delegate respondsToSelector:@selector(socketBecameWritable:)])
		[delegate socketBecameWritable:self];
}	

@end

@implementation AGSocket

+ (void)initialize {
	static BOOL initialized = NO;
	if (initialized) return;
	initialized = YES;
	[super initialize];
	signal(SIGPIPE, SIG_IGN);
}

+ (id)tcpSocket {
	return [[[self alloc] initWithFamily:PF_INET type:SOCK_STREAM protocol:0] autorelease];
}

+ (id)udpSocket {
	return [[[self alloc] initWithFamily:PF_INET type:SOCK_DGRAM protocol:0] autorelease];
}

- (id)init {
	return [self initWithFamily:PF_INET type:SOCK_STREAM protocol:0];
}

- (id)initWithFamily:(int)family type:(int)type protocol:(int)protocol {
	return [self initWithSocket:socket(family, type, protocol)];
}

- (void)dealloc {
	[self close];
	if (cfSocket) CFRelease(cfSocket);
	[localAddress release];
	[remoteAddress release];
	[super dealloc];
}

// need to figure out how to get the error code for a failed CFSocketConnectToAddress() with a positive (blocking) timeout, usually errno is just EINPROGRESS
// plus, here is a weird problem, CFSocketConnectToAddress() with a positive (blocking) timeout usually returns kCFSocketSuccess when I try to connect to a machine on my local net but with a bad port number, e.g, host 10.0.1.3 on port 8000. However, immediately afterwards I get a callback with error of ECONNREFUSED. This doesn't happen when trying to connect to hosts outside my local net.
- (BOOL)connectToAddress:(AGSocketAddress *)addr timeout:(int)sec {
	CFDataRef data;
	if (!(data = CFDataCreate(NULL, (UInt8 *)[addr socketAddress], [addr length]))) {
		error = ENOMEM;
		return NO;
	}
	BOOL success = CFSocketConnectToAddress(cfSocket, data, sec) == kCFSocketSuccess;
	CFRelease(data);
	// record error code
	error = success ? 0 : errno;
	// set the socket state if it is a stream type socket
	int type;
	socklen_t len = sizeof(int);
	getsockopt(native, SOL_SOCKET, SO_TYPE,&type, &len);
	if (success && type & (SOCK_STREAM | SOCK_SEQPACKET))
		state = sec < 0 ? AGSocketStateConnecting : AGSocketStateConnected;
	return success;
}

- (BOOL)connectToAddress:(AGSocketAddress *)addr {
	return [self connectToAddress:addr timeout:90];
}

- (BOOL)connectToAddressInBackground:(AGSocketAddress *)addr {
	return [self connectToAddress:addr timeout:-1];
}

- (BOOL)listenOnAddress:(AGSocketAddress *)addr {
	CFDataRef data;
	if (!(data = CFDataCreate(NULL, (UInt8 *)[addr socketAddress], [addr length]))) {
		error = ENOMEM;
		return NO;
	}
	BOOL success = CFSocketSetAddress(cfSocket, data) == kCFSocketSuccess;
	CFRelease(data);
	// record error code
	error = success ? 0 : errno;
	// set the socket state if it is a stream type socket
	int type;
	socklen_t len = sizeof(int);
	getsockopt(native, SOL_SOCKET, SO_TYPE, &type, &len);
	if (success && type & (SOCK_STREAM | SOCK_SEQPACKET))
		state = AGSocketStateListening;
	return success;
}

- (void)close {
	if (cfSocket) CFSocketInvalidate(cfSocket);
	close(native);
	native = -1;
	state = AGSocketStateClosed;
}

- (AGSocketAddress *)localAddress {
	struct sockaddr *addr;
	socklen_t len = SOCK_MAXADDRLEN;
	if (!(addr = malloc(len)))
		return localAddress;
	if (getsockname(native, addr, &len) == 0)
		[localAddress setSocketAddress:addr length:len];
	free(addr);
	return localAddress;
}

- (AGSocketAddress *)remoteAddress {
	struct sockaddr *addr;
	socklen_t len = SOCK_MAXADDRLEN;
	if (!(addr = malloc(len)))
		return remoteAddress;
	if (getpeername(native, addr, &len) == 0)
		[remoteAddress setSocketAddress:addr length:len];
	free(addr);
	return remoteAddress;
}

- (id)delegate {
	return delegate;
}

- (void)setDelegate:(id)obj {
	delegate = obj;
}

- (int)readBytes:(void *)bytes ofLength:(int)length {
	return [self readBytes:bytes ofLength:length fromAddress:nil withFlags:0];
}

- (int)readBytes:(void *)bytes ofLength:(int)len fromAddress:(AGSocketAddress *)addr {
	return [self readBytes:bytes ofLength:len fromAddress:addr withFlags:0];
}

- (int)readBytes:(void *)bytes ofLength:(int)length fromAddress:(AGSocketAddress *)addr withFlags:(int)flags {
	struct sockaddr *addrbuf;
	socklen_t addrlen = SOCK_MAXADDRLEN;
	if (!(addrbuf = malloc(addrlen))) {
		error = errno; // ENOMEM
		[NSException raise:AGSocketOperationException format:@"%s", strerror(error)];
	}
	int recvd = recvfrom(native, bytes, length, flags, addrbuf, &addrlen);
	// record error code
	error = recvd < 0 ? errno : 0;
	// set address
	if (!error)
		[addr setSocketAddress:addrbuf length:addrlen];
	// free addrbuf
	free(addrbuf);
	// reenable callback unless we found EOF
	if (recvd != 0)
		CFSocketEnableCallBacks(cfSocket, kCFSocketReadCallBack);
	// raise exception on error
	if (error)
		[NSException raise:AGSocketOperationException format:@"%s", strerror(error)];
	return recvd;
}

- (NSData *)readData {
	return [self readDataOfLength:[self bytesAvailable] + 1 fromAddress:nil withFlags:0];
}

- (NSData *)readDataFromAddress:(AGSocketAddress *)addr {
	return [self readDataOfLength:[self bytesAvailable] + 1 fromAddress:addr withFlags:0];
}

- (NSData *)readDataOfLength:(int)length {
	return [self readDataOfLength:length fromAddress:nil withFlags:0];
}

- (NSData *)readDataOfLength:(int)length fromAddress:(AGSocketAddress *)addr {
	return [self readDataOfLength:length fromAddress:addr withFlags:0];
}

- (NSData *)readDataOfLength:(int)length fromAddress:(AGSocketAddress *)addr withFlags:(int)flags {
	char *bytes;
	if (!(bytes = malloc(length))) {
		error = errno; // ENOMEM
		[NSException raise:AGSocketOperationException format:@"%s", strerror(error)];
	}
	int read;
	NS_DURING
		read = [self readBytes:bytes ofLength:length fromAddress:addr withFlags:flags];
	NS_HANDLER
		// free buffer and raise
		free(bytes);
		[localException raise];
	NS_ENDHANDLER
	return [NSData dataWithBytesNoCopy:bytes length:read];
}

- (int)writeBytes:(const void *)bytes ofLength:(int)length {
	return [self writeBytes:bytes ofLength:length toAddress:nil withFlags:0];
}

- (int)writeBytes:(const void *)bytes ofLength:(int)len toAddress:(AGSocketAddress *)addr {
	return [self writeBytes:bytes ofLength:len toAddress:addr withFlags:0];
}

- (int)writeBytes:(const void *)bytes ofLength:(int)len toAddress:(AGSocketAddress *)addr withFlags:(int)flags {
	int sent = sendto(native, bytes, len, flags, [addr socketAddress], [addr length]);
	// record error code
	error = sent < 0 ? errno : 0;
	// reenable callback unless we got EPIPE
	if (error != EPIPE)
		CFSocketEnableCallBacks(cfSocket, kCFSocketWriteCallBack);
	// raise for EPIPE
	if (error == EPIPE)
		[NSException raise:AGSocketBrokenPipeException format:@"%s", strerror(error)];
	// raise for other error
	if (error)
		[NSException raise:AGSocketOperationException format:@"%s", strerror(error)];
	return sent;
}

- (NSData *)writeData:(NSData *)data {
	return [self writeData:data toAddress:nil withFlags:0];
}

- (NSData *)writeData:(NSData *)data toAddress:(AGSocketAddress *)addr {
	return [self writeData:data toAddress:addr withFlags:0];
}

- (NSData *)writeData:(NSData *)data toAddress:(AGSocketAddress *)addr withFlags:(int)flags {
	int wrote, length = [data length];
	wrote = [self writeBytes:[data bytes] ofLength:length toAddress:addr withFlags:flags];
	return [data subdataWithRange:NSMakeRange(wrote, length - wrote)];
}

- (int)bytesAvailable {
	int avail;
	if (ioctl(native, FIONREAD, &avail) != 0)
		return 0;
	return avail;
}

- (int)error {
	return error;
}

- (BOOL)isReadable {
	if (native < 0) return NO;
	fd_set fdset;
	struct timeval tmout = { 0, 0 };
	FD_ZERO(&fdset);
	FD_SET(native, &fdset);
	return select(native + 1, &fdset, NULL, NULL, &tmout) > 0;
}
	
- (BOOL)isWritable {
	if (native < 0) return NO;
	fd_set fdset;
	struct timeval tmout = { 0, 0 };
	FD_ZERO(&fdset);
	FD_SET(native, &fdset);
	return select(native + 1, NULL, &fdset, NULL, &tmout) > 0;
}

- (AGSocketState)state {
	return state;
}

- (int)socket {
	return native;
}

- (NSString *)description {
	return [NSString stringWithFormat:@"%@ {\n\
	socket = %d,\n\
	state = %d,\n\
	cfSocket = 0x%x,\n\
	localAddress = %@,\n\
	remoteAddress = %@,\n\
	delegate = %@,\n\
	isReadable = %d,\n\
	isWritable = %d,\n\
	bytesAvailable = %d,\n\
	error = %s\n\
}", 
	[super description],
	[self socket],
	[self state],
	cfSocket,
	[self localAddress],
	[self remoteAddress],
	[self delegate],
	[self isReadable],
	[self isWritable],
	[self bytesAvailable],
	strerror([self error])];
}

@end

static void AGSocketCallBack(CFSocketRef native, CFSocketCallBackType type, CFDataRef addr, const void *data, void *info) {
	AGSocket *client = (AGSocket *)info;
	switch (type) {
	case kCFSocketConnectCallBack:
		[client socketConnectCallBack:data];
		break;
	case kCFSocketReadCallBack:
		[client socketReadCallBack:data];
		break;
	case kCFSocketWriteCallBack:
		[client socketWriteCallBack:data];
		break;
	}
}
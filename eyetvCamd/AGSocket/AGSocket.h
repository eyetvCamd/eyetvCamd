// AGSocket.h
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

#import "AGInet6SocketAddress.h"
#import "AGInetSocketAddress.h"
#import "AGSocketAddress.h"
#import "AGUnixSocketAddress.h"
#import <CoreFoundation/CFSocket.h>
#import <Foundation/NSObject.h>
#import <sys/types.h>
#import <sys/socket.h>

/*!
@enum AGSocketState
Possible return values for -[AGSocket state]. Most of these are only relevant for stream sockets, a non-stream socket can only have a state of AGSocketStateNone and AGSocketStateClosed.
@constant AGSocketStateNone The socket is ready.
@constant AGSocketStateConnecting The socket is trying an outgoing connection.
@constant AGSocketStateConnected The socket is connected.
@constant AGSocketStateListening The socket is listening for incoming connections.
@constant AGSocketStateClosed The socket is closed. */
typedef enum _AGSocketState {
	AGSocketStateNone = 0,
	AGSocketStateConnecting, 
	AGSocketStateConnected, 
	AGSocketStateListening, 
	AGSocketStateClosed
} AGSocketState;

@class AGSocketAddress, NSData;

/*!
@class AGSocket
@abstract A wrapper class for BSD sockets and CFSocket.
@discussion AGSocket is a wrapper class for BSD sockets and is the main class in the AGSocket framework. It can be used for TCP sockets, UDP sockets, raw sockets, whatever.

An AGSocket is created with -initWithFamily:type:protocol:, which is analogous to socket(). Since most of the time you probably just want a TCP or UDP socket, the convenience methods +tcpSocket and +udpSocket are provided.

An AGSocket will deliver a delegate message when the following events occur: the socket became readable, the socket became writable, the socket completed or failed to complete a connection to a remote socket, or the socket accepted an incoming connection. Not all events apply to all types of sockets, obviously. You are not required to set the delegate, nor is the delegate required to implement any of the delegate methods, if you do not want to be notified of events.
*/
@interface AGSocket : NSObject {
	int native, state, error;
	CFSocketRef cfSocket;
	AGSocketAddress *localAddress, *remoteAddress;
	id delegate;
}

/*!
@method tcpSocket
Creates a new TCP socket. */
+ (id)tcpSocket;

/*!
@method udpSocket
Creates a new UDP socket. */
+ (id)udpSocket;

/*!
@method initWithFamily:type:protocol:
Analog of socket(), initializes the receiver with a new socket of <i>family</i>, <i>type</i>, and <i>protocol</i>. For example, the following code creates a TCP socket:

<code>&nbsp;&nbsp;&nbsp;&nbsp;AGSocket *s = [[AGSocket alloc] initWithFamily:PF_INET type:SOCK_STREAM protocol:0];</code> */
- (id)initWithFamily:(int)family type:(int)type protocol:(int)protocol;

/*!
@method connectToAddress:
See -connectToAddress:timeout:. */
- (BOOL)connectToAddress:(AGSocketAddress *)addr;

/*!
@method connectToAddress:timeout:
If the receiver is stream-oriented, like TCP, the socket will attempt to connect to the specified address before the timeout expires. A timeout of 0 will allow the call to block indefinitely.

If the receiver is not stream-oriented, like UDP, this just sets the destination address for future writes so you don't have to specifiy the destination every time. 

Returns NO on failure, in which case you can get the reason for the failure from -error.*/
- (BOOL)connectToAddress:(AGSocketAddress *)addr timeout:(int)sec;

/*!
@method connectToAddressInBackground:
Same as -connectToAddress:timeout:, except it will return immediately and attempt the connection in the background. If it is possible to attempt the connection, returns YES and the delegate will be sent a -socketConnected: or -socketConnectFailed: message depending on the result. Returns NO if it is not possible to attempt the connection. */
- (BOOL)connectToAddressInBackground:(AGSocketAddress *)addr;

/*!
@method listenOnAddress:
If the receiver is stream-oriented, like TCP, the socket will listen for incoming connections on the given local address and send the delegate a -socket:acceptedChild: message when each new socket is accepted.

If the receiver is not stream-oriented, like UDP, this binds the socket to the given local address.

Returns NO on failure, in which case you can get the reason for the failure from -error.*/
- (BOOL)listenOnAddress:(AGSocketAddress *)addr;

/*!
@method close
Closes the socket and turns off all callbacks. This is done automatically by -dealloc, but go ahead and call it anyway if you want to. */
- (void)close;

/*!
@method localAddress
Returns the local socket address. */
- (AGSocketAddress *)localAddress;

/*!
@method remoteAddress
Returns the remote socket address. */
- (AGSocketAddress *)remoteAddress;

/*!
@method delegate
Returns the socket delegate. */
- (id)delegate;

/*!
@method setDelegate:
Sets the socket delegate to <i>obj</i>. */
- (void)setDelegate:(id)obj;

/*!
@method readBytes:ofLength:
See -readBytes:ofLength:fromAddress:withFlags:.*/
- (int)readBytes:(void *)bytes ofLength:(int)len;

/*!
@method readBytes:ofLength:fromAddress:
See -readBytes:ofLength:fromAddress:withFlags:.*/
- (int)readBytes:(void *)bytes ofLength:(int)len fromAddress:(AGSocketAddress *)addr;

/*!
@method readBytes:ofLength:fromAddress:withFlags:
Reads up to <i>len</i> bytes into buffer <i>bytes</i> and returns the actual number of bytes read. A return value of zero indicates end of file, otherwise this will block until at least one byte can be read. If <i>addr</i> is not nil, it will be set to the return address of the message. <i>flags</i> may include any of the flags defined for recvfrom(). 

Raises AGSocketOperationException on error. */
- (int)readBytes:(void *)bytes ofLength:(int)len fromAddress:(AGSocketAddress *)addr withFlags:(int)flags;

/*!
@method readData
See -readDataOfLength:fromAddress:withFlags:. */
- (NSData *)readData;

/*!
@method readDataFromAddress:
See -readDataOfLength:fromAddress:withFlags:. */
- (NSData *)readDataFromAddress:(AGSocketAddress *)addr;

/*!
@method readDataOfLength:
See -readDataOfLength:fromAddress:withFlags:. */
- (NSData *)readDataOfLength:(int)len;

/*!
@method readDataOfLength:fromAddress:
See -readDataOfLength:fromAddress:withFlags:. */
- (NSData *)readDataOfLength:(int)len fromAddress:(AGSocketAddress *)addr;

/*!
@method readDataOfLength:fromAddress:withFlags:
Reads up to <i>len</i> bytes and returns the bytes as an NSData. An empty data indicates end of file, otherwise this will block until at least one byte can be read. If <i>addr</i> is not nil, it will be set to the return address of the message. <i>flags</i> may include any of the flags defined for recvfrom(). 

Raises AGSocketOperationException on error. */
- (NSData *)readDataOfLength:(int)len fromAddress:(AGSocketAddress *)addr withFlags:(int)flags;

/*!
@method writeBytes:ofLength:
See -writeBytes:ofLength:toAddress:withFlags:. */
- (int)writeBytes:(const void *)bytes ofLength:(int)len;

/*!
@method writeBytes:ofLength:toAddress:
See -writeBytes:ofLength:toAddress:withFlags:. */
- (int)writeBytes:(const void *)bytes ofLength:(int)len toAddress:(AGSocketAddress *)addr;

/*!
@method writeBytes:ofLength:toAddress:withFlags:
Writes up to <i>len</i> bytes from buffer <i>bytes</i> and returns the actual number written. <i>addr</i> may be nil if -connectToAddress:... has been called previously. <i>flags</i> may include any of the flags defined for sendto(). 

On error, raises AGSocketBrokenPipeException if the error is because the remote side has shut down, AGSocketOperationException otherwise. */
- (int)writeBytes:(const void *)bytes ofLength:(int)len toAddress:(AGSocketAddress *)addr withFlags:(int)flags;

/*!
@method writeData:
See -writeData:toAddress:withFlags: */
- (NSData *)writeData:(NSData *)data;

/*!
@method writeData:toAddress:
See -writeData:toAddress:withFlags: */
- (NSData *)writeData:(NSData *)data toAddress:(AGSocketAddress *)addr;

/*!
@method writeData:toAddress:withFlags:
Writes <i>data</i> to the socket. If the entire data could not be sent, the unwritten part is returned, otherwise an empty data is returned. <i>addr</i> may be nil if -connectToAddress: has been called. <i>flags</i> may include any of the flags defined for sendto(). 

On error, raises AGSocketBrokenPipeException if the error is because the remote side has shut down, AGSocketOperationException otherwise. */
- (NSData *)writeData:(NSData *)data toAddress:(AGSocketAddress *)addr withFlags:(int)flags;

/*!
@method bytesAvailable
Returns the number of bytes available for reading at the socket. Don't use this method as a substitute for -isReadable, because it will return zero both when the socket is unreadable and when it can be read for end of file. */
- (int)bytesAvailable;

/*!
@method error
Returns the POSIX error code associated with the last socket operation. */
- (int)error;

/*!
@method isReadable
Returns YES if the socket can be read from without blocking. */
- (BOOL)isReadable;

/*!
@method isWritable
Returns YES if the socket can be written to without blocking. */
- (BOOL)isWritable;

/*!
@method state
Returns the current state of the socket, see AGSocketState for possible values. If the socket is not stream-oriented, like UDP, the state can be only AGSocketStateNone or AGSocketStateClosed. */
- (AGSocketState)state;

/*!
@method socket
Returns the file descriptor of the underlying socket. Don't use this for stuff an instance method exists for, like reading, writing, connecting, etc., since this will mess up the delivery of delegate messages. It is OK to use it for other stuff though, like getsockopt() and setsockopt(), or if you don't care about delegate messages. */
- (int)socket;

@end

/*!
@category NSObject(AGSocketDelegate)
@abstract Methods implemented by the socket delegate.
@discussion Methods implemented by the socket delegate. It is not necessary to implement all methods declared here, the delegate need only implement the methods it is interested in. */
@interface NSObject (AGSocketDelegate)

/*!
@method socketConnected:
The socket will deliver this message when -connectToAddressInBackground: successfully completes a connection. */
- (void)socketConnected:(AGSocket *)sock;

/*!
@method socketConnectFailed:
The socket will deliver this message when -connectToAddressInBackground: fails to complete a connection. The reason for the failure will be available from -error. */
- (void)socketConnectFailed:(AGSocket *)sock;

/*!
@method socket:acceptedChild:
The socket will deliver this message when -listenOnAddress: has accepted a new connection. The new socket is passed as <i>child</i>. */
- (void)socket:(AGSocket *)sock acceptedChild:(AGSocket *)child;

/*!
@method socketBecameReadable:
The socket will deliver this message when data is available at the socket for reading. */
- (void)socketBecameReadable:(AGSocket *)sock;

/*!
@method socketBecameWritable:
The socket will deliver this message when space becomes available at the socket for writing. */
- (void)socketBecameWritable:(AGSocket *)sock;
@end

#ifdef __cplusplus
	#define AG_EXPORT extern "C"
#endif

#ifndef AG_EXPORT
	#define AG_EXPORT extern
#endif

/*!
@const AGSocketOperationException
Raised if a read or write error occurs. */
AG_EXPORT NSString *const AGSocketOperationException;

/*!
@const AGSocketBrokenPipeException
Raised if a write error occurs due to a broken pipe, i.e., the remote side has shut down. */
AG_EXPORT NSString *const AGSocketBrokenPipeException;
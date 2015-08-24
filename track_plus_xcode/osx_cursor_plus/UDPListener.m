//
//  UDPListener.m
//  cursorMaster
//
//  Created by Corey Manders on 9/2/15.
//  Copyright (c) 2015 Ractiv Pte. Ltd. All rights reserved.
//

#import "UDPListener.h"
#define MYPORT 2002
@implementation UDPListener



-(void)myInit{
    self.udpSocket = [[GCDAsyncUdpSocket alloc] initWithDelegate:self delegateQueue:dispatch_get_main_queue()];
    
    self.udpSocket.delegate = self;
    NSError *error = nil;
    
    
    if (![self.udpSocket bindToPort:MYPORT error:&error])
    {
        NSLog(@"Error binding: %@", error);
        return;
    }
    if (![self.udpSocket beginReceiving:&error])
    {
        NSLog(@"Error receiving: %@", error);
        return;
    }
    NSLog(@"Set up to receive data on port %d",MYPORT );
}

-(void)checkConnection{
      NSLog(@"connected = %d",self.udpSocket.isConnected);
    
}

- (void)udpSocket:(GCDAsyncUdpSocket *)sock didReceiveData:(NSData *)data
      fromAddress:(NSData *)address
withFilterContext:(id)filterContext
{
    NSString *msg = [[NSString alloc] initWithData:data encoding:NSUTF8StringEncoding];
    NSLog(@"RECEIVED");
    if (msg)
    {
        NSLog(@"RECV: %@", msg);
        CGDisplayMoveCursorToPoint(0, CGPointMake(500, 0));
    }
    else
    {
        NSString *host = nil;
        uint16_t port = 0;
        [GCDAsyncUdpSocket getHost:&host port:&port fromAddress:address];
        
        NSLog(@"RECV: Unknown message from: %@:%hu", host, port);
    }
}

@end

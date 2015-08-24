//
//  UDPListener.h
//  cursorMaster
//
//  Created by Corey Manders on 9/2/15.
//  Copyright (c) 2015 Ractiv Pte. Ltd. All rights reserved.
//

#import <Foundation/Foundation.h>
#import "GCDAsyncUdpSocket.h"

@interface UDPListener : NSObject

@property (strong)GCDAsyncUdpSocket * udpSocket;

-(void)myInit;
-(void)checkConnection;
@end

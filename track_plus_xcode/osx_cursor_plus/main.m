//
//  main.m
//  osx_cursor_plus
//
//  Created by Lai Xue on 8/11/15.
//  Copyright (c) 2015 Ractiv Inc. All rights reserved.
//

#import <Cocoa/Cocoa.h>
#import <ApplicationServices/ApplicationServices.h>
#import <Foundation/Foundation.h>
#import <QuartzCore/QuartzCore.h>
#import <Quartz/Quartz.h>
#import "UDPListener.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <arpa/inet.h>


#define TM_BUF_SIZE 1400
#define TM_PACKETS_TO_SEND 10
#define TM_DEST_ADDR “127.0.0.1”
#define TM_DEST_PORT 33000
char testBuffer[TM_BUF_SIZE];


void putMouseDown(CGPoint currentLoc){
    CGEventRef click1_down = CGEventCreateMouseEvent(
                                                     NULL, kCGEventLeftMouseDown,
                                                     currentLoc,
                                                     kCGMouseButtonLeft);
    CGEventPost(kCGHIDEventTap, click1_down);
    
}

void putMouseUp(CGPoint currentLoc){
    CGEventRef click1_up = CGEventCreateMouseEvent(
                                                   NULL, kCGEventLeftMouseUp,
                                                   currentLoc,
                                                   kCGMouseButtonLeft);
    CGEventPost(kCGHIDEventTap, click1_up);
}

void scrollMouse(CGPoint currentLoc, int scrollAmount){
    
    
    CGEventRef cgEvent = CGEventCreateScrollWheelEvent(NULL, kCGScrollEventUnitLine, 1, scrollAmount);
    
    // You can post the CGEvent to the event stream to have it automatically sent to the window under the cursor
    CGEventPost(kCGHIDEventTap, cgEvent);
    
    // NSEvent *theEvent = [NSEvent eventWithCGEvent:cgEvent];
    CFRelease(cgEvent);
}


int regSocket;
int addrLen;
int testSocket;
struct sockaddr_in sourceAddr;
struct sockaddr_in servaddr,cliaddr;

CGEventRef myCGEventCallback(CGEventTapProxy proxy, CGEventType type,  CGEventRef event, void *refcon) {
    CGKeyCode keycode = (CGKeyCode)CGEventGetIntegerValueField(event, kCGKeyboardEventKeycode);
  //  printf("%u type %u\n", keycode,type);
    char sendline[3];
    sourceAddr.sin_port=htons(32001);
    sendline[0] = keycode;
    sendline[1] = type;
    sendline[2]= '\n';
    sendto(testSocket,sendline,strlen(sendline),0,
           (struct sockaddr *)&sourceAddr,sizeof(sourceAddr));
    return event;
}
/*
void startKeyListener(){
    // set up a key listerner
    
    //UDPListener * udpl = [[UDPListener alloc] init];
    testSocket= socket(AF_INET, SOCK_DGRAM, 0);
    NSLog(@"caught");
    bzero(&servaddr, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr=inet_addr("127.0.0.1");
    servaddr.sin_port = htons(32001);
    
    CFMachPortRef eventTap;
    CFRunLoopSourceRef runLoopSource;
    CGEventMask mask = CGEventMaskBit(kCGEventKeyDown);
    eventTap = CGEventTapCreate(kCGSessionEventTap, kCGHeadInsertEventTap, 0, mask, myCGEventCallback, NULL);
    runLoopSource = CFMachPortCreateRunLoopSource(kCFAllocatorDefault, eventTap, 0);
    CFRunLoopAddSource(CFRunLoopGetCurrent(), runLoopSource, kCFRunLoopCommonModes);
    CGEventTapEnable(eventTap, true);
    CFRunLoopRun();
}
*/
int main(int argc, const char * argv[]) {
    NSPoint mouseLoc;
    mouseLoc = [NSEvent mouseLocation]; //get current mouse position
    NSLog(@"Mouse location: %f %f", mouseLoc.x, mouseLoc.y);
    
    
    unsigned int counter;
    struct sockaddr_in destAddr;
    int errorCode;
    int returnVal;
    
    counter = 0;
    returnVal = 0;
    regSocket = socket(AF_INET, SOCK_DGRAM, 0);
    if (regSocket<0){
        fprintf(stderr,"error opening rec socket\n");
        exit(1);
    }
    unsigned int alen = sizeof(destAddr);
    
    
    destAddr.sin_family = AF_INET;
    destAddr.sin_port = htons(33000);
    destAddr.sin_addr.s_addr =  htonl(INADDR_ANY);
    errorCode = bind(regSocket,  (struct sockaddr *)&destAddr, sizeof(destAddr));
    
    
    if (getsockname(regSocket, (struct sockaddr *)&destAddr, &alen) < 0) {
        perror("getsockname failed");
        return 0;
    }
    
    
    printf("bind complete. Port number = %d\n", ntohs(destAddr.sin_port));
    BOOL mouseIsDown = NO;
    dispatch_async(dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0), ^{
    
        testSocket= socket(AF_INET, SOCK_DGRAM, 0);
        
        bzero(&servaddr, sizeof(servaddr));
        servaddr.sin_family = AF_INET;
        servaddr.sin_addr.s_addr=inet_addr("127.0.0.1");
        servaddr.sin_port = htons(32001);
        
        CFMachPortRef eventTap;
        CFRunLoopSourceRef runLoopSource;
        CGEventMask mask = CGEventMaskBit(kCGEventKeyDown)|CGEventMaskBit(kCGEventKeyUp);
        eventTap = CGEventTapCreate(kCGSessionEventTap, kCGHeadInsertEventTap, 0, mask, myCGEventCallback, NULL);
        runLoopSource = CFMachPortCreateRunLoopSource(kCFAllocatorDefault, eventTap, 0);
        CFRunLoopAddSource(CFRunLoopGetCurrent(), runLoopSource, kCFRunLoopCommonModes);
        CGEventTapEnable(eventTap, true);
        CFRunLoopRun();
        
    
    });
   
    fprintf(stderr,"error code = %d\n",errorCode);
    /* Make sure the socket was created successfully */
    
    
    //   CFRunLoopRun();
    fprintf(stderr,"About to enter run loop\n");
    // [udpl myInit];
    int scrollLoc = -1;
    while(1){
        addrLen = sizeof(sourceAddr);
        fprintf(stderr,"BEFORE RECEIVE\n ");
        errorCode = recvfrom(regSocket,
                             testBuffer,
                             TM_BUF_SIZE,
                             0,
                             (struct sockaddr *) &sourceAddr,
                             (socklen_t *)&addrLen);
        fprintf(stderr,"AFTER RECEIVE ERROR CODE = %d\n",errorCode);
        printf("--> %s  ####\n",testBuffer   );
        NSString * myString  = [NSString stringWithUTF8String:testBuffer];
        
        NSArray *array = [myString componentsSeparatedByCharactersInSet:[NSCharacterSet newlineCharacterSet]];
        int counter = 0;
        
        fprintf(stderr,"Number of array components = %ld\n",array.count);
        int indexXVal= -1, indexYVal=-1, indexPlane=-1;
        int middleXVal=-1, middleYVal=-1, middlePlane=-1;
        
        for (NSString * line in array){
            
            NSArray * subline = [line componentsSeparatedByString:@"!"];
            if (subline.count >= 4){
                if ([subline[3] isEqualToString:@"index"]){
                    NSLog(@"GOT IT -- %@",line);
                    indexXVal = [subline[0] intValue];
                    indexYVal = [subline[1] intValue];
                    indexPlane = [subline[2] intValue];
                    CGDisplayMoveCursorToPoint(0, CGPointMake(indexXVal, indexYVal));
                }
                else if ([subline[0] isEqualToString:@"middle"]){
                    middleXVal = [subline[0] intValue];
                    middleYVal = [subline[1] intValue];
                    middlePlane = [subline[2] intValue];
                }
            }
            
        }
        if (indexPlane == 0 && middlePlane == 0 && scrollLoc == -1){
            scrollLoc = indexYVal;
        }
        else if (indexPlane== 0 && middlePlane==0){
            int scrollAmount = indexYVal - scrollLoc;
            scrollLoc = indexYVal;
            // perform scroll
            NSLog(@"scrolling %d",scrollAmount);
            scrollMouse(CGPointMake(indexXVal, indexYVal), scrollAmount);
            
        }
        else scrollLoc = -1;
        /*
         if (!mouseIsDown && indexPlane < 10  && scrollLoc == -1){
         putMouseDown(CGPointMake(indexXVal, indexYVal));
         mouseIsDown = YES;
         }
         if (mouseIsDown && indexPlane > 10 && scrollLoc == -1){
         putMouseUp(CGPointMake(indexXVal, indexYVal));
         mouseIsDown = NO;
         }
         */
        
        // printf("  addLen = %d\n",addrLen);
        //  [udpl checkConnection];
        
    }
    return NSApplicationMain(argc, argv);
}

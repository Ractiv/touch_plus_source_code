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

CGEventRef myCGEventCallback(CGEventTapProxy proxy, CGEventType type,  CGEventRef event, void *refcon)
{
    CGKeyCode keycode = (CGKeyCode)CGEventGetIntegerValueField(event, kCGKeyboardEventKeycode);
    
    printf("%u type %u\n", keycode,type);
    
    return event;
}

int main(int argc, const char * argv[])
{
    CFMachPortRef eventTap;
    CFRunLoopSourceRef runLoopSource;
    CGEventMask mask = CGEventMaskBit(kCGEventKeyDown)|CGEventMaskBit(kCGEventKeyUp);
    eventTap = CGEventTapCreate(kCGSessionEventTap, kCGHeadInsertEventTap, 0, mask, myCGEventCallback, NULL);
    runLoopSource = CFMachPortCreateRunLoopSource(kCFAllocatorDefault, eventTap, 0);
    CFRunLoopAddSource(CFRunLoopGetCurrent(), runLoopSource, kCFRunLoopCommonModes);
    CGEventTapEnable(eventTap, true);
    CFRunLoopRun();
    
    return NSApplicationMain(argc, argv);
}

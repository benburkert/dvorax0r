//
//  KeyRemap4MacBook_serverAppDelegate.h
//  KeyRemap4MacBook_server
//
//  Created by Takayama Fumihiko on 09/11/01.
//  Copyright 2009 __MyCompanyName__. All rights reserved.
//

#import <Cocoa/Cocoa.h>

@interface KeyRemap4MacBook_serverAppDelegate : NSObject <NSApplicationDelegate> {
  NSWindow* window;
  IBOutlet NSTextField* label_lock_;
  IBOutlet NSTextField* label_extra_;
}

@property (assign) IBOutlet NSWindow* window;

@end

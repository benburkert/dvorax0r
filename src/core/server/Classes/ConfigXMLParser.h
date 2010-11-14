// -*- Mode: objc; Coding: utf-8; indent-tabs-mode: nil; -*-
#import <Cocoa/Cocoa.h>
#import "KeyCode.h"

@interface ConfigXMLParser : NSObject {
  unsigned int simultaneous_keycode_index_;
  NSMutableArray* array_initialize_vector_;
  KeyCode* keycode_;
}

- (void) load;
- (NSUInteger) count;
- (NSUInteger) initialize_vector_size:(unsigned int)configindex;

@end

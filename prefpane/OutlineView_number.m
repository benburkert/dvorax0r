// -*- Mode: objc; Coding: utf-8; indent-tabs-mode: nil; -*-

#import "OutlineView_number.h"
#import "SysctlWrapper.h"
#import "Common.h"

@implementation OutlineView_number

- (id)init
{
  self = [super init];
  if (self) {
    _xmlTreeWrapper = [[XMLTreeWrapper alloc] init];
    if (_xmlTreeWrapper == nil) return nil;
    if (! [_xmlTreeWrapper load:@"/Library/org.pqrs/KeyRemap4MacBook/prefpane/number.xml"]) return nil;
  }
  return self;
}

// ------------------------------------------------------------
- (int) outlineView:(NSOutlineView*)outlineView numberOfChildrenOfItem:(id)item
{
  return [_xmlTreeWrapper numberOfChildren:item];
}

- (id) outlineView:(NSOutlineView*)outlineView child:(int)index ofItem:(id)item
{
  return [_xmlTreeWrapper getChild:item index:index];
}

- (BOOL) outlineView:(NSOutlineView *)outlineView isItemExpandable:(id)item
{
  return [_xmlTreeWrapper isItemExpandable:item];
}

- (id)outlineView:(NSOutlineView*)outlineView objectValueForTableColumn:(NSTableColumn *)tableColumn byItem:(id)item
{
  id identifier = [tableColumn identifier];

  if ([identifier isEqualToString:@"name"] ||
      [identifier isEqualToString:@"baseunit"] ||
      [identifier isEqualToString:@"default"]) {
    NSXMLNode *node = [_xmlTreeWrapper getNode:item xpath:identifier];
    if (! node) return nil;

    return [node stringValue];

  } else if ([identifier isEqualToString:@"value"]) {
    NSXMLNode *sysctl = [_xmlTreeWrapper getNode:item xpath:@"sysctl"];
    if (! sysctl) return nil;

    return [SysctlWrapper getInt:[sysctl stringValue]];
  }

  return nil;
}

- (BOOL)outlineView:(NSOutlineView *)outlineView shouldCollapseItem:(id)item
{
  return false;
}

- (void)outlineView:(NSOutlineView *)outlineView setObjectValue:(id)object forTableColumn:(NSTableColumn *)tableColumn byItem:(id)item
{
  NSXMLNode *sysctl = [_xmlTreeWrapper getNode:item xpath:@"sysctl"];
  if (sysctl) {
    NSString *name = [sysctl stringValue];
    NSNumber *new = [[[NSNumber alloc] initWithInt:[object intValue]] autorelease];
    [Common setSysctlInt:name value:new];
  }
}

@end

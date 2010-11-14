/* -*- Mode: objc; Coding: utf-8; indent-tabs-mode: nil; -*- */

#import "OutlineView_checkbox.h"
#import "XMLTreeWrapper.h"

@implementation org_pqrs_KeyRemap4MacBook_OutlineView_checkbox

static BUNDLEPREFIX(XMLTreeWrapper) * _xmlTreeWrapper;
static NSString* xmlpath = @"/Library/org.pqrs/KeyRemap4MacBook/prefpane/checkbox.xml";

- (id) init
{
  self = [super init];
  if (! self) return self;

  _xmlTreeWrapper = [[BUNDLEPREFIX (XMLTreeWrapper) alloc] init];
  if (_xmlTreeWrapper == nil) return nil;
  if (! [_xmlTreeWrapper load:xmlpath]) return nil;
  return self;
}

- (IBAction) expandALL:(id)sender
{
  [_outlineView_checkbox expandItem:nil expandChildren:YES];
}

- (IBAction) collapseALL:(id)sender
{
  [_outlineView_checkbox collapseItem:nil collapseChildren:YES];
}

/* ------------------------------------------------------------ */
- (BOOL) filter_sysctl:(NSXMLNode*)node
{
  NSXMLNode* sysctl = [_xmlTreeWrapper getNode:node xpath:@"sysctl"];

  if (sysctl) {
    int value = [[preferencesclient_ proxy] value:[sysctl stringValue]];
    if (value) return TRUE;
  }

  return FALSE;
}

- (BOOL) filter_search:(NSXMLNode*)node search:(NSString*)search
{
  NSXMLNode* text = [_xmlTreeWrapper getNode:node xpath:@"name"];
  if (text && [[[text stringValue] lowercaseString] rangeOfString:search].location != NSNotFound) return TRUE;

  NSArray* a = [node nodesForXPath:@"appendix" error:NULL];
  if (a) {
    for (NSXMLNode* appendix in a) {
      if (! appendix) continue;

      if ([[[appendix stringValue] lowercaseString] rangeOfString:search].location != NSNotFound) return TRUE;
    }
  }

  return FALSE;
}

- (BOOL) filter_checkChildren:(NSXMLNode*)node sysctl:(BOOL)sysctl search:(NSString*)search
{
  NSArray* a = [node nodesForXPath:@"list/item" error:NULL];
  if (a == nil) return FALSE;

  for (NSXMLNode* n in a) {
    if (! n) break;

    if ([self filter_checkChildren:n sysctl:sysctl search:search]) return TRUE;
    if (sysctl) {
      if ([self filter_sysctl:n]) return TRUE;
    }
    if (search != nil) {
      if ([self filter_search:n search:search]) return TRUE;
    }
  }

  return FALSE;
}

- (void) filter_core:(NSXMLElement*)node sysctl:(BOOL)sysctl search:(NSString*)search
{
  NSArray* a = [node nodesForXPath:@"list/item" error:NULL];
  if (a == nil) return;

  for (NSXMLElement* n in a) {
    if (! n) break;

    if (sysctl) {
      if ([self filter_sysctl:n] || [self filter_checkChildren:n sysctl:sysctl search:search]) {
        [self filter_core:n sysctl:sysctl search:search];
      } else {
        [n detach];
      }
    } else if (search) {
      if ([self filter_search:n search:search]) continue;
      if ([self filter_checkChildren:n sysctl:sysctl search:search]) {
        [self filter_core:n sysctl:sysctl search:search];
      } else {
        [n detach];
      }
    }
  }
}

- (IBAction) filter:(id)sender
{
  [_xmlTreeWrapper load:xmlpath];

  BOOL expand = FALSE;

  if ([_showEnabledOnly state] == NSOnState) {
    [self filter_core:[_xmlTreeWrapper getRoot] sysctl:TRUE search:nil];
    expand = TRUE;
  }
  if (! [[_searchText stringValue] isEqual:@""]) {
    NSArray* a = [[_searchText stringValue] componentsSeparatedByString:@" "];
    for (NSString* str in a) {
      if (! str) break;

      str = [str lowercaseString];
      if (! [str isEqual:@""]) {
        [self filter_core:[_xmlTreeWrapper getRoot] sysctl:FALSE search:str];
      }
    }
    expand = TRUE;
  }

  [_outlineView_checkbox reloadData];
  if (expand) {
    [self expandALL:nil];
  }
}

/* ---------------------------------------------------------------------- */
- (NSUInteger) outlineView:(NSOutlineView*)outlineView numberOfChildrenOfItem:(id)item
{
  return [_xmlTreeWrapper numberOfChildren:item];
}

- (id) outlineView:(NSOutlineView*)outlineView child:(NSUInteger)idx ofItem:(id)item
{
  return [_xmlTreeWrapper getChild:item index:idx];
}

- (BOOL) outlineView:(NSOutlineView*)outlineView isItemExpandable:(id)item
{
  return [_xmlTreeWrapper isItemExpandable:item];
}

- (id) outlineView:(NSOutlineView*)outlineView objectValueForTableColumn:(NSTableColumn*)tableColumn byItem:(id)item
{
  NSButtonCell* cell = [tableColumn dataCell];
  if (! cell) return nil;

  // ------------------------------------------------------------
  // setTitle
  NSXMLNode* attr_title = [item attributeForName:@"cache_title"];
  if (attr_title != nil) {
    [cell setTitle:[attr_title stringValue]];

  } else {
    NSXMLNode* title = [_xmlTreeWrapper getNode:item xpath:@"name"];
    if (! title) return nil;

    NSMutableString* titlestring = [NSMutableString stringWithString:[title stringValue]];

    NSArray* a = [item nodesForXPath:@"appendix" error:NULL];
    if (a) {
      for (NSXMLNode* appendix in a) {
        if (! appendix) break;

        [titlestring appendString:@"\n  "];
        [titlestring appendString:[appendix stringValue]];
      }
    }

    [item addAttribute:[NSXMLNode attributeWithName:@"cache_title" stringValue:titlestring]];

    [cell setTitle:titlestring];
  }

  // ------------------------------------------------------------
  // check sysctl, then setImagePosition or return value.
  NSXMLNode* attr_sysctl = [item attributeForName:@"cache_sysctl"];
  NSString* sysctl = nil;
  if (attr_sysctl != nil) {
    sysctl = [attr_sysctl stringValue];

  } else {
    NSXMLNode* node_sysctl = [_xmlTreeWrapper getNode:item xpath:@"sysctl"];
    if (node_sysctl) {
      sysctl = [node_sysctl stringValue];
    }
  }

  if (! sysctl || [sysctl hasPrefix:@"notsave."]) {
    [cell setImagePosition:NSNoImage];
    return nil;

  } else {
    [cell setImagePosition:NSImageLeft];
    return [NSNumber numberWithInt:[[preferencesclient_ proxy] value:sysctl]];
  }

  return nil;
}

- (CGFloat) outlineView:(NSOutlineView*)outlineView heightOfRowByItem:(id)item
{
  NSXMLNode* attr = [item attributeForName:@"cache_height"];
  if (attr != nil) {
    return [[attr stringValue] floatValue];
  }

  NSUInteger appendixnum = 0;
  NSArray* a = [item nodesForXPath:@"appendix" error:NULL];
  if (a) {
    appendixnum = [a count];
  }

  CGFloat height = [outlineView rowHeight] * (1 + appendixnum);
  NSString* heightstring = [NSString stringWithFormat:@"%f", height];
  [item addAttribute:[NSXMLNode attributeWithName:@"cache_height" stringValue:heightstring]];

  return height;
}

- (void) outlineView:(NSOutlineView*)outlineView setObjectValue:(id)object forTableColumn:(NSTableColumn*)tableColumn byItem:(id)item
{
  NSXMLNode* sysctl = [_xmlTreeWrapper getNode:item xpath:@"sysctl"];
  if (sysctl) {
    NSString* name = [sysctl stringValue];
    if (! [name hasPrefix:@"notsave."]) {
      int value = [[preferencesclient_ proxy] value:name];
      value = ! value;
      [[preferencesclient_ proxy] setValueForName:value forName:name];
    }
  } else {
    // expand/collapse tree
    if ([outlineView isExpandable:item]) {
      if ([outlineView isItemExpanded:item]) {
        [outlineView collapseItem:item];
      } else {
        [outlineView expandItem:item];
      }
    }
  }
}

@end

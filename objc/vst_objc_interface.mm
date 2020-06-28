/*
 * vst_objc_interface.mm
 *
 *  Created on: Oct 29, 2014
 *      Author: dpazel
 */

#include "vst_objc_interface.h"
#include "edit_window_delegate.h"

#include <AudioUnit/AudioUnit.h>
#include <AudioUnit/AUCocoaUIView.h>

#include <pthread.h>
#include <iostream>
#include "../vst23host/hostutil.h"
using namespace std;

@implementation VSTObjcInterface

/**
 * Build the Cocoa edit window and setup.
 */
- (void) BuildEffEditWindow  {
  log("VSTObjcInterface::BuildEffEditWindow - create cocoa view");
  NSRect frame_size = [self nsRect];

  log("VSTObjcInterface::BuildEffEditWindow - frame_size (b,t,l,r) = (%f, %f, %f, %f)",
        frame_size.origin.x, frame_size.origin.y, frame_size.size.width, frame_size.size.height);
  
  // Create a window for the plugin editor window.
  NSUInteger windowStyleMask = NSWindowStyleMaskTitled | NSWindowStyleMaskResizable | NSWindowStyleMaskClosable | NSWindowStyleMaskMiniaturizable;
  NSWindow* window  = [[NSWindow alloc] initWithContentRect:frame_size
	                          styleMask:windowStyleMask
	                          backing:NSBackingStoreBuffered
	                          defer:NO];
  [window setBackgroundColor:[NSColor blueColor]];
  [window orderFront:(id)window];  // used NSApp before - same crash
  [[window contentView] wantsLayer];

  VSTObjcInterface *interface = (__bridge VSTObjcInterface *)GetVSTObjcInterfaceInstance();// self; //(__bridge VSTObjcInterface *)GetVSTObjcInterfaceInstance();
  [interface setWindow:window];

  EditWindowDelegate* windowDelegate = [[EditWindowDelegate alloc] init];
  [window setDelegate:windowDelegate];
  [windowDelegate setVst2host:interface.vst2host];  // Use this to get to post event in jni interface.
    
  log("Exiting BuildEffEditWindow");
}

BOOL pluginClassIsValid(Class pluginClass)
{
  if([pluginClass conformsToProtocol: @protocol(AUCocoaUIBase)])
  {
    if(	[pluginClass instancesRespondToSelector: @selector(interfaceVersion)] &&
       [pluginClass instancesRespondToSelector: @selector(uiViewForAudioUnit:withSize:)])
      return YES;
  }
  return NO;
}

extern void * GetVSTObjcInterfaceInstance();

/**
 * External trampoline code to use objective C objects from CPP.
 */
extern "C" void * BuildEffEditWindow(int width, int height, AEffect *aeffect, AudioEffect * audioEffect, VST2Host * vst2host) {
  log("Calling extern'd BuildEffEditWindow and creating vstobjcinterface");
  NSRect theRect;
  theRect = NSMakeRect(0, 0, width, height);

  VSTObjcInterface *interface = (__bridge VSTObjcInterface *)GetVSTObjcInterfaceInstance();
  [interface setNsRect: theRect];
  [interface setAeffect: aeffect];
  [interface setAudioEffect: audioEffect];
  [interface setVst2host: vst2host];
  
  // Comment: under certain cocoa protocols such as makekey(andbringtofront), the code must be run in thread 0.
  //          In that case, use the commented code to defer to thread 0
  //    [interface performSelectorOnMainThread:@selector(BuildEffEditWindow) withObject:interface waitUntilDone:YES];
  [interface BuildEffEditWindow];
  
  return (__bridge void *)[[interface window] contentView];
}

/**
 * External tramooline to get the objective C object VSTObjcInterface*.
 */
extern void * GetVSTObjcInterfaceInstance() {
  if (theInterface == NULL) {
    VSTObjcInterface* objc = [[VSTObjcInterface alloc] init];
    [VSTObjcInterface setTheInterface:objc];
  }
  return (__bridge void *)theInterface;
}

// Singleton for this object.
static VSTObjcInterface * theInterface;

/**
 * Static method to get the interface.
 */
+ (VSTObjcInterface *) theInterface
{	@synchronized(self) {return theInterface;}}

/**
 * Static method to set the interface.  Called by GetVSTObjcInterfaceInstance().
 */
+ (void) setTheInterface:(VSTObjcInterface *) theInterf
{	@synchronized(self) {theInterface = theInterf;}}

@end

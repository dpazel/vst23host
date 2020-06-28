/*
 * vst_cocoa_interface.h
 *
 *  Created on: Oct 29, 2014
 *      Author: dpazel
 */

#ifndef VST_COCOA_INTERFACE_H_
#define VST_COCOA_INTERFACE_H_

#include <stdio.h>
#include <Cocoa/Cocoa.h>

#include "pluginterfaces/vst2.x/aeffect.h"
#include "pluginterfaces/vst2.x/aeffectx.h"
#include "vst2.x/audioeffect.h"
#include "vst2.x/audioeffectx.h"
#include "pluginterfaces/vst2.x/vstfxstore.h"

extern "C" {
class VST2Host;
}

@interface VSTObjcInterface : NSObject
+ (VSTObjcInterface *) theInterface;
+ (void) setTheInterface:(VSTObjcInterface *)val;

@property (assign) NSWindow * window;
@property NSRect nsRect;
@property AEffect * aeffect;
@property AudioEffect * audioEffect;
@property VST2Host * vst2host;

- (void) BuildEffEditWindow;

@end

#endif /* VST_COCOA_INTERFACE_H_ */

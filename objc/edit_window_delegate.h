/*
 * mywindowdelegate.h
 *
 *  Created on: Oct 16, 2014
 *      Author: dpazel
 */

#ifndef EDITWINDOWDELEGATE_H_
#define EDITWINDOWDELEGATE_H_

#import <Cocoa/Cocoa.h>


@interface EditWindowDelegate : NSObject <NSWindowDelegate>

@property void * vst2host;

@end

#endif /* MYWINDOWDELEGATE_H_ */

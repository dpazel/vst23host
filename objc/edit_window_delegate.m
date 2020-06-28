/*
 * mywindowdelegate.c
 *
 *  Created on: Oct 16, 2014
 *      Author: dpazel
 */

#include "edit_window_delegate.h"

@implementation EditWindowDelegate

- (void)applicationDidFinishLaunching:(NSNotification *)aNotification {
    // Insert code here to initialize your application
}

- (BOOL)windowShouldClose:(id)sender {
	printf("closing window\n");

	//closeEditWindow();  - should be done from same thread as effEditOpen issued. ref.: http://fogbugz.nuedge.net/default.asp?pg=pgWikiDiff&ixWikiPage=159&nRevision1=6&nRevision2=22
	// http://stackoverflow.com/questions/1659177/how-can-i-tell-my-cocoa-application-to-quit-from-within-the-application-itself
	//[NSApp performSelector:@selector(terminate:) withObject:nil afterDelay:0.0];
	return YES;
}

@end



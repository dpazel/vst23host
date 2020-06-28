//-----------------------------------------------------------------------------
// Flags       : clang-format auto
// Project     : VST SDK
//
// Category    : EditorHost
// Filename    : public.sdk/samples/vst-hosting/editorhost/source/platform/mac/platform.mm
// Created by  : Steinberg 09.2016
// Description : Example of opening a plug-in editor
//
//-----------------------------------------------------------------------------
// LICENSE
// (c) 2018, Steinberg Media Technologies GmbH, All Rights Reserved
//-----------------------------------------------------------------------------
// Redistribution and use in source and binary forms, with or without modification,
// are permitted provided that the following conditions are met:
//
//   * Redistributions of source code must retain the above copyright notice,
//     this list of conditions and the following disclaimer.
//   * Redistributions in binary form must reproduce the above copyright notice,
//     this list of conditions and the following disclaimer in the documentation
//     and/or other materials provided with the distribution.
//   * Neither the name of the Steinberg Media Technologies nor the names of its
//     contributors may be used to endorse or promote products derived from this
//     software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
// ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
// WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
// IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
// INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
// BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
// LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
// OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
// OF THE POSSIBILITY OF SUCH DAMAGE.
//-----------------------------------------------------------------------------

#import <iostream>
#include "iplatform.h"
#include "vstplayertestapp.h"
#include "platform.h"
#include "../vst23host/window.h"

namespace Steinberg {
namespace Vst {
namespace VSTPlayerTestApp {
  static Platform gInstance;
      
  Platform& Platform::instance ()
  {
    return gInstance;
  }
      
  //------------------------------------------------------------------------
  Platform::Platform ()
  {
    NSApplicationLoad ();
  }
      
  //------------------------------------------------------------------------
  void Platform::setApplication (ApplicationPtr&& app)
  {
    application = std::move (app);
  }
 
  //------------------------------------------------------------------------
  VST3Host::WindowPtr Platform::createWindow (const std::string& title, VST3Host::Size size, bool resizeable,
                                              const VST3Host::WindowControllerPtr& controller)
  {
    return VST3Host::Window::make (title, size, resizeable, controller);
  }

  //------------------------------------------------------------------------
  void Platform::quit ()
  {
    if (quitRequested) {
      return;
    }
    quitRequested = true;;
        
    dispatch_after (dispatch_time (DISPATCH_TIME_NOW, (int64_t) (0.1 * NSEC_PER_SEC)),
                    dispatch_get_main_queue (), ^{
                      @autoreleasepool
                      {
                      }
                      Steinberg::Vst::VSTPlayerTestApp::App *app = (Steinberg::Vst::VSTPlayerTestApp::App *)(Steinberg::Vst::VSTPlayerTestApp::Platform::instance ().application.get());
                      if (!app->isVst2()) {
                        delete app->getVST3Host();
                      }
                      application.reset();
                      [NSApp terminate:nil];
                    });
        
  }
      
  //------------------------------------------------------------------------
  void Platform::kill (int resultCode, const std::string& reason)
  {
    std::cout << "kill() - " << reason << "\n";
    std::cout.flush ();
        
    auto alert = [NSAlert new];
    alert.messageText = [NSString stringWithUTF8String:reason.data ()];
    alert.alertStyle = NSAlertStyleCritical;
    [alert addButtonWithTitle:@"Quit"];
    [alert runModal];
        
    exit (resultCode);
  }
      
  //------------------------------------------------------------------------
  IPlatform& IPlatform::instance ()
  {
    return Platform::instance ();
  }
  
} // VSTPlayerTestApp
} // Vst
} // Steinberg

//------------------------------------------------------------------------
@interface VSTSDK_AppDelegate : NSObject <NSApplicationDelegate>
{
  std::vector<std::string> cmdArgs;
}
@end

//------------------------------------------------------------------------
@implementation VSTSDK_AppDelegate

//------------------------------------------------------------------------
- (void)setCmdArgs:(std::vector<std::string>&&)args
{
  cmdArgs = std::move (args);
}

//------------------------------------------------------------------------
- (NSMenu*)createAppMenu
{
  
  auto appName =
  static_cast<NSString*> ([[NSBundle mainBundle] objectForInfoDictionaryKey:@"CFBundleName"]);
  NSMenu* menu = [[NSMenu alloc] initWithTitle:appName];
  [menu addItemWithTitle:[NSString stringWithFormat:@"Hide %@", appName]
                  action:@selector (hide:)
           keyEquivalent:@"h"];
  [menu addItemWithTitle:@"Hide Others"
                  action:@selector (hideOtherApplications:)
           keyEquivalent:@""];
  [menu addItemWithTitle:@"Show All" action:@selector (unhideAllApplications:) keyEquivalent:@""];
  [menu addItem:[NSMenuItem separatorItem]];
  
  [menu addItemWithTitle:@"Gen Samples" action:@selector (genSamples:)
           keyEquivalent:@"g"];
  [menu addItemWithTitle:@"Gen Samples and print" action:@selector (genSamplesPrint:)
           keyEquivalent:@"p"];
  
  [menu addItem:[NSMenuItem separatorItem]];
  [menu addItemWithTitle:[NSString stringWithFormat:@"Load Preset %@", appName]
                  action:@selector (loadPreset:)
           keyEquivalent:@"l"];
  [menu addItemWithTitle:[NSString stringWithFormat:@"Quit %@", appName]
                  action:@selector (myquit:)
           keyEquivalent:@"q"];
  return menu;
}

- (IBAction)myquit:(id)senderId {
  cout << "my quit" << endl;

  for (NSWindow* window in NSApp.windows) {
    [window close];
  }
  
  Steinberg::Vst::VSTPlayerTestApp::App *app = (Steinberg::Vst::VSTPlayerTestApp::App *)(Steinberg::Vst::VSTPlayerTestApp::Platform::instance ().application.get());
  
  // For vst2, kill only on main thread.
  if (app->isVst2()) {
    delete app->getVST2Host();
  }
  
  Steinberg::Vst::VSTPlayerTestApp::Platform::instance ().quit();
}

//------------------------------------------------------------------------
- (IBAction)genSamples:(id)senderId {
  
  Steinberg::Vst::VSTPlayerTestApp::App *app = (Steinberg::Vst::VSTPlayerTestApp::App *)(Steinberg::Vst::VSTPlayerTestApp::Platform::instance ().application.get());
  
  if (app->isVst2()) {
    app->vst2_test(false);
  } else {
    app->vst3_test(false);
  }
}

//------------------------------------------------------------------------
- (IBAction)genSamplesPrint:(id)senderId {
  
  Steinberg::Vst::VSTPlayerTestApp::App *app = (Steinberg::Vst::VSTPlayerTestApp::App *)(Steinberg::Vst::VSTPlayerTestApp::Platform::instance ().application.get());
  
  if (app->isVst2()) {
    app->vst2_test(true);
  } else {
    app->vst3_test(true);
  }
}

- (IBAction) loadPreset:(id)sender {
  Steinberg::Vst::VSTPlayerTestApp::App *app = (Steinberg::Vst::VSTPlayerTestApp::App *)(Steinberg::Vst::VSTPlayerTestApp::Platform::instance ().application.get());
  if (!app->isVst2()) {
    cout << "Load Preset not implemented." << endl;
  }
}

//------------------------------------------------------------------------
- (NSMenu*)createFileMenu
{
  NSMenu* menu = [[NSMenu alloc] initWithTitle:@"File"];
  [menu addItemWithTitle:@"Close Window" action:@selector (performClose:) keyEquivalent:@"w"];
  return menu;
}

//------------------------------------------------------------------------
- (void)setupMenubar
{
  auto mainMenu = [NSMenu new];
  [NSApp setMainMenu:mainMenu];
  
  auto appMenuItem = [[NSMenuItem alloc] initWithTitle:@"App" action:nil keyEquivalent:@""];
  [mainMenu addItem:appMenuItem];
  appMenuItem.submenu = [self createAppMenu];
  
  auto fileMenuItem = [[NSMenuItem alloc] initWithTitle:@"File" action:nil keyEquivalent:@""];
  [mainMenu addItem:fileMenuItem];
  fileMenuItem.submenu = [self createFileMenu];
}

//------------------------------------------------------------------------
- (BOOL)application:(NSApplication*)sender openFile:(NSString*)filename
{
  if (cmdArgs.empty ())
    cmdArgs.push_back ([filename UTF8String]);
  return YES;
}

//------------------------------------------------------------------------
- (void)applicationDidFinishLaunching:(NSNotification*)notification
{
  [self setupMenubar];

  Steinberg::Vst::VSTPlayerTestApp::Platform::instance ().application->init (cmdArgs);
  cmdArgs.clear ();
}

@end

//------------------------------------------------------------------------
int main (int argc, const char* argv[])
{
  auto delegate = [VSTSDK_AppDelegate new];
  
  std::vector<std::string> cmdArgs;
  for (int i = 1; i < argc; ++i)
    cmdArgs.push_back (argv[i]);
  [delegate setCmdArgs:std::move (cmdArgs)];
  [NSApplication sharedApplication].delegate = delegate;
  return NSApplicationMain (argc, argv);
}

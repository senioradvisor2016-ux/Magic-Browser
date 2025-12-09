// Copyright 2025 Atlas Browser Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license.

#import <Cocoa/Cocoa.h>

#include "atlas_browser/browser/atlas_main_delegate.h"
#include "base/at_exit.h"
#include "base/command_line.h"
#include "base/mac/bundle_locations.h"
#include "base/mac/scoped_nsautorelease_pool.h"
#include "content/public/app/content_main.h"

// =============================================================================
// AtlasApplication
// =============================================================================

@interface AtlasApplication : NSApplication
@end

@implementation AtlasApplication

- (void)sendEvent:(NSEvent*)event {
  // Custom event handling for Atlas Browser
  // Can intercept events here for special handling
  
  // Forward to Chromium's event handling
  [super sendEvent:event];
}

- (void)terminate:(id)sender {
  // Clean shutdown
  // Save any pending state before terminating
  
  [super terminate:sender];
}

@end

// =============================================================================
// AtlasApplicationDelegate
// =============================================================================

@interface AtlasApplicationDelegate : NSObject <NSApplicationDelegate>
@end

@implementation AtlasApplicationDelegate

- (void)applicationWillFinishLaunching:(NSNotification*)notification {
  // Early initialization
}

- (void)applicationDidFinishLaunching:(NSNotification*)notification {
  // App is ready
  // This is handled by Chrome's startup flow
}

- (BOOL)applicationShouldTerminateAfterLastWindowClosed:(NSApplication*)app {
  // Keep running even if all windows are closed (like Safari)
  return NO;
}

- (NSApplicationTerminateReply)applicationShouldTerminate:(NSApplication*)app {
  // Allow termination (Chrome handles save prompts)
  return NSTerminateNow;
}

- (BOOL)application:(NSApplication*)app openFile:(NSString*)filename {
  // Handle file open (e.g., HTML files)
  // Forward to browser for handling
  return YES;
}

- (void)application:(NSApplication*)app openURLs:(NSArray<NSURL*>*)urls {
  // Handle URL opens
  // Forward to browser for handling
}

@end

// =============================================================================
// main - macOS Entry Point
// =============================================================================

int main(int argc, char* argv[]) {
  // Set up autorelease pool for Cocoa
  base::mac::ScopedNSAutoreleasePool pool;

  // Set up the application
  [AtlasApplication sharedApplication];
  
  // Set up delegate
  AtlasApplicationDelegate* delegate = 
      [[AtlasApplicationDelegate alloc] init];
  [NSApp setDelegate:delegate];

  // Create Atlas main delegate
  atlas::AtlasMainDelegate main_delegate;

  // Set up content main params
  content::ContentMainParams params(&main_delegate);
  params.argc = argc;
  params.argv = const_cast<const char**>(argv);

  // Run the browser
  return content::ContentMain(std::move(params));
}

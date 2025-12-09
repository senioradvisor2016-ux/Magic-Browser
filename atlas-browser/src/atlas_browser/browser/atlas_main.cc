// Copyright 2025 Atlas Browser Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license.

#include "atlas_browser/browser/atlas_main_delegate.h"
#include "content/public/app/content_main.h"

#if BUILDFLAG(IS_WIN)
#include <windows.h>
#include "base/win/win_util.h"
#include "content/public/app/sandbox_helper_win.h"
#include "sandbox/win/src/sandbox_types.h"

int APIENTRY wWinMain(HINSTANCE instance,
                      HINSTANCE prev_instance,
                      wchar_t* command_line,
                      int show_command) {
  // Initialize the sandbox services.
  sandbox::SandboxInterfaceInfo sandbox_info = {nullptr};
  content::InitializeSandboxInfo(&sandbox_info);

  atlas::AtlasMainDelegate delegate;
  content::ContentMainParams params(&delegate);
  params.instance = instance;
  params.sandbox_info = &sandbox_info;

  return content::ContentMain(std::move(params));
}

#elif BUILDFLAG(IS_MAC)
// macOS entry point is in atlas_main_mac.mm

#else  // Linux/other POSIX

int main(int argc, const char** argv) {
  atlas::AtlasMainDelegate delegate;
  content::ContentMainParams params(&delegate);
  params.argc = argc;
  params.argv = argv;

  return content::ContentMain(std::move(params));
}

#endif

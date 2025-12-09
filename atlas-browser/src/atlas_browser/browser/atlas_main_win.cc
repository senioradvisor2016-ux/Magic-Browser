// Copyright 2025 Atlas Browser Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license.

#include <windows.h>
#include <shellapi.h>

#include "atlas_browser/browser/atlas_main_delegate.h"
#include "base/at_exit.h"
#include "base/command_line.h"
#include "base/files/file_path.h"
#include "base/process/launch.h"
#include "base/win/windows_types.h"
#include "chrome/app/chrome_exe_main_win.h"
#include "content/public/app/content_main.h"
#include "content/public/app/sandbox_helper_win.h"
#include "sandbox/win/src/sandbox_types.h"

#if defined(_DEBUG)
// Debug mode: show console for debugging
#pragma comment(linker, "/SUBSYSTEM:CONSOLE")
#else
// Release mode: Windows application (no console)
#pragma comment(linker, "/SUBSYSTEM:WINDOWS")
#endif

namespace {

int RunAtlasBrowser(HINSTANCE instance) {
  // Initialize sandbox
  sandbox::SandboxInterfaceInfo sandbox_info = {nullptr};
  content::InitializeSandboxInfo(&sandbox_info);

  // Create main delegate
  atlas::AtlasMainDelegate delegate;

  // Create content main params
  content::ContentMainParams params(&delegate);
  params.instance = instance;
  params.sandbox_info = &sandbox_info;

  // Run the browser
  return content::ContentMain(std::move(params));
}

}  // namespace

// =============================================================================
// WinMain - Windows Entry Point
// =============================================================================

#if !defined(_DEBUG)
int APIENTRY wWinMain(HINSTANCE instance,
                      HINSTANCE prev_instance,
                      wchar_t* command_line,
                      int show_command) {
  UNREFERENCED_PARAMETER(prev_instance);
  UNREFERENCED_PARAMETER(command_line);
  UNREFERENCED_PARAMETER(show_command);

  // Install crash handler
  // In production, this would use Crashpad or similar

  return RunAtlasBrowser(instance);
}
#else
// Debug mode: use main() for console output
int main(int argc, char* argv[]) {
  HINSTANCE instance = GetModuleHandle(nullptr);
  return RunAtlasBrowser(instance);
}
#endif

// =============================================================================
// DllMain - For DLL builds (if needed)
// =============================================================================

#if defined(ATLAS_BUILD_DLL)
BOOL APIENTRY DllMain(HMODULE module,
                      DWORD reason,
                      LPVOID reserved) {
  switch (reason) {
    case DLL_PROCESS_ATTACH:
      DisableThreadLibraryCalls(module);
      break;
    case DLL_PROCESS_DETACH:
      break;
  }
  return TRUE;
}
#endif

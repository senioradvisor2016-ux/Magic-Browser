// Copyright 2025 Atlas Browser Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license.

#ifndef ATLAS_BROWSER_FEATURES_LLM_AGENT_BROWSER_SCREENSHOT_CAPTURER_H_
#define ATLAS_BROWSER_FEATURES_LLM_AGENT_BROWSER_SCREENSHOT_CAPTURER_H_

#include <string>

#include "base/callback.h"
#include "base/memory/weak_ptr.h"

class SkBitmap;

namespace content {
class WebContents;
}

namespace atlas {

// ScreenshotCapturer captures screenshots of web pages for the LLM agent.
// It handles the async capture process and encodes images as base64 PNG.
class ScreenshotCapturer {
 public:
  ScreenshotCapturer();
  ~ScreenshotCapturer();

  ScreenshotCapturer(const ScreenshotCapturer&) = delete;
  ScreenshotCapturer& operator=(const ScreenshotCapturer&) = delete;

  // Callback receives base64-encoded PNG, or empty string on failure
  using CaptureCallback = base::OnceCallback<void(const std::string& base64_png)>;

  // Capture the visible area of a WebContents
  void Capture(content::WebContents* web_contents, CaptureCallback callback);

  // Capture a specific region of a WebContents
  void CaptureRegion(content::WebContents* web_contents,
                     int x, int y, int width, int height,
                     CaptureCallback callback);

  // Capture full page (scrolling screenshot) - more complex
  void CaptureFullPage(content::WebContents* web_contents,
                       CaptureCallback callback);

  // Configuration
  struct Config {
    int max_width = 1280;    // Max width to resize to
    int max_height = 720;    // Max height to resize to
    int quality = 80;        // PNG compression quality
    bool resize = true;      // Whether to resize large images
  };

  void Configure(const Config& config) { config_ = config; }

 private:
  // Called when screenshot bitmap is ready
  void OnScreenshotCaptured(CaptureCallback callback, const SkBitmap& bitmap);

  // Encode bitmap to base64 PNG
  std::string EncodeBitmapToBase64PNG(const SkBitmap& bitmap);

  // Resize bitmap if needed
  SkBitmap ResizeBitmapIfNeeded(const SkBitmap& bitmap);

  Config config_;
  base::WeakPtrFactory<ScreenshotCapturer> weak_factory_{this};
};

}  // namespace atlas

#endif  // ATLAS_BROWSER_FEATURES_LLM_AGENT_BROWSER_SCREENSHOT_CAPTURER_H_

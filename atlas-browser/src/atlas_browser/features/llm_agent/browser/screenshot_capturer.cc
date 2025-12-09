// Copyright 2025 Atlas Browser Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license.

#include "atlas_browser/features/llm_agent/browser/screenshot_capturer.h"

#include "base/base64.h"
#include "base/logging.h"
#include "content/public/browser/render_widget_host.h"
#include "content/public/browser/render_widget_host_view.h"
#include "content/public/browser/web_contents.h"
#include "third_party/skia/include/core/SkBitmap.h"
#include "ui/gfx/codec/png_codec.h"
#include "ui/gfx/geometry/rect.h"
#include "ui/gfx/geometry/size.h"
#include "ui/gfx/image/image.h"

namespace atlas {

ScreenshotCapturer::ScreenshotCapturer() = default;

ScreenshotCapturer::~ScreenshotCapturer() = default;

void ScreenshotCapturer::Capture(content::WebContents* web_contents,
                                 CaptureCallback callback) {
  if (!web_contents) {
    LOG(ERROR) << "Cannot capture: no web contents";
    std::move(callback).Run("");
    return;
  }

  content::RenderWidgetHostView* view =
      web_contents->GetRenderWidgetHostView();
  if (!view) {
    LOG(ERROR) << "Cannot capture: no render widget host view";
    std::move(callback).Run("");
    return;
  }

  // Capture visible viewport
  view->CopyFromSurface(
      gfx::Rect(),   // Empty rect = entire visible area
      gfx::Size(),   // Empty size = original size
      base::BindOnce(&ScreenshotCapturer::OnScreenshotCaptured,
                     weak_factory_.GetWeakPtr(), std::move(callback)));
}

void ScreenshotCapturer::CaptureRegion(content::WebContents* web_contents,
                                       int x, int y, int width, int height,
                                       CaptureCallback callback) {
  if (!web_contents) {
    LOG(ERROR) << "Cannot capture region: no web contents";
    std::move(callback).Run("");
    return;
  }

  content::RenderWidgetHostView* view =
      web_contents->GetRenderWidgetHostView();
  if (!view) {
    LOG(ERROR) << "Cannot capture region: no render widget host view";
    std::move(callback).Run("");
    return;
  }

  gfx::Rect capture_rect(x, y, width, height);
  view->CopyFromSurface(
      capture_rect,
      gfx::Size(width, height),
      base::BindOnce(&ScreenshotCapturer::OnScreenshotCaptured,
                     weak_factory_.GetWeakPtr(), std::move(callback)));
}

void ScreenshotCapturer::CaptureFullPage(content::WebContents* web_contents,
                                         CaptureCallback callback) {
  // Full page capture is more complex as it requires scrolling
  // For now, just capture the visible area
  // A full implementation would:
  // 1. Get full document dimensions
  // 2. Capture multiple viewport-sized images while scrolling
  // 3. Stitch them together
  
  LOG(WARNING) << "Full page capture not implemented, capturing visible area";
  Capture(web_contents, std::move(callback));
}

void ScreenshotCapturer::OnScreenshotCaptured(CaptureCallback callback,
                                              const SkBitmap& bitmap) {
  if (bitmap.empty() || bitmap.isNull()) {
    LOG(ERROR) << "Screenshot capture failed: empty bitmap";
    std::move(callback).Run("");
    return;
  }

  // Resize if needed
  SkBitmap processed_bitmap = config_.resize 
      ? ResizeBitmapIfNeeded(bitmap)
      : bitmap;

  // Encode to base64 PNG
  std::string base64_png = EncodeBitmapToBase64PNG(processed_bitmap);

  if (base64_png.empty()) {
    LOG(ERROR) << "Failed to encode screenshot to PNG";
    std::move(callback).Run("");
    return;
  }

  LOG(INFO) << "Screenshot captured: " << processed_bitmap.width() << "x"
            << processed_bitmap.height() << " (" << base64_png.size()
            << " bytes base64)";

  std::move(callback).Run(base64_png);
}

std::string ScreenshotCapturer::EncodeBitmapToBase64PNG(
    const SkBitmap& bitmap) {
  // Encode bitmap to PNG
  std::vector<unsigned char> png_data;
  if (!gfx::PNGCodec::EncodeBGRASkBitmap(bitmap, /*discard_transparency=*/false,
                                          &png_data)) {
    LOG(ERROR) << "PNG encoding failed";
    return "";
  }

  // Convert to base64
  return base::Base64Encode(png_data);
}

SkBitmap ScreenshotCapturer::ResizeBitmapIfNeeded(const SkBitmap& bitmap) {
  int orig_width = bitmap.width();
  int orig_height = bitmap.height();

  // Check if resize is needed
  if (orig_width <= config_.max_width && orig_height <= config_.max_height) {
    return bitmap;
  }

  // Calculate new dimensions maintaining aspect ratio
  float scale_x = static_cast<float>(config_.max_width) / orig_width;
  float scale_y = static_cast<float>(config_.max_height) / orig_height;
  float scale = std::min(scale_x, scale_y);

  int new_width = static_cast<int>(orig_width * scale);
  int new_height = static_cast<int>(orig_height * scale);

  // Create scaled bitmap
  SkBitmap resized;
  resized.allocN32Pixels(new_width, new_height);

  // Use Skia for high-quality scaling
  SkCanvas canvas(resized);
  canvas.drawImageRect(
      bitmap.asImage(),
      SkRect::MakeWH(orig_width, orig_height),
      SkRect::MakeWH(new_width, new_height),
      SkSamplingOptions(SkFilterMode::kLinear, SkMipmapMode::kLinear),
      nullptr,
      SkCanvas::kFast_SrcRectConstraint);

  LOG(INFO) << "Resized screenshot from " << orig_width << "x" << orig_height
            << " to " << new_width << "x" << new_height;

  return resized;
}

}  // namespace atlas

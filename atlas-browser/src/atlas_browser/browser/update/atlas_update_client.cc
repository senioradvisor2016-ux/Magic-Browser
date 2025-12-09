// Copyright 2025 Atlas Browser Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license.

#include "atlas_browser/browser/update/atlas_update_client.h"

#include "base/files/file_util.h"
#include "base/hash/sha256.h"
#include "base/json/json_reader.h"
#include "base/json/json_writer.h"
#include "base/logging.h"
#include "base/strings/string_number_conversions.h"
#include "base/system/sys_info.h"
#include "base/task/thread_pool.h"
#include "build/build_config.h"
#include "net/base/load_flags.h"
#include "net/traffic_annotation/network_traffic_annotation.h"
#include "services/network/public/cpp/resource_request.h"
#include "services/network/public/cpp/simple_url_loader.h"

namespace atlas {

namespace {

// Current Atlas Browser version
constexpr char kCurrentVersion[] = "1.0.0";

// Maximum response size (1 MB)
constexpr size_t kMaxResponseSize = 1024 * 1024;

// Maximum download size (500 MB)
constexpr size_t kMaxDownloadSize = 500 * 1024 * 1024;

constexpr net::NetworkTrafficAnnotationTag kTrafficAnnotation =
    net::DefineNetworkTrafficAnnotation("atlas_update_client", R"(
        semantics {
          sender: "Atlas Browser Update Client"
          description:
            "Checks for and downloads Atlas Browser updates."
          trigger:
            "Periodic automatic check or user-initiated check."
          data:
            "Current version, OS, architecture."
          destination: OTHER
          destination_other: "Atlas Browser update server"
        }
        policy {
          cookies_allowed: NO
          setting:
            "Updates can be disabled in Atlas settings."
          policy_exception_justification:
            "Essential for keeping browser secure and up-to-date."
        })");

std::string GetPlatformString() {
#if BUILDFLAG(IS_WIN)
  return "windows";
#elif BUILDFLAG(IS_MAC)
  return "macos";
#elif BUILDFLAG(IS_LINUX)
  return "linux";
#else
  return "unknown";
#endif
}

std::string GetArchString() {
#if defined(ARCH_CPU_X86_64)
  return "x64";
#elif defined(ARCH_CPU_ARM64)
  return "arm64";
#elif defined(ARCH_CPU_X86)
  return "x86";
#else
  return "unknown";
#endif
}

}  // namespace

// =============================================================================
// Singleton
// =============================================================================

AtlasUpdateClient* AtlasUpdateClient::GetInstance() {
  static base::NoDestructor<AtlasUpdateClient> instance;
  return instance.get();
}

// =============================================================================
// Constructor / Destructor
// =============================================================================

AtlasUpdateClient::AtlasUpdateClient() {
  LOG(INFO) << "AtlasUpdateClient initialized, current version: "
            << kCurrentVersion;
}

AtlasUpdateClient::~AtlasUpdateClient() = default;

// =============================================================================
// Observer Management
// =============================================================================

void AtlasUpdateClient::AddObserver(Observer* observer) {
  observers_.AddObserver(observer);
}

void AtlasUpdateClient::RemoveObserver(Observer* observer) {
  observers_.RemoveObserver(observer);
}

// =============================================================================
// Configuration
// =============================================================================

void AtlasUpdateClient::Configure(const Config& config) {
  config_ = config;

  // Restart timer with new interval
  if (auto_check_enabled_) {
    check_timer_.Stop();
    check_timer_.Start(FROM_HERE, config_.check_interval,
                       base::BindRepeating(&AtlasUpdateClient::OnCheckTimer,
                                           weak_factory_.GetWeakPtr()));
  }
}

void AtlasUpdateClient::SetAutoCheckEnabled(bool enabled) {
  auto_check_enabled_ = enabled;

  if (enabled && !check_timer_.IsRunning()) {
    check_timer_.Start(FROM_HERE, config_.check_interval,
                       base::BindRepeating(&AtlasUpdateClient::OnCheckTimer,
                                           weak_factory_.GetWeakPtr()));
  } else if (!enabled) {
    check_timer_.Stop();
  }
}

void AtlasUpdateClient::SetCheckInterval(base::TimeDelta interval) {
  config_.check_interval = interval;
  if (check_timer_.IsRunning()) {
    check_timer_.Stop();
    check_timer_.Start(FROM_HERE, interval,
                       base::BindRepeating(&AtlasUpdateClient::OnCheckTimer,
                                           weak_factory_.GetWeakPtr()));
  }
}

const base::Version& AtlasUpdateClient::GetCurrentVersion() const {
  static const base::Version version(kCurrentVersion);
  return version;
}

// =============================================================================
// Update Check
// =============================================================================

void AtlasUpdateClient::CheckForUpdates() {
  if (state_ == State::kCheckingForUpdates ||
      state_ == State::kDownloading) {
    LOG(INFO) << "Update check already in progress";
    return;
  }

  LOG(INFO) << "Checking for updates...";
  SetState(State::kCheckingForUpdates);

  // Build request URL with query parameters
  GURL url(config_.update_url);
  url = url.Resolve("?");

  // Build request body
  base::Value::Dict body;
  body.Set("version", kCurrentVersion);
  body.Set("platform", GetPlatformString());
  body.Set("arch", GetArchString());
  body.Set("channel", config_.channel);
  body.Set("allow_delta", config_.allow_delta_updates);

  std::string body_json;
  base::JSONWriter::Write(body, &body_json);

  // Create request
  auto request = std::make_unique<network::ResourceRequest>();
  request->url = GURL(config_.update_url);
  request->method = "POST";
  request->load_flags = net::LOAD_DISABLE_CACHE;
  request->headers.SetHeader("Content-Type", "application/json");
  request->headers.SetHeader("User-Agent",
                             "AtlasBrowser/" + std::string(kCurrentVersion));

  url_loader_ =
      network::SimpleURLLoader::Create(std::move(request), kTrafficAnnotation);
  url_loader_->AttachStringForUpload(body_json, "application/json");

  url_loader_->DownloadToString(
      GetURLLoaderFactory().get(),
      base::BindOnce(&AtlasUpdateClient::OnUpdateCheckComplete,
                     weak_factory_.GetWeakPtr()),
      kMaxResponseSize);
}

void AtlasUpdateClient::OnCheckTimer() {
  CheckForUpdates();
}

void AtlasUpdateClient::OnUpdateCheckComplete(
    std::unique_ptr<std::string> response) {
  if (!response || response->empty()) {
    LOG(ERROR) << "Update check failed: empty response";
    NotifyError("Failed to check for updates");
    return;
  }

  UpdateInfo info;
  if (!ParseUpdateResponse(*response, &info)) {
    LOG(ERROR) << "Update check failed: invalid response";
    NotifyError("Invalid update response");
    return;
  }

  if (IsNewerVersion(info.version)) {
    LOG(INFO) << "Update available: " << info.version;
    update_info_ = info;
    SetState(State::kUpdateAvailable);

    for (auto& observer : observers_) {
      observer.OnUpdateAvailable(info);
    }
  } else {
    LOG(INFO) << "No update available, current version is latest";
    SetState(State::kIdle);
  }
}

bool AtlasUpdateClient::ParseUpdateResponse(const std::string& response,
                                             UpdateInfo* info) {
  auto json = base::JSONReader::Read(response);
  if (!json || !json->is_dict()) {
    return false;
  }

  const auto& dict = json->GetDict();

  // Check if update is available
  auto* available = dict.FindBool("update_available");
  if (!available || !*available) {
    return true;  // No update, but valid response
  }

  // Parse update info
  auto* version = dict.FindString("version");
  if (!version) return false;
  info->version = *version;

  auto* url = dict.FindString("download_url");
  if (!url) return false;
  info->download_url = *url;

  auto* hash = dict.FindString("sha256");
  if (hash) info->sha256_hash = *hash;

  auto* notes = dict.FindString("release_notes");
  if (notes) info->release_notes = *notes;

  auto size = dict.FindInt("file_size");
  if (size) info->file_size_bytes = *size;

  auto* critical = dict.FindBool("critical");
  info->is_critical = critical && *critical;

  auto* delta = dict.FindBool("is_delta");
  info->is_delta = delta && *delta;

  return true;
}

bool AtlasUpdateClient::IsNewerVersion(const std::string& new_version) const {
  base::Version current(kCurrentVersion);
  base::Version update(new_version);

  if (!current.IsValid() || !update.IsValid()) {
    return false;
  }

  return update > current;
}

// =============================================================================
// Download
// =============================================================================

void AtlasUpdateClient::DownloadUpdate() {
  if (state_ != State::kUpdateAvailable) {
    LOG(ERROR) << "No update available to download";
    return;
  }

  LOG(INFO) << "Downloading update: " << update_info_.version;
  SetState(State::kDownloading);

  // Create request
  auto request = std::make_unique<network::ResourceRequest>();
  request->url = GURL(update_info_.download_url);
  request->method = "GET";
  request->load_flags = net::LOAD_DISABLE_CACHE;

  url_loader_ =
      network::SimpleURLLoader::Create(std::move(request), kTrafficAnnotation);

  // Download to temp file
  base::FilePath temp_dir;
  if (!base::GetTempDir(&temp_dir)) {
    NotifyError("Failed to get temp directory");
    return;
  }

  base::FilePath download_path =
      temp_dir.AppendASCII("atlas_update_" + update_info_.version);

  url_loader_->SetOnDownloadProgressCallback(
      base::BindRepeating(&AtlasUpdateClient::OnDownloadProgress,
                          weak_factory_.GetWeakPtr()));

  url_loader_->DownloadToFile(
      GetURLLoaderFactory().get(),
      base::BindOnce(&AtlasUpdateClient::OnDownloadComplete,
                     weak_factory_.GetWeakPtr()),
      download_path);
}

void AtlasUpdateClient::OnDownloadProgress(uint64_t current) {
  if (update_info_.file_size_bytes > 0) {
    int percent = static_cast<int>(
        (current * 100) / update_info_.file_size_bytes);
    for (auto& observer : observers_) {
      observer.OnDownloadProgress(percent);
    }
  }
}

void AtlasUpdateClient::OnDownloadComplete(base::FilePath path) {
  if (path.empty()) {
    LOG(ERROR) << "Download failed";
    NotifyError("Download failed");
    return;
  }

  LOG(INFO) << "Download complete: " << path;

  // Verify download
  if (!VerifyUpdate(path)) {
    LOG(ERROR) << "Update verification failed";
    base::DeleteFile(path);
    NotifyError("Update verification failed");
    return;
  }

  downloaded_update_path_ = path;
  SetState(State::kReadyToInstall);

  for (auto& observer : observers_) {
    observer.OnUpdateReady();
  }
}

bool AtlasUpdateClient::VerifyUpdate(const base::FilePath& path) {
  if (update_info_.sha256_hash.empty()) {
    LOG(WARNING) << "No hash provided, skipping verification";
    return true;
  }

  // Read file and compute hash
  std::string contents;
  if (!base::ReadFileToString(path, &contents)) {
    LOG(ERROR) << "Failed to read update file";
    return false;
  }

  std::string hash = base::SHA256HashString(contents);
  std::string hash_hex = base::HexEncode(hash.data(), hash.size());

  if (!base::EqualsCaseInsensitiveASCII(hash_hex, update_info_.sha256_hash)) {
    LOG(ERROR) << "Hash mismatch: expected " << update_info_.sha256_hash
               << ", got " << hash_hex;
    return false;
  }

  LOG(INFO) << "Update verified successfully";
  return true;
}

// =============================================================================
// Apply Update
// =============================================================================

void AtlasUpdateClient::ApplyUpdate() {
  if (state_ != State::kReadyToInstall) {
    LOG(ERROR) << "No update ready to install";
    return;
  }

  LOG(INFO) << "Applying update...";
  SetState(State::kInstalling);

  // Platform-specific update application
#if BUILDFLAG(IS_WIN)
  // Windows: Run installer
  base::CommandLine cmd(downloaded_update_path_);
  cmd.AppendSwitch("/silent");
  base::LaunchProcess(cmd, base::LaunchOptions());
#elif BUILDFLAG(IS_MAC)
  // macOS: Open DMG or run installer
  base::CommandLine cmd(base::FilePath("/usr/bin/open"));
  cmd.AppendArgPath(downloaded_update_path_);
  base::LaunchProcess(cmd, base::LaunchOptions());
#elif BUILDFLAG(IS_LINUX)
  // Linux: Depends on package format
  // For now, just log
  LOG(INFO) << "Update downloaded to: " << downloaded_update_path_;
#endif

  // Note: In a real implementation, the browser would restart
  // after the update is installed
}

void AtlasUpdateClient::DismissUpdate() {
  if (state_ == State::kUpdateAvailable) {
    LOG(INFO) << "Update dismissed";
    SetState(State::kIdle);
  }
}

// =============================================================================
// Helpers
// =============================================================================

void AtlasUpdateClient::SetState(State state) {
  if (state_ != state) {
    state_ = state;
    for (auto& observer : observers_) {
      observer.OnUpdateStateChanged(state);
    }
  }
}

void AtlasUpdateClient::NotifyError(const std::string& error) {
  SetState(State::kError);
  for (auto& observer : observers_) {
    observer.OnUpdateError(error);
  }
}

scoped_refptr<network::SharedURLLoaderFactory>
AtlasUpdateClient::GetURLLoaderFactory() {
  // In a real implementation, this would get the system URL loader factory
  return nullptr;
}

}  // namespace atlas

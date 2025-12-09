// Copyright 2025 Atlas Browser Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license.

#ifndef ATLAS_BROWSER_BROWSER_UPDATE_ATLAS_UPDATE_CLIENT_H_
#define ATLAS_BROWSER_BROWSER_UPDATE_ATLAS_UPDATE_CLIENT_H_

#include <memory>
#include <string>

#include "base/callback.h"
#include "base/memory/weak_ptr.h"
#include "base/timer/timer.h"
#include "base/version.h"

namespace network {
class SimpleURLLoader;
class SharedURLLoaderFactory;
}  // namespace network

namespace atlas {

// AtlasUpdateClient handles checking for and downloading browser updates.
class AtlasUpdateClient {
 public:
  // Update information
  struct UpdateInfo {
    std::string version;
    std::string download_url;
    std::string sha256_hash;
    std::string release_notes;
    int64_t file_size_bytes;
    bool is_critical;
    bool is_delta;  // Delta update vs full update
  };

  // Update state
  enum class State {
    kIdle,
    kCheckingForUpdates,
    kUpdateAvailable,
    kDownloading,
    kReadyToInstall,
    kInstalling,
    kError,
  };

  // Observer interface
  class Observer {
   public:
    virtual ~Observer() = default;
    virtual void OnUpdateStateChanged(State state) {}
    virtual void OnUpdateAvailable(const UpdateInfo& info) {}
    virtual void OnDownloadProgress(int percent_complete) {}
    virtual void OnUpdateReady() {}
    virtual void OnUpdateError(const std::string& error) {}
  };

  // Singleton access
  static AtlasUpdateClient* GetInstance();

  AtlasUpdateClient();
  ~AtlasUpdateClient();

  AtlasUpdateClient(const AtlasUpdateClient&) = delete;
  AtlasUpdateClient& operator=(const AtlasUpdateClient&) = delete;

  // Observer management
  void AddObserver(Observer* observer);
  void RemoveObserver(Observer* observer);

  // Check for updates
  void CheckForUpdates();

  // Download available update
  void DownloadUpdate();

  // Apply downloaded update (will restart browser)
  void ApplyUpdate();

  // Dismiss update notification
  void DismissUpdate();

  // Get current state
  State GetState() const { return state_; }

  // Get update info (only valid when state is kUpdateAvailable or later)
  const UpdateInfo& GetUpdateInfo() const { return update_info_; }

  // Get current version
  const base::Version& GetCurrentVersion() const;

  // Enable/disable automatic update checks
  void SetAutoCheckEnabled(bool enabled);
  bool IsAutoCheckEnabled() const { return auto_check_enabled_; }

  // Set update check interval (default: 4 hours)
  void SetCheckInterval(base::TimeDelta interval);

  // Configuration
  struct Config {
    std::string update_url = "https://updates.atlasbrowser.com/api/v1/check";
    std::string channel = "stable";  // stable, beta, dev
    bool allow_delta_updates = true;
    base::TimeDelta check_interval = base::Hours(4);
  };

  void Configure(const Config& config);

 private:
  // Check timer callback
  void OnCheckTimer();

  // Network callbacks
  void OnUpdateCheckComplete(std::unique_ptr<std::string> response);
  void OnDownloadProgress(uint64_t current);
  void OnDownloadComplete(base::FilePath path);

  // Parse update response
  bool ParseUpdateResponse(const std::string& response, UpdateInfo* info);

  // Compare versions
  bool IsNewerVersion(const std::string& new_version) const;

  // Verify downloaded update
  bool VerifyUpdate(const base::FilePath& path);

  // Set state and notify observers
  void SetState(State state);
  void NotifyError(const std::string& error);

  // Get URL loader factory
  scoped_refptr<network::SharedURLLoaderFactory> GetURLLoaderFactory();

  State state_ = State::kIdle;
  UpdateInfo update_info_;
  Config config_;

  bool auto_check_enabled_ = true;
  base::RepeatingTimer check_timer_;

  base::FilePath downloaded_update_path_;

  std::unique_ptr<network::SimpleURLLoader> url_loader_;

  base::ObserverList<Observer> observers_;
  base::WeakPtrFactory<AtlasUpdateClient> weak_factory_{this};
};

}  // namespace atlas

#endif  // ATLAS_BROWSER_BROWSER_UPDATE_ATLAS_UPDATE_CLIENT_H_

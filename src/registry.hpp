// backend_registry.hpp

#pragma once
#include <chrono>
#include <mutex>
#include <optional>
#include <string>
#include <vector>

struct Backend {
  std::string host;
  int port;
  std::chrono::steady_clock::time_point last_heartbeat;

  std::string address() const { return host + ":" + std::to_string(port); }
};

class BackendRegistry {
public:
  void register_backend(const std::string& host, int port) {
    std::scoped_lock lock(mutex_);
    for (auto& backend : backends_) {
      if (backend.host == host && backend.port == port) {
        backend.last_heartbeat = std::chrono::steady_clock::now();
        return;
      }
    }
    backends_.push_back(Backend{host, port, std::chrono::steady_clock::now()});
  }

  std::optional<Backend> select_backend() {
    std::scoped_lock lock(mutex_);
    if (backends_.empty())
      return std::nullopt;
    index_ = (index_ + 1) % backends_.size();
    return backends_[index_];
  }

  void prune_stale(std::chrono::seconds max_age) {
    auto now = std::chrono::steady_clock::now();
    std::scoped_lock lock(mutex_);
    backends_.erase(std::remove_if(backends_.begin(), backends_.end(),
                                   [&](const Backend& b) {
                                     return now - b.last_heartbeat > max_age;
                                   }),
                    backends_.end());
  }

private:
  std::vector<Backend> backends_;
  std::mutex mutex_;
  size_t index_ = 0;
};

#pragma once

#include <boost/asio.hpp>
#include <atomic>
#include <chrono>
#include <iostream>
#include <memory>

namespace net = boost::asio;

class TokenBucketRateLimiter
    : public std::enable_shared_from_this<TokenBucketRateLimiter> {
public:
  explicit TokenBucketRateLimiter(net::io_context& ctx) : ctx_(ctx) {}

  void start_replenisher() {
    net::co_spawn(
        ctx_,
        [weak_self = weak_from_this()]() -> net::awaitable<void> {
          net::steady_timer timer(co_await net::this_coro::executor);

          while (true) {
            auto self = weak_self.lock();
            if (!self) {
              std::cout << "TokenBucketRateLimiter destroyed, exiting coroutine"
                        << std::endl;
              co_return;
            }
            std::cout << "Replenishing tokens" << std::endl;
            self->tokens = MAX_TOKENS;
            timer.expires_after(std::chrono::seconds(1));
            co_await timer.async_wait(net::use_awaitable);
          }
        },
        net::detached);
  }

  net::awaitable<bool> is_overloaded() {
    auto current_tokens = tokens.load();
    while (current_tokens > 0) {
      if (tokens.compare_exchange_weak(current_tokens, current_tokens - 1)) {
        co_return false;
      }
    }
    co_return true;
  }

private:
  static const int MAX_TOKENS = 200;
  std::atomic<int> tokens{MAX_TOKENS};
  net::io_context& ctx_;
};

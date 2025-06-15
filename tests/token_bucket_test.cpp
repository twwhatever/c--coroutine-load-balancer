#include "token_bucket_rate_limiter.hpp"

#include <gtest/gtest.h>

using namespace std::chrono_literals;

TEST(TokenBucketRateLimiter, NotOverloadedWhenTokensAvailable) {
  net::io_context ctx;
  auto limiter = std::make_shared<TokenBucketRateLimiter>(ctx);

  auto fut = net::co_spawn(ctx, limiter->is_overloaded(), net::use_future);
  ctx.run();
  EXPECT_FALSE(fut.get());
}

TEST(TokenBucketRateLimiter, BecomesOverloadedAfterTokensExhausted) {
  net::io_context ctx;
  auto limiter = std::make_shared<TokenBucketRateLimiter>(ctx);

  std::vector<std::future<bool>> results;
  for (int i = 0; i < 201; ++i) {
    results.push_back(net::co_spawn(ctx, limiter->is_overloaded(), net::use_future));
  }
  ctx.run();

  for (int i = 0; i < 200; ++i) {
    EXPECT_FALSE(results[i].get());
  }
  EXPECT_TRUE(results.back().get());
}

#include "register_handler.hpp"
#include "registry.hpp"

#include <gtest/gtest.h>

TEST(RegisterHandler, RegistersBackend) {
  BackendRegistry reg;
  http::request<http::string_body> req{http::verb::post, "/register", 11};
  req.set(http::field::content_type, "application/json");
  req.body() = R"({"host":"127.0.0.1","port":9000})";
  req.prepare_payload();

  auto res = handle_register(req, reg);
  EXPECT_EQ(res.result(), http::status::ok);

  auto backend = reg.select_backend();
  ASSERT_TRUE(backend.has_value());
  EXPECT_EQ(backend->host, "127.0.0.1");
  EXPECT_EQ(backend->port, 9000);
}

TEST(RegisterHandler, HandlesBadJson) {
  BackendRegistry reg;
  http::request<http::string_body> req{http::verb::post, "/register", 11};
  req.set(http::field::content_type, "application/json");
  req.body() = "{bad json}";
  req.prepare_payload();

  auto res = handle_register(req, reg);
  EXPECT_EQ(res.result(), http::status::bad_request);
}


#include "register_handler.hpp"
#include "registry.hpp"
#include "token_bucket_rate_limiter.hpp"

#include <boost/asio.hpp>
#include <boost/asio/awaitable.hpp>
#include <boost/asio/co_spawn.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <chrono>
#include <iostream>
#include <memory>

namespace beast = boost::beast;
namespace http = beast::http;
namespace net = boost::asio;
using tcp = net::ip::tcp;

net::io_context ioc;

BackendRegistry registry;

class RandomRateLimiter {
public:
  net::awaitable<bool> is_overloaded() {
    // Randomly simulate overload
    co_return rand() % 10 == 0; // 10% chance to simulate busy
  }
};

auto token_bucket = std::make_shared<TokenBucketRateLimiter>(ioc);

template <typename O>
net::awaitable<void> handle_request(tcp::socket client_socket,
                                    std::shared_ptr<O>& load_balancer) {
  beast::flat_buffer buffer;
  http::request<http::string_body> client_req;

  try {
    co_await http::async_read(client_socket, buffer, client_req,
                              net::use_awaitable);
  } catch (std::exception& e) {
    std::cerr << "Error reading client request: " << e.what() << std::endl;
    co_return;
  }

  try {
    if (co_await load_balancer->is_overloaded()) {
      std::cout << "Too many requests, return 429" << std::endl;
      http::response<http::string_body> resp{http::status::too_many_requests,
                                             client_req.version()};
      resp.set(http::field::server, "MiniRateLimiter");
      resp.set(http::field::content_type, "text/plain");
      resp.body() = "Too many requests, please retry later.";
      resp.prepare_payload();
      co_await http::async_write(client_socket, resp, net::use_awaitable);
      co_return;
    }
  } catch (std::exception& e) {
    std::cerr << "Error returning 429 response: " << e.what() << std::endl;
    co_return;
  }

  std::cout << "Forward to backend" << std::endl;
  tcp::socket backend_socket(co_await net::this_coro::executor);
  http::response<http::string_body> backend_resp;
  bool backend_failed = false;

  auto maybe_backend = registry.select_backend();
  if (!maybe_backend) {
    // return 503 - no backend available
    http::response<http::string_body> resp{http::status::service_unavailable,
                                           client_req.version()};
    resp.set(http::field::server, "MiniRateLimiter");
    resp.set(http::field::content_type, "text/plain");
    resp.body() = "Service unavailable, please retry later.";
    resp.prepare_payload();
    co_await http::async_write(client_socket, resp, net::use_awaitable);
    co_return;
  }

  Backend backend = *maybe_backend;
  std::string host = backend.host;
  int port = backend.port;

  try {
    tcp::resolver resolver(co_await net::this_coro::executor);
    auto endpoints = co_await resolver.async_resolve(host, std::to_string(port),
                                                     net::use_awaitable);
    co_await net::async_connect(backend_socket, endpoints, net::use_awaitable);

    // Forward client's request to the backend.
    co_await http::async_write(backend_socket, client_req, net::use_awaitable);

    beast::flat_buffer backend_buffer;
    co_await http::async_read(backend_socket, backend_buffer, backend_resp,
                              net::use_awaitable);

    std::cout << "Received response from backend" << std::endl;
  } catch (std::exception& e) {
    std::cerr << "Error communicating with backend: " << e.what() << std::endl;
    backend_failed = true;
  }

  if (backend_failed) {
    try {
      // Prepare 502 Bad Gateway response
      http::response<http::string_body> resp{http::status::bad_gateway, 11};
      resp.set(http::field::server, "MiniRateLimiter");
      resp.set(http::field::content_type, "text/plain");
      resp.body() = "Bad Gateway: backend connection failed";
      resp.prepare_payload();

      // Send error response to client
      boost::system::error_code ignored_ec;
      co_await http::async_write(
          client_socket, resp,
          net::redirect_error(net::use_awaitable, ignored_ec));
    } catch (std::exception& e) {
      std::cerr << "Error returning 502 response to client: " << e.what()
                << std::endl;
    }

    co_return;
  }

  try {
    co_await http::async_write(client_socket, backend_resp, net::use_awaitable);

    std::cout << "Returned response to client!" << std::endl;
  } catch (std::exception& e) {
    std::cerr << "Error returning response to client: " << e.what()
              << std::endl;
    // N.B. we should really return an error to the client here.
  }
}

net::awaitable<void> handle_control_request(tcp::socket control_socket,
                                            BackendRegistry& registry) {
  beast::flat_buffer buffer;
  http::request<http::string_body> control_req;

  try {
    co_await http::async_read(control_socket, buffer, control_req,
                              net::use_awaitable);
  } catch (std::exception& e) {
    std::cerr << "Error reading control request: " << e.what() << std::endl;
    co_return;
  }

  try {
    auto control_response = handle_register(control_req, registry);

    co_await http::async_write(control_socket, control_response,
                               net::use_awaitable);

    std::cout << "Returned control resposne!" << std::endl;
  } catch (std::exception& e) {
    std::cerr << "Error control resposne: " << e.what() << std::endl;
    // N.B. we should really return an error here.
  }
}

int main() {
  try {
    net::co_spawn(
        ioc,
        []() -> net::awaitable<void> {
          tcp::acceptor acceptor(co_await net::this_coro::executor,
                                 {tcp::v4(), 8080});
          std::cout << "Listening for client requests on port 8080"
                    << std::endl;

          while (true) {
            tcp::socket socket =
                co_await acceptor.async_accept(net::use_awaitable);
            std::cout << "Received client request!" << std::endl;
            net::co_spawn(ioc, handle_request(std::move(socket), token_bucket),
                          net::detached);
          }
        }(),
        net::detached);
    net::co_spawn(
        ioc,
        []() -> net::awaitable<void> {
          tcp::acceptor acceptor(co_await net::this_coro::executor,
                                 {tcp::v4(), 8081});
          std::cout << "Listening for control requests on port 8081"
                    << std::endl;

          while (true) {
            tcp::socket socket =
                co_await acceptor.async_accept(net::use_awaitable);
            std::cout << "Received control request!" << std::endl;
            net::co_spawn(ioc,
                          handle_control_request(std::move(socket), registry),
                          net::detached);
          }
        }(),
        net::detached);
    token_bucket->start_replenisher();
    ioc.run();
  } catch (std::exception& e) {
    std::cerr << "Fatal error: " << e.what() << std::endl;
  }
}

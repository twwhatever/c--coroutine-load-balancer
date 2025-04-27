
#include <iostream>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/asio.hpp>
#include <boost/asio/awaitable.hpp>
#include <boost/asio/co_spawn.hpp>

namespace beast = boost::beast;
namespace http = beast::http;
namespace net = boost::asio;
using tcp = net::ip::tcp;

net::io_context ioc;

bool is_overloaded() {
  // Randomly simulate overload
  return rand() % 10 == 0; // 10% chance to simulate busy
}

net::awaitable<void> handle_request(tcp::socket client_socket) {
  try {
    beast::flat_buffer buffer;

    http::request<http::string_body> client_req;
    co_await http::async_read(
      client_socket,
      buffer,
      client_req,
      net::use_awaitable);
    
    if (is_overloaded()) {
      std::cout << "Too many requests, return 429" << std::endl;
      http::response<http::string_body> resp{
        http::status::too_many_requests,
        client_req.version()};
      resp.set(http::field::server, "MiniLoadBalancer");
      resp.set(http::field::content_type, "text/plain");
      resp.body() = "Too many requests, please retry later.";
      resp.prepare_payload();
      co_await http::async_write(client_socket, resp, net::use_awaitable);
      co_return;
    }

    std::cout << "Forward to backend" << std::endl;

  } catch (std::exception& e) {
    std::cerr << "Error handling request: " << e.what() << std::endl;
  }
}

int main() {
  try {
    net::co_spawn(
      ioc, 
      []() -> net::awaitable<void> {
        tcp::acceptor acceptor(
          co_await net::this_coro::executor,
          {tcp::v4(), 8080});
          std::cout << "Listening on port 8080" << std::endl;
        
          while (true) {
            tcp::socket socket = co_await acceptor.async_accept(
              net::use_awaitable);
            std::cout << "Received client request!" << std::endl;
            net::co_spawn(
              ioc, handle_request(std::move(socket)), net::detached);
          }
      }(),
      net::detached);

    ioc.run();
  } catch (std::exception& e) {
    std::cerr << "Fatal error: " << e.what() << std::endl;
  }
}

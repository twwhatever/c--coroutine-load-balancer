// register_handler.hpp

#pragma once
#include "registry.hpp"

#include <boost/beast/http.hpp>
#include <nlohmann/json.hpp>

namespace http = boost::beast::http;
using json = nlohmann::json;

template <class Body, class Allocator>
http::response<http::string_body>
handle_register(const http::request<Body, http::basic_fields<Allocator>>& req,
                BackendRegistry& registry) {
  try {
    auto j = json::parse(req.body());
    std::string host = j.at("host").template get<std::string>();
    int port = j.at("port").template get<int>();

    registry.register_backend(host, port);

    http::response<http::string_body> res{http::status::ok, req.version()};
    res.set(http::field::content_type, "application/json");
    res.body() = R"({"status": "ok"})";
    res.prepare_payload();
    return res;
  } catch (const std::exception& e) {
    http::response<http::string_body> res{http::status::bad_request,
                                          req.version()};
    res.set(http::field::content_type, "application/json");
    res.body() = R"({"error": ")" + std::string(e.what()) + R"("})";
    res.prepare_payload();
    return res;
  }
}

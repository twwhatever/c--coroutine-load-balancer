# C++ Coroutine Load Balancer

This repository contains a small pedagogical project that showcases modern C++20 coroutines while implementing a minimal HTTP load balancer with rate limiting.  The code is intentionally compact so that the coroutine and algorithmic techniques are easy to read in a single place.

The implementation uses **Boost.Asio/Beast** for networking and the [token bucket](https://en.wikipedia.org/wiki/Token_bucket) algorithm for rate limiting.  Backends are selected in a simple round‑robin fashion.

## Building

1. Ensure `conan` and a C++20 compiler are available.
2. Run the helper script:

```bash
./build.sh
```

This installs dependencies with Conan and builds the `rate_limiter` executable under `build/`.

Passing `--clean` to `build.sh` removes the old build directory before building.

## Running the Example

Start one or more backend HTTP servers (for example using Python's built‑in server) and then run the balancer:

```bash
python3 -m http.server 9000 --bind 127.0.0.1 &
./build/rate_limiter
```

Backends register themselves via the control port (8081):

```bash
curl -X POST http://127.0.0.1:8081/register \
     -H "Content-Type: application/json" \
     -d '{"host":"127.0.0.1","port":9000}'
```

Client requests are sent to port 8080 and are forwarded to one of the registered backends.  When the rate limiter deems the system overloaded, the client receives HTTP 429.

## Tests

Two scripts demonstrate the project:

* `test.sh` – builds the balancer, starts a temporary backend and issues a few requests.
* `stress_test.sh` – optional load test using the `vegeta` tool and multiple backends.

Both scripts create logs in the `log/` directory.

## License

This project is released under the MIT License.  See the [LICENSE](LICENSE) file for details.

#include <core/core.hpp>
#include <iostream>
#include <sstream>
#include <stdlib.h>

int main([[maybe_unused]] int argc, [[maybe_unused]] char **argv) {
  constexpr long long fasten = webframe::core::_application::init();
  std::atomic<long long> pass{fasten};
  webframe::core::application app;
  app.route("/{number}/2",
            [&](long long steps) {
              for (unsigned long long i = 0; i < (1ull << steps); i++) {
                pass.fetch_add(rand(), std::memory_order_relaxed);
              }
              return "Hello World!";
            })
      .route("/{number}/1", [&](long long steps) {
        for (unsigned long long i = 0; i < (1ull << steps); i++) {
          pass.fetch_add(1, std::memory_order_relaxed);
        }
        return "Hello World!";
      });
  const char *port = argv[1];
  app.run(port, 1).wait_end(port);
}

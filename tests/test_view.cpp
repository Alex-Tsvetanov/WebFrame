// test_view.cpp - Unit tests for view abstraction types

#include "coroute/view/view_types.hpp"
#include <catch2/catch_test_macros.hpp>
#include <cmath>
#include <string>
#include <vector>

using namespace coroute;

// ============================================================================
// Test ViewModels
// ============================================================================

struct SimpleVm {
  std::string name;
  int value;
};

// ADL to_json for SimpleVm
inline void to_json(nlohmann::json &j, const SimpleVm &vm) {
  j = {{"name", vm.name}, {"value", vm.value}};
}

struct ComplexVm {
  std::string title;
  std::vector<std::string> items;
  bool active;
};

inline void to_json(nlohmann::json &j, const ComplexVm &vm) {
  j = {{"title", vm.title}, {"items", vm.items}, {"active", vm.active}};
}

// ============================================================================
// ViewTemplates Tests
// ============================================================================

TEST_CASE("ViewTemplates - single template for all platforms", "[view]") {
  ViewTemplates vt{"common_template"};

  CHECK(vt.web == "common_template");
  CHECK(vt.mobile == "common_template");
  CHECK(vt.desktop == "common_template");
}

TEST_CASE("ViewTemplates - platform-specific templates", "[view]") {
  ViewTemplates vt{"web.html", "mobile.html", "desktop.html"};

  CHECK(vt.web == "web.html");
  CHECK(vt.mobile == "mobile.html");
  CHECK(vt.desktop == "desktop.html");
}

// ============================================================================
// ViewResult Tests
// ============================================================================

TEST_CASE("ViewResult - stores ViewModel and templates", "[view]") {
  SimpleVm vm{.name = "test", .value = 42};
  ViewResult<SimpleVm> result{.templates = ViewTemplates{"test_template"},
                              .model = std::move(vm)};

  CHECK(result.templates.web == "test_template");
  CHECK(result.model.name == "test");
  CHECK(result.model.value == 42);
}

TEST_CASE("ViewResult - with complex ViewModel", "[view]") {
  ComplexVm vm{.title = "Test Title", .items = {"A", "B", "C"}, .active = true};
  ViewResult<ComplexVm> result{
      .templates = ViewTemplates{"listing", "mobile_list", "desktop_list"},
      .model = std::move(vm)};

  CHECK(result.templates.web == "listing");
  CHECK(result.templates.mobile == "mobile_list");
  CHECK(result.templates.desktop == "desktop_list");
  CHECK(result.model.title == "Test Title");
  CHECK(result.model.items.size() == 3);
  CHECK(result.model.active == true);
}

// ============================================================================
// ViewResultAny Tests (Type Erasure)
// ============================================================================

TEST_CASE("ViewResultAny - type erasure from SimpleVm", "[view]") {
  SimpleVm vm{.name = "erased", .value = 100};
  ViewResult<SimpleVm> typed_result{.templates = ViewTemplates{"simple"},
                                    .model = std::move(vm)};

  // Convert to type-erased form
  ViewResultAny erased(std::move(typed_result));

  CHECK(erased.templates.web == "simple");

  // Verify JSON conversion works
  nlohmann::json j = erased.to_json();
  CHECK(j["name"] == "erased");
  CHECK(j["value"] == 100);
}

TEST_CASE("ViewResultAny - type erasure from ComplexVm", "[view]") {
  ComplexVm vm{.title = "Complex", .items = {"X", "Y", "Z"}, .active = false};
  ViewResult<ComplexVm> typed_result{.templates = ViewTemplates{"complex"},
                                     .model = std::move(vm)};

  ViewResultAny erased(std::move(typed_result));

  nlohmann::json j = erased.to_json();
  CHECK(j["title"] == "Complex");
  CHECK(j["items"].size() == 3);
  CHECK(j["items"][0] == "X");
  CHECK(j["active"] == false);
}

TEST_CASE("ViewResultAny - preserves template information", "[view]") {
  SimpleVm vm{.name = "test", .value = 1};
  ViewResult<SimpleVm> typed_result{
      .templates = ViewTemplates{"web_t", "mobile_t", "desktop_t"},
      .model = std::move(vm)};

  ViewResultAny erased(std::move(typed_result));

  CHECK(erased.templates.web == "web_t");
  CHECK(erased.templates.mobile == "mobile_t");
  CHECK(erased.templates.desktop == "desktop_t");
}

// ============================================================================
// Edge Case Tests
// ============================================================================

// --- Empty Values ---

struct EmptyVm {
  std::string text;
  std::vector<int> numbers;
};

inline void to_json(nlohmann::json &j, const EmptyVm &vm) {
  j = {{"text", vm.text}, {"numbers", vm.numbers}};
}

TEST_CASE("ViewTemplates - empty template string", "[view][edge]") {
  ViewTemplates vt{""};

  CHECK(vt.web == "");
  CHECK(vt.mobile == "");
  CHECK(vt.desktop == "");
}

TEST_CASE("ViewResult - empty ViewModel strings", "[view][edge]") {
  EmptyVm vm{.text = "", .numbers = {}};
  ViewResult<EmptyVm> result{.templates = ViewTemplates{"empty"},
                             .model = std::move(vm)};

  CHECK(result.model.text == "");
  CHECK(result.model.numbers.empty());
}

TEST_CASE("ViewResultAny - empty ViewModel converts to valid JSON",
          "[view][edge]") {
  EmptyVm vm{.text = "", .numbers = {}};
  ViewResult<EmptyVm> typed{.templates = ViewTemplates{"empty"},
                            .model = std::move(vm)};

  ViewResultAny erased(std::move(typed));
  nlohmann::json j = erased.to_json();

  CHECK(j["text"] == "");
  CHECK(j["numbers"].is_array());
  CHECK(j["numbers"].empty());
}

// --- Special Characters ---

struct SpecialCharsVm {
  std::string html;
  std::string unicode;
  std::string quotes;
};

inline void to_json(nlohmann::json &j, const SpecialCharsVm &vm) {
  j = {{"html", vm.html}, {"unicode", vm.unicode}, {"quotes", vm.quotes}};
}

TEST_CASE("ViewResultAny - special characters in ViewModel", "[view][edge]") {
  SpecialCharsVm vm{.html = "<script>alert('xss')</script>",
                    .unicode = "Hello World", // Simplified for portability
                    .quotes = R"(He said "Hello" and 'Goodbye')"};
  ViewResult<SpecialCharsVm> typed{.templates = ViewTemplates{"special"},
                                   .model = std::move(vm)};

  ViewResultAny erased(std::move(typed));
  nlohmann::json j = erased.to_json();

  CHECK(j["html"] == "<script>alert('xss')</script>");
  CHECK(j["unicode"] == "Hello World");
  CHECK(j["quotes"] == R"(He said "Hello" and 'Goodbye')");
}

TEST_CASE("ViewTemplates - special characters in template names",
          "[view][edge]") {
  ViewTemplates vt{"path/to/template", "mobile-v2", "desktop_v1.0"};

  CHECK(vt.web == "path/to/template");
  CHECK(vt.mobile == "mobile-v2");
  CHECK(vt.desktop == "desktop_v1.0");
}

// --- Nested Objects ---

struct AddressVm {
  std::string street;
  std::string city;
};

inline void to_json(nlohmann::json &j, const AddressVm &vm) {
  j = {{"street", vm.street}, {"city", vm.city}};
}

struct PersonVm {
  std::string name;
  int age;
  AddressVm address;
  std::vector<std::string> tags;
};

inline void to_json(nlohmann::json &j, const PersonVm &vm) {
  j = {{"name", vm.name},
       {"age", vm.age},
       {"address", vm.address},
       {"tags", vm.tags}};
}

TEST_CASE("ViewResultAny - nested ViewModel objects", "[view][edge]") {
  PersonVm vm{.name = "Alice",
              .age = 30,
              .address = {.street = "123 Main St", .city = "Boston"},
              .tags = {"developer", "musician"}};
  ViewResult<PersonVm> typed{.templates = ViewTemplates{"person"},
                             .model = std::move(vm)};

  ViewResultAny erased(std::move(typed));
  nlohmann::json j = erased.to_json();

  CHECK(j["name"] == "Alice");
  CHECK(j["age"] == 30);
  CHECK(j["address"]["street"] == "123 Main St");
  CHECK(j["address"]["city"] == "Boston");
  CHECK(j["tags"].size() == 2);
  CHECK(j["tags"][0] == "developer");
}

// --- Numeric Edge Cases ---

struct NumericVm {
  int zero;
  int negative;
  int large;
  double decimal;
  bool flag;
};

inline void to_json(nlohmann::json &j, const NumericVm &vm) {
  j = {{"zero", vm.zero},
       {"negative", vm.negative},
       {"large", vm.large},
       {"decimal", vm.decimal},
       {"flag", vm.flag}};
}

TEST_CASE("ViewResultAny - numeric edge cases", "[view][edge]") {
  NumericVm vm{.zero = 0,
               .negative = -42,
               .large = 2147483647,
               .decimal = 3.14159265359,
               .flag = false};
  ViewResult<NumericVm> typed{.templates = ViewTemplates{"numeric"},
                              .model = std::move(vm)};

  ViewResultAny erased(std::move(typed));
  nlohmann::json j = erased.to_json();

  CHECK(j["zero"] == 0);
  CHECK(j["negative"] == -42);
  CHECK(j["large"] == 2147483647);
  // Use tolerance-based comparison for floating point
  double decimal_val = j["decimal"].get<double>();
  CHECK(std::abs(decimal_val - 3.14159265359) < 0.0000001);
  CHECK(j["flag"] == false);
}

// --- Large Collections ---

TEST_CASE("ViewResultAny - large vector in ViewModel", "[view][edge]") {
  std::vector<std::string> large_items;
  for (int i = 0; i < 1000; ++i) {
    large_items.push_back("item_" + std::to_string(i));
  }

  ComplexVm vm{.title = "Large Collection",
               .items = std::move(large_items),
               .active = true};
  ViewResult<ComplexVm> typed{.templates = ViewTemplates{"large"},
                              .model = std::move(vm)};

  ViewResultAny erased(std::move(typed));
  nlohmann::json j = erased.to_json();

  CHECK(j["items"].size() == 1000);
  CHECK(j["items"][0] == "item_0");
  CHECK(j["items"][999] == "item_999");
}

// --- Multiple Type Erasures ---

TEST_CASE("ViewResultAny - multiple independent erasures", "[view][edge]") {
  // Create two different ViewResultAny from different types
  SimpleVm vm1{.name = "first", .value = 1};
  ComplexVm vm2{.title = "second", .items = {"a"}, .active = true};

  ViewResult<SimpleVm> typed1{.templates = ViewTemplates{"t1"},
                              .model = std::move(vm1)};
  ViewResult<ComplexVm> typed2{.templates = ViewTemplates{"t2"},
                               .model = std::move(vm2)};

  ViewResultAny erased1(std::move(typed1));
  ViewResultAny erased2(std::move(typed2));

  // Both should work independently
  nlohmann::json j1 = erased1.to_json();
  nlohmann::json j2 = erased2.to_json();

  CHECK(j1["name"] == "first");
  CHECK(j1["value"] == 1);
  CHECK(j2["title"] == "second");
  CHECK(j2["items"].size() == 1);
}

// --- Router View Matching ---

#include "coroute/core/router.hpp"

TEST_CASE("Router - add and match view routes", "[view][router]") {
  coroute::Router router;
  bool handler_called = false;

  // Add a view route
  router.add_view("/test", [&](Request &) -> Task<ViewResultAny> {
    handler_called = true;
    SimpleVm vm{.name = "test", .value = 42};
    co_return ViewResultAny(ViewResult<SimpleVm>{
        .templates = ViewTemplates{"test"}, .model = std::move(vm)});
  });

  // Match should find the route
  auto match = router.match_view("/test");
  CHECK(match.handler != nullptr);
}

TEST_CASE("Router - view routes with parameters", "[view][router]") {
  coroute::Router router;

  router.add_view("/user/{id}", [](Request &) -> Task<ViewResultAny> {
    SimpleVm vm{.name = "user", .value = 0};
    co_return ViewResultAny(ViewResult<SimpleVm>{
        .templates = ViewTemplates{"user"}, .model = std::move(vm)});
  });

  auto match = router.match_view("/user/123");
  CHECK(match.handler != nullptr);
  CHECK(match.params.size() == 1);
  CHECK(match.params[0] == "123");
}

TEST_CASE("Router - view route not found", "[view][router]") {
  coroute::Router router;

  router.add_view("/existing", [](Request &) -> Task<ViewResultAny> {
    SimpleVm vm{.name = "", .value = 0};
    co_return ViewResultAny(ViewResult<SimpleVm>{
        .templates = ViewTemplates{"t"}, .model = std::move(vm)});
  });

  auto match = router.match_view("/nonexistent");
  CHECK(match.handler == nullptr);
}

TEST_CASE("Router - view routes independent from regular routes",
          "[view][router]") {
  coroute::Router router;

  // Add regular GET route
  router.add(HttpMethod::GET, "/api", [](Request &) -> Task<Response> {
    co_return Response::ok("API");
  });

  // Add view route with same path
  router.add_view("/api", [](Request &) -> Task<ViewResultAny> {
    SimpleVm vm{.name = "view", .value = 1};
    co_return ViewResultAny(ViewResult<SimpleVm>{
        .templates = ViewTemplates{"api"}, .model = std::move(vm)});
  });

  // Both should be findable independently
  auto regular_match = router.match(HttpMethod::GET, "/api");
  auto view_match = router.match_view("/api");

  CHECK(regular_match.handler != nullptr);
  CHECK(view_match.handler != nullptr);
}

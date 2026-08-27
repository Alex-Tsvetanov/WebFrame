#include <catch2/catch_test_macros.hpp>

#ifdef COROUTE_HAS_TEMPLATES

#include <coroute/view/deferred.hpp>

#include <string>
#include <vector>

using namespace coroute;

namespace
{

	// A ViewModel of the shape an application would actually write: some fields the
	// handler already had, one it did not want to wait for.
	struct Dashboard
	{
		std::string title;
		Deferred<int> visitors;
		Deferred<std::string> slow_report;
	};

	void to_json(nlohmann::json& json, const Dashboard& model)
	{
		json = nlohmann::json{
			{      "title",       model.title},
	        {   "visitors",    model.visitors},
	        {"slow_report", model.slow_report}
        };
	}

	Task<int> immediate(int value) { co_return value; }

}  // namespace

TEST_CASE("a resolved Deferred serialises to its value", "[deferred]")
{
	// Nothing to defer, so nothing is deferred. A value that arrived before the render
	// should cost the page nothing at all, not a slot and a round of streaming.
	const Deferred<int> ready(42);
	REQUIRE(ready.ready());

	const nlohmann::json json = ready;
	REQUIRE(json.is_number());
	REQUIRE(json.get<int>() == 42);
}

TEST_CASE("a pending Deferred is a placeholder only when someone is collecting", "[deferred]")
{
	const Deferred<int> pending;
	REQUIRE_FALSE(pending.ready());

	SECTION("with a collector it becomes a slot")
	{
		DeferredCollector collector;
		DeferredCollector::Scope scope(collector);

		const nlohmann::json json = pending;
		REQUIRE(json.is_object());
		REQUIRE(json.contains(deferred_slot_key));
		REQUIRE(json[deferred_slot_key].get<std::size_t>() == 0);
		REQUIRE(collector.pending().size() == 1);
	}

	SECTION("without a collector it is null, not a slot")
	{
		// Deliberate. Nobody is streaming, so a placeholder would be a promise the page
		// never keeps, and a hole that stays empty forever is worse than an honest null.
		const nlohmann::json json = pending;
		REQUIRE(json.is_null());
		REQUIRE(DeferredCollector::active() == nullptr);
	}
}

TEST_CASE("a model with several deferred fields registers each one", "[deferred]")
{
	Dashboard model{.title = "Overview", .visitors = {}, .slow_report = {}};

	DeferredCollector collector;
	DeferredCollector::Scope scope(collector);

	const nlohmann::json json = model;

	// The point of collecting during serialisation: a field cannot be registered
	// without emitting a placeholder, or emit one without being registered.
	REQUIRE(json["title"] == "Overview");
	REQUIRE(collector.pending().size() == 2);
	REQUIRE(json["visitors"][deferred_slot_key].get<std::size_t>() == 0);
	REQUIRE(json["slow_report"][deferred_slot_key].get<std::size_t>() == 1);

	SECTION("slot ids index the collected list")
	{
		const auto slot = json["visitors"][deferred_slot_key].get<std::size_t>();
		REQUIRE(collector.pending()[slot] != nullptr);
		REQUIRE_FALSE(collector.pending()[slot]->ready());
	}
}

TEST_CASE("a Deferred built from a task starts without being asked", "[deferred]")
{
	// The behaviour the whole design rests on. If the work only began when the value
	// was first read, several deferred fields would run one after another and deferring
	// would buy nothing over awaiting.
	const Deferred<int> value(immediate(7));
	REQUIRE(value.ready());
	REQUIRE(value.value() != nullptr);
	REQUIRE(*value.value() == 7);

	const nlohmann::json json = value;
	REQUIRE(json.get<int>() == 7);
}

TEST_CASE("on_ready fires once the value arrives, and immediately if it already has", "[deferred]")
{
	// The renderer registers a callback per slot and flushes a chunk from it, so a
	// callback that is silently dropped is a slot that never fills.

	SECTION("a value that arrives later")
	{
		const Deferred<int> pending;
		int calls = 0;
		pending.state()->on_ready([&] { ++calls; });
		REQUIRE(calls == 0);
	}

	SECTION("a value that is already there")
	{
		// Called rather than dropped, so a caller does not have to check readiness
		// first and lose the race when the answer lands in between.
		const Deferred<int> ready(1);
		int calls = 0;
		ready.state()->on_ready([&] { ++calls; });
		REQUIRE(calls == 1);
	}
}

TEST_CASE("slot ids stay stable as more are collected", "[deferred]")
{
	// A slot id is an index into the collected list and is written into the page, so
	// the two must not drift apart as later fields are serialised.
	DeferredCollector collector;
	DeferredCollector::Scope scope(collector);

	std::vector<std::size_t> slots;
	std::vector<Deferred<int>> deferreds(5);
	for (const auto& deferred : deferreds)
	{
		const nlohmann::json json = deferred;
		slots.push_back(json[deferred_slot_key].get<std::size_t>());
	}

	REQUIRE(collector.pending().size() == 5);
	for (std::size_t i = 0; i < slots.size(); ++i)
	{
		REQUIRE(slots[i] == i);
	}
}

TEST_CASE("a nested collector does not steal the outer one's slots", "[deferred]")
{
	// Scoped rather than set-and-clear, so a render that renders something else does
	// not leave the outer collector pointing at a destroyed object.
	DeferredCollector outer;
	const Deferred<int> first;
	const Deferred<int> second;

	{
		DeferredCollector::Scope outer_scope(outer);
		(void)nlohmann::json(first);

		{
			DeferredCollector inner;
			DeferredCollector::Scope inner_scope(inner);
			(void)nlohmann::json(second);
			REQUIRE(inner.pending().size() == 1);
		}

		REQUIRE(DeferredCollector::active() == &outer);
	}

	REQUIRE(outer.pending().size() == 1);
	REQUIRE(DeferredCollector::active() == nullptr);
}

#endif  // COROUTE_HAS_TEMPLATES

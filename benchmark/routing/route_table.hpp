#pragma once

// The route tables the routing experiment runs against, generated the same way for
// every arm and for the load generator.
//
// One generator rather than three, because the whole comparison rests on all three
// arms being asked the same question. A table generated separately per arm is a table
// that will eventually differ by one route and no one will notice.
//
// Two shapes, because the answer depends on which one you pick and a paper that reports
// only one is reporting an accident:
//
//   rest   every route lives under /api/v1/, the way a real API is laid out. The tree
//          carries every route down one shared spine before it branches.
//   flat   the first segment differs from the first character, so the tree branches
//          immediately. This is the shape most favourable to a prefix structure.
//
// Depth is a factor rather than a constant because the claim under test says a radix
// tree grows with path depth. A fixed depth could not show that either way.

#include <cstddef>
#include <string>
#include <string_view>
#include <vector>

namespace routebench
{

	enum class Shape
	{
		Rest,
		Flat,
	};

	struct TableSpec
	{
		size_t count = 100;
		Shape shape = Shape::Rest;
		bool params = true;  // last segment is a {id} capture
		size_t depth = 5;    // total path segments, capture segment included
	};

	inline const char* shape_name(Shape s) { return s == Shape::Rest ? "rest" : "flat"; }

	inline bool parse_shape(std::string_view text, Shape& out)
	{
		if (text == "rest")
		{
			out = Shape::Rest;
			return true;
		}
		if (text == "flat")
		{
			out = Shape::Flat;
			return true;
		}
		return false;
	}

	// A route pattern in the framework's own syntax ("/user/{id}") and one concrete
	// path that matches it. Kept together so a query can never drift from its route.
	struct Route
	{
		std::string pattern;
		std::string path;
	};

	inline std::vector<Route> generate(const TableSpec& spec)
	{
		static const char alnum[] = "abcdefghijklmnopqrstuvwxyz0123456789";

		// Two fixed segments for the rest shape, one for the flat shape, then the
		// per-route segments, then filler, then optionally the capture.
		const size_t fixed = (spec.shape == Shape::Rest) ? 4 : 1;
		const size_t tail = spec.params ? 1 : 0;
		const size_t depth = spec.depth < fixed + tail ? fixed + tail : spec.depth;

		std::vector<Route> out;
		out.reserve(spec.count);

		for (size_t i = 0; i < spec.count; ++i)
		{
			std::string head;
			if (spec.shape == Shape::Rest)
			{
				head = "/api/v1/g" + std::to_string(i / 100) + "/r" + std::to_string(i);
			}
			else
			{
				head = std::string("/") + alnum[i % 36] + std::to_string(i);
			}

			for (size_t s = fixed; s + tail < depth; ++s)
			{
				head += "/s" + std::to_string(s);
			}

			Route r;
			if (spec.params)
			{
				r.pattern = head + "/{id}";
				r.path = head + "/42";
			}
			else
			{
				// Without a capture the last segment still has to differ per route or
				// the table would collapse to one route.
				r.pattern = head;
				r.path = head;
			}
			out.push_back(std::move(r));
		}
		return out;
	}

}  // namespace routebench

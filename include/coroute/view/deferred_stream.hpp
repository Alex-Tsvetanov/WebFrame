#pragma once

#include <cstddef>
#include <string>
#include <string_view>

#include <nlohmann/json.hpp>

namespace coroute
{

	// ============================================================================
	// The wire format for a deferred slot
	// ============================================================================
	//
	// A page with deferred fields is sent in pieces. The first piece is the page
	// itself, with a hole per unresolved value and a small script that turns each hole
	// into a pending Promise. Every later piece fills one hole.
	//
	// The client contract is one function:
	//
	//     const visitors = await coroute.deferred(0);
	//
	// which is the whole point. A slot is a real Promise, so page code can await it,
	// combine slots with Promise.all, and attach a catch. A convention where the server
	// swaps some innerHTML can express none of those, and Deferred<T> on the server
	// would have no counterpart on this side of the wire.
	//
	// The DOM fallback is kept alongside it rather than instead of it: any element
	// carrying data-coroute-slot gets its text filled in too, so a page that uses no
	// JavaScript of its own still works, and one that does gets the typed version.

	// The bootstrap, emitted once near the top of the page.
	//
	// Deliberately tiny and dependency-free. A deferred page that had to download a
	// framework before it could show a hole would have spent its head start.
	[[nodiscard]] std::string deferred_runtime_script();

	// Escapes a JSON document for embedding inside a <script> element.
	//
	// This is a security boundary, not a formatting nicety. HTML parses the contents of
	// a script element looking for the closing tag before JavaScript ever sees it, so a
	// value containing </script> ends the block early and everything after it is parsed
	// as markup. A user-supplied string is enough to do that, which makes it stored XSS
	// on any page with a deferred field.
	//
	// The escapes chosen are the ones that are invisible to JSON.parse but stop the
	// HTML tokeniser: < > and & become \u form, and U+2028 and U+2029 are escaped
	// because they terminate a line in JavaScript but not in JSON.
	[[nodiscard]] std::string escape_for_script(std::string_view json_text);

	// One chunk: fills a slot and resolves its Promise.
	[[nodiscard]] std::string deferred_resolve_script(std::size_t slot, const nlohmann::json& value);

	// Puts the runtime into a rendered page.
	//
	// Position matters twice. It has to come before any page code that calls
	// coroute.deferred, since a streamed document executes scripts in the order they
	// arrive. And it cannot simply go first: a script before the doctype puts the
	// browser into quirks mode, which changes layout on a page that never asked for it.
	//
	// So it goes just inside <head> where there is one, just inside <body> otherwise,
	// and only in front of everything for a fragment with neither, where there is no
	// doctype to get in front of.
	[[nodiscard]] std::string with_deferred_runtime(std::string_view html);

	// Rejects a slot, so a page awaiting it fails instead of hanging.
	//
	// A handler that threw has to say so. Without this the Promise stays pending for as
	// long as the tab is open, which looks exactly like a slow query and is the worst
	// possible failure mode: no error, no timeout, no way to tell.
	[[nodiscard]] std::string deferred_reject_script(std::size_t slot, std::string_view reason);

}  // namespace coroute

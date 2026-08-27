#include "coroute/view/deferred_stream.hpp"

namespace coroute
{

	std::string deferred_runtime_script()
	{
		// Written as one string rather than assembled, because it is a fixed asset and
		// the only thing worse than inline JavaScript is inline JavaScript built by
		// string concatenation.
		//
		// coroute.deferred(slot) creates the Promise on first use, whichever side asks
		// first. That matters because a resolve chunk can arrive before the page code
		// that awaits it has run: the browser is parsing and executing as the response
		// streams, so the order is not guaranteed. Creating on demand from both sides
		// makes the race a non-event.
		return R"(<script>
(function () {
  var slots = {};
  function slotFor(id) {
    if (!(id in slots)) {
      var entry = {};
      entry.promise = new Promise(function (resolve, reject) {
        entry.resolve = resolve;
        entry.reject = reject;
      });
      slots[id] = entry;
    }
    return slots[id];
  }
  var api = window.coroute || (window.coroute = {});
  api.deferred = function (id) { return slotFor(id).promise; };
  api.__resolve = function (id, value) {
    slotFor(id).resolve(value);
    var nodes = document.querySelectorAll('[data-coroute-slot="' + id + '"]');
    for (var i = 0; i < nodes.length; i++) {
      nodes[i].textContent = typeof value === 'string' ? value : JSON.stringify(value);
    }
  };
  api.__reject = function (id, reason) {
    slotFor(id).reject(new Error(reason));
    var nodes = document.querySelectorAll('[data-coroute-slot="' + id + '"]');
    for (var i = 0; i < nodes.length; i++) {
      nodes[i].setAttribute('data-coroute-failed', '1');
    }
  };
})();
</script>
)";
	}

	std::string escape_for_script(std::string_view json_text)
	{
		std::string out;
		out.reserve(json_text.size() + 16);

		for (std::size_t i = 0; i < json_text.size(); ++i)
		{
			const unsigned char c = static_cast<unsigned char>(json_text[i]);
			switch (c)
			{
				// The HTML tokeniser looks for these inside a script element before any
				// JavaScript runs, so they cannot be left as themselves. \uXXXX is
				// invisible to JSON.parse, which is why it is the right escape.
				case '<':
					out += "\\u003C";
					break;
				case '>':
					out += "\\u003E";
					break;
				case '&':
					out += "\\u0026";
					break;
				default:
					// U+2028 and U+2029 are line terminators in JavaScript but ordinary
					// characters in JSON, so an unescaped one splits the statement in two.
					// They arrive here as their UTF-8 encoding, E2 80 A8 and E2 80 A9.
					if (c == 0xE2 && i + 2 < json_text.size() &&
					    static_cast<unsigned char>(json_text[i + 1]) == 0x80 &&
					    (static_cast<unsigned char>(json_text[i + 2]) == 0xA8 ||
					     static_cast<unsigned char>(json_text[i + 2]) == 0xA9))
					{
						out += static_cast<unsigned char>(json_text[i + 2]) == 0xA8 ? "\\u2028" : "\\u2029";
						i += 2;
					}
					else
					{
						out += static_cast<char>(c);
					}
					break;
			}
		}
		return out;
	}

	std::string deferred_resolve_script(std::size_t slot, const nlohmann::json& value)
	{
		return "<script>window.coroute.__resolve(" + std::to_string(slot) + "," +
		       escape_for_script(value.dump()) + ");</script>\n";
	}

	std::string deferred_reject_script(std::size_t slot, std::string_view reason)
	{
		// The reason goes through the same JSON encoding as a value, so quotes and
		// backslashes in an exception message cannot break out of the string, and then
		// through the same script escaping so it cannot break out of the element.
		return "<script>window.coroute.__reject(" + std::to_string(slot) + "," +
		       escape_for_script(nlohmann::json(std::string(reason)).dump()) + ");</script>\n";
	}

}  // namespace coroute

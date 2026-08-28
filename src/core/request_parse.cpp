#include "coroute/core/request_parse.hpp"

#include <cstddef>

namespace coroute
{

	namespace
	{
		// -1 for anything that is not a hexadecimal digit.
		int hex_value(char c)
		{
			if (c >= 0x30 && c <= 0x39) return c - 0x30;
			if (c >= 0x61 && c <= 0x66) return c - 0x61 + 10;
			if (c >= 0x41 && c <= 0x46) return c - 0x41 + 10;
			return -1;
		}
	}  // namespace

	std::string url_decode(std::string_view str, bool plus_is_space)
	{
		std::string result;
		result.reserve(str.size());

		for (std::size_t i = 0; i < str.size(); ++i)
		{
			if (str[i] == '%' && i + 2 < str.size())
			{
				// Both octets must be hexadecimal digits and nothing else.
				//
				// This used to hand the pair to strtol and accept it when strtol consumed
				// two characters, which it does for "+1" and for " 1", because it skips
				// leading space and takes a sign. So "%+1" decoded to 0x01. A decoder that
				// accepts sequences the grammar does not produce widens what can encode any
				// given octet, and the octets worth encoding here are the ones in "..".
				const int hi = hex_value(str[i + 1]);
				const int lo = hex_value(str[i + 2]);
				if (hi >= 0 && lo >= 0)
				{
					result += static_cast<char>((hi << 4) | lo);
					i += 2;
					continue;
				}
			}
			else if (plus_is_space && str[i] == '+')
			{
				result += ' ';
				continue;
			}
			result += str[i];
		}

		return result;
	}

	expected<Request, Error> parse_request_head(std::string_view data)
	{
Request req;

	// Find request line
	auto line_end = data.find("\r\n");
	if (line_end == std::string_view::npos)
	{
		return unexpected(Error::http(HttpError::BadRequest, "Invalid request line"));
	}

	std::string_view request_line = data.substr(0, line_end);

	// Parse method
	auto method_end = request_line.find(' ');
	if (method_end == std::string_view::npos)
	{
		return unexpected(Error::http(HttpError::BadRequest, "Invalid request line"));
	}
	req.set_method(request_line.substr(0, method_end));

	// Parse path
	auto path_start = method_end + 1;
	auto path_end = request_line.find(' ', path_start);
	if (path_end == std::string_view::npos)
	{
		return unexpected(Error::http(HttpError::BadRequest, "Invalid request line"));
	}

	std::string_view path_with_query = request_line.substr(path_start, path_end - path_start);

	// Split path and query string
	auto query_start = path_with_query.find('?');
	if (query_start != std::string_view::npos)
	{
		// URL decode the path
		req.set_path(url_decode(path_with_query.substr(0, query_start), false));
		req.set_query_string(std::string(path_with_query.substr(query_start + 1)));

		// Parse query parameters with URL decoding
		std::string_view qs = path_with_query.substr(query_start + 1);
		while (!qs.empty())
		{
			auto amp = qs.find('&');
			std::string_view param = (amp != std::string_view::npos) ? qs.substr(0, amp) : qs;

			auto eq = param.find('=');
			if (eq != std::string_view::npos)
			{
				req.add_query_param(url_decode(param.substr(0, eq), true), url_decode(param.substr(eq + 1), true));
			}
			else if (!param.empty())
			{
				// Parameter without value
				req.add_query_param(url_decode(param, true), "");
			}

			if (amp == std::string_view::npos) break;
			qs = qs.substr(amp + 1);
		}
	}
	else
	{
		req.set_path(url_decode(path_with_query, false));
	}

	// Parse HTTP version
	auto version_start = path_end + 1;
	req.set_http_version(std::string(request_line.substr(version_start)));

	// Parse headers
	size_t pos = line_end + 2;
	while (pos < data.size())
	{
		auto header_end = data.find("\r\n", pos);
		if (header_end == std::string_view::npos || header_end == pos)
		{
			break;  // End of headers
		}

		std::string_view header_line = data.substr(pos, header_end - pos);
		auto colon = header_line.find(':');
		if (colon != std::string_view::npos)
		{
			std::string_view key = header_line.substr(0, colon);
			std::string_view value = header_line.substr(colon + 1);

			// Trim leading whitespace from value
			while (!value.empty() && value[0] == ' ')
			{
				value = value.substr(1);
			}

			// Two Content-Length lines that disagree are the classic desync: this server
			// stores headers in a map, so the later one silently replaced the earlier one
			// and a proxy taking the first would frame the body differently. RFC 9112
			// section 6.3 requires rejecting the message rather than picking one.
			if (Request::fold(std::string(key)) == "content-length")
			{
				auto existing = req.header("content-length");
				if (existing && *existing != value)
				{
					return unexpected(
					    Error::http(HttpError::BadRequest, "Conflicting Content-Length"));
				}
			}

			req.add_header(std::string(key), std::string(value));
		}

		pos = header_end + 2;
	}

	// Transfer-Encoding is not implemented on this path, and saying so is the only
	// safe answer.
	//
	// It used to be ignored entirely: a chunked request has no Content-Length, so no
	// body was read and the chunk data was discarded with the read buffer. Checked
	// against a running server rather than assumed, and the octets are dropped rather
	// than executed, so this was not smuggling by itself. It is still a server that
	// disagrees with the client about how long the request was, which is what a
	// front-end proxy honouring Transfer-Encoding needs in order to desync against
	// it. RFC 9112 section 6.1 says a server that does not understand a transfer
	// coding responds 501.
	//
	// The status reaching the client is 400 rather than 501, because handle_connection
	// answers every parse failure with bad_request and discards the code carried in
	// the error. That is worth fixing separately; the framing is what matters here.
	//
	// ChunkedBodyReader exists and would decode this properly. Wiring it needs the
	// octets already buffered past the headers to be handed to it, and getting that
	// handover subtly wrong reintroduces the same desync, so refusing comes first and
	// implementing follows separately.
	if (req.header("transfer-encoding"))
	{
		return unexpected(
		    Error::http(HttpError::NotImplemented, "Transfer-Encoding is not supported"));
	}

		return req;
	}

}  // namespace coroute

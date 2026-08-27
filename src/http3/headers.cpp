#include "coroute/http3/headers.hpp"

#ifdef COROUTE_HAS_HTTP3

#include <algorithm>
#include <array>

namespace coroute::http3
{

	namespace
	{
		// RFC 9114 section 4.2. These carry HTTP/1.1 connection semantics that HTTP/3
		// expresses in the transport instead, so their presence makes a message
		// malformed rather than merely redundant.
		constexpr std::array<std::string_view, 5> connection_specific{"connection", "keep-alive", "proxy-connection",
		                                                              "transfer-encoding", "upgrade"};

		bool has_uppercase(std::string_view text) noexcept
		{
			return std::any_of(text.begin(), text.end(), [](char c) { return c >= 'A' && c <= 'Z'; });
		}

		bool is_connection_specific(std::string_view name) noexcept
		{
			return std::find(connection_specific.begin(), connection_specific.end(), name) != connection_specific.end();
		}
	}  // namespace

	std::string_view describe(HeaderError error) noexcept
	{
		switch (error)
		{
			case HeaderError::None:
				return "ok";
			case HeaderError::UppercaseFieldName:
				return "field name contains uppercase characters";
			case HeaderError::ConnectionSpecific:
				return "connection-specific field is not allowed in HTTP/3";
			case HeaderError::BadTeValue:
				return "TE field may only carry the value \"trailers\"";
			case HeaderError::UnknownPseudoHeader:
				return "unrecognised pseudo-header";
			case HeaderError::PseudoHeaderAfterField:
				return "pseudo-header appeared after a regular field";
			case HeaderError::DuplicatePseudoHeader:
				return "pseudo-header appeared more than once";
			case HeaderError::MissingPseudoHeader:
				return "a required pseudo-header is missing";
			case HeaderError::EmptyPath:
				return ":path is empty";
		}
		return "unknown";
	}

	bool RequestBuilder::fail(HeaderError error) noexcept
	{
		if (error_ == HeaderError::None)
		{
			error_ = error;
		}
		return false;
	}

	bool RequestBuilder::add(std::string_view name, std::string_view value)
	{
		if (error_ != HeaderError::None)
		{
			return false;  // already malformed; keep the first reason
		}

		// Not a normalisation step. RFC 9114 section 4.2 says a field name containing
		// uppercase makes the request malformed, precisely so that two intermediaries
		// cannot disagree about whether "Content-Length" and "content-length" are the
		// same field.
		if (has_uppercase(name))
		{
			return fail(HeaderError::UppercaseFieldName);
		}

		if (!name.empty() && name.front() == ':')
		{
			if (seen_regular_field_)
			{
				return fail(HeaderError::PseudoHeaderAfterField);
			}

			auto assign_once = [&](std::string& slot) -> bool
			{
				if (!slot.empty())
				{
					return fail(HeaderError::DuplicatePseudoHeader);
				}
				slot.assign(value);
				return true;
			};

			if (name == ":method") return assign_once(method_);
			if (name == ":scheme") return assign_once(scheme_);
			if (name == ":authority") return assign_once(authority_);
			if (name == ":path") return assign_once(path_);

			// :status is a response pseudo-header; anything else is simply unknown.
			// Either way a request must not carry it.
			return fail(HeaderError::UnknownPseudoHeader);
		}

		seen_regular_field_ = true;

		if (is_connection_specific(name))
		{
			return fail(HeaderError::ConnectionSpecific);
		}

		// TE survives, but only as "trailers". Any other value implies HTTP/1.1
		// transfer semantics that do not exist here.
		if (name == "te" && value != "trailers")
		{
			return fail(HeaderError::BadTeValue);
		}

		request_.add_header(std::string(name), std::string(value));
		return true;
	}

	std::optional<Request> RequestBuilder::build()
	{
		if (error_ != HeaderError::None)
		{
			return std::nullopt;
		}

		// RFC 9114 section 4.3.1: :method, :scheme and :path are all required for a
		// normal request. :authority is not, which is why it is absent from this check.
		if (method_.empty() || scheme_.empty() || path_.empty())
		{
			error_ = path_.empty() && !method_.empty() && !scheme_.empty() ? HeaderError::EmptyPath
			                                                               : HeaderError::MissingPseudoHeader;
			return std::nullopt;
		}

		request_.set_method(method_);
		request_.set_path(path_);

		// HTTP/3 has no Host field; :authority carries the same information. Handlers
		// and middleware shared with HTTP/1.1 look for Host, so it is synthesised here
		// rather than making every caller know which protocol it is serving.
		if (!authority_.empty() && request_.headers().find("host") == request_.headers().end())
		{
			request_.add_header("host", authority_);
		}

		request_.set_http_version("HTTP/3");

		return std::move(request_);
	}

	std::vector<HeaderField> response_fields(const Response& response)
	{
		std::vector<HeaderField> fields;
		fields.reserve(response.headers().size() + 1);

		// :status first: RFC 9114 requires pseudo-headers to precede regular fields.
		fields.push_back({":status", std::to_string(response.status())});

		for (const auto& [name, value] : response.headers())
		{
			std::string lowered(name);
			std::transform(lowered.begin(), lowered.end(), lowered.begin(),
			               [](unsigned char c) { return static_cast<char>(std::tolower(c)); });

			// Dropped, not forwarded. The same Response is produced by handlers shared
			// with HTTP/1.1, where these fields are ordinary; emitting one here would
			// be a protocol violation the peer may treat as fatal.
			if (is_connection_specific(lowered))
			{
				continue;
			}

			fields.push_back({std::move(lowered), value});
		}

		return fields;
	}

}  // namespace coroute::http3

#endif  // COROUTE_HAS_HTTP3

#include <catch2/catch_test_macros.hpp>

#ifdef COROUTE_HAS_TLS

#include <coroute/net/tls.hpp>

#include <openssl/ssl.h>

using namespace coroute::net;

TEST_CASE("the QUIC context differs from the TCP one where it must", "[tls]")
{
	// No certificate: create() skips loading when the paths are empty, which is
	// enough to exercise the protocol configuration without a fixture on disk.
	TlsConfig config;

	SECTION("QUIC forces TLS 1.3 even when the config asks for 1.2")
	{
		// RFC 9001 section 4.2: QUIC uses TLS 1.3 and nothing earlier. A config
		// written for the TCP listener will usually still allow 1.2, and inheriting
		// that would surface much later as an unexplained handshake rejection.
		config.min_version = TlsConfig::MinVersion::TLS_1_2;

		auto tcp = TlsContext::create(config);
		REQUIRE(tcp.has_value());
		REQUIRE(SSL_CTX_get_min_proto_version(tcp->native_handle()) == TLS1_2_VERSION);

		auto quic = TlsContext::create_quic(config);
		REQUIRE(quic.has_value());
		REQUIRE(SSL_CTX_get_min_proto_version(quic->native_handle()) == TLS1_3_VERSION);
	}

	SECTION("asking for TLS 1.3 on the TCP context still works")
	{
		// Guards against create_quic being the only path that can reach 1.3.
		config.min_version = TlsConfig::MinVersion::TLS_1_3;
		auto tcp = TlsContext::create(config);
		REQUIRE(tcp.has_value());
		REQUIRE(SSL_CTX_get_min_proto_version(tcp->native_handle()) == TLS1_3_VERSION);
	}

	SECTION("a QUIC context is usable after being moved")
	{
		// OpenSSL holds raw pointers to the ALPN list and the SNI argument inside the
		// SSL_CTX. create() returns by value, so every context reaching a caller has
		// already been moved at least once; if that invalidated the registered
		// pointers the handshake would read freed memory.
		auto quic = TlsContext::create_quic(config);
		REQUIRE(quic.has_value());

		TlsContext moved = std::move(*quic);
		REQUIRE(moved.native_handle() != nullptr);
		REQUIRE(SSL_CTX_get_min_proto_version(moved.native_handle()) == TLS1_3_VERSION);

		// And again through assignment, which takes a different code path.
		auto another = TlsContext::create_quic(config);
		REQUIRE(another.has_value());
		*another = std::move(moved);
		REQUIRE(another->native_handle() != nullptr);
	}
}

#endif  // COROUTE_HAS_TLS

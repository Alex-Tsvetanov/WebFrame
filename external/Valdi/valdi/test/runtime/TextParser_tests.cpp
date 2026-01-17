//
//  TextParser_tests.cpp
//  valdi-pc
//
//  Created by Simon Corsin on 20/01/23
//

#include "valdi_core/cpp/Utils/StringCache.hpp"
#include "valdi_core/cpp/Utils/TextParser.hpp"
#include <cctype>
#include <gtest/gtest.h>

using namespace Valdi;

namespace ValdiTest {

TEST(TextParser, canParseCharacter) {
    TextParser parser("He");

    ASSERT_TRUE(parser.parse('H'));
    ASSERT_EQ(static_cast<size_t>(1), parser.position());
    ASSERT_FALSE(parser.hasError());
    ASSERT_FALSE(parser.isAtEnd());

    ASSERT_FALSE(parser.tryParse('H'));
    ASSERT_EQ(static_cast<size_t>(1), parser.position());
    ASSERT_FALSE(parser.hasError());
    ASSERT_FALSE(parser.isAtEnd());

    ASSERT_TRUE(parser.tryParse('e'));
    ASSERT_EQ(static_cast<size_t>(2), parser.position());
    ASSERT_FALSE(parser.hasError());
    ASSERT_TRUE(parser.isAtEnd());

    ASSERT_FALSE(parser.tryParse('e'));
    ASSERT_EQ(static_cast<size_t>(2), parser.position());
    ASSERT_FALSE(parser.hasError());
    ASSERT_TRUE(parser.isAtEnd());

    ASSERT_FALSE(parser.parse('e'));
    ASSERT_EQ(static_cast<size_t>(2), parser.position());
    ASSERT_TRUE(parser.hasError());
    ASSERT_TRUE(parser.isAtEnd());
}

TEST(TextParser, canParseTerm) {
    TextParser parser("Hello World");

    ASSERT_TRUE(parser.parse("Hello"));
    ASSERT_EQ(static_cast<size_t>(5), parser.position());
    ASSERT_FALSE(parser.hasError());
    ASSERT_FALSE(parser.isAtEnd());

    ASSERT_FALSE(parser.tryParse("World"));
    ASSERT_EQ(static_cast<size_t>(5), parser.position());
    ASSERT_FALSE(parser.hasError());
    ASSERT_FALSE(parser.isAtEnd());

    ASSERT_FALSE(parser.tryParse(" World."));
    ASSERT_EQ(static_cast<size_t>(5), parser.position());
    ASSERT_FALSE(parser.hasError());
    ASSERT_FALSE(parser.isAtEnd());

    ASSERT_TRUE(parser.tryParse(" "));
    ASSERT_EQ(static_cast<size_t>(6), parser.position());
    ASSERT_FALSE(parser.hasError());
    ASSERT_FALSE(parser.isAtEnd());

    ASSERT_TRUE(parser.tryParse("Wor"));
    ASSERT_EQ(static_cast<size_t>(9), parser.position());
    ASSERT_FALSE(parser.hasError());
    ASSERT_FALSE(parser.isAtEnd());

    ASSERT_FALSE(parser.parse("ld!"));
    ASSERT_EQ(static_cast<size_t>(9), parser.position());
    ASSERT_TRUE(parser.hasError());
    ASSERT_FALSE(parser.isAtEnd());
}

TEST(TextParser, canParseWhitespaces) {
    TextParser parser("  Hello  \n World  ");

    ASSERT_TRUE(parser.tryParseWhitespaces());
    ASSERT_EQ(static_cast<size_t>(2), parser.position());
    ASSERT_FALSE(parser.hasError());
    ASSERT_FALSE(parser.isAtEnd());

    ASSERT_FALSE(parser.tryParseWhitespaces());
    ASSERT_EQ(static_cast<size_t>(2), parser.position());
    ASSERT_FALSE(parser.hasError());
    ASSERT_FALSE(parser.isAtEnd());

    ASSERT_TRUE(parser.skipCount(5));
    ASSERT_EQ(static_cast<size_t>(7), parser.position());
    ASSERT_FALSE(parser.hasError());
    ASSERT_FALSE(parser.isAtEnd());

    ASSERT_TRUE(parser.tryParseWhitespaces());
    ASSERT_EQ(static_cast<size_t>(11), parser.position());
    ASSERT_FALSE(parser.hasError());
    ASSERT_FALSE(parser.isAtEnd());

    ASSERT_FALSE(parser.tryParseWhitespaces());
    ASSERT_EQ(static_cast<size_t>(11), parser.position());
    ASSERT_FALSE(parser.hasError());
    ASSERT_FALSE(parser.isAtEnd());

    ASSERT_TRUE(parser.skipCount(5));
    ASSERT_EQ(static_cast<size_t>(16), parser.position());
    ASSERT_FALSE(parser.hasError());
    ASSERT_FALSE(parser.isAtEnd());

    ASSERT_TRUE(parser.tryParseWhitespaces());
    ASSERT_EQ(static_cast<size_t>(18), parser.position());
    ASSERT_FALSE(parser.hasError());
    ASSERT_TRUE(parser.isAtEnd());

    ASSERT_FALSE(parser.tryParseWhitespaces());
    ASSERT_EQ(static_cast<size_t>(18), parser.position());
    ASSERT_FALSE(parser.hasError());
    ASSERT_TRUE(parser.isAtEnd());
}

TEST(TextParser, canParseDouble) {
    TextParser parser("42.95 2 9999nope");

    auto d = parser.parseDouble();

    ASSERT_TRUE(d);
    ASSERT_EQ(42.95, d.value());
    ASSERT_EQ(static_cast<size_t>(5), parser.position());
    ASSERT_FALSE(parser.hasError());
    ASSERT_FALSE(parser.isAtEnd());

    ASSERT_TRUE(parser.skip());
    ASSERT_EQ(static_cast<size_t>(6), parser.position());
    ASSERT_FALSE(parser.hasError());
    ASSERT_FALSE(parser.isAtEnd());

    d = parser.parseDouble();

    ASSERT_TRUE(d);
    ASSERT_EQ(2.0, d.value());
    ASSERT_EQ(static_cast<size_t>(7), parser.position());
    ASSERT_FALSE(parser.hasError());
    ASSERT_FALSE(parser.isAtEnd());

    ASSERT_TRUE(parser.skip());
    ASSERT_EQ(static_cast<size_t>(8), parser.position());
    ASSERT_FALSE(parser.hasError());
    ASSERT_FALSE(parser.isAtEnd());

    d = parser.parseDouble();

    ASSERT_TRUE(d);
    ASSERT_EQ(9999.0, d.value());
    ASSERT_EQ(static_cast<size_t>(12), parser.position());
    ASSERT_FALSE(parser.hasError());
    ASSERT_FALSE(parser.isAtEnd());

    d = parser.parseDouble();

    ASSERT_FALSE(d);
    ASSERT_EQ(static_cast<size_t>(12), parser.position());
    ASSERT_TRUE(parser.hasError());
    ASSERT_FALSE(parser.isAtEnd());
}

TEST(TextParser, parseDoubleAutomaticallySkipsWhitespaces) {
    TextParser parser("   1    42.5    3");

    auto d = parser.parseDouble();

    ASSERT_TRUE(d);
    ASSERT_EQ(1.0, d.value());
    ASSERT_EQ(static_cast<size_t>(4), parser.position());
    ASSERT_FALSE(parser.hasError());
    ASSERT_FALSE(parser.isAtEnd());

    d = parser.parseDouble();

    ASSERT_TRUE(d);
    ASSERT_EQ(42.5, d.value());
    ASSERT_EQ(static_cast<size_t>(12), parser.position());
    ASSERT_FALSE(parser.hasError());
    ASSERT_FALSE(parser.isAtEnd());

    d = parser.parseDouble();

    ASSERT_TRUE(d);
    ASSERT_EQ(3.0, d.value());
    ASSERT_EQ(static_cast<size_t>(17), parser.position());
    ASSERT_FALSE(parser.hasError());
    ASSERT_TRUE(parser.isAtEnd());
}

TEST(TextParser, canParseInt) {
    TextParser parser("42 2 9999nope");

    auto i = parser.parseInt();

    ASSERT_TRUE(i);
    ASSERT_EQ(42, i.value());
    ASSERT_EQ(static_cast<size_t>(2), parser.position());
    ASSERT_FALSE(parser.hasError());
    ASSERT_FALSE(parser.isAtEnd());

    ASSERT_TRUE(parser.skip());
    ASSERT_EQ(static_cast<size_t>(3), parser.position());
    ASSERT_FALSE(parser.hasError());
    ASSERT_FALSE(parser.isAtEnd());

    i = parser.parseInt();

    ASSERT_TRUE(i);
    ASSERT_EQ(2, i.value());
    ASSERT_EQ(static_cast<size_t>(4), parser.position());
    ASSERT_FALSE(parser.hasError());
    ASSERT_FALSE(parser.isAtEnd());

    ASSERT_TRUE(parser.skip());
    ASSERT_EQ(static_cast<size_t>(5), parser.position());
    ASSERT_FALSE(parser.hasError());
    ASSERT_FALSE(parser.isAtEnd());

    i = parser.parseInt();

    ASSERT_TRUE(i);
    ASSERT_EQ(9999, i.value());
    ASSERT_EQ(static_cast<size_t>(9), parser.position());
    ASSERT_FALSE(parser.hasError());
    ASSERT_FALSE(parser.isAtEnd());

    i = parser.parseInt();

    ASSERT_FALSE(i);
    ASSERT_EQ(static_cast<size_t>(9), parser.position());
    ASSERT_TRUE(parser.hasError());
    ASSERT_FALSE(parser.isAtEnd());
}

TEST(TextParser, parseIntAutomaticallySkipsWhitespaces) {
    TextParser parser("        42      2 9999    ");

    auto i = parser.parseInt();

    ASSERT_TRUE(i);
    ASSERT_EQ(42, i.value());
    ASSERT_EQ(static_cast<size_t>(10), parser.position());
    ASSERT_FALSE(parser.hasError());
    ASSERT_FALSE(parser.isAtEnd());

    i = parser.parseInt();

    ASSERT_TRUE(i);
    ASSERT_EQ(2, i.value());
    ASSERT_EQ(static_cast<size_t>(17), parser.position());
    ASSERT_FALSE(parser.hasError());
    ASSERT_FALSE(parser.isAtEnd());

    i = parser.parseInt();

    ASSERT_TRUE(i);
    ASSERT_EQ(9999, i.value());
    ASSERT_EQ(static_cast<size_t>(22), parser.position());
    ASSERT_FALSE(parser.hasError());
    ASSERT_FALSE(parser.isAtEnd());
}

TEST(TextParser, canParseHex) {
    TextParser parser("a cf5 10#aa");

    auto l = parser.parseHexLong();

    ASSERT_TRUE(l);
    ASSERT_EQ(10, l.value());
    ASSERT_EQ(static_cast<size_t>(1), parser.position());
    ASSERT_FALSE(parser.hasError());
    ASSERT_FALSE(parser.isAtEnd());

    ASSERT_TRUE(parser.skip());
    ASSERT_EQ(static_cast<size_t>(2), parser.position());
    ASSERT_FALSE(parser.hasError());
    ASSERT_FALSE(parser.isAtEnd());

    l = parser.parseHexLong();

    ASSERT_TRUE(l);
    ASSERT_EQ(3317, l.value());
    ASSERT_EQ(static_cast<size_t>(5), parser.position());
    ASSERT_FALSE(parser.hasError());
    ASSERT_FALSE(parser.isAtEnd());

    ASSERT_TRUE(parser.skip());
    ASSERT_EQ(static_cast<size_t>(6), parser.position());
    ASSERT_FALSE(parser.hasError());
    ASSERT_FALSE(parser.isAtEnd());

    l = parser.parseHexLong();

    ASSERT_TRUE(l);
    ASSERT_EQ(16, l.value());
    ASSERT_EQ(static_cast<size_t>(8), parser.position());
    ASSERT_FALSE(parser.hasError());
    ASSERT_FALSE(parser.isAtEnd());

    l = parser.parseHexLong();

    ASSERT_FALSE(l);
    ASSERT_EQ(static_cast<size_t>(8), parser.position());
    ASSERT_TRUE(parser.hasError());
    ASSERT_FALSE(parser.isAtEnd());
}

TEST(TextParser, parseHexLongAutomaticallySkipsWhitespaces) {
    TextParser parser("        a      cf5 10    ");

    auto l = parser.parseHexLong();

    ASSERT_TRUE(l);
    ASSERT_EQ(10, l.value());
    ASSERT_EQ(static_cast<size_t>(9), parser.position());
    ASSERT_FALSE(parser.hasError());
    ASSERT_FALSE(parser.isAtEnd());

    l = parser.parseHexLong();

    ASSERT_TRUE(l);
    ASSERT_EQ(3317, l.value());
    ASSERT_EQ(static_cast<size_t>(18), parser.position());
    ASSERT_FALSE(parser.hasError());
    ASSERT_FALSE(parser.isAtEnd());

    l = parser.parseHexLong();

    ASSERT_TRUE(l);
    ASSERT_EQ(16, l.value());
    ASSERT_EQ(static_cast<size_t>(21), parser.position());
    ASSERT_FALSE(parser.hasError());
    ASSERT_FALSE(parser.isAtEnd());
}

TEST(TextParser, canParseIdentifier) {
    TextParser parser("42 hello what-_is-up  ");

    auto i = parser.parseIdentifier();

    ASSERT_TRUE(i);
    ASSERT_EQ("42", i.value());
    ASSERT_EQ(static_cast<size_t>(2), parser.position());
    ASSERT_FALSE(parser.hasError());
    ASSERT_FALSE(parser.isAtEnd());

    ASSERT_TRUE(parser.skip());
    ASSERT_EQ(static_cast<size_t>(3), parser.position());
    ASSERT_FALSE(parser.hasError());
    ASSERT_FALSE(parser.isAtEnd());

    i = parser.parseIdentifier();

    ASSERT_TRUE(i);
    ASSERT_EQ("hello", i.value());
    ASSERT_EQ(static_cast<size_t>(8), parser.position());
    ASSERT_FALSE(parser.hasError());
    ASSERT_FALSE(parser.isAtEnd());

    ASSERT_TRUE(parser.skip());
    ASSERT_EQ(static_cast<size_t>(9), parser.position());
    ASSERT_FALSE(parser.hasError());
    ASSERT_FALSE(parser.isAtEnd());

    i = parser.parseIdentifier();

    ASSERT_TRUE(i);
    ASSERT_EQ("what-_is-up", i.value());
    ASSERT_EQ(static_cast<size_t>(20), parser.position());
    ASSERT_FALSE(parser.hasError());
    ASSERT_FALSE(parser.isAtEnd());

    i = parser.parseIdentifier();

    ASSERT_FALSE(i);
    ASSERT_EQ(static_cast<size_t>(20), parser.position());
    ASSERT_TRUE(parser.hasError());
    ASSERT_FALSE(parser.isAtEnd());
}

TEST(TextParser, parseIdentifierAutomaticallySkipsWhitespaces) {
    TextParser parser("      42      hello       what-_is-up  ");

    auto i = parser.parseIdentifier();

    ASSERT_TRUE(i);
    ASSERT_EQ("42", i.value());
    ASSERT_EQ(static_cast<size_t>(8), parser.position());
    ASSERT_FALSE(parser.hasError());
    ASSERT_FALSE(parser.isAtEnd());

    i = parser.parseIdentifier();

    ASSERT_TRUE(i);
    ASSERT_EQ("hello", i.value());
    ASSERT_EQ(static_cast<size_t>(19), parser.position());
    ASSERT_FALSE(parser.hasError());
    ASSERT_FALSE(parser.isAtEnd());

    i = parser.parseIdentifier();

    ASSERT_TRUE(i);
    ASSERT_EQ("what-_is-up", i.value());
    ASSERT_EQ(static_cast<size_t>(37), parser.position());
    ASSERT_FALSE(parser.hasError());
    ASSERT_FALSE(parser.isAtEnd());
}

TEST(TextParser, canReadWhile) {
    TextParser parser("HELloWOrld");

    auto i = parser.readWhile([](char c) { return std::isupper(c); });

    ASSERT_EQ("HEL", i);
    ASSERT_EQ(static_cast<size_t>(3), parser.position());
    ASSERT_FALSE(parser.hasError());
    ASSERT_FALSE(parser.isAtEnd());

    i = parser.readWhile([](char c) { return std::isupper(c); });

    ASSERT_EQ("", i);
    ASSERT_EQ(static_cast<size_t>(3), parser.position());
    ASSERT_FALSE(parser.hasError());
    ASSERT_FALSE(parser.isAtEnd());

    i = parser.readWhile([](char c) { return std::islower(c); });

    ASSERT_EQ("lo", i);
    ASSERT_EQ(static_cast<size_t>(5), parser.position());
    ASSERT_FALSE(parser.hasError());
    ASSERT_FALSE(parser.isAtEnd());

    i = parser.readWhile([](char c) { return std::isupper(c); });

    ASSERT_EQ("WO", i);
    ASSERT_EQ(static_cast<size_t>(7), parser.position());
    ASSERT_FALSE(parser.hasError());
    ASSERT_FALSE(parser.isAtEnd());

    i = parser.readWhile([](char c) { return std::islower(c); });

    ASSERT_EQ("rld", i);
    ASSERT_EQ(static_cast<size_t>(10), parser.position());
    ASSERT_FALSE(parser.hasError());
    ASSERT_TRUE(parser.isAtEnd());

    i = parser.readWhile([](char c) { return std::islower(c); });

    ASSERT_EQ("", i);
    ASSERT_EQ(static_cast<size_t>(10), parser.position());
    ASSERT_FALSE(parser.hasError());
    ASSERT_TRUE(parser.isAtEnd());
}

TEST(TextParser, canReadUntilCharacter) {
    TextParser parser("hello|world|goodbye");

    auto i = parser.readUntilCharacterAndParse('|');

    ASSERT_TRUE(i);
    ASSERT_EQ("hello", i.value());
    ASSERT_EQ(static_cast<size_t>(6), parser.position());
    ASSERT_FALSE(parser.hasError());
    ASSERT_FALSE(parser.isAtEnd());

    i = parser.readUntilCharacterAndParse('|');

    ASSERT_TRUE(i);
    ASSERT_EQ("world", i.value());
    ASSERT_EQ(static_cast<size_t>(12), parser.position());
    ASSERT_FALSE(parser.hasError());
    ASSERT_FALSE(parser.isAtEnd());

    i = parser.readUntilCharacterAndParse('|');

    ASSERT_FALSE(i);
    ASSERT_EQ(static_cast<size_t>(19), parser.position());
    ASSERT_TRUE(parser.hasError());
    ASSERT_TRUE(parser.isAtEnd());
}

TEST(TextParser, providesHumanReadableError) {
    TextParser parser("hello world everyone!");

    ASSERT_TRUE(parser.skip());
    auto d = parser.parseDouble();

    ASSERT_FALSE(d);
    ASSERT_TRUE(parser.hasError());
    ASSERT_EQ("At position 1 (ello world every…): Expecting double", parser.getError().toString());

    parser = TextParser("hello world everyone!");

    ASSERT_TRUE(parser.skipCount(10));
    auto i = parser.parseInt();

    ASSERT_FALSE(i);
    ASSERT_TRUE(parser.hasError());
    ASSERT_EQ("At position 10 (d everyone!): Expecting int", parser.getError().toString());

    parser = TextParser("hello");

    ASSERT_TRUE(parser.skipCount(5));

    ASSERT_FALSE(parser.parseIdentifier());
    ASSERT_TRUE(parser.hasError());
    ASSERT_EQ("At position 5 (<EOF>): Expecting identifier", parser.getError().toString());

    parser = TextParser("|[]|");

    auto identifier = parser.parseIdentifier();

    ASSERT_FALSE(identifier);
    ASSERT_TRUE(parser.hasError());
    ASSERT_EQ("At position 0 (|[]|): Expecting identifier", parser.getError().toString());
}

} // namespace ValdiTest

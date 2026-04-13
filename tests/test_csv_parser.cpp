#include "ncei/csv_parser.hpp"

#include <gtest/gtest.h>

namespace ncei {
namespace {

TEST(CsvParserTest, SimpleThreeFields) {
	DelimitedParser parser;
	std::vector<std::string> fields = parser.parse_line("a,b,c");
	ASSERT_EQ(fields.size(), 3u);
	EXPECT_EQ(fields[0], "a");
	EXPECT_EQ(fields[1], "b");
	EXPECT_EQ(fields[2], "c");
}

TEST(CsvParserTest, QuotedFieldWithComma) {
	DelimitedParser parser;
	std::vector<std::string> fields = parser.parse_line("a,\"b,c\",d");
	ASSERT_EQ(fields.size(), 3u);
	EXPECT_EQ(fields[0], "a");
	EXPECT_EQ(fields[1], "b,c");
	EXPECT_EQ(fields[2], "d");
}

TEST(CsvParserTest, QuotedFieldWithEscapedQuotes) {
	DelimitedParser parser;
	std::vector<std::string> fields = parser.parse_line("a,\"he said \"\"hello\"\"\",c");
	ASSERT_EQ(fields.size(), 3u);
	EXPECT_EQ(fields[1], "he said \"hello\"");
}

TEST(CsvParserTest, EmptyFields) {
	DelimitedParser parser;
	std::vector<std::string> fields = parser.parse_line("a,,c");
	ASSERT_EQ(fields.size(), 3u);
	EXPECT_EQ(fields[0], "a");
	EXPECT_EQ(fields[1], "");
	EXPECT_EQ(fields[2], "c");
}

TEST(CsvParserTest, HeaderParsing) {
	DelimitedParser parser;
	std::vector<std::string> header = parser.parse_header("name,age,city\nAlice,30,NYC");
	ASSERT_EQ(header.size(), 3u);
	EXPECT_EQ(header[0], "name");
	EXPECT_EQ(header[1], "age");
	EXPECT_EQ(header[2], "city");
}

TEST(CsvParserTest, FullCsvMultipleLines) {
	DelimitedParser parser;
	std::string_view text = "name,age\nAlice,30\nBob,25";
	std::vector<std::vector<std::string>> rows = parser.parse(text);
	ASSERT_EQ(rows.size(), 3u);
	EXPECT_EQ(rows[0][0], "name");
	EXPECT_EQ(rows[1][0], "Alice");
	EXPECT_EQ(rows[2][1], "25");
}

TEST(CsvParserTest, SpaceDelimiter) {
	DelimitedParser parser(DelimitedParser::Delimiter::Space);
	std::vector<std::string> fields = parser.parse_line("a b c");
	ASSERT_EQ(fields.size(), 3u);
	EXPECT_EQ(fields[0], "a");
	EXPECT_EQ(fields[1], "b");
	EXPECT_EQ(fields[2], "c");
}

TEST(CsvParserTest, SingleColumn) {
	DelimitedParser parser;
	std::vector<std::string> fields = parser.parse_line("onlyfield");
	ASSERT_EQ(fields.size(), 1u);
	EXPECT_EQ(fields[0], "onlyfield");
}

TEST(CsvParserTest, TrailingDelimiter) {
	DelimitedParser parser;
	std::vector<std::string> fields = parser.parse_line("a,b,");
	ASSERT_EQ(fields.size(), 3u);
	EXPECT_EQ(fields[2], "");
}

TEST(CsvParserTest, EmptyInput) {
	DelimitedParser parser;
	std::vector<std::vector<std::string>> rows = parser.parse("");
	EXPECT_TRUE(rows.empty());
}

TEST(CsvParserTest, WindowsLineEndings) {
	DelimitedParser parser;
	std::vector<std::vector<std::string>> rows = parser.parse("a,b\r\nc,d\r\n");
	ASSERT_EQ(rows.size(), 2u);
	EXPECT_EQ(rows[0][0], "a");
	EXPECT_EQ(rows[0][1], "b");
	EXPECT_EQ(rows[1][0], "c");
	EXPECT_EQ(rows[1][1], "d");
}

TEST(CsvParserTest, MixedQuotedAndUnquoted) {
	DelimitedParser parser;
	std::vector<std::string> fields = parser.parse_line("plain,\"quoted\",plain2");
	ASSERT_EQ(fields.size(), 3u);
	EXPECT_EQ(fields[0], "plain");
	EXPECT_EQ(fields[1], "quoted");
	EXPECT_EQ(fields[2], "plain2");
}

} // namespace
} // namespace ncei

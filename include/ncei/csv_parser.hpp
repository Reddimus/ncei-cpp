#pragma once
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace ncei {

class DelimitedParser {
public:
	enum class Delimiter : char { Comma = ',', Space = ' ' };

	explicit DelimitedParser(Delimiter delim = Delimiter::Comma);

	[[nodiscard]] std::vector<std::vector<std::string>> parse(std::string_view text) const;
	[[nodiscard]] std::vector<std::string> parse_header(std::string_view text) const;
	[[nodiscard]] std::vector<std::string> parse_line(std::string_view line) const;

private:
	Delimiter delimiter_;
};

} // namespace ncei

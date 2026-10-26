#include "ncei/csv_parser.hpp"

#include <sstream>

namespace ncei {
namespace {
std::string_view next_record(std::string_view text, std::size_t& start) {
	const std::size_t begin = start;
	bool quoted = false;
	while (start < text.size()) {
		const char value = text[start++];
		if (value == '"') {
			if (quoted && start < text.size() && text[start] == '"') {
				++start;
			} else {
				quoted = !quoted;
			}
		} else if (value == '\n' && !quoted) {
			break;
		}
	}
	std::string_view record = text.substr(begin, start - begin);
	if (record.ends_with('\n')) {
		record.remove_suffix(1);
	}
	if (record.ends_with('\r')) {
		record.remove_suffix(1);
	}
	return record;
}
} // namespace

DelimitedParser::DelimitedParser(Delimiter delim) : delimiter_(delim) {}

std::vector<std::string> DelimitedParser::parse_line(std::string_view line) const {
	std::vector<std::string> fields;
	char delim = static_cast<char>(delimiter_);
	std::size_t i = 0;
	std::size_t len = line.size();

	while (i <= len) {
		if (i < len && line[i] == '"') {
			// Quoted field
			++i; // skip opening quote
			std::string field;
			while (i < len) {
				if (line[i] == '"') {
					if (i + 1 < len && line[i + 1] == '"') {
						// Escaped quote
						field += '"';
						i += 2;
					} else {
						// Closing quote
						++i;
						break;
					}
				} else {
					field += line[i];
					++i;
				}
			}
			fields.push_back(std::move(field));
			// Skip delimiter after closing quote
			if (i < len && line[i] == delim) {
				++i;
			} else if (i == len) {
				break;
			}
		} else {
			// Unquoted field
			std::size_t start = i;
			while (i < len && line[i] != delim) {
				++i;
			}
			fields.emplace_back(line.substr(start, i - start));
			if (i < len) {
				++i; // skip delimiter
			} else {
				break;
			}
		}
	}

	return fields;
}

std::vector<std::string> DelimitedParser::parse_header(std::string_view text) const {
	std::size_t start = 0;
	return parse_line(next_record(text, start));
}

std::vector<std::vector<std::string>> DelimitedParser::parse(std::string_view text) const {
	std::vector<std::vector<std::string>> rows;
	std::size_t start = 0;
	while (start < text.size()) {
		const std::string_view record = next_record(text, start);
		if (!record.empty()) {
			rows.push_back(parse_line(record));
		}
	}
	return rows;
}

} // namespace ncei

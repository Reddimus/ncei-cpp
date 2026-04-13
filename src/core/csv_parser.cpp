#include "ncei/csv_parser.hpp"

#include <sstream>

namespace ncei {

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
	// Find first line
	std::size_t pos = text.find('\n');
	std::string_view first_line = (pos != std::string_view::npos) ? text.substr(0, pos) : text;
	// Strip trailing \r
	if (!first_line.empty() && first_line.back() == '\r') {
		first_line = first_line.substr(0, first_line.size() - 1);
	}
	return parse_line(first_line);
}

std::vector<std::vector<std::string>> DelimitedParser::parse(std::string_view text) const {
	std::vector<std::vector<std::string>> rows;
	if (text.empty()) {
		return rows;
	}

	std::size_t start = 0;
	std::size_t len = text.size();

	while (start < len) {
		std::size_t pos = text.find('\n', start);
		std::string_view line;
		if (pos != std::string_view::npos) {
			line = text.substr(start, pos - start);
			start = pos + 1;
		} else {
			line = text.substr(start);
			start = len;
		}
		// Strip trailing \r
		if (!line.empty() && line.back() == '\r') {
			line = line.substr(0, line.size() - 1);
		}
		// Skip empty lines
		if (line.empty()) {
			continue;
		}
		rows.push_back(parse_line(line));
	}

	return rows;
}

} // namespace ncei

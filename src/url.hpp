#pragma once
#include <string>
#include <string_view>
namespace ncei::detail {
inline std::string encode(std::string_view value) {
	constexpr char hex[] = "0123456789ABCDEF";
	std::string out;
	for (unsigned char c : value) {
		if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') ||
			c == '-' || c == '_' || c == '.' || c == '~') {
			out += static_cast<char>(c);
		} else {
			out += '%';
			out += hex[c >> 4];
			out += hex[c & 15];
		}
	}
	return out;
}

} // namespace ncei::detail

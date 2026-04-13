#pragma once
#include <cstdint>
#include <string_view>

namespace ncei {

enum class ResponseFormat : std::uint8_t {
	CSV,
	JSON,
	SSV,
	PDF,
	NetCDF,
};

[[nodiscard]] constexpr std::string_view to_string(ResponseFormat fmt) noexcept {
	switch (fmt) {
		case ResponseFormat::CSV:
			return "csv";
		case ResponseFormat::JSON:
			return "json";
		case ResponseFormat::SSV:
			return "ssv";
		case ResponseFormat::PDF:
			return "pdf";
		case ResponseFormat::NetCDF:
			return "netcdf";
	}
	return "csv";
}

[[nodiscard]] constexpr std::string_view content_type(ResponseFormat fmt) noexcept {
	switch (fmt) {
		case ResponseFormat::CSV:
			return "text/csv";
		case ResponseFormat::JSON:
			return "application/json";
		case ResponseFormat::SSV:
			return "text/plain";
		case ResponseFormat::PDF:
			return "application/pdf";
		case ResponseFormat::NetCDF:
			return "application/x-netcdf";
	}
	return "text/csv";
}

} // namespace ncei

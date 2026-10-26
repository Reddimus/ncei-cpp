#pragma once
#include <glaze/glaze.hpp>
namespace ncei::detail {
// Null scalar fields retain defaults; every bounded view must contain complete JSON.
struct ncei_opts : glz::opts {
	bool skip_null_members_on_read = true;
	bool validate_trailing_whitespace = true;
	bool validate_skipped = true;
};
inline constexpr ncei_opts kReadOpts{{.null_terminated = false, .error_on_unknown_keys = false}};

} // namespace ncei::detail

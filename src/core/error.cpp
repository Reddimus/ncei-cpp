// Copyright (c) 2026 PredictionMarketsAI
// SPDX-License-Identifier: MIT

#include "ncei/error.hpp"

#include <glaze/glaze.hpp>
#include <glaze/json/generic.hpp>
#include <string>
#include <utility>

namespace ncei {

namespace {

// Multi-format NCEI error body parser. The HTTP error envelope shape
// differs between the CDO API (`{status, message}`), the Data Service
// API (`{errorMessage, errors:[...]}`), and a generic RFC 7807
// Problem Details fallback (`{title, detail, correlationId}`). Glaze
// can't pick at compile time, so we walk the AST once and dispatch.
//
// On any read failure, the parsed body falls back to a truncated
// raw-string error message (matches the pre-migration behavior of the
// nlohmann try/catch).

bool extract_string_into(const glz::generic::object_t& obj, const char* key, std::string& out) {
	glz::generic::object_t::const_iterator it = obj.find(key);
	if (it == obj.end() || !it->second.is_string()) {
		return false;
	}
	out = it->second.get<std::string>();
	return true;
}

void populate_message_from_generic(const glz::generic& root, Error& err) {
	if (!root.is_object()) {
		return;
	}
	const glz::generic::object_t& obj = root.get_object();

	// CDO format: {"status":"400","message":"Invalid token"}
	if (obj.find("status") != obj.end() && extract_string_into(obj, "message", err.message)) {
		return;
	}

	// Data Service format: {"errorMessage":"Bad request","errors":[...]}
	if (extract_string_into(obj, "errorMessage", err.message)) {
		glz::generic::object_t::const_iterator errs_it = obj.find("errors");
		if (errs_it != obj.end() && errs_it->second.is_array()) {
			const glz::generic::array_t& arr = errs_it->second.get_array();
			std::string details;
			for (const glz::generic& e : arr) {
				if (!e.is_string()) {
					continue;
				}
				if (!details.empty()) {
					details += "; ";
				}
				details += e.get<std::string>();
			}
			err.detail = std::move(details);
		}
		return;
	}

	// RFC 7807 Problem Details fallback
	extract_string_into(obj, "title", err.message);
	extract_string_into(obj, "detail", err.detail);
	if (err.correlation_id.empty()) {
		extract_string_into(obj, "correlationId", err.correlation_id);
	}
}

} // namespace

Error Error::from_response(int status, const std::string& body, const std::string& correlation_id) {
	Error err;
	err.http_status = status;
	err.correlation_id = correlation_id;

	// Determine error code from HTTP status
	// CDO uses 429 for rate limiting (not 503 like NWS)
	if (status == 404) {
		err.code = ErrorCode::NotFound;
	} else if (status == 400) {
		err.code = ErrorCode::InvalidRequest;
	} else if (status == 429) {
		err.code = ErrorCode::RateLimited;
	} else if (status >= 500) {
		err.code = ErrorCode::ServerError;
	} else {
		err.code = ErrorCode::Unknown;
	}

	// Try to parse error JSON in CDO, Data Service, or RFC 7807 format.
	glz::generic root{};
	glz::error_ctx ec = glz::read_json(root, body);
	if (!ec) {
		populate_message_from_generic(root, err);
	} else {
		// Not valid JSON — use raw body as message (truncated to keep error
		// objects from carrying multi-KB HTML pages on transient failures).
		err.message = body.substr(0, 256);
	}

	if (err.message.empty()) {
		err.message = "HTTP " + std::to_string(status);
	}

	return err;
}

} // namespace ncei

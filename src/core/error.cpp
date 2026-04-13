#include "ncei/error.hpp"

#include <nlohmann/json.hpp>

namespace ncei {

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

	// Try to parse error JSON in CDO, Data Service, or RFC 7807 format
	try {
		nlohmann::json j = nlohmann::json::parse(body);

		// CDO format: {"status":"400","message":"Invalid token"}
		if (j.contains("status") && j.contains("message") && j["message"].is_string()) {
			err.message = j["message"].get<std::string>();
		}
		// Data Service format: {"errorMessage":"Bad request","errors":[...]}
		else if (j.contains("errorMessage") && j["errorMessage"].is_string()) {
			err.message = j["errorMessage"].get<std::string>();
			if (j.contains("errors") && j["errors"].is_array() && !j["errors"].empty()) {
				std::string details;
				for (const nlohmann::json& e : j["errors"]) {
					if (e.is_string()) {
						if (!details.empty()) {
							details += "; ";
						}
						details += e.get<std::string>();
					}
				}
				err.detail = std::move(details);
			}
		}
		// RFC 7807 Problem Details fallback
		else {
			if (j.contains("title") && j["title"].is_string()) {
				err.message = j["title"].get<std::string>();
			}
			if (j.contains("detail") && j["detail"].is_string()) {
				err.detail = j["detail"].get<std::string>();
			}
			if (err.correlation_id.empty() && j.contains("correlationId") &&
				j["correlationId"].is_string()) {
				err.correlation_id = j["correlationId"].get<std::string>();
			}
		}
	} catch (...) {
		// Not valid JSON -- use raw body as message
		err.message = body.substr(0, 256);
	}

	if (err.message.empty()) {
		err.message = "HTTP " + std::to_string(status);
	}

	return err;
}

} // namespace ncei

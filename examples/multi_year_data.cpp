#include "ncei/ncei.hpp"

#include <cstdlib>
#include <iomanip>
#include <iostream>

int main() {
	const char* token_env = std::getenv("NCEI_CDO_TOKEN");
	if (!token_env) {
		std::cerr << "Set NCEI_CDO_TOKEN environment variable\n";
		return 1;
	}

	ncei::CDOClient::Config config;
	config.token = token_env;
	ncei::CDOClient client(std::move(config));

	// Fetch 3 years of TMAX data using get_data_all()
	// This demonstrates automatic date range splitting and pagination
	ncei::GetDataParams params;
	params.dataset_id = "GHCND";
	params.start_date = "2021-01-01";
	params.end_date = "2023-12-31";
	params.station_id = "GHCND:USW00013874";
	params.data_type_ids = std::vector<std::string>{"TMAX"};
	params.units = "standard";

	std::cout << "Fetching 3 years of TMAX data for RDU Airport...\n";
	std::cout << "(Date range will be automatically split into yearly chunks)\n\n";

	ncei::Result<std::vector<ncei::DataRecord>> result = client.get_data_all(params);
	if (!result) {
		std::cerr << "Error: " << result.error().message << "\n";
		return 1;
	}

	std::cout << "Total records retrieved: " << result->size() << "\n\n";

	// Show first and last 5 records
	std::size_t total = result->size();
	std::size_t preview_count = 5;

	if (total > 0) {
		std::cout << "First " << preview_count << " records:\n";
		for (std::size_t i = 0; i < preview_count && i < total; ++i) {
			const ncei::DataRecord& record = (*result)[i];
			std::string date_str = record.date;
			std::string::size_type t_pos = date_str.find('T');
			if (t_pos != std::string::npos) {
				date_str = date_str.substr(0, t_pos);
			}
			std::cout << "  " << date_str << "  " << record.datatype << "  " << std::fixed
					  << std::setprecision(1) << record.value << "\n";
		}

		if (total > preview_count * 2) {
			std::cout << "  ... (" << (total - preview_count * 2) << " more records) ...\n";
		}

		if (total > preview_count) {
			std::cout << "Last " << preview_count << " records:\n";
			std::size_t start = (total > preview_count) ? total - preview_count : 0;
			for (std::size_t i = start; i < total; ++i) {
				const ncei::DataRecord& record = (*result)[i];
				std::string date_str = record.date;
				std::string::size_type t_pos = date_str.find('T');
				if (t_pos != std::string::npos) {
					date_str = date_str.substr(0, t_pos);
				}
				std::cout << "  " << date_str << "  " << record.datatype << "  " << std::fixed
						  << std::setprecision(1) << record.value << "\n";
			}
		}
	}

	std::cout << "\nDaily requests remaining: " << client.daily_requests_remaining() << "\n";
	return 0;
}

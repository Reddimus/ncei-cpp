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

	// Query daily TMAX and TMIN for RDU airport, January 2024
	ncei::GetDataParams params;
	params.dataset_id = "GHCND";
	params.start_date = "2024-01-01";
	params.end_date = "2024-01-31";
	params.station_id = "GHCND:USW00013874";
	params.data_type_ids = std::vector<std::string>{"TMAX", "TMIN"};
	params.units = "standard"; // Fahrenheit
	params.limit = 1000;

	ncei::Result<ncei::CDOResponse<ncei::DataRecord>> result = client.get_data(params);
	if (!result) {
		std::cerr << "Error: " << result.error().message << "\n";
		return 1;
	}

	std::cout << "Daily temperatures at RDU Airport, January 2024:\n";
	std::cout << std::setw(12) << "Date" << std::setw(8) << "Type" << std::setw(10) << "Value"
			  << "\n";
	std::cout << std::string(30, '-') << "\n";

	for (const ncei::DataRecord& record : result->results) {
		// Extract just the date portion (before 'T')
		std::string date_str = record.date;
		std::string::size_type t_pos = date_str.find('T');
		if (t_pos != std::string::npos) {
			date_str = date_str.substr(0, t_pos);
		}

		std::cout << std::setw(12) << date_str << std::setw(8) << record.datatype << std::setw(10)
				  << std::fixed << std::setprecision(1) << record.value << "\n";
	}

	std::cout << "\nTotal records: " << result->results.size() << " of " << result->metadata.count
			  << "\n";
	return 0;
}

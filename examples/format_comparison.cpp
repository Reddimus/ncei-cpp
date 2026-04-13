#include "ncei/ncei.hpp"

#include <iostream>

int main() {
	ncei::DataServiceClient::Config config;
	ncei::DataServiceClient client(std::move(config));

	ncei::DataRequestParams csv_params;
	csv_params.dataset = "daily-summaries";
	csv_params.start_date = "2024-01-01";
	csv_params.end_date = "2024-01-07";
	csv_params.stations = std::vector<std::string>{"USW00013874"};
	csv_params.data_types = std::vector<std::string>{"TMAX", "TMIN"};
	csv_params.format = ncei::ResponseFormat::CSV;
	csv_params.include_station_name = true;

	std::cout << "=== CSV Format ===\n";
	ncei::Result<ncei::DataPointCollection> csv_result = client.get_data(csv_params);
	if (!csv_result) {
		std::cerr << "CSV error: " << csv_result.error().message << "\n";
	} else {
		std::cout << "Columns: ";
		for (const std::string& col : csv_result->columns) {
			std::cout << col << " ";
		}
		std::cout << "\nRecords: " << csv_result->records.size() << "\n";
		for (const ncei::DataPoint& dp : csv_result->records) {
			std::cout << "  " << dp.date << " " << dp.station;
			std::optional<std::string> tmax = dp.get("TMAX");
			std::optional<std::string> tmin = dp.get("TMIN");
			if (tmax)
				std::cout << " TMAX=" << *tmax;
			if (tmin)
				std::cout << " TMIN=" << *tmin;
			std::cout << "\n";
		}
	}

	ncei::DataRequestParams json_params = csv_params;
	json_params.format = ncei::ResponseFormat::JSON;

	std::cout << "\n=== JSON Format ===\n";
	ncei::Result<ncei::DataPointCollection> json_result = client.get_data(json_params);
	if (!json_result) {
		std::cerr << "JSON error: " << json_result.error().message << "\n";
	} else {
		std::cout << "Columns: ";
		for (const std::string& col : json_result->columns) {
			std::cout << col << " ";
		}
		std::cout << "\nRecords: " << json_result->records.size() << "\n";
		for (const ncei::DataPoint& dp : json_result->records) {
			std::cout << "  " << dp.date << " " << dp.station;
			std::optional<std::string> tmax = dp.get("TMAX");
			std::optional<std::string> tmin = dp.get("TMIN");
			if (tmax)
				std::cout << " TMAX=" << *tmax;
			if (tmin)
				std::cout << " TMIN=" << *tmin;
			std::cout << "\n";
		}
	}

	if (csv_result && json_result) {
		std::cout << "\n=== Comparison ===\n";
		std::cout << "CSV records: " << csv_result->records.size() << "\n";
		std::cout << "JSON records: " << json_result->records.size() << "\n";
		if (csv_result->records.size() == json_result->records.size()) {
			std::cout << "Record counts match.\n";
		} else {
			std::cout << "Record counts differ!\n";
		}
	}

	return 0;
}

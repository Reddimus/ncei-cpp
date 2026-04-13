#include "ncei/ncei.hpp"

#include <cstdlib>
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

	// List available datasets
	ncei::Result<ncei::CDOResponse<ncei::Dataset>> datasets = client.get_datasets();
	if (!datasets) {
		std::cerr << "Error: " << datasets.error().message << "\n";
		return 1;
	}

	std::cout << "Available datasets (" << datasets->metadata.count << " total):\n";
	for (const ncei::Dataset& ds : datasets->results) {
		std::cout << "  " << ds.id << " - " << ds.name << " (" << ds.min_date << " to "
				  << ds.max_date << ")\n";
	}

	// List data categories for GHCND
	ncei::CDOListParams cat_params;
	cat_params.dataset_id = "GHCND";
	ncei::Result<ncei::CDOResponse<ncei::DataCategory>> categories =
		client.get_data_categories(cat_params);
	if (!categories) {
		std::cerr << "Error: " << categories.error().message << "\n";
		return 1;
	}

	std::cout << "\nGHCND data categories (" << categories->metadata.count << " total):\n";
	for (const ncei::DataCategory& cat : categories->results) {
		std::cout << "  " << cat.id << " - " << cat.name << "\n";
	}

	std::cout << "\nDaily requests remaining: " << client.daily_requests_remaining() << "\n";
	return 0;
}

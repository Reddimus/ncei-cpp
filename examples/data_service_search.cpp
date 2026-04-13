#include "ncei/ncei.hpp"

#include <iostream>

int main() {
	ncei::DataServiceClient::Config config;
	ncei::DataServiceClient client(std::move(config));

	ncei::Result<ncei::DatasetMetadata> result = client.get_dataset_metadata("daily-summaries");

	if (!result) {
		std::cerr << "Error: " << result.error().message << "\n";
		return 1;
	}

	const ncei::DatasetMetadata& meta = *result;
	std::cout << "Dataset: " << meta.name << "\n";
	std::cout << "ID: " << meta.id << "\n";
	std::cout << "Description: " << meta.description << "\n";
	std::cout << "\nAvailable fields (" << meta.fields.size() << "):\n";

	for (const ncei::DatasetField& field : meta.fields) {
		std::cout << "  " << field.id << " - " << field.name;
		if (!field.data_type.empty()) {
			std::cout << " (" << field.data_type << ")";
		}
		if (!field.description.empty()) {
			std::cout << ": " << field.description;
		}
		std::cout << "\n";
	}

	return 0;
}

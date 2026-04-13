# NCEI C++ SDK - Root Makefile
# Wraps CMake for convenient day-to-day workflow

BUILD_DIR := build
CMAKE := cmake
NPROC := $(shell nproc 2>/dev/null || echo 4)
BENCH_ITERATIONS := 254

.PHONY: all build debug test lint clean configure help bench format coverage \
	run-basic_cdo_usage run-historical_temperature run-multi_year_data run-format_comparison run-data_service_search

# Default target
all: build

# Configure CMake (if needed)
configure:
	@mkdir -p $(BUILD_DIR)
	@cd $(BUILD_DIR) && $(CMAKE) .. -DCMAKE_BUILD_TYPE=Release -DCMAKE_EXPORT_COMPILE_COMMANDS=ON

# Configure debug build
configure-debug:
	@mkdir -p $(BUILD_DIR)
	@cd $(BUILD_DIR) && $(CMAKE) .. -DCMAKE_BUILD_TYPE=Debug -DCMAKE_EXPORT_COMPILE_COMMANDS=ON

# Build all targets
build: configure
	@$(CMAKE) --build $(BUILD_DIR) -j$(NPROC)

# Debug build
debug: configure-debug
	@$(CMAKE) --build $(BUILD_DIR) -j$(NPROC)

# Run tests
test: build
	@cd $(BUILD_DIR) && ctest --output-on-failure

# Run linting (clang-format check)
lint:
	@if command -v clang-format >/dev/null 2>&1; then \
		echo "Checking code formatting..."; \
		find src include tests examples \( -name '*.cpp' -o -name '*.hpp' \) -print0 | \
			xargs -0 clang-format --dry-run --Werror && \
		echo "Format check passed."; \
	else \
		echo "clang-format not found. Install clang-format to run lint."; \
		exit 1; \
	fi

# Format code in place
format:
	@if command -v clang-format >/dev/null 2>&1; then \
		echo "Formatting code..."; \
		find src include tests examples \( -name '*.cpp' -o -name '*.hpp' \) -print0 | xargs -0 clang-format -i; \
		echo "Done"; \
	else \
		echo "clang-format not found. Install clang-format to format code."; \
		exit 1; \
	fi

# Code coverage (requires lcov)
coverage:
	@mkdir -p build-coverage
	@cd build-coverage && $(CMAKE) .. -DCMAKE_BUILD_TYPE=Debug -DNCEI_ENABLE_COVERAGE=ON -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
	@$(CMAKE) --build build-coverage -j$(NPROC)
	@cd build-coverage && ctest --output-on-failure
	@lcov --capture --directory build-coverage --output-file build-coverage/coverage.info --ignore-errors mismatch
	@lcov --remove build-coverage/coverage.info '/usr/*' '*/build-coverage/_deps/*' --output-file build-coverage/coverage_filtered.info --ignore-errors unused
	@genhtml build-coverage/coverage_filtered.info --output-directory build-coverage/coverage-report
	@echo "Coverage report: build-coverage/coverage-report/index.html"

# Clean build artifacts
clean:
	@rm -rf $(BUILD_DIR) build-coverage
	@echo "Cleaned build directory"

# Run benchmark
bench: build
	@./tools/bench.sh $(BENCH_ITERATIONS)

# Run examples (load .env for NCEI_CDO_TOKEN)
run-basic_cdo_usage: build
	@if [ -f .env ]; then set -a && . ./.env && set +a; fi && ./$(BUILD_DIR)/examples/example_basic_cdo

run-historical_temperature: build
	@if [ -f .env ]; then set -a && . ./.env && set +a; fi && ./$(BUILD_DIR)/examples/example_historical_temperature

run-multi_year_data: build
	@if [ -f .env ]; then set -a && . ./.env && set +a; fi && ./$(BUILD_DIR)/examples/example_multi_year_data

run-format_comparison: build
	@if [ -f .env ]; then set -a && . ./.env && set +a; fi && ./$(BUILD_DIR)/examples/example_format_comparison

run-data_service_search: build
	@if [ -f .env ]; then set -a && . ./.env && set +a; fi && ./$(BUILD_DIR)/examples/example_data_service_search

# Help
help:
	@echo "NCEI C++ SDK Build System"
	@echo ""
	@echo "Targets:"
	@echo "  make build           - Configure and build the SDK (Release)"
	@echo "  make debug           - Configure and build the SDK (Debug)"
	@echo "  make test            - Run tests"
	@echo "  make bench           - Run benchmark ($(BENCH_ITERATIONS) iterations)"
	@echo "  make lint            - Check code formatting"
	@echo "  make format          - Format code in place"
	@echo "  make coverage        - Generate code coverage report (requires lcov)"
	@echo "  make clean           - Remove build artifacts"
	@echo "  make help            - Show this help"
	@echo ""
	@echo "Examples:"
	@echo "  make run-basic_cdo_usage       - Run basic CDO usage example"
	@echo "  make run-historical_temperature- Run historical temperature example"
	@echo "  make run-multi_year_data       - Run multi-year data example"
	@echo "  make run-format_comparison     - Run format comparison example"
	@echo "  make run-data_service_search   - Run data service search example"
	@echo ""
	@echo "Variables:"
	@echo "  BENCH_ITERATIONS=N  - Override benchmark iterations (default: 254)"

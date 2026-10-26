# NCEI C++ SDK - Root Makefile
# Wraps CMake for convenient day-to-day workflow

BUILD_DIR ?= build
BUILD_TYPE ?= Release
CMAKE_ARGS ?=
CPP_AUTO_AUDIT := python3 tools/cpp_auto_audit.py
CMAKE := cmake
NPROC := $(shell nproc 2>/dev/null || echo 4)

.PHONY: all build debug test lint clean configure help bench format pre-commit install-hooks coverage \
	run-basic_cdo_usage run-historical_temperature run-multi_year_data run-format_comparison run-data_service_search

# Default target
all: build

# Configure CMake (if needed)
configure:
	@mkdir -p $(BUILD_DIR)
	@cd $(BUILD_DIR) && $(CMAKE) .. -DCMAKE_BUILD_TYPE=$(BUILD_TYPE) -DCMAKE_EXPORT_COMPILE_COMMANDS=ON $(CMAKE_ARGS)

# Build all targets
build: configure
	@$(CMAKE) --build $(BUILD_DIR) -j$(NPROC)

# Debug build
debug:
	@$(MAKE) build BUILD_DIR=build-debug BUILD_TYPE=Debug

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
	$(CPP_AUTO_AUDIT)


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

# pre-commit: auto-format, then lint. Run before every commit to avoid
# the recurring "push -> CI clang-format/auto-audit fail -> follow-up fix
# PR" loop. Idempotent — `format` re-running is a no-op.
pre-commit: format lint

# Install a lint-only hook while preserving custom hooks.
install-hooks:
	@hook="$$(git rev-parse --git-path hooks)/pre-commit"; \
	if [ -f "$$hook" ]; then \
        case "$$(cat "$$hook")" in \
            "$$(printf '#!/bin/sh\nexec make lint')"|"$$(printf '#!/bin/sh\nexec make pre-commit')") ;; \
            *) echo "$$hook already exists; not replacing it"; exit 1 ;; \
        esac; \
    fi; \
	mkdir -p "$$(dirname "$$hook")" && printf '#!/bin/sh\nexec make lint\n' > "$$hook" && \
	chmod +x "$$hook" && echo "Installed $$hook (runs make lint)"


# Clean build artifacts
clean:
	@rm -rf $(BUILD_DIR) build-debug build-coverage
	@echo "Cleaned build directory"

# Run benchmark
bench: build
	@BUILD_DIR="$(BUILD_DIR)" bash ./tools/bench.sh

# Run examples (load .env for NCEI_CDO_TOKEN)
run-basic_cdo_usage: build
	@if [ -f .env ]; then set -a && . ./.env && set +a; fi && ./$(BUILD_DIR)/examples/basic_cdo_usage

run-historical_temperature: build
	@if [ -f .env ]; then set -a && . ./.env && set +a; fi && ./$(BUILD_DIR)/examples/historical_temperature

run-multi_year_data: build
	@if [ -f .env ]; then set -a && . ./.env && set +a; fi && ./$(BUILD_DIR)/examples/multi_year_data

run-format_comparison: build
	@if [ -f .env ]; then set -a && . ./.env && set +a; fi && ./$(BUILD_DIR)/examples/format_comparison

run-data_service_search: build
	@if [ -f .env ]; then set -a && . ./.env && set +a; fi && ./$(BUILD_DIR)/examples/data_service_search

# Help
help:
	@echo "NCEI C++ SDK Build System"
	@echo ""
	@echo "Targets:"
	@echo "  make build           - Configure and build the SDK (Release)"
	@echo "  make debug           - Configure and build the SDK (Debug)"
	@echo "  make test            - Run tests"
	@echo "  make bench           - Run benchmark (offline parsing)"
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


consumers:
	@bash tools/test_consumers.sh $(CMAKE_ARGS)

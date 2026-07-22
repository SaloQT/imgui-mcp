.PHONY: all build clean test demo setup release

all: build

build:
	@cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
	@cmake --build build --parallel
	@echo "Build complete: build/imgui_mcp_app"

clean:
	@rm -rf build
	@echo "Cleaned."

test: build
	@ctest --test-dir build --output-on-failure

demo: build
	@python3 demo.py

setup: build
	@./setup.sh

release:
	@./release.sh "$$(tr -d '[:space:]' < VERSION)"

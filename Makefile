.PHONY: all build clean test demo setup release

all: build

build:
	@mkdir -p build
	@cd build && cmake .. -DCMAKE_BUILD_TYPE=Release && make -j$$(nproc)
	@echo "Build complete: build/imgui_mcp_app"

clean:
	@rm -rf build
	@echo "Cleaned."

test: build
	@echo '{"cmd":"create_window","id":"t","title":"Test"}\n{"cmd":"get_state"}\n{"cmd":"quit"}' | ./build/imgui_mcp_app --width 100 --height 100 2>/dev/null | python3 -c "import sys,json; [print('✓',json.loads(l).get('type','')) for l in sys.stdin if l.strip()]"

demo: build
	@python3 demo.py

setup: build
	@./setup.sh

release:
	@./release.sh 1.0.0

CXX := clang++
ARM_CXX ?= arm-none-eabi-g++
ARM_PREFIX ?= arm-none-eabi-
CLANG_TIDY ?= $(shell command -v clang-tidy 2>/dev/null || printf /opt/homebrew/opt/llvm/bin/clang-tidy)
ANALYSIS_CXX ?= $(dir $(CLANG_TIDY))clang++
BUILD ?= build
CPPFLAGS := -Isrc -IdistingNT_API/include -Itests
WARN := -Wall -Wextra -Wpedantic -Werror -Wconversion -Wshadow
CXXFLAGS := -std=c++17 -O2 $(WARN) -fno-exceptions -fno-rtti -ffp-contract=off
SRC := src/pop_engine.cpp src/pop_plugin.cpp
TESTFLAGS := -DPOP_TESTING
.PHONY: test sanitize hardware imports-check inspect desktop analyze live perf fixtures-check clock-test manual package release-package
$(BUILD):
	mkdir -p $@
$(BUILD)/engine_test: src/pop_engine.cpp src/pop_engine.h src/pop_sample_math.h tests/pop_engine_test.cpp tests/check.h tests/fixtures/internal.h | $(BUILD)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $(TESTFLAGS) src/pop_engine.cpp tests/pop_engine_test.cpp -o $@
$(BUILD)/sample_math_test: src/pop_engine.cpp src/pop_engine.h src/pop_sample_math.h tests/pop_sample_math_test.cpp tests/check.h | $(BUILD)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $(TESTFLAGS) src/pop_engine.cpp tests/pop_sample_math_test.cpp -o $@
$(BUILD)/regressions_test: $(SRC) src/pop_engine.h src/pop_sample_math.h src/pop_plugin.h tests/nt_host_driver.cpp tests/nt_host_driver.h tests/pop_regressions_test.cpp tests/check.h | $(BUILD)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $(TESTFLAGS) $(SRC) tests/nt_host_driver.cpp tests/pop_regressions_test.cpp -o $@
$(BUILD)/clock_test: $(SRC) src/pop_engine.h src/pop_sample_math.h src/pop_plugin.h tests/nt_host_driver.cpp tests/nt_host_driver.h tests/pop_clock_test.cpp tests/fixtures/clock/vectors.h tests/check.h | $(BUILD)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $(TESTFLAGS) $(SRC) tests/nt_host_driver.cpp tests/pop_clock_test.cpp -o $@
$(BUILD)/clock_boundaries_test: src/pop_engine.cpp src/pop_engine.h src/pop_sample_math.h tests/pop_clock_boundaries_test.cpp tests/check.h | $(BUILD)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $(TESTFLAGS) src/pop_engine.cpp tests/pop_clock_boundaries_test.cpp -o $@
$(BUILD)/clock_math_test: src/pop_engine.cpp src/pop_engine.h src/pop_sample_math.h src/pop_clock_math.h tests/pop_clock_math_test.cpp tests/check.h | $(BUILD)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $(TESTFLAGS) src/pop_engine.cpp tests/pop_clock_math_test.cpp -o $@
$(BUILD)/clock_corrections_test: $(SRC) src/pop_engine.h src/pop_sample_math.h src/pop_plugin.h tests/nt_host_driver.cpp tests/nt_host_driver.h tests/pop_clock_corrections_test.cpp tests/check.h | $(BUILD)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $(TESTFLAGS) $(SRC) tests/nt_host_driver.cpp tests/pop_clock_corrections_test.cpp -o $@
clock-test: $(BUILD)/clock_test $(BUILD)/clock_boundaries_test $(BUILD)/clock_corrections_test $(BUILD)/clock_math_test
	python3 tests/clock_math_reference.py $(BUILD)/clock_math_test
	$(BUILD)/clock_test
	$(BUILD)/clock_boundaries_test
	$(BUILD)/clock_corrections_test
test: $(BUILD)/engine_test $(BUILD)/adapter_test $(BUILD)/rates_test $(BUILD)/sample_math_test $(BUILD)/regressions_test $(BUILD)/clock_test $(BUILD)/clock_boundaries_test $(BUILD)/clock_corrections_test $(BUILD)/clock_math_test
	python3 tests/clock_math_reference.py $(BUILD)/clock_math_test
	$(BUILD)/clock_corrections_test
	$(BUILD)/clock_test
	$(BUILD)/clock_boundaries_test
	$(BUILD)/sample_math_test
	python3 tests/check_arm_imports_test.py
	$(BUILD)/engine_test
	$(BUILD)/adapter_test
	$(BUILD)/rates_test
	$(BUILD)/regressions_test
sanitize:
	$(MAKE) BUILD=build/sanitize CXXFLAGS='-std=c++17 -O1 -g $(WARN) -fno-exceptions -fno-rtti -ffp-contract=off -fsanitize=address,undefined -fno-omit-frame-pointer' test
$(BUILD)/adapter_test: $(SRC) src/pop_engine.h src/pop_sample_math.h src/pop_plugin.h tests/nt_host_driver.cpp tests/nt_host_driver.h tests/pop_adapter_test.cpp tests/check.h tests/fixtures/internal.h | $(BUILD)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $(TESTFLAGS) $(SRC) tests/nt_host_driver.cpp tests/pop_adapter_test.cpp -o $@
$(BUILD)/rates_test: $(SRC) src/pop_engine.h src/pop_sample_math.h src/pop_plugin.h tests/nt_host_driver.cpp tests/nt_host_driver.h tests/pop_rates_test.cpp tests/check.h | $(BUILD)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $(TESTFLAGS) $(SRC) tests/nt_host_driver.cpp tests/pop_rates_test.cpp -o $@
live: $(BUILD)/adapter_test $(BUILD)/rates_test $(BUILD)/clock_test $(BUILD)/clock_boundaries_test $(BUILD)/clock_corrections_test
	$(BUILD)/clock_corrections_test
	$(BUILD)/adapter_test
	$(BUILD)/rates_test
	$(BUILD)/clock_test
	$(BUILD)/clock_boundaries_test
$(BUILD)/perf: $(SRC) src/pop_sample_math.h tests/nt_host_driver.cpp tests/nt_host_driver.h tests/pop_perf.cpp | $(BUILD)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $(TESTFLAGS) $(SRC) tests/nt_host_driver.cpp tests/pop_perf.cpp -o $@
perf: $(BUILD)/perf
	$(BUILD)/perf
ARMFLAGS := -std=c++17 -Os $(WARN) -mcpu=cortex-m7 -mfpu=fpv5-d16 -mfloat-abi=hard -mthumb -fPIC -fno-exceptions -fno-rtti -fno-unwind-tables -fno-asynchronous-unwind-tables -ffp-contract=off
$(BUILD)/arm/%.o: src/%.cpp src/pop_engine.h src/pop_sample_math.h src/pop_plugin.h src/pop_clock_math.h
	mkdir -p $(@D)
	$(ARM_CXX) $(CPPFLAGS) $(ARMFLAGS) -c $< -o $@
$(BUILD)/pop.o: $(BUILD)/arm/pop_engine.o $(BUILD)/arm/pop_plugin.o
	$(ARM_CXX) -nostdlib -r $^ -o $@
hardware: $(BUILD)/pop.o imports-check
imports-check: $(BUILD)/pop.o
	python3 tests/check_arm_imports.py --nm $(ARM_PREFIX)nm $(BUILD)/pop.o
	python3 tests/check_arm_identity.py $(BUILD)/pop.o
inspect: hardware
	$(ARM_PREFIX)readelf -h -S $(BUILD)/pop.o
	$(ARM_PREFIX)size -A $(BUILD)/pop.o
	$(ARM_PREFIX)nm -S --size-sort -C $(BUILD)/pop.o
	$(ARM_PREFIX)nm -u -C $(BUILD)/pop.o
	$(ARM_PREFIX)nm -g $(BUILD)/pop.o | grep ' T pluginEntry$$'
desktop: | $(BUILD)
ifeq ($(shell uname -s),Darwin)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -fPIC -dynamiclib -undefined dynamic_lookup $(SRC) -o $(BUILD)/pop.dylib
else
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -fPIC -shared $(SRC) -o $(BUILD)/pop.so
endif
$(BUILD)/compile_commands.json: $(SRC) src/pop_engine.h src/pop_sample_math.h src/pop_plugin.h src/pop_clock_math.h Makefile | $(BUILD)
	$(ANALYSIS_CXX) $(CPPFLAGS) $(CXXFLAGS) -MJ $(BUILD)/engine-command.json -c src/pop_engine.cpp -o $(BUILD)/engine-analysis.o
	$(ANALYSIS_CXX) $(CPPFLAGS) $(CXXFLAGS) -MJ $(BUILD)/plugin-command.json -c src/pop_plugin.cpp -o $(BUILD)/plugin-analysis.o
	python3 -c 'import json,pathlib; p=pathlib.Path("$(BUILD)"); (p/"compile_commands.json").write_text(json.dumps([json.loads((p/n).read_text().strip().rstrip(",")) for n in ["engine-command.json","plugin-command.json"]],indent=2))'
analyze: $(BUILD)/compile_commands.json
	cppcheck --enable=warning,style,performance,portability --std=c++17 --inline-suppr --suppress=missingIncludeSystem --suppress=noExplicitConstructor:distingNT_API/include/distingnt/serialisation.h --error-exitcode=1 -Isrc -IdistingNT_API/include src
	$(CXX) $(CPPFLAGS) -std=c++17 --analyze $(SRC) -Xanalyzer -analyzer-output=text
	$(CLANG_TIDY) -p $(BUILD) --config='{Checks: "-*,clang-analyzer-*,performance-*", HeaderFilterRegex: "(^|/)src/", SystemHeaders: false}' --warnings-as-errors='*' $(SRC)
manual:
	python3 tools/render_manual.py --output $(BUILD)/manual
package: hardware
	python3 tools/package.py --build $(BUILD) --output $(BUILD)/release
release-package: hardware manual
	python3 tools/package.py --build $(BUILD) --output $(BUILD)/release --require-pdf
fixtures-check:
	shasum -a 256 -c tests/fixtures/SHA256SUMS
$(addprefix $(BUILD)/,engine_test sample_math_test regressions_test clock_test clock_boundaries_test clock_corrections_test clock_math_test adapter_test rates_test perf): $(wildcard src/*.h)

# Builds the mount firmware's core for the PC, with the simulated platform.
#   make        the simulator, build/star-track-sim
#   make test   unit tests, then a scripted night on the simulator
#   make clean

ARDUINOJSON ?= $(HOME)/Arduino/libraries/ArduinoJson/src
CORE := ../star-track/src/core
BUILD := build

CXX ?= g++
CXXFLAGS ?= -O2 -g
CXXFLAGS += -std=c++17 -Wall -Wextra -I$(CORE) -I$(ARDUINOJSON) -I. -MMD -MP
CXXFLAGS += -DSIM_WEB_DIR='"$(abspath web)"' -DSIM_APP_DIR='"$(abspath ../../app/dist)"'

CORE_SRC := $(wildcard $(CORE)/*.cpp)
CORE_OBJ := $(patsubst $(CORE)/%.cpp,$(BUILD)/core/%.o,$(CORE_SRC))
SIM_OBJ := $(BUILD)/sim.o $(BUILD)/server.o $(BUILD)/main.o
TEST_OBJ := $(BUILD)/tests/test_core.o

all: $(BUILD)/star-track-sim

$(BUILD)/star-track-sim: $(CORE_OBJ) $(SIM_OBJ)
	$(CXX) $(CXXFLAGS) -o $@ $^

$(BUILD)/test_core: $(CORE_OBJ) $(TEST_OBJ)
	$(CXX) $(CXXFLAGS) -o $@ $^

$(BUILD)/core/%.o: $(CORE)/%.cpp | check-deps
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -c -o $@ $<

$(BUILD)/%.o: %.cpp | check-deps
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -c -o $@ $<

check-deps:
	@test -f $(ARDUINOJSON)/ArduinoJson.h || { \
	  echo "ArduinoJson not found in $(ARDUINOJSON)."; \
	  echo "Install it with: arduino-cli lib install ArduinoJson@7.2.1"; \
	  echo "or pass ARDUINOJSON=/path/to/ArduinoJson/src"; exit 1; }

test: $(BUILD)/test_core $(BUILD)/star-track-sim
	./$(BUILD)/test_core
	python3 tests/test_session.py ./$(BUILD)/star-track-sim
	python3 tests/test_fuzz.py ./$(BUILD)/star-track-sim 20 200

# A longer run of the random test: make fuzz SEEDS=200 ACTIONS=300
SEEDS ?= 200
ACTIONS ?= 300
fuzz: $(BUILD)/star-track-sim
	python3 tests/test_fuzz.py ./$(BUILD)/star-track-sim $(SEEDS) $(ACTIONS)

clean:
	rm -rf $(BUILD)

.PHONY: all test fuzz clean check-deps

-include $(CORE_OBJ:.o=.d) $(SIM_OBJ:.o=.d) $(TEST_OBJ:.o=.d)

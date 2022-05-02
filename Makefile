include Traces.mk

SOURCE_ROOT := src
BUILD_ROOT := build

SOURCES := $(shell find $(SOURCE_ROOT) -type f -name '*.cpp')
OBJECTS := $(patsubst $(SOURCE_ROOT)/%,$(BUILD_ROOT)/obj/%.o,$(SOURCES))
DEPENDS := $(addsuffix .d,$(OBJECTS))

EXECUTABLE := $(BUILD_ROOT)/bin/vk-hair.out

PACKAGES := vulkan glfw3 glm fmt

CXX := clang++
CXXFLAGS := -Wall -Werror -Wextra -std=c++17 -MD -MP -g -O3  $(shell pkg-config --cflags $(PACKAGES))
CXXDEFS := -DGLM_FORCE_RADIANS -DGLM_FORCE_DEPTH_ZERO_TO_ONE -DFMT_ENFORCE_COMPILE_STRING
LDFLAGS := $(shell pkg-config --libs $(PACKAGES))

all: $(OBJECTS) $(EXECUTABLE)

-include $(DEPENDS)

$(EXECUTABLE): $(OBJECTS)
	@mkdir -p $(@D)
	$(CXX) $^ -o $@ $(LDFLAGS)

$(BUILD_ROOT)/obj/%.o: $(SOURCE_ROOT)/%
	@mkdir -p $(@D)
	$(CXX) $(CXXFLAGS) $(CXXDEFS) -c -MF$@.d -o $@ $<

run: all
	(cd $(dir $(EXECUTABLE)); ./$(notdir $(EXECUTABLE)) $(ARGS))

debug: all
	(cd $(dir $(EXECUTABLE)); gdb -q --args ./$(notdir $(EXECUTABLE)) $(ARGS))

clean:
	rm -rf $(BUILD_ROOT)

.PHONY: all run debug clean

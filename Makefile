include Traces.mk

SOURCE_ROOT := src
BUILD_ROOT := build
DATA_ROOT := data
IMGUI_ROOT := submodules/imgui

SOURCES := $(shell find $(SOURCE_ROOT) -type f -name '*.cpp')
OBJECTS := $(patsubst $(SOURCE_ROOT)/%,$(BUILD_ROOT)/obj/%.o,$(SOURCES))
DEPENDS := $(addsuffix .d,$(OBJECTS))

EXECUTABLE := $(BUILD_ROOT)/bin/vk-hair.out

SHADER_SOURCES := $(shell find $(DATA_ROOT) -type f -name '*.glsl')
SHADER_OBJECTS := $(patsubst $(DATA_ROOT)/%.glsl,$(BUILD_ROOT)/bin/data/%.spv,$(SHADER_SOURCES))

MODEL_SOURCES := $(shell find $(DATA_ROOT) -type f -name '*.obj')
MODEL_OBJECTS := $(patsubst $(DATA_ROOT)/%.obj,$(BUILD_ROOT)/bin/data/%.obj,$(MODEL_SOURCES))

IMGUI_SOURCES := $(shell find $(IMGUI_ROOT) -maxdepth 1 -type f -name '*.cpp')
IMGUI_SOURCES += $(IMGUI_ROOT)/backends/imgui_impl_glfw.cpp $(IMGUI_ROOT)/backends/imgui_impl_vulkan.cpp
IMGUI_OBJECTS := $(patsubst $(IMGUI_ROOT)/%,$(BUILD_ROOT)/imgui/%.o,$(IMGUI_SOURCES))

OBJECTS += $(IMGUI_OBJECTS)
DEPENDS += $(addsuffix .d,$(IMGUI_OBJECTS))

PACKAGES := vulkan glfw3 glm fmt

SUBMODULE_INCLUDES := -Isubmodules -Isubmodules/VulkanMemoryAllocator/include -Isubmodules/imgui

CXX := clang++
CXXFLAGS := -Wall -Werror -Wextra -std=c++17 -MD -MP -g -O3  $(shell pkg-config --cflags $(PACKAGES)) $(SUBMODULE_INCLUDES)
CXXDEFS := -DGLM_FORCE_RADIANS -DGLM_FORCE_DEPTH_ZERO_TO_ONE -DFMT_ENFORCE_COMPILE_STRING -DGLFW_INCLUDE_VULKAN
LDFLAGS := $(shell pkg-config --libs $(PACKAGES))

# Required for VMA, unfortunately can't disable this using a pragma for a single file.
CXXFLAGS += -Wno-nullability-completeness

all: $(SHADER_OBJECTS) $(MODEL_OBJECTS) $(OBJECTS) $(EXECUTABLE)

-include $(DEPENDS)

$(EXECUTABLE): $(OBJECTS)
	@mkdir -p $(@D)
	$(CXX) $^ -o $@ $(LDFLAGS)

$(BUILD_ROOT)/obj/%.o: $(SOURCE_ROOT)/%
	@mkdir -p $(@D)
	$(CXX) $(CXXFLAGS) $(CXXDEFS) -c -MF$@.d -o $@ $<

$(BUILD_ROOT)/imgui/%.o: $(IMGUI_ROOT)/%
	@mkdir -p $(@D)
	$(CXX) $(CXXFLAGS) $(CXXDEFS) -c -MF$@.d -o $@ $<

$(filter %.spv,$(SHADER_OBJECTS)): SHADER_STAGE := comp
$(filter %/vs.spv,$(SHADER_OBJECTS)): SHADER_STAGE := vert
$(filter %/fs.spv,$(SHADER_OBJECTS)): SHADER_STAGE := frag

$(BUILD_ROOT)/bin/data/%.spv: $(DATA_ROOT)/%.glsl
	@mkdir -p $(@D)
	glslc -fshader-stage=$(SHADER_STAGE) -o $@ $<

$(BUILD_ROOT)/bin/data/%.obj: $(DATA_ROOT)/%.obj
	@mkdir -p $(@D)
	cp $< $@

run: all
	(cd $(dir $(EXECUTABLE)); ./$(notdir $(EXECUTABLE)) $(ARGS))

debug: all
	(cd $(dir $(EXECUTABLE)); lldb ./$(notdir $(EXECUTABLE)) -- $(ARGS))

clean:
	rm -rf $(BUILD_ROOT)

.PHONY: all run debug clean

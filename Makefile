# CPP Compiler and flags
CXX = g++
CXXFLAGS = -std=c++17 -O2 -I./src
LDFLAGS = -lglfw -lvulkan -ldl -lpthread -lX11 -lXxf86vm -lXrandr -lXi
DEBUGFLAGS = -DDEBUG -g

# Shader compiler and flags
GLSLC = glslc
GLSLCFLAGS = -O -fshader-stage=comp --target-env=vulkan1.0

# Directories
SRC_DIR = src
BIN_DIR = bin
SHADER_SRC_DIR = shaders
SHADER_BIN_DIR = $(BIN_DIR)/shaders

# Output name and firectory
TARGETNAME = raytracerV
TARGET = $(BIN_DIR)/$(TARGETNAME)

# Search all .cpp
SRCS = $(shell find $(SRC_DIR) -name '*.cpp')
OBJS = $(SRCS:.cpp=.o)

# Shaders
SHADERS = $(wildcard $(SHADER_SRC_DIR)/*.comp)
SPV_SHADERS = $(patsubst $(SHADER_SRC_DIR)/%.comp, $(SHADER_BIN_DIR)/%.comp.spv, $(SHADERS))

# Main target
all: prepare_dirs $(TARGET) shaders

# Create directories if they dont exist
prepare_dirs:
	mkdir -p $(BIN_DIR)
	mkdir -p $(SHADER_BIN_DIR)

# Compile executable
$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) $^ -o $@ $(LDFLAGS)

# .cpp -> .o
%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Shaer to SPIR-V
shaders: $(SPV_SHADERS)

$(SHADER_BIN_DIR)/%.spv: $(SHADER_SRC_DIR)/% | prepare_dirs
	$(GLSLC) $(GLSLCFLAGS) -o $@ $<

# Debug build
debug: CXXFLAGS += $(DEBUGFLAGS)
debug: release

# Release build
release: all
	./$(TARGET)

# Get rid of the junk!
clean:
	rm -rf $(BIN_DIR)/*.o $(BIN_DIR)/*.spv $(BIN_DIR)/shaders/*.spv $(TARGET)
	find $(SRC_DIR) -name '*.o' -delete

.PHONY: all clean debug release shaders prepare_dirs

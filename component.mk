# component.mk for rbdimmerESP32 library
# ESP-IDF component configuration for legacy GNU Make build system
# 
# This file supports older ESP-IDF versions that use Make instead of CMake
# For new projects, use CMakeLists.txt instead
#
# Author: dev@rbdimmer.com  
# Version: 1.0.0
# Website: https://rbdimmer.com
# Repository: https://github.com/robotdyn-dimmer/rbdimmerESP32

# Component name (defaults to directory name)
COMPONENT_NAME := rbdimmer

# Source files to compile
# Add all C/C++ source files from src directory
COMPONENT_SRCDIRS := src src/platform

# Object files to generate (automatically derived from COMPONENT_SRCDIRS)
COMPONENT_OBJS := src/rbdimmerESP32.o \
                  src/platform/esp_idf_platform.o

# Include directories visible to other components
COMPONENT_ADD_INCLUDEDIRS := src include

# Private include directories only for this component
COMPONENT_PRIV_INCLUDEDIRS := src/platform

# Component dependencies
# These ESP-IDF components are required for rbdimmer to work
COMPONENT_DEPENDS := driver esp_timer freertos log

# Compiler flags specific to this component
COMPONENT_CFLAGS += -Wall -Wextra -O2
COMPONENT_CXXFLAGS += -Wall -Wextra -O2

# Add component-specific definitions
CFLAGS += -DRBDIMMER_ESP_IDF=1
CXXFLAGS += -DRBDIMMER_ESP_IDF=1

# Enable debug logging if configured
ifdef CONFIG_RBDIMMER_ENABLE_DEBUG_LOG
CFLAGS += -DRBDIMMER_DEBUG_LOG=1
CXXFLAGS += -DRBDIMMER_DEBUG_LOG=1
endif

# Override default max channels if configured
ifdef CONFIG_RBDIMMER_MAX_CHANNELS
CFLAGS += -DRBDIMMER_MAX_CHANNELS=$(CONFIG_RBDIMMER_MAX_CHANNELS)
CXXFLAGS += -DRBDIMMER_MAX_CHANNELS=$(CONFIG_RBDIMMER_MAX_CHANNELS)
endif

# Override default max phases if configured
ifdef CONFIG_RBDIMMER_MAX_PHASES
CFLAGS += -DRBDIMMER_MAX_PHASES=$(CONFIG_RBDIMMER_MAX_PHASES)
CXXFLAGS += -DRBDIMMER_MAX_PHASES=$(CONFIG_RBDIMMER_MAX_PHASES)
endif

# Component version
COMPONENT_VERSION := 1.0.0

# Additional clean files
COMPONENT_EXTRA_CLEAN := 

# Export component library name for other components
COMPONENT_LIBRARY = lib$(COMPONENT_NAME).a
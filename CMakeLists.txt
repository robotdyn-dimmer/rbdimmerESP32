# CMakeLists.txt for rbdimmerESP32 library
# ESP-IDF component configuration using CMake build system
# 
# This file defines how the rbdimmerESP32 library integrates with ESP-IDF projects
# Author: dev@rbdimmer.com
# Version: 1.0.0
# Website: https://rbdimmer.com

# Register component with ESP-IDF build system
idf_component_register(
    # Source files to compile
    SRCS "src/rbdimmerESP32.cpp"
         "src/internal/rbdimmer_curves.c"
         "src/internal/rbdimmer_zerocross.c"
         "src/internal/rbdimmer_timer.c"
         "src/internal/rbdimmer_channel.c"
         "src/internal/rbdimmer_transition.c"

    # Include directories accessible to users of this component
    INCLUDE_DIRS "src"
    
    # Required ESP-IDF components
    REQUIRES driver 
             esp_timer 
             freertos
             log
    
    # Private requirements (not exposed to users)
    PRIV_REQUIRES esp_hw_support
)

# Component compile options
target_compile_options(${COMPONENT_LIB} PRIVATE 
    -Wall 
    -Wextra 
    # -Werror  # TODO: re-enable after stabilization
    -O2
)

# Add component-specific definitions
target_compile_definitions(${COMPONENT_LIB} PUBLIC
    RBDIMMER_ESP_IDF=1
)

# If building for Arduino framework within ESP-IDF
if(ARDUINO_PLATFORM)
    target_compile_definitions(${COMPONENT_LIB} PUBLIC
        RBDIMMER_ARDUINO_COMPAT=1
    )
endif()

# Configure component dependencies for Kconfig
# This allows users to configure the library through menuconfig
if(CONFIG_RBDIMMER_ENABLE_DEBUG_LOG)
    target_compile_definitions(${COMPONENT_LIB} PUBLIC
        RBDIMMER_DEBUG_LOG=1
    )
endif()

# Add configuration for maximum channels if specified
if(DEFINED CONFIG_RBDIMMER_MAX_CHANNELS)
    target_compile_definitions(${COMPONENT_LIB} PUBLIC
        RBDIMMER_MAX_CHANNELS=${CONFIG_RBDIMMER_MAX_CHANNELS}
    )
endif()

# Add configuration for maximum phases if specified  
if(DEFINED CONFIG_RBDIMMER_MAX_PHASES)
    target_compile_definitions(${COMPONENT_LIB} PUBLIC
        RBDIMMER_MAX_PHASES=${CONFIG_RBDIMMER_MAX_PHASES}
    )
endif()

# Include component version information
set(COMPONENT_VERSION "1.0.0")

# Export component information for other components
set(RBDIMMER_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src PARENT_SCOPE)
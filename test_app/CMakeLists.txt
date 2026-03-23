cmake_minimum_required(VERSION 3.16)

# Include the library component from the parent directory
set(EXTRA_COMPONENT_DIRS "..")

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(rbdimmer_test_app)

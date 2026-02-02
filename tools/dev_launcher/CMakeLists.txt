cmake_minimum_required(VERSION 3.13)
project(DevLauncher)

# Find required packages
find_package(Threads REQUIRED)

# Glob all sources
file(GLOB_RECURSE SRC_FILES *.cpp *.h)

# Create Executable
add_executable(DevLauncher ${SRC_FILES})

# Link dependencies
# We use the target names found in the root CMakeLists.txt
target_link_libraries(DevLauncher PRIVATE 
    ${BASELIB_TGT_NAME}
    ${FLTK_TGT_NAME}
    ${RAPID_JSON_TGT_NAME} # Implicitly via BaseLib usually, but good to be explicit if needed
    Threads::Threads
)

target_include_directories(DevLauncher PRIVATE "${PROJECT_SOURCE_DIR}")

# Standard compile features
target_compile_features(DevLauncher PRIVATE cxx_std_17)

# Copy to root build folder for easier access (Optional QOL)
add_custom_command(TARGET DevLauncher POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
    $<TARGET_FILE:DevLauncher>
    ${CMAKE_BINARY_DIR}/DevLauncher$<TARGET_FILE_SUFFIX:DevLauncher>
)

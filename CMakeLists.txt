
cmake_minimum_required(VERSION 3.16)
project(atm_simulator LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Match your SFML 2.x (x86_64) setup on Apple Silicon (Rosetta)
set(CMAKE_OSX_ARCHITECTURES x86_64)

# Your local SFML 2.x install
set(SFML_ROOT "$ENV{HOME}/sfml2")
set(SFML_INCLUDE_DIR "${SFML_ROOT}/include")
set(SFML_LIB_DIR     "${SFML_ROOT}/lib")

add_executable(atm_simulator
    main.cpp
    AtmInterface.cpp
)

# Include & link to SFML
target_include_directories(atm_simulator PRIVATE ${SFML_INCLUDE_DIR} .)
target_link_directories(atm_simulator  PRIVATE ${SFML_LIB_DIR})
target_link_libraries(atm_simulator PRIVATE
    sfml-graphics sfml-window sfml-system sfml-audio
)

# Let the exe find SFML dylibs at runtime (no env vars)
set_target_properties(atm_simulator PROPERTIES
    BUILD_RPATH "${SFML_LIB_DIR}"
)

# Copy assets (fonts, etc.) into build dir so relative loads work
add_custom_command(TARGET atm_simulator POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${CMAKE_SOURCE_DIR}/assets" "$<TARGET_FILE_DIR:atm_simulator>/assets"
)

cmake_minimum_required(VERSION 3.21)

# Library semver; the rule set version is DLPVERSION in antiLeech.h
project(specialdlp VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(SDLP_BUILD_TESTS "Build specialdlp tests" ${PROJECT_IS_TOP_LEVEL})
option(SDLP_INSTALL "Add install rules" ${PROJECT_IS_TOP_LEVEL})

set(SDLP_SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/specialdlp/specialdlp)

file(STRINGS ${SDLP_SRC_DIR}/antiLeech.h _sdlp_ver REGEX "^#define DLPVERSION[ \t]+[0-9]+" LIMIT_COUNT 1)
string(REGEX MATCH "[0-9]+$" SDLP_RULES_VERSION "${_sdlp_ver}")

set(SDLP_SOURCES
    ${SDLP_SRC_DIR}/antiLeech.cpp
    ${SDLP_SRC_DIR}/antiLeech.h
    ${SDLP_SRC_DIR}/sdlp_api.cpp
    ${SDLP_SRC_DIR}/sdlp_compat.cpp
    ${SDLP_SRC_DIR}/sdlp_compat.h
    ${SDLP_SRC_DIR}/sdlp_rules.h
    ${CMAKE_CURRENT_SOURCE_DIR}/include/specialdlp.h
)

if(WIN32)
    configure_file(cmake/specialdlp.rc.in ${CMAKE_CURRENT_BINARY_DIR}/specialdlp.rc @ONLY)
    list(APPEND SDLP_SOURCES ${CMAKE_CURRENT_BINARY_DIR}/specialdlp.rc)
endif()

# SHARED (not MODULE) so tests can link it; hosts load it at runtime.
add_library(specialdlp SHARED ${SDLP_SOURCES})
target_include_directories(specialdlp
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include
    PRIVATE ${SDLP_SRC_DIR})
target_compile_definitions(specialdlp PRIVATE
    SDLP_BUILDING
    SPECIAL_DLP_VERSION
    SDLP_LIB_VERSION="${PROJECT_VERSION}")
set_target_properties(specialdlp PROPERTIES
    CXX_VISIBILITY_PRESET hidden
    VISIBILITY_INLINES_HIDDEN ON
    INSTALL_NAME_DIR "@rpath")  # no SOVERSION: hosts swap one plain file on update

if(MSVC)
    target_compile_options(specialdlp PRIVATE /utf-8 /W3 /permissive-)
else()
    target_compile_options(specialdlp PRIVATE -Wall -Wextra)
    # upstream rule chains mix && and || without parens, on purpose
    set_source_files_properties(${SDLP_SRC_DIR}/antiLeech.cpp PROPERTIES COMPILE_OPTIONS
        "-Wno-parentheses;-Wno-unused-parameter;-Wno-ignored-qualifiers;-Wno-logical-op-parentheses")
    if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
        set_source_files_properties(${SDLP_SRC_DIR}/antiLeech.cpp PROPERTIES COMPILE_OPTIONS
            "-Wno-parentheses;-Wno-unused-parameter;-Wno-ignored-qualifiers")
    endif()
endif()

if(SDLP_INSTALL)
    include(GNUInstallDirs)
    install(TARGETS specialdlp
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR})
    install(FILES include/specialdlp.h DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
endif()

if(SDLP_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

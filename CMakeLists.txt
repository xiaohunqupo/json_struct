### Project
cmake_minimum_required(VERSION 3.15)
project(json_struct VERSION "1.0.3" HOMEPAGE_URL "https://github.com/jorgen/json_struct" DESCRIPTION "A library for parsing JSON directly to C++ structs and vice versa." LANGUAGES CXX)
set(CPACK_PACKAGE_VENDOR "Jørgen Lind")

set(ADDITIONAL_MODULES_DIR "${CMAKE_CURRENT_LIST_DIR}/cmake")
list(APPEND CMAKE_MODULE_PATH "${ADDITIONAL_MODULES_DIR}" "${ADDITIONAL_MODULES_DIR}/GenPkgConfig")

set(CMAKE_CXX_STANDARD_REQUIRED FALSE)
set(CMAKE_CXX_STANDARD 20)

### Misc
if(CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME)
    set(IS_TOPLEVEL_PROJECT TRUE)
else()
    set(IS_TOPLEVEL_PROJECT FALSE)
endif()
SET_PROPERTY(GLOBAL PROPERTY USE_FOLDERS ON)

if (DEFINED ENV{JSON_STRUCT_DISABLE_PCH})
  set(JSON_STRUCT_OPT_DISABLE_PCH_DEFAULT "$ENV{JSON_STRUCT_DISABLE_PCH}")
else()
  set(JSON_STRUCT_OPT_DISABLE_PCH_DEFAULT "OFF")
endif()

### Options
option(JSON_STRUCT_OPT_BUILD_BENCHMARKS "Build json_struct benchmarks" OFF)
option(JSON_STRUCT_OPT_BUILD_EXAMPLES "Build json_struct examples" ${IS_TOPLEVEL_PROJECT})
option(JSON_STRUCT_OPT_BUILD_TESTS "Build and perform json_struct tests" ${IS_TOPLEVEL_PROJECT})
option(JSON_STRUCT_OPT_INSTALL "Generate and install json_struct target" ${IS_TOPLEVEL_PROJECT})
option(JSON_STRUCT_OPT_DISABLE_PCH "Disable precompiled headers" ${JSON_STRUCT_OPT_DISABLE_PCH_DEFAULT})

### Compiler
function(set_compiler_flags_for_target target)
  set_property(TARGET ${target} PROPERTY CXX_STANDARD 11)
  if (MSVC)
    if(MSVC_VERSION GREATER_EQUAL 1920) #VS 2019 and above
      target_compile_options(${target} PRIVATE /W4 /WX)
    endif()
  elseif(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    target_compile_options(${target} PRIVATE -Wall -Wextra -Wpedantic -Werror -Wno-array-bounds)
  else()
    target_compile_options(${target} PRIVATE -Wall -Wextra -Wpedantic -Werror)
  endif()
endfunction()

### Include
set(JSON_STRUCT_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/include)

### Examples
if(JSON_STRUCT_OPT_BUILD_EXAMPLES)
  add_subdirectory(examples)
endif()

### Tests
if(JSON_STRUCT_OPT_BUILD_TESTS)
  enable_testing()
  add_subdirectory(tests)
endif()

### Benchmarks
if(JSON_STRUCT_OPT_BUILD_BENCHMARKS)
  if(CMAKE_COMPILER_IS_GNUCC AND CMAKE_CXX_COMPILER_VERSION VERSION_LESS 7.0)
    set(JSON_STRUCT_OPT_BUILD_BENCHMARKS OFF)
  endif()
  if(CMAKE_CXX_COMPILER_ID STREQUAL "AppleClang" AND CMAKE_CXX_COMPILER_VERSION VERSION_LESS 9.2)
    set(JSON_STRUCT_OPT_BUILD_BENCHMARKS OFF)
  endif()
endif()
if(JSON_STRUCT_OPT_BUILD_BENCHMARKS)
  add_subdirectory(performance)
endif()

### INSTALL
include(GNUInstallDirs)
include(GenPkgConfig)
include(CPackComponent)
include(CMakePackageConfigHelpers)

string(REPLACE "/${CMAKE_LIBRARY_ARCHITECTURE}" "" CMAKE_INSTALL_LIBDIR_ARCHIND "${CMAKE_INSTALL_LIBDIR}")

add_library(${PROJECT_NAME} INTERFACE)
add_library(${PROJECT_NAME}::${PROJECT_NAME} ALIAS ${PROJECT_NAME})
target_include_directories(${PROJECT_NAME}
        INTERFACE
            $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
            $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>)

set(CPACK_PACKAGE_NAME "${PROJECT_NAME}")
set(CPACK_PACKAGE_DESCRIPTION "${PROJECT_DESCRIPTION}")
set(CPACK_DEBIAN_PACKAGE_ARCHITECTURE "all")
set(CPACK_DEBIAN_PACKAGE_NAME "libjsonstruct-dev")
set(CPACK_RPM_PACKAGE_NAME "libjsonstruct-devel")
set(CPACK_PACKAGE_HOMEPAGE_URL "${PROJECT_HOMEPAGE_URL}")
set(CPACK_PACKAGE_MAINTAINER "${CPACK_PACKAGE_VENDOR}")
set(CPACK_DEBIAN_PACKAGE_DEPENDS "")
set(CPACK_DEBIAN_PACKAGE_MAINTAINER "${CPACK_PACKAGE_MAINTAINER}")
set(CPACK_PACKAGE_MAINTAINER "${CPACK_PACKAGE_VENDOR}")
set(CPACK_DEB_COMPONENT_INSTALL ON)
set(CPACK_RPM_COMPONENT_INSTALL ON)
set(CPACK_NSIS_COMPONENT_INSTALL ON)
set(CPACK_DEBIAN_COMPRESSION_TYPE "xz")
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/COPYING")
set(CPACK_RESOURCE_FILE_README "${CMAKE_CURRENT_SOURCE_DIR}/README.md")

set(CMAKE_CONFIG_FILE_BASENAME "${PROJECT_NAME}Config.cmake")
set(CMAKE_EXPORT_FILE_BASENAME "${PROJECT_NAME}Export.cmake")
set(CMAKE_CONFIG_VERSION_FILE_BASENAME "${PROJECT_NAME}ConfigVersion.cmake")
set(CMAKE_CONFIG_VERSION_FILE_NAME "${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_CONFIG_VERSION_FILE_BASENAME}")

set(EXPORT_NAMESPACE "${PROJECT_NAME}::")

write_basic_package_version_file(
    "${CMAKE_CONFIG_VERSION_FILE_NAME}"
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY AnyNewerVersion
    ARCH_INDEPENDENT
)

export(TARGETS "${PROJECT_NAME}"
    NAMESPACE "${EXPORT_NAMESPACE}"
    FILE "${CMAKE_EXPORT_FILE_BASENAME}"
    EXPORT_LINK_INTERFACE_LIBRARIES
)

if(JSON_STRUCT_OPT_INSTALL)
    install(TARGETS "${PROJECT_NAME}"
        EXPORT ${PROJECT_NAME}
        INCLUDES
            DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}"
            # COMPONENT "${SDK_COMPONENT_NAME}" # component is not allowed for includes! Headers are installed separately! Includes only marks the headers for export
    )


    install(FILES ${CMAKE_CONFIG_VERSION_FILE_NAME}
        DESTINATION ${CMAKE_INSTALL_LIBDIR_ARCHIND}/cmake/${PROJECT_NAME})

    install(EXPORT "${PROJECT_NAME}"
        FILE "${CMAKE_CONFIG_FILE_BASENAME}"
        NAMESPACE "${EXPORT_NAMESPACE}"
        DESTINATION "${CMAKE_INSTALL_LIBDIR_ARCHIND}/cmake/${PROJECT_NAME}"
    )


    configure_pkg_config_file("${PROJECT_NAME}"
        NAME "${PROJECT_NAME}"
        VERSION "${PROJECT_VERSION}"
        DESCRIPTION "${CPACK_PACKAGE_DESCRIPTION}"
        URL "${CPACK_PACKAGE_HOMEPAGE_URL}"
        INSTALL_LIB_DIR "${CMAKE_INSTALL_LIBDIR_ARCHIND}"
        INSTALL_INCLUDE_DIR "${CMAKE_INSTALL_INCLUDEDIR}"
    )



    file(GLOB HEADERS_TO_INSTALL ${CMAKE_CURRENT_SOURCE_DIR}/include/json_struct/*)
    install(FILES ${HEADERS_TO_INSTALL}
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/${PROJECT_NAME}
    )

    install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/package.xml
        DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/${PROJECT_NAME}
    )

    include(CPack)
endif()

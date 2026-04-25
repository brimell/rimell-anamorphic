cmake_minimum_required(VERSION 3.20)

project(RimellAnamorphicOFX VERSION 0.1.0 LANGUAGES CXX)

include(FetchContent)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

FetchContent_Declare(
  openfx
  GIT_REPOSITORY https://github.com/AcademySoftwareFoundation/openfx.git
  GIT_TAG 1b74e4369c02327012bf2abdd4f39319f52b8cbe
)

FetchContent_GetProperties(openfx)
if(NOT openfx_POPULATED)
  FetchContent_Populate(openfx)
endif()

set(OFX_PLUGIN_NAME RimellAnamorphic)
set(OFX_BUNDLE_NAME "${OFX_PLUGIN_NAME}.ofx.bundle")

if(APPLE)
  set(OFX_ARCH_DIR "MacOS")
  set(DEFAULT_OFX_INSTALL_DIR "/Library/OFX/Plugins")
elseif(WIN32)
  set(OFX_ARCH_DIR "Win64")
  set(DEFAULT_OFX_INSTALL_DIR "C:/Program Files/Common Files/OFX/Plugins")
else()
  set(OFX_ARCH_DIR "Linux-x86-64")
  set(DEFAULT_OFX_INSTALL_DIR "$ENV{HOME}/.OFX/Plugins")
endif()

set(OFX_PLUGIN_INSTALL_DIR "${DEFAULT_OFX_INSTALL_DIR}" CACHE PATH "OFX plugin install directory")
set(OFX_BUNDLE_ROOT "${CMAKE_BINARY_DIR}/${OFX_BUNDLE_NAME}")
set(OFX_CONTENTS_DIR "${OFX_BUNDLE_ROOT}/Contents")
set(OFX_BINARY_DIR "${OFX_CONTENTS_DIR}/${OFX_ARCH_DIR}")

add_library(${OFX_PLUGIN_NAME} MODULE
  src/Diagnostics.cpp
  src/Describe.cpp
  src/HostSuites.cpp
  src/LensMap.cpp
  src/ParameterDefinition.cpp
  src/ParameterLogic.cpp
  src/Parameters.cpp
  src/RenderCore.cpp
  src/Render.cpp
  src/RimellAnamorphic.cpp
)

if(APPLE)
  enable_language(OBJCXX)
  target_sources(${OFX_PLUGIN_NAME} PRIVATE src/MetalRender.mm)
  target_link_libraries(${OFX_PLUGIN_NAME} PRIVATE
    "-framework Metal"
    "-framework Foundation"
    "-framework QuartzCore"
  )
endif()

target_include_directories(${OFX_PLUGIN_NAME} PRIVATE
  "${openfx_SOURCE_DIR}/include"
)

target_compile_definitions(${OFX_PLUGIN_NAME} PRIVATE
  RIMELL_ANAMORPHIC_VERSION="${PROJECT_VERSION}"
)

if(MSVC)
  target_compile_options(${OFX_PLUGIN_NAME} PRIVATE /W4 /permissive-)
else()
  target_compile_options(${OFX_PLUGIN_NAME} PRIVATE -Wall -Wextra -Wpedantic)
endif()

set_target_properties(${OFX_PLUGIN_NAME} PROPERTIES
  PREFIX ""
  SUFFIX ".ofx"
  LIBRARY_OUTPUT_DIRECTORY "${OFX_BINARY_DIR}"
  RUNTIME_OUTPUT_DIRECTORY "${OFX_BINARY_DIR}"
  ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
  C_VISIBILITY_PRESET hidden
  CXX_VISIBILITY_PRESET hidden
  VISIBILITY_INLINES_HIDDEN YES
)

if(APPLE)
  set_target_properties(${OFX_PLUGIN_NAME} PROPERTIES
    BUNDLE OFF
  )
endif()

configure_file(
  resources/Info.plist.in
  "${OFX_CONTENTS_DIR}/Info.plist"
  @ONLY
)

install(DIRECTORY "${OFX_BUNDLE_ROOT}"
  DESTINATION "${OFX_PLUGIN_INSTALL_DIR}"
  USE_SOURCE_PERMISSIONS
)

add_custom_target(package-ofx
  COMMAND ${CMAKE_COMMAND} -E tar cfv
          "${CMAKE_BINARY_DIR}/${OFX_PLUGIN_NAME}-${PROJECT_VERSION}.tar.gz"
          --format=gnutar
          "${OFX_BUNDLE_NAME}"
  WORKING_DIRECTORY "${CMAKE_BINARY_DIR}"
  DEPENDS ${OFX_PLUGIN_NAME}
  COMMENT "Packaging ${OFX_BUNDLE_NAME}"
)

add_executable(lens-map-sanity
  tests/lens_map_sanity.cpp
  src/LensMap.cpp
  src/ParameterLogic.cpp
)

target_include_directories(lens-map-sanity PRIVATE
  "${CMAKE_CURRENT_SOURCE_DIR}/src"
  "${openfx_SOURCE_DIR}/include"
)

target_compile_options(lens-map-sanity PRIVATE
  $<$<CXX_COMPILER_ID:MSVC>:/W4 /permissive->
  $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-Wall -Wextra -Wpedantic>
)

add_executable(render-sanity
  tests/render_sanity.cpp
  src/ParameterLogic.cpp
  src/RenderCore.cpp
)

target_include_directories(render-sanity PRIVATE
  "${CMAKE_CURRENT_SOURCE_DIR}/src"
  "${openfx_SOURCE_DIR}/include"
)

target_compile_options(render-sanity PRIVATE
  $<$<CXX_COMPILER_ID:MSVC>:/W4 /permissive->
  $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-Wall -Wextra -Wpedantic>
)

cmake_minimum_required (VERSION 3.0 FATAL_ERROR)
project (MWE CXX C)

find_program(CMAKE_CXX_COMPILER NAMES $ENV{CXX} g++ PATHS ENV PATH NO_DEFAULT_PATH)

if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Intel")
  # require at least intel 17.0
  if (CMAKE_CXX_COMPILER_VERSION VERSION_LESS 17.0)
    message(FATAL_ERROR "Intel version must be at least 17.0")
  endif()
elseif("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
  # require at least 5.2
  if (CMAKE_CXX_COMPILER_VERSION VERSION_LESS 5.2)
    message(FATAL_ERROR "GCC version must be at least 5.2")
  endif()
else()
  message(WARNING "Using untested compiler")
endif()

find_package(HPX REQUIRED)

add_executable(
  DG_HYPER_SWE_HPX
  ${PROJECT_SOURCE_DIR}/src/migrate_component.cpp
)

hpx_setup_target(
  DG_HYPER_SWE_HPX
  COMPONENT_DEPENDENCIES iostreams
)

target_include_directories(DG_HYPER_SWE_HPX PRIVATE ${HPX_INCLUDE_DIRS})
target_link_libraries(DG_HYPER_SWE_HPX ${HPX_LIBRARIES})



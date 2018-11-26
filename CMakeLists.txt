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

set(CMAKE_VERBOSE_MAKEFILE ON)

add_definitions(-g -Wall -std=c++14)

##################################################################
# inheritance_2_classes_concrete_simple
add_executable(
  inheritance_2_classes_concrete_simple
  ${PROJECT_SOURCE_DIR}/src/inheritance_2_classes_concrete_simple.cpp
)

hpx_setup_target(
  inheritance_2_classes_concrete_simple
  COMPONENT_DEPENDENCIES iostreams
)

target_include_directories(inheritance_2_classes_concrete_simple PRIVATE ${HPX_INCLUDE_DIRS})
target_link_libraries(inheritance_2_classes_concrete_simple ${HPX_LIBRARIES})

##################################################################
# migrate_polymorphic_component
add_executable(
  migrate_polymorphic_component
  ${PROJECT_SOURCE_DIR}/src/migrate_polymorphic_component.cpp
)

hpx_setup_target(
  migrate_polymorphic_component
  COMPONENT_DEPENDENCIES iostreams
)

target_include_directories(migrate_polymorphic_component PRIVATE ${HPX_INCLUDE_DIRS})
target_link_libraries(migrate_polymorphic_component ${HPX_LIBRARIES})



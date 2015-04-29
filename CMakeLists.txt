project(ciss)
cmake_minimum_required(VERSION 2.8)
set(CMAKE_C_FLAGS "-std=c11 -Wall -pedantic -O3")
set(CMAKE_C_FLAGS_DEBUG "-std=c11 -Wall -pedantic -O0 -g")

if("${CMAKE_C_COMPILER_ID}" STREQUAL "Clang")
  message(STATUS "Using clang compiler")
  set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -Wdocumentation")
elseif("${CMAKE_C_COMPILER_ID}" STREQUAL "GNU")
  message(STATUS "Using GNU compiler")
  set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -Wextra")
endif()

# Periscop projets
find_package(osl REQUIRED)
find_package(candl REQUIRED)

# GMP
find_library(GMP_LIBRARY gmp)
find_path(GMP_INCLUDE_DIR gmp.h)

# isl
find_library(ISL_LIBRARY isl)
find_path(ISL_INCLUDE_DIR isl/ctx.h)

include_directories(${OSL_INCLUDE_DIR})
include_directories(${CANDL_INCLUDE_DIR})
include_directories(${GMP_INCLUDE_DIR})
include_directories(${ISL_INCLUDE_DIR})

aux_source_directory(. SRC_LIST)
add_executable(${PROJECT_NAME} ${SRC_LIST})

target_link_libraries(${PROJECT_NAME} ${OSL_LIBRARY})
target_link_libraries(${PROJECT_NAME} ${CANDL_LIBRARY})
target_link_libraries(${PROJECT_NAME} ${GMP_LIBRARY})
target_link_libraries(${PROJECT_NAME} ${ISL_LIBRARY})

aux_source_directory(test TEST_LIST)
add_executable("${PROJECT_NAME}_test" ${TEST_LIST} convert.c)
target_link_libraries("${PROJECT_NAME}_test" ${OSL_LIBRARY})
target_link_libraries("${PROJECT_NAME}_test" ${CANDL_LIBRARY})
target_link_libraries("${PROJECT_NAME}_test" ${GMP_LIBRARY})
target_link_libraries("${PROJECT_NAME}_test" ${ISL_LIBRARY})

cmake_minimum_required(VERSION 3.20)

project(ttest VERSION 0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED TRUE)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(Trilinos REQUIRED COMPONENTS TeuchosCore TeuchosComm)

if(NOT "${TeuchosComm_MPI_EXEC}" STREQUAL "" OR "MPI" IN_LIST
    TeuchosComm_TPL_LIST)
  if(Teuchos_MPI_CXX_COMPILER)
    set(MPI_CXX_COMPILER "${Teuchos_MPI_CXX_COMPILER}")
  endif()
  find_package(MPI REQUIRED COMPONENTS CXX)
endif()

if(PROJECT_IS_TOP_LEVEL)
  set(__enable_test ON)
else()
  set(__enable_test OFF)
endif()
option(TTEST_ENABLE_TESTING "Enable tests for ttest" ${__enable_test})
if(NOT PROJECT_IS_TOP_LEVEL)
  mark_as_advanced(TTEST_ENABLE_TESTING)
endif()

if(PROJECT_IS_TOP_LEVEL)
  set(__enable_install ON)
else()
  set(__enable_install OFF)
endif()
option(TTEST_INSTALL "Enable install for ttest" ${__enable_install})
if(NOT PROJECT_IS_TOP_LEVEL)
  mark_as_advanced(TTEST_INSTALL)
endif()

add_subdirectory(src)
add_subdirectory(test)

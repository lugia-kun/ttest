if(NOT TTEST_ENABLE_TESTING)
  return()
endif()

enable_testing()

add_executable(ttest_test main.cxx)
target_link_libraries(ttest_test ttest)

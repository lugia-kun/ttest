
add_library(ttest STATIC ttest.cxx)
add_library(TTest::ttest ALIAS ttest)

if(TARGET TeuchosComm::teuchoscomm)
  target_link_libraries(ttest TeuchosCore::teuchoscore TeuchosComm::teuchoscomm)
else()
  target_link_libraries(ttest ${TeuchosCore_LIBRARIES} ${TeuchosComm_LIBRARIES}
    ${TeuchosCore_TPL_LIBRARIES} ${TeuchosComm_TPL_LIBRARIES})
  target_include_directories(ttest
    PUBLIC ${TeuchosCore_INCLUDE_DIRS} ${TeuchosComm_INCLUDE_DIRS})
endif()

if(NOT "${TeuchosComm_MPI_EXEC}" STREQUAL "" OR "MPI" IN_LIST
    TeuchosComm_TPL_LIST)
  target_link_libraries(ttest MPI::MPI_CXX)
endif()

target_include_directories(ttest
  INTERFACE $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
            $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>)

if(TTEST_INSTALL)
  install(TARGETS ttest EXPORT TTest RUNTIME DESTINATION bin)

  install(EXPORT TTest
    NAMESPACE TTest::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/ttest)
endif()

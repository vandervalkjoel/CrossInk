add_executable(ClockModelTest
  ClockModelTest.cpp
  ${REPO_ROOT}/src/clock/ClockModel.cpp
)

target_include_directories(ClockModelTest PRIVATE ../../src)

target_link_libraries(ClockModelTest PRIVATE
  crosspoint_test_common
  GTest::gtest_main
)

gtest_discover_tests(ClockModelTest)

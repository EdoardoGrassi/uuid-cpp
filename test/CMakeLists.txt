# get GoogleTest suite
include(FetchContent)
FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG release-1.10.0
)
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
set(gtest_build_gmock OFF)
FetchContent_MakeAvailable(googletest)

include(GoogleTest)

add_executable(${PROJECT_NAME}_tests "uuid_tests.cpp")
target_link_libraries(${PROJECT_NAME}_tests PRIVATE uuid-cpp gtest_main)
gtest_discover_tests(${PROJECT_NAME}_tests)
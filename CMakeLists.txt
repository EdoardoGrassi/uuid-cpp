cmake_minimum_required(VERSION 3.8)
project("uuid-cpp")

set(UUID_NAMESPACE "uuid" CACHE STRING "Main namespace of the library")
option(UUID_BUILD_TESTS "Build the unit tests" ON)

add_library(${PROJECT_NAME} STATIC
    "src/uuid.cpp"
    "src/uuid_engine.cpp"
)

target_compile_features(${PROJECT_NAME} PUBLIC cxx_std_20)
target_compile_definitions(${PROJECT_NAME} PUBLIC UUID_NAMESPACE_HPP=${UUID_NAMESPACE})

target_include_directories(${PROJECT_NAME} PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

if(WIN32)
    find_library(IPHLPAPI "iphlpapi" REQUIRED)
    #message(STATUS "iphlpaipi found at ${IPHLPAPI}")
    target_link_libraries(${PROJECT_NAME} PRIVATE ${IPHLPAPI})
endif()

install(TARGETS ${PROJECT_NAME} EXPORT ${PROJECT_NAME}-targets)


# tests
if (UUID_BUILD_TESTS)
    enable_testing()
    add_subdirectory("test")
endif()

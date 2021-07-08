cmake_minimum_required(VERSION 3.8 FATAL_ERROR)
project("uuid-cpp" LANGUAGES CXX)

#set(UUID_NAMESPACE "uuid" CACHE STRING "Main namespace of the library")
option(UUID_CPP_BUILD_TESTS "Build the unit tests" ON)

add_library(uuid-cpp STATIC
    "src/uuid_core.cpp"
    "src/uuid_engine.cpp"
 )

target_compile_features(uuid-cpp PUBLIC cxx_std_20)
#target_compile_definitions(uuid-cpp PUBLIC UUID_NAMESPACE_HPP=${UUID_NAMESPACE})

target_include_directories(uuid-cpp PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

if(WIN32)
    find_library(IPHLPAPI "iphlpapi" REQUIRED)
    #message(STATUS "iphlpaipi found at ${IPHLPAPI}")
    target_link_libraries(uuid-cpp PRIVATE ${IPHLPAPI})

elseif(APPLE)
    
elseif(UNIX)
    find_package(libuuid REQUIRED)
    target_link_libraries(uuid-cpp PRIVATE ${libuuid_LIBRARIES})
endif()

install(TARGETS uuid-cpp EXPORT ${PROJECT_NAME}-targets)
export(EXPORT ${PROJECT_NAME}-targets NAMESPACE uuid-cpp::)

add_executable(main "test/main.cpp")
target_link_libraries(main PRIVATE uuid-cpp)

# tests
if (UUID_CPP_BUILD_TESTS)
    enable_testing()
    add_subdirectory("test")
endif()

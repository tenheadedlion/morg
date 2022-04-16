cmake_minimum_required(VERSION 3.9)
project(morg)
set(CMAKE_CXX_STANDARD 20)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)

include(FetchContent)
FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/e2239ee6043f73722e7aa812a459f54a28552929.zip
)
# For Windows: Prevent overriding the parent project's compiler/linker settings
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

FetchContent_Declare(
  concurrentqueue
  URL https://github.com/cameron314/concurrentqueue/archive/22c78daf65d2c8cce9399a29171676054aa98807.zip
)
FetchContent_MakeAvailable(concurrentqueue)

set(MORG_INCLUDE_DIR ${PROJECT_SOURCE_DIR}/include)
set(MORG_LIB morg_lib)
add_library(${MORG_LIB} INTERFACE)
target_include_directories(${MORG_LIB} INTERFACE
  ${MORG_INCLUDE_DIR}
)

enable_testing()
file(GLOB_RECURSE MORG_TESTS_FILES ${CMAKE_CURRENT_SOURCE_DIR}/tests/*.cpp)
set(MORG_TEST morg_test)
add_executable(${MORG_TEST} 
    ${MORG_TESTS_FILES}
    #${CMAKE_CURRENT_SOURCE_DIR}/tests/main.cpp
    )
target_link_libraries(${MORG_TEST} ${MORG_LIB} gtest_main concurrentqueue)

include(GoogleTest)
gtest_discover_tests(${MORG_TEST})


set(MORG_EXEC morg)
add_executable(${MORG_EXEC} 
    ${CMAKE_CURRENT_SOURCE_DIR}/morg.cpp)
# https://stackoverflow.com/a/29871891/9639179
set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)
target_link_libraries(${MORG_EXEC} ${MORG_LIB} concurrentqueue Threads::Threads)



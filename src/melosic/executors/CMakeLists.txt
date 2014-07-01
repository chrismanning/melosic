set(EXECUTORS_HEADERS)
set(EXECUTORS_HEADERS ${EXECUTORS_HEADERS} inline_executor.hpp)
set(EXECUTORS_HEADERS ${EXECUTORS_HEADERS} loop_executor.hpp)
set(EXECUTORS_HEADERS ${EXECUTORS_HEADERS} thread_pool.hpp)

add_library(executors SHARED ${EXECUTORS_HEADERS} default_executor.cpp)

add_subdirectory(test)

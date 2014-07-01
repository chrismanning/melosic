set(EXECUTORS_HEADERS)
set(EXECUTORS_HEADERS ${EXECUTORS_HEADERS} inline_executor.hpp)
set(EXECUTORS_HEADERS ${EXECUTORS_HEADERS} loop_executor.hpp)
set(EXECUTORS_HEADERS ${EXECUTORS_HEADERS} thread_pool.hpp)

add_library(executors SHARED ${EXECUTORS_HEADERS} default_executor.cpp)
set_target_properties(executors PROPERTIES COMPILE_DEFINITIONS EXECUTORS_EXPORTS=executors_EXPORTS)

add_subdirectory(test)

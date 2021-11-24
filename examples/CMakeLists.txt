set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

add_executable(future-int future-int.cpp)
target_link_libraries(future-int aco::thread_pool Threads::Threads)

add_executable(future-void future-void.cpp)
target_link_libraries(future-void aco::thread_pool Threads::Threads)

add_executable(push push.cpp)
target_link_libraries(push aco::thread_pool Threads::Threads)

# 项目名称
project(sunnet)
# CMake最低版本要求
cmake_minimum_required(VERSION 2.8)
# 头文件目录
include_directories(include)
# 查找./src目录下的所有源文件，保存到DIR_SRCS变量中
aux_source_directory(./src DIR_SRCS)
# 使用C++11
add_definitions(-std=c++11)
# 指定生成目标文件
add_executable(sunnet ${DIR_SRCS})

# 库文件 
# find_package(Threads) 
# target_link_libraries(sunnet ${cmake_THREAD_LIBS_INIT})


# set_target_properties(${TARGET} PROPERTIES
# COMPILE_FLAGS "-pthread"
# LINK_FLAGS "-pthread")

# add_library(sunnet${src})
set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)
target_link_libraries(sunnet PUBLIC Threads::Threads)
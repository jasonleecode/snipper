cmake_minimum_required(VERSION 3.16)
project(snipper VERSION 1.0.0 LANGUAGES CXX)

# 设置C++标准
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 设置编译选项
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -O2")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -g -O0")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O3 -DNDEBUG")

# 添加runtime子目录
add_subdirectory(runtime)

# 包含目录
include_directories(${CMAKE_CURRENT_SOURCE_DIR})

# 主程序源文件
set(SNIPPER_SOURCES
    Snipper.cpp
)

# 编辑器源文件
set(EDITOR_SOURCES
    editor/groot.cpp
)

# 创建主程序可执行文件
add_executable(snipper ${SNIPPER_SOURCES})

# 创建编辑器可执行文件
add_executable(groot ${EDITOR_SOURCES})

# 链接库
target_link_libraries(snipper 
    PRIVATE 
    snipper_runtime
    pthread
)

target_link_libraries(groot 
    PRIVATE 
    snipper_runtime
)

# 设置输出目录
set_target_properties(snipper PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)

set_target_properties(groot PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)

# 安装规则
install(TARGETS snipper groot
    RUNTIME DESTINATION bin
)

# 安装配置文件
install(FILES task.json
    DESTINATION share/snipper
)

# 打印配置信息
message(STATUS "Snipper configuration:")
message(STATUS "  Version: ${PROJECT_VERSION}")
message(STATUS "  C++ Standard: ${CMAKE_CXX_STANDARD}")
message(STATUS "  Build Type: ${CMAKE_BUILD_TYPE}")
message(STATUS "  Install Prefix: ${CMAKE_INSTALL_PREFIX}")

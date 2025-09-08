# Runtime库的CMake配置
cmake_minimum_required(VERSION 3.16)

# 设置库名称
set(LIBRARY_NAME "snipper_runtime")

# 包含nlohmann/json
include(FetchContent)
FetchContent_Declare(
    json
    GIT_REPOSITORY https://github.com/nlohmann/json.git
    GIT_TAG v3.11.2
)
FetchContent_MakeAvailable(json)

# 设置C++标准
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 设置编译选项
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -O0 -Wall -Wextra")
else()
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O2 -Wall -Wextra")
endif()

# 包含头文件目录
include_directories(${CMAKE_CURRENT_SOURCE_DIR})

# 收集所有源文件
set(RUNTIME_SOURCES
    runtime.cpp
    core/context.cpp
    core/rule.cpp
    core/engine.cpp
    condition/condition_evaluator.cpp
    condition/operators.cpp
    expression/expression.cpp
    priority/priority_manager.cpp
    behavior_tree/bt_node.cpp
    behavior_tree/bt_parser.cpp
    behavior_tree/bt_executor.cpp
    behavior_tree/bt_manager.cpp
    scheduler/cron_parser.cpp
    scheduler/timer.cpp
    scheduler/frequency_limiter.cpp
    scheduler/resource_monitor.cpp
    scheduler/scheduler.cpp
    persistence/storage_factory.cpp
    persistence/memory_storage.cpp
    persistence/file_storage.cpp
    persistence/history_recorder.cpp
    persistence/rule_state_manager.cpp
    persistence/config_hot_reload.cpp
    persistence/persistence.cpp
)

# 创建静态库
add_library(${LIBRARY_NAME} STATIC ${RUNTIME_SOURCES})

# 链接nlohmann/json
target_link_libraries(${LIBRARY_NAME} nlohmann_json::nlohmann_json)

# 设置库的包含目录
target_include_directories(${LIBRARY_NAME} PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
    $<INSTALL_INTERFACE:include>
)

# 设置库的编译特性
target_compile_features(${LIBRARY_NAME} PUBLIC cxx_std_17)

# 设置输出目录
set_target_properties(${LIBRARY_NAME} PROPERTIES
    ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib
    LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)

# 简单的安装配置
install(TARGETS ${LIBRARY_NAME}
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
    RUNTIME DESTINATION bin
)

# 打印配置信息
message(STATUS "Runtime library configuration:")
message(STATUS "  Library name: ${LIBRARY_NAME}")
message(STATUS "  Source files: ${RUNTIME_SOURCES}")
message(STATUS "  Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "  C++ standard: ${CMAKE_CXX_STANDARD}")

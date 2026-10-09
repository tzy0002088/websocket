# 公共测试代码: 测试客户端封装、python server 进程管理等
# 编译一次, 供所有测试用例链接(而非每个测试二进制各自编译一份)
file(GLOB SOURCES *.cpp)

add_library(ws_test_common STATIC ${SOURCES})
target_include_directories(ws_test_common PUBLIC .)
target_compile_definitions(ws_test_common PUBLIC WS_SOURCE_DIR="${CMAKE_SOURCE_DIR}")
target_link_libraries(ws_test_common PUBLIC websocket pthread)

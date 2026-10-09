# benchmark 库自带约 70 个微基准测试, 与本项目测试无关,
# 关闭其测试注册, 保持 ctest 只包含本项目自己的测试
set(BENCHMARK_ENABLE_TESTING OFF)

add_subdirectory(googletest EXCLUDE_FROM_ALL)
add_subdirectory(benchmark EXCLUDE_FROM_ALL)

# tests/CMakeLists.txt
#
# 测试总入口
#
# 设计原则：
# - 每个被测层级对应一个子目录（core/, ui/）
# - 使用 CTest 注册，可通过 ctest 命令统一运行
# - 测试可执行文件命名：tst_<模块名>
# - 对齐 MindPath 测试架构

enable_testing()

add_subdirectory(core)
# 未来扩展：
# add_subdirectory(ui)

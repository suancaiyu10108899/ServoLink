# tests/core/CMakeLists.txt
#
# Core 层单元测试
#
# 测试 3 个核心模块：
# 1. tst_kinematic_solver — 运动学求解器
# 2. tst_transmission_analyzer — 传动分析器
# 3. tst_parameter_optimizer — 参数优化器

# ── 求解器测试 ──
add_executable(tst_kinematic_solver
    tst_kinematic_solver.cpp
)
target_link_libraries(tst_kinematic_solver PRIVATE servolink_core Qt6::Core Qt6::Test)
add_test(NAME tst_kinematic_solver COMMAND tst_kinematic_solver)

# ── 传动分析测试 ──
add_executable(tst_transmission_analyzer
    tst_transmission_analyzer.cpp
)
target_link_libraries(tst_transmission_analyzer PRIVATE servolink_core Qt6::Core Qt6::Test)
add_test(NAME tst_transmission_analyzer COMMAND tst_transmission_analyzer)

# ── 优化器测试 ──
add_executable(tst_parameter_optimizer
    tst_parameter_optimizer.cpp
)
target_link_libraries(tst_parameter_optimizer PRIVATE servolink_core Qt6::Core Qt6::Test)
add_test(NAME tst_parameter_optimizer COMMAND tst_parameter_optimizer)

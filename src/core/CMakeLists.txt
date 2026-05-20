# src/core/CMakeLists.txt
#
# Core 层：机构学领域模型、运动学求解器、参数优化器
#
# 设计原则：
# - 纯 C++ 计算逻辑，不依赖 Qt Widgets
# - 仅依赖 Qt::Core（QString、QVector 等容器）
# - 值类型主导，避免 QObject 继承

add_library(servolink_core STATIC
    LinkageParams.h
    MechanismState.h
    LinkageAnalysis.h
    KinematicSolver.h
    KinematicSolver.cpp
    TransmissionAnalyzer.h
    TransmissionAnalyzer.cpp
    ParameterOptimizer.h
    ParameterOptimizer.cpp
    UnitSystem.h
    OperationResult.h
)

target_include_directories(servolink_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(servolink_core PUBLIC Qt6::Core)

# src/ui/CMakeLists.txt
#
# UI 层：Qt Widgets 界面组件
#
# 设计原则：
# - 依赖 core 层接口，不直接依赖具体实现
# - 信号槽用于用户交互 → 业务逻辑转发
# - 样式集中管理（StyleConstants.h）

add_library(servolink_ui STATIC
    MainWindow.h
    MainWindow.cpp
    KinematicsView.h
    KinematicsView.cpp
    CurvePlot.h
    CurvePlot.cpp
    OptimizeDialog.h
    OptimizeDialog.cpp
    ReverseDialog.h
    ReverseDialog.cpp
    StyleConstants.h
)

target_include_directories(servolink_ui PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(servolink_ui PUBLIC servolink_core servolink_storage Qt6::Widgets)

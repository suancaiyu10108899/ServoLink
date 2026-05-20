# src/app/CMakeLists.txt
#
# 应用入口

add_executable(ServoLink
    main.cpp
)

target_link_libraries(ServoLink PRIVATE servolink_ui servolink_storage servolink_core Qt6::Widgets)

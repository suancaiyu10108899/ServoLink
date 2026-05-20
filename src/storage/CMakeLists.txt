# src/storage/CMakeLists.txt
#
# 持久化层：JSON 文件存储后端
#
# 设计原则：
# - 只依赖 core 层的领域模型和存储接口
# - 不依赖 ui 层

add_library(servolink_storage STATIC
    IStorageBackend.h
    JsonStorageBackend.h
    JsonStorageBackend.cpp
    ProjectFile.h
)

target_include_directories(servolink_storage PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(servolink_storage PUBLIC servolink_core)

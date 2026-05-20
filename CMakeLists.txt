cmake_minimum_required(VERSION 3.25)

project(ServoLink
    VERSION 0.1.0
    DESCRIPTION "ServoLink - Control surface linkage mechanism design tool"
    LANGUAGES CXX
)

# ---- C++ 标准 ----
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)

# ---- CTest ----
include(CTest)

# ---- 查找 Qt ----
find_package(Qt6 REQUIRED COMPONENTS Widgets Test)

# ---- 进入子目录 ----
add_subdirectory(src)
add_subdirectory(tests)

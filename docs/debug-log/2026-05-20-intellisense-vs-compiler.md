# DebugLog — 2026-05-20 — IntelliSense 报错但编译器通过

## 现象
VSCode 中 `#include <QString>` 等 Qt 头文件全部报红：
```
无法打开 源 文件 "QString"
```
包含 StyleConstants.h、MainWindow.h 等所有依赖 Qt 的文件都显示红色波浪线。

但 `cmake --build` 编译**完全通过**（0 warnings），`ctest` 36/36 测试全部通过。

## 排查过程
1. 检查 `CMakePresets.json` — Qt 路径配置正确
2. 检查构建日志 — `cmake --preset` 成功找到 `Qt6::Widgets`、`Qt6::Core`
3. 检查 Ninja 编译输出 — 所有 `.cpp` 无编译错误
4. 结论：实际编译正常，问题在 IDE 侧

## 根因

VSCode 的 C++ IntelliSense（基于 `compile_commands.json`）需要知道 `-I D:\Qt\6.11.0\...` 等头文件路径才能解析 `#include`。

如果从未运行过 `cmake --preset`（或 `CMakePresets.json` 中已设置 `CMAKE_EXPORT_COMPILE_COMMANDS: ON` 但 configure 未执行），`build/` 目录下没有 `compile_commands.json`，IntelliSense 就无法定位 Qt 头文件。

## 解决方案

在项目根目录执行一次 CMake configure：

```
cmake --preset win-msvc-debug
```

这会生成 `build/win-msvc-debug/compile_commands.json`。

然后在 VSCode 中按 `Ctrl+Shift+P` → 输入 `Reload Window` → 回车。

（也可以完全关闭 VSCode 再打开。）

## 教训

**IDE 报红 ≠ 编译器报错。**

Qt + CMake 项目中，IntelliSense 依赖 `compile_commands.json` 来获取头文件路径。如果：
- 刚克隆项目
- 刚删除了 build 目录
- 从未运行过 CMake configure

就需要先 `cmake --preset` 一次，然后重新加载 VSCode 窗口。

这是一个典型的"IDE 报错但代码没问题"的场景，遇到时先跑 `cmake --build` 验证——如果编译通过，就不要被红字吓到。

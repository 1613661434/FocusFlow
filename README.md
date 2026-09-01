# FocusFlow

FocusFlow（个人任务规划与专注管理系统）是一款使用 C++17、Qt 6 Widgets
和 SQLite 开发的本地桌面工具。项目对应课程自拟题目：

> 基于 C++/Qt 的个人任务规划与专注管理系统设计与实现

它面向学生、职场人士和自由职业者等普通个人用户，不依赖账号或云服务，
任务、设置与专注记录默认只保存在本机。

## 已实现功能

- 今日概览：待办、今日到期、逾期、今日完成、今日及近 7 天专注时长
- 任务管理：新增、编辑、完成、恢复、软删除、搜索和条件筛选
- 任务属性：项目、分类、重要程度、截止时间和预计用时
- 智能推荐：综合重要程度、截止时间、逾期情况、任务耗时与状态计算优先级
- 项目与分类：新增、编辑、归档项目以及维护彩色分类
- 专注计时：25 分钟默认番茄钟、自定义时长、暂停、继续和提前结束
- 休息周期：短休息、长休息、长休息间隔及自动开始选项
- 任务关联：专注记录可以关联到具体任务
- 提醒：系统托盘、桌面消息、系统默认提示音或自定义音频
- 声音控制：独立的专注/休息声音、音量、最长播放时间和重复次数
- 数据统计：近 7 天专注/完成趋势及近 30 天分类专注占比
- 数据管理：SQLite 一致性备份、安全恢复、任务及专注记录 CSV 导出

## 技术结构

```text
src/
├── app/             主窗口、页面导航、主题和系统托盘
├── data/            SQLite 连接、建表、初始化与恢复
├── models/          任务、项目和计时设置数据模型
├── repositories/    任务、项目、设置、专注记录和统计数据访问
├── services/        计时、优先级、声音和数据备份等业务逻辑
└── views/           仪表盘、任务、项目、计时、统计和设置页面
tests/               Qt Test 自动化测试
docs/                项目计划与架构文档
tools/               构建和 Windows 打包脚本
```

更详细的说明见 [项目计划](docs/PROJECT_PLAN.md) 和
[系统架构](docs/ARCHITECTURE.md)。

## 开发环境

- CMake 3.21 或更高版本
- Qt 6.5 或更高版本，包含 Widgets、Sql、Multimedia、Charts 和 Test
- 支持 C++17 的编译器
- 本机当前验证环境：Qt 6.10.0 + MinGW 13.1 + Ninja

## 构建与测试

在 PowerShell 中执行：

```powershell
cmake -S . -B build -G Ninja `
  -DCMAKE_PREFIX_PATH=D:/Qt/6.10.0/mingw_64 `
  -DCMAKE_MAKE_PROGRAM=D:/Qt/Tools/Ninja/ninja.exe `
  -DCMAKE_CXX_COMPILER=D:/Qt/Tools/mingw1310_64/bin/g++.exe
cmake --build build --parallel 4
ctest --test-dir build --output-on-failure
```

也可以直接运行：

```powershell
.\tools\build.ps1
```

构建完成后，程序位于 `build/FocusFlow.exe`。

## Windows 演示包

```powershell
.\tools\package.ps1
```

脚本会先执行 Release 构建和测试，再调用 `windeployqt` 收集 Qt 运行库，
最终在 `dist/` 下生成带时间戳的独立文件夹和 ZIP 压缩包。

## 数据与恢复

数据库和自定义声音保存在 Qt 的应用数据目录中，确切路径可在
“设置 → 数据管理”中查看。运行数据、构建产物和个人音频均被 `.gitignore`
排除，不会进入 Git。

“从备份恢复”会先生成当前数据库的一致性备份，再暂存所选文件；退出并
重新打开 FocusFlow 后才替换数据库。这样可以避免程序运行时直接覆盖已打开
的 SQLite 文件。

## Git 回档

查看历史：

```powershell
git log --oneline
```

如需查看旧版本，建议先创建临时分支，避免影响当前开发：

```powershell
git switch -c inspect-old-version <提交号>
```

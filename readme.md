# 南开大学 2025 秋季操作系统课程实验代码

## 实验指导手册

- 在线：[2025 年的 ucore 指导书](http://oslab.mobisys.cc/lab2025/_book/index.html)
- 本地：[guide_book](guide_book)

## 环境配置

### 教师推荐的环境配置

- 操作系统环境：[Ubuntu Linux（14.04 及其以后版本）（64 bit）](guide_book/lab0/0_Linux.html)
- 编译器工具：[riscv-gcc 交叉编译器（SiFive 预编译工具链）](guide_book/lab0/3_startdash.html)
- 图形界面编辑器：[gedit（Ubuntu 自带）](guide_book/lab0/softwares.html)
- 命令行编辑器：[nano（轻量级文本编辑器）](guide_book/lab0/softwares.html)
- 高级编辑器：[VSCode（支持虚拟机和 WSL 连接）](guide_book/lab0/softwares.html)
- 代码索引工具：[exuberant-ctags（生成程序语言对象索引）](guide_book/lab0/softwares.html)
- 文件比较工具：[diff & patch（比较文本文件差异）](guide_book/lab0/softwares.html)
- 模拟器：[QEMU（4.1.0 以上版本，支持 riscv32/riscv64 和 OpenSBI）](guide_book/lab0/3_startdash.html)

### 教师推荐的配置方法

- riscv-gcc 编译器安装：[从 SiFive 下载预编译工具链并配置环境变量](guide_book/lab0/3_startdash.html)
- QEMU 模拟器安装：[下载源码编译或使用包管理器安装](guide_book/lab0/3_startdash.html)
- 编辑器配置：[根据环境选择合适的编辑器（gedit/nano/VSCode）](guide_book/lab0/softwares.html)
- 环境变量配置：[设置 RISCV 和 PATH 环境变量](guide_book/lab0/3_startdash.html)
- Linux 基础命令：[熟悉常用 Linux 命令操作](guide_book/lab0/0_Linux.html)

### 当前可行配置（可完成 Lab1）

- **操作系统环境**：Ubuntu 24.04.3 LTS (64 bit) - WSL2 环境
- **编译器工具**：riscv64-unknown-elf-gcc 13.2.0-11ubuntu1+12 (通过 apt 包管理器安装)
- **图形界面编辑器**：未安装 gedit (WSL2 环境不支持)
- **命令行编辑器**：nano (已安装，路径：/usr/bin/nano)
- **高级编辑器**：VSCode (已安装，通过 WSL 连接)
- **代码索引工具**：exuberant-ctags (未安装)
- **文件比较工具**：diff & patch (已安装，路径：/usr/bin/diff, /usr/bin/patch)
- **模拟器**：QEMU 4.1.1 (已安装，支持 riscv32/riscv64 和 OpenSBI)

**配置状态说明**：

- ✅ 核心工具已配置：riscv64 交叉编译器、QEMU 模拟器、nano 编辑器、VSCode
- ❌ 环境变量未配置：RISCV 环境变量未设置
- ❌ 部分工具缺失：gedit (WSL2 不支持)、exuberant-ctags
- ⚠️ 配置方式：使用 Ubuntu 包管理器而非教师推荐的 SiFive 预编译工具链

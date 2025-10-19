<h1 align="center"> 2025年南开大学操作系统实验 </h1>
<p align="center">
<a href="https://cc.nankai.edu.cn/"><img src="https://img.shields.io/badge/NKU-CS-07679f"></a>
<a href="http://oslab.mobisys.cc/"><img src="https://img.shields.io/badge/NKU-OS-86006a"></a>
</p>
<h5 align="center"><em>章壹程，仇科文，杨宇翔 </em></h5>
<p align="center">
  <a href="https://github.com/luyanhexay/NKU-OS-2025Fall-labcode/labcode/lab1">lab1</a> |
  <a href="https://github.com/luyanhexay/NKU-OS-2025Fall-labcode/labcode/lab2">lab2</a>
</p>

## 实验指导手册

- 在线：[2025 年的 ucore 指导书](http://oslab.mobisys.cc/lab2025/_book/index.html)

## 环境配置

### 软件版本
- 操作系统： WSL2 (Ubuntu 24.04)
- 编译器： riscv64-unknown-elf-gcc (riscv64-unknown-elf-gcc (GCC) 15.1.0)
- 仿真器： QEMU (QEMU emulator version 4.1.1)

### 编译器安装

由于WSL版本较高，实验指导书上的多种方法均不可行（或者说难度很大），最终取巧选择编译原理课程的预编译版本作为编译器。可以点[此处](https://nankai.feishu.cn/file/KUBxb8oZfoVFhlxgQVpcrZ2Tnvd?from=from_copylink)进行下载。
具体配置方法较为简单：
```
tar -xzvf riscv-elf-toolchains.tar.gz
cd riscv-elf-toolchains
chmod +x ./init.sh
./init.sh
source ~/.bashrc
```

### 仿真器安装
这个就可以完全按照实验指导书上的步骤：
```
wget https://download.qemu.org/qemu-4.1.1.tar.xz
tar xvJf qemu-4.1.1.tar.xz
cd qemu-4.1.1
./configure --target-list=riscv32-softmmu,riscv64-softmmu
make -j
sudo make install
```
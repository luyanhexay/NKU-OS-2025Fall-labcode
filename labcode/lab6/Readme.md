<h1 align="center"> 南开大学操作系统实验六 </h1>
<p align="center">
<a href="https://cc.nankai.edu.cn/"><img src="https://img.shields.io/badge/NKU-CS-07679f"></a>
<a href="http://oslab.mobisys.cc/"><img src="https://img.shields.io/badge/NKU-OS-86006a"></a>
</p>
<h5 align="center"><em>章壹程，仇科文，杨宇翔 </em></h5>
<p align="center">
  <a href="#练习-0填写已有实验">练习0</a>|
  <a href="#练习-1-理解调度器框架的实现">练习1</a>|
  <a href="#练习-2-实现-round-robin-调度算法需要编码">练习2</a>|
  <a href="#扩展练习-challenge1-实现-stride-scheduling-调度算法需要编码">Challenge1</a>|
  <a href="#扩展练习-challenge2-在-ucore-上实现尽可能多的各种基本调度算法fifo-sjf并设计各种测试用例能够定量地分析出各种调度算法在各种指标上的差异说明调度算法的适用范围">Challenge2</a>|
  <a href="#分工">分工</a>
</p>

## 练习 0：填写已有实验

在实验六中，我们首先将实验 2/3/4/5 的代码搬运并整合到当前代码树中，并根据实验六的调度器框架对进程控制块与时钟中断路径做了必要的更新，从而保证后续调度算法与用户态测试程序能够稳定运行。为了便于验收与现场演示，我们保留了指导手册默认的 `priority` 用户程序作为主要观测点，并通过 `make -C labcode/lab6 grade DEFS+=-DSCHED_POLICY=0` 让练习部分在 RR 调度器下通过自动检查。

## 练习 1: 理解调度器框架的实现（不需要编码）

本练习侧重于理解调度器框架的解耦设计。我们在阅读代码时重点关注了 `struct sched_class` 的五个回调接口如何被 `sched.c` 统一调用，以及 `run_queue` 同时支持链表与斜堆的原因，即不同调度策略对“取下一个进程”的复杂度需求不同。

## 练习 2: 实现 Round Robin 调度算法（需要编码）

我们在 `kern/schedule/default_sched.c` 中补全了 RR 调度器的 `init/enqueue/dequeue/pick_next/proc_tick`，并确保时钟中断路径会驱动 `proc_tick` 递减时间片、触发 `need_resched`，从而让抢占式轮转调度真实生效。该部分在 RR 模式下可通过 `make -C labcode/lab6 grade DEFS+=-DSCHED_POLICY=0` 验证。

## 扩展练习 Challenge 1: 实现 Stride Scheduling 调度算法（需要编码）

在 Challenge 1 中，我们将默认调度策略切换为 Stride，并在 `kern/schedule/default_sched_stride.c` 中补全了 Stride 调度器实现。实现上我们采用斜堆（`skew_heap`）维护就绪队列，使得每次选择 stride 最小的进程不必线性扫描队列；当一个进程被选中运行后，我们按照 `stride += BIG_STRIDE / priority` 更新其 stride 值，从而在足够长的时间尺度上逼近“获得的时间片数量与优先级成正比”的目标。为了避免除零与边界情况，我们对 `priority==0` 的场景做了默认值处理。

为了验证实现，我们使用指导手册中的 `priority` 程序进行现象观测。该程序会在父进程与子进程中设置不同优先级，并在固定时间窗口内统计各子进程的工作量，最后输出一个归一化结果用于对比。我们在 QEMU 中运行 `make -C labcode/lab6 qemu`，可以观察到内核打印 `sched class: stride_scheduler`，并且 `priority` 的 `sched result:` 呈现随优先级递增而递增的比例关系，这与 Stride 的预期一致。

## 扩展练习 Challenge 2 ：在 ucore 上实现尽可能多的各种基本调度算法(FIFO, SJF,...)，并设计各种测试用例，能够定量地分析出各种调度算法在各种指标上的差异，说明调度算法的适用范围

Challenge 2 是一个开放问题，因此我们选择实现两个对比鲜明、且可以在现有框架中较为独立落地的调度算法，并配套一个基准程序做定量对比。具体来说，我们补充实现了一个非抢占式的 SJF（Shortest Job First）调度器和一个带权重的 Lottery 调度器，并在 `kern/schedule/sched.c` 中加入了通过编译宏选择调度器的机制，使得同一份内核可以在不改代码的前提下切换不同调度策略进行对比演示。我们约定 `SCHED_POLICY` 的取值语义为：`0=RR`，`1=Stride`，`2=SJF`，`3=Lottery`。

在 SJF 中，我们将 `proc->lab6_priority` 解释为“预计 CPU burst 长度”，并以“值越小，越先调度”为规则进行选择；由于该策略强调平均周转时间最小化，我们采用非抢占式实现，使得进程只会在主动让出（yield）、阻塞或退出时发生切换。与之相对，Lottery 调度器将 `proc->lab6_priority` 解释为“票数/权重”，并通过随机抽签在就绪队列中选择下一个进程；在时间片耗尽时触发重调度，从而在统计意义上实现按权重分配 CPU 时间。

为了完成定量对比，我们新增了用户态基准程序 `schedbench`，它包含两个实验：其一是“周转时间”实验，构造 1 个长任务与 4 个短任务并统计每个子进程完成时间，从而对比不同调度器的平均周转时间与 makespan；其二是“权重份额”实验，在固定时间窗口内统计不同优先级进程的工作量并输出归一化结果，用于对比 RR/Stride/Lottery 的公平性与权重倾向。由于 SJF 为非抢占式，为避免误导，该程序在 `SCHED_POLICY=2` 时跳过权重份额实验。

现场演示时，我们建议先用 `make -C labcode/lab6 clean` 确保编译参数变更会触发重编译，然后通过如下方式运行并观察输出。下面命令会将默认用户程序切换为 `schedbench`，并通过 `SCHED_POLICY` 选择调度器：

```bash
make -C labcode/lab6 clean
make -C labcode/lab6 qemu DEFS+="-DSCHED_POLICY=0 -DUSER_TEST_PROG=schedbench"   # RR
make -C labcode/lab6 clean
make -C labcode/lab6 qemu DEFS+="-DSCHED_POLICY=1 -DUSER_TEST_PROG=schedbench"   # Stride
make -C labcode/lab6 clean
make -C labcode/lab6 qemu DEFS+="-DSCHED_POLICY=2 -DUSER_TEST_PROG=schedbench"   # SJF
make -C labcode/lab6 clean
make -C labcode/lab6 qemu DEFS+="-DSCHED_POLICY=3 -DUSER_TEST_PROG=schedbench"   # Lottery
```

一次典型输出中，RR 的份额归一化结果接近 `1 1 1 1 1`，Stride 的份额结果随优先级上升而上升，Lottery 的份额结果总体上也体现出“高优先级更易获得 CPU”的趋势但存在随机波动；同时 SJF 的平均周转时间通常小于 RR/Stride，这与其设计目标一致。我们认为，通过同一 workload 在不同调度策略下的指标对比，可以较为直观地说明各算法的适用范围：SJF 更偏向提升短任务体验与平均周转时间，而 Lottery/Stride 更适合表达与实现“权重公平”的资源分配需求。
## 扩展阅读：Linux 的 CFS 调度算法 （感兴趣的同学可以学习并实现，不计入成绩）

## 分工

本实验中，我们完成了练习部分所需的基础代码整合与 RR 调度器实现，并在此基础上实现了 Stride 调度器与 Challenge 2 的两种扩展调度策略及其基准程序。整体工作分工沿用仓库历史中的约定，其中 Challenge 部分由我们重点完成并负责整理现场演示所需的编译运行流程与输出解读。

此外需要说明的是，本机环境缺少 `riscv64-unknown-elf-gdb`，因此我们对 `labcode/lab6/tools/grade.sh` 做了兼容处理，使其在没有交叉 GDB 的情况下仍能完成输出捕获与检查；这一改动不影响在 QEMU 中直接运行 `make qemu` 的观测方式。

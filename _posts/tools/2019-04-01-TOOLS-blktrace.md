---
layout:             post
title:              "blktrace"
date:               2019-04-01 05:30:30 +0800
categories:         [MMU]
excerpt:            blktrace tools.
tags:
  - MMU
---

> [GitHub ASM code: .endm](https://github.com/BiscuitOS/HardStack/tree/master/Language/Assembly/ARM-GNU-Assembly/Instruction/%5B.endm%5D)
>
> Email: BuddyZhang1 <buddy.zhang@aliyun.com>

# 目录

> - [工具原理](#工具原理)
>
> - [工具安装](#工具安装)
>
> - [工具使用](#工具使用)
>
> - [附录](#附录)

--------------------------------------------------------------
<span id="工具原理"></span>

![MMU](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND00000B.jpg)

# 工具原理

blktrace 是一个针对 Linux 内核中块设备 I/O 层的跟踪工具，用来收集磁盘 IO 信息中
当 IO 进行到块设备层（block 层，所以叫 blk trace）时的详细信息（如IO请求提交，入队，
合并，完成等等一些列的信息），是由 Linux 内核块设备层的维护者开发的，目前已经集成到内核
2.6.17 及其之后的内核版本中。通过使用这个工具，使用者可以获取 I/O 请求队列的各种详细的
情况，包括进行读写的进程名称、进程号、执行时间、读写的物理块号、块大小等等，是一个 Linux
下分析I/O相关内容的很好的工具。

透过 blktrace 来观察 io 行为的时候，第一件事情需要选择目标设备，以便分析该设备的 io 行为。
blktrace 分为内核部分和应用部分，应用部分收到我们要捕捉的设备名单，传给内核。内核分布在
block 层的各个 tracepoint 就会开始工作，把相关的数据透过 relayfs 传递到 blktrace
的应用部分，应用部分把这些数据记到磁盘，以便后续分析。

##### blktrace 工作流程

{% highlight bash %}
(1) blktrace 测试的时候，会分配物理机上逻辑 cpu 个数个线程，并且每一个线程绑定
    一个逻辑cpu来收集数据。

(2) blktrace 在 debugfs 挂载的路径（默认是/sys/kernel/debug）下每个线程产生
    一个文件（就有了对应的文件描述符），然后调用 ioctl 函数（携带文件描述符，
    _IOWR(0x12,115, struct blk_user_trace_setup)，& blk_user_trace_setup三
    个参数），产生系统调用将这些东西给内核去调用相应函数来处理，由内核经由 debugfs 文
    件系统往此文件描述符写入数据

(3) blktrace 需要结合 blkparse 来使用，由 blkparse 来解析 blktrace 产生的特定格式的
    二进制数据

(4) blkparse 仅打开 blktrace 产生的文件，从文件里面取数据做展示以及最后做
    per cpu 的统计输出，但 blkparse 中展示的数据状态（如 A，U，Q，详细见下）是
    blkparse 在 t->action & 0xffff 之后自己把数值转换为“A，Q，U之类的状态”来展示的。
{% endhighlight %}

-------------------------------------------------------------
<span id="工具安装"></span>

![MMU](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND00000A.jpg)

本教程安装基于 BiscuitOS 制作的 Linux 5.0 系统，其他平台参照安装。如需要安装基于 BiscuitOS
的 Linux 5.0 开发环境，请参考下面文章：

> [Linux 5.0 arm32 开发环境搭建教程](https://biscuitos.github.io/blog/Linux-5.0-arm32-Usermanual/)

##### 获取源码

首先从 blktrace 的站点获取相应的源码，源码地址如下：

> [blktrack: http://brick.kernel.dk/snaps/](http://brick.kernel.dk/snaps/)

从 blktrack 站点上根据需求下载一个版本，例如本教程中选择下载 "blktrace-1.2.0.tar.gz"。
将下载好的源码压缩包放到 BiscuitOS 项目的 dl 目录下，例如使用如下命令：

{% highlight bash %}
cp ~/Download/blktrace-1.2.0.tar.gz BiscuitOS/dl
{% endhighlight %}

##### 解压源码

由于本教程是基于 BiscuitOS 制作的 Linux 5.0 开发环境，因此参考如下命令进行操作：

{% highlight bash %}
mkdir -p BiscuitOS/output/linux-5.0-arm32/package/blktrace
cp -rf BiscuitOS/dl/blktrace-1.2.0.tar.gz  BiscuitOS/output/linux-5.0-arm32/package/blktrace
cd BiscuitOS/output/linux-5.0-arm32/package/blktrace
tar xf blktrace-1.2.0.tar.gz
cd blktrace-1.2.0
make clean
{% endhighlight %}

##### 编译源码

由于只需要 blktrace 和 blkparse 两个工具，开发者可以参考一下命令进行编译：

{% highlight bash %}
make CC=BiscuitOS/output/linux-5.0-arm32/arm-linux-gnueabi/arm-linux-gnueabi/bin/arm-linux-gnueabi-gcc blktrace blkparse
{% endhighlight %}

这里由于工具运行在 arm32 平台上，所以需要使用交叉编译工具，开发者根据实际情况进行调整。

##### 工具安装

由于本教程是基于 BiscuitOS 制作的 Linux 5.0 开发环境，因此参考如下命令进行行安装：

{% highlight bash %}
cp -rf blktrace blkparse BiscuitOS/output/linux-5.0-arm32/rootfs/rootfs/usr/bin
{% endhighlight %}

##### 更新 rootfs

接着更新 rootfs，并在 BiscuitOS 中使用这个工具，开发者根据实际情况进行更新，BiscuitOS
可以参考如下命令：

{% highlight bash %}
cd BiscuitOS/output/linux-5.0-arm32/
./RunQemuKernel.sh pack
{% endhighlight %}

##### 运行工具


-------------------------------------------------------------
<span id="工具使用"></span>

![MMU](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND00000A.jpg)





-----------------------------------------------

# <span id="附录">附录</span>

> [The GNU Assembler](http://tigcc.ticalc.org/doc/gnuasm.html)
>
> [Debugging on ARM Boot Stage](https://biscuitos.github.io/blog/BOOTASM-debuggingTools/#header)
>
> [BiscuitOS Home](https://biscuitos.github.io/)
>
> [BiscuitOS Driver](https://biscuitos.github.io/blog/BiscuitOS_Catalogue/)
>
> [BiscuitOS Kernel Build](https://biscuitos.github.io/blog/Kernel_Build/)
>
> [Linux Kernel](https://www.kernel.org/)
>
> [Bootlin: Elixir Cross Referencer](https://elixir.bootlin.com/linux/latest/source)
>
> [搭建高效的 Linux 开发环境](https://biscuitos.github.io/blog/Linux-debug-tools/)

## 赞赏一下吧 🙂

![MMU](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/HAB000036.jpg)

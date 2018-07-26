---
layout: post
title:  "Hello BiscuitOS"
date:   2018-07-01 12:43:30 +0800
categories: [All]
excerpt: This is a simple guide to introduce how to running BiscuitOS.
tags:
  - 1st
  - Kernel
  - Linux
---

## 简介

BiscuitOS 是一个基于古老版本 Linux 的发行版，其主要目的就是给开发者提
供一个简单， 易用，有趣的 Linux 调试环境，让开发者专注于代码调试，减少
繁琐的移植和编译问题。 截止目前，BiscuitOS 已经支持 Linux 0.11 到 
Linux 1.0.1 各版本内核。 BiscuitOS 支持内核列表

| 内核版本号         | 版本说明                           |
| ------------------ | -----------------------------------|
| 0.11               | 1991/10/16 Liuns 在网上发布了 Linux 0.11 版本 |  
| 0.12               | 1992/1/5 Linux 开始采用 GPL 权限发布 |
| 0.95.3             |                                      |
| 0.95a              |                                      |
| 0.96.1             |                                      |
| 0.97.1             |                                      |
| 0.98.1             |                                      |
| 0.99.1             |                                      |
| 1.0.1              |                                      |
| 1.0.1.1 minix      |                                      |
| 1.0.1.1 ext2       |                                      |

## Feature

BiscuitOS 作为一个 古老 Linux 发行版，完整还原了早期 Linux 内核，并提供基础的 Rootfs，使其基于 qemu 可以在现代 PC 机上运行并调试。BiscuitOS 是一款专注代码开发使用的发行版.

其以下特点符合多数开发者需求：
  
  * BiscuitOS 是一款基于 Intel X86 的操作系统，其可以运行在 qemu 模拟的 
    X86 环境中，开发者不需要任何硬件平台就可以直接运行 BiscuitOS， 对于
    无硬件环境的开发者较友好。者可以运用 qemu 模拟器实现从操作系统的第一
    行代码调试到最后一行代码。丰富而强大的调试手段是作为内核开发者期待的杀手锏。

  * BiscuitOS 的内核支持 Kbuild 编译环境，完全兼容主线 Linux 内核的 Kbuild
    机制，是实践 Kconfig 语法和内核编译过程研究的最好环境。Kbuild 编译系统
    能自动优化编译过程，让 BiscuitOS 内核的编译时间降低到最小。开发者可以
    使用 make menuconfig 命令对 BiscuitOS 进行自定义定制，生成的内核配置文
    件利于多个开发者之间协同开发。

  * BiscuitOS 的内核支持自定义链接过程，可通过修改链接脚本控制整个链接过
    程，并添加或删除自定义段，实现对内核加载到控制。对于想研究内核链接过
    程的开发者是个不错的实践环境。
    
  * BiscuitOS 提供了多版本的内核，对于想研究某个子系统的发展历史的开发者，
    可以跟随内核版本的发展研究特定子系统的来世今生。

  * BiscuitOS 提供了 debug call 机制，开发者只用编写自己的代码，使用 debug 
    call 机制，就能在内核不同阶段运行你的代码，这个功能利于开发者快速调试
    自己的代码
    
  * BiscuitOS 提供了一套完整的调试机制，从上电开始，开发者就可以使用基于 
    gdb 等调试 工具，逐行进行在线调试。
    
  * BiscuitOS 提供了一套 POSIX system call 的 demo code，开发者可以通过 
    demo code 从用户空间到内核空间调试每一个系统调用。
    
  * BiscuitOS 也提供了一套自定义的 LibC 库， 开发者可以自行创建符合 POSIX 
    的系统调用。
    
  * BiscuitOS 提供了一套通过中断机制，开发者可以通过这套机制使用并调试触发
    每种中断的方法。
    
  * BiscuitOS 包含了一套精简到 VFS 机制，并支持 minix, ext, ext2, msdos 
    等 9 种文件 系统， BiscuitOS 也提供了制作这 9 种 Rootfs 的方法。以便开
    发者了解文件系统，VFS 和 Rootfs 之间的联系。
    
  * BiscuitOS 包含了一套精简到 VM 机制，并支持逻辑地址，虚拟地址，线性地
    址和物理地 址之间的转换代码，同时支持 kmalloc，vmalloc，page 等内存分
    配器。为开发者提供了一套 demo code 以便完整理解 linux 内存机制的运作
    原理。
    
  * BiscuitOS 还提供了调度，驱动等子系统的 demo code，开发者可以通过调试
    demo code， 可以深入理解每种机制的运作原理。

## BiscuitOS 的使用

我为 BiscuitOS 专门开发了一套 Buildroot，用于制作不同版本的 BiscuitOS。
开发者可以通过 github 获得。在使用 BiscuitOS 之前，开发者需要准备一套 
Linux PC，最好是 Ubuntu 16.04 ，在使用 BiscuitOS 请安装必要的开发工具，
本节以 Ubuntu 16.04 为例子进行讲解，其他发行版请参考教程：

安装必要的开发工具，使用如下命令：

{% highlight ruby %}
  sudo apt-get install qemu gcc make gdb git figlet
  sudo apt-get install libncurses5-dev
  sudo apt-get install lib32z1 lib32z1-dev
{% endhighlight %}

*注意！* 如果是第一次使用 git，请按如下配置 git 对应的内容：

{% highlight ruby %}
  git config —global user.name “Your Name”
  git config —global user.email “Your Email"
{% endhighlight %}

安装完必须的工具之后，接着就是下载 BiscuitOS 源码，BiscuitOS 源码采用 
GPL 开源协议，开发者在遵循 GPL 协议下可以免费使用这个项目。
使用如下命令进行远吗下载：

{% highlight ruby %}
  git clone https://github.com/BuddyZhang1/BiscuitOS.git
{% endhighlight %}

正确下载完源码之后，由于 BiscuitOS 基于 Kbuild 构建，开发者可以根据自身
需求配置并编译内核。目前 BiscuitOS 支持的内核配置如下：

| 配置文件                     | 描述                                     |
| ---------------------------- | ---------------------------------------- |
| linux_0_11_defconfig         | 用于编译 linux 0.11 内核版本的 BiscuitOS |
| linux_0_12_defconfig         | 用于编译 linux 0.12 内核版本的 BiscuitOS |
| linux_0_95_3_defconfig       | 用于编译 linux 0.95.3 内核版本的 BiscuitOS |
| linux_0_95a_defconfig        | 用于编译 linux 0.95a 内核版本的 BiscuitOS |
| linux_0_96_1_defconfig       | 用于编译 linux 0.96.1 内核版本的 BiscuitOS |
| linux_0_97_1_defconfig       | 用于编译 linux 0.97.1 内核版本的 BiscuitOS |
| linux_0_98_1_defconfig       | 用于编译 linux 0.98.1 内核版本的 BiscuitOS |
| linux_0_99_1_defconfig       | 用于编译 linux 0.99.1 内核版本的 BiscuitOS |
| linux_1_0_1_defconfig        | 用于编译 linux 1.0.1 内核版本的 BiscuitOS |
| linux_1_0_1_1_minix_defconfig | 用于编译 linux 1.0.1.1 minix-fs 内核版本的 BiscuitOS |
| linux_1_0_1_1_ext2_defconfig  | 用于编译 linux 1.0.1.1 ext2-fs 内核版本的 BiscuitOS |

开发者可以从上面表格中选取一个内核配置文件进行编译，比如使用 
linux_1_0_1_1_ext2_defconfig 配置文件编译 linux 1.0.1.1 版本内核，并且
使用 ext2 作为文件系统的 BiscuitOS，使用如下命令，其他版本编译类似：

{% highlight ruby %}
  cd BiscuitOS/
  make linux_1_0_1_1_ext2_defconfig
  make clean
  make
{% endhighlight %}

成功编译之后，将会在 kernel 目录下生成对于版本的 linux 源码和 BiscuitOS
镜像。开发者可以直接运行该内核版本的系统，使用如下命令：

{% highlight ruby %}
  cd BiscuitOS/kernel/linux_xxx/
  make start
{% endhighlight %}

运行如下图

由于内核源码也是基于 Kbuild 构建的，所以开发者可以根据自己的需求裁剪内核，
使用如下命令：

{% highlight ruby %}
  cd BiscuitOS/kernel/linux_xxx/
  make menuconfig
  make start
{% endhighlight %}

至于其他版本的 BiscuitOS 编译方法，请参考如下 Blog

![Alt text](https://github.com/EmulateSpace/PictureSet/blob/master/github/readme_top.jpg)

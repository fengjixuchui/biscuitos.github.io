---
layout: post
title:  "SerenityOS on BiscuitOS"
date:   2021-06-06 07:24:30 +0800
categories: [Build]
excerpt: SerenityOS.
tags:
  - SerenityOS
---

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND00000L0.PNG)

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI100100.png)

#### 目录

> - [SerenityOS 介绍](#A)
>
> - [SerentiyOS 部署](#B)
>
> - [SerenityOS 运行](#C)
>
> - [SerenityOS 内核编译](#D)
>
> - [附录](#附录)

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND000100.png)

------------------------------------------

<span id="A"></span>

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND00000G.jpg)

#### SerenityOS 介绍

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH000625.png)

SerenityOS 是我无意间看新闻时候刷到一篇名为 [《我决定辞掉工作，全职开发我的操作系统》](https://blog.csdn.net/coderising/article/details/117433248)，当看我这篇文章之后，他的操作系统让我感到很震撼，而且他将他的操作系统开源了，于是我就想在我的 BiscuitOS 环境中玩一下这个奇迹一般的牛人开发出来的操作系统。我将利用这篇文章介绍 SerenityOS，以及如何在 BiscuitOS 上部署其开发环境。那么让我们一起了解一下 SerenityOS:

> [SerenityOS 官网](https://www.serenityos.org/)
>
> [SerenityOS Github Page](https://github.com/SerenityOS/serenity)

SerenityOS 给我的感觉是他与 Linux 不同，作者用 C++ 编写操作系统，并将系统运行在 GTK 界面上，各种有趣的应用和游戏，大名鼎鼎的 DOOM 也在 SerenityOS 上运行起来了，是不是很 Geek.

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH000626.png)

除了上面的功能外，SerenityOS 在操作系统层面还具有以下特点:

* 抢占式多任务
* 多线程
* 合成窗口服务器
* IPv4 网络支持 ARP, TCP, UDP 和 ICMP
* ext2 文件系统
* 类 Unix 的 libc 和 userland
* POSIX 信号
* 支持管道和 IO 重定向的 Shell
* mmap()
* /proc 文件系统
* 本机 sockets
* 虚拟终端 (with /dev/pts filesystem)
* 事件循环库 (LibCore)
* 高级 GUI 库  (LibGUI)
* 可视化 GUI 设计工具
* PNG 格式支持
* 文本编辑器
* IRC 客户端
* DNS 查询
* 桌面游戏：扫雷和贪吃蛇
* 端口系统
 
![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND000100.png)

------------------------------------------

<span id="B"></span>

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND00000H.jpg)

#### SerenityOS 部署

为了让开发者们能便捷玩上 SerenityOS，BiscuitOS 也添加了对 SerenityOS 的支持，开发者可以专注于 SerenityOS 的使用，部署问题就留给 BiscuitOS. 那么接下来介绍如何部署 SerenityOS.

###### BiscuitOS 硬件准备

由于项目构建基于 Ubuntu，因此需要准备一台运行 Ubuntu 14.04/16.04/18.04 的主机，主机需要保持网络的连通, 推荐使用 Ubuntu 18.04. 安装完 Ubuntu 之后需要安装基础开发软件，请参考:

> - [BiscuitOS 基础开发工具安装指南](https://biscuitos.github.io/blog/Develop_tools)

基础软件安装之后还需要安装 SerenityOS 开发环境所需的工具，请参考如下命令:

{% highlight bash %}
sudo apt install build-essential cmake curl libmpfr-dev libmpc-dev 
sudo apt install libgmp-dev e2fsprogs ninja-build qemu-system-i386 qemu-utils
sudo apt install libgtk-3-dev
{% endhighlight %}

另外，由于 SerenityOS 的编译需要 GCC-11 的支持，因此需要在同一个 Ubuntu 中支持多个 Gcc 版本，请参考如下命令:

{% highlight bash %}
sudo add-apt-repository ppa:ubuntu-toolchain-r/test
sudo apt-get update
sudo apt-get install -y gcc-11 g++-11
sudo update-alternatives --install /usr/bin/gcc gcc /usr/bin/gcc-11 80
sudo update-alternatives --install /usr/bin/g++ g++ /usr/bin/g++-11 80
sudo update-alternatives --config gcc
 * 2            /usr/bin/gcc-11    80        manual mode
sudo update-alternatives --config g++
 * 2            /usr/bin/g++-11    80        manual mode
gcc --version
{% endhighlight %}

###### BiscuitOS 软件准备

开发者准好硬件设备之后，首先在 Ubuntu 上安装相应的开发工具，基础环境搭建完毕之后，开发者从 GitHub 上获取 BiscuitOS 项目源码，使用如下命令:

{% highlight bash %}
git clone https://github.com/BiscuitOS/BiscuitOS.git --depth=1
cd BiscuitOS
{% endhighlight %}

BiscuitOS 源码下载完毕后，使用 BiscuitOS 自动部署功能，部署一个 SerenityOS 的开发部署，使用自动部署功能之后，BiscuitOS 项目会自动下载并部署 SerenityOS 所需的源码和编译工具等，可能需要一些时间，请保持网络的通畅. 开发者参考下面命令进行部署:

{% highlight bash %}
cd BiscuitOS
make SerenityOS_defconfig
make
cd BiscuitOS/output/SerenityOS-on-BiscuitOS
{% endhighlight %}

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH000627.png)

以上便是 SerenityOS 开发环境，其中 SerenityOS 包含了 Kernel、应用、游戏等相关的软件源码，RunSerenityOS-on-BiscuitOS.sh 脚本用于 SerenityOS 的编译、打包和运行.

------------------------------------------

<span id="C"></span>

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND00000T.jpg)

#### SerenityOS 运行

SerenityOS 开发环境部署完毕之后，接下来就是运行 SerenityOS，BiscuitOS 提供了精简的命令用于实现其运行:

{% highlight bash %}
cd BiscuitOS/output/SerenityOS-on-BiscuitOS
./RunSerenityOS-on-BiscuitOS.sh run
{% endhighlight %}

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH000628.png)

------------------------------------------

<span id="D"></span>

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND00000I.jpg)

#### SerenityOS 内核编译

SerenityOS 内核源码位于开发环境的 SerenityOS/Kernel 目录下:

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH000629.png)

开发者可以根据自己的需求修改内核源码，当修改完毕之后，编译内核请参考如下命令:

{% highlight bash %}
cd BiscuitOS/output/SerenityOS-on-BiscuitOS
./RunSerenityOS-on-BiscuitOS.sh build
./RunSerenityOS-on-BiscuitOS.sh run
{% endhighlight %}

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND000100.png)

-----------------------------------------------

#### <span id="Z0">附录</span>

> [BiscuitOS Home](https://biscuitos.github.io/)
>
> [BiscuitOS Blog 2.0](https://biscuitos.github.io/blog/BiscuitOS_Catalogue/)
>
> [Linux Kernel](https://www.kernel.org/)
>
> [Bootlin: Elixir Cross Referencer](https://elixir.bootlin.com/linux/latest/source)
>

#### 捐赠一下吧 🙂

![MMU](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/HAB000036.jpg)

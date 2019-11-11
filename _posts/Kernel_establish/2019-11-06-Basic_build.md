---
layout: post
title:  "XXXXXXX"
date:   2019-11-06 14:45:30 +0800
categories: [Build]
excerpt: Basic.
tags:
  - Linux
---

## 目录

![](https://raw.githubusercontent.com/EmulateSpace/GIFBaseX/master/RPI/RPI000.GIF)

> - [4.1 BiscuitOS 实践项目](A0)
>
> - [4.2 uboot 实践](#B0)
>
> - [4.3 Linux 内核实践](#C0)
>
> - [4.4 Linux 驱动实践](#D0)
>
> - [4.5 Linux rootfs 实践](#E0)
>
> - [4.6 应用程序实践](#F0)
>
> - [4.7 静态库/动态库实践](#G0)
>
> - [4.8 游戏实践](#H0)

-----------------------------------

## <span id="A0">4.1 BiscuitOS 实践项目</span>

巧妇难为无米之炊，Linux 内核学习最困难也最需要的就是内核实践，
内核实践环境的搭建对初学者来说是一道难于逾越的大山，本书与 BiscuitOS
社区合作，采用 BiscuitOS 项目帮助各位读者快速搭建开发环境，
让读者能够讲书中的内容在 Linux 内核上实践，以致学以致用。

BiscuitOS 项目是一个用于快速部署 Linux 0.x、1.x、2.x、3.x、4.x、以及
最新 5.x 内核开发环境，开发者可以使用 BiscuitOS 项目快速实践内核、驱动、
以及应用程序。本节用于介绍 BiscuitOS 项目的部署以及使用 BiscuitOS
项目制作一个可直接运行的 Linux。

开发者首先准备一台 Linux 主机，推荐使用 Ubuntu 16.04/18.04。主机链接
网络的情况下，安装必要的开发工具，读者可以参考下面命令:

{% highlight bash %}
sudo apt-get install -y qemu gcc make gdb git figlet
sudo apt-get install -y libncurses5-dev iasl
sudo apt-get install -y device-tree-compiler
sudo apt-get install -y flex bison libssl-dev libglib2.0-dev
sudo apt-get install -y libfdt-dev libpixman-1-dev
sudo apt-get install -y python pkg-config u-boot-tools intltool xsltproc
sudo apt-get install -y gperf libglib2.0-dev libgirepository1.0-dev
sudo apt-get install -y gobject-introspection
sudo apt-get install -y python2.7-dev python-dev bridge-utils
sudo apt-get install -y uml-utilities net-tools
sudo apt-get install -y libattr1-dev libcap-dev
sudo apt-get install -y kpartx
sudo apt-get install -y debootstrap bsdtar
{% endhighlight %}

安装完毕基础软件之后，读者使用如下命令部署 BiscuitOS 项目:

{% highlight bash %}
git clone https://github.com/BiscuitOS/BiscuitOS.git --depth=1
cd BiscuitOS
{% endhighlight %}

至此，BiscuitOS 项目部署成功。BiscuitOS 项目支持在多种平台,
包括 arm、arm64、riscv 等。读者可以使用 BiscuitOS 搭建一个
用于实践的平台，接下来的几节内容将详细介绍几个重要实践
环境的部署。

-----------------------------------

## <span id="B0">4.2 uboot 实践</span>

在 Linux 内核启动之前需要使用 bootloader 将内核和基础文件加载
到指定位置，并初始化基础的环境之后，Linux 才能真正的运行。不同
的硬件平台使用的 bootloader 有所不同，比如大多数读者比较熟知的
uboot。BiscuitOS 项目支持对主流版本 uboot 的实践，读者可以参考
本节内容进行实践环境的部署与使用。基于 4.1 节中 BiscuitOS 项目
部署，读者只需使用如下命令就能实现 uboot 开发环境的部署，如下:
(以 uboot-2019.1 为例)

{% highlight bash %}
cd BiscuitOS/
make u-boot-2019.01_defconfig
cd BiscuitOS/output/linux-5.0-arm32/u-boot/u-boot/

{% endhighlight %}

执行上面的命令，BiscuitOS 项目会自动部署 uboot 开发环境，
接着对 uboot 进行交叉编译。编译输出如下:


编译完成之后，读者可以使用下面命令运行 uboot，如下:

{% highlight bash %}
cd BiscuitOS/output/linux-5.0-arm32/
./RunBiscuitOS.sh uboot
{% endhighlight %}

uboot 运行如下图:


读者可以参考上面提到的命令，然后对源码进行修改，修改
完毕之后在 BiscuitOS 上运行调试 uboot。

-----------------------------------

## <span id="C0">4.3 Linux 内核实践</span>

BiscuitOS 项目支持多个版本以及多个硬件平台的 Linux 内核，
读者可以参考本节的内容，制作适合自己的 Linux 内核。本节
以制作一个基于 Linux 5.0 内核，并运行于 ARM64 平台上的
基础系统为例，介绍内核实践的基本步骤。

首先，读者根据 4.1 节的内容，准备好了 BiscuitOS 项目，然后
查看 BiscuitOS 项目下的 configs 目录，该目录包含了多种内核
环境部署的配置文件，使用如下命令进行 ARM64 linux 5.0 开发
环境的搭建:

{% highlight bash %}
cd BiscuitOS/
ls configs/
make linux-5.0-arm64_defconfig
make
{% endhighlight %}

执行上面的命令之后，BiscuitOS 会自动部署 Linux 5.0 ARM64 的
开发环境，当上面的命令执行完毕之后，BiscuitOS 会提示相应的
输出信息，读者可以根据上面信息中提示的 README.md 文件进行
内核实践，读者也可以参考下面的命令进行内核实践，如下:

{% highlight bash %}
cd BiscuitOS/output/linux-5.0-aarch
cd linux/linux/
make ARCH=arm64 clean
make ARCH=arm64 defconfig
{% endhighlight %}

如何是第一次编译 Linux，则执行上面的命令，其作用是为 Linux
内核源码设置默认的配置文件，接下来读者可以对内核进行
裁剪，使用如下命令:

{% highlight bash %}
make ARCH=arm64 menuconfig
{% endhighlight %}

内核基于 Kbuild 构建编译系统，开发者可以使用 Kbuild 提供
的机制进行内核配置。配置完毕之后就是编译内核，可以使用
如下命令:

{% highlight bash %}
make ARCH=arm64 CROSS_COMPILE=BiscuitOS/output/linux-5.0-aarch/aarch64-linux-gnu/aarch64-linux-gnu/bin/aarch64-linux-gnu- Image -j4
{% endhighlight %}

再上面命令，CROSS_COMPILE 指向交叉编译工具，BiscuitOS
项目已经自动部署交叉编译工具，只需将 CROSS_COMPILE 
按上述命令即可，参数 Image 表示内核最终编译生成的
二进制文件，"-j4" 代表加速内核编译。当上面的命令执行
完成之后，内核编译就完成了，此时可以使用下面的命令
运行 Linux，如下:

{% highlight bash %}
cd BiscuitOS/output/linux-5.0-aarch
./RunBiscuitOS.sh
{% endhighlight %}







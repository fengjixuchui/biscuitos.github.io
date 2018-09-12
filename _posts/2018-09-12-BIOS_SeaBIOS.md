---
layout: post
title:  "BiscuitOS 运行并调试 SeaBIOS"
date:   2018-09-12 22:53:30 +0800
categories: [BIOS]
excerpt: BiscuitOS 运行并调试 SeaBIOS.
tags:
  - BIOS
  - SeaBIOS
---

## 简介

SeaBIOS 是开源 16bit Intel X86 BIOS 项目，遵循标准 BIOS 调用接口，SeaBIOS 
能够运行在硬件模拟器或原生的 X86 硬件上。

## 基础原理

BIOS 程序是计算机上电后，CPU 第一个开始运行的程序，完全运行在裸机上，用于完
成对系统硬件的初始化，并且为驱动 OS 做好准备。BIOS 程序根据具体的硬件具有很
高的耦合度，基本上不同的硬件架构都会对应不同的 BIOS 程序，所以一般计算机生产
商都会自己开发 BIOS 程序，虽然可能有很多部分是可以共用的，但是每个厂商设计的
计算机硬件架构都会有区别，所以还是有一部分需要自己定制。SeaBIOS 虽然运行在模
拟器上，但是也有自己适配的一套硬件架构，理解 SeaBIOS 适配的硬件架构对于理解 
SeaBIOS 的代码有很重要的作用。

SeaBIOS 也像正常的 BIOS 一样，在虚拟机上电的时候，会被加载到地址空间的 
0xFFFFFFF0 处，并且该处是一条跳转指令，跳转去执行 SeaBIOS 的代码，完成硬件的
初始化，中断服务函数的设置，ACPI 表，SMBIOS 表等的创建，最后引导启动 OS。

SeaBIOS 和正常 BIOS 的区别在于其运行的环境是一个由 Hypervisor 模拟出来的环境，
有很多地方可以简化，主要表现为：

虚拟化环境中对硬件设备进行模拟的对象是硬件设备的功能及对外的接口，并不需要去
模拟硬件设备的内部运行机制和设备的物理特性，所以就可以大大简化一些模拟设备的
配置和初始化，其中包括：

> 芯片配置上，传统的 BIOS 需要对主板上的各个芯片，如 CPU、南桥芯片、北桥芯片
> 等进行各种配置，让其能够正常运行起来。而在虚拟化环境中，这些物理芯片一般都
> 是不存在的，所以那些涉及到芯片内部运行机制、芯片物理特性的配置都将不再需要，
> SeaBIOS 只需要关注和芯片功能相关的配置即可。

> 内存配置上，传统的BIOS都会包含 MRC 代码（内存厂商提供）用于对内存进行初始
> 化，但是在虚拟化环境中，内存已经完全配置好了，所以 SeaBIOS 里面根本找不到
> 内存初始化的代码，最多只是对 e820 表进行配置，或对内存的属性进行更改。

> 总线配置上，计算机上的总线，特别是一些较高速的总线如 PCI，DMI，PCIe，QPI 
> 之类的总线，都会分成物理层、链路层、传输层等多层结构，为了让物理总线能够正
> 常运行起来，传统的 BIOS 都需要对这些总线各个层的寄存器进行配置，对总线的运
> 行模式、运行速率等参数进行调整、训练（training），这也是一个比较耗时的操
> 作。但是在虚拟化环境中，这些总线物理特性上的配置和初始化都可以省去，因为并
> 没有真正的物理总线存在，SeaBIOS 要做的就是对这些总线在软件层面上的接口进行
> 配置，如 PCI 总线的配置空间

从 SeaBIOS 提供给 OS 的各种信息表上看，如 APCI 表、SMBIOS 表、E820 表等，传
统的BIOS 基本上都得靠自己去探索系统的所有信息逐步建立这些信息表，这可能是一
个比较漫长的过程。而在虚拟化环境中，这些表很多并不需要 SeaBIOS 自己去逐步建
立，很多可以是Hypervisor 就可以通过特定的方式将这些表传给 SeaBIOS，SeaBIOS 
只需要在有必要的时候对这些表的某些信息进行更新。

所以，总的来说，可以将 SeaBIOS 看成简化版的 BIOS，主要从上面说的几个方面进
行了简化。

## 实践 SeaBIOS

#### 搭建开发环境

BiscuitOS 最新版本已经支持源码编译 SeaBIOS，并可以使用编译生成的 BIOS 启动 
Linux。开发者可以按照如下步骤在 BiscuitOS 上运行 SeaBIOS。

首先制作搭建 BiscuitOS 系统，请参考如下文档：

[制作基于 Linux 1.0.1.2 ext2-fs 内核 BiscuitOS](https://biscuitos.github.io/blog/Linux1.0.1.2_ext2fs_Usermanual/)

搭建完之后，请安装必要的开发工具，使用如下命令：

{% highlight ruby %}
sudo apt-get install iasl
{% endhighlight %}

#### 编译 SeaBIOS

准备好开发环境之后，开发者配置 BiscuitOS， 以此支持 SeaBIOS，按如下步骤：

首先，在中端中输入如下命令

{% highlight ruby %}
cd BiscuitOS/
make update
make clean
make linux_1_0_1_2_ext2_defconfig
make menuconfig
{% endhighlight %}

由于 BiscuitOS 的内核使用 Kbuild 构建起来的，在执行完 make menuconfig 之后，
系统会弹出内核配置的界面，开发者根据如下步骤进行配置：

![Menuconfig](https://raw.githubusercontent.com/EmulateSpace/PictureSet/master/BiscuitOS/kernel/BIOS000001.png)

选择 **bootloaders**，回车

![Menuconfig1](https://raw.githubusercontent.com/EmulateSpace/PictureSet/master/BiscuitOS/kernel/BIOS000002.png)

选择 **Intel i386 CPU**, 回车

![Menuconfig2](https://raw.githubusercontent.com/EmulateSpace/PictureSet/master/BiscuitOS/kernel/BIOS000003.png)

选择 **Intel i386 BIOS Setup**, 回车

![Menuconfig3](https://raw.githubusercontent.com/EmulateSpace/PictureSet/master/BiscuitOS/kernel/BIOS000004.png)

选择 **SeaBIOS Intel i386**, 回车

![Menuconfig4](https://raw.githubusercontent.com/EmulateSpace/PictureSet/master/BiscuitOS/kernel/BIOS000005.png)

最后选择 **SeaBIOS**，目前版本的 BiscuitOS 对 SeaBIOS 只提供了版本选项，开发
者可以根据自己需要选择一个合适的 SeaBIOS 版本，推荐使用 rel-1.9.3 版本。
SeaBIOS 版本支持列表请看附录。

选择好 SeaBIOS 之后，开发者保存选中的配置，使用如下命令进行编译

{% highlight ruby %}
make
{% endhighlight %}

第一次使用 SeaBIOS，编译系统会从 SeaBIOS 的官网下载源码，根据开发者的网络
环境会花费不同的时间。下载完源码之后，编译系统会编译 SeaBIOS 并将编译生成
的 SeaBIOS 安装到指定的 Linux 源码目录。在本例子中，SeaBIOS 源码编译生成的
镜像 SeaBIOS.bin 会被安装到如下目录：

{% highlight ruby %}
BiscuitOS/kernel/linux_1.0.1.2/SeaBIOS.bin
{% endhighlight %}

#### 运行 SeaBIOS

编译完之后，开发者可以使用如下命令运行 SeaBIOS，

{% highlight ruby %}
cd BiscuitOS/kernel/linux_1.0.1.2/
make start
{% endhighlight %}

运行如下图，SeaBIOS 在中端中输出 BiscuitOS logo

![Menuconfig8](https://raw.githubusercontent.com/EmulateSpace/PictureSet/master/BiscuitOS/kernel/BIOS000006.png)

#### 修改 SeaBIOS 源码

BiscuitOS 的编译体制类似于 Buildroot，所以开发者要修改 SeaBIOS 源码必须按
照如下步骤。例如本例中，SeaBIOS 的版本为 rel-1.9.3，其源码位于

{% highlight ruby %}
BiscuitOS/output/BIOS/SeaBIOS_rel-1.9.3
{% endhighlight %}

开发者可以在此目录下修改任意代码，修改完之后，请参照如下命令生成补丁(本例子
以 SeaBIOS rel-1.9.3 作介绍，其他版本照样)

{% highlight ruby %}
cd BiscuitOS/output/BIOS/SeaBIOS_rel-1.9.3
git status
git add （添加你修改的文件）
git commit -m "(添加你的修改注释)"
git format-patch -n rel-1.9.3
mv 0*.patch  */BiscuitOS/boot/i386/SeaBIOS/patch/rel-1.9.3
cd  BiscuitOS/
make clean
make
{% endhighlight %}

按照上面的修改，编译之后，新的 SeaBIOS.bin 将包含你的修改，参照上一节进行 
SeaBIOS 的运行。

### 附录 A

SeaBIOS 支持版本号：

{% highlight ruby %}
rel-0.1.0
rel-0.1.1
rel-0.1.2
rel-0.1.3
rel-0.2.0
rel-0.2.1
rel-0.2.2
rel-0.2.3
rel-0.3.0
rel-0.4.0
rel-0.4.1
rel-0.4.2
rel-0.5.0
rel-0.5.1
rel-0.6.0
rel-0.6.1
rel-0.6.1.1
rel-0.6.1.2
rel-0.6.1.3
rel-0.6.2
rel-1.10.0
rel-1.10.1
rel-1.10.2
rel-1.10.3
rel-1.11.0
rel-1.11.1
rel-1.11.2
rel-1.6.3
rel-1.6.3.1
rel-1.6.3.2
rel-1.7.0
rel-1.7.1
rel-1.7.2
rel-1.7.2.1
rel-1.7.2.2
rel-1.7.3
rel-1.7.3.1
rel-1.7.3.2
rel-1.7.4
rel-1.7.5
rel-1.7.5-rc1
rel-1.7.5.1
rel-1.7.5.2
rel-1.8.0
rel-1.8.1
rel-1.8.2
rel-1.9.0
rel-1.9.1
rel-1.9.2
rel-1.9.3
{% endhighlight %}

### 附录 B

引用链接

[SeaBIOS 介绍](https://blog.csdn.net/lindahui2008/article/details/80948396)

[SeaBIOS](https://www.coreboot.org/SeaBIOS)

[BiscuitOS](https://biscuitos.github.io/blog/HomePage/)

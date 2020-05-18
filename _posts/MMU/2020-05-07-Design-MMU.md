---
layout: post
title:  "BiscuitOS Design MMU Project"
date:   2020-05-07 08:35:30 +0800
categories: [HW]
excerpt: MMU.
tags:
  - MMU
---

![](https://gitee.com/BiscuitOS/GIFBaseX/raw/master/RPI/GIF000210.gif)

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI100100.png)

#### 目录

> - [项目介绍](#A)
>
> - [项目实现](#B)
>
> - [项目实践](#C)
>
> - [进阶研究](#D)
>
> - [附录/捐赠](#Z0)

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND000100.png)

----------------------------------

<span id="A"></span>

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND00000P.jpg)

#### 项目介绍

"BiscuitOS Design MMU Project" 项目从架构者角度解析如何设计一个简单完整的
内存管理子系统，并实践动手去实现一个真实可用的内存管理子系统. 通过这个
项目，给开发者从架构者角度介绍如何去规划一个内存管理子系统，以及如何对内存
管理器进行选型，并且通过编写一个真实可用的内存管理子系统来实践相关的设计。
最后通过技术推演介绍如何通过这个简单的内存管理子系统如何演变为目前主流的
Linux 内存管理子系统。

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND000100.png)

-------------------------------------------------

#### <span id="A3000000">开发者计划</span>

"BiscuitOS Design MMU Project" 项目是一个开源的项目，欢迎各位开发者
一同进行开发。如果开发者在使用和开发过程中遇到问题，可以在 BiscuitOS 社区
进行讨论，或者提交相应的 Patch 到我的邮箱, 具体联系方式如下:

> - BiscuitOS 社区微信: Zhang514981221
>
> - BiscuitOs 社区邮箱: buddy.zhang@aliyun.com

----------------------------------

<span id="B"></span>

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND00000H.jpg)

#### 项目实现

> - [基础原理](#A0000001)
>
> - [架构内存管理子系统](#A0000002)
>
> - [项目实践环境](#A0000003)
>
> - [项目运行](#A0000004)

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND000100.png)

---------------------------------------

#### <span id="A0000001">基础原理</span>

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI001000.png)

对于 "内存管理子系统" 接触和理解最多的应该是 Linux 内核里面的各种内存分配
器和各种内存管理行为等。回归到最开始的地方，内存管理子系统的出现要解决的问题，
根据计算机架构可以知道，当系统运行之后需要将数据从永久存储介质上拷贝到内存
上缓存，然后 CPU 与内存交换数据进以便完整特定的任务。将研究的重点放在内存
之后会发现一些问题:

{% highlight bash %}
1. 数据从永久存储介质上拷贝到内存后，如何存放?
2. CPU 如何从内存上找到需要的数据?
3. 随着系统运行时间变成，能使用的内存越来越小，但并不是所有的内存都在使用.
{% endhighlight %}

内存遇到的问题远比上面提到多太多，但总结一点就是如果管理内存的分配和回收。
只要内核具有一个用于有效管理内存分配和回收的系统，内核才能稳定长时间的运行.
通过上面的简单分析，可以得到内存管理子系统的基本任务就是进行内存的分配和
回收，至于其他内存行为都是基于内存的分配和回收衍生而来，那么本项目就是用
于介绍如何设计一个带内存分配回收的子系统。在研究内存子系统的实现之前需要
明确几个基本概念:

###### 物理地址

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI001001.png)

CPU 通过地址总线找到指定内存上的每一个位置，这里称内存在这条地址总线上的
地址称为物理地址.

###### 虚拟地址

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI001002.png)

当系统的 MMU 开启后，CPU 会根据自己的位数看到一块从地址 0 到 2^32/2^64 的
地址空间，这块地址空间称为虚拟空间，将这块虚拟地址空间划分为一个字节为内
存块, 将这些内存块的从低地址向高地址编号，那么这个编号就是虚拟地址.

###### MMU

简单的理解 MMU 就是一个硬件设备，用于打开或关闭虚拟地址空间。当打开虚拟地址
空间，MMU 根据指定的页表将一个对虚拟地址的访问自动转换成物理地址的访问，如
下图:

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI001003.png)

MMU 开启的时候，CPU 看到一块虚拟空间，当 CPU 访问虚拟地址之后，MMU 自动
将虚拟地址转换成物理地址. 当 MMU 关闭的时候，CPU 只能看到物理地址空间，
CPU 可以直接访问物理内存。

###### 页表

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI001004.png)

当 MMU 打开之后，CPU 只能看到和访问虚拟地址空间，不能直接访问物理地址空间，
因此需要需要 MMU 将虚拟地址转换成物理地址。MMU 根据页表自动进行虚拟地址到
物理地址的转换. 因此页表就是用来记录虚拟地址到物理地址的转换关系。系统可以
根据需求建立两级、三级、四级或者五级页表。系统能使用多少级页表由 CPU 决定。
但物理使用多少级页表，在 MMU 打开之前应该将设置好的页表存储到指定位置.

###### 物理页/页帧

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI001005.png)

将物理内存划分为 PAGE_SIZE 大小的物理内存块，一个物理内存块称为物理页。
将物理地址开始，将所有的物理空间划分为多个物理页，将物理页从低物理地址
到高物理地址进行编号，该号码就是物理页帧.

###### TLB

TLB 又称为 "块表", TLB 用于存储系统经常使用的虚拟地址和物理地址转换关系。
有了 TLB 之后，CPU 访问虚拟地址时候，首先从 TLB 查找虚拟地址和物理地址的
转换关系，如果找到，那么直接访问物理物理地址; 如果没有找到，那么只能通过
MMU 查页表进行查找.

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND000100.png)

-------------------------------------

#### <span id="A0000002">架构内存管理子系统</span>

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI001006.png)

内存管理子系统最基础的任务就是管理内存的分配和回收。在多数采用 MMU 的系统
中，内存管理的无非就是三种成员之间的关系，这三个成员就是: "虚拟内存"、
"物理内存" 和 "映射关系". 只要处理好三者之间的关系，内存管理子系统就能
正常工作并完成基本任务.

###### 虚拟内存

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI001007.png)

对于虚拟内存，是在 MMU 开启之后，CPU 只能看到 (访问) 一段连续的地址空间，空
间的长度与系统的位数有关，例如 32 位系统中，虚拟空间为 4GB。为了让系统稳定
使用虚拟内存，那么内存管理子系统必须能够管理虚拟内存的分配和使用，以保证
虚拟内存的良性循环使用. 那么内存管理子系统的第一个任务就是使用一个内存管理
器分配和释放虚拟内存. 使用什么的内存分配器管理虚拟内存应该依据一定的条件，
具体条件后面章节讨论。为了构建一个最简单的内存管理子系统，这里可以构建一个
最简单的虚拟内存管理器，其实现如下:

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI001008.png)

其设计思路是将虚拟内存空间划分成固定长度的内存块，然后使用一个 bitmap，bitmap
里面的每个 bit 对应一个虚拟内存块。当虚拟内存初始化阶段，所有虚拟内存还未
使用，将对应的 bitmap 全部 bit 清零。

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI001009.png)

当该虚拟内存管理器开始使用的时候，如果需要分配一定数量的虚拟内存区块，那么
分配器首先计算需要分配的虚拟内存占用了几个虚拟内存块，以此可以知道需要在 
bitmap 中找到连续几个未置位的 bit 的位置，内存管理器在 bitmap 找到满足条件
的位置之后，将对应的 bit 置位后返回 bit 对应的虚拟内存，这样就完成虚拟内存
的分配。

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI001010.png)

当系统使用完虚拟内存之后，需要将虚拟内存返回给虚拟内存分配器，分配器收到
释放的虚拟内存之后，计算需要释放的内存块数量，然后在 bitmap 将对应的 bit
清零。经过上面的步骤完成了虚拟内存分配器的回收工作。

以上便是一个虚拟内存分配器最基本任务的实现过程。这里值得注意的是虚拟内存
管理器只涉及虚拟内存的分配和回收，至于页表映射工作，确切说不属于虚拟内存
管理器的范围, 虚拟内存管理器只需管理好虚拟内存的分配和回收就行，简单有时
就是最好的.

###### 物理内存

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI001001.png)

物理内存就是实际的 RAM 内存，也就是实际存储数据的介质。在 MMU 启用后，CPU
是不能直接访问物理内存，需要通过访问虚拟地址，再通过 MMU 和页表进行转换后
才能访问到物理内存。物理内存也可以抽象成和虚拟内存一样的连续空间，也需要
内存管理器进行物理内存的分配与回收，因此物理内存管理器也是内存管理子系统
中不可获取的一部分。为了保持内存管理子系统最简单，这里采用和虚拟内存分配器
一样的算法，即采用 bitmap 管理所有的物理内存的分配和回收，如下:

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI001011.png)

在系统中，将物理内存划分成固定长度的物理内存块，然后使用一个 bitmap，bitmap
里面的每个 bit 对应一个物理内存块。在系统上电初始化阶段，将 bitmap 中的
所有 bit 清零。

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI001012.png)

当该物理内存管理器开始使用的时候，如果需要分配一定数量的物理内存区块，那么
分配器首先计算需要分配的物理内存占用了几个物理内存块，以此可以知道需要在 
bitmap 中找到连续几个未置位的 bit 的位置，内存管理器在 bitmap 找到满足条件
的位置之后，将对应的 bit 置位后返回 bit 对应的物理内存，这样就完成物理内存
的分配。

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI001013.png)

当系统使用完物理内存之后，需要将物理内存返回给物理内存分配器，分配器收到
释放的物理内存之后，计算需要释放的内存块数量，然后在 bitmap 将对应的 bit
清零。经过上面的步骤完成了虚拟内存分配器的回收工作。

以上便是一个物理内存分配器最基本任务的实现过程。这里值得注意的是物理内存
管理器只涉及虚拟内存的分配和回收，至于页表映射工作，确切说不属于物理内存
管理器的范围, 物理内存管理器只需管理好物理内存的分配和回收就行，简单有时
就是最好的.

###### 映射关系

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI001004.png)

所谓映射关系就是将虚拟地址与物理地址建立联系，以便系统访问虚拟地址时候，
MMU 能够将遵循这个映射关系找到对应的物理地址。通常将建立虚拟地址和物理
地址的映射关系称为建立页表，因此页表就是用来描述虚拟地址转换成物理地址
的逻辑。MMU 启用之前需要准备好页表，MMU 启用之后根据页表自动将虚拟地址
转换成物理地址.

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI001014.png)

映射关系可以是线性映射和随机映射。所谓线性映射就是将整块的虚拟地址和
整块的物理地址建立页表，因此只要其中虚拟地址或者物理地址就可以通过简单
的线性关系就可以找到对应的虚拟地址或物理地址。

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI001015.png)

所谓随机映射就是将虚拟地址与随机的物理地址进行映射。随机映射页包含了动态
映射。

无论使用那种映射都需要建立对应的页表来实现各种映射关系，页表是一种与体系相关
的机制，内核提供了二级页表、三级页表、四级页表和五级页表的软件实现，但实际使
用几级页表还需要看在什么体系上使用. 因此页表的技术细节不做讨论，只做通用页表
介绍.

###### 内存管理子系统架构

通过上面的讨论，已经准备好构建内存管理子系统所需的部件，接下来讨论如何
架构内存管理子系统，并让其工作起来。架构内存管理子系统的时候，准备了一个
虚拟内存分配器和一个物理内存分配器，以及建立虚拟内存到物理内存之间的页表。
有了以上部件，且 MMU 已经开启，接下来一步一步研究如何实现这个内存管理器。

首先，两个内存管理器都是通过 bitmap 进行管理，因此需要为 bitmap 预留一定
的内存空间存储这两个 bitmap，接着是虚拟地址到物理地址转换使用的页表页需要
一定的内存空间进行存储。由于这些数据要一直驻留来内存里，系统可以快速访问
到，因此这里使用的内存空间只能使用固定映射，即页表建立之后就不再改变，
至于使用线性映射还是随机映射的问题，更多是查考两种映射是否会影响到系统
访问内存的效率，如果影响不到，使用哪一种都无所谓，只要确保映射建立之后
就不要改变。这里为了让内存管理子系统足够简单，那么这里采用线性映射，如下图:

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI001017.png)

接下来将剩余的虚拟内核和物理内存分配同样大小的内存块，这里采用大家熟悉的
PAGE_SIZE，这样方便映射，如下图:

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI001018.png)

从上图可以知道，一般情况下，虚拟内存和物理内存是不相等的，为了充分使用
物理内核和虚拟内存，那么就要使用随机映射将虚拟内存与物理内存进行映射，
这里称这个虚拟内存为非线性区.

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI001019.png)

对于页表的规划，可以采用二级页表，也可以采用四级的页表，只不过这些都是
软件部分的页表，实际的硬件页表要与具体的平台有关.

-------------------------------------------

###### <span id="A0000003">项目实践环境</span>

BiscuitOS Design MMU Project 项目基于模块实现。寄主机器是运行
在 ARM 上的 Linux 5.0 系统。项目在 Linux 5.0 上通过 DTS 预留了一块 100MB
的物理内存，然后基于这段物理内存和真实的 TLB/CACHE 和页表建立了上述说描述
的内存管理子系统，DTS 配置如下:

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI001016.png)

从上图可以看出，项目在 DTS 中定义了一个名为 BiscuitOS 的节点，节点的 ram
属性用来描述整个规划的物理内存和虚拟内存。ram 属性指向了 BiscuitOS_memory
节点，节点是 reserved-memory 节点中的子节点，从节点 reg 属性可以知道，项目
从当前系统中预留了一段物理内存，其起始地址是 0x70000000, 长度为 0x6400000.
节点中还定义了各个虚拟内存和物理内存的长度等信息。项目就是依据这个配置预留
了指定的内存。

项目中内存管理相关的体系代码参考了 ARMv7 的实现逻辑，因此 TLB/CACHE 和页表
的实现均参考 ARMv7 手册进行构建，体系相关的资料，请参考:

> - [ARM_Architecture_Reference_Manual](https://gitee.com/BiscuitOS_team/Documentation/blob/master/Datasheet/ARM/ARM_Architecture_Reference_Manual.pdf)
>
> - [ARMv7_architecture_reference_manual](https://gitee.com/BiscuitOS_team/Documentation/blob/master/Datasheet/ARM/ARMv7_architecture_reference_manual.pdf)
>
> - [Cortex-A9_MPcore_Technical_Reference_Manual](https://gitee.com/BiscuitOS_team/Documentation/blob/master/Datasheet/ARM/Cortex-A9_MPcore_Technical_Reference_Manual.pdf)
>
> - [Cortex_A9_Technical_Reference_Manual](https://gitee.com/BiscuitOS_team/Documentation/blob/master/Datasheet/ARM/Cortex_A9_Technical_Reference_Manual.pdf)

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND000100.png)

------------------------------------------------

<span id="C"></span>

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND00000I.jpg)

#### 项目实践

> - [实践准备](#C0000)
>
> - [实践部署](#C0001)
>
> - [实践执行](#C0002)
>
> - [源码开发](#C0005)
>
> - [测试建议](#C0004)

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND000100.png)

--------------------------------------------

#### <span id="C0000">实践准备</span>

本实践是基于 BiscuitOS Linux 5.0 ARM32 环境进行搭建，因此开发者首先
准备实践环境，请查看如下文档进行搭建:

> - [BiscuitOS Linux 5.0 ARM32 环境部署](https://biscuitos.github.io/blog/Linux-5.0-arm32-Usermanual/)

--------------------------------------------

#### <span id="C0001">实践部署</span>

准备好基础开发环境之后，开发者接下来部署项目所需的开发环境。项目准备好
了多个版本，其中包括最原始的开发环境，也包含了功能开发完毕的项目。如果
开发者想实际写代码的话，可以使用最原始的项目。如果开发者想实际调试项目
的话，可以选择功能开发完毕的项目. 开发者可以使用如下命令:

{% highlight bash %}
cd BiscuitOS/
make linux-5.0-arm32_defconfig
make menuconfig
{% endhighlight %}

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI000746.png)

选择并进入 "[\*] Package  --->" 目录。

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI000747.png)

选择并进入 "[\*]   Memory Development History  --->" 目录。

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI001020.png)

选择并进入 "[\*]   BiscuitOS MMU Design  --->" 目录。

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI001021.png)

对于目前支持的版本，"BiscuitOS Memory Design: Base" 对应的是项目处于最基础
的阶段，未包含任何与内存管理子系统相关的代码，只提供了一个基础运行的环境，
如果开发者想跟着教程一行代码一行代码的编写，那么可以选择这个选项.

"BiscuitOS Memory Design: 0.11" 对应的项目已经包含了一个可以运行的内存管理
子系统，如果开发者只想调试或者作为参考调试的话，可以勾选上这个选项。推荐
开发者勾选 "BiscuitOS Memory Design: 0.11" 和 "BiscuitOS Memory Design: Base",
保存并退出。接着执行如下命令:

{% highlight bash %}
make
{% endhighlight %}

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI000750.png)

成功之后将出现上图的内容，接下来开发者执行如下命令以便切换到项目的路径:

{% highlight bash %}
cd BiscuitOS/output/linux-5.0-arm32/package/BiscuitOS_Memory-Base
make download
cd BiscuitOS/output/linux-5.0-arm32/package/BiscuitOS_Memory-0.11
make download
{% endhighlight %}

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI001022.png)

至此源码已经下载完成，开发者可以使用 tree 等工具查看源码:

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI001023.png)

BiscuitOS.dts 文件用于修改主机中的 DTS，以此能够预留足够的物理内存给项目
使用. main.c 函数是驱动入口相关的函数，与本项目没有太大联系，开发者可以忽略.
Makefile 是模块外部编译的相关规则; "init/main.c" 函数包含了 start_kernel 函数，
是整个项目的入口函数，开发者可以从这个函数开始进行开发. 同理下面是 
"BiscuitOS_Memory-0.11" 的源码树:

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI001024.png)

BiscuitOS.dts 文件用于修改主机中的 DTS，以此能够预留足够的物理内存给项目
使用. main.c 函数是驱动入口相关的函数，与本项目没有太大联系，开发者可以忽略.
Makefile 是模块外部编译的相关规则; "init/main.c" 函数包含了 start_kernel 函数，
是整个项目的入口函数，开发者可以从这个函数开始进行开发. arch 目录下面的源码
是与硬件相关的内存处理过程. "mm/BiscuitOS_phys.c" 是 BiscuitOS 物理内存
分配器实现过程, "mm/BiscuitOS_virt.c" 是 BiscuitOS 虚拟内存分配器实现过程.
接下来的内容与 "BiscuitOS_Memory-Base" 为例进行讲解，其他类似.

如果你是第一次使用这个项目，需要修改主机 DTS 的内容。如果不是可以跳到下一节。
开发者参考源码目录里面的 "BiscuitOS.dts" 文件，将文件中描述的内容添加
到系统的 DTS 里面，"BiscuitOS.dts" 里的内容用来从系统中预留 100MB 的物理
内存供项目使用，具体如下:

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI001016.png)

开发者将 "BiscuitOS.dts" 的内容添加到:

{% highlight bash %}
cd BiscuitOS/output/linux-5.0-arm32/linux/linux/arch/arm/boot/dts/vexpress-v2p-ca9.dts
{% endhighlight %}

添加完毕之后，使用如下命令更新 DTS:

{% highlight bash %}
cd BiscuitOS/output/linux-5.0-arm32/package/BiscuitOS_Memory-Base
make kernel
{% endhighlight %}

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI001025.png)

--------------------------------------------

#### <span id="C0002">实践执行</span>

环境部署完毕之后，开发者可以向通用模块一样对源码进行编译和安装使用，使用
如下命令:

{% highlight bash %}
cd BiscuitOS/output/linux-5.0-arm32/package/BiscuitOS_Memory-Base
make
{% endhighlight %}

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI001026.png)

以上就是模块成功编译，接下来将 ko 模块安装到 BiscuitOS 中，使用如下命令:

{% highlight bash %}
cd BiscuitOS/output/linux-5.0-arm32/package/BiscuitOS_Memory-Base
make install
make pack
{% endhighlight %}

以上准备完毕之后，最后就是在 BiscuitOS 运行这个模块了，使用如下命令:

{% highlight bash %}
cd BiscuitOS/output/linux-5.0-arm32/package/BiscuitOS_Memory-Base
make run
{% endhighlight %}

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI001027.png)

在 BiscuitOS 中插入了模块 "BiscuitOS_Memory-Base.ko"，打印如上信息，那么
BiscuitOS Design MMU Project 项目的基础开发环境搭建完毕。

------------------------------------------------

<span id="C0005"></span>

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND00000Z.jpg)

#### 源码开发

本节用于介绍基于 BiscuitOS_Memory-Base 项目进行源码开发，开发者可以参考
本节教程实际动手写代码吧!

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI001006.png)

回忆一下上面章节的讨论，一个内存管理子系统要运作起来，关键是处理好 "虚拟内存"、
"物理内存" 和 "映射关系" 三者之间的关系. 因此本节就从这三个角度来介绍代码的
开发过程。还有一个值得注意的问题，"鸡和蛋的问题"，这类问题在内核很多地方太
常见了，内核通过多增加一层的办法来解决这个问题，回到今天讨论的问题，对于当前
的实践平台，所包含的资源如下:

{% highlight bash %}
1. 一块可用的物理内存，物理地址从 0x70000000 开始, 长度为 0x6400000.
2. 一块可用的虚拟内存，虚拟地址从 0x90000000 开始，长度为 0x6400000.
3. 内核所使用的页目录位于虚拟地址 0x80004000.
4. 系统运行在 ARMv7，包含二级硬件页表，一个 TLB.
{% endhighlight %}

设想一个内存管理子系统的 "鸡和蛋" 问题，内存管理子系统为了初始化需要分配
一些内存给内存管理子系统的初始化，看着有点饶，这就是一个典型的 "蛋和鸡"
问题，是内存管理子系统不初始化好就不能给内存管理子系统初始化分配内存，
内存管理子系统初始化时得不到所需的内存就不能完成内存管理子系统的初始化.
那么开发者是不是要思考一下怎么处理这个 "蛋和鸡" 的问题。

为了解决这个问题，可以通过 "增加一层" 想法来解决，就是内存管理子系统初始化
之前提供一个简单的内存管理，用于分配内存管理初始化时所需的内存，等内存管理
子系统初始化完毕之后就使用内存管理子系统分配和回收所有内存. 通常情况下，
内核初始化过程中，体系相关的初始化阶段就会提供一个基础可用的简单内存为
系统提供初始化，等初始化完毕之后就切到内存管理子系统分配内存。因此我们
也采用这样的逻辑来初始化我们的内存管理子系统.
 
--------------------------------------------------------

#### 构建体系

体系部分的代码就是内存管理子系统初始化之前，为系统准备的一个简单可用的内存
环境，在这个阶段需要完成以下几个任务:

{% highlight bash %}
1. 探测所有可用物理内存和虚拟内存
2. 建立一个简单的映射关系，是系统可以使用内存.
3. 为内存管理子系统预留或分配所需的内存.
{% endhighlight %}

###### 探测任务

通过之前的信息描述，已经知道物理内存和虚拟内存的基本信息，接下来规划项目
的代码。由于这部分是与体系有关的代码，因此在源码目录树下创建名为 "arch"
的目录，代表 "Architecture", 以便以后支持更多的系统架构. 接着规划将与体系
相关的源码包括头文件全部放在这个目录下, 例如:

{% highlight bash %}
arch/mmu.c
arch/include/asm-generated/memory.h
{% endhighlight %}

接着将内存相关的信息定义在 "arch/include/asm-generated/memory.h" 文件里，
如下:

{% highlight bash %}
#ifndef _BISCUITOS_MEMORY_H
#define _BISCUITOS_MEMORY_H

/* BiscuitOS RAM information */
#define BISCUITOS_RAM_BASE              0x70000000
#define BISCUITOS_RAM_SIZE              0x6400000

/* BiscuitOS Virtual information */
#define BISCUITOS_PAGE_OFFSET           0x90000000
#define BISCUITOS_MEMORY_SIZE           0x6400000

#endif
{% endhighlight %}

宏 BISCUITOS_RAM_BASE 描述了物理内存的起始地址，BISCUITOS_RAM_SIZE 宏描述
物理内存的长度. BISCUITOS_PAGE_OFFSET 宏描述了虚拟内存的起始地址，
BISCUITOS_MEMORY_SIZE 宏描述虚拟内存的长度，因此可以得到下图的内存布局:

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI001028.png)

接着规划映射关系，由于内存管理子系统相关数据要让内核能够随时访问，不能被换页
或者随机映射到不同的物理内存上，因此内存管理子系统相关的数据就放在线性区内，
其他虚拟内存作为非线性区。至于线性区和非线性区的比例，这里不做讨论，这个需要
根据实际的需求来决定。那么经过规划出现下图的内存布局:

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI001029.png)

经过之前讨论可以知道，内存管理子系统的虚拟内存分配器和物理内存分配器都使用
bitmap 进行管理，那么将 bitmap 规划在线性区。页表使用的内存同样页规划在线性
区，因此在 ""arch/include/asm-generated/memory.h" 文件里定义如下:

{% highlight bash %}
/* Swapper PGTable */
#define BISCUITOS_PGTABLE_BASE          0x80004000
#define BISCUITOS_PGTABLE_END           0x80008000
#define BISCUITOS_PGTABLE_ENTRY         2048
#define BISCUITOS_PTETABLE_BASE         0x90005000
#define BISCUITOS_PTETABLE_ENTRY        128

/* Linear Mapping Area */
#define BISCUITOS_LINEAR_BASE           0x70000000
#define BISCUITOS_LINEAR_SIZE           0x400000

/* Cross Mapping Area  */
#define BISCUITOS_CROSS_BASE_PHYS       0x70400000
#define BISCUITOS_CROSS_SIZE_PHYS       0x2000000
#define BISCUITOS_CROSS_BASE_VIRT       0x90400000
#define BISCUITOS_CROSS_SIZE_VIRT       0x5000000

/* BITMAP */
#define BISCUITOS_BITMAP_PHYS           0x90200000
#define BISCUITOS_BITMAP_VIRT           0x90300000
{% endhighlight %}

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI001030.png)

宏 BISCUITOS_PGTABLE_BASE 指向内核使用的页表的页目录地址，本项目和内核采用
同一个页目录. BISCUITOS_LINEAR_BASE 宏指向了线性区的物理地址，宏 
BISCUITOS_LINEAR_SIZE 指向了线性区的长度. BISCUITOS_CROSS 相关的宏定义了
分线性映射区的信息. 宏 BISCUITOS_BITMAP_PHYS 定义了内存管理子系统的物理
内存分配器使用的 bitmap 位置, 宏 BISCUITOS_BITMAP_VIRT 定义了内存管理子系统
的虚拟内存分配器使用的 bitmap 位置.

规划完内存布局之后，接下来就是体系相关初始化规划。可用的虚拟内存和物理内存
已经知道，所以体系初始化的第一步就是建立映射关系，将体系相关的代码存放在:

{% highlight bash %}
arch/mmu.c
{% endhighlight %}

{% highlight c %}
void setup_mmu_init(void)
{
        unsigned long addr;
        struct map_desc_bs map;

        /* 1) Clear out all the space mapping */
        for (addr = BISCUITOS_PAGE_OFFSET;
                addr < (BISCUITOS_PAGE_OFFSET + BISCUITOS_MEMORY_SIZE);
                                        addr += PGDIR_SIZE_BS) {
                pmd_clear_bs(pmd_off_k_bs(addr));
        }

        /* 2) Establish Linear Mapping */
        map.pfn = __phys_to_pfn_bs(BISCUITOS_LINEAR_BASE);
        map.virtual = __phys_to_virt_bs(BISCUITOS_LINEAR_BASE);
        map.length = BISCUITOS_LINEAR_SIZE;
        map.type = MT_MEMORY_RWX_BS;

        create_mapping_bs(&map);
        memset(ptecount_bs, 0, sizeof(int));
}
{% endhighlight %}

在 mmu.c 里面创建函数 setup_mmu_init() 并在 init/mian.c 的 startk_kernel()
函数里进行调用，该函数就是体系对内存的初始化. 初始化的第一步是将所有可用的
虚拟内存对应的页表清除。pmd_clear_bs() 和 pmd_off_k_bs() 函数都是页表操作
的函数，用于清除找到虚拟地址对应的 PMD 入口。

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI001031.png)

清除 PMD 入口之后，对应的虚拟地址将不能映射物理地址，那么对应的虚拟地址就
无法访问. 接着第二步操作是为线性区建立页表，从之前的规划了，线性区的起始虚拟
地址是 BISCUITOS_PAGE_OFFSET, 长度是 BISCUITOS_LINEAR_SIZE, 对应映射的物理
内存起始地址是 BISCUITOS_LINEAR_BASE, 这里调用 creat_mapping_bs() 函数进行
映射，通过 struct map_desc_bs 结构描述一段映射关系. 映射关系建立之后，系统
可以直接使用 BISCUITOS_PAGE_OFFSET 开始，长度 BISCUITOS_LINEAR_SIZE 的虚拟
内核，与之映射的物理内存的起始地址是 BISCUITOS_LINEAR_BASE. 如下图:

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI001032.png)

在映射区内物理地址和虚拟地址可以通过简单的线性公式就可以计算，于是在 "memory.h"
文件中定义线性区地址转换的函数，如下:

{% highlight c %}
/*
 * Physical vs virtual RAM address space conversion. These are
 * private definitions which should NOT be used outside memory.h
 * files. Use virt_to_phys/phys_to_virt/__pa/__va instead.
 */
#define __virt_to_phys_bs(x)    ((x) - BISCUITOS_PAGE_OFFSET + \
                                       BISCUITOS_RAM_BASE)
#define __phys_to_virt_bs(x)    ((x) - BISCUITOS_RAM_BASE + \
                                       BISCUITOS_PAGE_OFFSET) 
#define __pa_bs(x)              __virt_to_phys_bs((unsigned long)(x))
#define __va_bs(x)              ((void *)__phys_to_virt_bs((unsigned long)(x)))
{% endhighlight %}

线性区建立完毕之后，为内存管理子系统的建立初始化相关的数据，在 "arch/mmu.c"
文件的 setup_mmu_init() 函数中继续添加代码:

{% highlight bash %}
        memset(ptecount_bs, 0, sizeof(int));

        /* 3) Initialize Bitmap */
        BiscuitOS_bitmap_maxbit_phys = BISCUITOS_CROSS_SIZE_PHYS / PAGE_SIZE_BS;
        memset(BiscuitOS_bitmap_phys, 0x00,
                        BiscuitOS_bitmap_maxbit_phys / BITS_PER_LONG_BS);

        BiscuitOS_bitmap_maxbit_virt = BISCUITOS_CROSS_SIZE_VIRT / PAGE_SIZE_BS;
        memset(BiscuitOS_bitmap_virt, 0x00,
                        BiscuitOS_bitmap_maxbit_virt / BITS_PER_LONG_BS);

        /* 4) Usage */
        BiscuitOS_Memory_Done = 1;
{% endhighlight %}

内存管理子系统的虚拟内存分配器和物理内存分配器都使用 Bitmap 进行管理，于是
在线性区建立指定的 bitmap。

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI001030.png)

首先建立物理内存分配器的 bitmap:

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI001033.png)

系统非映射的物理内存起始地址是 BISCUITOS_CROSS_BASE_PHYS, 整个非映射区的长度
是 BISCUITOS_CROSS_SIZE_PHYS, 将该区域划分为为体积相同的物理内存块，这里将每
个物理内存块的大小设置为 PAGE_SIZE_BS, 并且将物理块编号，统计物理块的数量，
即使用公式:

{% highlight bash %}
BiscuitOS_bitmap_maxbit_phys = BISCUITOS_CROSS_SIZE_PHYS / PAGE_SIZE_BS;
{% endhighlight %}

在 bitmap 中，每个 bit 按顺序对应一个物理内存区块，于是就有了上面计算物理
内存分配器 bitmap 的最大 bit 数量. 物理内存分配器对应的 bitmap 位置在
BiscuitOS_bitmap_phys, 使用 memset 将对应的 bit 全部清零，这样表示整个
非线性区物理内存已经初始化完毕. 接着采用同样的办法建立虚拟内存分配器的
bitmap:

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI001034.png)

系统非映射的虚拟内存起始地址是 BISCUITOS_CROSS_BASE_VIRT, 整个非映射区的长度
是 BISCUITOS_CROSS_SIZE_VIRT, 将该区域划分为为体积相同的虚拟内存块，这里将每
个虚拟内存块的大小也设置为 PAGE_SIZE_BS, 这样便于与物理内存进行映射, 并且将
虚拟内存块编号，统计虚拟内存块的数量，即使用公式:

{% highlight bash %}
BiscuitOS_bitmap_maxbit_virt = BISCUITOS_CROSS_SIZE_VIRT / PAGE_SIZE_BS;
{% endhighlight %}

在 bitmap 中，每个 bit 按顺序对应一个虚拟内存区块，于是就有了上面计算虚拟
内存分配器 bitmap 的最大 bit 数量. 虚拟内存分配器对应的 bitmap 位置在
BiscuitOS_bitmap_virt, 使用 memset 将对应的 bit 全部清零，由于 bitmap 是
一个 unsigned long 的数组，且 memset 按字节进行设置，因此需要计算所有 bit
占用的字节数.这样表示整个非线性区虚拟内存已经初始化完毕. 经过上面的处理，
那么体系已经帮助内存管理子系统初始化好了，接下来就是内存管理子系统的使用了.
这里将全局变量 BiscuitOS_Memory_Done 设置为 1，以此表示内存管理子系统可以
使用.

-------------------------------------------------

###### 物理内存分配器构建

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI001011.png)

上图就是一个初始化完毕的物理内存分配器，bitmap 里所有的 bit 都是清零的，以此
表示对应的物理内存都是可以分配的。于是在内核源码树下创建一个名为 "mm" 的目录，
在 "mm" 的目录下创建一个名为 "BiscuitOS_phys.c" 的文件，该文件用来实现物理
内存分配器。

{% highlight bash %}
phys_addr_t BiscuitOS_alloc_phys(unsigned long size)
{
        unsigned long bits;
        unsigned long pos;

        if (!BiscuitOS_Memory_Done)
                panic("BiscuitOS Memory dones't readly!");

        /* Cacluate the size for allocator, align with PAGE_SIZE */
        bits = (size + PAGE_SIZE_BS - 1) / PAGE_SIZE_BS;

        /* Find free bit on bitmap */
        pos = bitmap_find_next_zero_area_off(BiscuitOS_bitmap_phys,
                                             BiscuitOS_bitmap_maxbit_phys,
                                             0,
                                             bits,
                                             0,
                                             0);
        if (pos >= BiscuitOS_bitmap_maxbit_phys) {
                printk("BiscuitOS no free physical memory.\n");
                return PHYS_MAX_BS;
        }

        /* Mark allocated bit */
        bitmap_set(BiscuitOS_bitmap_phys, pos, bits);

        return BiscuitOS_bitmap_to_phys(pos);
}
{% endhighlight %}

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI001012.png)

当物理内存分配器分配内存时候，通过调用 "BiscuitOS_alloc_phys()" 函数，
函数首先确认 "BiscuitOS_Memory_Done" 是否为 1，以此确认内存管理子系统
是否可以使用，如果不能使用那么 "panic"; 如果可以使用，那么首先计算需要
分配的物理内存占用几个 bit，然后调用函数 bitmap_find_next_zero_area_off()
函数在 bitmap 中，从 bit0 开始查找第一次出现长度为 bits 的位置，如果
没有找到，那么当前物理内存分配器不能提供请求长度的物理内存; 如果找到
那么返回出现的位置，然后调用 bitmap_set() 函数将对应的 bit 全部置位，以此
表示对应的物理内存已经分配. 最后调用 BiscuitOS_bitmap_to_phys() 函数将
对应的 bit 转换为物理地址并返回。内存管理子系统提供了如下函数用于将
bitmap 和物理地址之间的转换:

{% highlight bash %}
static inline phys_addr_t BiscuitOS_bitmap_to_phys(unsigned long bits)
{
        return (bits * PAGE_SIZE_BS) + BISCUITOS_CROSS_BASE_PHYS;
}

static inline unsigned long BiscuitOS_phys_to_bitmap(phys_addr_t addr)
{
        return (addr - BISCUITOS_CROSS_BASE_PHYS) / PAGE_SIZE_BS;
}
{% endhighlight %}

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI001013.png)

{% highlight bash %}
void BiscuitOS_free_phys(phys_addr_t addr, unsigned long size)
{
        unsigned long bits;
        unsigned long pos;

        pos = BiscuitOS_phys_to_bitmap(addr);

        if (pos < 0 || pos >= BiscuitOS_bitmap_maxbit_phys) {
                printk("Invalid physical address.\n");
                dump_stack();
        }

        bits = size / PAGE_SIZE_BS;

        /* Clear bitmap */
        bitmap_clear(BiscuitOS_bitmap_phys, pos, bits);
}
{% endhighlight %}

当释放物理内存给物理内存分配器，使用函数 BiscuitOS_free_phys(). 函数
首先调用 BiscuitOS_phys_to_bitmap() 函数将物理地址转换成在 bitmap 的
偏移，然后计算需要释放的物理内存沾了多少个 bit，最后调用 bitmap_clear()
函数将对应的 bit 清零，以上便完成了物理内存分配器的基本任务.

-------------------------------------------------

###### 虚拟内存分配器构建

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI001008.png)

上图就是一个初始化完毕的虚拟内存分配器，bitmap 里所有的 bit 都是清零的，以此
表示对应的虚拟内存都是可以分配的。于是在内核源码树下创建一个名为 "mm" 的目录，
在 "mm" 的目录下创建一个名为 "BiscuitOS_vir.c" 的文件，该文件用来实现虚拟
内存分配器。

{% highlight bash %}
unsigned long BiscuitOS_alloc_virt(unsigned long size)
{
        unsigned long bits;
        unsigned long pos;

        if (!BiscuitOS_Memory_Done)
                panic("BiscuitOS Memory dones't readly!");

        /* Cacluate the size for allocator, align with PAGE_SIZE */
        bits = (size + PAGE_SIZE_BS - 1) / PAGE_SIZE_BS;

        /* Find free bit on bitmap */
        pos = bitmap_find_next_zero_area_off(BiscuitOS_bitmap_virt,
                                             BiscuitOS_bitmap_maxbit_virt,
                                             0,
                                             bits,
                                             0,
                                             0);
        if (pos >= BiscuitOS_bitmap_maxbit_virt) {
                printk("BiscuitOS no free Virtual memory.\n");
                return VIRT_MAX_BS;
        }

        /* Mark allocated bit */
        bitmap_set(BiscuitOS_bitmap_virt, pos, bits);

        return BiscuitOS_bitmap_to_virt(pos);
}
{% endhighlight %}

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI001009.png)

当虚拟内存分配器分配内存时候，通过调用 "BiscuitOS_alloc_virt()" 函数，
函数首先确认 "BiscuitOS_Memory_Done" 是否为 1，以此确认内存管理子系统
是否可以使用，如果不能使用那么 "panic"; 如果可以使用，那么首先计算需要
分配的虚拟内存占用几个 bit，这里在每次分配中特意添加了一个 PAGE_SIZE
的内存区域用于隔离其他分配的虚拟内存. 然后调用函数 
bitmap_find_next_zero_area_off() 函数在 bitmap 中，从 bit0 开始查找第
一次出现长度为 bits 的位置，如果没有找到，那么当前虚拟内存分配器不能提
供请求长度的虚拟内存; 如果找到那么返回出现的位置，然后调用 bitmap_set() 函
数将对应的 bit 全部置位，以此表示对应的虚拟内存已经分配. 最后调用 
BiscuitOS_bitmap_to_virt() 函数将对应的 bit 转换为虚拟地址并返回。内存管理
子系统提供了如下函数用于将 bitmap 和虚拟地址之间的转换:

{% highlight bash %}
static inline unsigned long BiscuitOS_bitmap_to_virt(unsigned long bits)
{
        return (bits * PAGE_SIZE_BS) + BISCUITOS_CROSS_BASE_VIRT;
}

static inline unsigned long BiscuitOS_virt_to_bitmap(unsigned long addr)
{
        return (addr - BISCUITOS_CROSS_BASE_VIRT) / PAGE_SIZE_BS;
}
{% endhighlight %}

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI001010.png)

{% highlight bash %}
void BiscuitOS_free_virt(unsigned long addr, unsigned long size)
{
        unsigned long bits;
        unsigned long pos;

        pos = BiscuitOS_virt_to_bitmap(addr);

        if (pos < 0 || pos >= BiscuitOS_bitmap_maxbit_virt) {
                printk("Invalid Virtual address.\n");
                dump_stack();
        }

        bits = (size + PAGE_SIZE_BS) / PAGE_SIZE_BS;

        /* Clear bitmap */
        bitmap_clear(BiscuitOS_bitmap_virt, pos, bits);
}
{% endhighlight %}

当释放虚拟内存给虚拟内存分配器，使用函数 BiscuitOS_free_virt(). 函数
首先调用 BiscuitOS_virt_to_bitmap() 函数将虚拟地址转换成在 bitmap 的
偏移，然后计算需要释放的虚拟内存沾了多少个 bit，此时要多加一个 PAGE_SIZE_BS,
以此和分配时候对应. 最后调用 bitmap_clear() 函数将对应的 bit 清零，以上便完
成了虚拟内存分配器的基本任务.

----------------------------------------

###### 映射构建

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI001015.png)

映射构建就是将分配的虚拟内存和物理内存建立映射关系，其实现逻辑位于 "mm" 目录
下的 "BiscuitOS_virt.c" 文件.

{% highlight bash %}
unsigned long BiscuitOS_alloc(unsigned long size)
{
        unsigned long addr;
        phys_addr_t phys;

        /* 1) Allocate virtual address add Guard Page */
        addr = BiscuitOS_alloc_virt(size + PAGE_SIZE_BS);
        if (addr == VIRT_MAX_BS)
                return addr;

        /* 2) Allocate phys address */
        phys = BiscuitOS_alloc_phys(size);
        if (phys == PHYS_MAX_BS)
                goto out_phys;

        /* Mapping Virtual address with Physical address */
        if (BiscuitOS_map_area_bs(addr, phys, size) < 0)
                goto out_map;

        return addr;

out_map:
        BiscuitOS_free_phys(phys, size);
out_phys:
        BiscuitOS_free_virt(addr, size + PAGE_SIZE_BS);
        return VIRT_MAX_BS;
}
{% endhighlight %}

内存管理子系统提供了 "BiscuitOS_alloc()" 函数用于为调用者分配一块可用
的内存，其实现逻辑很简单，第一步就是从虚拟内存分配器中分配指定长度的虚拟
内存，然后再从物理内存分配器中分配所需的物理内存，最后调用 
BiscuitOS_map_area_bs() 函数将虚拟内存和物理内存建立映射. 映射关系建立完毕
之后就可以将虚拟地址反馈给申请者使用. BiscuitOS_map_area_bs() 函数的实现如下:

{% highlight bash %}
static int BiscuitOS_map_area_bs(unsigned long virt, phys_addr_t phys,
                                                        unsigned long size)
{
        unsigned long end = PAGE_ALIGN_BS(virt + size);
        pgd_t_bs *pgd; 
        pte_t_bs *pte, *ptep;

        do {
                pmd_t_bs *pmd;
                pgd = pgd_offset_bs(swapper_pg_dir_bs, virt);
                pmd = pmd_offset_bs(pgd, virt);

                if (pmd_none_bs(*pmd)) {
                        /* alloc new pte */
                        ptep = pte_alloc_bs(); 
                        __pmd_populate_bs(pmd, __pa_bs(ptep), 
                                                _PAGE_KERNEL_TABLE_BS);
                        clean_dcache_area_bs(ptep, 
                                sizeof(pte_t_bs) * PTRS_PER_PTE_BS);
                }

                pte = pte_offset_kernel_bs(pmd, virt);
                set_pte_bs(pte, mk_pte_bs(phys, PAGE_KERNEL_BS));
                __flush_tlb_one_bs(virt);
                virt += PAGE_SIZE_BS;
                phys += PAGE_SIZE_BS;
        } while (virt != end);

        return 0;
}
{% endhighlight %}

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI001031.png)

BiscuitOS_map_area_bs() 函数通过虚拟地址，找到虚拟地址对应的 PGD entry,
然后在通过 PGD entry 找到对应的 PMD entry. 如果此时 PMD entry 为空，即
PTE 页表没有分配，那么函数调用 pte_alloc_bs() 从线性区的 PTE Table 里面
分配一个 PTE 页填充到 PMD 入口里，最后刷新一下 PTE 页表的 Dcache。接着
函数通过虚拟地址和 PMD 入口找到了对应的 PTE 入口，然后将物理地址和 PTE
标志一同写入到 PTE entry 里，最后调用 \_\_flush_tlb_one_bs() 函数是 PTE
entry 有效，至此虚拟地址和物理地址之间的页表建立完毕. 如果此时虚拟地址大
于一个 PTE entry，那么循环建立页表. 至此虚拟地址已经可以使用.

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI001019.png)

映射关系的接触用于调用者将申请的内存返回给内存管理子系统，那么使用函数
"BiscuitOS_free()", 函数实现如下:

{% highlight bash %}
void BiscuitOS_free(unsigned long addr, unsigned long size)
{
        phys_addr_t phys;

        /* Find special physical address */
        phys = BiscuitOS_virt_to_phys(addr);
        if (phys == PHYS_MAX_BS)
                panic("Can't find Physical Address.");

        /* Unmapping */
        BiscuitOS_unmap_area_bs(addr, size);

        /* Free Physical and Virtual */
        BiscuitOS_free_virt(addr, size);
        BiscuitOS_free_phys(phys, size);
}
{% endhighlight %}

"BiscuitOS_free()" 函数首先通过调用 BiscuitOS_virt_to_phys() 函数查找
页表找到对应的物理地址，然后调用 "BiscuitOS_unmap_area_bs()" 函数解除
虚拟地址和物理地址之间的映射关系，最后分别调用 BiscuitOS_free_virt()
和 BiscuitOS_free_phys() 函数释放对应的虚拟内存和物理内存.

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI001019.png)

内存管理子系统提供了页表查询功能，可以通过页表查询虚拟地址对应的物理地址，
实现如下图:

{% highlight bash %}
phys_addr_t BiscuitOS_virt_to_phys(unsigned long virt)
{
        unsigned long phys = PHYS_MAX_BS;

        pgd_t_bs *pgd = pgd_offset_bs(swapper_pg_dir_bs, virt);
        pmd_t_bs *pmd = pmd_offset_bs(pgd, virt);
        if (pmd_present_bs(*pmd)) {
                pte_t_bs *pte = pte_offset_kernel_bs(pmd, virt);
                if (pte_present_bs(*pte)) {
                        phys = pte_phys_bs(*pte);
                }
        }
        return phys;
}
{% endhighlight %}

函数首先通过虚拟地址找到对应的 PGD entry，然后在找到 PMD entry, 此时需要
确认 PMD entry 是否存在，如果存在，那么函数继续根据虚拟地址和 PMD entry
找到对应的 PTE entry，此时只要确认 PTE 存在，那么物理地址就存储在 PTE entry
里，于是调用 pte_phys_bs() 函数从 PTE entry 里取出物理地址。

内存管理子系统通过函数 "BiscuitOS_unmap_area_bs" 解除虚拟地址和物理地址之间
的映射，实现如下:

{% highlight bash %}
static void BiscuitOS_unmap_area_bs(unsigned long addr, unsigned long size)
{
        unsigned long end = PAGE_ALIGN_BS(addr + size);
        pgd_t_bs *pgd;

        /* Unmapping */
        do {
                pte_t_bs *pte;
                pmd_t_bs *pmd;

                pgd = pgd_offset_bs(swapper_pg_dir_bs, addr);
                pmd = pmd_offset_bs(pgd, addr);

                pte = pte_offset_kernel_bs(pmd, addr);
                pte_clear_bs(addr, pte);
                __flush_tlb_one_bs(addr);
                
                addr += PAGE_SIZE_BS;
        } while (addr != end);
 
}
{% endhighlight %}

函数通过虚拟地址找到了对应的 PGD entry 和 PMD entry 之后，在找到对应的 PTE
entry，调用 pte_clear_bs() 将 PTE entry 里面的内容清零即可，最后调用
\_\_flush_tlb_one_bs() 函数刷新对应的 TLB. 这样就解除了一段虚拟内存和物理
内存之间的映射关系。

------------------------------------------------

###### <span id="C0004">测试建议</span>

> - [物理内存分配器测试](#C00041)
>
> - [虚拟内存分配器测试](#C00042)
>
> - [内存管理子系统测试](#C00043)

---------------------------------------------

###### <span id="C00041">物理内存分配器测试</span>

首先测试物理内存正常申请和释放，在 init/main.c 函数中添加如下测试函数:

{% highlight bash %}
vi BiscuitOS/output/linux-5.0-arm32/package/BiscuitOS_Memory-0.11/BiscuitOS_Memory-0.11/init/main.c
{% endhighlight %}

{% highlight bash %}
static void BiscuitOS_test_alloc_phys(void)
{
        phys_addr_t *phys;

        /* Alloc */
        phys = BiscuitOS_alloc_phys(PAGE_SIZE_BS);
        if (phys == PHYS_MAX_BS)
                panic("Alloc failed.\n");
        printk("Phys: %#lx\n", (unsigned long)phys);

        /* free */
        BiscuitOS_free_phys(phys, PAGE_SIZE_BS);
}

asmlinkage void __init start_kernel_bs(void)
{
        setup_mmu_init();
        BiscuitOS_test_alloc_phys();
}
{% endhighlight %}

将 BiscuitOS_test_alloc_phys() 函数添加到 start_kernel() 函数的底部进行
调用，然后使用如下命令运行测试代码:

{% highlight bash %}
cd BiscuitOS/output/linux-5.0-arm32/package/BiscuitOS_Memory-0.11
make
make install
make pack
make run
{% endhighlight %}

当 BiscuitOS 运行之后，使用如下命令加载模块:

{% highlight bash %}
insmod /lib/modules/5.0.0/extra/BiscuitOS_Memory-0.11.ko
{% endhighlight %}

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI001036.png)

从上图的测试结果可以看出，物理内存分配器可以正常分配物理内存.接着测试将所有
的物理内存全部申请完毕，在 init/main.c 函数中添加如下测试函数:

{% highlight bash %}
vi BiscuitOS/output/linux-5.0-arm32/package/BiscuitOS_Memory-0.11/BiscuitOS_Memory-0.11/init/main.c
{% endhighlight %}

{% highlight bash %}
static void BiscuitOS_test_alloc_all_phys(void)
{
        phys_addr_t *phys;

        while (1) {
                phys = BiscuitOS_alloc_phys(PAGE_SIZE_BS);
                if (phys == PHYS_MAX_BS)
                        break;
                printk("Phys: %#lx\n", (unsigned long)phys);
        }
}

asmlinkage void __init start_kernel_bs(void)
{
        setup_mmu_init();
        BiscuitOS_test_alloc_all_phys();
} 
{% endhighlight %}

将 BiscuitOS_test_alloc_all_phys() 函数添加到 start_kernel() 函数的底部进行
调用，然后使用如下命令运行测试代码:

{% highlight bash %}
cd BiscuitOS/output/linux-5.0-arm32/package/BiscuitOS_Memory-0.11
make
make install
make pack
make run
{% endhighlight %}

当 BiscuitOS 运行之后，使用如下命令加载模块:

{% highlight bash %}
insmod /lib/modules/5.0.0/extra/BiscuitOS_Memory-0.11.ko
{% endhighlight %}

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI001035.png)

从运行的结果可以看出，物理内存分配器将所有的物理内存都分配了。

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND000100.png)

---------------------------------------------

###### <span id="C00042">虚拟内存分配器测试</span>

首先测试虚拟内存正常申请和释放，在 init/main.c 函数中添加如下测试函数:

{% highlight bash %}
vi BiscuitOS/output/linux-5.0-arm32/package/BiscuitOS_Memory-0.11/BiscuitOS_Memory-0.11/init/main.c
{% endhighlight %}

{% highlight bash %}
static void BiscuitOS_test_alloc_virt(void)
{
        unsigned long addr;

        /* Alloc */
        addr = BiscuitOS_alloc_virt(PAGE_SIZE_BS);
        if (addr == VIRT_MAX_BS)
                panic("Alloc failed.\n");
        printk("Virt: %#lx\n", (unsigned long)addr);

        /* free */
        BiscuitOS_free_virt(addr, PAGE_SIZE_BS);
}

asmlinkage void __init start_kernel_bs(void)
{
        setup_mmu_init();
        BiscuitOS_test_alloc_virt();
}
{% endhighlight %}

将 BiscuitOS_test_alloc_virt() 函数添加到 start_kernel() 函数的底部进行
调用，然后使用如下命令运行测试代码:

{% highlight bash %}
cd BiscuitOS/output/linux-5.0-arm32/package/BiscuitOS_Memory-0.11
make
make install
make pack
make run
{% endhighlight %}

当 BiscuitOS 运行之后，使用如下命令加载模块:

{% highlight bash %}
insmod /lib/modules/5.0.0/extra/BiscuitOS_Memory-0.11.ko
{% endhighlight %}

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI001037.png)

从运行的结果来看，虚拟内存分配器正常分配和回收虚拟内存. 接着测试将所有
的虚拟内存全部申请完毕，在 init/main.c 函数中添加如下测试函数:

{% highlight bash %}
vi BiscuitOS/output/linux-5.0-arm32/package/BiscuitOS_Memory-0.11/BiscuitOS_Memory-0.11/init/main.c
{% endhighlight %}

{% highlight bash %}
static void BiscuitOS_test_alloc_all_virt(void)
{
        unsigned long addr;

        /* Alloc */
        while (1) {
                addr = BiscuitOS_alloc_virt(PAGE_SIZE_BS);
                if (addr == VIRT_MAX_BS)
                        break;
                printk("Virt: %#lx\n", (unsigned long)addr);
        }
}

asmlinkage void __init start_kernel_bs(void)
{
        setup_mmu_init();
        BiscuitOS_test_alloc_all_virt();
}
{% endhighlight %}

将 BiscuitOS_test_alloc_all_virt() 函数添加到 start_kernel() 函数的底部进行
调用，然后使用如下命令运行测试代码:

{% highlight bash %}
cd BiscuitOS/output/linux-5.0-arm32/package/BiscuitOS_Memory-0.11
make
make install
make pack
make run
{% endhighlight %}

当 BiscuitOS 运行之后，使用如下命令加载模块:

{% highlight bash %}
insmod /lib/modules/5.0.0/extra/BiscuitOS_Memory-0.11.ko
{% endhighlight %}

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI001038.png)

从运行的结果可以看出，虚拟内存分配器将所有的虚拟内存都分配了。

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND000100.png)

---------------------------------------------

###### <span id="C00043">内存管理子系统测试</span>

首先测试内存管理子系统正常申请和释放内存，在 init/main.c 函数中添加如下测试函数:

{% highlight bash %}
vi BiscuitOS/output/linux-5.0-arm32/package/BiscuitOS_Memory-0.11/BiscuitOS_Memory-0.11/init/main.c
{% endhighlight %}

{% highlight bash %}
static void BiscuitOS_test_alloc(void)
{
        phys_addr_t *phys;

        /* Alloc */
        phys = BiscuitOS_alloc_phys(PAGE_SIZE_BS);
        if (phys == PHYS_MAX_BS)
                panic("Alloc failed.\n");
        printk("Phys: %#lx\n", (unsigned long)phys);

        /* free */
        BiscuitOS_free_phys(phys, PAGE_SIZE_BS);
}

asmlinkage void __init start_kernel_bs(void)
{
        setup_mmu_init();
        BiscuitOS_test_alloc_phys();
}
{% endhighlight %}

将 BiscuitOS_test_alloc() 函数添加到 start_kernel() 函数的底部进行
调用，然后使用如下命令运行测试代码:

{% highlight bash %}
cd BiscuitOS/output/linux-5.0-arm32/package/BiscuitOS_Memory-0.11
make
make install
make pack
make run
{% endhighlight %}

当 BiscuitOS 运行之后，使用如下命令加载模块:

{% highlight bash %}
insmod /lib/modules/5.0.0/extra/BiscuitOS_Memory-0.11.ko
{% endhighlight %}

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI001039.png)

从运行的结果可以看出，内存管理子系统能够正常分配内存给调用者. 接着测试将所有
的虚拟内存全部申请完毕，在 init/main.c 函数中添加如下测试函数:

{% highlight bash %}
vi BiscuitOS/output/linux-5.0-arm32/package/BiscuitOS_Memory-0.11/BiscuitOS_Memory-0.11/init/main.c
{% endhighlight %}

{% highlight bash %}
static void BiscuitOS_test_alloc_all(void)
{
        void *addr;
        
        /* Alloc */
        while (1) {
                addr = BiscuitOS_alloc(PAGE_SIZE_BS);
                if (addr == VIRT_MAX_BS)
                        panic("Alloc failed.\n");
                sprintf(addr, "BiscuitOS-%#lx", addr);
                printk("%s\n", addr);
        }
}

asmlinkage void __init start_kernel_bs(void)
{
        setup_mmu_init();
        BiscuitOS_test_alloc_all();
}
{% endhighlight %}

将 BiscuitOS_test_alloc_all() 函数添加到 start_kernel() 函数的底部进行
调用，然后使用如下命令运行测试代码:

{% highlight bash %}
cd BiscuitOS/output/linux-5.0-arm32/package/BiscuitOS_Memory-0.11
make
make install
make pack
make run
{% endhighlight %}

当 BiscuitOS 运行之后，使用如下命令加载模块:

{% highlight bash %}
insmod /lib/modules/5.0.0/extra/BiscuitOS_Memory-0.11.ko
{% endhighlight %}

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI001040.png)

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND000100.png)

----------------------------------

<span id="D"></span>

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND00000P.jpg)

#### 进阶研究

> - [内存管理子系统演变](#D00001)

--------------------------------------

#### <span id="D00001">内存管理子系统演变</span>

通过本文的介绍，开发者已经知道一个基础内存管理子系统如何构成的，那么这个
基础的内存管理子系统如何演化成目前主流的 Linux 内存管理器呢？ 那么接下
一起来讨论这个问题:

###### Bootmem/MEMBLOCK 分配器演变

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI000775.png)

在本文介绍的基础内存管理子系统中，在解决 "鸡和蛋" 的问题时候，通过让体系
来构建一个基础的可用内容，这时开发者或者可以考虑如何在内存管理子系统初始化
之前，让 Bootmem/MEMBLOCK 分配器来管理这个阶段的内存，等为内存管理分配器
分配了初始化所使用的内存之后，Bootmem 和 MEMBLOCK 分配器的任务就已经完成，
那么 Bootmem/MEMBLOCK 内存分配器将内存管理权交接给内存管理子系统，这就是
bootmem 和 MEMBLOCK 的演变过程。

###### Buddy 分配器演变

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI000827.png)

在本文介绍的基础内存管理子系统中，使用一个简单的 bitmap 管理物理内存的分配
和回收，如果把所有的物理内存设置为 PAGE_SIZE 大小，然后使用 struct page 维护
每个物理页，然后使用 Buddy 算法管理物理页，这样简陋的 Buddy 内存分配器
就演变而来，如果将物理内存按功能划分成不同的分区，每个分区维护各自的物理
页，那么 struct zone 就出现了，然后将分区分为 ZONE_DMA/ZONE_DMA32、ZONE_NORMAL
和 ZONE_HIGHMEM, 这样一个低版本高的 Buddy 内存管理器就演变而来.

###### 线性区

在本文介绍的基础内存管理子系统中，如果将新划分的 ZONE_DMA/ZONE_DMA32 和 
ZONE_NORMAL 与内核虚拟地址空间进行一一映射，因此形成了内核的线性空间.

###### 非线性区

在本文介绍的基础内存管理子系统中，如果物理内存与虚拟内存长度不同，因此
不能将将所有的虚拟内存建立线性映射区，因此就出现了非线性区。如果将线性
映射的物理内存称为低端内存，然后将非线性的物理内存定义为高端内存，这样
演变成 Linux 的低端和高端内存.

###### SLAB/SLUB 分配器演变

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/HK000268.png)

在本文介绍的基础内存管理子系统中，线性区的虚拟内存分配都是按 PAGE_SIZE
进行分配的，如果如让虚拟内存分配器可以分更小块的虚拟内存，以及可以根据
对象进行分配，那么这样就可以演变成一个低版本的 SLAB/SLUB 分配器.

###### VMALLOC 分配器演变

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/HK000289.png)

在本文介绍的基础内存管理子系统中，如果将非线性映射的虚拟内存划分一定
长度的内存区域，并使用红黑树管理虚拟内存的分配，然后将分配的虚拟内存
映射到动态分配的物理内存上，这样就演变成 Linux 的 VMALLOC 内存分配器。

###### KMAP 分配器

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI000969.png)

在本文介绍的基础内存管理子系统中，如果将非线性映射的虚拟内存划分一定
长度的内存区域，并建立这段虚拟内存的固定 PTE 页表，然后从这段虚拟
内存中分配指定长度的虚拟内存，然后分配随机的物理内存，然后将物理
地址写入指定的 PTE 入口中，这样申请者就可以使用这段虚拟内存，不使用的
时候就清除 PTE entry 的内容，然后释放物理内存给系统，这样就演变成
KMAP 内存分配器。

###### FIXMAP 分配器

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI000984.png)

在本文介绍的基础内存管理子系统中，如果将固定的虚拟地址作为固定任务
使用，但需要为固定任务分配这段内存之后，可以直接获得虚拟地址，然后
从物理内存分配物理内存，并与虚拟内存建立页表，这样任务就可以使用这段
虚拟内存。如果不适用这段虚拟内存，只需清除对应 PTE 页表的入口，然后
释放物理内存就行. 这样就演变成 FIXMAP 分配器.

--------------------------------------

#### <span id="Z0">附录</span>

> [BiscuitOS Home](https://biscuitos.github.io/)
>
> [BiscuitOS Driver](https://biscuitos.github.io/blog/BiscuitOS_Catalogue/)
>
> [Linux Kernel](https://www.kernel.org/)
>
> [Bootlin: Elixir Cross Referencer](https://elixir.bootlin.com/linux/latest/source)
>

#### 捐赠一下吧 🙂

![MMU](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/HAB000036.jpg)

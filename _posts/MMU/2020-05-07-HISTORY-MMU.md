---
layout: post
title:  "Memory Manager Unit History"
date:   2020-05-07 08:35:30 +0800
categories: [HW]
excerpt: MMU.
tags:
  - MMU
---

![](/assets/PDB/BiscuitOS/kernel/IND00000L0.PNG)

![](/assets/PDB/RPI/RPI100100.png)

#### 目录

> - [项目介绍](#A)
>
>   - [项目用途](#A1000000)
>
>   - [项目开源许可](#A2000000)
>
>   - [开发者计划](#A3000000)
>
> - [项目实现](#B)
>
>   - [物理地址布局](#A0000001)
>
>   - [虚拟地址布局](#A0000002)
>
>   - [模块实现](#A0000003)
>
>   - [项目运行](#A0000004)
>
> - [项目实践](#C)
>
> - [MMU 进阶研究](#D)
>
> - [MMU 时间轴](#R)
>
> - [MMU 历史版本](#T)
>
> - 内存分配器
>
>   - [Bootmem 分配器](https://biscuitos.github.io/blog/HISTORY-bootmem/)
>  
>   - [MEMBLOCK 分配器](https://biscuitos.github.io/blog/MMU-ARM32-MEMBLOCK-index)
>  
>   - [PERCPU 分配器](https://biscuitos.github.io/blog/HISTORY-PERCPU)
>  
>   - [Buddy 分配器](https://biscuitos.github.io/blog/HISTORY-buddy)
>  
>   - [PCP 分配器](https://biscuitos.github.io/blog/HISTORY-PCP)
>  
>   - [SLAB 分配器](https://biscuitos.github.io/blog/HISTORY-SLAB/)
>  
>   - [SLUB 分配器](https://biscuitos.github.io/blog/HISTORY-SLUB)
>  
>   - [SLOB 分配器](https://biscuitos.github.io/blog/HISTORY-SLOB)
>  
>   - [VMALLOC 分配器](https://biscuitos.github.io/blog/HISTORY-VMALLOC)
>  
>   - [KMAP 分配器](https://biscuitos.github.io/blog/HISTORY-KMAP)
>  
>   - [FIXMAP 分配器](https://biscuitos.github.io/blog/HISTORY-FIXMAP)
>  
>   - [DMA 内存分配器](https://biscuitos.github.io/blog/HISTORY-DMA)
>  
>   - [CMA 内存分配器](https://biscuitos.github.io/blog/CMA)
>
> - [附录/捐赠](#Z0)

![](/assets/PDB/BiscuitOS/kernel/IND000100.png)

----------------------------------

<span id="A"></span>

![](/assets/PDB/BiscuitOS/kernel/IND00000P.jpg)

#### 项目介绍

![](/assets/PDB/RPI/RPI001042.png)

BiscuitOS Memory Manager Unit History 项目是使用内核模块的方式，从系统预留的
物理内存上构建一个早期的 Linux 内核管理子系统，并在上面实现多个内存管理器
和内存行为. 该系统包含了基础的内存管理器: 

{% highlight c %}
# Memory Allocator
1. Bootmem
2. MEMBLOCK
3. PERCPU
4. Buddy
5. Slab
6. Slub
7. Slob
8. VMALLOC
9. KMAP
10. Fixmap
11. CMA
{% endhighlight %}

以及基础的内存管理行为:

{% highlight bash %}
# Memory Action
1. TLB 刷新
2. CACHE 刷新
3. 页表的建立
4. 内存的分配回收
5. Kswap
6. VMSCAN
7. VMscan
8. SWAP
9. SHMEM/Tmpfs
10. Mempool 
{% endhighlight %}

通过以上功能建立一个独立真实可用的内存管理子系统。项目基于该模块构建不同历
史版本的内存管理子系统，以便给开发者提供一个分析运行多个历史版本的 Linux
内存管理子系统. 目前该项目支持的 Linux 版本如下:

{% highlight bash %}
# Kernel Version
Linux 2.6.12
Linux 2.6.12.1
Linux 2.6.12.2
Linux 2.6.12.3
Linux 2.6.12.4
Linux 2.6.12.5
Linux 2.6.12.6
Linux 2.6.13
Linux 2.6.13.1
Linux 2.6.14
Linux 2.6.15
{% endhighlight %}

![](/assets/PDB/BiscuitOS/kernel/IND000100.png)

-------------------------------------------------

#### <span id="A1000000">项目用途</span>

> - [从简如手](#A10000001)
>
> - [PatchWork](#A10000002)
>
> - [分配器历史](#A10000003)
>
> - [动手写内存管理](#A10000004)

---------------------------------------

###### <span id="A10000001">从简如手</span>

任何复杂的是否都是由最简单的事物构成，内存管理子系统也不例外。该项目提供了
Linux 早期的内存管理子系统版本，开发者可以选择相对简单的版本进行研究，从
浅入深，最后掌握内存管理子系统。因此高版本的 linux 内存管理太难深入的话，
可以考虑使用这个项目。

![](/assets/PDB/BiscuitOS/kernel/IND000100.png)

---------------------------------

###### <span id="A10000002">PatchWork</span>

该项目比较特殊的用法就是研究补丁。开发者可以使用该项目制作一个特定版本的
内存管理器，然后基于 Patchwork 或者 tig 等工具，在当前版本上打上特定的
的补丁，然后研究补丁的作用，或者复现特定的问题。

![](/assets/PDB/RPI/RPI000744.png)

开发者可以通过 PatchWork 的官网上获得指定的补丁，或者通过 linux 源码树
上获得补丁信息:

> - [Patchwork 官网](https://patchwork.kernel.org/)
>
> - [Patchwork: linux-mm](https://patchwork.kernel.org/project/linux-mm/list/)
>
> - [Linux Commit information](https://git.kernel.org/pub/scm/linux/kernel/git/torvalds/linux.git/)
>
> - [Linux on Github(torvalds)](https://github.com/torvalds/linux)

![](/assets/PDB/BiscuitOS/kernel/IND000100.png)

----------------------------------

###### <span id="A10000003">分配器历史</span>

该项目的一个更重要目的就是研究某个内存分配的发展历史。通过研究内存分配器
的历史变化，可以知道某个内存管理器是什么时候加入到内核的，以及内存管理器
内部实现原理变化历史，也是对深入理解内存管理一种不错的方法。目前支持的内
存管理器历史支持的内存管理器件如下表:

> - [Linux 分配器历史](#T)

----------------------------------

###### <span id="A10000004">动手写内存管理</span>

该项目一个比较有趣的用途就是参照这个项目，开发者独立建立一个模块，然后从
第一行代码开始实现一个最简单版本的内存管理子系统，这也是这个项目比较推荐的
用途。不仅可以锻炼完整项目开发经验，并且可以通过解决开发过程中遇到的 bug
来增加自己解决问题的能力。具体过程可以参考:

> - [动手构建一个内存管理子系统](https://biscuitos.github.io/blog/Design-MMU/)

![](/assets/PDB/BiscuitOS/kernel/IND000100.png)

-------------------------------------------------

#### <span id="A2000000">项目开源许可</span>

项目中关于 linux 内存管理子系统实现的代码均来自 Linux 内核源码，在项目源码
中，对于函数变量均添加了 "\_bs" 的后缀，以及宏添加了 "\_BS" 的后缀，但这些
代码都是从 Linux 中获得，并非自己编写，并遵循原始代码对应的开源许可权限。对
于我独立开发的代码，均遵循 GPL 开源权限。

![](/assets/PDB/RPI/RPI000745.jpeg)

![](/assets/PDB/BiscuitOS/kernel/IND000100.png)

-------------------------------------------------

#### <span id="A3000000">开发者计划</span>

BiscuitOS Memory Manager Unit History 项目是一个开源的项目，欢迎各位开发者
一同进行开发。如果开发者在使用和开发过程中遇到问题，可以在 BiscuitOS 社区
进行讨论，或者提交相应的 Patch 到我的邮箱, 具体联系方式如下:

> - BiscuitOS 社区微信: Zhang514981221
>
> - BiscuitOs 社区邮箱: buddy.zhang@aliyun.com

----------------------------------

<span id="B"></span>

![](/assets/PDB/BiscuitOS/kernel/IND00000H.jpg)

#### 项目实现

> - [物理地址布局](#A0000001)
>
> - [虚拟地址布局](#A0000002)
>
> - [模块实现](#A0000003)
>
> - [项目运行](#A0000004)

![](/assets/PDB/BiscuitOS/kernel/IND000100.png)

---------------------------------------

###### <span id="A0000001">物理地址布局</span>

BiscuitOS Memory Manager Unit History 项目通过独立模块的方式，从内核中预留
一段物理内存和虚拟内存作为项目管理的地址空间。由于采用模块的方式，项目构建
的内存管理子系统是与运行主机的内存管理子系统隔绝，以此进行独立的内存管理。
项目对物理地址的布局采用了 ZONE_DMA, ZONE_DMA32, ZONE_NORMAL 和 ZONE_HIGHMEM
的模式，将物理内存划分成不同的区间. 项目中定义了物理地址的起始地址是 
"0x70000000", 终止物理地址是 "0x76400000", 总长度为 100MB. 如下图:

![](/assets/PDB/RPI/RPI000736.png)

在有的 Linux 版本中支持 ZONE_DMA, ZONE_NORMAL 和 ZONE_HIGHMEM, 如上图所示:
ZONE_DMA 区域是为 GFP_DMA 分配的物理内存，该区域位于物理内存的最前端，
起始地址是 "0x70000000" 终止地址是 "0x70400000", 其长度为 4M Bytes 空间; 
ZONE_NORMAL 是用于线性映射的区域，该区域的物理内存和虚拟地址一一映射，其
起始地址是 "0x70400000" 终止地址是 "0x74400000"，其长度为 64M Bytes 空间; 
ZONE_HIGHMEM 区域是高端内存物理区域，这个区域的物理页主要给 VMALLOC，KMAP 
和 FIXMAP 内存分配器使用，并动态的与这些区域的虚拟地址建立页表，该区域的
起始地址是 "0x74400000" 终止地址是 "0x76400000"， 长度为 32M Bytes 空间.

![](/assets/PDB/RPI/RPI000735.png)

在有的 Linux 版本中开始支持 ZONE_DMA32 区域，在这种情况下，物理内存布局如上图:
ZONE_DMA32 是 32 位系统上预留给特定硬件设备使用的物理区域，该区域的
起始地址是 "0x70300000" 终止地址是 "0x70400000", 其长度为 1M Bytes 空间。
ZONE_DMA 区域是为 GFP_DMA 分配的物理内存，该区域位于物理内存的最前端，
起始地址是 "0x70000000" 终止地址是 "0x70400000", 其长度为 4M Bytes 空间; 
ZONE_NORMAL 是用于线性映射的区域，该区域的物理内存和虚拟地址一一映射，其
起始地址是 "0x70400000" 终止地址是 "0x74400000"，其长度为 64M Bytes 空间; 
ZONE_HIGHMEM 区域是高端内存物理区域，这个区域的物理页主要给 VMALLOC，KMAP 
和 FIXMAP 内存分配器使用，并动态的与这些区域的虚拟地址建立页表，该区域的
起始地址是 "0x74400000" 终止地址是 "0x76400000"， 长度为 32M Bytes 空间.

![](/assets/PDB/BiscuitOS/kernel/IND000100.png)

-------------------------------------

###### <span id="A0000002">虚拟地址布局</span>

![](/assets/PDB/RPI/RPI000737.png)

BiscuitOS Memory Manager Unit History 项目的虚拟地址布局如上图。项目中定义
了虚拟地址的起始地址即 PAGE_OFFSET 的值为 "0x90000000"。首先是一段长度为
68MB 的线性地址映射空间，这段虚拟地址就是将 ZONE_DMA/ZONE_DMA32 和 ZONE_NORMAL
的物理地址和虚拟地址进行一一映射。在项目中称这段虚拟内存区域为 
"Linear Mapping Space" 或者线性映射空间。线性映射空间的起始地址从 "0x90000000"
到 "0x94400000" 结束，总长度为 68MB，线性映射区的结束虚拟地址也称为 
high_memory, 其指明了 high_memory 之后的虚拟地址对应的物理地址来自 ZONE_HIGHM;
线性映射区之后是一个 Hole 区域，该区域的长度是 10 个 page 的长度，紧接其后的
是 VMALLOC 虚拟内存区，该区域的虚拟地址供 VMALLOC 内存分配器使用，VMALLOC 分
配器分配的内存就是虚拟地址连续但物理地址不连续的内存，导致这个原因的是 
VMALLOC 动态从 ZONE_HIGHMEM 中获得定量的物理内存页，再动态建立页表，在这个过
程中，由于物理页可能是不连续的，因此会出现虚拟地址连续但物理地址不一定连续的
情况。VMALLOC 区域的起始地址是 VMALLOC_START (0x9440A000), 结束虚拟地址是
VMALLOC_END (0x95E0A000), VMALLOC 区域总长度为 26MB; 虚拟区域结束地址是
0x96400000, FIXMAP 区域称为固定映射区域，该区域长度为一个 PAGE_SIZE 的长度，
FIXMAP 区域包含了一个页对应的虚拟地址空间，因此每一个虚拟入口可以
映射物理地址。FIXADDR_TOP 指向 FIXMAP 最后一个映射入口地址，其值为 
"0x963ff000", FIXADDR_START 指向 FIXMAP 的第一个映射入口地址，其值为 
"0x96395000"; 在虚拟地址空间的倒数第二个 PMD_SIZE 处，往前退 2MB 的空间正好
是 KMAP 映射区间，这段区域称为零时映射区域。其地址范围起始于 PKMAP_BASE
"0x96000000", 终止于 PKMAP_ADDR(LAST_PKMAP) "0x96200000". 该区域的虚拟地址
动态与 ZONE_HIGHMEM 的物理页进行零时映射，映射完毕之后在调用指定的函数解除
映射关系。除以上定义的区域之外，其他虚拟地址区域均称为 "Hole".

![](/assets/PDB/BiscuitOS/kernel/IND000100.png)

-------------------------------------------

###### <span id="A0000003">模块实现</span>

BiscuitOS Memory Manager Unit History 项目基于模块实现。寄主机器是运行
在 ARM 上的 Linux 5.0 系统。项目在 Linux 5.0 上通过 DTS 预留了一块 100MB
的物理内存，然后基于这段物理内存和真实的 TLB/CACHE 和页表建立了上述说描述
的内存管理子系统，DTS 配置如下:

![](/assets/PDB/RPI/RPI000738.png)

从上图可以看出，项目在 DTS 中定义了一个名为 BiscuitOS 的节点，节点的 ram
属性用来描述整个规划的物理内存和虚拟内存。ram 属性指向了 BiscuitOS_memory
节点，节点是 reserved-memory 节点中的子节点，从节点 reg 属性可以知道，项目
从当前系统中预留了一段物理内存，其起始地址是 0x70000000, 长度为 0x6400000.
节点中还定义了各个虚拟内存和物理内存的长度等信息。项目就是依据这个配置预留
了指定的内存。

模块是一个标准的 Platform 驱动，驱动也包含了独立的链接脚本，该脚本也是项目
内存规划的重要组成部分，其内容如下:

![](/assets/PDB/RPI/RPI000739.png)

链接脚本中定义了很多 section，以便实现内存管理子系统中所需的特定功能。
".early_param_bs" section 主要用来模拟从 Uboot 的 ATAG 中解析内存信息参数
的相关函数; ".bootmem_bs" section 内的函数将在 bootmem 内存初始化之后调用;
".percpu_bs" section 内的函数将在 PERCPU 初始化之后，供静态 PERCPU 变量相关
函数使用; ".buddy_bs" section 是 buddy 内存分配器初始化结束之后，系统将自动
调用该区域内的函数; ".pcp_bs" section 是 PCP 内存分配器初始化完毕之后，系统
将自动调用该区域的函数; ".slab_bs" section 是 SLAB 分配器初始化之后，系统将
自动调用该区域内的函数; ".vmalloc_bs" section 是 VMALLOC 内存分配器初始化完
毕之后，系统自动调用该区域内的函数; ".kmap_bs" section 是 KMAP 内存分配器
初始化完毕之后系统自动调用该 section 内的函数; ".fixmap_bs" section 是
FIXMAP 内存分配器初始化完毕之后，系统自动调用该 section 内的函数;
".init_bs" section 是内存管理子系统初始化完毕之后，系统自动调用该 section
内的函数。因此项目为这些 section 添加了类似与 initcall 机制，具体接口如下:

{% highlight c %}
bootmem_initcall_bs();
percpu_initcall_bs();
buddy_initcall_bs();
pcp_initcall_bs();
slab_initcall_bs();
vmalloc_initcall_bs();
kmap_initcall_bs();
fixmap_initcall_bs();
module_initcall_bs();
login_initcall_bs();
{% endhighlight %}

![](/assets/PDB/RPI/RPI000740.png)

arch 目录下包含内存初始化早期，与体系结构相关的处理部分。mm 目录下面包含
了与各个内存分配器和内存管理行为相关的代码。init 目录下是整个模块的初始化
入口流程。modules 目录下包含了内存分配器的使用例程和测试代码. fs 目录下
包含了内存管理信息输出到文件系统的相关实现。项目中内存管理相关的体系代码
参考了 ARMv7 的实现逻辑，因此 TLB/CACHE 和页表的实现均参考 ARMv7 手册进行
构建，体系相关的资料，请参考:

> - [ARM_Architecture_Reference_Manual](https://gitee.com/BiscuitOS_team/Documentation/blob/master/Datasheet/ARM/ARM_Architecture_Reference_Manual.pdf)
>
> - [ARMv7_architecture_reference_manual](https://gitee.com/BiscuitOS_team/Documentation/blob/master/Datasheet/ARM/ARMv7_architecture_reference_manual.pdf)
>
> - [Cortex-A9_MPcore_Technical_Reference_Manual](https://gitee.com/BiscuitOS_team/Documentation/blob/master/Datasheet/ARM/Cortex-A9_MPcore_Technical_Reference_Manual.pdf)
>
> - [Cortex_A9_Technical_Reference_Manual](https://gitee.com/BiscuitOS_team/Documentation/blob/master/Datasheet/ARM/Cortex_A9_Technical_Reference_Manual.pdf)

![](/assets/PDB/BiscuitOS/kernel/IND000100.png)

---------------------------------------

###### <span id="A0000004">项目运行</span>

由于 BiscuitOS Memory Manager Unit History 项目是由模块实现，因此可以向通用
的模块方式进行编译和安装，具体情况如下:

![](/assets/PDB/RPI/RPI000741.png)

编译完毕之后，在目标机上运行模块:

![](/assets/PDB/RPI/RPI000742.png)

当插入模块之后，系统成功的构建了一个 linux 2.6.15 的内存管理子系统，此时
可以使用如下命令查看该子系统的内存信息:

![](/assets/PDB/RPI/RPI000743.png)

从上面获得信息中完整描述了该内存管理子系统的使用情况。

------------------------------------------------

<span id="C"></span>

![](/assets/PDB/BiscuitOS/kernel/IND00000I.jpg)

#### 项目实践

> - [实践准备](#C0000)
>
> - [实践部署](#C0001)
>
> - [实践执行](#C0002)
>
> - [实践建议](#C0003)
>
> - [测试建议](#C0004)

![](/assets/PDB/BiscuitOS/kernel/IND000100.png)

--------------------------------------------

#### <span id="C0000">实践准备</span>

本实践是基于 BiscuitOS Linux 5.0 ARM32 环境进行搭建，因此开发者首先
准备实践环境，请查看如下文档进行搭建:

> - [BiscuitOS Linux 5.0 ARM32 环境部署](https://biscuitos.github.io/blog/Linux-5.0-arm32-Usermanual/)

--------------------------------------------

#### <span id="C0001">实践部署</span>

准备好基础开发环境之后，开发者接下来部署项目所需的开发环境。由于项目
支持多个版本的 MMU，开发者可以根据需求进行选择，本文以 linux 2.6.12 
版本的 MMU 进行讲解。开发者使用如下命令:

{% highlight bash %}
cd BiscuitOS/
make linux-5.0-arm32_defconfig
make menuconfig
{% endhighlight %}

![](/assets/PDB/RPI/RPI000746.png)

选择并进入 "[\*] Package  --->" 目录。

![](/assets/PDB/RPI/RPI000747.png)

选择并进入 "[\*]   Memory Development History  --->" 目录。

![](/assets/PDB/RPI/RPI000748.png)

选择并进入 "[\*]   MMU: Memory Manager Unit  --->" 目录。

![](/assets/PDB/RPI/RPI000749.png)

选择 "[\*]   MMU on linux 2.6.12  --->" 目录，保存并退出。接着执行如下命令:

{% highlight bash %}
make
{% endhighlight %}

![](/assets/PDB/RPI/RPI000750.png)

成功之后将出现上图的内容，接下来开发者执行如下命令以便切换到项目的路径:

{% highlight bash %}
cd BiscuitOS/output/linux-5.0-arm32/package/BiscuitOS_MMU-2.6.12
make download
{% endhighlight %}

![](/assets/PDB/RPI/RPI000751.png)

至此源码已经下载完成，开发者可以使用 tree 等工具查看源码:

![](/assets/PDB/RPI/RPI000752.png)

arch 目录下包含内存初始化早期，与体系结构相关的处理部分。mm 目录下面包含
了与各个内存分配器和内存管理行为相关的代码。init 目录下是整个模块的初始化
入口流程。modules 目录下包含了内存分配器的使用例程和测试代码. fs 目录下
包含了内存管理信息输出到文件系统的相关实现。

如果你是第一次使用这个项目，需要修改 DTS 的内容。如果不是可以跳到下一节。
开发者参考源码目录里面的 "BiscuitOS.dts" 文件，将文件中描述的内容添加
到系统的 DTS 里面，"BiscuitOS.dts" 里的内容用来从系统中预留 100MB 的物理
内存供项目使用，具体如下:

![](/assets/PDB/RPI/RPI000738.png)

开发者将 "BiscuitOS.dts" 的内容添加到:

{% highlight bash %}
cd BiscuitOS/output/linux-5.0-arm32/linux/linux/arch/arm/boot/dts/vexpress-v2p-ca9.dts
{% endhighlight %}

添加完毕之后，使用如下命令更新 DTS:

{% highlight bash %}
cd BiscuitOS/output/linux-5.0-arm32/package/BiscuitOS_MMU-2.6.12
make kernel
{% endhighlight %}

![](/assets/PDB/RPI/RPI000753.png)

--------------------------------------------

#### <span id="C0002">实践执行</span>

环境部署完毕之后，开发者可以向通用模块一样对源码进行编译和安装使用，使用
如下命令:

{% highlight bash %}
cd BiscuitOS/output/linux-5.0-arm32/package/BiscuitOS_MMU-2.6.12
make
{% endhighlight %}

![](/assets/PDB/RPI/RPI000754.png)

以上就是模块成功编译，接下来将 ko 模块安装到 BiscuitOS 中，使用如下命令:

{% highlight bash %}
cd BiscuitOS/output/linux-5.0-arm32/package/BiscuitOS_MMU-2.6.12
make install
make pack
{% endhighlight %}

以上准备完毕之后，最后就是在 BiscuitOS 运行这个模块了，使用如下命令:

{% highlight bash %}
cd BiscuitOS/output/linux-5.0-arm32/package/BiscuitOS_MMU-2.6.12
make run
{% endhighlight %}

![](/assets/PDB/RPI/RPI000755.png)

在 BiscuitOS 中插入了模块 "BiscuitOS_MMU-2.6.12.ko"，打印如上信息，那么
BiscuitOS Memory Manager Unit History 项目的内存管理子系统已经可以使用，
接下来开发者可以在 BiscuitOS 中使用如下命令查看内存管理子系统的使用情况:

{% highlight bash %}
cat /proc/buddyinfo_bs
cat /proc/vmstat_bs
{% endhighlight %}

![](/assets/PDB/RPI/RPI000756.png)

------------------------------------------------

<span id="C0003"></span>

![](/assets/PDB/BiscuitOS/kernel/IND00000Q.jpg)

#### 实践建议

为了方便开发者调试和对项目进行二次开发，本节总结了几个有用的方法，开发者
参考使用.

> - [快速函数跳转](#C00030)
>
> - [干扰打印](#C00031)
>
> - [initcall 机制](#C00032)
>
> - [升级策略](#C00033)

----------------------------------------

###### <span id="C00030">快速函数跳转</span>

在源码浏览过程中，可以借助 ctags 等工具实现函数的快速跳转，开发者可以参考
下面步骤进行部署，例如在上面提到的项目中，使用如下命令:

{% highlight bash %}
cd BiscuitOS/output/linux-5.0-arm32/package/BiscuitOS_MMU-2.6.12
ctags -R
{% endhighlight %}

然后创建或修改 .vimrc 文件，如下:

{% highlight bash %}
vi ~/.vimrc
{% endhighlight %}

![](/assets/PDB/RPI/RPI000757.png)

有了上面的工具之后，可以在要跳转的函数处使用 "Ctrl ]" 组合键进行跳转，返回
跳转点可以使用 "Ctrl t" 的组合键进行跳转。

-----------------------------------------------

###### <span id="C00031">干扰打印</span>

在开发过程中，由于在公共函数添加了打印函数，因此不能准确确认哪个操作带来
的打印，为了解决这个问题，开发者可以使用如下函数进行打印:

{% highlight bash %}
#include "biscuitos/kernel.h"

bs_debug_enable()
bs_debug_disable()
bs_debug(...)
{% endhighlight %}

在项目中提供了上面三个接口，通过头文件 "biscuitos/kernel.h" 进行引用，
当要调试某段代码块的时候，在外部使用 bs_debug_enable() 函数打开调试
开关，然后使用 bs_debug() 函数打印相关的信息。在代码块结尾使用
"bs_debug_disable()" 函数关闭。这套函数将解决大部分的混乱打印问题。

![](/assets/PDB/RPI/RPI000758.png)

![](/assets/PDB/BiscuitOS/kernel/IND000100.png)

--------------------------------------

###### <span id="C00032">Initcall 机制</span>

这里的 Initcall 机制和内核的 Initcall 机制是一致的，引入这个机制利于
对特定的阶段进行调试。项目中提供了一套接口实现 Initcall 机制，如下:

{% highlight bash %}
#include "biscuitos/init.h"
bootmem_initcall_bs();
作用:
     该接口会将一个函数添加到 .bootmem_bs section 内，模块初始化完毕
     bootmem 内存分配器之后，自动调用 .bootmem_bs section 内部的所有
     函数。使用如下图:
{% endhighlight %}

![](/assets/PDB/RPI/RPI000759.png)

{% highlight bash %}
#include "biscuitos/init.h"
percpu_initcall_bs();
作用:
     该接口会将一个函数添加到 .percpu_bs section 内，模块初始化完毕
     PERCPU 内存分配器之后，自动调用 .percpu_bs section 内部的所有
     函数。使用如下图:
{% endhighlight %}

![](/assets/PDB/RPI/RPI000760.png)

{% highlight bash %}
#include "biscuitos/init.h"
buddy_initcall_bs();
作用:
     该接口会将一个函数添加到 .buddy_bs section 内，模块初始化完毕
     buddy 内存分配器之后，自动调用 .buddy_bs section 内部的所有
     函数。使用如下图:
{% endhighlight %}

![](/assets/PDB/RPI/RPI000761.png)

{% highlight bash %}
#include "biscuitos/init.h"
pcp_initcall_bs();
作用:
     该接口会将一个函数添加到 .pcp_bs section 内，模块初始化完毕 
     PCP 内存分配器之后，自动调用 .pcp_bs section 内部的所有  
     函数。使用如下图:
{% endhighlight %}

![](/assets/PDB/RPI/RPI000762.png)

{% highlight bash %}
#include "biscuitos/init.h"
slab_initcall_bs();
作用:
     该接口会将一个函数添加到 .slab_bs section 内，模块初始化完毕   
     Slab 内存分配器之后，自动调用 .slab_bs section 内部的所有    
     函数。使用如下图:
{% endhighlight %}

![](/assets/PDB/RPI/RPI000763.png)

{% highlight bash %}
#include "biscuitos/init.h"
vmalloc_initcall_bs();
作用:
     该接口会将一个函数添加到 .vmalloc_bs section 内，模块初始化完毕
     Slab 内存分配器之后，自动调用 .vmalloc_bs section 内部的所有
     函数。使用如下图:
{% endhighlight %}

![](/assets/PDB/RPI/RPI000763.png)

{% highlight bash %}
#include "biscuitos/init.h"
kmap_initcall_bs();
作用:
     该接口会将一个函数添加到 .kmap_bs section 内，模块初始化完毕
     Kmap 内存分配器之后，自动调用 .kmap_bs section 内部的所有
     函数。使用如下图:
{% endhighlight %}

![](/assets/PDB/RPI/RPI000764.png)

{% highlight bash %}
#include "biscuitos/init.h"
fixmap_initcall_bs();
作用:
     该接口会将一个函数添加到 .fixmap_bs section 内，模块初始化完毕   
     Fixmap 内存分配器之后，自动调用 .fixmap_bs section 内部的所有   
     函数。使用如下图:
{% endhighlight %}

![](/assets/PDB/RPI/RPI000765.png)

{% highlight bash %}
#include "biscuitos/init.h"
login_initcall_bs();
作用:
     该接口会将一个函数添加到 .login_bs section 内，模块初始化完毕
     整个 MMU 之后，自动调用 .login_bs section 内部的所有
     函数。使用如下图:
{% endhighlight %}

![](/assets/PDB/RPI/RPI000766.png)

----------------------------------------

###### <span id="C00033">升级策略</span>

当开发者向从某一个版本的内存管理子系统升级到特定版本的内存管理子系统，
开发者可以使用如下几个办法进行升级。

第一种方法使用 Meld 工具进行辅助升级。首先从 kernel 源码网站上下载当前
版本的源码和将要升级版本的源码，网站如下:

> - [Linux Kernel 源码国内镜像源](http://ftp.sjtu.edu.cn/sites/ftp.kernel.org/pub/linux/kernel/)

例如下载好 linux-2.6.14.tar.gz 和 linux-2.6.15.tar.gz 之后，解压到特定目录，
然后使用如下命令:

{% highlight bash %}
meld linux-2.6.14 linux-2.6.15
{% endhighlight %}

![](/assets/PDB/RPI/RPI000767.png)

![](/assets/PDB/RPI/RPI000768.png)

通过上面提示的差异进行代码合入。该方法适用与对原始版本的 MMU 细节特别了解，
知道合入之后的差异和引起的问题，不建议新手使用。

第二种方式就是合入标准补丁的方式。这种方式首先需要获得 Linux 源码树完整
补丁树，开发者可以参考 BiscuitOS 提供的方案，如下:

> - [Linux newest arm32 Usermanual](https://biscuitos.github.io/blog/Linux-newest-arm32-Usermanual/)

通过上面的文档，可以在下面的路径中获得最新的源码数:

{% highlight bash %}
BiscuitOS/output/linux-newest-arm32/linux/linux
{% endhighlight %}

开发者也可以不采用 BiscuitOS 提供的方案，自行从 Github 上获得 Torvalds 
最新的 Linux 分支，使用如下命令:

{% highlight bash %}
git clone https://github.com/torvalds/linux.git
{% endhighlight %}

两种方案都会获得 Linux 内核源码数，接着开发者可以参考下面的方案进行补丁
获得，例如进行 linux 2.6.14 版本的内存管理子系统升级到 linux 2.6.15 版本，
以 BiscuitOS 版本为例:

{% highlight bash %}
cd BiscuitOS/output/linux-newest-arm32/linux/linux
git branch linux-2.6.15
git checkout linux-2.6.15
git reset --hard v2.6.15
{% endhighlight %}

通过上面的操作可以获得 linux 2.6.15 相关的补丁，此时借助 tig 工具查看补丁:

{% highlight bash %}
cd BiscuitOS/output/linux-newest-arm32/linux/linux
tig mm/
{% endhighlight %}

![](/assets/PDB/RPI/RPI000769.png)

从上图可以获得 mm 目录下提交的相关补丁，可以查看补丁的描述。如果觉得
补丁可用，那么开发者可以独立出上图的补丁，例如在上图中，选中了一个补丁，
补丁由 Haren Myneni 提交，提交的 commin 信息是:

{% highlight bash %}
[PATCH] fix in __alloc_bootmem_core() when there is no free

commit 66d43e98ea6ff291cd4e524386bfb99105feb180
{% endhighlight %}

此时退出 tig，使用 "git format-patch" 命令生成对应的补丁:

{% highlight bash %}
git format-patch -1 66d43e98ea6ff291cd4e524386bfb99105feb180
{% endhighlight %}

于是就可以获得一个 patch:

![](/assets/PDB/RPI/RPI000770.png)

开发者可以利用这个补丁进行合入到当前内存管理子系统。

![](/assets/PDB/BiscuitOS/kernel/IND000100.png)

--------------------------------------

###### <span id="C0004">测试建议</span>

BiscuitOS Memory Manager Unit History 项目提供了大量的测试用例用于测试
不同内存分配器的功能。结合项目提供的 initcall 机制，项目将测试用例分作
两类，第一类类似于内核源码树内编译，也就是同 MMU 子系统一同编译的源码。
第二类类似于模块编译，是在 MMU 模块加载之后独立加载的模块。以上两种方案
皆在最大程度的测试内存管理器的功能。

要在项目中使用以上两种测试代码，开发者可以通过项目提供的 Makefile 进行
配置。以 linux 2.6.12 为例， Makefile 的位置如下:

{% highlight bash %}
/xspace/OpenSource/BiscuitOS/BiscuitOS/output/linux-5.0-arm32/package/BiscuitOS_MMU-2.6.12/BiscuitOS_MMU-2.6.12/Makefile
{% endhighlight %}

![](/assets/PDB/RPI/RPI000771.png)

Makefile 内提供了两种方案的编译开关，例如需要使用打开 buddy 内存管理器的
源码树内部调试功能，需要保证 Makefile 内下面语句不被注释:

{% highlight bash %}
$(MODULE_NAME)-m                += modules/buddy/main.o
{% endhighlight %}

如果要关闭 buddy 内存管理器的源码树内部调试功能，可以将其注释:

{% highlight bash %}
# $(MODULE_NAME)-m                += modules/buddy/main.o
{% endhighlight %}

同理，需要打开 buddy 模块测试功能，可以参照下面的代码:

{% highlight bash %}
obj-m                             += $(MODULE_NAME)-buddy.o
$(MODULE_NAME)-buddy-m            := modules/buddy/module.o
{% endhighlight %}

如果不需要 buddy 模块测试功能，可以参考下面代码, 将其注释:

{% highlight bash %}
# obj-m                             += $(MODULE_NAME)-buddy.o
# $(MODULE_NAME)-buddy-m            := modules/buddy/module.o
{% endhighlight %}

在上面的例子中，例如打开了 buddy 的模块调试功能，重新编译模块并在 BiscuitOS
上运行，如下图，可以在 "lib/module/5.0.0/extra/" 目录下看到两个模块:

![](/assets/PDB/RPI/RPI000772.png)

然后先向 BiscuitOS 中插入 "BiscuitOS_MMU-2.6.12.ko" 模块，然后再插入
"BiscuitOS_MMU-2.6.12-buddy.ko" 模块。如下:

![](/assets/PDB/RPI/RPI000773.png)

以上便是测试代码的使用办法。开发者如果想在源码中启用或关闭某些宏，可以
修改 Makefile 中内容:

![](/assets/PDB/RPI/RPI000774.png)

从上图可以知道，如果要启用某些宏，可以在 ccflags-y 中添加 "-D" 选项进行
启用，源码的编译参数也可以添加到 ccflags-y 中去。开发者除了使用上面的办法
进行测试之外，也可以使用项目提供的 initcall 机制进行调试，具体请参考:

> - [Initcall 机制调试说明](#C00032)

![](/assets/PDB/BiscuitOS/kernel/IND000100.png)

----------------------------------

<span id="T"></span>

![](/assets/PDB/BiscuitOS/kernel/IND00000E.jpg)

#### MMU 历史版本

> - [Bootmem 分配器历史版本](https://biscuitos.github.io/blog/HISTORY-bootmem/#H)
>
> - [MEMBLOCK 分配器历史版本](https://biscuitos.github.io/blog/HISTORY-MEMBLOCK/#H)
>
> - [PERCPU 分配器历史版本](https://biscuitos.github.io/blog/HISTORY-PERCPU/#H)
>
> - [Buddy 分配器历史版本](https://biscuitos.github.io/blog/HISTORY-buddy/#H)
>
> - [PCP 分配器历史版本](https://biscuitos.github.io/blog/HISTORY-PCP/#H)
>
> - [SLAB 分配器历史版本](https://biscuitos.github.io/blog/HISTORY-SLAB/#H)
>
> - [SLUB 分配器历史版本](https://biscuitos.github.io/blog/HISTORY-SLUB/#H)
>
> - [SLOB 分配器历史版本](https://biscuitos.github.io/blog/HISTORY-SLOB/#H)
>
> - [VMALLOC 分配器历史版本](https://biscuitos.github.io/blog/HISTORY-VMALLOC/#H)
>
> - [KMAP 分配器历史版本](https://biscuitos.github.io/blog/HISTORY-KMAP/#H)
>
> - [FIXMAP 分配器历史版本](https://biscuitos.github.io/blog/HISTORY-FIXMAP/#H)
>
> - [DMA 内存分配器历史版本](https://biscuitos.github.io/blog/HISTORY-DMA/#H)
>
> - [CMA 内存分配器历史版本](https://biscuitos.github.io/blog/HISTORY-CMA/#H)

![](/assets/PDB/BiscuitOS/kernel/IND000100.png)

-----------------------------------------------

<span id="R"></span>

![](/assets/PDB/BiscuitOS/kernel/IND00000H.jpg)

#### MMU 时间轴

![](/assets/PDB/RPI/RPI000999.png)

![](/assets/PDB/BiscuitOS/kernel/IND000100.png)

----------------------------------

<span id="D"></span>

![](/assets/PDB/BiscuitOS/kernel/IND00000P.jpg)

#### MMU 进阶研究

> - [动手构建一个内存管理子系统](https://biscuitos.github.io/blog/Design-MMU/)

![](/assets/PDB/BiscuitOS/kernel/IND000100.png)

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

![MMU](/assets/PDB/BiscuitOS/kernel/HAB000036.jpg)

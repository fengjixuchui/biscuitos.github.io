---
layout: post
title:  "Linux Memory Allocator on Userspace"
date:   2020-02-22 09:23:30 +0800
categories: [HW]
excerpt: Memory.
tags:
  - [MMU]
---

![](/assets/PDB/BiscuitOS/kernel/IND00000L0.PNG)

![](/assets/PDB/RPI/RPI100100.png)

## 目录

> - [项目介绍](#A)
>
> - [MEMBLOCAK Memory Allocator](#B)
>
> - [PERCPU(UP) Memory Allocator](#C)
>
> - [PERCPU(SMP) Memory Allocator](#D)
>
> - [Buddy-Normal Memory Allocator](#E)
>
> - [Buddy-HighMEM Memory Allocator](#F)
>
> - [PCP Memory Allocator](#G)
>
> - [Slub Memory Allocator](#H)
>
> - [Kmem_cache Memory Allocator](#J)
>
> - [Kmalloc Memory Allocator](#K)
>
> - [NAME Memory Allocator](#L)
>
> - [VMALLOC Memory Allocator](#M)
>
> - [KMAP Memory Allocator](#N)
>
> - [Fixmap Memory Allocator](#P)
>
> - [附录/捐赠](#Z0)

----------------------------------

<span id="A"></span>

![](/assets/PDB/BiscuitOS/kernel/IND00000P.jpg)

#### 项目介绍

BiscuitOS Open-Memory 项目是将 Linux 内核主流的内存分配器移植到用户空间。
移植之后的内存分配器保留了内存分配器的核心实现，并且移除了非核心的功能，
即用简化的方式在用户空间实现了内存分配器。对于 Linux 内存分配器的初期了解
以及实践提供了便利。

BiscuitOS Open-Memory 项目提供了 Linux 运行的必须内存分配器，即包括了
物理内存管理分配器 MEMBLOCK、Buddy, 也包含了虚拟内存管理分配器 SLUB、
Kmem_cache、Kmalloc、VMALLOC、KMAP，并且包含了特定功能的内存分配器
PERCPU、NAME 内存分配器。

BiscuitOS Open-Memory 项目根据各个内存分配器在 Linux 的生命周期，编写
了相互独立或者相互依赖的内存分配器，如下图:

![](/assets/PDB/HK/HK000225.png)

从上图可以看出，第一个内存分配器是 MEMBLOCK 分配器，项目中提供了一个独立
的用户空间 MEMBLOCK 内存分配器项目。第二个内存分配器是 PERCPU 内存分配器，
其基于 MEMBLOCK 内存分配器，因此用户空间的 PERCPU 分配器是基于用户空间
MEMBLOCK 内存分配器实现的项目。第三个内存分配器是 Buddy 内存分配器，项目
中的 Buddy-Normal/Buddy-Highmem/PCP 都是独立的内存分配器项目。第四个内存
分配器是 SLUB/Kmem_cache/Kmalloc/NAME 分配器，这些分配器是基于用户空间
的 Buddy-normal 分配器构建的。第五个内存分配器是 VMALLOC 分配器，其基于
用户空间的 Buddy-Highmem 和 用户空间的 kmalloc 内存分配器构建。第六个
内存分配是 KMAP/FIXMAP 内存分配器，其基于用户空间的 Buddy-Highmem 分配器
构建的项目。开发者可以根据上面的关系选择研究的顺序。

BiscuitOS Open-Memory 项目构建的用户空间分配器基于上图的虚拟地址布局:

![](/assets/PDB/HK/HK000226.png)

ERROR_AREA 是内核的错误处理区域; Userspace 代表应用程序运行的虚拟地址
空间;  TASK_SIZE 是用户空间应用程序堆栈的最大地址; PKMAP_BASE 到 
PAGE_OFFSET 的区域是 KMAP 内存分配器维护的区域; PAGE_OFFSET 代表
内核空间的起始虚拟地址; swapper_pg_dir 指向内核页表目录的基地址;
KERNEL_START 代表内核镜像的起始地址; KERNEL_END 代表内核镜像的终止
地址; lowmemory 代表物理地址和虚拟地址一一映射的区域; high_memory
只向低端物理内存结束的虚拟地址; VMALLOC_OFFSET 指向一段虚拟内存区域，
用于隔开内核一一映射的虚拟内存区和 VMALLOC 区域; VMALLOC_START 与
VMALLOC_END 指向 VMALLOC 内存区域; FIXADDR_START 与 FIXADDR_END 指向
FIXMAP 区域.

BiscuitOS Open-Memory 提供了在 X86 或者 BiscuitOS 实践详细教程，开发者
可以通过教程对多个内存分配器进行实践研究。项目中内存分配器的源码来源于
Linux 5.x，原理根据经典书籍 《深入理解 Linux 虚拟内存管理》。更多 Linux
实践文档请参考 BiscuitOS 文档:

> - [BiscuitOS 主页: https://biscuitos.github.io/](https://biscuitos.github.io/)
>
> - [BiscuitOS 实践文档目录: /blog/BiscuitOS_Catalogue/](/blog/BiscuitOS_Catalogue/) 
>
> - 作者邮箱: BuddyZhang1 <buddy.zhang@aliyun.com>

----------------------------------

<span id="B"></span>

![](/assets/PDB/BiscuitOS/kernel/IND00000T.jpg)

#### MEMBLOCAK Memory Allocator

![](/assets/PDB/HK/HK000227.png)

> - [MEMBLOCK 内存分配器简介](#B0)
>
> - [用户空间 MEMBLOCK 内存分配器实践](#B1)
>
> - [用户空间 MEMBLOCK 内存分配器 BiscuitOS 实践](#B2)

![](/assets/PDB/BiscuitOS/kernel/IND000101.jpg)

----------------------------------------------------------

#### <span id="B0">MEMBLOCK 内存分配器简介</span>

MEMBLOCK 物理内存分配器是 Linux 启动初期管理所有物理内存的管理器，为当前
Linux 提供了内存分配，虽然 MEMBLOCK 管理的是物理内存，但此时 MMU 已经打开，
系统只存在虚拟地址，因此此时的物理内存就是假想出来的。MEMBLOCK 内存分配器
提供了大粒度的内存分配，也提供了小粒度的内存分配器，MEMBLOCK 内存分配器
也是作为 Buddy、PERCPU 内存分配器的基础内存分配器。

MEMBLOCK 内存分配器在内核启动初期完成任务之后，系统将丢弃使用 MEMBLOCK
内存分配器，转而使用 Buddy 内存分配器管理物理内存。MEMBLOCK 内存分配器
的实现相对简单，其将物理内存分作基础的两类物理内存: "可用的物理内存" 和
"预留的物理内存"。系统启动初期，Linux 内核会将被占用或从 MEMBLOCK 分配
的物理内存加入到 MEMBLOCK 预留区，剩余的物理内存作为 "可用物理内存"。
MEMBLOCK 内存分配器完成使命之后，会将 "可用物理内存" 的物理内存以 
"可用物理页" 的形式传递给 Buddy 内存管理器进行管理，而 "预留区内存" 的
物理内存也会以 "预留物理页" 的形式在 mem_map[] 中进行管理，以便将来
做特殊功能使用。"预留物理页" 可以永久作为系统预留，不给其他内核功能使用，
例如内核镜像占用的物理页就是永久预留; 也有做临时预留的物理页，例如
CMA 占用的物理页作为临时预留，直到 CMA 内存管理器初始化时才解除预留。
更多 MEMBLOCK 内存分配器的原理和实践参考下列文章:

> - [MEMBLOCK 内存分配器原理及实践](/blog/MMU-ARM32-MEMBLOCK-index/#header)

![](/assets/PDB/HK/HK000225.png) 

上图是 MEMBLOCK 与其他内存分配器的生命周期，可以看出 MEMBLOCK 通常
完成使命之后，内核就放弃使用 MEMBLOCK 管理物理内存。不过内存也支持
MEMBLOCK 长期维护物理内存，需要进行相应的内核配置。

----------------------------------

<span id="B1"></span>

![](/assets/PDB/BiscuitOS/kernel/IND00000M.jpg)

#### 用户空间 MEMBLOCK 内存分配器实践

> - [实践部署](#B10)
>
> - [实践执行](#B11)
>
> - [实践分析](#B12)

![](/assets/PDB/BiscuitOS/kernel/IND000101.jpg)

---------------------------------------

###### <span id="B10">实践部署</span>

在进行用户空间 MEMBLOCK 内存分配实践之前，开发者应该准备一台 X86 Linux
主机，主机安装基础的 gcc 和 make 开发工具，参考并执行下面命令:

{% highlight bash %}
mkdir -p BiscuitOS_MEM
cd BiscuitOS_MEM
wget https://gitee.com/BiscuitOS_team/HardStack/raw/Gitee/Memory-Allocator/BiscuitOS_MM.sh
chmod 755 BiscuitOS_MM.sh
{% endhighlight %}

执行完上面的命令之后，会获得名为 "BiscuitOS_MM.sh" 的脚本, 该脚本用来
自动部署实践所需的源码。准备工作一切准备就绪，接下来看下一节内容.

---------------------------------------

###### <span id="B11">实践执行</span>

接下来，执行下面的命令

{% highlight bash %}
./BiscuitOS_MM.sh
{% endhighlight %}

![](/assets/PDB/HK/HK000228.png)

执行完上面的命令会出现上图的对话框，该对话框用于部署不同内存分配器的
源码项目。对于用户空间的 MEMBLOCK 内存分配器，这里输入 1 并回车，如果
是第一次执行上面的命令，那么脚本会从网上下载并部署所需的源码。待下载
部署完毕之后，可以在当前目录获得名为 "MEMBLOCK" 的目录，此时进入该目录，
查看源码如下图:

![](/assets/PDB/HK/HK000229.png)

用户空间 MEMBLOCK 内存分配器包含了 7 个文件，"mian.c" 文件包含了用户空间
MEMBLOCK 内存分配器的使用例子，开发者可以参考这些例子对 MEMBLOCK 内存
分配器进行更多的探索. "mm/memblock.c" 和 "include/linux/memblock.h" 文件
包含了用户空间的核心实现。接下来使用下面命令运行用户空间的 MEMBLOCK 内存
分配器:

{% highlight bash %}
cd MEMBLOCK/
make clean
make
./biscuitos
{% endhighlight %}

![](/assets/PDB/HK/HK000230.png)

以上就是用户空间 MEMBLOCK 运行情况，开发者可以从 "main.c" 文件的 "main()"
函数作为入口来实践 MEMBLOCK 的实现过程。

---------------------------------------

###### <span id="B12">实践分析</span>

用户空间的 MEMBLOCK 实现逻辑是使用 "malloc()" 函数从用户空间申请一定
长度的虚拟内存充当 MEMBLOCK 的 "物理内存"，并且在 Makefile 文件中，通过
宏 CONFIG_MEMBLOCK_BASE 来设置这块 "物理内存" 的起始物理地址，并使用宏
CONFIG_MEMBLOCK_SIZE 设置 "物理内存" 的长度，开发者可以在 Makefile 中
修改这两个值自定义 MEMBLOCK 的 "物理内存". 在默认情况下，在用户空间为
MEMBLOCK 创建了一块长度为 16M，起始物理地址为 0x60000000 的 "物理内存"，
MEMBLOCK 内存管理器基于以上信息进行初始化，初始化完毕之后，为提供提供
了 "memblock_alloc()"、 "memblock_free()" 等函数供系统其他部分分配内存。
"main.c" 函数中提供了多个用户空间 MEMBLOCK 内存管理器的使用例子，开发者
可以基于这些例子对 MEMBLOCK 内存分配器的实现逻辑进行研究实践.

----------------------------------

<span id="B2"></span>

![](/assets/PDB/BiscuitOS/kernel/IND00000I.jpg)

#### 用户空间 MEMBLOCK 内存分配器 BiscuitOS 实践

> - [实践部署](#B20)
>
> - [实践执行](#B21)
>
> - [实践分析](#B22)

![](/assets/PDB/BiscuitOS/kernel/IND000101.jpg)

-----------------------------------------------

##### <span id="B20">实践部署</span>

BiscuitOS 已经支持用户空间的 MEMBLOCK 内存分配器实践，如果
还未部署 BiscuitOS，请参考下列文章, 如果已经部署 BiscuitOS
项目的童鞋请使用 "git pull" 更新 BiscuitOS，并查看下一节:

> - [BiscuitOS linux 5.0 部署介绍](/blog/Linux-5.0-arm32-Usermanual/)

-----------------------------------------------

##### <span id="B21">实践执行</span>

在部署或更新 BiscuitOS 之后，参考下面命令获得用户空间 MEMBLOCK 项目:

{% highlight bash %}
cd BiscuitOS/
make linux-5.0-arm32_defconfig
make menuconfig
{% endhighlight %}

![](/assets/PDB/HK/HK000231.png)

选择并进入 "Package  --->",

![](/assets/PDB/HK/HK000232.png)

选择并进入 "MEMBLOCK Memory Allocator  --->",

![](/assets/PDB/HK/HK000233.png)

选择 "MEMBLOCK Allocator on Userspace  --->", 保存并退出.

选择完毕之后，接下来使用如下命令部署用户空间的 MEMBLOCK 内存分配器:

{% highlight bash %}
make
cd BiscuitOS/output/linux-5.0-arm32/package/memblock_userspace-0.0.1
{% endhighlight %}

执行完 make 命令之后，系统会提示所使用版本对于的安装目录，开发者可以
根据自己的版本参考执行。执行完毕之后，使用如下命令:

{% highlight bash %}
make download
make clean
make
make install
make pack
tree
{% endhighlight %}

![](/assets/PDB/HK/HK000235.png)

执行完上面的代码之后，BiscuitOS 自动部署用户空间 MEMBLOCK 内存分配器
项目的源码，并将目标文件安装到 BiscuitOS，接下来在 BiscuitOS 上运行
用户空间 MEMBLOCK 内存分配器，使用如下命令:

{% highlight bash %}
cd BiscuitOS/output/linux-5.0-arm32/
./RunBiscuitOS.sh
{% endhighlight %}

![](/assets/PDB/HK/HK000234.png)

上图就是用户空间 MEMBLOCK 在 BiscuitOS 运行的效果。

---------------------------------------

###### <span id="B22">实践分析</span>

用户空间的 MEMBLOCK 实现逻辑是使用 "malloc()" 函数从用户空间申请一定
长度的虚拟内存充当 MEMBLOCK 的 "物理内存"，并且在 Makefile 文件中，通过
宏 CONFIG_MEMBLOCK_BASE 来设置这块 "物理内存" 的起始物理地址，并使用宏
CONFIG_MEMBLOCK_SIZE 设置 "物理内存" 的长度，开发者可以在 Makefile 中
修改这两个值自定义 MEMBLOCK 的 "物理内存". 在默认情况下，在用户空间为
MEMBLOCK 创建了一块长度为 16M，起始物理地址为 0x60000000 的 "物理内存"，
MEMBLOCK 内存管理器基于以上信息进行初始化，初始化完毕之后，为提供提供
了 "memblock_alloc()"、 "memblock_free()" 等函数供系统其他部分分配内存。
"main.c" 函数中提供了多个用户空间 MEMBLOCK 内存管理器的使用例子，开发者
可以基于这些例子对 MEMBLOCK 内存分配器的实现逻辑进行研究实践.


----------------------------------

<span id="C"></span>

![](/assets/PDB/BiscuitOS/kernel/IND00000F.jpg)

#### PERCPU(UP) Memory Allocator

![](/assets/PDB/HK/HK000236.png)

> - [PERCPU(UP) 内存分配器简介](#C0)
>
> - [用户空间 PERCPU(UP) 内存分配器实践](#C1)
>
> - [用户空间 PERCPU(UP) 内存分配器 BiscuitOS 实践](#C2)

![](/assets/PDB/BiscuitOS/kernel/IND000101.jpg)

----------------------------------------------------------

#### <span id="C0">PERCPU(UP) 内存分配器简介</span>

PERCPU 内存分配器的主要用途就是为每个 CPU 分配本地内存。在 Linux 中，
每当创建一个 percpu 变量或结构的时候，系统都会为每个 CPU 分配同样大小
内存。CPU 操作 percpu 变量对应的私有内存，不存在竞态，因此 CPU 可以不加锁
放心使用各自的私有内存。PERCPU 内存分配器就是为 percpu 变量或结构提供
相应逻辑的内存，因此每个 percpu 会消耗自身大小 CPUS 倍的内存。

PERCPU 内存分配的建立在 MEMBLOCK 内存分配器之后，Buddy 内存分配器之前，
属于比较早期的内存分配器。PERCPU 内存分配器从 MEMBLOCK 内存分配器中
分配一部分物理内存，PERCPU 内存分配器基于 BITMAP 逻辑将内存分成指定
大小的块，每一块内存通过 BITMAP 进行管理。

由于系统存在 UP 单核架构和 SMP 多核架构，因此 PERCPU 内存分配器的实现
逻辑存在一定的差异。对于 UP 单核架构，那么 PERCPU 内存分配器只需维护
一个 CPU 使用的内存，其实现逻辑相对于 SMP 架构简单很多，因此对开发者
来说，研究 PERCPU(UP) 内存分配器相对友好。

![](/assets/PDB/HK/HK000225.png) 

上图是 PERCPU(UP) 与其他内存分配器的生命周期，PERCPU(UP) 在 Linux 
初始阶段依赖 MEMBLOCK 阶段，当系统初始化完毕之后，如果 PERCPU 内存分配
器需要扩充内存的时候，PERCPU 可以从 Buddy 内存分配器或者 VMALLOC 内存
分配器中获得相应的内存。更多 PERCPU 相关的内核实例可以参考如下链接:

> - [PERCPU 内核实践例子 GITHUB](https://github.com/BiscuitOS/HardStack/tree/master/Memory-Allocator/PERCPU)

----------------------------------

<span id="C1"></span>

![](/assets/PDB/BiscuitOS/kernel/IND00000B.jpg)

#### 用户空间 PERCPU(UP) 内存分配器实践

> - [实践部署](#C10)
>
> - [实践执行](#C11)
>
> - [实践分析](#C12)

![](/assets/PDB/BiscuitOS/kernel/IND000101.jpg)

---------------------------------------

###### <span id="C10">实践部署</span>

在进行用户空间 PERCPU(UP) 内存分配实践之前，开发者应该准备一台 X86 Linux
主机，主机安装基础的 gcc 和 make 开发工具，参考并执行下面命令:

{% highlight bash %}
mkdir -p BiscuitOS_MEM
cd BiscuitOS_MEM
wget https://gitee.com/BiscuitOS_team/HardStack/raw/Gitee/Memory-Allocator/BiscuitOS_MM.sh
chmod 755 BiscuitOS_MM.sh
{% endhighlight %}

执行完上面的命令之后，会获得名为 "BiscuitOS_MM.sh" 的脚本, 该脚本用来
自动部署实践所需的源码。准备工作一切准备就绪，接下来看下一节内容.

---------------------------------------

###### <span id="C11">实践执行</span>

接下来，执行下面的命令

{% highlight bash %}
./BiscuitOS_MM.sh
{% endhighlight %}

![](/assets/PDB/HK/HK000228.png)

执行完上面的命令会出现上图的对话框，该对话框用于部署不同内存分配器的
源码项目。对于用户空间的 PERCPU(UP) 内存分配器，这里输入 2 并回车，如果
是第一次执行上面的命令，那么脚本会从网上下载并部署所需的源码。待下载
部署完毕之后，可以在当前目录获得名为 "PERCPU-UP" 的目录，此时进入该目录，
查看源码如下图:

![](/assets/PDB/HK/HK000237.png)

用户空间 PERCPU(UP) 内存分配器包含了 14 个文件，"mian.c" 文件包含了用户空间
PERCPU(UP) 内存分配器的使用例子，开发者可以参考这些例子对 PERCPU(UP) 内存
分配器进行更多的探索. "mm/percpu.c" 和 "include/linux/percpu.h" 文件
包含了用户空间的核心实现。接下来使用下面命令运行用户空间的 PERCPU(UP) 内存
分配器:

{% highlight bash %}
cd PERCPU-UP/
make clean
make
./biscuitos
{% endhighlight %}

![](/assets/PDB/HK/HK000238.png)

以上就是用户空间 PERCPU(UP) 运行情况，开发者可以从 "main.c" 文件的 "main()"
函数作为入口来实践 PERCPU(UP) 的实现过程。

---------------------------------------

###### <span id="C12">实践分析</span>

用户空间的 PERCPU(UP) 实现逻辑是从用户空间的 MEMBLOCK 内存分配器中
分配指定大小的内存，然后将这些内存分成指定大小的内存块，每个内存块
通过 BITMAP 进行管理，其实现如下图:

![](/assets/PDB/HK/HK000223.png)

如上图，map_size 指向的空间就是 PERPCU(UP) 维护的内存，PERCPU(UP) 内存
内存管理器通过 alloc_BITMAP 和 pcpu_block_md 对所有的可用内存进行分配
和释放。用户空间的 PERCPU(UP) 实现了其逻辑，开发者可以从 "main.c" 文件
的 "main()" 函数研究 PERCPU(UP) 的工作原理。

----------------------------------

<span id="C2"></span>

![](/assets/PDB/BiscuitOS/kernel/IND00000Q.jpg)

#### 用户空间 PERCPU(UP) 内存分配器 BiscuitOS 实践

> - [实践部署](#C20)
>
> - [实践执行](#C21)
>
> - [实践分析](#C22)

![](/assets/PDB/BiscuitOS/kernel/IND000101.jpg)

-----------------------------------------------

##### <span id="C20">实践部署</span>

BiscuitOS 已经支持用户空间的 PERCPU(UP) 内存分配器实践，如果
还未部署 BiscuitOS，请参考下列文章, 如果已经部署 BiscuitOS
项目的童鞋请使用 "git pull" 更新 BiscuitOS，并查看下一节:

> - [BiscuitOS linux 5.0 部署介绍](/blog/Linux-5.0-arm32-Usermanual/)

-----------------------------------------------

##### <span id="C21">实践执行</span>

在部署或更新 BiscuitOS 之后，参考下面命令获得用户空间 PERCPU(UP) 项目:

{% highlight bash %}
cd BiscuitOS/
make linux-5.0-arm32_defconfig
make menuconfig
{% endhighlight %}

![](/assets/PDB/HK/HK000231.png)

选择并进入 "Package  --->",

![](/assets/PDB/HK/HK000239.png)

选择并进入 "percpu  --->",

![](/assets/PDB/HK/HK000240.png)

选择 "PERCPU(UP) Memory Allocator on Userspace  --->", 保存并退出.

选择完毕之后，接下来使用如下命令部署用户空间的 PERCPU(UP) 内存分配器:

{% highlight bash %}
make
cd BiscuitOS/output/linux-5.0-arm32/package/UP_PERCPU_userspace-0.0.1
{% endhighlight %}

执行完 make 命令之后，系统会提示所使用版本对于的安装目录，开发者可以
根据自己的版本参考执行。执行完毕之后，使用如下命令:

{% highlight bash %}
make download
make clean
make
make install
make pack
tree
{% endhighlight %}

![](/assets/PDB/HK/HK000241.png)

执行完上面的代码之后，BiscuitOS 自动部署用户空间 PERCPU(UP) 内存分配器
项目的源码，并将目标文件安装到 BiscuitOS，接下来在 BiscuitOS 上运行
用户空间 PERCPU(UP) 内存分配器，使用如下命令:

{% highlight bash %}
cd BiscuitOS/output/linux-5.0-arm32/
./RunBiscuitOS.sh
{% endhighlight %}

![](/assets/PDB/HK/HK000242.png)

上图就是用户空间 PERCPU(UP) 在 BiscuitOS 运行的效果。

---------------------------------------

###### <span id="C22">实践分析</span>

用户空间的 PERCPU(UP) 实现逻辑是从用户空间的 MEMBLOCK 内存分配器中
分配指定大小的内存，然后将这些内存分成指定大小的内存块，每个内存块
通过 BITMAP 进行管理，其实现如下图:

![](/assets/PDB/HK/HK000223.png)

如上图，map_size 指向的空间就是 PERPCU(UP) 维护的内存，PERCPU(UP) 内存
内存管理器通过 alloc_BITMAP 和 pcpu_block_md 对所有的可用内存进行分配
和释放。用户空间的 PERCPU(UP) 实现了其逻辑，开发者可以从 "main.c" 文件
的 "main()" 函数研究 PERCPU(UP) 的工作原理。

------------------------------------------------

<span id="D"></span>

![](/assets/PDB/BiscuitOS/kernel/IND00000A.jpg)

#### PERCPU(SMP) Memory Allocator

![](/assets/PDB/HK/HK000236.png)

> - [PERCPU(SMP) 内存分配器简介](#D0)
>
> - [用户空间 PERCPU(SMP) 内存分配器实践](#D1)
>
> - [用户空间 PERCPU(SMP) 内存分配器 BiscuitOS 实践](#D2)

![](/assets/PDB/BiscuitOS/kernel/IND000101.jpg)

----------------------------------------------------------

#### <span id="D0">PERCPU(SMP) 内存分配器简介</span>

PERCPU 内存分配器的主要用途就是为每个 CPU 分配本地内存。在 Linux 中，
每当创建一个 percpu 变量或结构的时候，系统都会为每个 CPU 分配同样大小
内存。CPU 操作 percpu 变量对应的私有内存，不存在竞态，因此 CPU 可以不加锁
放心使用各自的私有内存。PERCPU 内存分配器就是为 percpu 变量或结构提供
相应逻辑的内存，因此每个 percpu 会消耗自身大小 CPUS 倍的内存。

PERCPU 内存分配的建立在 MEMBLOCK 内存分配器之后，Buddy 内存分配器之前，
属于比较早期的内存分配器。PERCPU 内存分配器从 MEMBLOCK 内存分配器中
分配一部分物理内存，PERCPU 内存分配器基于 BITMAP 逻辑将内存分成指定
大小的块，每一块内存通过 BITMAP 进行管理。

由于系统存在 UP 单核架构和 SMP 多核架构，因此 PERCPU 内存分配器的实现
逻辑存在一定的差异。对于 SMP 多核架构，那么 PERCPU 内存分配器为每个 CPU
维护其本地内存，其实现逻辑相对于 UP 架构复杂很多，因此对开发者
来说，研究 PERCPU(SMP) 内存分配器相对不友好。

![](/assets/PDB/HK/HK000225.png) 

上图是 PERCPU(SMP) 与其他内存分配器的生命周期，PERCPU(SMP) 在 Linux 
初始阶段依赖 MEMBLOCK 阶段，当系统初始化完毕之后，如果 PERCPU 内存分配
器需要扩充内存的时候，PERCPU 可以从 Buddy 内存分配器或者 VMALLOC 内存
分配器中获得相应的内存。更多 PERCPU 相关的内核实例可以参考如下链接:

> - [PERCPU 内核实践例子 GITHUB](https://github.com/BiscuitOS/HardStack/tree/master/Memory-Allocator/PERCPU)

----------------------------------

<span id="D1"></span>

![](/assets/PDB/BiscuitOS/kernel/IND00000H.jpg)

#### 用户空间 PERCPU(SMP) 内存分配器实践

> - [实践部署](#D10)
>
> - [实践执行](#D11)
>
> - [实践分析](#D12)

![](/assets/PDB/BiscuitOS/kernel/IND000101.jpg)

---------------------------------------

###### <span id="D10">实践部署</span>

在进行用户空间 PERCPU(SMP) 内存分配实践之前，开发者应该准备一台 X86 Linux
主机，主机安装基础的 gcc 和 make 开发工具，参考并执行下面命令:

{% highlight bash %}
mkdir -p BiscuitOS_MEM
cd BiscuitOS_MEM
wget https://gitee.com/BiscuitOS_team/HardStack/raw/Gitee/Memory-Allocator/BiscuitOS_MM.sh
chmod 755 BiscuitOS_MM.sh
{% endhighlight %}

执行完上面的命令之后，会获得名为 "BiscuitOS_MM.sh" 的脚本, 该脚本用来
自动部署实践所需的源码。准备工作一切准备就绪，接下来看下一节内容.

---------------------------------------

###### <span id="D11">实践执行</span>

接下来，执行下面的命令

{% highlight bash %}
./BiscuitOS_MM.sh
{% endhighlight %}

![](/assets/PDB/HK/HK000228.png)

执行完上面的命令会出现上图的对话框，该对话框用于部署不同内存分配器的
源码项目。对于用户空间的 PERCPU(SMP) 内存分配器，这里输入 3 并回车，如果
是第一次执行上面的命令，那么脚本会从网上下载并部署所需的源码。待下载
部署完毕之后，可以在当前目录获得名为 "PERCPU-SMP" 的目录，此时进入该目录，
查看源码如下图:

![](/assets/PDB/HK/HK000237.png)

用户空间 PERCPU(SMP) 内存分配器包含了 14 个文件，"mian.c" 文件包含了用户空间
PERCPU(SMP) 内存分配器的使用例子，开发者可以参考这些例子对 PERCPU(SMP) 内存
分配器进行更多的探索. "mm/percpu.c" 和 "include/linux/percpu.h" 文件
包含了用户空间的核心实现。接下来使用下面命令运行用户空间的 PERCPU(SMP) 内存
分配器:

{% highlight bash %}
cd PERCPU-SMP/
make clean
make
./biscuitos
{% endhighlight %}

![](/assets/PDB/HK/HK000238.png)

以上就是用户空间 PERCPU(SMP) 运行情况，开发者可以从 "main.c" 文件的 "main()"
函数作为入口来实践 PERCPU(SMP) 的实现过程。

---------------------------------------

###### <span id="D12">实践分析</span>

用户空间的 PERCPU(SMP) 实现逻辑是从用户空间的 MEMBLOCK 内存分配器中
分配指定大小的内存，然后将这些内存分成指定大小的内存块，每个内存块
通过 BITMAP 进行管理，其实现如下图:

![](/assets/PDB/HK/HK000223.png)

如上图，map_size 指向的空间就是 PERPCU(UP) 维护的内存，PERCPU(SMP) 内存
内存管理器通过 alloc_BITMAP 和 pcpu_block_md 对所有的可用内存进行分配
和释放。用户空间的 PERCPU(SMP) 实现了其逻辑，开发者可以从 "main.c" 文件
的 "main()" 函数研究 PERCPU(SMP) 的工作原理。

用户空间的 PERCPU(SMP) 支持多 CPU 的设定，开发者可以参考如下命令进行
不同 CPU 时 PERCPU(SMP) 的行为:

###### 8 CPUs

{% highlight bash %}
cd PERCPU-SMP/
make clean
make CPUS=8
./biscuitos
{% endhighlight %}

![](/assets/PDB/HK/HK000243.png)

上图是 8 核模式下用户空间 PERCPU(SMP) 内存分配器的运行效果.

###### 32 CPUs

{% highlight bash %}
cd PERCPU-SMP/
make clean
make CPUS=32
./biscuitos
{% endhighlight %}

![](/assets/PDB/HK/HK000244.png)

上图是 32 核模式下用户空间 PERCPU(SMP) 内存分配器的运行效果.

###### 128 CPUs

{% highlight bash %}
cd PERCPU-SMP/
make clean
make CPUS=128
./biscuitos
{% endhighlight %}

![](/assets/PDB/HK/HK000245.png)

上图是 128 核模式下用户空间 PERCPU(SMP) 内存分配器的运行效果.

###### 256 CPUs

{% highlight bash %}
cd PERCPU-SMP/
make clean
make CPUS=256
./biscuitos
{% endhighlight %}

![](/assets/PDB/HK/HK000246.png)

上图是 256 核模式下用户空间 PERCPU(SMP) 内存分配器的运行效果.

----------------------------------

<span id="D2"></span>

![](/assets/PDB/BiscuitOS/kernel/IND00000J.jpg)

#### 用户空间 PERCPU(SMP) 内存分配器 BiscuitOS 实践

> - [实践部署](#D20)
>
> - [实践执行](#D21)
>
> - [实践分析](#D22)

![](/assets/PDB/BiscuitOS/kernel/IND000101.jpg)

-----------------------------------------------

##### <span id="D20">实践部署</span>

BiscuitOS 已经支持用户空间的 PERCPU(SMP) 内存分配器实践，如果
还未部署 BiscuitOS，请参考下列文章, 如果已经部署 BiscuitOS
项目的童鞋请使用 "git pull" 更新 BiscuitOS，并查看下一节:

> - [BiscuitOS linux 5.0 部署介绍](/blog/Linux-5.0-arm32-Usermanual/)

-----------------------------------------------

##### <span id="D21">实践执行</span>

在部署或更新 BiscuitOS 之后，参考下面命令获得用户空间 PERCPU(SMP) 项目:

{% highlight bash %}
cd BiscuitOS/
make linux-5.0-arm32_defconfig
make menuconfig
{% endhighlight %}

![](/assets/PDB/HK/HK000231.png)

选择并进入 "Package  --->",

![](/assets/PDB/HK/HK000239.png)

选择并进入 "percpu  --->",

![](/assets/PDB/HK/HK000247.png)

选择 "PERCPU(UP) Memory Allocator on Userspace  --->", 保存并退出.

选择完毕之后，接下来使用如下命令部署用户空间的 PERCPU(SMP) 内存分配器:

{% highlight bash %}
make
cd BiscuitOS/output/linux-5.0-arm32/package/SMP_PERCPU_userspace-0.0.1
{% endhighlight %}

执行完 make 命令之后，系统会提示所使用版本对于的安装目录，开发者可以
根据自己的版本参考执行。执行完毕之后，使用如下命令:

{% highlight bash %}
make download
make clean
make
make install
make pack
tree
{% endhighlight %}

![](/assets/PDB/HK/HK000248.png)

执行完上面的代码之后，BiscuitOS 自动部署用户空间 PERCPU(SMP) 内存分配器
项目的源码，并将目标文件安装到 BiscuitOS，接下来在 BiscuitOS 上运行
用户空间 PERCPU(SMP) 内存分配器，使用如下命令:

{% highlight bash %}
cd BiscuitOS/output/linux-5.0-arm32/
./RunBiscuitOS.sh
{% endhighlight %}

![](/assets/PDB/HK/HK000249.png)

上图就是用户空间 PERCPU(SMP) 在 BiscuitOS 运行的效果。

---------------------------------------

###### <span id="D22">实践分析</span>

用户空间的 PERCPU(SMP) 实现逻辑是从用户空间的 MEMBLOCK 内存分配器中
分配指定大小的内存，然后将这些内存分成指定大小的内存块，每个内存块
通过 BITMAP 进行管理，其实现如下图:

![](/assets/PDB/HK/HK000223.png)

如上图，map_size 指向的空间就是 PERPCU(UP) 维护的内存，PERCPU(SMP) 内存
内存管理器通过 alloc_BITMAP 和 pcpu_block_md 对所有的可用内存进行分配
和释放。用户空间的 PERCPU(SMP) 实现了其逻辑，开发者可以从 "main.c" 文件
的 "main()" 函数研究 PERCPU(SMP) 的工作原理。

-----------------------------------------------


<span id="E"></span>

![](/assets/PDB/BiscuitOS/kernel/IND00000C.jpg)

#### Buddy-Normal Memory Allocator

![](/assets/PDB/HK/HK000250.png)

> - [Buddy-Normal 内存分配器简介](#E0)
>
> - [用户空间 Buddy-Normal 内存分配器实践](#E1)
>
> - [用户空间 Buddy-Normal 内存分配器 BiscuitOS 实践](#E2)

![](/assets/PDB/BiscuitOS/kernel/IND000101.jpg)

----------------------------------------------------------

#### <span id="E0">Buddy-Normal 内存分配器简介</span>

Buddy 内存分配器用于管理可用物理内存, 其按 "页" 大小提供内存分配和回收。
MEMBLOCK 物理内存分配器完成 Linux 内存启动初期的任务之后，将物理内存按
"页" 大小进行划分，并将物理内存进行不同页类型进行划分。MEMBLOCK 将预留
区内的物理内存划分为 "预留页"，而将其余大部分可用物理内存划分为 "可用物理页"。
可用物理页可能来自低端物理内存，即 Normal 区域; 同理可用物理页可能来自
高端物理内存或 DMA 内存，这里对 DMA 区域不做研究，因此高端物理内存称为
HighMEM。

MEMBLOCK 内存分配器在生命周期末尾，将所有 "可用物理页"　按一定数量块传递
给 Buddy 内存管理器，这里的数量块按 2 的阶乘。系统使用 zone 的概念将物理
内存页分作: Normal Zone、DMA Zone 和 Highmem Zone，每个 Zone 维护着一个
freelist 链表，这个链表含有多个成员，每个成员是 2 的阶乘，并从 2^0、 2^1、
2^2 .... 2^10、2^11 的顺序排列。每个成员由各自维护一个链表，链表中的成员
即一个可用的物理页，页的大小正好是 2^n. 如下图:

![](/assets/PDB/HK/HK000250.png)

Buddy 内存分配器就负责管理每个 zone 上面的 freelist，当系统需要指定大小
的内存时候，Buddy 内存分配器就去 freelist 对应的节点上找，看这个节点对应
的链表上有没有可用的物理内存页块，如果有，那么直接将这个可用的内存页块从
链表中移除返回给调用者使用; 如果对应的节点的链表上没有可用的物理页块，那么
Buddy 内存分配器就会继续向更大体积的节点查找一个可用的物理也块，如果找到，
那么 Buddy 内存分配器会将更大体积的物理页块拆除同等体积的两块，然后将后一
物理页块插入到第一级的节点上，而如果此时前一物理页块的体积正好是调用者所需
的大小，那么将这个物理页块返回给调用者; 如果此时前一物理页块的体积比调用者
需求的还大，那么 Buddy 内存管理器继续循环将大的物理页块再拆成两块，同样
将后一块插入到下一级节点上，直到前一物理页块的体积与调用者需求的一致才
停止。经过上面的操作之后，Zone 的 freelist 各节点上都保存一定量的成员。

当系统将释放物理页块给 Buddy 内存管理器的时候，Buddy 内存分配器首先定位
这些物理页块属于哪个 Zone，确定 Zone 之后，Buddy 根据物理页块的大小，将
其加入到 freelist 对应的节点上，添加过程中，Buddy 内存管理器会查找
物理页块对应的兄弟页块是否页在 freelist 对应的节点上，如果在，那么
Buddy 内存管理器就将两个物理页块合并成一个物理页块，并将新的物理页块
插入到更高一级节点上; 如果新物理页块发现新兄弟物理页块也在链表中，
那么 Buddy 继续将其合并成新的物理页块并向高一级节点合并，知道没有
兄弟物理页块或已经到达 freelist 最大节点时，Buddy 内存分配器才会停止
合并; 如果合并生成的新物理页块的兄弟物理页块不存在 freelist 节点上，
那么新物理页块就插入到 freelist 对应的节点。

以上便是 Buddy 内存管理器的基本逻辑。在用户空间的 Buddy-Normal 分配器
项目中，Buddy-Normal 只维护 Zone-Normal 的物理页块，其余逻辑与 Buddy
内存分配器一致。

----------------------------------

<span id="E1"></span>

![](/assets/PDB/BiscuitOS/kernel/IND00000H.jpg)

#### 用户空间 Buddy-Normal 内存分配器实践

> - [实践部署](#E10)
>
> - [实践执行](#E11)
>
> - [实践分析](#E12)

![](/assets/PDB/BiscuitOS/kernel/IND000101.jpg)

---------------------------------------

###### <span id="E10">实践部署</span>

在进行用户空间 Buddy-Normal 内存分配实践之前，开发者应该准备一台 X86 Linux
主机，主机安装基础的 gcc 和 make 开发工具，参考并执行下面命令:

{% highlight bash %}
mkdir -p BiscuitOS_MEM
cd BiscuitOS_MEM
wget https://gitee.com/BiscuitOS_team/HardStack/raw/Gitee/Memory-Allocator/BiscuitOS_MM.sh
chmod 755 BiscuitOS_MM.sh
{% endhighlight %}

执行完上面的命令之后，会获得名为 "BiscuitOS_MM.sh" 的脚本, 该脚本用来
自动部署实践所需的源码。准备工作一切准备就绪，接下来看下一节内容.

---------------------------------------

###### <span id="E11">实践执行</span>

接下来，执行下面的命令

{% highlight bash %}
./BiscuitOS_MM.sh
{% endhighlight %}

![](/assets/PDB/HK/HK000228.png)

执行完上面的命令会出现上图的对话框，该对话框用于部署不同内存分配器的
源码项目。对于用户空间的 Buddy-Normal 内存分配器，这里输入 4 并回车，如果
是第一次执行上面的命令，那么脚本会从网上下载并部署所需的源码。待下载
部署完毕之后，可以在当前目录获得名为 "Buddy-Normal" 的目录，此时进入该目录，
查看源码如下图:

![](/assets/PDB/HK/HK000251.png)

用户空间 Buddy-Normal 内存分配器包含了 7 个文件，"mian.c" 文件包含了用户空间
Buddy-Normal 内存分配器的使用例子，开发者可以参考这些例子对 Buddy-Normal 内存
分配器进行更多的探索. "mm/buddy.c" 和 "include/linux/buddy.h" 文件
包含了用户空间的核心实现。接下来使用下面命令运行用户空间的 Buddy-Normal 内存
分配器:

{% highlight bash %}
cd Buddy-Normal
make clean
make
./biscuitos
{% endhighlight %}

![](/assets/PDB/HK/HK000252.png)

以上就是用户空间 Buddy-Normal 运行情况，开发者可以从 "main.c" 文件的 "main()"
函数作为入口来实践 Buddy-Normal 的实现过程。

---------------------------------------

###### <span id="E12">实践分析</span>

用户空间的 Buddy-Normal 内存分配器实现逻辑是使用 "malloc()" 函数在用户
空间分配一定长度的虚拟内存，然后将这段虚拟内存按页的大小，即 PAGE_SZIE
的大小计算出这段虚拟内存可以分成的页的总数，然后创建一个指针 mem_map 指
向这段虚拟内存的起始地址，这个指针是一个 struct page 的数组，数组的长度
正好是 "页的总数 * struct page" 的总长度，如下图:

![](/assets/PDB/HK/HK000253.png)

用户空间的 Buddy-Normal 内存管理器就是基于 mem_map[] 数组，将所有的物理
页加入到 Buddy-Normal 内存分配器里。接着可以使用用户空间 Buddy-Normal 
内存分配器提供的 "__alloc_pages()" 和 "__free_pages()" 函数进行内存的
分配和释放。

----------------------------------

<span id="E2"></span>

![](/assets/PDB/BiscuitOS/kernel/IND00000S.jpg)

#### 用户空间 Buddy-Normal 内存分配器 BiscuitOS 实践

> - [实践部署](#E20)
>
> - [实践执行](#E21)
>
> - [实践分析](#E22)

![](/assets/PDB/BiscuitOS/kernel/IND000101.jpg)

-----------------------------------------------

##### <span id="E20">实践部署</span>

BiscuitOS 已经支持用户空间的 Buddy-Normal 内存分配器实践，如果
还未部署 BiscuitOS，请参考下列文章, 如果已经部署 BiscuitOS
项目的童鞋请使用 "git pull" 更新 BiscuitOS，并查看下一节:

> - [BiscuitOS linux 5.0 部署介绍](/blog/Linux-5.0-arm32-Usermanual/)

-----------------------------------------------

##### <span id="E21">实践执行</span>

在部署或更新 BiscuitOS 之后，参考下面命令获得用户空间 Buddy-Normal 项目:

{% highlight bash %}
cd BiscuitOS/
make linux-5.0-arm32_defconfig
make menuconfig
{% endhighlight %}

![](/assets/PDB/HK/HK000231.png)

选择并进入 "Package  --->",

![](/assets/PDB/HK/HK000254.png)

选择并进入 "Buddy Physical Memory Allocator  --->",

![](/assets/PDB/HK/HK000255.png)

选择 "Buddy Allocator on Userspace  --->", 保存并退出.

选择完毕之后，接下来使用如下命令部署用户空间的 Buddy-Normal 内存分配器:

{% highlight bash %}
make
cd BiscuitOS/output/linux-5.0-arm32/package/buddy_userspace-0.0.1
{% endhighlight %}

执行完 make 命令之后，系统会提示所使用版本对于的安装目录，开发者可以
根据自己的版本参考执行。执行完毕之后，使用如下命令:

{% highlight bash %}
make download
make clean
make
make install
make pack
tree
{% endhighlight %}

![](/assets/PDB/HK/HK000256.png)

执行完上面的代码之后，BiscuitOS 自动部署用户空间 Buddy-Normal 内存分配器
项目的源码，并将目标文件安装到 BiscuitOS，接下来在 BiscuitOS 上运行
用户空间 Buddy-Normal 内存分配器，使用如下命令:

{% highlight bash %}
cd BiscuitOS/output/linux-5.0-arm32/
./RunBiscuitOS.sh
{% endhighlight %}

![](/assets/PDB/HK/HK000257.png)

上图就是用户空间 Buddy-Normal 在 BiscuitOS 运行的效果。

---------------------------------------

###### <span id="E22">实践分析</span>

用户空间的 Buddy-Normal 内存分配器实现逻辑是使用 "malloc()" 函数在用户
空间分配一定长度的虚拟内存，然后将这段虚拟内存按页的大小，即 PAGE_SZIE
的大小计算出这段虚拟内存可以分成的页的总数，然后创建一个指针 mem_map 指
向这段虚拟内存的起始地址，这个指针是一个 struct page 的数组，数组的长度
正好是 "页的总数 * struct page" 的总长度，如下图:

![](/assets/PDB/HK/HK000253.png)

用户空间的 Buddy-Normal 内存管理器就是基于 mem_map[] 数组，将所有的物理
页加入到 Buddy-Normal 内存分配器里。接着可以使用用户空间 Buddy-Normal
内存分配器提供的 "__alloc_pages()" 和 "__free_pages()" 函数进行内存的
分配和释放。

-----------------------------------------------


<span id="F"></span>

![](/assets/PDB/BiscuitOS/kernel/IND00000R.jpg)

#### Buddy-HighMEM Memory Allocator

![](/assets/PDB/HK/HK000250.png)

> - [Buddy-HighMEM 内存分配器简介](#F0)
>
> - [用户空间 Buddy-HighMEM 内存分配器实践](#F1)
>
> - [用户空间 Buddy-HighMEM 内存分配器 BiscuitOS 实践](#F2)

![](/assets/PDB/BiscuitOS/kernel/IND000101.jpg)

----------------------------------------------------------

#### <span id="F0">Buddy-HighMEM 内存分配器简介</span>

Buddy 内存分配器用于管理可用物理内存, 其按 "页" 大小提供内存分配和回收。
MEMBLOCK 物理内存分配器完成 Linux 内存启动初期的任务之后，将物理内存按
"页" 大小进行划分，并将物理内存进行不同页类型进行划分。MEMBLOCK 将预留
区内的物理内存划分为 "预留页"，而将其余大部分可用物理内存划分为 "可用物理页"。
可用物理页可能来自低端物理内存，即 Normal 区域; 同理可用物理页可能来自
高端物理内存或 DMA 内存，这里对 DMA 区域不做研究，因此高端物理内存称为
HighMEM。

MEMBLOCK 内存分配器在生命周期末尾，将所有 "可用物理页"　按一定数量块传递
给 Buddy 内存管理器，这里的数量块按 2 的阶乘。系统使用 zone 的概念将物理
内存页分作: Normal Zone、DMA Zone 和 Highmem Zone，每个 Zone 维护着一个
freelist 链表，这个链表含有多个成员，每个成员是 2 的阶乘，并从 2^0、 2^1、
2^2 .... 2^10、2^11 的顺序排列。每个成员由各自维护一个链表，链表中的成员
即一个可用的物理页，页的大小正好是 2^n. 如下图:

![](/assets/PDB/HK/HK000250.png)

Buddy 内存分配器就负责管理每个 zone 上面的 freelist，当系统需要指定大小
的内存时候，Buddy 内存分配器就去 freelist 对应的节点上找，看这个节点对应
的链表上有没有可用的物理内存页块，如果有，那么直接将这个可用的内存页块从
链表中移除返回给调用者使用; 如果对应的节点的链表上没有可用的物理页块，那么
Buddy 内存分配器就会继续向更大体积的节点查找一个可用的物理也块，如果找到，
那么 Buddy 内存分配器会将更大体积的物理页块拆除同等体积的两块，然后将后一
物理页块插入到第一级的节点上，而如果此时前一物理页块的体积正好是调用者所需
的大小，那么将这个物理页块返回给调用者; 如果此时前一物理页块的体积比调用者
需求的还大，那么 Buddy 内存管理器继续循环将大的物理页块再拆成两块，同样
将后一块插入到下一级节点上，直到前一物理页块的体积与调用者需求的一致才
停止。经过上面的操作之后，Zone 的 freelist 各节点上都保存一定量的成员。

当系统将释放物理页块给 Buddy 内存管理器的时候，Buddy 内存分配器首先定位
这些物理页块属于哪个 Zone，确定 Zone 之后，Buddy 根据物理页块的大小，将
其加入到 freelist 对应的节点上，添加过程中，Buddy 内存管理器会查找
物理页块对应的兄弟页块是否页在 freelist 对应的节点上，如果在，那么
Buddy 内存管理器就将两个物理页块合并成一个物理页块，并将新的物理页块
插入到更高一级节点上; 如果新物理页块发现新兄弟物理页块也在链表中，
那么 Buddy 继续将其合并成新的物理页块并向高一级节点合并，知道没有
兄弟物理页块或已经到达 freelist 最大节点时，Buddy 内存分配器才会停止
合并; 如果合并生成的新物理页块的兄弟物理页块不存在 freelist 节点上，
那么新物理页块就插入到 freelist 对应的节点。

以上便是 Buddy 内存管理器的基本逻辑。在用户空间的 Buddy-HighMEM 分配器
项目中，Buddy-HighMEM 不仅维护 Zone-Normal 的物理页块，还维护了 
Zone-HighMEM 的物理页块。其余逻辑与 Buddy 内存分配器一致。

----------------------------------

<span id="F1"></span>

![](/assets/PDB/BiscuitOS/kernel/IND00000H.jpg)

#### 用户空间 Buddy-HighMEM 内存分配器实践

> - [实践部署](#F10)
>
> - [实践执行](#F11)
>
> - [实践分析](#F12)

![](/assets/PDB/BiscuitOS/kernel/IND000101.jpg)

---------------------------------------

###### <span id="F10">实践部署</span>

在进行用户空间 Buddy-HighMEM 内存分配实践之前，开发者应该准备一台 X86 Linux
主机，主机安装基础的 gcc 和 make 开发工具，参考并执行下面命令:

{% highlight bash %}
mkdir -p BiscuitOS_MEM
cd BiscuitOS_MEM
wget https://gitee.com/BiscuitOS_team/HardStack/raw/Gitee/Memory-Allocator/BiscuitOS_MM.sh
chmod 755 BiscuitOS_MM.sh
{% endhighlight %}

执行完上面的命令之后，会获得名为 "BiscuitOS_MM.sh" 的脚本, 该脚本用来
自动部署实践所需的源码。准备工作一切准备就绪，接下来看下一节内容.

---------------------------------------

###### <span id="F11">实践执行</span>

接下来，执行下面的命令

{% highlight bash %}
./BiscuitOS_MM.sh
{% endhighlight %}

![](/assets/PDB/HK/HK000228.png)

执行完上面的命令会出现上图的对话框，该对话框用于部署不同内存分配器的
源码项目。对于用户空间的 Buddy-HighMEM 内存分配器，这里输入 5 并回车，如果
是第一次执行上面的命令，那么脚本会从网上下载并部署所需的源码。待下载
部署完毕之后，可以在当前目录获得名为 "Buddy-HighMEM" 的目录，此时进入该目录，
查看源码如下图:

![](/assets/PDB/HK/HK000251.png)

用户空间 Buddy-HighMEM 内存分配器包含了 7 个文件，"mian.c" 文件包含了用户空间
Buddy-HighMEM 内存分配器的使用例子，开发者可以参考这些例子对 Buddy-HighMEM 内存
分配器进行更多的探索. "mm/buddy.c" 和 "include/linux/buddy.h" 文件
包含了用户空间的核心实现。接下来使用下面命令运行用户空间的 Buddy-HighMEM 内存
分配器:

{% highlight bash %}
cd Buddy-HighMEM
make clean
make
./biscuitos
{% endhighlight %}

![](/assets/PDB/HK/HK000258.png)

以上就是用户空间 Buddy-HighMEM 运行情况，开发者可以从 "main.c" 文件的 "main()"
函数作为入口来实践 Buddy-HighMEM 的实现过程。

---------------------------------------

###### <span id="F12">实践分析</span>

用户空间的 Buddy-HighMEM 内存分配器实现逻辑是使用 "malloc()" 函数在用户
空间分配两段一定长度的虚拟内存，然后将这些段虚拟内存按页的大小，即 
PAGE_SZIE 的大小计算出每段虚拟内存可以分成的页的总数，然后创建一个指针 
mem_map 指向这段虚拟内存的起始地址，这个指针是一个 struct page 的数组，
数组的长度正好是 "两段页的总数 * struct page" 的总长度，如下图:

![](/assets/PDB/HK/HK000259.png)

用户空间的 Buddy-HighMEM 内存管理器就是基于 mem_map[] 数组，将所有的物理
页加入到 Buddy-HighMEM 内存分配器里。其中 MEMORY_SIZE 区域的物理页加入到
Zone-Normal 的 freelist 里，而 HigMem 的物理页加入到 Zone-HighMEM 的
freelist 里面。接着可以使用用户空间 Buddy-HighMEM 内存分配器提供的 
"__alloc_pages()" 和 "__free_pages()" 函数从 HighMEM 或着 Normal 区域里
分配和释放物理页。

----------------------------------

<span id="F2"></span>

![](/assets/PDB/BiscuitOS/kernel/IND00000O.jpg)

#### 用户空间 Buddy-HighMEM 内存分配器 BiscuitOS 实践

> - [实践部署](#F20)
>
> - [实践执行](#F21)
>
> - [实践分析](#F22)

![](/assets/PDB/BiscuitOS/kernel/IND000101.jpg)

-----------------------------------------------

##### <span id="F20">实践部署</span>

BiscuitOS 已经支持用户空间的 Buddy-HighMEM 内存分配器实践，如果
还未部署 BiscuitOS，请参考下列文章, 如果已经部署 BiscuitOS
项目的童鞋请使用 "git pull" 更新 BiscuitOS，并查看下一节:

> - [BiscuitOS linux 5.0 部署介绍](/blog/Linux-5.0-arm32-Usermanual/)

-----------------------------------------------

##### <span id="F21">实践执行</span>

在部署或更新 BiscuitOS 之后，参考下面命令获得用户空间 Buddy-HighMEM 项目:

{% highlight bash %}
cd BiscuitOS/
make linux-5.0-arm32_defconfig
make menuconfig
{% endhighlight %}

![](/assets/PDB/HK/HK000231.png)

选择并进入 "Package  --->",

![](/assets/PDB/HK/HK000254.png)

选择并进入 "Buddy Physical Memory Allocator  --->",

![](/assets/PDB/HK/HK000260.png)

选择 "Buddy Allocator (Highmem) on Userspace  --->", 保存并退出.

选择完毕之后，接下来使用如下命令部署用户空间的 Buddy-HighMEM 内存分配器:

{% highlight bash %}
make
cd BiscuitOS/output/linux-5.0-arm32/package/buddy_highmem_userspace-0.0.1
{% endhighlight %}

执行完 make 命令之后，系统会提示所使用版本对于的安装目录，开发者可以
根据自己的版本参考执行。执行完毕之后，使用如下命令:

{% highlight bash %}
make download
make clean
make
make install
make pack
tree
{% endhighlight %}

![](/assets/PDB/HK/HK000261.png)

执行完上面的代码之后，BiscuitOS 自动部署用户空间 Buddy-HighMEM 内存分配器
项目的源码，并将目标文件安装到 BiscuitOS，接下来在 BiscuitOS 上运行
用户空间 Buddy-HighMEM 内存分配器，使用如下命令:

{% highlight bash %}
cd BiscuitOS/output/linux-5.0-arm32/
./RunBiscuitOS.sh
{% endhighlight %}

![](/assets/PDB/HK/HK000262.png)

上图就是用户空间 Buddy-HighMEM 在 BiscuitOS 运行的效果。

---------------------------------------

###### <span id="F22">实践分析</span>

用户空间的 Buddy-HighMEM 内存分配器实现逻辑是使用 "malloc()" 函数在用户
空间分配两段一定长度的虚拟内存，然后将这些段虚拟内存按页的大小，即
PAGE_SZIE 的大小计算出每段虚拟内存可以分成的页的总数，然后创建一个指针
mem_map 指向这段虚拟内存的起始地址，这个指针是一个 struct page 的数组，
数组的长度正好是 "两段页的总数 * struct page" 的总长度，如下图:

![](/assets/PDB/HK/HK000259.png)

用户空间的 Buddy-HighMEM 内存管理器就是基于 mem_map[] 数组，将所有的物理
页加入到 Buddy-HighMEM 内存分配器里。其中 MEMORY_SIZE 区域的物理页加入到
Zone-Normal 的 freelist 里，而 HigMem 的物理页加入到 Zone-HighMEM 的
freelist 里面。接着可以使用用户空间 Buddy-HighMEM 内存分配器提供的
"__alloc_pages()" 和 "__free_pages()" 函数从 HighMEM 或着 Normal 区域里
分配和释放物理页。

-----------------------------------------------


<span id="G"></span>

![](/assets/PDB/BiscuitOS/kernel/IND00000E.jpg)

#### PCP Memory Allocator

![](/assets/PDB/HK/HK000263.png)

> - [PCP 内存分配器简介](#G0)
>
> - [用户空间 PCP 内存分配器实践](#G1)
>
> - [用户空间 PCP 内存分配器 BiscuitOS 实践](#G2)

![](/assets/PDB/BiscuitOS/kernel/IND000101.jpg)

----------------------------------------------------------

#### <span id="G0">PCP 内存分配器简介</span>

PCP 内存分配器用来分配和回收一个物理页。在内核中分配和回收一个物理页是一个
很频繁的操作，如果每次分配或回收一个物理页都向 Buddy 内存分配器请求，
那么运气好的话 freelist 2^0 节点上正好有可用的物理页，运气最差的时候
需要从 freelist 最大物理块节点拆分合并处所需的物理页，这样极大增加了
系统的负担，于是内核就使用 PCP 内存分配器在系统上维护一个冷热页表，
所谓冷热页表其实就是系统前几次从 Buddy 分配 2^0 的物理页，用完之后进行
释放，此时 PCP 内存分配器会将这些物理页劫持下来加入到自己的维护的冷热
页表内，当系统再次需要一个物理页时，系统就可以快速从 PCP 维护的冷热
页表上获取。当系统多次释放一个物理页到 PCP 冷热页表之后，PCP 会将一部分
物理页释放回去给 Buddy 内存管理器，以此保证系统能高效使用一个物理页。
其逻辑如下图:

![](/assets/PDB/HK/HK000263.png)

以上便是 PCP 内存管理器的基本逻辑。在用户空间的 PCP 分配器
项目中，PCP 维护这冷热页表，极大提高了一个物理页的分配效率。

----------------------------------

<span id="G1"></span>

![](/assets/PDB/BiscuitOS/kernel/IND00000T.jpg)

#### 用户空间 PCP 内存分配器实践

> - [实践部署](#G10)
>
> - [实践执行](#G11)
>
> - [实践分析](#G12)

![](/assets/PDB/BiscuitOS/kernel/IND000101.jpg)

---------------------------------------

###### <span id="G10">实践部署</span>

在进行用户空间 PCP 内存分配实践之前，开发者应该准备一台 X86 Linux
主机，主机安装基础的 gcc 和 make 开发工具，参考并执行下面命令:

{% highlight bash %}
mkdir -p BiscuitOS_MEM
cd BiscuitOS_MEM
wget https://gitee.com/BiscuitOS_team/HardStack/raw/Gitee/Memory-Allocator/BiscuitOS_MM.sh
chmod 755 BiscuitOS_MM.sh
{% endhighlight %}

执行完上面的命令之后，会获得名为 "BiscuitOS_MM.sh" 的脚本, 该脚本用来
自动部署实践所需的源码。准备工作一切准备就绪，接下来看下一节内容.

---------------------------------------

###### <span id="G11">实践执行</span>

接下来，执行下面的命令

{% highlight bash %}
./BiscuitOS_MM.sh
{% endhighlight %}

![](/assets/PDB/HK/HK000228.png)

执行完上面的命令会出现上图的对话框，该对话框用于部署不同内存分配器的
源码项目。对于用户空间的 PCP 内存分配器，这里输入 6 并回车，如果
是第一次执行上面的命令，那么脚本会从网上下载并部署所需的源码。待下载
部署完毕之后，可以在当前目录获得名为 "PCP" 的目录，此时进入该目录，
查看源码如下图:

![](/assets/PDB/HK/HK000251.png)

用户空间 PCP 内存分配器包含了 7 个文件，"mian.c" 文件包含了用户空间
PCP 内存分配器的使用例子，开发者可以参考这些例子对 PCP 内存
分配器进行更多的探索. "mm/buddy.c" 和 "include/linux/buddy.h" 文件
包含了用户空间的核心实现。接下来使用下面命令运行用户空间的 PCP 内存
分配器:

{% highlight bash %}
cd PCP
make clean
make
./biscuitos
{% endhighlight %}

![](/assets/PDB/HK/HK000264.png)

以上就是用户空间 PCP 运行情况，开发者可以从 "main.c" 文件的 "main()"
函数作为入口来实践 PCP 的实现过程。

---------------------------------------

###### <span id="G12">实践分析</span>

用户空间的 PCP 内存分配器是基于用户空间的 Buddy-Normal 内存分配器，
PCP 主要是为 Zone_Normal 创建了一个 PCP 热页链表。每当调用者申请一个
物理页的时候，PCP 首先在自己的 PCP 热页链表上查找可用的物理页，如果
找到那么直接返回给调用者; 如果没有找到，那么 PCP 会从用户空间的
Buddy-Normal 内存分配器中分配一定数量的单个物理页到 PCP 的热页链表
中维护，最后在从 PCP 热页链表中返回一个物理物理页给调用者。如下图:

![](/assets/PDB/HK/HK000263.png)

当调用者返回一个物理页给用户空间的 PCP 内存分配器，此时用户空间
PCP 内存分配器维护的热页链表的数量已经超过一定数量，那么用户空间
的 PCP 内存分配器会将一定数量的物理页归还给用户空间的 Buddy 内存
分配器。通过上面的逻辑系统能够高效的获取或释放一个物理页。

----------------------------------

<span id="G2"></span>

![](/assets/PDB/BiscuitOS/kernel/IND00000M.jpg)

#### 用户空间 PCP 内存分配器 BiscuitOS 实践

> - [实践部署](#G20)
>
> - [实践执行](#G21)
>
> - [实践分析](#G22)

![](/assets/PDB/BiscuitOS/kernel/IND000101.jpg)

-----------------------------------------------

##### <span id="G20">实践部署</span>

BiscuitOS 已经支持用户空间的 PCP 内存分配器实践，如果
还未部署 BiscuitOS，请参考下列文章, 如果已经部署 BiscuitOS
项目的童鞋请使用 "git pull" 更新 BiscuitOS，并查看下一节:

> - [BiscuitOS linux 5.0 部署介绍](/blog/Linux-5.0-arm32-Usermanual/)

-----------------------------------------------

##### <span id="G21">实践执行</span>

在部署或更新 BiscuitOS 之后，参考下面命令获得用户空间 PCP 项目:

{% highlight bash %}
cd BiscuitOS/
make linux-5.0-arm32_defconfig
make menuconfig
{% endhighlight %}

![](/assets/PDB/HK/HK000231.png)

选择并进入 "Package  --->",

![](/assets/PDB/HK/HK000254.png)

选择并进入 "Buddy Physical Memory Allocator  --->",

![](/assets/PDB/HK/HK000265.png)

选择 "PCP Allocator on Userspace  --->", 保存并退出.

选择完毕之后，接下来使用如下命令部署用户空间的 PCP 内存分配器:

{% highlight bash %}
make
cd BiscuitOS/output/linux-5.0-arm32/package/PCP_userspace-0.0.1
{% endhighlight %}

执行完 make 命令之后，系统会提示所使用版本对于的安装目录，开发者可以
根据自己的版本参考执行。执行完毕之后，使用如下命令:

{% highlight bash %}
make download
make clean
make
make install
make pack
tree
{% endhighlight %}

![](/assets/PDB/HK/HK000266.png)

执行完上面的代码之后，BiscuitOS 自动部署用户空间 PCP 内存分配器
项目的源码，并将目标文件安装到 BiscuitOS，接下来在 BiscuitOS 上运行
用户空间 PCP 内存分配器，使用如下命令:

{% highlight bash %}
cd BiscuitOS/output/linux-5.0-arm32/
./RunBiscuitOS.sh
{% endhighlight %}

![](/assets/PDB/HK/HK000267.png)

上图就是用户空间 PCP 在 BiscuitOS 运行的效果。

---------------------------------------

###### <span id="G22">实践分析</span>

用户空间的 PCP 内存分配器是基于用户空间的 Buddy-Normal 内存分配器，
PCP 主要是为 Zone_Normal 创建了一个 PCP 热页链表。每当调用者申请一个
物理页的时候，PCP 首先在自己的 PCP 热页链表上查找可用的物理页，如果
找到那么直接返回给调用者; 如果没有找到，那么 PCP 会从用户空间的
Buddy-Normal 内存分配器中分配一定数量的单个物理页到 PCP 的热页链表
中维护，最后在从 PCP 热页链表中返回一个物理物理页给调用者。如下图:

![](/assets/PDB/HK/HK000263.png)

当调用者返回一个物理页给用户空间的 PCP 内存分配器，此时用户空间
PCP 内存分配器维护的热页链表的数量已经超过一定数量，那么用户空间
的 PCP 内存分配器会将一定数量的物理页归还给用户空间的 Buddy 内存
分配器。通过上面的逻辑系统能够高效的获取或释放一个物理页。

-----------------------------------------------

<span id="H"></span>

![](/assets/PDB/BiscuitOS/kernel/IND00000S.jpg)

#### Slub Memory Allocator

![](/assets/PDB/HK/HK000268.png)

> - [Slub 内存分配器简介](#H0)
>
> - [用户空间 Slub 内存分配器实践](#H1)
>
> - [用户空间 Slub 内存分配器 BiscuitOS 实践](#H2)

![](/assets/PDB/BiscuitOS/kernel/IND000101.jpg)

----------------------------------------------------------

#### <span id="H0">Slub 内存分配器简介</span>

Slub 内存分配器是为内核提供小粒度的功能。内核中经常要为小于 PAGE_SIZE
的变量或结构分配内存，这些内存小则几个字节，多则几百个几千个字节。对于
不同长度的小粒度分配，Slub 内存分配器正好用于这些需求。Slub 内存分配器
使用 "kmem_cache" 和 "kmem_cache_node" 对这些小粒度的内存进行统一管理，
并作为 Kmem_cache、kmalloc 和 Name 内存分配器的基础分配器。

Slub 内存分配器是基于 Buddy 内存分配器，Slub 内存分配器从 Buddy 内存分配
中获得指定大小的内存，并将这些内存使用 "kmem_cache" 进行管理，也就所谓的
缓存，每个 "kmem_cache" 管理长度相同的小粒度内存分配和回收，这也正好符合
内核对有的小粒度内存经常分配的需求。内核也提供小粒度而不经常分配的内存
管理器，也就是通常所说的 kmalloc 内存分配器。再有内核也会为一些结构或
数据分配一定的小粒度内存用于存储名字或字符串信息，因此 Name 分配器也是
内核所必须的一个内存分配器。

以上提到的多个内存分配器都是基于 Slub 内存分配器构建的，其目的都是满足
内核对小粒度内存的不同需求。

![](/assets/PDB/HK/HK000268.png)

Slub 内存分配器的工作原理就是从 Buddy 内存分配器中获得一些物理页，将这些
物理页称为 slab_page，然后将这些 slab_page 对应的内存空间划分为指定大小
的内存块，每个内存块按顺序，将每个内存块作为一个指针，并指向下一个内存
块，这样就行形成了 freelist 链表。接着每个 CPU 都有私有的 freelist 链表，
这个链表通过 "struct kmem_cache_cpu" 进行维护，这样的设计确保任务从一个
CPU 迁移到另外一个 CPU 之后，任务还能继续从之前的 freelist 中获得内存。
每个缓存通过 "struct kmem_cache" 进行维护，然后使用 cpu_slab 成员指向
特定 CPU 上的 freelist 获得所需要的缓存。当释放小粒度的内存到 slub 内存
分配器的时候，Slub 内存分配器还是找到对应的 freelist 和 对应的 slab_page,
将释放的内存插入到 freelist 链表上。

----------------------------------

<span id="H1"></span>

![](/assets/PDB/BiscuitOS/kernel/IND00000Y.jpg)

#### 用户空间 Slub 内存分配器实践

> - [实践部署](#H10)
>
> - [实践执行](#H11)
>
> - [实践分析](#H12)

![](/assets/PDB/BiscuitOS/kernel/IND000101.jpg)

---------------------------------------

###### <span id="H10">实践部署</span>

在进行用户空间 Slub 内存分配实践之前，开发者应该准备一台 X86 Linux
主机，主机安装基础的 gcc 和 make 开发工具，参考并执行下面命令:

{% highlight bash %}
mkdir -p BiscuitOS_MEM
cd BiscuitOS_MEM
wget https://gitee.com/BiscuitOS_team/HardStack/raw/Gitee/Memory-Allocator/BiscuitOS_MM.sh
chmod 755 BiscuitOS_MM.sh
{% endhighlight %}

执行完上面的命令之后，会获得名为 "BiscuitOS_MM.sh" 的脚本, 该脚本用来
自动部署实践所需的源码。准备工作一切准备就绪，接下来看下一节内容.

---------------------------------------

###### <span id="H11">实践执行</span>

接下来，执行下面的命令

{% highlight bash %}
./BiscuitOS_MM.sh
{% endhighlight %}

![](/assets/PDB/HK/HK000228.png)

执行完上面的命令会出现上图的对话框，该对话框用于部署不同内存分配器的
源码项目。对于用户空间的 Slub 内存分配器，这里输入 7 并回车，如果
是第一次执行上面的命令，那么脚本会从网上下载并部署所需的源码。待下载
部署完毕之后，可以在当前目录获得名为 "Slub" 的目录，此时进入该目录，
查看源码如下图:

![](/assets/PDB/HK/HK000269.png)

用户空间 Slub 内存分配器包含了 14 个文件，"mian.c" 文件包含了用户空间
Slub 内存分配器的使用例子，开发者可以参考这些例子对 Slub 内存
分配器进行更多的探索. "mm/slub.c" 和 "include/linux/slub.h" 文件
包含了用户空间的核心实现。接下来使用下面命令运行用户空间的 Slub 内存
分配器:

{% highlight bash %}
cd Slub
make clean
make
./biscuitos
{% endhighlight %}

![](/assets/PDB/HK/HK000270.png)

以上就是用户空间 Slub 运行情况，开发者可以从 "main.c" 文件的 "main()"
函数作为入口来实践 Slub 的实现过程。

---------------------------------------

###### <span id="H12">实践分析</span>

用户空间的 Slub 内存分配器是基于用户空间的 Buddy 内存分配器，其进行
自身初始化之后，从用户空间的 Buddy 内存分配器中获得一定数量的物理页，
将这些物理页分成指定长度的内存块，再将内存块作为一个指针指向下一个
内存区块，以此形成下面的逻辑结构:

![](/assets/PDB/HK/HK000268.png)

freelist 形成之后，由于只模拟了一个 CPU，因此用户空间的 Slub 内存分配器
做了相应的 CPU 迁移处理，让 freelist 都是在同一个 CPU 上，不会迁移到其他
CPU 上，这样处理之后，每个缓存通过 "kmem_cache" 进行管理，并通过 cpu_slab
指向同一个 CPU 的 "kmem_cache_cpu" 上。然后通过 freelist 进行小粒度内存的
申请和释放任务。

----------------------------------

<span id="H2"></span>

![](/assets/PDB/BiscuitOS/kernel/IND00000V.jpg)

#### 用户空间 Slub 内存分配器 BiscuitOS 实践

> - [实践部署](#H20)
>
> - [实践执行](#H21)
>
> - [实践分析](#H22)

![](/assets/PDB/BiscuitOS/kernel/IND000101.jpg)

-----------------------------------------------

##### <span id="H20">实践部署</span>

BiscuitOS 已经支持用户空间的 Slub 内存分配器实践，如果
还未部署 BiscuitOS，请参考下列文章, 如果已经部署 BiscuitOS
项目的童鞋请使用 "git pull" 更新 BiscuitOS，并查看下一节:

> - [BiscuitOS linux 5.0 部署介绍](/blog/Linux-5.0-arm32-Usermanual/)

-----------------------------------------------

##### <span id="H21">实践执行</span>

在部署或更新 BiscuitOS 之后，参考下面命令获得用户空间 Slub 项目:

{% highlight bash %}
cd BiscuitOS/
make linux-5.0-arm32_defconfig
make menuconfig
{% endhighlight %}

![](/assets/PDB/HK/HK000231.png)

选择并进入 "Package  --->",

![](/assets/PDB/HK/HK000271.png)

选择并进入 "Slab/Slub/Slob Memory Allocator  --->",

![](/assets/PDB/HK/HK000272.png)

选择 "SLUB Allocator on Userspace  --->", 保存并退出.

选择完毕之后，接下来使用如下命令部署用户空间的 Slub 内存分配器:

{% highlight bash %}
make
cd BiscuitOS/output/linux-5.0-arm32/package/slub_userspace-0.0.1
{% endhighlight %}

执行完 make 命令之后，系统会提示所使用版本对于的安装目录，开发者可以
根据自己的版本参考执行。执行完毕之后，使用如下命令:

{% highlight bash %}
make download
make clean
make
make install
make pack
tree
{% endhighlight %}

![](/assets/PDB/HK/HK000273.png)

执行完上面的代码之后，BiscuitOS 自动部署用户空间 Slub 内存分配器
项目的源码，并将目标文件安装到 BiscuitOS，接下来在 BiscuitOS 上运行
用户空间 Slub 内存分配器，使用如下命令:

{% highlight bash %}
cd BiscuitOS/output/linux-5.0-arm32/
./RunBiscuitOS.sh
{% endhighlight %}

![](/assets/PDB/HK/HK000274.png)

上图就是用户空间 Slub 在 BiscuitOS 运行的效果。

---------------------------------------

###### <span id="H22">实践分析</span>

用户空间的 Slub 内存分配器是基于用户空间的 Buddy 内存分配器，其进行
自身初始化之后，从用户空间的 Buddy 内存分配器中获得一定数量的物理页，
将这些物理页分成指定长度的内存块，再将内存块作为一个指针指向下一个
内存区块，以此形成下面的逻辑结构:

![](/assets/PDB/HK/HK000268.png)

freelist 形成之后，由于只模拟了一个 CPU，因此用户空间的 Slub 内存分配器
做了相应的 CPU 迁移处理，让 freelist 都是在同一个 CPU 上，不会迁移到其他
CPU 上，这样处理之后，每个缓存通过 "kmem_cache" 进行管理，并通过 cpu_slab
指向同一个 CPU 的 "kmem_cache_cpu" 上。然后通过 freelist 进行小粒度内存的
申请和释放任务。

-----------------------------------------------

<span id="J"></span>

![](/assets/PDB/BiscuitOS/kernel/IND00000K.jpg)

#### Kmem_cache Memory Allocator

![](/assets/PDB/HK/HK000268.png)

> - [Kmem_cache 内存分配器简介](#J0)
>
> - [用户空间 Kmem_cache 内存分配器实践](#J1)
>
> - [用户空间 Kmem_cache 内存分配器 BiscuitOS 实践](#J2)

![](/assets/PDB/BiscuitOS/kernel/IND000101.jpg)

----------------------------------------------------------

#### <span id="J0">Kmem_cache 内存分配器简介</span>

Kmem_cache 分配器用于给系统经常使用变量或结构分配缓存。内核高频率需要给
特定的数据结构分配内存，由于分配和释放的频率特别高，那么系统会消耗很多
时间在 Slub 分配器里分配需求的内存，基于以上原因，内核使用了 Kmem_cache
内存分配器，它为特定数据结构分配缓存，缓存里面的内存都按数据结构的长度
进行分块，只要有新的申请，Kmem_cache 立即从自己的 freelist 上找到一块
可用的内存给调用者，但调用者返回这块内存时，Kmem_cache 又将内存块插入到
freelist 的链表上，以便调用者下次使用，其实现如下图:

![](/assets/PDB/HK/HK000268.png)

Kmem_cache 分配器基于 Slub 内存分配器，从原理上讲 Kmem_cache 是 slub
内存分配器的重要功能。Kmem_cache 经常为 Linux 经常给文件系统，任务调到
涉及的数据结构提供高速缓存。

----------------------------------

<span id="J1"></span>

![](/assets/PDB/BiscuitOS/kernel/IND00000L.jpg)

#### 用户空间 Kmem_cache 内存分配器实践

> - [实践部署](#J10)
>
> - [实践执行](#J11)
>
> - [实践分析](#J12)

![](/assets/PDB/BiscuitOS/kernel/IND000101.jpg)

---------------------------------------

###### <span id="J10">实践部署</span>

在进行用户空间 Kmem_cache 内存分配实践之前，开发者应该准备一台 X86 Linux
主机，主机安装基础的 gcc 和 make 开发工具，参考并执行下面命令:

{% highlight bash %}
mkdir -p BiscuitOS_MEM
cd BiscuitOS_MEM
wget https://gitee.com/BiscuitOS_team/HardStack/raw/Gitee/Memory-Allocator/BiscuitOS_MM.sh
chmod 755 BiscuitOS_MM.sh
{% endhighlight %}

执行完上面的命令之后，会获得名为 "BiscuitOS_MM.sh" 的脚本, 该脚本用来
自动部署实践所需的源码。准备工作一切准备就绪，接下来看下一节内容.

---------------------------------------

###### <span id="J11">实践执行</span>

接下来，执行下面的命令

{% highlight bash %}
./BiscuitOS_MM.sh
{% endhighlight %}

![](/assets/PDB/HK/HK000228.png)

执行完上面的命令会出现上图的对话框，该对话框用于部署不同内存分配器的
源码项目。对于用户空间的 Kmem_cache 内存分配器，这里输入 8 并回车，如果
是第一次执行上面的命令，那么脚本会从网上下载并部署所需的源码。待下载
部署完毕之后，可以在当前目录获得名为 "Kmem_cache" 的目录，此时进入该目录，
查看源码如下图:

![](/assets/PDB/HK/HK000275.png)

用户空间 Kmem_cache 内存分配器包含了 13 个文件，"mian.c" 文件包含了用户空间
Kmem_cache 内存分配器的使用例子，开发者可以参考这些例子对 Kmem_cache 内存
分配器进行更多的探索. "mm/slub.c" 和 "include/linux/slub.h" 文件
包含了用户空间的核心实现。接下来使用下面命令运行用户空间的 Kmem_cache 内存
分配器:

{% highlight bash %}
cd Kmem_cache
make clean
make
./biscuitos
{% endhighlight %}

![](/assets/PDB/HK/HK000276.png)

以上就是用户空间 Kmem_cache 运行情况，开发者可以从 "main.c" 文件的 "main()"
函数作为入口来实践 Kmem_cache 的实现过程。

---------------------------------------

###### <span id="J12">实践分析</span>

用户空间的 Kmem_cache 内存分配器基于用户空间的 Slub 内存分配器，其目的
主要是为系统提供小粒度内存的高速缓存。其实现如下图:

![](/assets/PDB/HK/HK000268.png)

用户空间的 Kmem_cache 内存分配器为调用者提供了 "kmem_cache_create()"、
"kmem_cache_zalloc()" 以及 "kmem_cache_free()" 等接口。这些接口满足了
调用者申请和释放小粒度的内存需求。

----------------------------------

<span id="J2"></span>

![](/assets/PDB/BiscuitOS/kernel/IND00000C.jpg)

#### 用户空间 Kmem_cache 内存分配器 BiscuitOS 实践

> - [实践部署](#J20)
>
> - [实践执行](#J21)
>
> - [实践分析](#J22)

![](/assets/PDB/BiscuitOS/kernel/IND000101.jpg)

-----------------------------------------------

##### <span id="J20">实践部署</span>

BiscuitOS 已经支持用户空间的 Kmem_cache 内存分配器实践，如果
还未部署 BiscuitOS，请参考下列文章, 如果已经部署 BiscuitOS
项目的童鞋请使用 "git pull" 更新 BiscuitOS，并查看下一节:

> - [BiscuitOS linux 5.0 部署介绍](/blog/Linux-5.0-arm32-Usermanual/)

-----------------------------------------------

##### <span id="J21">实践执行</span>

在部署或更新 BiscuitOS 之后，参考下面命令获得用户空间 Kmem_cache 项目:

{% highlight bash %}
cd BiscuitOS/
make linux-5.0-arm32_defconfig
make menuconfig
{% endhighlight %}

![](/assets/PDB/HK/HK000231.png)

选择并进入 "Package  --->",

![](/assets/PDB/HK/HK000271.png)

选择并进入 "Slab/Kmem_cache/Slob Memory Allocator  --->",

![](/assets/PDB/HK/HK000277.png)

选择 "Kmem_cache Allocator on Userspace  --->", 保存并退出.

选择完毕之后，接下来使用如下命令部署用户空间的 Kmem_cache 内存分配器:

{% highlight bash %}
make
cd BiscuitOS/output/linux-5.0-arm32/package/kmem_cache_userspace-0.0.1
{% endhighlight %}

执行完 make 命令之后，系统会提示所使用版本对于的安装目录，开发者可以
根据自己的版本参考执行。执行完毕之后，使用如下命令:

{% highlight bash %}
make download
make clean
make
make install
make pack
tree
{% endhighlight %}

![](/assets/PDB/HK/HK000278.png)

执行完上面的代码之后，BiscuitOS 自动部署用户空间 Kmem_cache 内存分配器
项目的源码，并将目标文件安装到 BiscuitOS，接下来在 BiscuitOS 上运行
用户空间 Kmem_cache 内存分配器，使用如下命令:

{% highlight bash %}
cd BiscuitOS/output/linux-5.0-arm32/
./RunBiscuitOS.sh
{% endhighlight %}

![](/assets/PDB/HK/HK000279.png)

上图就是用户空间 Kmem_cache 在 BiscuitOS 运行的效果。

---------------------------------------

###### <span id="J22">实践分析</span>

用户空间的 Kmem_cache 内存分配器基于用户空间的 Slub 内存分配器，其目的
主要是为系统提供小粒度内存的高速缓存。其实现如下图:

![](/assets/PDB/HK/HK000268.png)

用户空间的 Kmem_cache 内存分配器为调用者提供了 "kmem_cache_create()"、
"kmem_cache_zalloc()" 以及 "kmem_cache_free()" 等接口。这些接口满足了
调用者申请和释放小粒度的内存需求。

-----------------------------------------------

<span id="K"></span>

![](/assets/PDB/BiscuitOS/kernel/IND00000K.jpg)

#### Kmalloc Memory Allocator

![](/assets/PDB/HK/HK000280.png)

> - [Kmalloc 内存分配器简介](#K0)
>
> - [用户空间 Kmalloc 内存分配器实践](#K1)
>
> - [用户空间 Kmalloc 内存分配器 BiscuitOS 实践](#K2)

![](/assets/PDB/BiscuitOS/kernel/IND000101.jpg)

----------------------------------------------------------

#### <span id="K0">Kmalloc 内存分配器简介</span>

Kmalloc 内存分配器是用于分配第频率的的小粒度内存分配器。在 Linux 内核中，
经常会分配一些介于 8、 16、 32、 256 等长度之间的小粒度内存，这类型的内存
申请不是内核高频率申请的长度，因此 Kmem_cache 内存分配器无法满足这类型需求，
因此内核设计了 Kmalloc 内存分配器，这里内存分配器其工作原理与 Kmem_cache
内存分配类似，但 kmalloc 会创建长度大于 8 字节而小于 64M 的缓存，这些缓存
的长度都是 2 的阶乘。每当调用者请求分配不是 2 阶乘的内存，那么 Kmalloc
内存分配器就会分配比请求内存大的 2^n 缓存上为请求者分配内存，因此这里也会
存在一定量的内存浪费，但总比 Kmem_cache 内存分配器为其独自建立一个缓存浪费
的内存少很多。因此 Kmalloc() 内存分配器称为了 Linux 不可或缺的一个内存
分配器，其工作原理如下图:

![](/assets/PDB/HK/HK000280.png)

Kmalloc 内存分配器为内核提供了 "kmalloc()/kfree" 以及 "kzalloc()" 等函数，
通过这些函数，调用者可以快速的获得所需的小粒度内存。其很好的平衡了小粒度
内存的 "时间和空间" 问题。

----------------------------------

<span id="K1"></span>

![](/assets/PDB/BiscuitOS/kernel/IND00000R.jpg)

#### 用户空间 Kmalloc 内存分配器实践

> - [实践部署](#K10)
>
> - [实践执行](#K11)
>
> - [实践分析](#K12)

![](/assets/PDB/BiscuitOS/kernel/IND000101.jpg)

---------------------------------------

###### <span id="K10">实践部署</span>

在进行用户空间 Kmalloc 内存分配实践之前，开发者应该准备一台 X86 Linux
主机，主机安装基础的 gcc 和 make 开发工具，参考并执行下面命令:

{% highlight bash %}
mkdir -p BiscuitOS_MEM
cd BiscuitOS_MEM
wget https://gitee.com/BiscuitOS_team/HardStack/raw/Gitee/Memory-Allocator/BiscuitOS_MM.sh
chmod 755 BiscuitOS_MM.sh
{% endhighlight %}

执行完上面的命令之后，会获得名为 "BiscuitOS_MM.sh" 的脚本, 该脚本用来
自动部署实践所需的源码。准备工作一切准备就绪，接下来看下一节内容.

---------------------------------------

###### <span id="K11">实践执行</span>

接下来，执行下面的命令

{% highlight bash %}
./BiscuitOS_MM.sh
{% endhighlight %}

![](/assets/PDB/HK/HK000228.png)

执行完上面的命令会出现上图的对话框，该对话框用于部署不同内存分配器的
源码项目。对于用户空间的 Kmalloc 内存分配器，这里输入 9 并回车，如果
是第一次执行上面的命令，那么脚本会从网上下载并部署所需的源码。待下载
部署完毕之后，可以在当前目录获得名为 "Kmalloc" 的目录，此时进入该目录，
查看源码如下图:

![](/assets/PDB/HK/HK000269.png)

用户空间 Kmalloc 内存分配器包含了 14 个文件，"mian.c" 文件包含了用户空间
Kmalloc 内存分配器的使用例子，开发者可以参考这些例子对 Kmalloc 内存
分配器进行更多的探索. "mm/slub.c" 和 "include/linux/slub.h" 文件
包含了用户空间的核心实现。接下来使用下面命令运行用户空间的 Kmalloc 内存
分配器:

{% highlight bash %}
cd Kmalloc
make clean
make
./biscuitos
{% endhighlight %}

![](/assets/PDB/HK/HK000281.png)

以上就是用户空间 Kmalloc 运行情况，开发者可以从 "main.c" 文件的 "main()"
函数作为入口来实践 Kmalloc 的实现过程。

---------------------------------------

###### <span id="K12">实践分析</span>

用户空间的 Kmalloc() 内存分配器基于用户空间的 Slub 内存分配器，用户空间的
kmalloc 内存分配器按照 kmalloc 内存分配器的实现逻辑，创建 kmalloc_cache[]
数组，该数组就是用于维护 2^n 长度的缓存，这些缓存只有在调用者分配的时候
才会实际去用户空间的 Slub 内存分配器上分配 slab page 作为缓存。其实现如下图:

![](/assets/PDB/HK/HK000280.png)

用户空间的 Kmalloc 内存分配器对于调用者分配不是 2^n 阶乘的长度内存，Kmalloc
内存分配器会多分配一部分内存，但使用者不应该去使用多的这部分内存。

----------------------------------

<span id="K2"></span>

![](/assets/PDB/BiscuitOS/kernel/IND00000Z.jpg)

#### 用户空间 Kmalloc 内存分配器 BiscuitOS 实践

> - [实践部署](#K20)
>
> - [实践执行](#K21)
>
> - [实践分析](#K22)

![](/assets/PDB/BiscuitOS/kernel/IND000101.jpg)

-----------------------------------------------

##### <span id="K20">实践部署</span>

BiscuitOS 已经支持用户空间的 Kmalloc 内存分配器实践，如果
还未部署 BiscuitOS，请参考下列文章, 如果已经部署 BiscuitOS
项目的童鞋请使用 "git pull" 更新 BiscuitOS，并查看下一节:

> - [BiscuitOS linux 5.0 部署介绍](/blog/Linux-5.0-arm32-Usermanual/)

-----------------------------------------------

##### <span id="K21">实践执行</span>

在部署或更新 BiscuitOS 之后，参考下面命令获得用户空间 Kmalloc 项目:

{% highlight bash %}
cd BiscuitOS/
make linux-5.0-arm32_defconfig
make menuconfig
{% endhighlight %}

![](/assets/PDB/HK/HK000231.png)

选择并进入 "Package  --->",

![](/assets/PDB/HK/HK000271.png)

选择并进入 "Slab/Kmalloc/Slob Memory Allocator  --->",

![](/assets/PDB/HK/HK000282.png)

选择 "Kmalloc Allocator on Userspace  --->", 保存并退出.

选择完毕之后，接下来使用如下命令部署用户空间的 Kmalloc 内存分配器:

{% highlight bash %}
make
cd BiscuitOS/output/linux-5.0-arm32/package/kmalloc_userspace-0.0.1
{% endhighlight %}

执行完 make 命令之后，系统会提示所使用版本对于的安装目录，开发者可以
根据自己的版本参考执行。执行完毕之后，使用如下命令:

{% highlight bash %}
make download
make clean
make
make install
make pack
tree
{% endhighlight %}

![](/assets/PDB/HK/HK000283.png)

执行完上面的代码之后，BiscuitOS 自动部署用户空间 Kmalloc 内存分配器
项目的源码，并将目标文件安装到 BiscuitOS，接下来在 BiscuitOS 上运行
用户空间 Kmalloc 内存分配器，使用如下命令:

{% highlight bash %}
cd BiscuitOS/output/linux-5.0-arm32/
./RunBiscuitOS.sh
{% endhighlight %}

![](/assets/PDB/HK/HK000284.png)

上图就是用户空间 Kmalloc 在 BiscuitOS 运行的效果。

---------------------------------------

###### <span id="K22">实践分析</span>

用户空间的 Kmalloc() 内存分配器基于用户空间的 Slub 内存分配器，用户空间的
kmalloc 内存分配器按照 kmalloc 内存分配器的实现逻辑，创建 kmalloc_cache[]
数组，该数组就是用于维护 2^n 长度的缓存，这些缓存只有在调用者分配的时候
才会实际去用户空间的 Slub 内存分配器上分配 slab page 作为缓存。其实现如下图:

![](/assets/PDB/HK/HK000280.png)

用户空间的 Kmalloc 内存分配器对于调用者分配不是 2^n 阶乘的长度内存，Kmalloc
内存分配器会多分配一部分内存，但使用者不应该去使用多的这部分内存。

-----------------------------------------------

<span id="L"></span>

![](/assets/PDB/BiscuitOS/kernel/IND00000W.jpg)

#### NAME Memory Allocator

> - [NAME 内存分配器简介](#L0)
>
> - [用户空间 NAME 内存分配器实践](#L1)
>
> - [用户空间 NAME 内存分配器 BiscuitOS 实践](#L2)

![](/assets/PDB/BiscuitOS/kernel/IND000101.jpg)

----------------------------------------------------------

#### <span id="L0">NAME 内存分配器简介</span>

NAME 内存分配器用于给名字或字符串分配内存。在 Linux 中，需要给结构数据
的名字分配一个名字，名字具有一些格式或纯粹的字符串，内核就专门采用 NAME
分配器完整这个任务。

NAME 分配器基于 Slub 和 Kmem_cache 内存分配器，其实现逻辑由与 Kmalloc 分配
器类似，但 NAME 内存分配器不仅能提供小粒度的内存分配，还可以处理格式化名字。
例如在 Linux 中，很多设备的名字是根据总线和设备 ID 组成，这类型的名字就是
典型的格式化名字，此时 NAME 不仅可以解析格式化参数动态计算出所需内存的长度，
并分配相应的内存，而且还灵活的提供了内核格式化设置的功能。

NAME 分配器通过提供 "kstrdup()" 内存分配并拷贝字符串的功能，还提供了
"kasprintf()" 这类型的函数用于为格式化名字分配和设置名字。

----------------------------------

<span id="L1"></span>

![](/assets/PDB/BiscuitOS/kernel/IND00000R.jpg)

#### 用户空间 NAME 内存分配器实践

> - [实践部署](#L10)
>
> - [实践执行](#L11)
>
> - [实践分析](#L12)

![](/assets/PDB/BiscuitOS/kernel/IND000101.jpg)

---------------------------------------

###### <span id="L10">实践部署</span>

在进行用户空间 NAME 内存分配实践之前，开发者应该准备一台 X86 Linux
主机，主机安装基础的 gcc 和 make 开发工具，参考并执行下面命令:

{% highlight bash %}
mkdir -p BiscuitOS_MEM
cd BiscuitOS_MEM
wget https://gitee.com/BiscuitOS_team/HardStack/raw/Gitee/Memory-Allocator/BiscuitOS_MM.sh
chmod 755 BiscuitOS_MM.sh
{% endhighlight %}

执行完上面的命令之后，会获得名为 "BiscuitOS_MM.sh" 的脚本, 该脚本用来
自动部署实践所需的源码。准备工作一切准备就绪，接下来看下一节内容.

---------------------------------------

###### <span id="L11">实践执行</span>

接下来，执行下面的命令

{% highlight bash %}
./BiscuitOS_MM.sh
{% endhighlight %}

![](/assets/PDB/HK/HK000228.png)

执行完上面的命令会出现上图的对话框，该对话框用于部署不同内存分配器的
源码项目。对于用户空间的 NAME 内存分配器，这里输入 a 并回车，如果
是第一次执行上面的命令，那么脚本会从网上下载并部署所需的源码。待下载
部署完毕之后，可以在当前目录获得名为 "NAME_ALLOC" 的目录，此时进入该目录，
查看源码如下图:

![](/assets/PDB/HK/HK000269.png)

用户空间 NAME 内存分配器包含了 14 个文件，"mian.c" 文件包含了用户空间
NAME 内存分配器的使用例子，开发者可以参考这些例子对 NAME 内存
分配器进行更多的探索. "mm/kasprintf.c" 和 "include/linux/slub.h" 文件
包含了用户空间的核心实现。接下来使用下面命令运行用户空间的 NAME 内存
分配器:

{% highlight bash %}
cd NAME_ALLOC
make clean
make
./biscuitos
{% endhighlight %}

![](/assets/PDB/HK/HK000285.png)

以上就是用户空间 NAME 运行情况，开发者可以从 "main.c" 文件的 "main()"
函数作为入口来实践 NAME 的实现过程。

---------------------------------------

###### <span id="L12">实践分析</span>

用户空间的 NAME 分配器基于用户空间的 Slub 内存分配器，与 Kmem_cache 内存分配
和 kmalloc 内存分配器不同点就是多个格式化解析的过程，NAME 分配器在
"mm/kasprintf.c" 文件中涉及对格式化参数的解析。开发者也可以参考该文件
的实现逻辑，从而研究格式化参数解析过程。

----------------------------------

<span id="L2"></span>

![](/assets/PDB/BiscuitOS/kernel/IND00000P.jpg)

#### 用户空间 NAME 内存分配器 BiscuitOS 实践

> - [实践部署](#L20)
>
> - [实践执行](#L21)
>
> - [实践分析](#L22)

![](/assets/PDB/BiscuitOS/kernel/IND000101.jpg)

-----------------------------------------------

##### <span id="L20">实践部署</span>

BiscuitOS 已经支持用户空间的 NAME 内存分配器实践，如果
还未部署 BiscuitOS，请参考下列文章, 如果已经部署 BiscuitOS
项目的童鞋请使用 "git pull" 更新 BiscuitOS，并查看下一节:

> - [BiscuitOS linux 5.0 部署介绍](/blog/Linux-5.0-arm32-Usermanual/)

-----------------------------------------------

##### <span id="L21">实践执行</span>

在部署或更新 BiscuitOS 之后，参考下面命令获得用户空间 NAME 项目:

{% highlight bash %}
cd BiscuitOS/
make linux-5.0-arm32_defconfig
make menuconfig
{% endhighlight %}

![](/assets/PDB/HK/HK000231.png)

选择并进入 "Package  --->",

![](/assets/PDB/HK/HK000271.png)

选择并进入 "Slab/NAME/Slob Memory Allocator  --->",

![](/assets/PDB/HK/HK000286.png)

选择 "Name Allocator on Userspace  --->", 保存并退出.

选择完毕之后，接下来使用如下命令部署用户空间的 NAME 内存分配器:

{% highlight bash %}
make
cd BiscuitOS/output/linux-5.0-arm32/package/name_userspace-0.0.1
{% endhighlight %}

执行完 make 命令之后，系统会提示所使用版本对于的安装目录，开发者可以
根据自己的版本参考执行。执行完毕之后，使用如下命令:

{% highlight bash %}
make download
make clean
make
make install
make pack
tree
{% endhighlight %}

![](/assets/PDB/HK/HK000287.png)

执行完上面的代码之后，BiscuitOS 自动部署用户空间 NAME 内存分配器
项目的源码，并将目标文件安装到 BiscuitOS，接下来在 BiscuitOS 上运行
用户空间 NAME 内存分配器，使用如下命令:

{% highlight bash %}
cd BiscuitOS/output/linux-5.0-arm32/
./RunBiscuitOS.sh
{% endhighlight %}

![](/assets/PDB/HK/HK000288.png)

上图就是用户空间 NAME 在 BiscuitOS 运行的效果。

---------------------------------------

###### <span id="L22">实践分析</span>

用户空间的 NAME 分配器基于用户空间的 Slub 内存分配器，与 Kmem_cache 内存分配
和 kmalloc 内存分配器不同点就是多个格式化解析的过程，NAME 分配器在
"mm/kasprintf.c" 文件中涉及对格式化参数的解析。开发者也可以参考该文件
的实现逻辑，从而研究格式化参数解析过程。

-----------------------------------------------

<span id="M"></span>

![](/assets/PDB/BiscuitOS/kernel/IND00000G.jpg)

#### VMALLOC Memory Allocator

![](/assets/PDB/HK/HK000289.png)

> - [VMALLOC 内存分配器简介](#M0)
>
> - [用户空间 VMALLOC 内存分配器实践](#M1)
>
> - [用户空间 VMALLOC 内存分配器 BiscuitOS 实践](#M2)

![](/assets/PDB/BiscuitOS/kernel/IND000101.jpg)

----------------------------------------------------------

#### <span id="M0">VMALLOC 内存分配器简介</span>

VMALLOC 内存分配器用于分配虚拟地址连续但物理地址不连续的内存。在 Linux
中，一部分物理内存是通过页表直接映射到虚拟地址，这部分映射从系统初始化
开始就一致保持一致映射，而另外一部分物理内存在系统初始化之后不能与虚拟
地址进行映射。系统运行一段时间之后，已经映射的物理内存消耗之后形成了很多
碎片，这就导致这段区域物理内存和虚拟内存都存在了碎片。在这种情况下，
如果调用者需要大块连续的虚拟内存，此时在已经映射的区域中很难找到。
Linux 于是设计了 VMALLOC 内存分配器，该内存分配器提供的虚拟地址是连续的，
但物理地址不一定连续，这样至少可以满足对连续虚拟内存的需求。

VMALLOC 内存分配通过页表和一颗红黑树实现，红黑树用户实现虚拟地址的分配，
页表则实现物理内存和虚拟内存之间的映射关系。总体来说 VMALLOC 内存分配
的实现比较简单，其基于 Buddy 内存管理分配器获得物理页，在通过从自身的
红黑树中找到一块未使用的连续物理内存，接着将这块物理内存和虚拟内存之间
建立相应的页表。

![](/assets/PDB/HK/HK000226.png)

VMALLOC 内存分配器分配的虚拟地址位于 VMALLOC_START 到 VMALLOC_END 之间，
低端物理内存固定映射到虚拟地址后，在映射的末端 VMALLOC_OFFSET 之后便是
VMALLOC 内存分配区域。VMALLOC 内存分配器为内核提供了 "vmalloc()/vfree()"
等函数用于分配 VMALLOC 区域内的虚拟内存。

----------------------------------

<span id="M1"></span>

![](/assets/PDB/BiscuitOS/kernel/IND00000I.jpg)

#### 用户空间 VMALLOC 内存分配器实践

> - [实践部署](#M10)
>
> - [实践执行](#M11)
>
> - [实践分析](#M12)

![](/assets/PDB/BiscuitOS/kernel/IND000101.jpg)

---------------------------------------

###### <span id="M10">实践部署</span>

在进行用户空间 VMALLOC 内存分配实践之前，开发者应该准备一台 X86 Linux
主机，主机安装基础的 gcc 和 make 开发工具，参考并执行下面命令:

{% highlight bash %}
mkdir -p BiscuitOS_MEM
cd BiscuitOS_MEM
wget https://gitee.com/BiscuitOS_team/HardStack/raw/Gitee/Memory-Allocator/BiscuitOS_MM.sh
chmod 755 BiscuitOS_MM.sh
{% endhighlight %}

执行完上面的命令之后，会获得名为 "BiscuitOS_MM.sh" 的脚本, 该脚本用来
自动部署实践所需的源码。准备工作一切准备就绪，接下来看下一节内容.

---------------------------------------

###### <span id="M11">实践执行</span>

接下来，执行下面的命令

{% highlight bash %}
./BiscuitOS_MM.sh
{% endhighlight %}

![](/assets/PDB/HK/HK000228.png)

执行完上面的命令会出现上图的对话框，该对话框用于部署不同内存分配器的
源码项目。对于用户空间的 VMALLOC 内存分配器，这里输入 b 并回车，如果
是第一次执行上面的命令，那么脚本会从网上下载并部署所需的源码。待下载
部署完毕之后，可以在当前目录获得名为 "VMALLOC" 的目录，此时进入该目录，
查看源码如下图:

![](/assets/PDB/HK/HK000290.png)

用户空间 VMALLOC 内存分配器包含了 17 个文件，"mian.c" 文件包含了用户空间
VMALLOC 内存分配器的使用例子，开发者可以参考这些例子对 VMALLOC 内存
分配器进行更多的探索. "mm/vmalloc.c" 和 "include/linux/vmalloc.h" 文件
包含了用户空间的核心实现。接下来使用下面命令运行用户空间的 VMALLOC 内存
分配器:

{% highlight bash %}
cd VMALLOC
make clean
make
./biscuitos
{% endhighlight %}

![](/assets/PDB/HK/HK000291.png)

以上就是用户空间 VMALLOC 运行情况，开发者可以从 "main.c" 文件的 "main()"
函数作为入口来实践 VMALLOC 的实现过程。

---------------------------------------

###### <span id="M12">实践分析</span>

用户空间的 VMALLOC 内存分配器基于用户空间的 Buddy 内存分配器和 Slub 
内存分配器，多个分配器代码复杂，但与 VMALLOC 内存分配器相关的代码
相对精简高效。在实现过程中使用了一颗红黑树，开发者可以通过研究
VMALLOC 内存分配器的行为间接深入研究红黑树的工作原理，更多红黑树
实践原理文档请参考如下链接:

> - [红黑树的设计原理与实践研究](/blog/Tree_RBTree/)

由于用户空间 VMALLOC 直接分配的虚拟地址是 "虚拟的"，因此我在设计
用户空间 VMALLOC 内存分配器的时候也设计了一个软件的 MMU，用户帮助
VMALLOC 虚拟地址转换为可用的虚拟地址。

----------------------------------

<span id="M2"></span>

![](/assets/PDB/BiscuitOS/kernel/IND00000X.jpg)

#### 用户空间 VMALLOC 内存分配器 BiscuitOS 实践

> - [实践部署](#M20)
>
> - [实践执行](#M21)
>
> - [实践分析](#M22)

![](/assets/PDB/BiscuitOS/kernel/IND000101.jpg)

-----------------------------------------------

##### <span id="M20">实践部署</span>

BiscuitOS 已经支持用户空间的 VMALLOC 内存分配器实践，如果
还未部署 BiscuitOS，请参考下列文章, 如果已经部署 BiscuitOS
项目的童鞋请使用 "git pull" 更新 BiscuitOS，并查看下一节:

> - [BiscuitOS linux 5.0 部署介绍](/blog/Linux-5.0-arm32-Usermanual/)

-----------------------------------------------

##### <span id="M21">实践执行</span>

在部署或更新 BiscuitOS 之后，参考下面命令获得用户空间 VMALLOC 项目:

{% highlight bash %}
cd BiscuitOS/
make linux-5.0-arm32_defconfig
make menuconfig
{% endhighlight %}

![](/assets/PDB/HK/HK000231.png)

选择并进入 "Package  --->",

![](/assets/PDB/HK/HK000292.png)

选择并进入 "Vmalloc Memory Allocator  --->",

![](/assets/PDB/HK/HK000293.png)

选择 "VMALLOC Allocator on Userspace  --->", 保存并退出.

选择完毕之后，接下来使用如下命令部署用户空间的 VMALLOC 内存分配器:

{% highlight bash %}
make
cd BiscuitOS/output/linux-5.0-arm32/package/vmalloc_userspace-0.0.1
{% endhighlight %}

执行完 make 命令之后，系统会提示所使用版本对于的安装目录，开发者可以
根据自己的版本参考执行。执行完毕之后，使用如下命令:

{% highlight bash %}
make download
make clean
make
make install
make pack
tree
{% endhighlight %}

![](/assets/PDB/HK/HK000294.png)

执行完上面的代码之后，BiscuitOS 自动部署用户空间 VMALLOC 内存分配器
项目的源码，并将目标文件安装到 BiscuitOS，接下来在 BiscuitOS 上运行
用户空间 VMALLOC 内存分配器，使用如下命令:

{% highlight bash %}
cd BiscuitOS/output/linux-5.0-arm32/
./RunBiscuitOS.sh
{% endhighlight %}

![](/assets/PDB/HK/HK000295.png)

上图就是用户空间 VMALLOC 在 BiscuitOS 运行的效果。

---------------------------------------

###### <span id="M22">实践分析</span>

用户空间的 VMALLOC 内存分配器基于用户空间的 Buddy 内存分配器和 Slub
内存分配器，多个分配器代码复杂，但与 VMALLOC 内存分配器相关的代码
相对精简高效。在实现过程中使用了一颗红黑树，开发者可以通过研究
VMALLOC 内存分配器的行为间接深入研究红黑树的工作原理，更多红黑树
实践原理文档请参考如下链接:

> - [红黑树的设计原理与实践研究](/blog/Tree_RBTree/)

由于用户空间 VMALLOC 直接分配的虚拟地址是 "虚拟的"，因此我在设计
用户空间 VMALLOC 内存分配器的时候也设计了一个软件的 MMU，用户帮助
VMALLOC 虚拟地址转换为可用的虚拟地址。

-----------------------------------------------

<span id="N"></span>

![](/assets/PDB/BiscuitOS/kernel/IND00000H.jpg)

#### KMAP Memory Allocator

> - [KMAP 内存分配器简介](#N0)
>
> - [用户空间 KMAP 内存分配器实践](#N1)
>
> - [用户空间 KMAP 内存分配器 BiscuitOS 实践](#N2)

![](/assets/PDB/BiscuitOS/kernel/IND000101.jpg)

----------------------------------------------------------

#### <span id="N0">KMAP 内存分配器简介</span>

KMAP 内存分配器用于将 KAMP 区域的虚拟地址与高端物理页进行临时映射，以供调用者
使用. 在 Linux 中，特别是 Module 中，动态插入到内核后，需要分配一些内存，
但是由于 VMALLOC 和低端虚拟内存都已经分配的差不多，而 Module 或调用者
只是临时分配内存，待任务完成之后，马上将虚拟地址归还给 KMAP 分配器。
KMAP 内存分配的实现很精简，通过将 KMAP 区域按 PAGE_SIZE 分作多个区块，
每个区块按顺序编号。但调用者从 KMAP 进行分配的时候，KMAP 内存分配器从
KMAP 区域找到空闲的页块，然后再将物理页与该区域进行映射。当使用完毕之后，
KMAP 内存分配器将解除该区域的映射。在整个过程中，KMAP 只负责页表的建立和
摧毁，而物理页则需调用者提供，物理页可以来自 Zone-Normal，也可以来自
Zone-HighMEM。如果调用者的物理页来自 Zone-Normal, 那么 KMAP 不做任何的
页表操作; 如果物理页来自 Zone-HighMEM, 那么 KMAP 才会对这里物理页进行页表
的创建和摧毁操作。KMAP 虚拟内存区域如下:

![](/assets/PDB/HK/HK000226.png)

不同架构对 KMAP 区域的布局有所不同，但不影响其工作逻辑的一致性。KMAP 内存
分配器为内核提供了 "kamp()/kunmap()" 函数对，也提供了 
"kmap_atomic()/kunmap_atomic()" 函数对，"kamp()/kunmap()" 会从 KMAP 区域
分配虚拟地址，而 "kmap_atomic()/kunmap_atomic" 会从 FIXMAP 区域分配内存。

----------------------------------

<span id="N1"></span>

![](/assets/PDB/BiscuitOS/kernel/IND00000J.jpg)

#### 用户空间 KMAP 内存分配器实践

> - [实践部署](#N10)
>
> - [实践执行](#N11)
>
> - [实践分析](#N12)

![](/assets/PDB/BiscuitOS/kernel/IND000101.jpg)

---------------------------------------

###### <span id="N10">实践部署</span>

在进行用户空间 KMAP 内存分配实践之前，开发者应该准备一台 X86 Linux
主机，主机安装基础的 gcc 和 make 开发工具，参考并执行下面命令:

{% highlight bash %}
mkdir -p BiscuitOS_MEM
cd BiscuitOS_MEM
wget https://gitee.com/BiscuitOS_team/HardStack/raw/Gitee/Memory-Allocator/BiscuitOS_MM.sh
chmod 755 BiscuitOS_MM.sh
{% endhighlight %}

执行完上面的命令之后，会获得名为 "BiscuitOS_MM.sh" 的脚本, 该脚本用来
自动部署实践所需的源码。准备工作一切准备就绪，接下来看下一节内容.

---------------------------------------

###### <span id="N11">实践执行</span>

接下来，执行下面的命令

{% highlight bash %}
./BiscuitOS_MM.sh
{% endhighlight %}

![](/assets/PDB/HK/HK000228.png)

执行完上面的命令会出现上图的对话框，该对话框用于部署不同内存分配器的
源码项目。对于用户空间的 KMAP 内存分配器，这里输入 c 并回车，如果
是第一次执行上面的命令，那么脚本会从网上下载并部署所需的源码。待下载
部署完毕之后，可以在当前目录获得名为 "KMAP" 的目录，此时进入该目录，
查看源码如下图:

![](/assets/PDB/HK/HK000296.png)

用户空间 KMAP 内存分配器包含了 17 个文件，"mian.c" 文件包含了用户空间
KMAP 内存分配器的使用例子，开发者可以参考这些例子对 KMAP 内存
分配器进行更多的探索. "mm/highmem.c" 和 "include/linux/highmem.h" 文件
包含了用户空间的核心实现。接下来使用下面命令运行用户空间的 KMAP 内存
分配器:

{% highlight bash %}
cd KMAP
make clean
make
./biscuitos
{% endhighlight %}

![](/assets/PDB/HK/HK000297.png)

以上就是用户空间 KMAP 运行情况，开发者可以从 "main.c" 文件的 "main()"
函数作为入口来实践 KMAP 的实现过程。

---------------------------------------

###### <span id="N12">实践分析</span>

用户空间的 KMAP 内存分配器基于用户空间的 Buddy-Highmem 内存分配器构建。
其实现了页表的创建和一个软件的 MMU 用于将分配的虚拟内存转换成实际可用的
虚拟内存。用户空间的 KMAP 内存分配器实现相对简单，在 Buddy-Highmem 的
基础上完成了核心逻辑。

----------------------------------

<span id="N2"></span>

![](/assets/PDB/BiscuitOS/kernel/IND00000F.jpg)

#### 用户空间 KMAP 内存分配器 BiscuitOS 实践

> - [实践部署](#N20)
>
> - [实践执行](#N21)
>
> - [实践分析](#N22)

![](/assets/PDB/BiscuitOS/kernel/IND000101.jpg)

-----------------------------------------------

##### <span id="N20">实践部署</span>

BiscuitOS 已经支持用户空间的 KMAP 内存分配器实践，如果
还未部署 BiscuitOS，请参考下列文章, 如果已经部署 BiscuitOS
项目的童鞋请使用 "git pull" 更新 BiscuitOS，并查看下一节:

> - [BiscuitOS linux 5.0 部署介绍](/blog/Linux-5.0-arm32-Usermanual/)

-----------------------------------------------

##### <span id="N21">实践执行</span>

在部署或更新 BiscuitOS 之后，参考下面命令获得用户空间 KMAP 项目:

{% highlight bash %}
cd BiscuitOS/
make linux-5.0-arm32_defconfig
make menuconfig
{% endhighlight %}

![](/assets/PDB/HK/HK000231.png)

选择并进入 "Package  --->",

![](/assets/PDB/HK/HK000298.png)

选择并进入 "Kmap Memory Allocator  --->",

![](/assets/PDB/HK/HK000299.png)

选择 "Kmap Memory Allocator on Userspace  --->", 保存并退出.

选择完毕之后，接下来使用如下命令部署用户空间的 KMAP 内存分配器:

{% highlight bash %}
make
cd BiscuitOS/output/linux-5.0-arm32/package/kmap_userspace-0.0.1
{% endhighlight %}

执行完 make 命令之后，系统会提示所使用版本对于的安装目录，开发者可以
根据自己的版本参考执行。执行完毕之后，使用如下命令:

{% highlight bash %}
make download
make clean
make
make install
make pack
tree
{% endhighlight %}

![](/assets/PDB/HK/HK000300.png)

执行完上面的代码之后，BiscuitOS 自动部署用户空间 KMAP 内存分配器
项目的源码，并将目标文件安装到 BiscuitOS，接下来在 BiscuitOS 上运行
用户空间 KMAP 内存分配器，使用如下命令:

{% highlight bash %}
cd BiscuitOS/output/linux-5.0-arm32/
./RunBiscuitOS.sh
{% endhighlight %}

![](/assets/PDB/HK/HK000301.png)

上图就是用户空间 KMAP 在 BiscuitOS 运行的效果。

---------------------------------------

###### <span id="N22">实践分析</span>

用户空间的 KMAP 内存分配器基于用户空间的 Buddy-Highmem 内存分配器构建。
其实现了页表的创建和一个软件的 MMU 用于将分配的虚拟内存转换成实际可用的
虚拟内存。用户空间的 KMAP 内存分配器实现相对简单，在 Buddy-Highmem 的
基础上完成了核心逻辑。

-----------------------------------------------

# <span id="Z0">附录</span>

> [BiscuitOS Home](https://biscuitos.github.io/)
>
> [BiscuitOS 实践文档目录](/blog/BiscuitOS_Catalogue/)
>
> [Linux Kernel](https://www.kernel.org/)
>
> [Bootlin: Elixir Cross Referencer](https://elixir.bootlin.com/linux/latest/source)

## 赞赏一下吧 🙂

![](/assets/PDB/BiscuitOS/kernel/HAB000036.jpg)

---
layout: post
title:  "Bootmem"
date:   2020-05-07 09:35:30 +0800
categories: [HW]
excerpt: MMU.
tags:
  - MMU
---

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND00000L0.PNG)

> Email: BuddyZhang1 <buddy.zhang@aliyun.com>

#### 目录

> - [Bootmem 分配器原理](#A)
>
> - [Bootmem 分配器使用](#B)
>
> - [Bootmem 分配器实践](#C)
>
> - [Bootmem 源码分析](#D)
>
> - [Bootmem 分配器调试](#C0004)
>
> - [Bootmem 分配进阶研究](#F)
>
> - [Bootmem 时间轴](#G)
>
> - [Bootmem 历史补丁](#H)
>
> - [Bootmem API](#K)
>
> - [附录/捐赠](#Z0)

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND000100.png)

----------------------------------

<span id="A"></span>

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND00000T.jpg)

#### Bootmem 分配器原理

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI000775.png)

Bootmem 分配器是 Linux 对早期虚拟内存和物理内存的管理，是 Buddy 内存分配器
的基础。Bootmem 内存分配器在 Linux 早期版本中采用，但到了新版本内核中，Linux
使用 MEMBLOCK 替代了 Bootmem 内存分配器。Bootmem 内存分配器采用 bitmap 的
模式对可用内存进行管理，每个 bit 代表 PAGE_SIZE 大小的内存块。

Architecture相关的机制从 bootloader 中获得可用物理内存的信息之后，Bootmem
建立一张巨大的 bitmap 表，表中的每个 bit 代表一个 PAGE_SIZE 大小的内存块。
Bootmem 建立 bitmap 之后，将内核中预留给 kernel 镜像、INITRD 等内存块在
bitmap 中进行置位标志，对未使用的内存块对应的 bit 进行清零操作。Bootmem
内存分配器本身初始化完毕只有，此时系统的 MMU 已经打开，并且当前的物理内存
和虚拟内存都是一一映射的，因此 Bootmem 此时管理的是虚拟内存而非物理内存。

Bootmem 提供了多个 API 供给内核其他子系统分配所需的内存，当 Bootmem 完成
自己的任务之后，会将 Bootmem 中所有空闲的内存块传递给 Buddy 管理器。而预留
的内存继续作为系统预留内存一直存在。Bootmem 相关的函数和变量均使用 "\_\_init"
和 "\_\_initdata" 进行标记，因此 Bootmem 相关的函数都存储在 ".init.text", 
相关的变量则存储在 ".init.data" section 内，当系统初始化到一定阶段之后，
系统将 ".init.data" 和 ".init.text" section 的内存清零归正常内存使用，因此
此时 Bootmem 占用的资源全部从当前系统中移除。 

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI000776.png)

Linux 在 bootmem 内存分配器初始化之前，各个体系架构从 bootloader 出获得
物理 RAM 的基本信息，例如在 ARM 架构中，uboot 通过 ATAG 参数将物理内存的
相关信息传递给 Linux 内核，内核使用 struct meminfo 进行管理维护，每个
meminfo 就是一块可用的物理块。ARM 架构将 struct meminfo 的信息传递给 Bootmem，
Bootmem 根据这些信息构建了适当长度的 bitmap 表，每个 bit 代表 PAGE_SIZE 
长度的内存块。第一块 meminfo 物理内存的起始地址存储在:

{% highlight bash %}
NODE_DATA(0)->bdata->node_boot_start
{% endhighlight %}

在 Linux 后续初始化的过程中，会将物理内存划分为 ZONE_DMA/ZONE_DMA32、
ZONE_NORMAL 和 ZONE_HIGHMEM, 如果 ZONE_HIGHMEM 确实存在，那么 ZONE_NORMAL
和 ZONE_HIGHMEM 的划分就是从:

{% highlight bash %}
NODE_DATA(0)->bdata->node_low_pfn
{% endhighlight %}

"node_low_pfn" 之后的物理页划分到了 ZONE_HIGHMEM 里面。如下图:

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI000735.png)

在将 meminfo 的物理内存信息转化为 Bootmem 的 bitmap 之后，Bootmem 就依据
bitmap 进行物理内存的分配和回收 (由于此时 MMU 已经打开，实际管理的是虚拟
内存，但由于此时虚拟内存和物理内存是一一对应的，因此就称为对物理内存的管理)。

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI000777.png)

Bootmem 建立 bitmap 只有，将所有的 bit 都置位，然后根据体系传递关于物理 RAM
的信息之后，将可用的物理内存对应的 bit 清零，然后将系统预留的物理内存全部
置位，这就形成上图所示的内存管理架构。该 bitmap 的起始地址存储在:

{% highlight bash %}
NODE_DATA(0)->bdata->node_bootmem_map
{% endhighlight %}

Bootmem 内存分配器相关的源码分布在:

{% highlight bash %}
#include <linux/bootmem.h>
mm/bootmem.c
{% endhighlight %}

---------------------------------

###### Bootmem 的优点

Bootmem 结构简单，使用 bitmap 对内存进行简单的管理，在 Linux 启动的早期
确实不需要很复杂的内存管理器。

###### Bootmem 的缺点

Bootmem 的缺点就是会对所有的物理内存在 bitmap 中建立对应的 bit，这样在超级
大物理内存的平台上将要花费很多时间进行创建，并且创建之后并不使用 bitmap
中的大部分。再者就是 bitmap 每个 bit 维护 PAGE_SIZE 大小的内存块，这样
的内存分配器粒度太大，对分配超级小的内存颗粒会造成很大的内存浪费。

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND000100.png)

---------------------------------

<span id="B"></span>

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND00000H.jpg)

#### Bootmem 分配器使用

Bootmem 内存分配器的主要完成三个任务，第一个任务就是分配内存，第二个任务
就是回收内存，第三个任务就是预留内存。因此 Bootmem 提供了大量的函数以便
完成三个任务:

###### 分配内存

{% highlight bash %}
alloc_bootmem()
alloc_bootmem_low()
alloc_bootmem_node()
alloc_bootmem_low_pages()
alloc_bootmem_pages()
alloc_bootmem_limit()
alloc_bootmem_low_limit()
alloc_bootmem_low_pages_limit()
alloc_bootmem_node_limit()
alloc_bootmem_pages_node_limit()
alloc_bootmem_low_pages_node_limit()
alloc_remap()
__alloc_bootmem()
__alloc_bootmem_node()
__alloc_bootmem_limit()
__alloc_bootmem_node_limit()
__alloc_bootmem_core()
alloc_large_system_hash()
{% endhighlight %}

###### 回收内存

{% highlight bash %}
free_bootmem_node()
free_all_bootmem_node()
free_bootmem()
free_bootmem_core()
free_all_bootmem_core()
{% endhighlight %}

###### 预留内存

{% highlight bash %}
reserve_bootmem()
reserve_bootmem_core()
reserve_bootmem_node()
{% endhighlight %}

具体函数解析说明，请查看:

> - [Bootmem API](#K)

------------------------------------------------

<span id="C"></span>

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND00000I.jpg)

#### Bootmem 分配器实践

> - [实践准备](#C0000)
>
> - [实践部署](#C0001)
>
> - [实践执行](#C0002)
>
> - [实践建议](https://biscuitos.github.io/blog/HISTORY-MMU/#C0003)
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

准备好基础开发环境之后，开发者接下来部署项目所需的开发环境。由于项目
支持多个版本的 Bootmem，开发者可以根据需求进行选择，本文以 linux 2.6.12 
版本的 Bootmem 进行讲解。开发者使用如下命令:

{% highlight bash %}
cd BiscuitOS/
make linux-5.0-arm32_defconfig
make menuconfig
{% endhighlight %}

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI000746.png)

选择并进入 "[\*] Package  --->" 目录。

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI000747.png)

选择并进入 "[\*]   Memory Development History  --->" 目录。

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI000778.png)

选择并进入 "[\*]   Bootmem Allocator  --->" 目录。

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI000779.png)

选择 "[\*]   bootmem on linux 2.6.12  --->" 目录，保存并退出。接着执行如下命令:

{% highlight bash %}
make
{% endhighlight %}

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI000750.png)

成功之后将出现上图的内容，接下来开发者执行如下命令以便切换到项目的路径:

{% highlight bash %}
cd BiscuitOS/output/linux-5.0-arm32/package/BiscuitOS_bootmem-2.6.12
make download
{% endhighlight %}

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI000780.png)

至此源码已经下载完成，开发者可以使用 tree 等工具查看源码:

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI000781.png)

arch 目录下包含内存初始化早期，与体系结构相关的处理部分。mm 目录下面包含
了与各个内存分配器和内存管理行为相关的代码。init 目录下是整个模块的初始化
入口流程。modules 目录下包含了内存分配器的使用例程和测试代码. fs 目录下
包含了内存管理信息输出到文件系统的相关实现。入口函数是 init/main.c 的
start_kernel()。

如果你是第一次使用这个项目，需要修改 DTS 的内容。如果不是可以跳到下一节。
开发者参考源码目录里面的 "BiscuitOS.dts" 文件，将文件中描述的内容添加
到系统的 DTS 里面，"BiscuitOS.dts" 里的内容用来从系统中预留 100MB 的物理
内存供项目使用，具体如下:

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI000738.png)

开发者将 "BiscuitOS.dts" 的内容添加到:

{% highlight bash %}
cd BiscuitOS/output/linux-5.0-arm32/linux/linux/arch/arm/boot/dts/vexpress-v2p-ca9.dts
{% endhighlight %}

添加完毕之后，使用如下命令更新 DTS:

{% highlight bash %}
cd BiscuitOS/output/linux-5.0-arm32/package/BiscuitOS_bootmem-2.6.12
make kernel
{% endhighlight %}

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI000782.png)

--------------------------------------------

#### <span id="C0002">实践执行</span>

环境部署完毕之后，开发者可以向通用模块一样对源码进行编译和安装使用，使用
如下命令:

{% highlight bash %}
cd BiscuitOS/output/linux-5.0-arm32/package/BiscuitOS_bootmem-2.6.12
make
{% endhighlight %}

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI000783.png)

以上就是模块成功编译，接下来将 ko 模块安装到 BiscuitOS 中，使用如下命令:

{% highlight bash %}
cd BiscuitOS/output/linux-5.0-arm32/package/BiscuitOS_bootmem-2.6.12
make install
make pack
{% endhighlight %}

以上准备完毕之后，最后就是在 BiscuitOS 运行这个模块了，使用如下命令:

{% highlight bash %}
cd BiscuitOS/output/linux-5.0-arm32/package/BiscuitOS_bootmem-2.6.12
make run
{% endhighlight %}

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI000784.png)

在 BiscuitOS 中插入了模块 "BiscuitOS_bootmem-2.6.12.ko"，打印如上信息，那么
BiscuitOS Memory Manager Unit History 项目的内存管理子系统已经可以使用，
接下来开发者可以在 BiscuitOS 中使用如下命令查看内存管理子系统的使用情况:

{% highlight bash %}
cat /proc/buddyinfo_bs
cat /proc/vmstat_bs
{% endhighlight %}

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI000756.png)

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
/xspace/OpenSource/BiscuitOS/BiscuitOS/output/linux-5.0-arm32/package/BiscuitOS_bootmem-2.6.12/BiscuitOS_bootmem-2.6.12/Makefile
{% endhighlight %}

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI000771.png)

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

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI000772.png)

然后先向 BiscuitOS 中插入 "BiscuitOS_bootmem-2.6.12.ko" 模块，然后再插入
"BiscuitOS_bootmem-2.6.12-buddy.ko" 模块。如下:

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI000773.png)

以上便是测试代码的使用办法。开发者如果想在源码中启用或关闭某些宏，可以
修改 Makefile 中内容:

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI000774.png)

从上图可以知道，如果要启用某些宏，可以在 ccflags-y 中添加 "-D" 选项进行
启用，源码的编译参数也可以添加到 ccflags-y 中去。开发者除了使用上面的办法
进行测试之外，也可以使用项目提供的 initcall 机制进行调试，具体请参考:

> - [Initcall 机制调试说明](#C00032)

由于 bootmem 内存分配器只存在于内存管理子系统的早期，因此只能在内存管理
早期进行测试，Initcall 机制提供了以下函数用于 bootmem 调试:

{% highlight bash %}
bootmem_initcall_bs()
{% endhighlight %}

从项目的 Initcall 机制可以知道，bootmem_initcall_bs() 调用的函数将
在 bootmem 分配器初始化完毕之后自动调用，因此可用此法调试 bootmem。
bootmem 相关的测试代码位于:

{% highlight bash %}
BiscuitOS/output/linux-5.0-arm32/package/BiscuitOS_bootmem-2.6.12/BiscuitOS_bootmem-2.6.12/module/bootmem/
{% endhighlight %}

在 Makefile 中打开调试开关:

{% highlight bash %}
$(MODULE_NAME)-m                += modules/bootmem/main.o
{% endhighlight %}

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND000100.png)

----------------------------------

<span id="H"></span>

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND00000T.jpg)

#### Bootmem 历史补丁

> - [Bootmem Linux 2.6.12](#H-linux-2.6.12)
>
> - [Bootmem Linux 2.6.12.1](#H-linux-2.6.12.1)
>
> - [Bootmem Linux 2.6.12.2](#H-linux-2.6.12.2)
>
> - [Bootmem Linux 2.6.12.3](#H-linux-2.6.12.3)
>
> - [Bootmem Linux 2.6.12.4](#H-linux-2.6.12.4)
>
> - [Bootmem Linux 2.6.12.5](#H-linux-2.6.12.5)
>
> - [Bootmem Linux 2.6.12.6](#H-linux-2.6.12.6)
>
> - [Bootmem Linux 2.6.13](#H-linux-2.6.13)
>
> - [Bootmem Linux 2.6.13.1](#H-linux-2.6.13.1)
>
> - [Bootmem Linux 2.6.14](#H-linux-2.6.14)
>
> - [Bootmem Linux 2.6.15](#H-linux-2.6.15)

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND000100.png)

----------------------------------------------

<span id="H-linux-2.6.12"></span>

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI000785.JPG)

#### Bootmem Linux 2.6.12

Linux 2.6.12 依旧采用 bootmem 作为其早期的内存管理器。采用 bitmap 管理所有
的可用物理内存。

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI000775.png)

向外提供了用于分配内存的接口:

{% highlight bash %}
alloc_bootmem()
alloc_bootmem_low()
alloc_bootmem_node()
alloc_bootmem_low_pages()
alloc_bootmem_pages()
__alloc_bootmem()
__alloc_bootmem_node()
alloc_large_system_hash()
{% endhighlight %}

向外提供用于回收内存的接口:

{% highlight bash %}
free_all_bootmem_node()
free_bootmem()
free_bootmem_node()
{% endhighlight %}

向外提供用于预留内存的接口:

{% highlight bash %}
reserve_bootmem()
{% endhighlight %}

具体函数解析说明，请查看:

> - [Bootmem API](#K)

###### 与项目相关

bootmem 内存分配器与本项目相关的调用顺序如下:

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI000804.png)

###### 补丁

对于 Linux 2.6.12 的补丁，Linus 将 Linux 内核源码树加入到 git 中来，因此
这个版本的代码均不产生补丁。更多补丁的使用请参考:

> - [BiscuitOS Memory Manager Patch 建议](https://biscuitos.github.io/blog/HISTORY-MMU/#C00033)

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND000100.png)

----------------------------------------------

<span id="H-linux-2.6.12.1"></span>

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI000786.JPG)

#### Bootmem Linux 2.6.12.1

Linux 2.6.12.1 依旧采用 bootmem 作为其早期的内存管理器。采用 bitmap 管理所有
的可用物理内存。

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI000775.png)

向外提供了用于分配内存的接口:

{% highlight bash %}
alloc_bootmem()
alloc_bootmem_low()
alloc_bootmem_node()
alloc_bootmem_low_pages()
alloc_bootmem_pages()
__alloc_bootmem()
__alloc_bootmem_node()
alloc_large_system_hash()
{% endhighlight %}

向外提供用于回收内存的接口:

{% highlight bash %}
free_all_bootmem_node()
free_bootmem()
free_bootmem_node()
{% endhighlight %}

向外提供用于预留内存的接口:

{% highlight bash %}
reserve_bootmem()
{% endhighlight %}

具体函数解析说明，请查看:

> - [Bootmem API](#K)

###### 与项目相关

bootmem 内存分配器与本项目相关的调用顺序如下:

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI000804.png)

###### 补丁

对于 Linux 2.6.12 的补丁，Linus 将 Linux 内核源码树加入到 git 中来，因此
这个版本的代码均不产生补丁。更多补丁的使用请参考:

> - [BiscuitOS Memory Manager Patch 建议](https://biscuitos.github.io/blog/HISTORY-MMU/#C00033)

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND000100.png)

----------------------------------------------

<span id="H-linux-2.6.12.2"></span>

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI000787.JPG)

#### Bootmem Linux 2.6.12.2

Linux 2.6.12.2 依旧采用 bootmem 作为其早期的内存管理器。采用 bitmap 管理所有
的可用物理内存。

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI000775.png)

向外提供了用于分配内存的接口:

{% highlight bash %}
alloc_bootmem()
alloc_bootmem_low()
alloc_bootmem_node()
alloc_bootmem_low_pages()
alloc_bootmem_pages()
__alloc_bootmem()
__alloc_bootmem_node()
alloc_large_system_hash()
{% endhighlight %}

向外提供用于回收内存的接口:

{% highlight bash %}
free_all_bootmem_node()
free_bootmem()
free_bootmem_node()
{% endhighlight %}

向外提供用于预留内存的接口:

{% highlight bash %}
reserve_bootmem()
{% endhighlight %}

具体函数解析说明，请查看:

> - [Bootmem API](#K)

###### 与项目相关

bootmem 内存分配器与本项目相关的调用顺序如下:

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI000804.png)

###### 补丁

相对上一个版本 linux 2.6.12.1，bootmem 内存分配器并未做改动。这个版本
的代码均不产生补丁。更多补丁的使用请参考:

> - [BiscuitOS Memory Manager Patch 建议](https://biscuitos.github.io/blog/HISTORY-MMU/#C00033)

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND000100.png)

----------------------------------------------

<span id="H-linux-2.6.12.3"></span>

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI000788.JPG)

#### Bootmem Linux 2.6.12.3

Linux 2.6.12.3 依旧采用 bootmem 作为其早期的内存管理器。采用 bitmap 管理所有
的可用物理内存。

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI000775.png)

向外提供了用于分配内存的接口:

{% highlight bash %}
alloc_bootmem()
alloc_bootmem_low()
alloc_bootmem_node()
alloc_bootmem_low_pages()
alloc_bootmem_pages()
__alloc_bootmem()
__alloc_bootmem_node()
alloc_large_system_hash()
{% endhighlight %}

向外提供用于回收内存的接口:

{% highlight bash %}
free_all_bootmem_node()
free_bootmem()
free_bootmem_node()
{% endhighlight %}

向外提供用于预留内存的接口:

{% highlight bash %}
reserve_bootmem()
{% endhighlight %}

具体函数解析说明，请查看:

> - [Bootmem API](#K)

###### 与项目相关

bootmem 内存分配器与本项目相关的调用顺序如下:

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI000804.png)

###### 补丁

相对上一个版本 linux 2.6.12.2，bootmem 内存分配器并未做改动。这个版本
的代码均不产生补丁。更多补丁的使用请参考:

> - [BiscuitOS Memory Manager Patch 建议](https://biscuitos.github.io/blog/HISTORY-MMU/#C00033)

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND000100.png)

----------------------------------------------

<span id="H-linux-2.6.12.4"></span>

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI000789.JPG)

#### Bootmem Linux 2.6.12.4

Linux 2.6.12.4 依旧采用 bootmem 作为其早期的内存管理器。采用 bitmap 管理所有
的可用物理内存。

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI000775.png)

向外提供了用于分配内存的接口:

{% highlight bash %}
alloc_bootmem()
alloc_bootmem_low()
alloc_bootmem_node()
alloc_bootmem_low_pages()
alloc_bootmem_pages()
__alloc_bootmem()
__alloc_bootmem_node()
alloc_large_system_hash()
{% endhighlight %}

向外提供用于回收内存的接口:

{% highlight bash %}
free_all_bootmem_node()
free_bootmem()
free_bootmem_node()
{% endhighlight %}

向外提供用于预留内存的接口:

{% highlight bash %}
reserve_bootmem()
{% endhighlight %}

具体函数解析说明，请查看:

> - [Bootmem API](#K)

###### 与项目相关

bootmem 内存分配器与本项目相关的调用顺序如下:

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI000804.png)

###### 补丁

相对上一个版本 linux 2.6.12.3，bootmem 内存分配器并未做改动。这个版本
的代码均不产生补丁。更多补丁的使用请参考:

> - [BiscuitOS Memory Manager Patch 建议](https://biscuitos.github.io/blog/HISTORY-MMU/#C00033)

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND000100.png)

----------------------------------------------

<span id="H-linux-2.6.12.5"></span>

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI000790.JPG)

#### Bootmem Linux 2.6.12.5

Linux 2.6.12.5 依旧采用 bootmem 作为其早期的内存管理器。采用 bitmap 管理所有
的可用物理内存。

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI000775.png)

向外提供了用于分配内存的接口:

{% highlight bash %}
alloc_bootmem()
alloc_bootmem_low()
alloc_bootmem_node()
alloc_bootmem_low_pages()
alloc_bootmem_pages()
__alloc_bootmem()
__alloc_bootmem_node()
alloc_large_system_hash()
{% endhighlight %}

向外提供用于回收内存的接口:

{% highlight bash %}
free_all_bootmem_node()
free_bootmem()
free_bootmem_node()
{% endhighlight %}

向外提供用于预留内存的接口:

{% highlight bash %}
reserve_bootmem()
{% endhighlight %}

具体函数解析说明，请查看:

> - [Bootmem API](#K)

###### 与项目相关

bootmem 内存分配器与本项目相关的调用顺序如下:

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI000804.png)

###### 补丁

相对上一个版本 linux 2.6.12.4，bootmem 内存分配器并未做改动。这个版本
的代码均不产生补丁。更多补丁的使用请参考:

> - [BiscuitOS Memory Manager Patch 建议](https://biscuitos.github.io/blog/HISTORY-MMU/#C00033)

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND000100.png)

----------------------------------------------

<span id="H-linux-2.6.12.6"></span>

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI000791.JPG)

#### Bootmem Linux 2.6.12.6

Linux 2.6.12.6 依旧采用 bootmem 作为其早期的内存管理器。采用 bitmap 管理所有
的可用物理内存。

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI000775.png)

向外提供了用于分配内存的接口:

{% highlight bash %}
alloc_bootmem()
alloc_bootmem_low()
alloc_bootmem_node()
alloc_bootmem_low_pages()
alloc_bootmem_pages()
__alloc_bootmem()
__alloc_bootmem_node()
alloc_large_system_hash()
{% endhighlight %}

向外提供用于回收内存的接口:

{% highlight bash %}
free_all_bootmem_node()
free_bootmem()
free_bootmem_node()
{% endhighlight %}

向外提供用于预留内存的接口:

{% highlight bash %}
reserve_bootmem()
{% endhighlight %}

具体函数解析说明，请查看:

> - [Bootmem API](#K)

###### 与项目相关

bootmem 内存分配器与本项目相关的调用顺序如下:

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI000804.png)

###### 补丁

相对上一个版本 linux 2.6.12.5，bootmem 内存分配器并未做改动。这个版本
的代码均不产生补丁。更多补丁的使用请参考:

> - [BiscuitOS Memory Manager Patch 建议](https://biscuitos.github.io/blog/HISTORY-MMU/#C00033)

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND000100.png)

----------------------------------------------

<span id="H-linux-2.6.13"></span>

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI000792.JPG)

#### Bootmem Linux 2.6.13

Linux 2.6.13 依旧采用 bootmem 作为其早期的内存管理器。采用 bitmap 管理所有
的可用物理内存。

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI000775.png)

向外提供了用于分配内存的接口:

{% highlight bash %}
alloc_bootmem()
alloc_bootmem_low()
alloc_bootmem_node()
alloc_bootmem_low_pages()
alloc_bootmem_pages()
__alloc_bootmem()
__alloc_bootmem_node()
alloc_large_system_hash()
{% endhighlight %}

向外提供用于回收内存的接口:

{% highlight bash %}
free_all_bootmem_node()
free_bootmem()
free_bootmem_node()
{% endhighlight %}

向外提供用于预留内存的接口:

{% highlight bash %}
reserve_bootmem()
{% endhighlight %}

具体函数解析说明，请查看:

> - [Bootmem API](#K)

###### 与项目相关

bootmem 内存分配器与本项目相关的调用顺序如下:

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI000804.png)

###### 补丁

相对上一个版本 linux 2.6.12.6，bootmem 内存分配器提交了四个补丁。如下:

{% highlight bash %}
tig mm/bootmem.c include/linux/bootmem.h

2005-06-23 00:07 Dave Hansen    o [PATCH] sparsemem base: simple NUMA remap space allocator
                                  [main] 6f167ec721108c9282d54424516a12c805e3c306 - commit 4 of 5
2005-06-23 00:07 Andy Whitcroft o [PATCH] sparsemem memory model
                                  [main] d41dee369bff3b9dcb6328d4d822926c28cc2594 - commit 3 of 5
2005-06-25 14:58 Vivek Goyal    o [PATCH] kdump: Retrieve saved max pfn
                                  [main] 92aa63a5a1bf2e7b0c79e6716d24b76dbbdcf951 - commit 2 of 5
2005-06-25 14:59 Nick Wilson    o [PATCH] Use ALIGN to remove duplicate code
                                  [main] 8c0e33c133021ee241e9d51255b9fb18eb34ef0e - commit 1 of 5
{% endhighlight %}

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI000796.png)

{% highlight bash %}
git format-patch -1 6f167ec721108c9282d54424516a12
vi 0001-PATCH-sparsemem-base-simple-NUMA-remap-space-allocat.patch
{% endhighlight %}

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI000797.png)

对应 bootmem 内存分配器来说，这个补丁添加了在不支持 CONFIG_HAVE_ARCH_ALLOC_REMAP
的情况下，提供 "alloc_remap()" 函数的实现.

{% highlight bash %}
git format-patch -1 d41dee369bff3b9dcb6328d4d822926c28cc259
vi 0001-PATCH-sparsemem-memory-model.patch
{% endhighlight %}

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI000809.png)

该 PATCH 添加了 bootmem 内存分配器支持早期的 PFN 和 page 之间的转换。

{% highlight bash %}
git format-patch -1 92aa63a5a1bf2e7b0c79e6716d24b76dbbdcf951
vi 0001-PATCH-kdump-Retrieve-saved-max-pfn.patch
{% endhighlight %}

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI000798.png)

该补丁通过描述可以知道在 i386 体系结构中，如果启用 CONFIG_CRASH_DUMP 宏，
那么会在 bootmem.c 中定义一个变量 "saved_max_pfn", 这个全局变量用于存储
系统中最大物理帧的值，以便当 max_pfn 变量被修改之后，"saved_max_pfn" 还可以
基础标志系统使用的最大物理帧，这样将有效确保用户对物理内存的操作不会超过
"saved_max_pfn" 之外.

{% highlight bash %}
git format-patch -1 8c0e33c133021ee241e9d51255b9fb18eb34ef0e
vi 0001-PATCH-Use-ALIGN-to-remove-duplicate-code.patch
{% endhighlight %}

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI000799.png)

这个补丁只是将 bootmem 内存分配中对齐操作全部替换成了 ALIGN() 函数。
更多补丁的使用请参考:

> - [BiscuitOS Memory Manager Patch 建议](https://biscuitos.github.io/blog/HISTORY-MMU/#C00033)

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND000100.png)

----------------------------------------------

<span id="H-linux-2.6.13.1"></span>

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI000793.JPG)

#### Bootmem Linux 2.6.13.1

Linux 2.6.13.1 依旧采用 bootmem 作为其早期的内存管理器。采用 bitmap 管理所有
的可用物理内存。

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI000775.png)

向外提供了用于分配内存的接口:

{% highlight bash %}
alloc_bootmem()
alloc_bootmem_low()
alloc_bootmem_node()
alloc_bootmem_low_pages()
alloc_bootmem_pages()
__alloc_bootmem()
__alloc_bootmem_node()
alloc_large_system_hash()
{% endhighlight %}

向外提供用于回收内存的接口:

{% highlight bash %}
free_all_bootmem_node()
free_bootmem()
free_bootmem_node()
{% endhighlight %}

向外提供用于预留内存的接口:

{% highlight bash %}
reserve_bootmem()
{% endhighlight %}

具体函数解析说明，请查看:

> - [Bootmem API](#K)

###### 与项目相关

bootmem 内存分配器与本项目相关的调用顺序如下:

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI000804.png)

###### 补丁

相对上一个版本 linux 2.6.13，bootmem 内存分配器并未做改动。这个版本
的代码均不产生补丁。更多补丁的使用请参考:

> - [BiscuitOS Memory Manager Patch 建议](https://biscuitos.github.io/blog/HISTORY-MMU/#C00033)

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND000100.png)

----------------------------------------------

<span id="H-linux-2.6.14"></span>

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI000794.JPG)

#### Bootmem Linux 2.6.14

Linux 2.6.14 依旧采用 bootmem 作为其早期的内存管理器。采用 bitmap 管理所有
的可用物理内存。

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI000775.png)

向外提供了用于分配内存的接口:

{% highlight bash %}
alloc_bootmem()
alloc_bootmem_low()
alloc_bootmem_node()
alloc_bootmem_low_pages()
alloc_bootmem_pages()
__alloc_bootmem()
__alloc_bootmem_node()
alloc_large_system_hash()
alloc_bootmem_limit()
__alloc_bootmem_limit()
alloc_bootmem_low_limit()
__alloc_bootmem_limit()
alloc_bootmem_node_limit()
alloc_bootmem_pages_node_limit()
alloc_bootmem_low_pages_node_limit()
{% endhighlight %}

向外提供用于回收内存的接口:

{% highlight bash %}
free_all_bootmem_node()
free_bootmem()
free_bootmem_node()
{% endhighlight %}

向外提供用于预留内存的接口:

{% highlight bash %}
reserve_bootmem()
{% endhighlight %}

具体函数解析说明，请查看:

> - [Bootmem API](#K)

###### 与项目相关

bootmem 内存分配器与本项目相关的调用顺序如下:

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI000804.png)

###### 补丁

相对上一个版本 linux 2.6.13.1，bootmem 内存分配器增加了多个补丁，如下:

{% highlight bash %}
tig mm/bootmem.c include/linux/bootmem.h

2005-09-12 18:49 Andi Kleen     o [PATCH] x86-64: Reverse order of bootmem lists
                                  [main] 5d3d0f7704ed0bc7eaca0501eeae3e5da1ea6c87 - commit 3 of 8
2005-09-30 12:38 Linus Torvalds o Revert "x86-64: Reverse order of bootmem lists"
                                  [main] 6e3254c4e2927c117044a02acf5f5b56e1373053 - commit 2 of 8
2005-10-19 15:52 Yasunori Goto  o [PATCH] swiotlb: make sure initial DMA allocations really are in DMA memory
                                  [main] 281dd25cdc0d6903929b79183816d151ea626341 - commit 1 of 8
{% endhighlight %}

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI000800.png)

{% highlight bash %}
git format-patch -1 5d3d0f7704ed0bc7eaca0501eeae3e5da1ea6c87
vi 0001-PATCH-x86-64-Reverse-order-of-bootmem-lists.patch
{% endhighlight %}

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI000801.png)

补丁修改了 bootmem 分配器的分配顺序，从 node 0 开始而不是从最后一个 node 开始。

{% highlight bash %}
git format-patch -1 6e3254c4e2927c117044a02acf5f5b56e1373053
vi 0001-Revert-x86-64-Reverse-order-of-bootmem-lists.patch
{% endhighlight %}

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI000802.png)

{% highlight bash %}
git format-patch -1 281dd25cdc0d6903929b79183816d151ea626341
vi 0001-PATCH-swiotlb-make-sure-initial-DMA-allocations-real.patch
{% endhighlight %}

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI000803.png)

该补丁中，由于对 bootmem 分配的内存进行了限制，因此产生了一套待限制的
分配函数，以便在使用 bootmem 分配时能够限制范围。bootmem 提供了这些函数
的具体实现过程。更多补丁的使用请参考:

> - [BiscuitOS Memory Manager Patch 建议](https://biscuitos.github.io/blog/HISTORY-MMU/#C00033)

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND000100.png)

----------------------------------------------

<span id="H-linux-2.6.15"></span>

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI000795.JPG)

#### Bootmem Linux 2.6.15

Linux 2.6.15 依旧采用 bootmem 作为其早期的内存管理器。采用 bitmap 管理所有
的可用物理内存。

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI000775.png)

向外提供了用于分配内存的接口:

{% highlight bash %}
alloc_bootmem()
alloc_bootmem_low()
alloc_bootmem_node()
alloc_bootmem_low_pages()
alloc_bootmem_pages()
__alloc_bootmem()
__alloc_bootmem_node()
alloc_large_system_hash()
alloc_bootmem_limit()
__alloc_bootmem_limit()
alloc_bootmem_low_limit()
__alloc_bootmem_limit()
alloc_bootmem_node_limit()
alloc_bootmem_pages_node_limit()
alloc_bootmem_low_pages_node_limit()
{% endhighlight %}

向外提供用于回收内存的接口:

{% highlight bash %}
free_all_bootmem_node()
free_bootmem()
free_bootmem_node()
{% endhighlight %}

向外提供用于预留内存的接口:

{% highlight bash %}
reserve_bootmem()
{% endhighlight %}

具体函数解析说明，请查看:

> - [Bootmem API](#K)

###### 与项目相关

bootmem 内存分配器与本项目相关的调用顺序如下:

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI000804.png)

###### 补丁

相对上一个版本 linux 2.6.14，bootmem 内存分配器增加了两个补丁，如下:

{% highlight bash %}
tig mm/bootmem.c include/linux/bootmem.h

2005-12-12 00:37 Haren Myneni   o [PATCH] fix in __alloc_bootmem_core() when there is no free page in first node's memory
                                  [main] 66d43e98ea6ff291cd4e524386bfb99105feb180 - commit 1 of 10
2005-10-29 18:16 Nick Piggin    o [PATCH] core remove PageReserved
                                  [main] b5810039a54e5babf428e9a1e89fc1940fabff11 - commit 2 of 10
{% endhighlight %}

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI000805.png)

{% highlight bash %}
git format-patch -1 b5810039a54e5babf428e9a1e89fc1940fabff11
vi 0001-PATCH-core-remove-PageReserved.patch
{% endhighlight %}

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI000806.png)

补丁在 free_all_bootmem_core() 函数中添加了将 struct page 的引用计数设置为 0.
该函数的主要用途就是将 bootmem 管理的可用物理页传递给 buddy 内存分配器。

{% highlight bash %}
git format-patch -1 66d43e98ea6ff291cd4e524386bfb99105feb180
vi 0001-PATCH-fix-in-__alloc_bootmem_core-when-there-is-no-f.patch
{% endhighlight %}

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI000807.png)

这个补丁用于解决当第一个 node 没有可用的物理内存之后，bootmem 内存分配器
应该去下一个 node 中查找. 在 \_\_alloc_bootmem_core() 函数中，restart_scan
继续查找可用的物理内存，当当前 node 没有可用的物理内存之后，即条件满足就
跳出循环，继续去下一个 node 中查找。更多补丁的使用请参考:

> - [BiscuitOS Memory Manager Patch 建议](https://biscuitos.github.io/blog/HISTORY-MMU/#C00033)

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND000100.png)

-----------------------------------------------

<span id="G"></span>

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND00000H.jpg)

#### Bootmem 历史时间轴

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI000808.png)

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND000100.png)

-----------------------------------------------

<span id="K"></span>

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND00000B.jpg)

#### Bootmem API

###### \_\_alloc_bootmem

{% highlight bash %}
static inline void *__alloc_bootmem (unsigned long size, 
                                   unsigned long align, unsigned long goal)
  作用: bootmem 分配器分配指定大小的物理内存.
{% endhighlight %}

###### \_\_alloc_bootmem_core

{% highlight bash %}
static void * __init
__alloc_bootmem_core(struct bootmem_data *bdata, unsigned long size,
              unsigned long align, unsigned long goal, unsigned long limit)
  作用: bootmem 分配器分配内存核心实现
{% endhighlight %}

###### \_\_alloc_bootmem_limit

{% highlight bash %}
void * __init __alloc_bootmem_limit (unsigned long size, unsigned long align, 
                                unsigned long goal, unsigned long limit)
  作用: bootmem 分配器在限定的范围内分配可用物理内存.
{% endhighlight %}

###### \_\_alloc_bootmem_node

{% highlight bash %}
static inline void *__alloc_bootmem_node (pg_data_t *pgdat, 
              unsigned long size, unsigned long align, unsigned long goal)
  作用: bootmem 分配器在指定 node 上分配可用物理内存.
{% endhighlight %}

###### \_\_alloc_bootmem_node_limit

{% highlight bash %}
void * __init __alloc_bootmem_node_limit (pg_data_t *pgdat, unsigned long size, 
               unsigned long align, unsigned long goal, unsigned long limit)
  作用: bootmem 分配器在指定的 node 和限定范围内分配可用物理内存.
{% endhighlight %}

###### alloc_bootmem

{% highlight bash %}
#define alloc_bootmem(x) \
        __alloc_bootmem((x), SMP_CACHE_BYTES, __pa(MAX_DMA_ADDRESS))
  作用: bootmem 分配器分配指定大小的物理内存.
{% endhighlight %}

###### alloc_bootmem_limit

{% highlight bash %}
#define alloc_bootmem_limit(x, limit) \
        __alloc_bootmem_limit((x), SMP_CACHE_BYTES, __pa(MAX_DMA_ADDRESS), (limit))
  作用: bootmem 分配器在限定的范围内分配可用物理内存.
{% endhighlight %}

###### alloc_bootmem_low

{% highlight bash %}
#define alloc_bootmem_low(x) \
        __alloc_bootmem((x), SMP_CACHE_BYTES, 0)
  作用: bootmem 分配器从低端开始分配指定大小的可用物理内存.
{% endhighlight %}

###### alloc_bootmem_low_limit

{% highlight bash %}
#define alloc_bootmem_low_limit(x, limit)                       \
        __alloc_bootmem_limit((x), SMP_CACHE_BYTES, 0, (limit))
  作用: bootmem 分配器从低端开始，在限定范围内分配可用物理内存.
{% endhighlight %}

###### alloc_bootmem_node

{% highlight bash %}
#define alloc_bootmem_node(pgdat, x) \
        __alloc_bootmem_node((pgdat), (x), SMP_CACHE_BYTES, __pa(MAX_DMA_ADDRESS))
  作用: bootmem 在指定 node 上，分配指定大小的物理内存.
{% endhighlight %}

###### alloc_bootmem_node_limit

{% highlight bash %}
#define alloc_bootmem_node_limit(pgdat, x, limit)                       \
        __alloc_bootmem_node_limit((pgdat), (x), SMP_CACHE_BYTES, __pa(MAX_DMA_ADDRESS), (limit))
  作用: bootmem 分配器在指定 node 上，在限定范围内分配可用物理内存.
{% endhighlight %}

###### alloc_bootmem_pages

{% highlight bash %}
#define alloc_bootmem_pages(x) \
        __alloc_bootmem((x), PAGE_SIZE, __pa(MAX_DMA_ADDRESS))
  作用: bootmem 分配器以 PAGE_SIZE 为单位分配可用物理内存.
{% endhighlight %}

###### alloc_bootmem_pages_limit

{% highlight bash %}
#define alloc_bootmem_pages_limit(x, limit)                             \
        __alloc_bootmem_limit((x), PAGE_SIZE, __pa(MAX_DMA_ADDRESS), (limit))
  作用: bootmem 分配器在限定范围内，以 PAGE_SIZE 为单位分配可用物理内存.
{% endhighlight %}

###### alloc_bootmem_pages_node

{% highlight bash %}
#define alloc_bootmem_pages_node(pgdat, x) \
        __alloc_bootmem_node((pgdat), (x), PAGE_SIZE, __pa(MAX_DMA_ADDRESS))
  作用: bootmem 分配器在指定 node 上，以 PAGE_SIZE 为单位分配可用物理内存.
{% endhighlight %}

###### alloc_bootmem_pages_node_limit

{% highlight bash %}
#define alloc_bootmem_pages_node_limit(pgdat, x, limit)                 \
        __alloc_bootmem_node_limit((pgdat), (x), PAGE_SIZE, __pa(MAX_DMA_ADDRESS), (limit))
  作用: bootmem 分配器在指定 node 上，以 PAGE_SIZE 为单位，在限定范围内分配
        可用物理内存.
{% endhighlight %}

###### alloc_bootmem_low_pages

{% highlight bash %}
#define alloc_bootmem_low_pages(x) \
        __alloc_bootmem((x), PAGE_SIZE, 0)
  作用: bootmem 分配从低端开始，以 PAGE_SIZE 为单位分配可用物理内存.
{% endhighlight %}

###### alloc_bootmem_low_pages_limit

{% highlight bash %}
#define alloc_bootmem_low_pages_limit(x, limit)         \
        __alloc_bootmem_limit((x), PAGE_SIZE, 0, (limit))
  作用: bootmem 分配器从低端开始，以 PAGE_SIZE 为单位在指定范围内分配可用内存.
{% endhighlight %}

###### alloc_bootmem_low_pages_node

{% highlight bash %}
#define alloc_bootmem_low_pages_node(pgdat, x) \
        __alloc_bootmem_node((pgdat), (x), PAGE_SIZE, 0)
  作用: bootmem 分配器在指定节点上，以 PAGE_SIZE 为单位，从低端开始分配可用
        物理内存.
{% endhighlight %}

###### alloc_bootmem_low_pages_node_limit

{% highlight bash %}
#define alloc_bootmem_low_pages_node_limit(pgdat, x, limit)             \
        __alloc_bootmem_node_limit((pgdat), (x), PAGE_SIZE, 0, (limit))
  作用: bootmem 分配器在指定节点上，以 PAGE_SIZE 为单位，从低端开始在限定
        范围内分配可用物理内存.
{% endhighlight %}

###### bootmem_bootmap_pages

{% highlight bash %}
unsigned long __init bootmem_bootmap_pages (unsigned long pages)
   作用: 计算参数 pages 需要的 bit 数量.
{% endhighlight %}

###### free_all_bootmem

{% highlight bash %}
unsigned long __init free_all_bootmem (void)
  作用: bootmem 分配器转移所有可用物理内存给 buddy 内存分配器.
{% endhighlight %}

###### free_all_bootmem_core

{% highlight bash %}
static unsigned long __init free_all_bootmem_core(pg_data_t *pgdat)
  作用: bootmem 分配器将所有可用物理页转移给 buddy 内存分配器核心实现。
{% endhighlight %}

###### free_all_bootmem_node

{% highlight bash %}
unsigned long __init free_all_bootmem_node (pg_data_t *pgdat)
  作用: bootmem 将特定节点上的物理页转移给 buddy 内存分配器.
{% endhighlight %}

###### free_bootmem

{% highlight bash %}
void __init free_bootmem (unsigned long addr, unsigned long size)
  作用: bootmem 分配器释放指定的物理内存
{% endhighlight %}

###### free_bootmem_core

{% highlight bash %}
static void __init free_bootmem_core(bootmem_data_t *bdata, 
                                    unsigned long addr, unsigned long size)
  作用: bootmem 内存分配器释放内存的核心实现
{% endhighlight %}

###### free_bootmem_node

{% highlight bash %}
void __init free_bootmem_node (pg_data_t *pgdat, unsigned long physaddr, 
                                                        unsigned long size)
  作用: bootmem 分配器按 node 释放指定物理内存.
{% endhighlight %}

###### init_bootmem

{% highlight bash %}
unsigned long __init init_bootmem (unsigned long start, unsigned long pages)
  作用: bootmme 内存分配器初始化.
{% endhighlight %}

###### init_bootmem_core

{% highlight bash %}
static unsigned long __init init_bootmem_core (pg_data_t *pgdat,
        unsigned long mapstart, unsigned long start, unsigned long end)
  作用: 初始化 bootmem 相关的基础数据.
{% endhighlight %}

###### init_bootmem_node

{% highlight bash %}
unsigned long __init init_bootmem_node (pg_data_t *pgdat, 
        unsigned long freepfn, unsigned long startpfn, unsigned long endpfn)
  作用: bootmem 按 node 初始化 node 内的物理页.
{% endhighlight %}

###### reserve_bootmem

{% highlight bash %}
void __init reserve_bootmem (unsigned long addr, unsigned long size)
  作用: bootmem 分配器预留指定的物理内存.
{% endhighlight %}

###### reserve_bootmem_core

{% highlight bash %}
static void __init reserve_bootmem_core(bootmem_data_t *bdata, 
                                     unsigned long addr, unsigned long size)
  作用: bootmem 分配器预留内存的核心实现.
{% endhighlight %}

###### reserve_bootmem_node

{% highlight bash %}
void __init reserve_bootmem_node (pg_data_t *pgdat, unsigned long physaddr, 
                                                          unsigned long size)
  作用: bootmem 按 node 预留指定的物理页.
{% endhighlight %}


-----------------------------------------------

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

---
layout: post
title:  "PERCPU Allocator"
date:   2020-05-07 09:36:30 +0800
categories: [HW]
excerpt: MMU.
tags:
  - MMU
---

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND00000L0.PNG)

> Email: BuddyZhang1 <buddy.zhang@aliyun.com>

#### 目录

> - [PERCPU 分配器原理](#A)
>
> - [PERCPU 分配器使用](#B)
>
> - [PERCPU 分配器实践](#C)
>
> - [PERCPU 源码分析](#D)
>
> - [PERCPU 分配器调试](#C0004)
>
> - [PERCPU 分配进阶研究](#F)
>
> - [PERCPU 时间轴](#G)
>
> - [PERCPU 历史补丁](#H)
>
> - [PERCPU API](#K)
>
> - [附录/捐赠](#Z0)

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND000100.png)

----------------------------------

<span id="A"></span>

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND00000T.jpg)

#### PERCPU 分配器原理

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/HK000236.png)

PERCPU 机制的存在是用于在 SMP/UP 系统中，系统为了让每个 CPU 都对某个变量具有
私有副本，因此 PERCPU 变量孕育而生。系统每当定义一个 PERCPU 变量之后，系统
都会根据当前系统 CPU 数量定义多分副本，每个 CPU 使用自己的副本而无需加锁。
PERCPU 变量又称为 CPU 私有变量。PERCPU 变量可以通过静态声明定义，也可以通过
PERCPU 内存分配器进行动态分配。PERCPU 分配器就是用动态管理 PERCPU 变量的
分配和回收。

###### 静态 PERPCU

{% highlight bash %}
static DEFINE_PER_CPU(struct node_percpu, node_percpu_bs);
{% endhighlight %}

对于静态定义声明的 PERPCU 变量，内核提供了通用接口 "DEFINE_PER_CPU()",
在内核源码编译链接阶段，链接器将所有静态定义声明的 PERCPU 变量全部放置
在 .data.percpu section 内部。当 PERCPU/MEMBLOCK 初始化完毕之后，内核从
PERCPU/MEMBLOCK 分配器中分配指定的内存，然后将 .data.percpu section 内部
的数据全部拷贝到新分配的内存中，这段内存称为了 PERCPU 分配器使用并管理
的内存。但由于不同的 Linux 版本，实现也不尽相同。对于 Linux 2.6.x 系列，
静态 PERCPU 布局如下:

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI000810.png)

在 Linux 2.6.x 版本中，Linux 内核在 PERCPU 初始化完毕之后，buddy 初始化
之前，从可用物理内存中分配了一段可用的物理内存，然后将 ".data.percpu" section
的内容全部拷贝到新申请的物理内存，新申请的内存仅供用于存储静态 PERCPU 变量，
动态分配的 PERCPU 变量不使用这段内存。PERPCU 分配器通过变量 
\_\_per_cpu_offset 标示每个变量新内存地址与 .data.percpu 之间的位移，再通过相关
函数进行读取和修改。

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/HK000223.png)

对于 Linux 5.x 系列，静态 PERCPU 在编译和链接阶段也是全部存储在 ".data.percpu"
section 内，同样 Linux 在 MEMBLOCK 分配器初始化完毕之后，且 Buddy 分配器
初始化之前，从 MEMBLOCK 分配器中根据 CPU 的数量等条件申请一定长度的可用
物理内存，然后将 ".data.percpu" section 内部的数据全部拷贝到新申请的内存。与
之前版本不同的是新申请的内存是供静态和动态 PERPCU 变量共同使用。静态 PERCPU
的使用与之前版本一致。静态 PERCPU 变量的使用如下:

{% highlight c %}
#include <linux/percpu.h>

struct node_percpu {
        unsigned long index;
        unsigned long offset;
};

static __unused DEFINE_PER_CPU_BS(struct node_percpu, node_percpu_bs);

static int __unused TestCase_percpu(void)
{
        struct node_percpu *node;
        int cpu = 0;

        /* set percpu */
	for_each_possible_cpu(cpu)
                node = &per_cpu(node_percpu_bs, cpu);
                node->index = cpu + 0x80;
                node->offset = cpu * 0x100;

                cpu++;
                if (cpu < NR_CPUS)
                        prefetch(&per_cpu(node_percpu_bs, cpu));
        }

        /* get current cpu percpu */
        node = &__get_cpu_var(node_percpu_bs);
        printk("%d index %#lx\n", smp_processor_id(), node->index);

        /* get special cpu percpu */
        cpu = 2;
        BUG_ON(cpu >= NR_CPUS);
        node = &per_cpu(node_percpu_bs, cpu);
        printk("%d index %#lx\n", cpu, node->index);

        return 0;
}
{% endhighlight %}

###### 动态 PERPCU

PERCPU 内存分配器初始化完毕后，系统可以动态申请并使用 PERCPU 变量，PERCPU
内存分配器为一个 PERCPU 变量分配内存的时候，会根据当前 CPU 的数量为每个
CPU 分配足够副本空间，以便每个 CPU 独立使用自己的副本。但由于不同 Linux
版本 PERCPU 分配器的实现逻辑不一样，大概可以分为以下几个版本.

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI000811.png)

在 Linux 2.6.x 中，内核提供了 "alloc_percpu()" 和 "free_percpu()" 
两个函数实现了 PERCPU 变量的动态申请和释放。该版本内核使用以下结构维护
一个动态申请的 PERCPU 变量:

{% highlight bash %}
struct percpu_data {
        void *ptrs[NR_CPUS];
        void *blkp;
};
{% endhighlight %}

在调用 "alloc_percpu()" 函数的时候，内核为该 PERCPU 变量定义了一个
struct percpu_data 结构，并通过 kmalloc() 函数从 slab 内存分配器中分配指
定的内存。接着调用 kmalloc_node() 函数或者 kmalloc() 函数为每个 CPU 分配
PERCPU 变量对应结构大小的内存，最后将副本对应的内存清零。至于回收 PERCPU
变量，首先将释放该结构每个 CPU 的副本申请的内存，最后在释放 "struct percpu_data"
本身占用的内存。

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/HK000223.png)

在 Linux 5.x 中，内核也同样提供了 alloc_percpu() 和 free_percpu() 函数进行
PERCPU 的动态申请和回收，与 Linux 2.6.x 版本不同的是 PERCPU 分配器初始化
完毕之后，已经从 MEMBLOCK 分配器中获得一段内存，PERCPU 将这段内存分配成
与 CPU 数量一样的多块内存区域，每个 CPU 按顺序占用一段内存区域。PERCPU 内存
分配器采用 bitmap 的方式管理这些内存，当动态分配 PERCPU 变量的时候，PERCPU
内存分配器就通过 bitmap 找到符合要求的内存块，然后 PERCPU 在每个内存区域的
相同偏移处定义该变量的 CPU 副本，于是当访问 PERCPU 变量的时候，每个 CPU 就
访问该变量对应的副本。当 PERCPU 变量释放的时候，PERCPU 内存分配器将其对应
的 bitmap 位清零，表示该段内存可用。动态 PERCPU 变量的使用例程如下:

{% highlight bash %}
#include <linux/percpu.h>

struct node_percpu {
        unsigned long index;
        unsigned long offset;
};

static int __unused TestCase_alloc_percpu(void)
{
        struct node_percpu __percpu *np, *ptr;
        int cpu;

        /* Allocate percpu */
        np = alloc_percpu(struct node_percpu);
        if (!np) {
                printk("%s __alloc_percpu failed.\n", __func__);
                return -ENOMEM;
        }

        /* setup */
        for_each_possible_cpu(cpu) {
                ptr = per_cpu_ptr(np, cpu);
                ptr->index = cpu * 0x10;
        }

        /* usage */
        for_each_possible_cpu(cpu) {
                ptr = per_cpu_ptr(np, cpu);
                printk("CPU-%d Index %#lx\n", cpu, ptr->index);
        }

        /* free percpu */
        free_percpu(np);
        return 0;
}
{% endhighlight %}

---------------------------------

###### PERCPU 的优点

PERCPU 内存分配器可以动态分配 PERCPU 变量，每个 CPU 可以使用自己的副本
而无需加锁.

###### PERCPU 的缺点

对于 Linux 2.6.x 版本的静态 PERCPU 变量，如果当前系统在模块中未使用静态
的 PERCPU 变量，那么系统为静态 PERCPU 变量新分配的内存将会造成大量的浪费。

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND000100.png)

---------------------------------

<span id="B"></span>

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND00000H.jpg)

#### PERCPU 分配器使用

PERCPU 分配器提供了 PERCPU 变量的静态定义、动态申请和释放，以及相关 PERCPU
获得函数。

###### PERCPU 变量定义

{% highlight bash %}
DEFINE_PER_CPU()
__percpu
{% endhighlight %}

###### PERCPU 分配

{% highlight bash %}
alloc_percpu()
__alloc_percpu()
{% endhighlight %}

###### PERCPU 释放

{% highlight bash %}
free_percpu()
{% endhighlight %}

###### PERCPU 读取

{% highlight bash %}
for_each_possible_cpu()
per_cpu_ptr()
__get_cpu_var()
per_cpu
get_cpu_var()
put_cpu_var()
{% endhighlight %}

具体函数解析说明，请查看:

> - [PERCPU API](#K)

------------------------------------------------

<span id="C"></span>

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND00000I.jpg)

#### PERCPU 分配器实践

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
支持多个版本的 PERCPU，开发者可以根据需求进行选择，本文以 linux 2.6.12 
版本的 PERCPU 进行讲解。开发者使用如下命令:

{% highlight bash %}
cd BiscuitOS/
make linux-5.0-arm32_defconfig
make menuconfig
{% endhighlight %}

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI000746.png)

选择并进入 "[\*] Package  --->" 目录。

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI000747.png)

选择并进入 "[\*]   Memory Development History  --->" 目录。

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI000812.png)

选择并进入 "[\*]   PERCPU(UP/SMP) Allocator  --->" 目录。

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI000813.png)

选择 "[\*]   PERCPU on linux 2.6.12  --->" 目录，保存并退出。接着执行如下命令:

{% highlight bash %}
make
{% endhighlight %}

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI000750.png)

成功之后将出现上图的内容，接下来开发者执行如下命令以便切换到项目的路径:

{% highlight bash %}
cd BiscuitOS/output/linux-5.0-arm32/package/BiscuitOS_PERCPU-2.6.12
make download
{% endhighlight %}

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI000814.png)

至此源码已经下载完成，开发者可以使用 tree 等工具查看源码:

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI000815.png)

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
BiscuitOS/output/linux-5.0-arm32/linux/linux/arch/arm/boot/dts/vexpress-v2p-ca9.dts
{% endhighlight %}

添加完毕之后，使用如下命令更新 DTS:

{% highlight bash %}
cd BiscuitOS/output/linux-5.0-arm32/package/BiscuitOS_PERCPU-2.6.12
make kernel
{% endhighlight %}

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI000816.png)

--------------------------------------------

#### <span id="C0002">实践执行</span>

环境部署完毕之后，开发者可以向通用模块一样对源码进行编译和安装使用，使用
如下命令:

{% highlight bash %}
cd BiscuitOS/output/linux-5.0-arm32/package/BiscuitOS_PERCPU-2.6.12
make
{% endhighlight %}

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI000817.png)

以上就是模块成功编译，接下来将 ko 模块安装到 BiscuitOS 中，使用如下命令:

{% highlight bash %}
cd BiscuitOS/output/linux-5.0-arm32/package/BiscuitOS_PERCPU-2.6.12
make install
make pack
{% endhighlight %}

以上准备完毕之后，最后就是在 BiscuitOS 运行这个模块了，使用如下命令:

{% highlight bash %}
cd BiscuitOS/output/linux-5.0-arm32/package/BiscuitOS_PERCPU-2.6.12
make run
{% endhighlight %}

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI000818.png)

在 BiscuitOS 中插入了模块 "BiscuitOS_PERCPU-2.6.12.ko"，打印如上信息，那么
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
/xspace/OpenSource/BiscuitOS/BiscuitOS/output/linux-5.0-arm32/package/BiscuitOS_PERCPU-2.6.12/BiscuitOS_PERCPU-2.6.12/Makefile
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

然后先向 BiscuitOS 中插入 "BiscuitOS_PERCPU-2.6.12.ko" 模块，然后再插入
"BiscuitOS_PERCPU-2.6.12-buddy.ko" 模块。如下:

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI000773.png)

以上便是测试代码的使用办法。开发者如果想在源码中启用或关闭某些宏，可以
修改 Makefile 中内容:

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI000774.png)

从上图可以知道，如果要启用某些宏，可以在 ccflags-y 中添加 "-D" 选项进行
启用，源码的编译参数也可以添加到 ccflags-y 中去。开发者除了使用上面的办法
进行测试之外，也可以使用项目提供的 initcall 机制进行调试，具体请参考:

> - [Initcall 机制调试说明](#C00032)

Initcall 机制提供了以下函数用于 PERCPU 调试:

{% highlight bash %}
percpu_initcall_bs()
{% endhighlight %}

从项目的 Initcall 机制可以知道，percpu_initcall_bs() 调用的函数将
在 PERCPU 分配器初始化完毕之后自动调用，因此可用此法调试静态 PERCPU 变量。
对于动态分配的 PERCPU 变量，请使用 "slab_initcall_bs()" 之后的 INIT 入口
进行调试。PERCPU 相关的测试代码位于:

{% highlight bash %}
BiscuitOS/output/linux-5.0-arm32/package/BiscuitOS_PERCPU-2.6.12/BiscuitOS_PERCPU-2.6.12/module/percpu/
{% endhighlight %}

在 Makefile 中打开调试开关:

{% highlight bash %}
$(MODULE_NAME)-m                += modules/percpu/main.o
{% endhighlight %}

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND000100.png)

----------------------------------

<span id="H"></span>

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND00000T.jpg)

#### PERCPU 历史补丁

> - [PERCPU Linux 2.6.12](#H-linux-2.6.12)
>
> - [PERCPU Linux 2.6.12.1](#H-linux-2.6.12.1)
>
> - [PERCPU Linux 2.6.12.2](#H-linux-2.6.12.2)
>
> - [PERCPU Linux 2.6.12.3](#H-linux-2.6.12.3)
>
> - [PERCPU Linux 2.6.12.4](#H-linux-2.6.12.4)
>
> - [PERCPU Linux 2.6.12.5](#H-linux-2.6.12.5)
>
> - [PERCPU Linux 2.6.12.6](#H-linux-2.6.12.6)
>
> - [PERCPU Linux 2.6.13](#H-linux-2.6.13)
>
> - [PERCPU Linux 2.6.13.1](#H-linux-2.6.13.1)
>
> - [PERCPU Linux 2.6.14](#H-linux-2.6.14)
>
> - [PERCPU Linux 2.6.15](#H-linux-2.6.15)

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND000100.png)

----------------------------------------------

<span id="H-linux-2.6.12"></span>

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI000785.JPG)

#### PERCPU Linux 2.6.12

Linux 2.6.12 依旧采用 PERCPU 作为其早期的内存管理器。采用 bitmap 管理所有
的可用物理内存。

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI000810.png)

###### PERCPU 变量定义

{% highlight bash %}
DEFINE_PER_CPU()
__percpu
{% endhighlight %}

###### PERCPU 分配

{% highlight bash %}
alloc_percpu()
__alloc_percpu()
{% endhighlight %}

###### PERCPU 释放

{% highlight bash %}
free_percpu()
{% endhighlight %}

###### PERCPU 读取

{% highlight bash %}
for_each_possible_cpu()
per_cpu_ptr()
__get_cpu_var()
per_cpu
get_cpu_var()
put_cpu_var()
{% endhighlight %}

具体函数解析说明，请查看:

> - [PERCPU API](#K)

###### 与项目相关

PERCPU 内存分配器与本项目相关的调用顺序如下:

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI000819.png)

###### 补丁

对于 Linux 2.6.12 的补丁，Linus 将 Linux 内核源码树加入到 git 中来，因此
这个版本的代码均不产生补丁。更多补丁的使用请参考:

> - [BiscuitOS Memory Manager Patch 建议](https://biscuitos.github.io/blog/HISTORY-MMU/#C00033)

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND000100.png)

----------------------------------------------

<span id="H-linux-2.6.12.1"></span>

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI000786.JPG)

#### PERCPU Linux 2.6.12.1

Linux 2.6.12.1 依旧采用 PERCPU 作为其早期的内存管理器。采用 bitmap 管理所有
的可用物理内存。

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI000810.png)

向外提供了用于分配内存的接口:

###### PERCPU 变量定义

{% highlight bash %}
DEFINE_PER_CPU()
__percpu
{% endhighlight %}

###### PERCPU 分配

{% highlight bash %}
alloc_percpu()
__alloc_percpu()
{% endhighlight %}

###### PERCPU 释放

{% highlight bash %}
free_percpu()
{% endhighlight %}

###### PERCPU 读取

{% highlight bash %}
for_each_possible_cpu()
per_cpu_ptr()
__get_cpu_var()
per_cpu
get_cpu_var()
put_cpu_var()
{% endhighlight %}

> - [PERCPU API](#K)

###### 与项目相关

PERCPU 内存分配器与本项目相关的调用顺序如下:

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI000819.png)

###### 补丁

对于 Linux 2.6.12 的补丁，Linus 将 Linux 内核源码树加入到 git 中来，因此
这个版本的代码均不产生补丁。更多补丁的使用请参考:

> - [BiscuitOS Memory Manager Patch 建议](https://biscuitos.github.io/blog/HISTORY-MMU/#C00033)

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND000100.png)

----------------------------------------------

<span id="H-linux-2.6.12.2"></span>

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI000787.JPG)

#### PERCPU Linux 2.6.12.2

Linux 2.6.12.2 依旧采用 PERCPU 作为其早期的内存管理器。采用 bitmap 管理所有
的可用物理内存。

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI000810.png)

###### PERCPU 变量定义

{% highlight bash %}
DEFINE_PER_CPU()
__percpu
{% endhighlight %}

###### PERCPU 分配

{% highlight bash %}
alloc_percpu()
__alloc_percpu()
{% endhighlight %}

###### PERCPU 释放

{% highlight bash %}
free_percpu()
{% endhighlight %}

###### PERCPU 读取

{% highlight bash %}
for_each_possible_cpu()
per_cpu_ptr()
__get_cpu_var()
per_cpu
get_cpu_var()
put_cpu_var()
{% endhighlight %}

具体函数解析说明，请查看:

> - [PERCPU API](#K)

###### 与项目相关

PERCPU 内存分配器与本项目相关的调用顺序如下:

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI000819.png)

###### 补丁

相对上一个版本 linux 2.6.12.1，PERCPU 内存分配器并未做改动。这个版本
的代码均不产生补丁。更多补丁的使用请参考:

> - [BiscuitOS Memory Manager Patch 建议](https://biscuitos.github.io/blog/HISTORY-MMU/#C00033)

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND000100.png)

----------------------------------------------

<span id="H-linux-2.6.12.3"></span>

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI000788.JPG)

#### PERCPU Linux 2.6.12.3

Linux 2.6.12.3 依旧采用 PERCPU 作为其早期的内存管理器。采用 bitmap 管理所有
的可用物理内存。

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI000810.png)

###### PERCPU 变量定义

{% highlight bash %}
DEFINE_PER_CPU()
__percpu
{% endhighlight %}

###### PERCPU 分配

{% highlight bash %}
alloc_percpu()
__alloc_percpu()
{% endhighlight %}

###### PERCPU 释放

{% highlight bash %}
free_percpu()
{% endhighlight %}

###### PERCPU 读取

{% highlight bash %}
for_each_possible_cpu()
per_cpu_ptr()
__get_cpu_var()
per_cpu
get_cpu_var()
put_cpu_var()
{% endhighlight %}

具体函数解析说明，请查看:

> - [PERCPU API](#K)

###### 与项目相关

PERCPU 内存分配器与本项目相关的调用顺序如下:

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI000819.png)

###### 补丁

相对上一个版本 linux 2.6.12.2，PERCPU 内存分配器并未做改动。这个版本
的代码均不产生补丁。更多补丁的使用请参考:

> - [BiscuitOS Memory Manager Patch 建议](https://biscuitos.github.io/blog/HISTORY-MMU/#C00033)

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND000100.png)

----------------------------------------------

<span id="H-linux-2.6.12.4"></span>

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI000789.JPG)

#### PERCPU Linux 2.6.12.4

Linux 2.6.12.4 依旧采用 PERCPU 作为其早期的内存管理器。采用 bitmap 管理所有
的可用物理内存。

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI000810.png)

###### PERCPU 变量定义

{% highlight bash %}
DEFINE_PER_CPU()
__percpu
{% endhighlight %}

###### PERCPU 分配

{% highlight bash %}
alloc_percpu()
__alloc_percpu()
{% endhighlight %}

###### PERCPU 释放

{% highlight bash %}
free_percpu()
{% endhighlight %}

###### PERCPU 读取

{% highlight bash %}
for_each_possible_cpu()
per_cpu_ptr()
__get_cpu_var()
per_cpu
get_cpu_var()
put_cpu_var()
{% endhighlight %}

具体函数解析说明，请查看:

> - [PERCPU API](#K)

###### 与项目相关

PERCPU 内存分配器与本项目相关的调用顺序如下:

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI000819.png)

###### 补丁

相对上一个版本 linux 2.6.12.3，PERCPU 内存分配器并未做改动。这个版本
的代码均不产生补丁。更多补丁的使用请参考:

> - [BiscuitOS Memory Manager Patch 建议](https://biscuitos.github.io/blog/HISTORY-MMU/#C00033)

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND000100.png)

----------------------------------------------

<span id="H-linux-2.6.12.5"></span>

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI000790.JPG)

#### PERCPU Linux 2.6.12.5

Linux 2.6.12.5 依旧采用 PERCPU 作为其早期的内存管理器。采用 bitmap 管理所有
的可用物理内存。

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI000810.png)

###### PERCPU 变量定义

{% highlight bash %}
DEFINE_PER_CPU()
__percpu
{% endhighlight %}

###### PERCPU 分配

{% highlight bash %}
alloc_percpu()
__alloc_percpu()
{% endhighlight %}

###### PERCPU 释放

{% highlight bash %}
free_percpu()
{% endhighlight %}

###### PERCPU 读取

{% highlight bash %}
for_each_possible_cpu()
per_cpu_ptr()
__get_cpu_var()
per_cpu
get_cpu_var()
put_cpu_var()
{% endhighlight %}

具体函数解析说明，请查看:

> - [PERCPU API](#K)

###### 与项目相关

PERCPU 内存分配器与本项目相关的调用顺序如下:

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI000819.png)

###### 补丁

相对上一个版本 linux 2.6.12.4，PERCPU 内存分配器并未做改动。这个版本
的代码均不产生补丁。更多补丁的使用请参考:

> - [BiscuitOS Memory Manager Patch 建议](https://biscuitos.github.io/blog/HISTORY-MMU/#C00033)

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND000100.png)

----------------------------------------------

<span id="H-linux-2.6.12.6"></span>

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI000791.JPG)

#### PERCPU Linux 2.6.12.6

Linux 2.6.12.6 依旧采用 PERCPU 作为其早期的内存管理器。采用 bitmap 管理所有
的可用物理内存。

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI000810.png)

###### PERCPU 变量定义

{% highlight bash %}
DEFINE_PER_CPU()
__percpu
{% endhighlight %}

###### PERCPU 分配

{% highlight bash %}
alloc_percpu()
__alloc_percpu()
{% endhighlight %}

###### PERCPU 释放

{% highlight bash %}
free_percpu()
{% endhighlight %}

###### PERCPU 读取

{% highlight bash %}
for_each_possible_cpu()
per_cpu_ptr()
__get_cpu_var()
per_cpu
get_cpu_var()
put_cpu_var()
{% endhighlight %}

具体函数解析说明，请查看:

> - [PERCPU API](#K)

###### 与项目相关

PERCPU 内存分配器与本项目相关的调用顺序如下:

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI000819.png)

###### 补丁

相对上一个版本 linux 2.6.12.5，PERCPU 内存分配器并未做改动。这个版本
的代码均不产生补丁。更多补丁的使用请参考:

> - [BiscuitOS Memory Manager Patch 建议](https://biscuitos.github.io/blog/HISTORY-MMU/#C00033)

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND000100.png)

----------------------------------------------

<span id="H-linux-2.6.13"></span>

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI000792.JPG)

#### PERCPU Linux 2.6.13

Linux 2.6.13 依旧采用 PERCPU 作为其早期的内存管理器。采用 bitmap 管理所有
的可用物理内存。

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI000810.png)

###### PERCPU 变量定义

{% highlight bash %}
DEFINE_PER_CPU()
__percpu
{% endhighlight %}

###### PERCPU 分配

{% highlight bash %}
alloc_percpu()
__alloc_percpu()
{% endhighlight %}

###### PERCPU 释放

{% highlight bash %}
free_percpu()
{% endhighlight %}

###### PERCPU 读取

{% highlight bash %}
for_each_possible_cpu()
per_cpu_ptr()
__get_cpu_var()
per_cpu
get_cpu_var()
put_cpu_var()
{% endhighlight %}

具体函数解析说明，请查看:

> - [PERCPU API](#K)

###### 与项目相关

PERCPU 内存分配器与本项目相关的调用顺序如下:

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI000819.png)

###### 补丁

相对上一个版本 linux 2.6.12.6，PERCPU 内存分配器提交了四个补丁。如下:

{% highlight bash %}
tig include/linux/percpu.h init/main.c

2005-07-28 21:15 Andi Kleen        o [PATCH] x86_64: Some cleanup in setup64.c
                                     [main] a940199f206dcf51c65fae27e2ce412f2c5a2b22 - commit 1 of 4
{% endhighlight %}

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI000820.png)

{% highlight bash %}
git format-patch -1 a940199f206dcf51c65fae27e2ce412f2c5a
vi 0001-PATCH-sparsemem-base-simple-NUMA-remap-space-allocat.patch
{% endhighlight %}

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI000821.png)

该补丁将 PERCPU 静态变量所在的 .data..percpu section 的起始地址 
"\_\_per_cpu_start" 和终止地址 "\_\_per_cpu_end" 从 "init/main.c" 中移除并
在 "sections.h" 中统一导出声明. 更多补丁的使用请参考:

> - [BiscuitOS Memory Manager Patch 建议](https://biscuitos.github.io/blog/HISTORY-MMU/#C00033)

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND000100.png)

----------------------------------------------

<span id="H-linux-2.6.13.1"></span>

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI000793.JPG)

#### PERCPU Linux 2.6.13.1

Linux 2.6.13.1 依旧采用 PERCPU 作为其早期的内存管理器。采用 bitmap 管理所有
的可用物理内存。

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI000810.png)

###### PERCPU 变量定义

{% highlight bash %}
DEFINE_PER_CPU()
__percpu
{% endhighlight %}

###### PERCPU 分配

{% highlight bash %}
alloc_percpu()
__alloc_percpu()
{% endhighlight %}

###### PERCPU 释放

{% highlight bash %}
free_percpu()
{% endhighlight %}

###### PERCPU 读取

{% highlight bash %}
for_each_possible_cpu()
per_cpu_ptr()
__get_cpu_var()
per_cpu
get_cpu_var()
put_cpu_var()
{% endhighlight %}

具体函数解析说明，请查看:

> - [PERCPU API](#K)

###### 与项目相关

PERCPU 内存分配器与本项目相关的调用顺序如下:

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI000819.png)

###### 补丁

相对上一个版本 linux 2.6.13，PERCPU 内存分配器并未做改动。这个版本
的代码均不产生补丁。更多补丁的使用请参考:

> - [BiscuitOS Memory Manager Patch 建议](https://biscuitos.github.io/blog/HISTORY-MMU/#C00033)

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND000100.png)

----------------------------------------------

<span id="H-linux-2.6.14"></span>

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI000794.JPG)

#### PERCPU Linux 2.6.14

Linux 2.6.14 依旧采用 PERCPU 作为其早期的内存管理器。采用 bitmap 管理所有
的可用物理内存。

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI000810.png)

###### PERCPU 变量定义

{% highlight bash %}
DEFINE_PER_CPU()
__percpu
{% endhighlight %}

###### PERCPU 分配

{% highlight bash %}
alloc_percpu()
__alloc_percpu()
{% endhighlight %}

###### PERCPU 释放

{% highlight bash %}
free_percpu()
{% endhighlight %}

###### PERCPU 读取

{% highlight bash %}
for_each_possible_cpu()
per_cpu_ptr()
__get_cpu_var()
per_cpu
get_cpu_var()
put_cpu_var()
{% endhighlight %}

具体函数解析说明，请查看:

> - [PERCPU API](#K)

###### 与项目相关

PERCPU 内存分配器与本项目相关的调用顺序如下:

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI000819.png)

###### 补丁

相对上一个版本 linux 2.6.13.1，PERCPU 内存分配器增加了一个补丁，如下:

{% highlight bash %}
tig mm/slab.c

2005-05-01 08:58 Manfred Spraul           o [PATCH] add kmalloc_node, inline cleanup
                                            [main] 97e2bde47f886a317909c8a8f9bd2fcd8ce2f0b0 - commit 22 of 23 
{% endhighlight %}

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI000823.png)

{% highlight bash %}
git format-patch -1 97e2bde47f886a317909c8a8f9bd2fcd8ce2f0b0
vi 0001-PATCH-add-kmalloc_node-inline-cleanup.patch
{% endhighlight %}

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI000822.png)

该补丁将 PERCPU 内存分配器每个 CPU 副本分配函数从 kmem_cache_alloc_node()
替换成了 kmalloc_node() 函数，从补丁描述来看，替换成新的分配函数之后，性能
有了提高. 更多补丁的使用请参考:

> - [BiscuitOS Memory Manager Patch 建议](https://biscuitos.github.io/blog/HISTORY-MMU/#C00033)

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND000100.png)

----------------------------------------------

<span id="H-linux-2.6.15"></span>

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI000795.JPG)

#### PERCPU Linux 2.6.15

Linux 2.6.15 依旧采用 PERCPU 作为其早期的内存管理器。采用 bitmap 管理所有
的可用物理内存。

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI000810.png)

###### PERCPU 变量定义

{% highlight bash %}
DEFINE_PER_CPU()
__percpu
{% endhighlight %}

###### PERCPU 分配

{% highlight bash %}
alloc_percpu()
__alloc_percpu()
{% endhighlight %}

###### PERCPU 释放

{% highlight bash %}
free_percpu()
{% endhighlight %}

###### PERCPU 读取

{% highlight bash %}
for_each_possible_cpu()
per_cpu_ptr()
__get_cpu_var()
per_cpu
get_cpu_var()
put_cpu_var()
{% endhighlight %}

具体函数解析说明，请查看:

> - [PERCPU API](#K)

###### 与项目相关

PERCPU 内存分配器与本项目相关的调用顺序如下:

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI000819.png)

###### 补丁

相对上一个版本 linux 2.6.14，PERCPU 内存分配器增加了一个补丁，如下:

{% highlight bash %}
tig include/linux/percpu.h

2005-11-13 16:07 Paul Mundt     o [PATCH] Shut up per_cpu_ptr() on UP
                                  [main] 66341a905ef5b3e7aea65b5d9bd1b0361b0ccc61 - commit 1 of 2
{% endhighlight %}

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI000824.png)

{% highlight bash %}
git format-patch -1 66341a905ef5b3e7aea65b5d9bd1b0361b0ccc61
vi 0001-PATCH-Shut-up-per_cpu_ptr-on-UP.patch
{% endhighlight %}

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI000825.png)

该补丁对 UP 架构的 PERCPU 变量访问时，由原先的对变量直接访问，增加了
对 cpu 的访问。更多补丁的使用请参考:

> - [BiscuitOS Memory Manager Patch 建议](https://biscuitos.github.io/blog/HISTORY-MMU/#C00033)

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND000100.png)

-----------------------------------------------

<span id="G"></span>

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND00000H.jpg)

#### PERCPU 历史时间轴

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI000826.png)

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND000100.png)

-----------------------------------------------

<span id="K"></span>

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND00000B.jpg)

#### PERCPU API

###### \_\_alloc_percpu

{% highlight bash %}
void *__alloc_percpu(size_t size, size_t align)
  作用: PERCPU 分配器动态分配一个 PERCPU 变量.
{% endhighlight %}

###### alloc_percpu

{% highlight bash %}
#define alloc_percpu(type) \
        ((type *)(__alloc_percpu(sizeof(type), __alignof__(type))))
  作用: PERCPU 分配器动态分配一个 PERCPU 变量.
{% endhighlight %}

###### DEFINE_PER_CPU

{% highlight bash %}
#define DEFINE_PER_CPU(type, name) \
    __attribute__((__section__(".data.percpu"))) __typeof__(type) per_cpu__##name
  作用: 定义一个静态 PERCPU 变量.
{% endhighlight %}

###### free_percpu

{% highlight bash %}
void free_percpu(const void *objp)
  作用: PERCPU 分配器释放一个 PERCPU 变量. 
{% endhighlight %}

###### get_cpu_var

{% highlight bash %}
#define get_cpu_var(var) (*({ preempt_disable(); &__get_cpu_var(var); }))
  作用: 读取 PERCPU 变量的值 (禁止抢占方式). 与 put_cpu_var() 成对使用.
{% endhighlight %}                 

###### \_\_per_cpu_end

{% highlight bash %}
extern char __per_cpu_end[]
  作用: .data.percpu section 的终止地址.
{% endhighlight %}

###### \_\_per_cpu_offset

{% highlight bash %}
unsigned long __per_cpu_offset[NR_CPUS];
  作用: .data.percpu section 与 PERCPU 分配管理的内存之间的偏移.
{% endhighlight %}

###### \_\_per_cpu_start

{% highlight bash %}
extern char __per_cpu_start[];
  作用: .data.percpu section 的起始地址.
{% endhighlight %}

###### per_cpu

{% highlight bash %}
#define per_cpu(var, cpu) (*RELOC_HIDE(&per_cpu__##var, __per_cpu_offset[cpu]))
  作用: 读取特定 CPU 的 PERCPU 副本.
{% endhighlight %}

###### per_cpu_ptr

{% highlight bash %}
#define per_cpu_ptr(ptr, cpu)                   \
({                                              \
        struct percpu_data *__p = (struct percpu_data *)~(unsigned long)(ptr); \
        (__typeof__(ptr))__p->ptrs[(cpu)];      \
})
  作用: 读取特定 CPU 的 PERCPU 变量副本.
{% endhighlight %}

###### put_cpu_var

{% highlight bash %}
#define put_cpu_var(var) preempt_enable()
  作用: 接触 PERCPU 变量的占用. 与 get_cpu_var() 成对使用.
{% endhighlight %}

###### setup_per_cpu_areas

{% highlight bash %}
static void __init setup_per_cpu_areas(void)
  作用: 初始化 PERCPU 管理器相关的静态 PERCPU 变量内存区域.
{% endhighlight %}

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND000100.png)

----------------------------------

<span id="F"></span>

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND00000K.jpg)

#### PERCPU 进阶研究

> - [PERCPU(UP) Memory Allocator On Userspace](https://biscuitos.github.io/blog/Memory-Userspace/#C)
>
> - [PERCPU(SMP) Memory Allocator On Userspace](https://biscuitos.github.io/blog/Memory-Userspace/#D)

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND000100.png)

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

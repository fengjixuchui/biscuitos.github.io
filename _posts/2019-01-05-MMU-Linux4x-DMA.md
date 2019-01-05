---
layout: post
title:  "Linux 4.x 内核空间 DMA 虚拟内存地址"
date:   2019-01-05 14:32:30 +0800
categories: [MMU]
excerpt: Linux 4.x Kernel DMA Virtual Space.
tags:
  - MMU
---

> Architecture: i386 32bit Machine Ubuntu 16.04
>
> Linux version: 4.15.0-39-generic
>
> Email: BuddyZhang1 <buddy.zhang@aliyun.com>

# 目录

> - [DMA 虚拟内存区](#DMA 虚拟内存区)
>
> - [DMA 虚拟内存区中分配内存](#分配内存)
>
> - [DMA 虚拟内存实践](#DMA 虚拟内存实践)
>
> - [总结](#总结)
>
> - [附录](#附录)

--------------------------------------------------

# <span id="DMA 虚拟内存区">DMA 虚拟内存区</span>

在 IA32 体系结构中，由于 CPU 的地址总线只有 32 位，在不开启 PAE 的情况下，CPU 
可以访问 4G 的线性地址空间。Linux 采用了 3:1 的策略，即内核占用 1G 的线性地址
空间，用户占用 3G 的线性地址空间。如果 Linux 物理内存小于 1G 的空间，通常将线
性地址空间和物理空间一一映射，这样可以提供访问速度。但是，当 Linux 物理内存超
过 1G，内核的线性地址就不够用，所以，为了解决这个问题，Linux 把内核的虚拟地址
空间分作线性区和非线性区两个部分，线性区规定最大为 896M，剩下的 128M 为非线性
区。从而，线性区映射的物理内存都是低端内存，剩下的物理内存作为高端内存。

在低端内存被分作两部分：

> - ZONE_DMA
>
> - ZONE_NORMAL

对于整个 1G 内核虚拟地址空间，通常把空间开头的 16MB 称为 DMA 区，即 ZONE_DMA 
区，因为以往硬件将 DMA 空间固定在了物理内存的低 16MB 空间，这段物理内存一一映
射到内核开头的虚拟内存区，所以这段虚拟内存区域就称为 DMA 虚拟内存区。

**特别值得注意的是**在低端虚拟内存中，Linux 内核将虚拟地址与物理地址采用一一对
应的线性方式进行固定映射，但这种映射只是内核虚拟地址和物理页框的一种“预定”，并
不是“霸占”或“独占”了这些物理页，只有低端内核虚拟地址真正使用了这些物理页框时，
内核低端虚拟地址才和物理页框一一对应的线性映射。而在平时，这个页框没被低端虚拟
地址使用的时候，该页框完全可以被用户空间以及 kmalloc 分配使用。

------------------------------------------------------

# <span id="分配内存">DMA 虚拟内存区中分配内存</span>

IA32 体系结构中，Linux 4.x 可以使用 kmalloc() 函数和 GFP_DMA 标志从 DMA 虚拟内
存中获得虚拟内存。函数使用方法如下：

{% highlight ruby %}
#ifdef CONFIG_DEBUG_VA_KERNEL_DMA
    /*
     * DMA Virtual Space
     * 0    3G                                                  4G
     * +----+---------------------------------+-----------------+
     * |    |                                 |                 |
     * |    |        DMA Virtual Space        |                 |
     * |    |                                 |                 |
     * +----+---------------------------------+-----------------+
     *      A                                 A
     *      |                                 |
     *      |                                 |
     *      |                                 |
     *      o                                 o
     *  PAGE_OFFSET                     MAX_DMA_ADDRESS
     */
    unsigned int *DMA_int = NULL;

    DMA_int = (unsigned int *)kmalloc(sizeof(unsigned int), GFP_DMA);
    printk("[*]unsigned int *DMA_int:        Address: %#08x\n",
                              (unsigned int)(unsigned long)DMA_int);
    if (DMA_int)
        kfree(DMA_int);
#endif
{% endhighlight %}

kmalloc() 用于分配一段可用的虚拟内存，标志 GFP_DMA 指定这块虚拟内存从 DMA 虚拟
区获得。从上面的代码可以看出，DMA 虚拟内存的范围由 PAGE_OFFSET 开始，到 
MAX_DMA_ADDRESS 结束。

--------------------------------------------------

# <span id= "DMA 虚拟内存实践">DMA 虚拟内存实践</span>

BiscuitOS 提供了相关的实例代码，开发者可以使用如下命令：
首先，开发者先准备 BiscuitOS 系统，内核版本 linux 1.0.1.2。开发可以参照文档构
建 BiscuitOS 调试环境：

[Linux 1.0.1.2 内核构建方法](https://biscuitos.github.io/blog/Linux1.0.1.2_ext2fs_Usermanual/)

由于代码需要运行在 IA32 系统上，如果开发者的系统不是 IA32，那么可以按照下面的
教程搭建一个 IA32 虚拟机：

[Ubuntu IA32 虚拟环境build](https://biscuitos.github.io/blog/Linux1.0.1.2_ext2fs_Usermanual/)

接着，开发者配置内核，使用如下命令：

{% highlight ruby %}
cd BiscuitOS
make clean
make update
make linux_1_0_1_2_ext2_defconfig
make
cd BiscuitOS/kernel/linux_1.0.1.2/
make clean
make menuconfig
{% endhighlight %}

由于 BiscuitOS 的内核使用 Kbuild 构建起来的，在执行完 make menuconfig 之后，系
统会弹出内核配置的界面，开发者根据如下步骤进行配置：

![MMU](https://raw.githubusercontent.com/EmulateSpace/PictureSet/master/BiscuitOS/kernel/MMU000003.png)

选择 kernel hacking，回车

![MMU](https://raw.githubusercontent.com/EmulateSpace/PictureSet/master/BiscuitOS/kernel/MMU000004.png)

选择 Demo Code for variable subsystem mechanism, 回车

![MMU](https://raw.githubusercontent.com/EmulateSpace/PictureSet/master/BiscuitOS/kernel/MMU000005.png)

选择 MMU(Memory Manager Unit) on X86 Architecture, 回车

![MMU](https://raw.githubusercontent.com/EmulateSpace/PictureSet/master/BiscuitOS/kernel/MMU000433.png)

选择 Debug MMU(Memory Manager Unit) mechanism on X86 Architecture 之后选择 
Addressing Mechanism  回车

![MMU](https://raw.githubusercontent.com/EmulateSpace/PictureSet/master/BiscuitOS/kernel/MMU000434.png)

选择 Virtual address, 回车

![MMU](https://raw.githubusercontent.com/EmulateSpace/PictureSet/master/BiscuitOS/kernel/MMU000525.png)

选择 Virtual address and Virtual space 之后，接着选择 Choice Kernel/User 
Virtual Address Space, 回车。

![MMU](https://raw.githubusercontent.com/EmulateSpace/PictureSet/master/BiscuitOS/kernel/MMU000526.png)

该选项用于选择程序运行在用户空间还是内核空间，这里选择内核空间。选择 Kernel 
Virtual Address Space. 回车之后按 Esc 退出。

![MMU](https://raw.githubusercontent.com/EmulateSpace/PictureSet/master/BiscuitOS/kernel/MMU000527.png)

最后开发者选择 .data segment,下拉菜单打开后，选择 DMA Virtual Space (kmalloc: 
GFP_DMA) 选项，回车保存并退出。

运行实例代码，使用如下代码：

{% highlight ruby %}
cd BiscuitOS/kernel/linux_1.0.1.2/
make 
cd tools/demo/mmu/addressing/virtual_address/data/kernel/
{% endhighlight %}

如果开发者的主机本身即是 IA32，那么使用如下命令安装并运行模块程序

{% highlight ruby %}
sudo insmod kern_data.ko
dmesg | tail -n 20
{% endhighlight %}

如果开发者的主机不是 IA32，那么使用如下命令运行并安装模块

{% highlight ruby %}
cp kern_data.c /虚拟机指定目录
cp .tmp/Makefile /虚拟机指定目录
{% endhighlight %}

开发者可以通过多种方式将上面两个文件拷贝到虚拟机指定目录下，然后在虚拟机上执行
如下命令

{% highlight ruby %}
make
make install
{% endhighlight %}

源码如下：

![MMU](https://raw.githubusercontent.com/EmulateSpace/PictureSet/master/BiscuitOS/kernel/MMU000529.png)

Makefile

{% highlight ruby %}
# Module on higher linux version
obj-m += kern_data.o

kern_dataDIR ?= /lib/modules/$(shell uname -r)/build

PWD       := $(shell pwd)

ROOT := $(dir $(M))
DEMOINCLUDE := -I$(ROOT)../include -I$(ROOT)/include

all:
        $(MAKE) -C $(kern_dataDIR) M=$(PWD) modules

install:
        @sudo insmod kern_data.ko
        @dmesg | tail -n 20
        @sudo rmmod kern_data

clean:
        @rm -rf *.o *.o.d *~ core .depend .*.cmd *.ko *.ko.unsigned *.mod.c .tmp_ \
    .cache.mk *.save *.bak Modules.* modules.order Module.* *.b

CFLAGS_kern_data.o := -Wall $(DEMOINCLUDE)
CFLAGS_kern_data.o += -DCONFIG_DEBUG_VA_KERNEL_DMA -fno-common
{% endhighlight %}

运行结果如图：

![MMU](https://raw.githubusercontent.com/EmulateSpace/PictureSet/master/BiscuitOS/kernel/MMU000528.png)

从上面数据可知，kmalloc() 函数使用 GFP_DMA 标志之后，所分配的内存位于 
0xC0000000 到 0xC1000000 之间，所以分配成功。

---------------------------------------------

# <span id="总结">总结</span>

DMA 虚拟内存区域位于 Linux 内核虚拟区域开始的位置，其与物理内存开始的区域一
一对应，并进行一一映射，这里的映射就是虚拟地址到物理地址的转换是一个线性公式，
也就是虚拟地址做一个简单的线性计算就可以获得物理地址，这样大大加速的地址转换
的效率。DMA 内存区域有特定的作用，开发者应该根据体系相关的文档进行使用。

通过上面的实践可知，当开发者需要使用到 DMA 虚拟地址，可以使用 kmalloc() 函数和
GFP_DMA 标志进行分配使用，使用之后，可以使用 kfree() 进行释放。 

-----------------------------------------------

# <span id="附录">附录</span>

[Linux 的内核空间（低端内存，高端内存）](https://blog.csdn.net/qq_38410730/article/details/81105132)

赞赏一下吧 🙂

![MMU](https://raw.githubusercontent.com/EmulateSpace/PictureSet/master/BiscuitOS/kernel/HAB000036.jpg)

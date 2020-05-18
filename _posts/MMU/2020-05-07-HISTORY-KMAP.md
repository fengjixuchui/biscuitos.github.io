---
layout: post
title:  "KMAP Allocator"
date:   2020-05-07 09:39:30 +0800
categories: [HW]
excerpt: MMU.
tags:
  - MMU
---

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND00000L0.PNG)

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI100100.png)

#### 目录

> - [KMAP 分配器原理](#A)
>
> - [KMAP 分配器使用](#B)
>
> - [KMAP 分配器实践](#C)
>
> - [KMAP 源码分析](#D)
>
> - [KMAP 分配器调试](#E)
>
> - [KMAP 分配进阶研究](#F)
>
> - [KMAP 时间轴](#G)
>
> - [KMAP 历史补丁](#H)
>
> - [KMAP API](#K)
>
> - [附录/捐赠](#Z0)

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND000100.png)

----------------------------------

<span id="A"></span>

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND00000T.jpg)

#### KMAP 分配器原理

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/HK000226.png)

KMAP 内存管理器用于分配和释放零时映射虚拟内存。Linux 从内核虚拟空间划分
一段大小为 2MB 的虚拟内存，起始地址 PKMAP_BASE。在有的体系中，物理内存
远远大于内核的虚拟地址空间，Linux 需要零时将指定的物理内存映射到虚拟地址
上，以便完成指定的任务，当任务完成之后就解除物理内存和虚拟内存之间的关系。
那么 VMALLOC 也可以实现这样的功能，当为什么不使用 VMALLOC 分配器分配呢?
可能的原因有很多，其中之一是 VMALLOC 分配器分配的虚拟地址和物理地址之间
映射时间一般很长，毕竟短时间的映射页表开销也不小; 另外一个原因可能是
VMALLOC 分配器分配的虚拟地址是连续了，为了最大限度保持虚拟内存的连续，
所以不建议使用 VMALLOC 做短时间的零时映射. Linux 内核于是推出了 KMAP 内存
管理器，用于短时间的虚拟内存到物理内存映射，满足一些任务的需要.

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI000969.png)

KMAP 内存管理器实现很简练. PKMAP 分配器拥有固定长度的虚拟内存，然后将
固定长度的虚拟内存划分为 LAST_PKMAP 个入口，每个入口对应一个虚拟地址。
KMAP 分配器将其制作成一个大数组 pkmap_count[LAST_PKMAP], 总共包含 LAST_PKMAP
个入口，数组中的成员用于标记对于虚拟内存的使用情况. 每个入口的虚拟地址
转换关系如下:

{% highlight bash %}
#define PKMAP_ADDR(nr)  (PKMAP_BASE + ((nr) << PAGE_SHIFT))
{% endhighlight %}

因此，KMAP 只需一个 nr 号就可以获得一个唯一的虚拟地址。当 KMAP 获得虚拟
地址之后就可以建立页表将虚拟地址与物理地址进行映射。这里值得注意的是 KMAP
分配器只负责提供虚拟地址和建立虚拟地址与物理地址的映射，对于物理内存的获得，
需要请求者从 Buddy/PCP 内存分配器中获得，一般从 ZONE_HIGHMEM 中获得物理
内存.

---------------------------------

###### KMAP 的优点

对于短暂的零时映射有用.

###### KMAP 的缺点

由于要动态建立页表, 系统开销较大.

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND000100.png)

---------------------------------

<span id="B"></span>

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND00000H.jpg)

#### KMAP 分配器使用

> - [基础用法介绍](#B0000)
>
> - [KMAP 分配虚拟内存](#B0001)

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND000100.png)

-----------------------------------------------

#### <span id="B0000">基础用法介绍</span>

KMAP 分配器提供了用于分配和释放虚拟内存的相关函数接口:

###### KMAP 分配

{% highlight bash %}
void fastcall *kmap_high(struct page *page)
kmap_atomic
kmap_atomic_pfn
kmap_atomic_to_page
{% endhighlight %}

###### KMAP 释放

{% highlight bash %}
void fastcall kunmap_high(struct page *page)
kunmap_atomic
{% endhighlight %}

具体函数解析说明，请查看:

> - [KMAP API](#K)

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND000100.png)

----------------------------------------------------

#### <span id="B0001">KMAP 分配虚拟内存</span>

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI000969.png)

从 KMAP 区域内存分配、使用和释放虚拟内存，开发者可以参考如下代码:

{% highlight c %}
#include <linux/mm.h>
#include <linux/highmem.h>

static int TestCase_kmap(void)
{
        struct page *page;
        void *addr;

        /* alloc page */
        page = alloc_page(__GFP_HIGHMEM);
        if (!page || !PageHighMem(page)) {
                printk("%s alloc_page() failed.\n", __func__);
                return -ENOMEM;
        }

        /* kmap */
        addr = kmap(page);
        if (!addr) {
                printk("%s kmap() failed.\n", __func__);
                __free_page(page);
                return -EINVAL;
        }

        sprintf((char *)addr, "BiscuitOS-%s", __func__);
        printk("[%#lx] %s\n", (unsigned long)addr, (char *)addr);

        /* kunmap */
        kunmap(page);
	__free_page(page);
        return 0;
}
{% endhighlight %}

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND000100.png)

------------------------------------------------

<span id="C"></span>

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND00000I.jpg)

#### KMAP 分配器实践

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
支持多个版本的 KMAP，开发者可以根据需求进行选择，本文以 linux 2.6.12 
版本的 KMAP 进行讲解。开发者使用如下命令:

{% highlight bash %}
cd BiscuitOS/
make linux-5.0-arm32_defconfig
make menuconfig
{% endhighlight %}

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI000746.png)

选择并进入 "[\*] Package  --->" 目录。

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI000747.png)

选择并进入 "[\*]   Memory Development History  --->" 目录。

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI000970.png)

选择并进入 "[\*]   KMAP Allocator  --->" 目录。

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI000971.png)

选择 "[\*]   KMAP on linux 2.6.12  --->" 目录，保存并退出。接着执行如下命令:

{% highlight bash %}
make
{% endhighlight %}

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI000750.png)

成功之后将出现上图的内容，接下来开发者执行如下命令以便切换到项目的路径:

{% highlight bash %}
cd BiscuitOS/output/linux-5.0-arm32/package/BiscuitOS_KMAP-2.6.12
make download
{% endhighlight %}

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI000972.png)

至此源码已经下载完成，开发者可以使用 tree 等工具查看源码:

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI000973.png)

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
cd BiscuitOS/output/linux-5.0-arm32/package/BiscuitOS_KMAP-2.6.12
make kernel
{% endhighlight %}

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI000974.png)

--------------------------------------------

#### <span id="C0002">实践执行</span>

环境部署完毕之后，开发者可以向通用模块一样对源码进行编译和安装使用，使用
如下命令:

{% highlight bash %}
cd BiscuitOS/output/linux-5.0-arm32/package/BiscuitOS_KMAP-2.6.12
make
{% endhighlight %}

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI000975.png)

以上就是模块成功编译，接下来将 ko 模块安装到 BiscuitOS 中，使用如下命令:

{% highlight bash %}
cd BiscuitOS/output/linux-5.0-arm32/package/BiscuitOS_KMAP-2.6.12
make install
make pack
{% endhighlight %}

以上准备完毕之后，最后就是在 BiscuitOS 运行这个模块了，使用如下命令:

{% highlight bash %}
cd BiscuitOS/output/linux-5.0-arm32/package/BiscuitOS_KMAP-2.6.12
make run
{% endhighlight %}

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI000976.png)

在 BiscuitOS 中插入了模块 "BiscuitOS_KMAP-2.6.12.ko"，打印如上信息，那么
BiscuitOS Memory Manager Unit History 项目的内存管理子系统已经可以使用。

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
/xspace/OpenSource/BiscuitOS/BiscuitOS/output/linux-5.0-arm32/package/BiscuitOS_KMAP-2.6.12/BiscuitOS_KMAP-2.6.12/Makefile
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

然后先向 BiscuitOS 中插入 "BiscuitOS_KMAP-2.6.12.ko" 模块，然后再插入
"BiscuitOS_KMAP-2.6.12-buddy.ko" 模块。如下:

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI000773.png)

以上便是测试代码的使用办法。开发者如果想在源码中启用或关闭某些宏，可以
修改 Makefile 中内容:

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI000774.png)

从上图可以知道，如果要启用某些宏，可以在 ccflags-y 中添加 "-D" 选项进行
启用，源码的编译参数也可以添加到 ccflags-y 中去。开发者除了使用上面的办法
进行测试之外，也可以使用项目提供的 initcall 机制进行调试，具体请参考:

> - [Initcall 机制调试说明](#C00032)

Initcall 机制提供了以下函数用于 KMAP 调试:

{% highlight bash %}
kmap_initcall_bs()
{% endhighlight %}

从项目的 Initcall 机制可以知道，kmap_initcall_bs() 调用的函数将
在 KMAP 分配器初始化完毕之后自动调用。KMAP 相关的测试代码位于:

{% highlight bash %}
BiscuitOS/output/linux-5.0-arm32/package/BiscuitOS_KMAP-2.6.12/BiscuitOS_KMAP-2.6.12/module/kmap/
{% endhighlight %}

在 Makefile 中打开调试开关:

{% highlight bash %}
$(MODULE_NAME)-m                += modules/kmap/main.o
{% endhighlight %}

KMAP 测试代码也包含模块测试，在 Makefile 中打开调试开关:

{% highlight bash %}
obj-m                           += $(MODULE_NAME)-kmap.o
$(MODULE_NAME)-kmap-m           := modules/kmap/module.o
{% endhighlight %}

KMAP 模块测试结果如下:

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI000977.png)

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND000100.png)

----------------------------------

<span id="H"></span>

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND00000T.jpg)

#### KMAP 历史补丁

> - [KMAP Linux 2.6.12](#H-linux-2.6.12)
>
> - [KMAP Linux 2.6.12.1](#H-linux-2.6.12.1)
>
> - [KMAP Linux 2.6.12.2](#H-linux-2.6.12.2)
>
> - [KMAP Linux 2.6.12.3](#H-linux-2.6.12.3)
>
> - [KMAP Linux 2.6.12.4](#H-linux-2.6.12.4)
>
> - [KMAP Linux 2.6.12.5](#H-linux-2.6.12.5)
>
> - [KMAP Linux 2.6.12.6](#H-linux-2.6.12.6)
>
> - [KMAP Linux 2.6.13](#H-linux-2.6.13)
>
> - [KMAP Linux 2.6.13.1](#H-linux-2.6.13.1)
>
> - [KMAP Linux 2.6.14](#H-linux-2.6.14)
>
> - [KMAP Linux 2.6.15](#H-linux-2.6.15)

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND000100.png)

----------------------------------------------

<span id="H-linux-2.6.12"></span>

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI000785.JPG)

#### KMAP Linux 2.6.12

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI000969.png)

Linux 2.6.12 采用 KMAP 分配器管理 KMAP 虚拟内存区域。

###### KMAP 分配

{% highlight bash %}
void fastcall *kmap_high(struct page *page)
kmap_atomic
kmap_atomic_pfn
kmap_atomic_to_page
{% endhighlight %}

###### KMAP 释放

{% highlight bash %}
void fastcall kunmap_high(struct page *page)
kunmap_atomic
{% endhighlight %}

具体函数解析说明，请查看:

> - [KMAP API](#K)

###### 与项目相关

KMAP 内存分配器与本项目相关的 kmap_high() 调用顺序如下:

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI000979.png)

KMAP 内存分配器与本项目相关的 kunmap_high() 调用顺序如下:

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI000980.png)

项目中虚拟内存布局如下:

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI000737.png)

在项目中，KMAP 虚拟内存的管理的范围是: 0x96000000 到 0x96200000. 

###### 补丁

对于 Linux 2.6.12 的补丁，Linus 将 Linux 内核源码树加入到 git 中来，因此
这个版本的代码均不产生补丁。更多补丁的使用请参考:

> - [BiscuitOS Memory Manager Patch 建议](https://biscuitos.github.io/blog/HISTORY-MMU/#C00033)

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND000100.png)

----------------------------------------------

<span id="H-linux-2.6.12.1"></span>

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI000786.JPG)

#### KMAP Linux 2.6.12.1

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI000969.png)

Linux 2.6.12.1 采用 KMAP 分配器管理 KMAP 虚拟内存区域。

###### KMAP 分配

{% highlight bash %}
void fastcall *kmap_high(struct page *page)
kmap_atomic
kmap_atomic_pfn
kmap_atomic_to_page
{% endhighlight %}

###### KMAP 释放

{% highlight bash %}
void fastcall kunmap_high(struct page *page)
kunmap_atomic
{% endhighlight %}

具体函数解析说明，请查看:

> - [KMAP API](#K)

###### 与项目相关

KMAP 内存分配器与本项目相关的 kmap_high() 调用顺序如下:

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI000979.png)

KMAP 内存分配器与本项目相关的 kunmap_high() 调用顺序如下:

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI000980.png)

项目中虚拟内存布局如下:

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI000737.png)

在项目中，KMAP 虚拟内存的管理的范围是: 0x96000000 到 0x96200000. 

###### 补丁

相对于前一个版本 linux 2.6.12, 该版本并未产生补丁。更多补丁的使用请参考:

> - [BiscuitOS Memory Manager Patch 建议](https://biscuitos.github.io/blog/HISTORY-MMU/#C00033)

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND000100.png)

----------------------------------------------

<span id="H-linux-2.6.12.2"></span>

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI000787.JPG)

#### KMAP Linux 2.6.12.2

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI000969.png)

Linux 2.6.12.2 采用 KMAP 分配器管理 KMAP 虚拟内存区域。

###### KMAP 分配

{% highlight bash %}
void fastcall *kmap_high(struct page *page)
kmap_atomic
kmap_atomic_pfn
kmap_atomic_to_page
{% endhighlight %}

###### KMAP 释放

{% highlight bash %}
void fastcall kunmap_high(struct page *page)
kunmap_atomic
{% endhighlight %}

具体函数解析说明，请查看:

> - [KMAP API](#K)

###### 与项目相关

KMAP 内存分配器与本项目相关的 kmap_high() 调用顺序如下:

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI000979.png)

KMAP 内存分配器与本项目相关的 kunmap_high() 调用顺序如下:

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI000980.png)

项目中虚拟内存布局如下:

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI000737.png)

在项目中，KMAP 虚拟内存的管理的范围是: 0x96000000 到 0x96200000. 

###### 补丁

相对于前一个版本 linux 2.6.12.1, 该版本并未产生补丁。更多补丁的使用请参考:

> - [BiscuitOS Memory Manager Patch 建议](https://biscuitos.github.io/blog/HISTORY-MMU/#C00033)

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND000100.png)

----------------------------------------------

<span id="H-linux-2.6.12.3"></span>

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI000788.JPG)

#### KMAP Linux 2.6.12.3

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI000969.png)

Linux 2.6.12.3 采用 KMAP 分配器管理 KMAP 虚拟内存区域。

###### KMAP 分配

{% highlight bash %}
void fastcall *kmap_high(struct page *page)
kmap_atomic
kmap_atomic_pfn
kmap_atomic_to_page
{% endhighlight %}

###### KMAP 释放

{% highlight bash %}
void fastcall kunmap_high(struct page *page)
kunmap_atomic
{% endhighlight %}

具体函数解析说明，请查看:

> - [KMAP API](#K)

###### 与项目相关

KMAP 内存分配器与本项目相关的 kmap_high() 调用顺序如下:

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI000979.png)

KMAP 内存分配器与本项目相关的 kunmap_high() 调用顺序如下:

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI000980.png)

项目中虚拟内存布局如下:

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI000737.png)

在项目中，KMAP 虚拟内存的管理的范围是: 0x96000000 到 0x96200000. 

###### 补丁

相对于前一个版本 linux 2.6.12.2, 该版本并未产生补丁。更多补丁的使用请参考:

> - [BiscuitOS Memory Manager Patch 建议](https://biscuitos.github.io/blog/HISTORY-MMU/#C00033)

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND000100.png)

----------------------------------------------

<span id="H-linux-2.6.12.4"></span>

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI000789.JPG)

#### KMAP Linux 2.6.12.4

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI000969.png)

Linux 2.6.12.4 采用 KMAP 分配器管理 KMAP 虚拟内存区域。

###### KMAP 分配

{% highlight bash %}
void fastcall *kmap_high(struct page *page)
kmap_atomic
kmap_atomic_pfn
kmap_atomic_to_page
{% endhighlight %}

###### KMAP 释放

{% highlight bash %}
void fastcall kunmap_high(struct page *page)
kunmap_atomic
{% endhighlight %}

具体函数解析说明，请查看:

> - [KMAP API](#K)

###### 与项目相关

KMAP 内存分配器与本项目相关的 kmap_high() 调用顺序如下:

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI000979.png)

KMAP 内存分配器与本项目相关的 kunmap_high() 调用顺序如下:

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI000980.png)

项目中虚拟内存布局如下:

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI000737.png)

在项目中，KMAP 虚拟内存的管理的范围是: 0x96000000 到 0x96200000. 

###### 补丁

相对于前一个版本 linux 2.6.12.3, 该版本并未产生补丁。更多补丁的使用请参考:

> - [BiscuitOS Memory Manager Patch 建议](https://biscuitos.github.io/blog/HISTORY-MMU/#C00033)

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND000100.png)

----------------------------------------------

<span id="H-linux-2.6.12.5"></span>

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI000790.JPG)

#### KMAP Linux 2.6.12.5

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI000969.png)

Linux 2.6.12.5 采用 KMAP 分配器管理 KMAP 虚拟内存区域。

###### KMAP 分配

{% highlight bash %}
void fastcall *kmap_high(struct page *page)
kmap_atomic
kmap_atomic_pfn
kmap_atomic_to_page
{% endhighlight %}

###### KMAP 释放

{% highlight bash %}
void fastcall kunmap_high(struct page *page)
kunmap_atomic
{% endhighlight %}

具体函数解析说明，请查看:

> - [KMAP API](#K)

###### 与项目相关

KMAP 内存分配器与本项目相关的 kmap_high() 调用顺序如下:

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI000979.png)

KMAP 内存分配器与本项目相关的 kunmap_high() 调用顺序如下:

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI000980.png)

项目中虚拟内存布局如下:

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI000737.png)

在项目中，KMAP 虚拟内存的管理的范围是: 0x96000000 到 0x96200000. 

###### 补丁

相对于前一个版本 linux 2.6.12.4, 该版本并未产生补丁。更多补丁的使用请参考:

> - [BiscuitOS Memory Manager Patch 建议](https://biscuitos.github.io/blog/HISTORY-MMU/#C00033)

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND000100.png)

----------------------------------------------

<span id="H-linux-2.6.12.6"></span>

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI000791.JPG)

#### KMAP Linux 2.6.12.6

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI000969.png)

Linux 2.6.12.6 采用 KMAP 分配器管理 KMAP 虚拟内存区域。

###### KMAP 分配

{% highlight bash %}
void fastcall *kmap_high(struct page *page)
kmap_atomic
kmap_atomic_pfn
kmap_atomic_to_page
{% endhighlight %}

###### KMAP 释放

{% highlight bash %}
void fastcall kunmap_high(struct page *page)
kunmap_atomic
{% endhighlight %}

具体函数解析说明，请查看:

> - [KMAP API](#K)

###### 与项目相关

KMAP 内存分配器与本项目相关的 kmap_high() 调用顺序如下:

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI000979.png)

KMAP 内存分配器与本项目相关的 kunmap_high() 调用顺序如下:

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI000980.png)

项目中虚拟内存布局如下:

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI000737.png)

在项目中，KMAP 虚拟内存的管理的范围是: 0x96000000 到 0x96200000. 

###### 补丁

相对于前一个版本 linux 2.6.12.5, 该版本并未产生补丁。更多补丁的使用请参考:

> - [BiscuitOS Memory Manager Patch 建议](https://biscuitos.github.io/blog/HISTORY-MMU/#C00033)

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND000100.png)

----------------------------------------------

<span id="H-linux-2.6.13"></span>

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI000792.JPG)

#### KMAP Linux 2.6.13

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI000969.png)

Linux 2.6.13 采用 KMAP 分配器管理 KMAP 虚拟内存区域。

###### KMAP 分配

{% highlight bash %}
void fastcall *kmap_high(struct page *page)
kmap_atomic
kmap_atomic_pfn
kmap_atomic_to_page
{% endhighlight %}

###### KMAP 释放

{% highlight bash %}
void fastcall kunmap_high(struct page *page)
kunmap_atomic
{% endhighlight %}

具体函数解析说明，请查看:

> - [KMAP API](#K)

###### 与项目相关

KMAP 内存分配器与本项目相关的 kmap_high() 调用顺序如下:

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI000979.png)

KMAP 内存分配器与本项目相关的 kunmap_high() 调用顺序如下:

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI000980.png)

项目中虚拟内存布局如下:

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI000737.png)

在项目中，KMAP 虚拟内存的管理的范围是: 0x96000000 到 0x96200000. 

###### 补丁

相对于前一个版本 linux 2.6.12.6, 该版本并产生一个补丁。

{% highlight bash %}
tig mm/highmem.c include/linux/highmem.h

2005-06-25 14:58 Vivek Goyal       o [PATCH] kdump: Routines for copying dump pages
                                     [main] 60e64d46a58236e3c718074372cab6a5b56a3b15
{% endhighlight %}

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI000981.png)

{% highlight bash %}
git format-patch -1 60e64d46a58236e3c718074372cab6a5b56a3b15
vi 0001-PATCH-kdump-Routines-for-copying-dump-pages.patch
{% endhighlight %}

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI000982.png)

该补丁添加了 kmap_atomic_pfn() 实现.


更多补丁的使用请参考:

> - [BiscuitOS Memory Manager Patch 建议](https://biscuitos.github.io/blog/HISTORY-MMU/#C00033)

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND000100.png)

----------------------------------------------

<span id="H-linux-2.6.13.1"></span>

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI000793.JPG)

#### KMAP Linux 2.6.13.1

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI000969.png)

Linux 2.6.13.1 采用 KMAP 分配器管理 KMAP 虚拟内存区域。

###### KMAP 分配

{% highlight bash %}
void fastcall *kmap_high(struct page *page)
kmap_atomic
kmap_atomic_pfn
kmap_atomic_to_page
{% endhighlight %}

###### KMAP 释放

{% highlight bash %}
void fastcall kunmap_high(struct page *page)
kunmap_atomic
{% endhighlight %}

具体函数解析说明，请查看:

> - [KMAP API](#K)

###### 与项目相关

KMAP 内存分配器与本项目相关的 kmap_high() 调用顺序如下:

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI000979.png)

KMAP 内存分配器与本项目相关的 kunmap_high() 调用顺序如下:

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI000980.png)

项目中虚拟内存布局如下:

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI000737.png)

在项目中，KMAP 虚拟内存的管理的范围是: 0x96000000 到 0x96200000. 

###### 补丁

相对于前一个版本 linux 2.6.13, 该版本并未产生补丁。更多补丁的使用请参考:

> - [BiscuitOS Memory Manager Patch 建议](https://biscuitos.github.io/blog/HISTORY-MMU/#C00033)

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND000100.png)

----------------------------------------------

<span id="H-linux-2.6.14"></span>

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI000794.JPG)

#### KMAP Linux 2.6.14

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI000969.png)

Linux 2.6.14 采用 KMAP 分配器管理 KMAP 虚拟内存区域。

###### KMAP 分配

{% highlight bash %}
void fastcall *kmap_high(struct page *page)
kmap_atomic
kmap_atomic_pfn
kmap_atomic_to_page
{% endhighlight %}

###### KMAP 释放

{% highlight bash %}
void fastcall kunmap_high(struct page *page)
kunmap_atomic
{% endhighlight %}

具体函数解析说明，请查看:

> - [KMAP API](#K)

###### 与项目相关

KMAP 内存分配器与本项目相关的 kmap_high() 调用顺序如下:

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI000979.png)

KMAP 内存分配器与本项目相关的 kunmap_high() 调用顺序如下:

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI000980.png)

项目中虚拟内存布局如下:

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI000737.png)

在项目中，KMAP 虚拟内存的管理的范围是: 0x96000000 到 0x96200000. 

###### 补丁

相对于前一个版本 linux 2.6.13.1, 该版本并未产生补丁。更多补丁的使用请参考:

> - [BiscuitOS Memory Manager Patch 建议](https://biscuitos.github.io/blog/HISTORY-MMU/#C00033)

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND000100.png)

----------------------------------------------

<span id="H-linux-2.6.15"></span>

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI000795.JPG)

#### KMAP Linux 2.6.15

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI000969.png)

Linux 2.6.12.1 采用 KMAP 分配器管理 KMAP 虚拟内存区域。

###### KMAP 分配

{% highlight bash %}
void fastcall *kmap_high(struct page *page)
kmap_atomic
kmap_atomic_pfn
kmap_atomic_to_page
{% endhighlight %}

###### KMAP 释放

{% highlight bash %}
void fastcall kunmap_high(struct page *page)
kunmap_atomic
{% endhighlight %}

具体函数解析说明，请查看:

> - [KMAP API](#K)

###### 与项目相关

KMAP 内存分配器与本项目相关的 kmap_high() 调用顺序如下:

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI000979.png)

KMAP 内存分配器与本项目相关的 kunmap_high() 调用顺序如下:

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI000980.png)

项目中虚拟内存布局如下:

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI000737.png)

在项目中，KMAP 虚拟内存的管理的范围是: 0x96000000 到 0x96200000. 

###### 补丁
 
相对于前一个版本 linux 2.6.13, 该版本并未产生补丁。更多补丁的使用请参考:

> - [BiscuitOS Memory Manager Patch 建议](https://biscuitos.github.io/blog/HISTORY-MMU/#C00033)

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND000100.png)

-----------------------------------------------

<span id="G"></span>

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND00000H.jpg)

#### KMAP 历史时间轴

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI000983.png)

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND000100.png)

-----------------------------------------------

<span id="K"></span>

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND00000B.jpg)

#### KMAP API

###### flush_all_zero_pkmaps

{% highlight bash %}
static void flush_all_zero_pkmaps(void)
  作用: KMAP 分配器刷新所有未使用 KMAP 虚拟内存的页表.
{% endhighlight %}

###### kmap

{% highlight bash %}
void *kmap(struct page *page)
  作用: 将物理页与 KMAP 虚拟内存进行映射.
{% endhighlight %}

###### kmap_high

{% highlight bash %}
void fastcall *kmap_high(struct page *page)
  作用: 将一个物理页与 KMAP 虚拟内存进行映射.
{% endhighlight %}

###### kunmap

{% highlight bash %}
void kunmap(struct page *page)
  作用: 解除物理页与 KMAP 虚拟内存的映射关系.
{% endhighlight %}

###### kunmap_high

{% highlight bash %}
void fastcall kunmap_high(struct page *page)
  作用: 解除物理页与 KMAP 虚拟内存的映射关系.
{% endhighlight %}

###### map_new_virtual

{% highlight bash %}
static inline unsigned long map_new_virtual(struct page *page)
  作用: 将物理页映射到 KMAP 虚拟区.
{% endhighlight %}

###### page_address

{% highlight bash %}
void *page_address(struct page *page)
  作用: 获得物理页对应的虚拟地址.
{% endhighlight %}

###### page_address_init

{% highlight bash %}
void __init page_address_init(void)
  作用: 初始化高端地址映射池
{% endhighlight %}

###### page_slot

{% highlight bash %}
static struct page_address_slot *page_slot(struct page *page)
  作用: 获得高端物理页在高端地址映射池的 slot.
{% endhighlight %}

###### set_page_address

{% highlight bash %}
void set_page_address(struct page *page, void *virtual)
  作用: 设置物理页的虚拟地址.
{% endhighlight %}

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND000100.png)

----------------------------------

<span id="F"></span>

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND00000K.jpg)

#### KMAP 进阶研究

> - [用户空间实现一个 KMAP 内存分配器](https://biscuitos.github.io/blog/Memory-Userspace/#N)

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND000100.png)

-----------------------------------------------

<span id="E"></span>

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND00000L.jpg)

#### KMAP 内存分配器调试

> - [BiscuitOS KMAP 内存分配器调试](#C0004)

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

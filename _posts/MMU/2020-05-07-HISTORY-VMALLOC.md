---
layout: post
title:  "VMALLOC Allocator"
date:   2020-05-07 09:37:30 +0800
categories: [HW]
excerpt: MMU.
tags:
  - MMU
---

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND00000L0.PNG)

> Email: BuddyZhang1 <buddy.zhang@aliyun.com>
>
> WeChat: Buddy514981221 (张仁泽)

#### 目录

> - [VMALLOC 分配器原理](#A)
>
> - [VMALLOC 分配器使用](#B)
>
> - [VMALLOC 分配器实践](#C)
>
> - [VMALLOC 源码分析](#D)
>
> - [VMALLOC 分配器调试](#E)
>
> - [VMALLOC 分配进阶研究](#F)
>
> - [VMALLOC 时间轴](#G)
>
> - [VMALLOC 历史补丁](#H)
>
> - [VMALLOC API](#K)
>
> - [附录/捐赠](#Z0)

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND000100.png)

----------------------------------

<span id="A"></span>

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND00000T.jpg)

#### VMALLOC 分配器原理

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/HK000226.png)

VMALLOC 内存分配器称为 "Virtual Memory Allocator", 从定义可以知道 VMALLOC
内存的主要任务就是分配虚拟内存。在 Linux 内核中，划分了一块虚拟内存区域给
VMALLOC 内存分配器进行管理, VMALLOC 分配器提供相应的函数将这段虚拟内存分配
给内核其他子系统. 在不同的体系结构中 VMALLOC 内存管理器管理的虚拟内存区域
可能不同，但可以通过 VMALLOC_START 和 VMALLOC_END 进行确认.

VMALLOC 内存分配器分配虚拟内存之后，虚拟内存还没有与物理内存进行映射，因此
VMALLOC 内存需要和物理页进行映射之后才算真正的从 VMALLOC 分配器中分配可用
的虚拟内存。物理页可用通过 Buddy 内存管理器分配，也可以通过 PCP 内存管理器
进行分配，物理内存可以来自 ZONE_HIGHMEM，也可以来自非 ZONE_HIGHMEM 的区间。
由于 VMALLOC 分配器可以分配连续的虚拟地址，但与其映射的物理地址可能不是连续
的物理页块，也可能是连续的物理页块，因此通俗称 VMALLOC 内存分配器分配的内存
特点是 "虚拟地址连续，物理地址不一定连续的内存"。

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/HK000289.png)

VMALLOC 分配器的历史上采用过多种数据结构管理 VMALLOC 虚拟区虚拟地址的分配
和回收，比如最新版本采用红黑树管理 VMALLOC 虚拟区域的分配和回收，而在较早
的内核中，只是采用简单的遍历来查找 VMALLOC 虚拟区域内可用的虚拟内存块和回收
虚拟内存区块. 当无论采用什么方法管理 VMALLOc 虚拟区域，VMALLOC 内存管理器
总要保证能从 VMALLOC 区域内进行虚拟地址的分配和释放。

当 VMALLOC 内存分配器从 VMALLOC 虚拟区域内找到一块可用的虚拟区域之后，都会
在指定虚拟内存之后加上一个 PAGE_SIZE 的虚拟内存，这块虚拟内存称为 VMALLOC
的 GUARD 虚拟内存块，主要是用于隔离其他已经分配的虚拟内存区块. 虚拟地址分配
完毕之后，VMALLOC 内存管理器就从 Buddy 或者 PCP 内存管理器中分配对应数量的
物理页，然后将虚拟内存和物理内存建立页表。VMALLOC 分配器建立的页表可以是三级
页表，也可以高达五级页表。以上操作完毕之后，VMALLOC 分配器将虚拟地址传递给
调用者，调用者就可以使用这段虚拟内存区域. 由于 VMALLOC 按 PAGE_SIZE 为单位
从 Buddy/PCP 内存管理器中分配物理内存，并且 VMALLOC 分配虚拟地址时按 PAGE_SIZE
大小进行对齐，因此 VMALLOC 内存分配器分配以 PAGE_SIZE 为单位的虚拟内存块。
VMALLOC 内存分配器提供的分配接口如下:

{% highlight bash %}
void *vmalloc(unsigned long size)
void *vmalloc_32(unsigned long size)
void *vmalloc_node(unsigned long size, int node)
void *__vmalloc(unsigned long size, gfp_t gfp_mask, pgprot_t prot)
void *__vmalloc_node(unsigned long size, gfp_t gfp_mask, pgprot_t prot, int node)
{% endhighlight %}

当使用者使用完 VMALLOC 内存分配器分配的内存之后，通过 VMALLOC 释放函数将虚拟
地址传递给 VMALLOC 分配器，VMALLOC 分配器执行与分配相反的动作，首先将虚拟地址
与物理地址关联的页表清除，然后释放对应的物理页块给 Buddy 内存管理器或者 PCP
内存管理器, 执行完毕之后 VMALLOC 释放才完成. VMALLOC 内存分配器提供的释放
接口如下:

{% highlight bash %}
void vfree(void *addr)
{% endhighlight %}

###### VMALLOC 页表

当从 VMALLOC 分配器获得一段 VMALLOc 虚拟内存块之后，接下来是分配一段物理内存，
并建立 VMALLOC 虚拟地址与物理地址之间的映射关系，即建立页表. VMALLOC 内存
分配器可以支持多级页表，例如 linux 2.6 版本的 VMALLOC 分配器支持如下页表:

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI000978.png)

VMALLOC 分配器通过建立 4 级页表最终将物理内存和 VMALLOC 虚拟内存建立映射
关系. Linux 5.x 版本的 VMALLOC 分配器支持如下页表:

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI000968.png)

###### Linux 2.6 版本的 VMALLOC

在 Linux 2.6 版本上，VMALLOC 使用 struct vm_struct 结构维护一段 VMALLOC
虚拟内存块，定义如下:

{% highlight bash %}
struct vm_struct {
        void                    *addr;
        unsigned long           size;
        unsigned long           flags;
        struct page             **pages;
        unsigned int            nr_pages;
        unsigned long           phys_addr;
        struct vm_struct        *next;
};
{% endhighlight %}

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI000967.png)

VMALLOC 内存管理器通过建立一个 struct vm_struct 结构的 vmlist 链表。当 VMALLOC
从 VMALLOC 虚拟内存区域找一块可用内存时，遍历 vmlist 链表，以此确认找到
的虚拟内存区块未使用，然后将其加入到 vmlist 里。当释放 VMALLOC 虚拟块的
时候，VMALLOC 分配器将其对应的 struct vm_struct 从 vmlist 链表中移除.

###### Linux 5.x 版本的 VMALLOC

在 Linux 5.x 版本上，VMALLOC 使用一颗红黑树管理着所有的 VMALLOC 虚拟内存。
当 VMALLOC 管理器需要分配虚拟内存时，VMALLOc 从红黑树上查找一块可用的虚拟
内存。当释放时，VMALLOC 分配器再将虚拟地址插入到红黑树中.

---------------------------------

###### VMALLOC 的优点

当系统运行一段时间后，线性映射区已经很难找到连续的虚拟内存区域了，这时可以
从 VMALLOC 区域获得连续的物理内存。

###### VMALLOC 的缺点

由于要动态建立页表，分配虚拟内存的效率远低于 Slab/Slub 等线性区分配器.

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND000100.png)

---------------------------------

<span id="B"></span>

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND00000H.jpg)

#### VMALLOC 分配器使用

> - [基础用法介绍](#B0000)
>
> - [VMALLOC 分配虚拟内存](#B0001)

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND000100.png)

-----------------------------------------------

#### <span id="B0000">基础用法介绍</span>

VMALLOC 分配器提供了用于分配和释放虚拟内存的相关函数接口:

###### VMALLOC 分配

{% highlight bash %}
void *vmalloc(unsigned long size)
void *vmalloc_32(unsigned long size)
void *vmalloc_node(unsigned long size, int node)
void *__vmalloc(unsigned long size, gfp_t gfp_mask, pgprot_t prot)
void *__vmalloc_node(unsigned long size, gfp_t gfp_mask, pgprot_t prot, int node)
{% endhighlight %}

###### VMALLOC 释放

{% highlight bash %}
void vfree(void *addr)
{% endhighlight %}

具体函数解析说明，请查看:

> - [VMALLOC API](#K)

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND000100.png)

----------------------------------------------------

#### <span id="B0001">VMALLOC 分配虚拟内存</span>

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/HK000289.png)

从 VMALLOC 区域内存分配、使用和释放虚拟内存，开发者可以参考如下代码:

{% highlight c %}
#include <linux/mm.h>
#include <linux/vmalloc.h>

static int TestCase_vmalloc(void)
{
        void *data;

        /* Alloc */
        data = vmalloc(PAGE_SIZE);
        sprintf((char *)data, "BiscuitOS-%s", __func__);
        printk("[%#lx] %s\n", (unsigned long)data, (char *)data);

        /* free */
        vfree(data);
        return 0;
}
{% endhighlight %}

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND000100.png)

------------------------------------------------

<span id="C"></span>

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND00000I.jpg)

#### VMALLOC 分配器实践

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
支持多个版本的 VMALLOC，开发者可以根据需求进行选择，本文以 linux 2.6.12 
版本的 VMALLOC 进行讲解。开发者使用如下命令:

{% highlight bash %}
cd BiscuitOS/
make linux-5.0-arm32_defconfig
make menuconfig
{% endhighlight %}

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI000746.png)

选择并进入 "[\*] Package  --->" 目录。

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI000747.png)

选择并进入 "[\*]   Memory Development History  --->" 目录。

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI000945.png)

选择并进入 "[\*]   VMALLOC Allocator  --->" 目录。

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI000946.png)

选择 "[\*]   VMALLOC on linux 2.6.12  --->" 目录，保存并退出。接着执行如下命令:

{% highlight bash %}
make
{% endhighlight %}

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI000750.png)

成功之后将出现上图的内容，接下来开发者执行如下命令以便切换到项目的路径:

{% highlight bash %}
cd BiscuitOS/output/linux-5.0-arm32/package/BiscuitOS_VMALLOC-2.6.12
make download
{% endhighlight %}

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI000947.png)

至此源码已经下载完成，开发者可以使用 tree 等工具查看源码:

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI000948.png)

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
cd BiscuitOS/output/linux-5.0-arm32/package/BiscuitOS_VMALLOC-2.6.12
make kernel
{% endhighlight %}

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI000949.png)

--------------------------------------------

#### <span id="C0002">实践执行</span>

环境部署完毕之后，开发者可以向通用模块一样对源码进行编译和安装使用，使用
如下命令:

{% highlight bash %}
cd BiscuitOS/output/linux-5.0-arm32/package/BiscuitOS_VMALLOC-2.6.12
make
{% endhighlight %}

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI000950.png)

以上就是模块成功编译，接下来将 ko 模块安装到 BiscuitOS 中，使用如下命令:

{% highlight bash %}
cd BiscuitOS/output/linux-5.0-arm32/package/BiscuitOS_VMALLOC-2.6.12
make install
make pack
{% endhighlight %}

以上准备完毕之后，最后就是在 BiscuitOS 运行这个模块了，使用如下命令:

{% highlight bash %}
cd BiscuitOS/output/linux-5.0-arm32/package/BiscuitOS_VMALLOC-2.6.12
make run
{% endhighlight %}

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI000951.png)

在 BiscuitOS 中插入了模块 "BiscuitOS_VMALLOC-2.6.12.ko"，打印如上信息，那么
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
/xspace/OpenSource/BiscuitOS/BiscuitOS/output/linux-5.0-arm32/package/BiscuitOS_VMALLOC-2.6.12/BiscuitOS_VMALLOC-2.6.12/Makefile
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

然后先向 BiscuitOS 中插入 "BiscuitOS_VMALLOC-2.6.12.ko" 模块，然后再插入
"BiscuitOS_VMALLOC-2.6.12-buddy.ko" 模块。如下:

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI000773.png)

以上便是测试代码的使用办法。开发者如果想在源码中启用或关闭某些宏，可以
修改 Makefile 中内容:

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI000774.png)

从上图可以知道，如果要启用某些宏，可以在 ccflags-y 中添加 "-D" 选项进行
启用，源码的编译参数也可以添加到 ccflags-y 中去。开发者除了使用上面的办法
进行测试之外，也可以使用项目提供的 initcall 机制进行调试，具体请参考:

> - [Initcall 机制调试说明](#C00032)

Initcall 机制提供了以下函数用于 VMALLOC 调试:

{% highlight bash %}
vmalloc_initcall_bs()
{% endhighlight %}

从项目的 Initcall 机制可以知道，vmalloc_initcall_bs() 调用的函数将
在 VMALLOC 分配器初始化完毕之后自动调用。VMALLOC 相关的测试代码位于:

{% highlight bash %}
BiscuitOS/output/linux-5.0-arm32/package/BiscuitOS_VMALLOC-2.6.12/BiscuitOS_VMALLOC-2.6.12/module/vmalloc/
{% endhighlight %}

在 Makefile 中打开调试开关:

{% highlight bash %}
$(MODULE_NAME)-m                += modules/vmalloc/main.o
{% endhighlight %}

VMALLOC 测试代码也包含模块测试，在 Makefile 中打开调试开关:

{% highlight bash %}
obj-m                           += $(MODULE_NAME)-vmalloc.o
$(MODULE_NAME)-vmalloc-m        := modules/vmalloc/module.o
{% endhighlight %}

VMALLOC 模块测试结果如下:

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI000952.png)

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND000100.png)

----------------------------------

<span id="H"></span>

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND00000T.jpg)

#### VMALLOC 历史补丁

> - [VMALLOC Linux 2.6.12](#H-linux-2.6.12)
>
> - [VMALLOC Linux 2.6.12.1](#H-linux-2.6.12.1)
>
> - [VMALLOC Linux 2.6.12.2](#H-linux-2.6.12.2)
>
> - [VMALLOC Linux 2.6.12.3](#H-linux-2.6.12.3)
>
> - [VMALLOC Linux 2.6.12.4](#H-linux-2.6.12.4)
>
> - [VMALLOC Linux 2.6.12.5](#H-linux-2.6.12.5)
>
> - [VMALLOC Linux 2.6.12.6](#H-linux-2.6.12.6)
>
> - [VMALLOC Linux 2.6.13](#H-linux-2.6.13)
>
> - [VMALLOC Linux 2.6.13.1](#H-linux-2.6.13.1)
>
> - [VMALLOC Linux 2.6.14](#H-linux-2.6.14)
>
> - [VMALLOC Linux 2.6.15](#H-linux-2.6.15)

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND000100.png)

----------------------------------------------

<span id="H-linux-2.6.12"></span>

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI000785.JPG)

#### VMALLOC Linux 2.6.12

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/HK000289.png)

Linux 2.6.12 采用 VMALLOC 分配器管理 VMALLOC 虚拟内存区域。

###### VMALLOC 分配

{% highlight bash %}
void *vmalloc(unsigned long size)
void *vmalloc_32(unsigned long size)
void *vmalloc_node(unsigned long size, int node)
void *__vmalloc(unsigned long size, gfp_t gfp_mask, pgprot_t prot)
void *__vmalloc_node(unsigned long size, gfp_t gfp_mask, pgprot_t prot, int node)
{% endhighlight %}

###### VMALLOC 释放

{% highlight bash %}
void vfree(void *addr)
{% endhighlight %}

具体函数解析说明，请查看:

> - [VMALLOC API](#K)

###### 与项目相关

VMALLOC 内存分配器与本项目相关的 vmalloc() 调用顺序如下:

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI000953.png)

VMALLOC 内存分配器与本项目相关的 vfree() 调用顺序如下:

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI000954.png)

项目中虚拟内存布局如下:

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI000737.png)

在项目中，VMALLOC 虚拟内存的管理的范围是: 0x9440A000 到 0x95E0A000. 

###### 补丁

对于 Linux 2.6.12 的补丁，Linus 将 Linux 内核源码树加入到 git 中来，因此
这个版本的代码均不产生补丁。更多补丁的使用请参考:

> - [BiscuitOS Memory Manager Patch 建议](https://biscuitos.github.io/blog/HISTORY-MMU/#C00033)

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND000100.png)

----------------------------------------------

<span id="H-linux-2.6.12.1"></span>

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI000786.JPG)

#### VMALLOC Linux 2.6.12.1

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/HK000289.png)

Linux 2.6.12.1 采用 VMALLOC 分配器管理 VMALLOC 虚拟内存区域。

###### VMALLOC 分配

{% highlight bash %}
void *vmalloc(unsigned long size)
void *vmalloc_32(unsigned long size)
void *vmalloc_node(unsigned long size, int node)
void *__vmalloc(unsigned long size, gfp_t gfp_mask, pgprot_t prot)
void *__vmalloc_node(unsigned long size, gfp_t gfp_mask, pgprot_t prot, int node)
{% endhighlight %}

###### VMALLOC 释放

{% highlight bash %}
void vfree(void *addr)
{% endhighlight %}

具体函数解析说明，请查看:

> - [VMALLOC API](#K)

###### 与项目相关

VMALLOC 内存分配器与本项目相关的 vmalloc() 调用顺序如下:

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI000953.png)

VMALLOC 内存分配器与本项目相关的 vfree() 调用顺序如下:

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI000954.png)

项目中虚拟内存布局如下:

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI000737.png)

在项目中，VMALLOC 虚拟内存的管理的范围是: 0x9440A000 到 0x95E0A000. 

###### 补丁

与 Linux 2.6.12 的相比，VMALLOC 并为产生补丁。更多补丁的使用请参考:

> - [BiscuitOS Memory Manager Patch 建议](https://biscuitos.github.io/blog/HISTORY-MMU/#C00033)

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND000100.png)

----------------------------------------------

<span id="H-linux-2.6.12.2"></span>

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI000787.JPG)

#### VMALLOC Linux 2.6.12.2

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/HK000289.png)

Linux 2.6.12.2 采用 VMALLOC 分配器管理 VMALLOC 虚拟内存区域。

###### VMALLOC 分配

{% highlight bash %}
void *vmalloc(unsigned long size)
void *vmalloc_32(unsigned long size)
void *vmalloc_node(unsigned long size, int node)
void *__vmalloc(unsigned long size, gfp_t gfp_mask, pgprot_t prot)
void *__vmalloc_node(unsigned long size, gfp_t gfp_mask, pgprot_t prot, int node)
{% endhighlight %}

###### VMALLOC 释放

{% highlight bash %}
void vfree(void *addr)
{% endhighlight %}

具体函数解析说明，请查看:

> - [VMALLOC API](#K)

###### 与项目相关

VMALLOC 内存分配器与本项目相关的 vmalloc() 调用顺序如下:

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI000953.png)

VMALLOC 内存分配器与本项目相关的 vfree() 调用顺序如下:

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI000954.png)

项目中虚拟内存布局如下:

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI000737.png)

在项目中，VMALLOC 虚拟内存的管理的范围是: 0x9440A000 到 0x95E0A000. 

###### 补丁
 
与 Linux 2.6.12.1 的相比，VMALLOC 并为产生补丁。更多补丁的使用请参考:

> - [BiscuitOS Memory Manager Patch 建议](https://biscuitos.github.io/blog/HISTORY-MMU/#C00033)

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND000100.png)

----------------------------------------------

<span id="H-linux-2.6.12.3"></span>

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI000788.JPG)

#### VMALLOC Linux 2.6.12.3

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/HK000289.png)

Linux 2.6.12.3 采用 VMALLOC 分配器管理 VMALLOC 虚拟内存区域。

###### VMALLOC 分配

{% highlight bash %}
void *vmalloc(unsigned long size)
void *vmalloc_32(unsigned long size)
void *vmalloc_node(unsigned long size, int node)
void *__vmalloc(unsigned long size, gfp_t gfp_mask, pgprot_t prot)
void *__vmalloc_node(unsigned long size, gfp_t gfp_mask, pgprot_t prot, int node)
{% endhighlight %}

###### VMALLOC 释放

{% highlight bash %}
void vfree(void *addr)
{% endhighlight %}

具体函数解析说明，请查看:

> - [VMALLOC API](#K)

###### 与项目相关

VMALLOC 内存分配器与本项目相关的 vmalloc() 调用顺序如下:

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI000953.png)

VMALLOC 内存分配器与本项目相关的 vfree() 调用顺序如下:

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI000954.png)

项目中虚拟内存布局如下:

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI000737.png)

在项目中，VMALLOC 虚拟内存的管理的范围是: 0x9440A000 到 0x95E0A000. 

###### 补丁
 
与 Linux 2.6.12.2 的相比，VMALLOC 并为产生补丁。更多补丁的使用请参考:


> - [BiscuitOS Memory Manager Patch 建议](https://biscuitos.github.io/blog/HISTORY-MMU/#C00033)

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND000100.png)

----------------------------------------------

<span id="H-linux-2.6.12.4"></span>

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI000789.JPG)

#### VMALLOC Linux 2.6.12.4

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/HK000289.png)

Linux 2.6.12.4 采用 VMALLOC 分配器管理 VMALLOC 虚拟内存区域。

###### VMALLOC 分配

{% highlight bash %}
void *vmalloc(unsigned long size)
void *vmalloc_32(unsigned long size)
void *vmalloc_node(unsigned long size, int node)
void *__vmalloc(unsigned long size, gfp_t gfp_mask, pgprot_t prot)
void *__vmalloc_node(unsigned long size, gfp_t gfp_mask, pgprot_t prot, int node)
{% endhighlight %}

###### VMALLOC 释放

{% highlight bash %}
void vfree(void *addr)
{% endhighlight %}

具体函数解析说明，请查看:

> - [VMALLOC API](#K)

###### 与项目相关

VMALLOC 内存分配器与本项目相关的 vmalloc() 调用顺序如下:

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI000953.png)

VMALLOC 内存分配器与本项目相关的 vfree() 调用顺序如下:

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI000954.png)

项目中虚拟内存布局如下:

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI000737.png)

在项目中，VMALLOC 虚拟内存的管理的范围是: 0x9440A000 到 0x95E0A000. 

###### 补丁
 
与 Linux 2.6.12.3 的相比，VMALLOC 并为产生补丁。更多补丁的使用请参考:

> - [BiscuitOS Memory Manager Patch 建议](https://biscuitos.github.io/blog/HISTORY-MMU/#C00033)

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND000100.png)

----------------------------------------------

<span id="H-linux-2.6.12.5"></span>

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI000790.JPG)

#### VMALLOC Linux 2.6.12.5

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/HK000289.png)

Linux 2.6.12.5 采用 VMALLOC 分配器管理 VMALLOC 虚拟内存区域。

###### VMALLOC 分配

{% highlight bash %}
void *vmalloc(unsigned long size)
void *vmalloc_32(unsigned long size)
void *vmalloc_node(unsigned long size, int node)
void *__vmalloc(unsigned long size, gfp_t gfp_mask, pgprot_t prot)
void *__vmalloc_node(unsigned long size, gfp_t gfp_mask, pgprot_t prot, int node)
{% endhighlight %}

###### VMALLOC 释放

{% highlight bash %}
void vfree(void *addr)
{% endhighlight %}

具体函数解析说明，请查看:

> - [VMALLOC API](#K)

###### 与项目相关

VMALLOC 内存分配器与本项目相关的 vmalloc() 调用顺序如下:

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI000953.png)

VMALLOC 内存分配器与本项目相关的 vfree() 调用顺序如下:

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI000954.png)

项目中虚拟内存布局如下:

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI000737.png)

在项目中，VMALLOC 虚拟内存的管理的范围是: 0x9440A000 到 0x95E0A000. 

###### 补丁
 
与 Linux 2.6.12.4 的相比，VMALLOC 并为产生补丁。更多补丁的使用请参考:


> - [BiscuitOS Memory Manager Patch 建议](https://biscuitos.github.io/blog/HISTORY-MMU/#C00033)

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND000100.png)

----------------------------------------------

<span id="H-linux-2.6.12.6"></span>

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI000791.JPG)

#### VMALLOC Linux 2.6.12.6

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/HK000289.png)

Linux 2.6.12.6 采用 VMALLOC 分配器管理 VMALLOC 虚拟内存区域。

###### VMALLOC 分配

{% highlight bash %}
void *vmalloc(unsigned long size)
void *vmalloc_32(unsigned long size)
void *vmalloc_node(unsigned long size, int node)
void *__vmalloc(unsigned long size, gfp_t gfp_mask, pgprot_t prot)
void *__vmalloc_node(unsigned long size, gfp_t gfp_mask, pgprot_t prot, int node)
{% endhighlight %}

###### VMALLOC 释放

{% highlight bash %}
void vfree(void *addr)
{% endhighlight %}

具体函数解析说明，请查看:

> - [VMALLOC API](#K)

###### 与项目相关

VMALLOC 内存分配器与本项目相关的 vmalloc() 调用顺序如下:

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI000953.png)

VMALLOC 内存分配器与本项目相关的 vfree() 调用顺序如下:

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI000954.png)

项目中虚拟内存布局如下:

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI000737.png)

在项目中，VMALLOC 虚拟内存的管理的范围是: 0x9440A000 到 0x95E0A000. 

###### 补丁
 
与 Linux 2.6.12.5 的相比，VMALLOC 并为产生补丁。更多补丁的使用请参考:

> - [BiscuitOS Memory Manager Patch 建议](https://biscuitos.github.io/blog/HISTORY-MMU/#C00033)

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND000100.png)

----------------------------------------------

<span id="H-linux-2.6.13"></span>

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI000792.JPG)

#### VMALLOC Linux 2.6.13

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/HK000289.png)

Linux 2.6.13 采用 VMALLOC 分配器管理 VMALLOC 虚拟内存区域。

###### VMALLOC 分配

{% highlight bash %}
void *vmalloc(unsigned long size)
void *vmalloc_32(unsigned long size)
void *vmalloc_node(unsigned long size, int node)
void *__vmalloc(unsigned long size, gfp_t gfp_mask, pgprot_t prot)
void *__vmalloc_node(unsigned long size, gfp_t gfp_mask, pgprot_t prot, int node)
{% endhighlight %}

###### VMALLOC 释放

{% highlight bash %}
void vfree(void *addr)
{% endhighlight %}

具体函数解析说明，请查看:

> - [VMALLOC API](#K)

###### 与项目相关

VMALLOC 内存分配器与本项目相关的 vmalloc() 调用顺序如下:

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI000953.png)

VMALLOC 内存分配器与本项目相关的 vfree() 调用顺序如下:

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI000954.png)

项目中虚拟内存布局如下:

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI000737.png)

在项目中，VMALLOC 虚拟内存的管理的范围是: 0x9440A000 到 0x95E0A000. 

###### 补丁
 
相对上一个版本 linux 2.6.12.6，VMALLOC 内存分配器增加了多个补丁，如下:

{% highlight bash %}
tig mm/vmalloc.c include/linux/vmalloc.h

2005-05-01 08:59 Pavel Pisa     o [PATCH] DocBook: changes and extensions to the kernel documentation
                                  [main] 4dc3b16ba18c0f967ad100c52fa65b01a4f76ff0
2005-05-20 14:27 Andi Kleen     o [PATCH] x86_64: Fixed guard page handling again in iounmap
                                  [main] 7856dfeb23c16ef3d8dac8871b4d5b93c70b59b9
{% endhighlight %}

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI000955.png)

{% highlight bash %}
git format-patch -1 4dc3b16ba18c0f967ad100c52fa65b01a4f76ff0
vi 0001-PATCH-DocBook-changes-and-extensions-to-the-kernel-d.patch
{% endhighlight %}

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI000956.png)

该补丁新增了页表的权限 PAGE_KERNEL_EXEC, 使分配的虚拟内存拥有执行和读写权限.

{% highlight bash %}
git format-patch -1 7856dfeb23c16ef3d8dac8871b4d5b93c70b59b9
vi 0001-PATCH-x86_64-Fixed-guard-page-handling-again-in-ioun.patch 
{% endhighlight %}

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI000957.png)

该补丁新增了 \_\_remove_vm_area() 函数并更新了 remove_vm_area() 函数实现.
更多补丁使用方法请参考下面文章:

> - [BiscuitOS Memory Manager Patch 建议](https://biscuitos.github.io/blog/HISTORY-MMU/#C00033)

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND000100.png)

----------------------------------------------

<span id="H-linux-2.6.13.1"></span>

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI000793.JPG)

#### VMALLOC Linux 2.6.13.1

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/HK000289.png)

Linux 2.6.13.1 采用 VMALLOC 分配器管理 VMALLOC 虚拟内存区域。

###### VMALLOC 分配

{% highlight bash %}
void *vmalloc(unsigned long size)
void *vmalloc_32(unsigned long size)
void *vmalloc_node(unsigned long size, int node)
void *__vmalloc(unsigned long size, gfp_t gfp_mask, pgprot_t prot)
void *__vmalloc_node(unsigned long size, gfp_t gfp_mask, pgprot_t prot, int node)
{% endhighlight %}

###### VMALLOC 释放

{% highlight bash %}
void vfree(void *addr)
{% endhighlight %}

具体函数解析说明，请查看:

> - [VMALLOC API](#K)

###### 与项目相关

VMALLOC 内存分配器与本项目相关的 vmalloc() 调用顺序如下:

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI000953.png)

VMALLOC 内存分配器与本项目相关的 vfree() 调用顺序如下:

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI000954.png)

项目中虚拟内存布局如下:

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI000737.png)

在项目中，VMALLOC 虚拟内存的管理的范围是: 0x9440A000 到 0x95E0A000. 

###### 补丁
 
与 Linux 2.6.13 的相比，VMALLOC 并为产生补丁。更多补丁的使用请参考:

> - [BiscuitOS Memory Manager Patch 建议](https://biscuitos.github.io/blog/HISTORY-MMU/#C00033)

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND000100.png)

----------------------------------------------

<span id="H-linux-2.6.14"></span>

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI000794.JPG)

#### VMALLOC Linux 2.6.14

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/HK000289.png)

Linux 2.6.14 采用 VMALLOC 分配器管理 VMALLOC 虚拟内存区域。

###### VMALLOC 分配

{% highlight bash %}
void *vmalloc(unsigned long size)
void *vmalloc_32(unsigned long size)
void *vmalloc_node(unsigned long size, int node)
void *__vmalloc(unsigned long size, gfp_t gfp_mask, pgprot_t prot)
void *__vmalloc_node(unsigned long size, gfp_t gfp_mask, pgprot_t prot, int node)
{% endhighlight %}

###### VMALLOC 释放

{% highlight bash %}
void vfree(void *addr)
{% endhighlight %}

具体函数解析说明，请查看:

> - [VMALLOC API](#K)

###### 与项目相关

VMALLOC 内存分配器与本项目相关的 vmalloc() 调用顺序如下:

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI000953.png)

VMALLOC 内存分配器与本项目相关的 vfree() 调用顺序如下:

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI000954.png)

项目中虚拟内存布局如下:

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI000737.png)

在项目中，VMALLOC 虚拟内存的管理的范围是: 0x9440A000 到 0x95E0A000. 

###### 补丁
 
相对上一个版本 linux 2.6.13.1，VMALLOC 内存分配器增加了多个补丁，如下:

{% highlight bash %}
tig mm/vmalloc.c include/linux/vmalloc.h

2005-09-03 15:54 Deepak Saxena  o [PATCH] arm: allow for arch-specific IOREMAP_MAX_ORDER
                                  [main] fd195c49fb17a21e232f50bddb2267150053cf34
2005-09-09 13:10 Pekka Enberg   o [PATCH] update kfree, vfree, and vunmap kerneldoc
                                  [main] 80e93effce55044c5a7fa96e8b313640a80bd4e9
2005-10-07 07:46 Al Viro        o [PATCH] gfp flags annotations - part 1
                                  [main] dd0fc66fb33cd610bc1a5db8a5e232d34879b4d7
{% endhighlight %}

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI000958.png)

{% highlight bash %}
git format-patch -1 fd195c49fb17a21e232f50bddb2267150053cf34
vi 0001-PATCH-arm-allow-for-arch-specific-IOREMAP_MAX_ORDER.patch
{% endhighlight %}

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI000959.png)

该补丁将 IOREMAP_MAX_ORDER 宏移到 vmalloc.h.

{% highlight bash %}
git format-patch -1 80e93effce55044c5a7fa96e8b313640a80bd4e9
vi 0001-PATCH-update-kfree-vfree-and-vunmap-kerneldoc.patch
{% endhighlight %}

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI000960.png)

该补丁修改了相关的注释.

{% highlight bash %}
git format-patch -1 dd0fc66fb33cd610bc1a5db8a5e232d34879b4d7
vi 0001-PATCH-gfp-flags-annotations-part-1.patch
{% endhighlight %}

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI000961.png)

该补丁将 VMALLOC 分配器中 gfp 标志的类型替换成 gfp_t. 更多不行请参考下文:

> - [BiscuitOS Memory Manager Patch 建议](https://biscuitos.github.io/blog/HISTORY-MMU/#C00033)

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND000100.png)

----------------------------------------------

<span id="H-linux-2.6.15"></span>

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI000795.JPG)

#### VMALLOC Linux 2.6.15

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/HK000289.png)

Linux 2.6.15 采用 VMALLOC 分配器管理 VMALLOC 虚拟内存区域。

###### VMALLOC 分配

{% highlight bash %}
void *vmalloc(unsigned long size)
void *vmalloc_32(unsigned long size)
void *vmalloc_node(unsigned long size, int node)
void *__vmalloc(unsigned long size, gfp_t gfp_mask, pgprot_t prot)
void *__vmalloc_node(unsigned long size, gfp_t gfp_mask, pgprot_t prot, int node)
{% endhighlight %}

###### VMALLOC 释放

{% highlight bash %}
void vfree(void *addr)
{% endhighlight %}

具体函数解析说明，请查看:

> - [VMALLOC API](#K)

###### 与项目相关

VMALLOC 内存分配器与本项目相关的 vmalloc() 调用顺序如下:

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI000953.png)

VMALLOC 内存分配器与本项目相关的 vfree() 调用顺序如下:

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI000954.png)

项目中虚拟内存布局如下:

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI000737.png)

在项目中，VMALLOC 虚拟内存的管理的范围是: 0x9440A000 到 0x95E0A000. 

###### 补丁
 
相对上一个版本 linux 2.6.14，VMALLOC 内存分配器增加了多个补丁，如下:

{% highlight bash %}
tig mm/vmalloc.c include/linux/vmalloc.h

2005-10-29 18:15 Christoph Lameter o [PATCH] vmalloc_node
                                     [main] 930fc45a49ddebe7555cc5c837d82b9c27e65ff4
2005-10-29 18:16 Hugh Dickins      o [PATCH] mm: init_mm without ptlock
                                     [main] 872fec16d9a0ed3b75b8893aa217e49cca575ee5
2005-11-07 01:01 Randy Dunlap      o [PATCH] kernel-doc: fix warnings in vmalloc.c
                                     [main] d44e0780bcc47c9b8851099c0dfc1dda3c9db5a9
{% endhighlight %}

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI000962.png)

{% highlight bash %}
git format-patch -1 930fc45a49ddebe7555cc5c837d82b9c27e65ff4
vi 0001-PATCH-vmalloc_node.patch
{% endhighlight %}

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI000963.png)

新增 vmalloc_node() 和 \_\_vmalloc_node() 函数, 支持从指定的 NODE 上分配
VMALLOC 内存.

{% highlight bash %}
git format-patch -1 872fec16d9a0ed3b75b8893aa217e49cca575ee5
vi 0001-PATCH-mm-init_mm-without-ptlock.patch
{% endhighlight %}

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI000964.png)

该补丁用于在 VMALLOC 建立 PTE 页表时移除了 page_table_lock 锁.

{% highlight bash %}
git format-patch -1 d44e0780bcc47c9b8851099c0dfc1dda3c9db5a9
vi 0001-PATCH-kernel-doc-fix-warnings-in-vmalloc.c.patch
{% endhighlight %}

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI000960.png)

该补丁在 VMALLOC 分配器中添加了 node 相关的注释. 更多补丁使用请参考下文:

> - [BiscuitOS Memory Manager Patch 建议](https://biscuitos.github.io/blog/HISTORY-MMU/#C00033)

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND000100.png)

-----------------------------------------------

<span id="G"></span>

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND00000H.jpg)

#### VMALLOC 历史时间轴

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI000966.png)

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND000100.png)

-----------------------------------------------

<span id="K"></span>

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND00000B.jpg)

#### VMALLOC API



###### \_\_get_vm_area

{% highlight bash %}
struct vm_struct *__get_vm_area(unsigned long size, unsigned long flags,
                                unsigned long start, unsigned long end)
  作用: 分配一段可用的 VMALLOC 虚拟内存.
{% endhighlight %}

###### get_vm_area

{% highlight bash %}
struct vm_struct *get_vm_area(unsigned long size, unsigned long flags)
  作用: 分配一段可用的 VMALLOC 虚拟内存.
{% endhighlight %}

###### \_\_get_vm_area_node

{% highlight bash %}
struct vm_struct *__get_vm_area_node(unsigned long size, unsigned long flags,
                                unsigned long start, unsigned long end, int node)
  作用: 从指定节点上分配一段可用的 VMALLOC 虚拟内存.
{% endhighlight %}

###### get_vm_area_node

{% highlight bash %}
struct vm_struct *get_vm_area_node(unsigned long size, unsigned long flags, int node)
  作用: 从指定节点上分配一段可用的 VMALLOC 虚拟内存.
{% endhighlight %}

###### map_vm_area

{% highlight bash %}
int map_vm_area(struct vm_struct *area, pgprot_t prot, struct page ***pages)
  作用: 映射一段 VMALLOC 虚拟内存到物理内存上.
{% endhighlight %}

###### unmap_vm_area

{% highlight bash %}
void unmap_vm_area(struct vm_struct *area)
  作用: 解除一段 VMALLOC 虚拟内存的映射关系.
{% endhighlight %}

###### \_\_remove_vm_area

{% highlight bash %}
struct vm_struct *__remove_vm_area(void *addr)
  作用: 释放一段 VMALLOC 虚拟内存到 VMALLOC 内存分配器.
{% endhighlight %}

###### remove_vm_area

{% highlight bash %}
struct vm_struct *remove_vm_area(void *addr)
  作用: 释放一段 VMALLOC 虚拟内存到 VMALLOC 内存分配器.
{% endhighlight %}

###### vfree

{% highlight bash %}
void vfree(void *addr)
  作用: 释放一段虚拟内存到 VMALLOC 分配器.
{% endhighlight %}

###### \_\_vmalloc

{% highlight bash %}
void *__vmalloc(unsigned long size, gfp_t gfp_mask, pgprot_t prot)
  作用: 从 VMALLOC 分配器中分配指定长度的 VMALLOC 虚拟内存.
{% endhighlight %}

###### vmalloc

{% highlight bash %}
void *vmalloc(unsigned long size)
  作用: 从 VMALLOC 分配器中分配 VMALLOC 虚拟内存.
{% endhighlight %}

###### vmalloc_32

{% highlight bash %}
void *vmalloc_32(unsigned long size)
  作用: 分配一段 32 位的 VMALLOC 虚拟内存.
{% endhighlight %}

###### \_\_vmalloc_area

{% highlight bash %}
void *__vmalloc_area(struct vm_struct *area, gfp_t gfp_mask, pgprot_t prot)
  作用: VMALLOC 内存分配器从 VMALLOC 区域分配一段虚拟内存.
{% endhighlight %}

###### \_\_vmalloc_area_node

{% highlight bash %}
void *__vmalloc_area_node(struct vm_struct *area, gfp_t gfp_mask,
                                pgprot_t prot, int node)
  作用: VMALLOC 分配器从指定节点上分配一段 VMALLOC 虚拟内存块.
{% endhighlight %}

###### vmalloc_exec

{% highlight bash %}
void *vmalloc_exec(unsigned long size)
  作用: 分配一段可以执行的 VMALLOC 虚拟内存.
{% endhighlight %}

###### \_\_vmalloc_node

{% highlight bash %}
void *__vmalloc_node(unsigned long size, gfp_t gfp_mask, pgprot_t prot, int node)
  作用: VMALLOC 分配器从指定节点上分配 VMALLOC 虚拟内存.
{% endhighlight %}

###### vmalloc_node

{% highlight bash %}
void *vmalloc_node(unsigned long size, int node)
  作用: VMALLOC 分配器从指定节点上分配 VMALLOC 虚拟内存.
{% endhighlight %}

###### vmap

{% highlight bash %}
void *vmap(struct page **pages, unsigned int count,
                unsigned long flags, pgprot_t prot)
  作用: 将一段 VMALLOC 虚拟内存与物理内存进行映射.
{% endhighlight %}

###### vmap_pmd_range

{% highlight bash %}
static inline int vmap_pmd_range(pud_t *pud, unsigned long addr,
                        unsigned long end, pgprot_t prot, struct page ***pages)
  作用: 建立一段 VMALLOC 虚拟内存的 PMD 页表.
{% endhighlight %}

###### vmap_pte_range

{% highlight bash %}
static int vmap_pte_range(pmd_t *pmd, unsigned long addr,
                        unsigned long end, pgprot_t prot, struct page ***pages)
  作用: 建立一段 VMALLOC 虚拟内存的 PTE 页表.
{% endhighlight %}

###### vmap_pud_range

{% highlight bash %}
static inline int vmap_pud_range(pgd_t *pgd, unsigned long addr,
                        unsigned long end, pgprot_t prot, struct page ***pages)
  作用: 建立一段 VMALLOC 虚拟内存的 PUD 页表.
{% endhighlight %}

###### \_\_vunmap

{% highlight bash %}
void __vunmap(void *addr, int deallocate_pages)
  作用: 解除一段 VMALLOC 虚拟内存的映射关系.
{% endhighlight %}

###### vunmap

{% highlight bash %}
void vunmap(void *addr)
  作用: 解除一段 VMALLOC 虚拟内存的映射关系.
{% endhighlight %}

###### vunmap_pmd_range

{% highlight bash %}
static inline void vunmap_pmd_range(pud_t *pud, unsigned long addr,
                                                unsigned long end)
  作用: 解除一段 VMALLOC 虚拟内存的 PMD 页表.
{% endhighlight %}

###### vunmap_pte_range

{% highlight bash %}
static void vunmap_pte_range(pmd_t *pmd, unsigned long addr, unsigned long end)
  作用: 解除一段 VMALLOC 虚拟内存的 PTE 页表.
{% endhighlight %}

###### vunmap_pud_range

{% highlight bash %}
static inline void vunmap_pud_range(pgd_t *pgd, unsigned long addr,
                                                unsigned long end)
  作用: 解除一段 VMALLOC 虚拟内存的 PUD 页表.
{% endhighlight %}

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND000100.png)

----------------------------------

<span id="F"></span>

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND00000K.jpg)

#### VMALLOC 进阶研究

> - [用户空间实现一个 VMALLOC 内存分配器](https://biscuitos.github.io/blog/Memory-Userspace/#M)

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND000100.png)

-----------------------------------------------

<span id="E"></span>

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND00000L.jpg)

#### VMALLOC 内存分配器调试

> - [BiscuitOS VMALLOC 内存分配器调试](#C0004)

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

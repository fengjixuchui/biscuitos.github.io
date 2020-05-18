---
layout: post
title:  "SLAB Allocator"
date:   2020-05-07 09:39:30 +0800
categories: [HW]
excerpt: MMU.
tags:
  - MMU
---

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND00000L0.PNG)

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI100100.png)

#### 目录

> - [SLAB 分配器原理](#A)
>
> - [SLAB 分配器使用](#B)
>
> - [SLAB 分配器实践](#C)
>
> - [SLAB 源码分析](#D)
>
> - [SLAB 分配器调试](#E)
>
> - [SLAB 分配进阶研究](#F)
>
> - [SLAB 时间轴](#G)
>
> - [SLAB 历史补丁](#H)
>
> - [SLAB API](#K)
>
> - [附录/捐赠](#Z0)

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND000100.png)

----------------------------------

<span id="A"></span>

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND00000T.jpg)

#### SLAB 分配器原理

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/HK000226.png)

在 Linux 初始化过程中，将除了 ZONE_HIGHMEM 分区之外所有分区的物理页都与
Linux 内核虚拟地址进行了线性映射，所谓线性映射就是将大块虚拟内存与大块
物理内存进行映射，以形成连续的虚拟地址映射到连续的物理地址上，因此称这个
虚拟内存区域为线性映射区。Linux 系统初始化阶段便建立好了线性区的映射关系后，
映射关系不再改变，因此不需要在建立和撤销页表查找。于是只需简单的线性转换
就可以在虚拟地址和物理地址之间转换，这个开发者一个错觉，觉得线性区的虚拟
地址是不需要建立页表的，这样的结论是不正确的。

Buddy 内存管理器用于管理物理内存的分配，其以 PAGE_SIZE 为单位进行分配。
对于建立线性映射的物理内存来说，申请者只需从 Buddy 内存分配器分配物理
内存之后，通过一个简单的线性转换就可以获得物理内存对应的虚拟内存，调用
者就可以直接使用该虚拟内存，但这样的操作逻辑不代表 Buddy 内存管理器能够
管理线性区虚拟内存的分配和释放，开发者应该明确区分这个概念。

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI000998.png)

Buddy 内存管理器只能按 PAGE_SIZE 分配物理内存，这样导致获得的虚拟内存大小
总是按 PAGE_SIZE 的倍数。如果请求者正好需要 PAGE_SIZE 为倍数的虚拟内存，
那么不存在什么问题，但内核经常分配长度很小的虚拟内存，例如几个字节到几百个
字节，如果分配几个字节，系统就传递一个 PAGE_SIZE 的虚拟内存给申请者，那么
申请者就会浪费很多内存，此时其他进程或线程也不能使用这些浪费的内存。因此
SLAB 内存分配器就产生了。

SLAB 内存分配器用于管理、分配和释放小块虚拟内存

---------------------------------

###### SLAB 的优点

虚拟地址是固定的，对应永久映射的部分只要建立映射，就可以永久使用.

###### SLAB 的缺点

空间有限.

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND000100.png)

---------------------------------

<span id="B"></span>

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND00000H.jpg)

#### SLAB 分配器使用

> - [基础用法介绍](#B0000)
>
> - [SLAB 永久映射使用](#B0001)
>
> - [SLAB 临时映射使用](#B0002)

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND000100.png)

-----------------------------------------------

#### <span id="B0000">基础用法介绍</span>

SLAB 分配器提供了用于分配和释放虚拟内存的相关函数接口:

###### SLAB 分配

{% highlight bash %}
set_fixmap_nocache
set_fixmap
kmap_atomic
{% endhighlight %}

###### SLAB 释放

{% highlight bash %}
clear_fixmap
kunmap_atomic
{% endhighlight %}

###### 转换函数

{% highlight bash %}
__fix_to_virt
__virt_to_fix
fix_to_virt
virt_to_fix
{% endhighlight %}

具体函数解析说明，请查看:

> - [SLAB API](#K)

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND000100.png)

----------------------------------------------------

#### <span id="B0001">SLAB 永久映射使用</span>

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI000984.png)

从 SLAB 区域内存分配、使用和释放虚拟内存，开发者可以参考如下代码:

{% highlight c %}
#include <linux/mm.h>
#include <linux/highmem.h>
#include <asm/fixmap.h>

/* FIX Phys Addr */
#define FIX_APIC_ADDR 0xffe00000

static int TestCase_kmap(void)
{
	unsigned long apic_virt = fix_to_virt(FIX_APIC_BASE);
	unsigned long val;

	/* SLAB */
	set_fixmap_nocache(apic_virt, FIX_APIC_ADDR);

	/* Read/Write */
	val = *apic_virt;

        return 0;
}
{% endhighlight %}

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND000100.png)

----------------------------------------------------

#### <span id="B0002">SLAB 临时映射使用</span>

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI000985.png)

从 SLAB 区域内存分配、使用和释放虚拟内存，开发者可以参考如下代码:

{% highlight c %}
#include <linux/mm.h>
#include <linux/highmem.h>
#include <asm/fixmap.h>

static int TestCase_fixmap(void)
{
        struct page *page;
        void *addr;

        /* alloc */
        page = alloc_page(__GFP_HIGHMEM);
        if (!page || !PageHighMem(page)) {
                printk("%s alloc_page() failed.\n", __func__);
                return -ENOMEM;
        }

        /* Fixmap */
	addr = kmap_atomic(page, KM_USER0);

        sprintf((char *)addr, "BiscuitOS-%s", __func__);
        printk("[%#lx] %s\n", (unsigned long)addr, (char *)addr);

	/* Unmap */
	kunmap_atomic(addr, KM_USER0);
        __free_page(page);
        return 0;
}
{% endhighlight %}

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND000100.png)


------------------------------------------------

<span id="C"></span>

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND00000I.jpg)

#### SLAB 分配器实践

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
支持多个版本的 SLAB，开发者可以根据需求进行选择，本文以 linux 2.6.12 
版本的 SLAB 进行讲解。开发者使用如下命令:

{% highlight bash %}
cd BiscuitOS/
make linux-5.0-arm32_defconfig
make menuconfig
{% endhighlight %}

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI000746.png)

选择并进入 "[\*] Package  --->" 目录。

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI000747.png)

选择并进入 "[\*]   Memory Development History  --->" 目录。

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI000986.png)

选择并进入 "[\*]   SLAB Allocator  --->" 目录。

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI000987.png)

选择 "[\*]   SLAB on linux 2.6.12  --->" 目录，保存并退出。接着执行如下命令:

{% highlight bash %}
make
{% endhighlight %}

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI000750.png)

成功之后将出现上图的内容，接下来开发者执行如下命令以便切换到项目的路径:

{% highlight bash %}
cd BiscuitOS/output/linux-5.0-arm32/package/BiscuitOS_SLAB-2.6.12
make download
{% endhighlight %}

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI000988.png)

至此源码已经下载完成，开发者可以使用 tree 等工具查看源码:

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI000989.png)

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
cd BiscuitOS/output/linux-5.0-arm32/package/BiscuitOS_SLAB-2.6.12
make kernel
{% endhighlight %}

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI000990.png)

--------------------------------------------

#### <span id="C0002">实践执行</span>

环境部署完毕之后，开发者可以向通用模块一样对源码进行编译和安装使用，使用
如下命令:

{% highlight bash %}
cd BiscuitOS/output/linux-5.0-arm32/package/BiscuitOS_SLAB-2.6.12
make
{% endhighlight %}

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI000991.png)

以上就是模块成功编译，接下来将 ko 模块安装到 BiscuitOS 中，使用如下命令:

{% highlight bash %}
cd BiscuitOS/output/linux-5.0-arm32/package/BiscuitOS_SLAB-2.6.12
make install
make pack
{% endhighlight %}

以上准备完毕之后，最后就是在 BiscuitOS 运行这个模块了，使用如下命令:

{% highlight bash %}
cd BiscuitOS/output/linux-5.0-arm32/package/BiscuitOS_SLAB-2.6.12
make run
{% endhighlight %}

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI000992.png)

在 BiscuitOS 中插入了模块 "BiscuitOS_SLAB-2.6.12.ko"，打印如上信息，那么
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
/xspace/OpenSource/BiscuitOS/BiscuitOS/output/linux-5.0-arm32/package/BiscuitOS_SLAB-2.6.12/BiscuitOS_SLAB-2.6.12/Makefile
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

然后先向 BiscuitOS 中插入 "BiscuitOS_SLAB-2.6.12.ko" 模块，然后再插入
"BiscuitOS_SLAB-2.6.12-buddy.ko" 模块。如下:

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI000773.png)

以上便是测试代码的使用办法。开发者如果想在源码中启用或关闭某些宏，可以
修改 Makefile 中内容:

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI000774.png)

从上图可以知道，如果要启用某些宏，可以在 ccflags-y 中添加 "-D" 选项进行
启用，源码的编译参数也可以添加到 ccflags-y 中去。开发者除了使用上面的办法
进行测试之外，也可以使用项目提供的 initcall 机制进行调试，具体请参考:

> - [Initcall 机制调试说明](#C00032)

Initcall 机制提供了以下函数用于 SLAB 调试:

{% highlight bash %}
fixmap_initcall_bs()
{% endhighlight %}

从项目的 Initcall 机制可以知道，fixmap_initcall_bs() 调用的函数将
在 SLAB 分配器初始化完毕之后自动调用。SLAB 相关的测试代码位于:

{% highlight bash %}
BiscuitOS/output/linux-5.0-arm32/package/BiscuitOS_SLAB-2.6.12/BiscuitOS_SLAB-2.6.12/module/fixmap/
{% endhighlight %}

在 Makefile 中打开调试开关:

{% highlight bash %}
$(MODULE_NAME)-m                += modules/fixmap/main.o
{% endhighlight %}

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND000100.png)

----------------------------------

<span id="H"></span>

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND00000T.jpg)

#### SLAB 历史补丁

> - [SLAB Linux 2.6.12](#H-linux-2.6.12)
>
> - [SLAB Linux 2.6.12.1](#H-linux-2.6.12.1)
>
> - [SLAB Linux 2.6.12.2](#H-linux-2.6.12.2)
>
> - [SLAB Linux 2.6.12.3](#H-linux-2.6.12.3)
>
> - [SLAB Linux 2.6.12.4](#H-linux-2.6.12.4)
>
> - [SLAB Linux 2.6.12.5](#H-linux-2.6.12.5)
>
> - [SLAB Linux 2.6.12.6](#H-linux-2.6.12.6)
>
> - [SLAB Linux 2.6.13](#H-linux-2.6.13)
>
> - [SLAB Linux 2.6.13.1](#H-linux-2.6.13.1)
>
> - [SLAB Linux 2.6.14](#H-linux-2.6.14)
>
> - [SLAB Linux 2.6.15](#H-linux-2.6.15)

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND000100.png)

----------------------------------------------

<span id="H-linux-2.6.12"></span>

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI000785.JPG)

#### SLAB Linux 2.6.12

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI000984.png)

Linux 2.6.12 采用 SLAB 分配器管理 SLAB 虚拟内存区域。

###### SLAB 分配

{% highlight bash %}
set_fixmap_nocache
set_fixmap
kmap_atomic
{% endhighlight %}

###### SLAB 释放

{% highlight bash %}
clear_fixmap
kunmap_atomic
{% endhighlight %}

###### 转换函数

{% highlight bash %}
__fix_to_virt
__virt_to_fix
fix_to_virt
virt_to_fix
{% endhighlight %}

具体函数解析说明，请查看:

> - [SLAB API](#K)

###### 与项目相关

SLAB 内存分配器与本项目相关的 kmap_atomic/kunmap_atomic 调用顺序如下:

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI000993.png)

项目中虚拟内存布局如下:

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI000737.png)

在项目中，SLAB 虚拟内存的管理的范围是: 0x96395000 到 0x963FF000. 

###### 补丁

对于 Linux 2.6.12 的补丁，Linus 将 Linux 内核源码树加入到 git 中来，因此
这个版本的代码均不产生补丁。更多补丁的使用请参考:

> - [BiscuitOS Memory Manager Patch 建议](https://biscuitos.github.io/blog/HISTORY-MMU/#C00033)

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND000100.png)

----------------------------------------------

<span id="H-linux-2.6.12.1"></span>

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI000786.JPG)

#### SLAB Linux 2.6.12.1

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI000984.png)

Linux 2.6.12.1 采用 SLAB 分配器管理 SLAB 虚拟内存区域。

###### SLAB 分配

{% highlight bash %}
set_fixmap_nocache
set_fixmap
kmap_atomic
{% endhighlight %}

###### SLAB 释放

{% highlight bash %}
clear_fixmap
kunmap_atomic
{% endhighlight %}

###### 转换函数

{% highlight bash %}
__fix_to_virt
__virt_to_fix
fix_to_virt
virt_to_fix
{% endhighlight %}

具体函数解析说明，请查看:

> - [SLAB API](#K)

###### 与项目相关

SLAB 内存分配器与本项目相关的 kmap_atomic/kunmap_atomic 调用顺序如下:

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI000993.png)

项目中虚拟内存布局如下:

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI000737.png)

在项目中，SLAB 虚拟内存的管理的范围是: 0x96395000 到 0x963FF000.

###### 补丁

相对于前一个版本 linux 2.6.12, 该版本并未产生补丁。更多补丁的使用请参考:

> - [BiscuitOS Memory Manager Patch 建议](https://biscuitos.github.io/blog/HISTORY-MMU/#C00033)

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND000100.png)

----------------------------------------------

<span id="H-linux-2.6.12.2"></span>

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI000787.JPG)

#### SLAB Linux 2.6.12.2

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI000984.png)

Linux 2.6.12.2 采用 SLAB 分配器管理 SLAB 虚拟内存区域。

###### SLAB 分配

{% highlight bash %}
set_fixmap_nocache
set_fixmap
kmap_atomic
{% endhighlight %}

###### SLAB 释放

{% highlight bash %}
clear_fixmap
kunmap_atomic
{% endhighlight %}

###### 转换函数

{% highlight bash %}
__fix_to_virt
__virt_to_fix
fix_to_virt
virt_to_fix
{% endhighlight %}

具体函数解析说明，请查看:

> - [SLAB API](#K)

###### 与项目相关

SLAB 内存分配器与本项目相关的 kmap_atomic/kunmap_atomic 调用顺序如下:

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI000993.png)

项目中虚拟内存布局如下:

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI000737.png)

在项目中，SLAB 虚拟内存的管理的范围是: 0x96395000 到 0x963FF000.

###### 补丁

相对于前一个版本 linux 2.6.12.1, 该版本并未产生补丁。更多补丁的使用请参考:

> - [BiscuitOS Memory Manager Patch 建议](https://biscuitos.github.io/blog/HISTORY-MMU/#C00033)

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND000100.png)

----------------------------------------------

<span id="H-linux-2.6.12.3"></span>

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI000788.JPG)

#### SLAB Linux 2.6.12.3

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI000984.png)

Linux 2.6.12.3 采用 SLAB 分配器管理 SLAB 虚拟内存区域。

###### SLAB 分配

{% highlight bash %}
set_fixmap_nocache
set_fixmap
kmap_atomic
{% endhighlight %}

###### SLAB 释放

{% highlight bash %}
clear_fixmap
kunmap_atomic
{% endhighlight %}

###### 转换函数

{% highlight bash %}
__fix_to_virt
__virt_to_fix
fix_to_virt
virt_to_fix
{% endhighlight %}

具体函数解析说明，请查看:

> - [SLAB API](#K)

###### 与项目相关

SLAB 内存分配器与本项目相关的 kmap_atomic/kunmap_atomic 调用顺序如下:

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI000993.png)

项目中虚拟内存布局如下:

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI000737.png)

在项目中，SLAB 虚拟内存的管理的范围是: 0x96395000 到 0x963FF000.

###### 补丁

相对于前一个版本 linux 2.6.12.2, 该版本并未产生补丁。更多补丁的使用请参考:

> - [BiscuitOS Memory Manager Patch 建议](https://biscuitos.github.io/blog/HISTORY-MMU/#C00033)

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND000100.png)

----------------------------------------------

<span id="H-linux-2.6.12.4"></span>

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI000789.JPG)

#### SLAB Linux 2.6.12.4

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI000984.png)

Linux 2.6.12.4 采用 SLAB 分配器管理 SLAB 虚拟内存区域。

###### SLAB 分配

{% highlight bash %}
set_fixmap_nocache
set_fixmap
kmap_atomic
{% endhighlight %}

###### SLAB 释放

{% highlight bash %}
clear_fixmap
kunmap_atomic
{% endhighlight %}

###### 转换函数

{% highlight bash %}
__fix_to_virt
__virt_to_fix
fix_to_virt
virt_to_fix
{% endhighlight %}

具体函数解析说明，请查看:

> - [SLAB API](#K)

###### 与项目相关

SLAB 内存分配器与本项目相关的 kmap_atomic/kunmap_atomic 调用顺序如下:

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI000993.png)

项目中虚拟内存布局如下:

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI000737.png)

在项目中，SLAB 虚拟内存的管理的范围是: 0x96395000 到 0x963FF000.

###### 补丁

相对于前一个版本 linux 2.6.12.3, 该版本并未产生补丁。更多补丁的使用请参考:

> - [BiscuitOS Memory Manager Patch 建议](https://biscuitos.github.io/blog/HISTORY-MMU/#C00033)

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND000100.png)

----------------------------------------------

<span id="H-linux-2.6.12.5"></span>

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI000790.JPG)

#### SLAB Linux 2.6.12.5

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI000984.png)

Linux 2.6.12.5 采用 SLAB 分配器管理 SLAB 虚拟内存区域。

###### SLAB 分配

{% highlight bash %}
set_fixmap_nocache
set_fixmap
kmap_atomic
{% endhighlight %}

###### SLAB 释放

{% highlight bash %}
clear_fixmap
kunmap_atomic
{% endhighlight %}

###### 转换函数

{% highlight bash %}
__fix_to_virt
__virt_to_fix
fix_to_virt
virt_to_fix
{% endhighlight %}

具体函数解析说明，请查看:

> - [SLAB API](#K)

###### 与项目相关

SLAB 内存分配器与本项目相关的 kmap_atomic/kunmap_atomic 调用顺序如下:

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI000993.png)

项目中虚拟内存布局如下:

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI000737.png)

在项目中，SLAB 虚拟内存的管理的范围是: 0x96395000 到 0x963FF000.

###### 补丁

相对于前一个版本 linux 2.6.12.4, 该版本并未产生补丁。更多补丁的使用请参考:

> - [BiscuitOS Memory Manager Patch 建议](https://biscuitos.github.io/blog/HISTORY-MMU/#C00033)

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND000100.png)

----------------------------------------------

<span id="H-linux-2.6.12.6"></span>

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI000791.JPG)

#### SLAB Linux 2.6.12.6

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI000984.png)

Linux 2.6.12.6 采用 SLAB 分配器管理 SLAB 虚拟内存区域。

###### SLAB 分配

{% highlight bash %}
set_fixmap_nocache
set_fixmap
kmap_atomic
{% endhighlight %}

###### SLAB 释放

{% highlight bash %}
clear_fixmap
kunmap_atomic
{% endhighlight %}

###### 转换函数

{% highlight bash %}
__fix_to_virt
__virt_to_fix
fix_to_virt
virt_to_fix
{% endhighlight %}

具体函数解析说明，请查看:

> - [SLAB API](#K)

###### 与项目相关

SLAB 内存分配器与本项目相关的 kmap_atomic/kunmap_atomic 调用顺序如下:

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI000993.png)

项目中虚拟内存布局如下:

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI000737.png)

在项目中，SLAB 虚拟内存的管理的范围是: 0x96395000 到 0x963FF000.

###### 补丁

相对于前一个版本 linux 2.6.12.5, 该版本并未产生补丁。更多补丁的使用请参考:

> - [BiscuitOS Memory Manager Patch 建议](https://biscuitos.github.io/blog/HISTORY-MMU/#C00033)

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND000100.png)

----------------------------------------------

<span id="H-linux-2.6.13"></span>

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI000792.JPG)

#### SLAB Linux 2.6.13

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI000984.png)

Linux 2.6.13 采用 SLAB 分配器管理 SLAB 虚拟内存区域。

###### SLAB 分配

{% highlight bash %}
set_fixmap_nocache
set_fixmap
kmap_atomic
{% endhighlight %}

###### SLAB 释放

{% highlight bash %}
clear_fixmap
kunmap_atomic
{% endhighlight %}

###### 转换函数

{% highlight bash %}
__fix_to_virt
__virt_to_fix
fix_to_virt
virt_to_fix
{% endhighlight %}

具体函数解析说明，请查看:

> - [SLAB API](#K)

###### 与项目相关

SLAB 内存分配器与本项目相关的 kmap_atomic/kunmap_atomic 调用顺序如下:

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI000993.png)

项目中虚拟内存布局如下:

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI000737.png)

在项目中，SLAB 虚拟内存的管理的范围是: 0x96395000 到 0x963FF000.

###### 补丁

相对于前一个版本 linux 2.6.12.6, 该版本并产生一个补丁。

{% highlight bash %}
tig mm/highmem.c include/linux/highmem.h include/asm-i386/highmem.h arch/i386/mm/highmem.c

2005-06-23 00:08 Alexey Dobriyan   o [PATCH] Remove i386_ksyms.c, almost.
                                     [main] 129f69465b411592247c408f93d7106939223be1
2005-06-25 14:58 Vivek Goyal       o [PATCH] kdump: Routines for copying dump pages
                                     [main] 60e64d46a58236e3c718074372cab6a5b56a3b15
{% endhighlight %}

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI000994.png)

{% highlight bash %}
git format-patch -1 129f69465b411592247c408f93d7106939223be1
vi 0001-PATCH-Remove-i386_ksyms.c-almost.patch
{% endhighlight %}

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI000995.png)

该补丁用于导出 kmap_atomic/kunmap_atomic 函数.

{% highlight bash %}
git format-patch -1 60e64d46a58236e3c718074372cab6a5b56a3b15
vi 0001-PATCH-kdump-Routines-for-copying-dump-pages.patch
{% endhighlight %}

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI000996.png)

该补丁增加了 kmap_atomic_pfn() 函数实现. 更多补丁的使用请参考:

> - [BiscuitOS Memory Manager Patch 建议](https://biscuitos.github.io/blog/HISTORY-MMU/#C00033)

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND000100.png)

----------------------------------------------

<span id="H-linux-2.6.13.1"></span>

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI000793.JPG)

#### SLAB Linux 2.6.13.1

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI000984.png)

Linux 2.6.13.1 采用 SLAB 分配器管理 SLAB 虚拟内存区域。

###### SLAB 分配

{% highlight bash %}
set_fixmap_nocache
set_fixmap
kmap_atomic
{% endhighlight %}

###### SLAB 释放

{% highlight bash %}
clear_fixmap
kunmap_atomic
{% endhighlight %}

###### 转换函数

{% highlight bash %}
__fix_to_virt
__virt_to_fix
fix_to_virt
virt_to_fix
{% endhighlight %}

具体函数解析说明，请查看:

> - [SLAB API](#K)

###### 与项目相关

SLAB 内存分配器与本项目相关的 kmap_atomic/kunmap_atomic 调用顺序如下:

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI000993.png)

项目中虚拟内存布局如下:

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI000737.png)

在项目中，SLAB 虚拟内存的管理的范围是: 0x96395000 到 0x963FF000.

###### 补丁

相对于前一个版本 linux 2.6.13, 该版本并未产生补丁。更多补丁的使用请参考:

> - [BiscuitOS Memory Manager Patch 建议](https://biscuitos.github.io/blog/HISTORY-MMU/#C00033)

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND000100.png)

----------------------------------------------

<span id="H-linux-2.6.14"></span>

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI000794.JPG)

#### SLAB Linux 2.6.14

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI000984.png)

Linux 2.6.14 采用 SLAB 分配器管理 SLAB 虚拟内存区域。

###### SLAB 分配

{% highlight bash %}
set_fixmap_nocache
set_fixmap
kmap_atomic
{% endhighlight %}

###### SLAB 释放

{% highlight bash %}
clear_fixmap
kunmap_atomic
{% endhighlight %}

###### 转换函数

{% highlight bash %}
__fix_to_virt
__virt_to_fix
fix_to_virt
virt_to_fix
{% endhighlight %}

具体函数解析说明，请查看:

> - [SLAB API](#K)

###### 与项目相关

SLAB 内存分配器与本项目相关的 kmap_atomic/kunmap_atomic 调用顺序如下:

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI000993.png)

项目中虚拟内存布局如下:

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI000737.png)

在项目中，SLAB 虚拟内存的管理的范围是: 0x96395000 到 0x963FF000.

###### 补丁

相对于前一个版本 linux 2.6.13.1, 该版本并未产生补丁。更多补丁的使用请参考:

> - [BiscuitOS Memory Manager Patch 建议](https://biscuitos.github.io/blog/HISTORY-MMU/#C00033)

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND000100.png)

----------------------------------------------

<span id="H-linux-2.6.15"></span>

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI000795.JPG)

#### SLAB Linux 2.6.15

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI000984.png)

Linux 2.6.15 采用 SLAB 分配器管理 SLAB 虚拟内存区域。

###### SLAB 分配

{% highlight bash %}
set_fixmap_nocache
set_fixmap
kmap_atomic
{% endhighlight %}

###### SLAB 释放

{% highlight bash %}
clear_fixmap
kunmap_atomic
{% endhighlight %}

###### 转换函数

{% highlight bash %}
__fix_to_virt
__virt_to_fix
fix_to_virt
virt_to_fix
{% endhighlight %}

具体函数解析说明，请查看:

> - [SLAB API](#K)

###### 与项目相关

SLAB 内存分配器与本项目相关的 kmap_atomic/kunmap_atomic 调用顺序如下:

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI000993.png)

项目中虚拟内存布局如下:

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI000737.png)

在项目中，SLAB 虚拟内存的管理的范围是: 0x96395000 到 0x963FF000.

###### 补丁

相对于前一个版本 linux 2.6.14, 该版本并未产生补丁。更多补丁的使用请参考:

> - [BiscuitOS Memory Manager Patch 建议](https://biscuitos.github.io/blog/HISTORY-MMU/#C00033)

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND000100.png)

-----------------------------------------------

<span id="G"></span>

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND00000H.jpg)

#### SLAB 历史时间轴

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI000997.png)

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND000100.png)

-----------------------------------------------

<span id="K"></span>

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND00000B.jpg)

#### SLAB API

###### clear_fixmap

{% highlight bash %}
#define clear_fixmap(idx) \
                __set_fixmap(idx, 0, __pgprot(0))
  作用: 清除 SLAB 内存区上的一个固定映射.
{% endhighlight %}

###### \_\_fix_to_virt

{% highlight bash %}
#define __fix_to_virt(x)        (FIXADDR_TOP - ((x) << PAGE_SHIFT))
  作用: 将 SLAB 索引转换为虚拟地址.
{% endhighlight %}

###### fix_to_virt

{% highlight bash %}
static __always_inline unsigned long fix_to_virt(const unsigned int idx)
  作用: 将 SLAB 索引转换为虚拟地址.
{% endhighlight %}

###### kmap_atomic

{% highlight bash %}
void *kmap_atomic(struct page *page, enum km_type type)
  作用: 将物理页映射到 SLAB 的临时映射区.
{% endhighlight %}

###### kmap_atomic_to_page

{% highlight bash %}
struct page *kmap_atomic_to_page(void *ptr)
  作用: 获得 SLAB 虚拟内存对应的物理页.
{% endhighlight %}

###### kmap_atomic_pfn

{% highlight bash %}
void *kmap_atomic_pfn(unsigned long pfn, enum km_type type)
  作用: 将 PFN 对应的物理页映射到 KMAP 的临时虚拟内存上.
{% endhighlight %}

###### kunmap_atomic

{% highlight bash %}
void kunmap_atomic(void *kvaddr, enum km_type type)
  作用: 解除 SLAB 临时虚拟内存与物理页之间的映射关系.
{% endhighlight %}

###### set_fixmap

{% highlight bash %}
#define set_fixmap(idx, phys) \
                __set_fixmap(idx, phys, PAGE_KERNEL)
  作用: 建立 SLAB 虚拟内存区的一个固定映射.
{% endhighlight %}

###### set_fixmap_nocache

{% highlight bash %}
#define set_fixmap_nocache(idx, phys) \
                __set_fixmap(idx, phys, PAGE_KERNEL_NOCACHE)
  作用: 建立 SLAB 虚拟内存区一个不带 cache 的固定映射.
{% endhighlight %}

###### \_\_virt_to_fix

{% highlight bash %}
#define __virt_to_fix(x)        ((FIXADDR_TOP - ((x)&PAGE_MASK)) >> PAGE_SHIFT)
  作用: 将虚拟地址转换为 SLAB 索引.
{% endhighlight %}

###### virt_to_fix

{% highlight bash %}
static inline unsigned long virt_to_fix(const unsigned long vaddr)
  作用: 将虚拟地址转换为 SLAB 索引.
{% endhighlight %}

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND000100.png)

----------------------------------

<span id="F"></span>

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND00000K.jpg)

#### SLAB 进阶研究

> - [用户空间实现一个 SLAB 内存分配器](https://biscuitos.github.io/blog/Memory-Userspace/#N)

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND000100.png)

-----------------------------------------------

<span id="E"></span>

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND00000L.jpg)

#### SLAB 内存分配器调试

> - [BiscuitOS SLAB 内存分配器调试](#C0004)

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

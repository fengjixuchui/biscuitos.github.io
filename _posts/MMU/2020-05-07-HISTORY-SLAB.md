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

----------------------------------

<span id="D"></span>

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND00000I.jpg)

#### SLAB 源码分析

> - [Linux 2.6.34](#D0)
>
> - [Linux 3.0 (Filling...)](#D1)
>
> - [Linux 4.1 (Filling...)](#D2)
>
> - [Linux 5.0 (Filling...)](#D3)

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND000100.png)

----------------------------------------------

<span id="D0"></span>

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND00000Z.jpg)

#### Linux 2.6.34

> - [逻辑解析](#D02)
>
> - 数据结构
>
>   - [struct array_cache](#D0101)
>
>   - [struct cache_names](#D0105)
>
>   - [struct cache_sizes](#D0104)
>
>   - [struct ccupdate_struct](#D0108)
>
>   - [struct kmem_cache](#D0103)
>
>   - [struct kmem_list3](#D0100)
>
>   - [struct slab](#D0102)
>
>   - [kmem_bufctl_t](#D0106)
>
>   - [struct arraycache_init](#D0107)
>
> - 函数解析
>
>   - [alloc_arraycache](#D030031)
>
>   - [alloc_kmemlist](#D030032)
>
>   - [alloc_slabmgmt](#D030009)
>
>   - [\_\_\_\_cache_alloc](#D030018)
>
>   - [\_\_cache_alloc](#D030020)
>
>   - [cache_alloc_refill](#D030017)
>
>   - [cache_estimate](#D030002)
>
>   - [cache_flusharray](#D030046)
>
>   - [\_\_cache_free](#D030045)
>
>   - [cache_grow](#D030016)
>
>   - [cache_init_objs](#D030015)
>
>   - [calculate_slab_order](#D030023)
>
>   - [cpu_cache_get](#D030004)
>
>   - [drain_freelist](#D030056)
>
>   - [\_\_do_cache_alloc](#D030019)
>
>   - [do_ccupdate_local](#D030034)
>
>   - [do_drain](#D030058)
>
>   - [\_\_do_kmalloc](#D030026)
>
>   - [do_tune_cpucache](#D030033)
>
>   - [drain_array](#D030059)
>
>   - [enable_cpucache](#D030036)
>
>   - [\_\_find_general_cachep](#D030024)
>
>   - [free_block](#D030053)
>
>   - [index_to_obj](#D030006)
>
>   - [init_list](#D030041)
>
>   - [kfree](#D030055)
>
>   - [\_\_kmalloc](#D030027)
>
>   - [kmalloc](#D030028)
>
>   - [kmalloc_node](#D030030)
>
>   - [kmem_cache_alloc](#D030021)
>
>   - [kmem_cache_alloc_notrace](#D030029)
>
>   - [kmem_cache_create](#D030039)
>
>   - [\_\_kmem_cache_destroy](#D030057)
>
>   - [kmem_cache_free](#D030051)
>
>   - [kmem_cache_init](#D030040)
>
>   - [kmem_cache_init_late](#D030044)
>
>   - [kmem_cache_zalloc](#D030022)
>
>   - [kmem_find_general_cachep](#D030025)
>
>   - [kmem_freepages](#D030049)
>
>   - [kmem_getpages](#D030008)
>
>   - [kmem_list3_init](#D030000)
>
>   - [kmem_rcu_free](#D030050)
>
>   - [kzalloc](#D030035)
>
>   - [MAKE_ALL_LIST](#D030043)
>
>   - [MAKE_LIST](#D030042)
>
>   - [obj_to_index](#D030048)
>
>   - [page_get_cache](#D030011)
>
>   - [page_get_slab](#D030012)
>
>   - [page_set_cache](#D030010)
>
>   - [page_set_slab](#D030013)
>
>   - [set_up_list3s](#D030001)
>
>   - [setup_cpu_cache](#D030037)
>
>   - [slab_bufctl](#D030007)
>
>   - [slab_destroy](#D030052)
>
>   - [slab_get_obj](#D030005)
>
>   - [slab_is_available](#D030038)
>
>   - [slab_map_pages](#D030014)
>
>   - [slab_mgmt_size](#D030003)
>
>   - [slab_put_obj](#D030047)
>
>   - [virt_to_slab](#D030054)

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND000100.png)

-----------------------------------------

###### <span id="D030000"></span>

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/TB.png)


![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND000100.png)

-----------------------------------------

###### <span id="D030059">drain_array</span>

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/TB000099.png)

drain_array() 函数用于从本地高速缓存上释放一定数量的缓存对象。参数 cachep 指向
高速缓存，l3 指向高速缓存的 slab 链表，ac 参数指向本地高速缓存，force 参数指明
是否全部释放, node 指明 NODE 信息.

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/TB000001.png)

高速缓存针对每个 CPU 设置了一个本地高速缓存，本地高速缓存通过一个缓存栈维护
一定数量的可用缓存对象，drain_array() 函数用于从这个缓存栈上释放一定数量的
可用缓存对象回 slab. 函数首先确认本地高速缓存存在以及缓存栈上维护一定数量的
缓存对象，接着函数如果探测到本地高速缓存已经被使用过，且 force 参数为 0，那么
函数仅仅将本地高速缓存的 touched 设置为 0; 反之那么函数检测到缓存栈上是否一定
数量的可用缓存对象，那么函数在 4019 行确认需要释放缓存对象的数量，如果 force
为真，那么将缓存栈上的所有缓存对象释放回 slab，反之将上限的 30% 返回到 slab。
如果返回的数量大于缓存栈上可用缓存对象的数量，那么返回缓存栈上 50% 的可用缓存
对象到 slab。于是在 4022 行函数调用 free_block() 函数将这些可用缓存返回给共享
高速缓存或则 slab，最后为了保持函数栈先进后出的特点，调用 memmove() 函数将
剩余的所有缓存对象整体移动待缓存栈的底部.

> [free_block](#D030053)

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND000100.png)

-----------------------------------------

###### <span id="D030058">do_drain</span>

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/TB000098.png)

do_drain() 函数用于释放一个本地高速缓存. 参数 arg 指向一个高速缓存. SLAB
分配器为了加速缓存对象的分配，SLAB 分配器为每个 CPU 创建了本地高速缓存。
本地高速缓存使用缓存栈维护一定数量的可用缓存对象，在 do_drain() 函数里，
函数会将缓存栈内的所有缓存对象全部释放给 slab。

函数在 2417 行调用 cpu_cache_get() 函数获得对应的本地缓存，然后使用
free_block() 函数将所有的缓存对象释放给 slab，最后将本地高速缓存的可用
缓存对象设置为 0.

> [cpu_cache_get](#D030004)
>
> [free_block](#D030053)

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND000100.png)

-----------------------------------------

###### <span id="D030057">\_\_kmem_cache_destroy</span>

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/TB000097.png)

\_\_kmem_cache_destroy() 函数用于释放高速缓存的本地缓存、slab 链表，以及
高速缓存本身. 参数 cachep 指向高速缓存.

函数在 1929 行调用 for_each_online_cpu() 函数遍历所有在线的 CPU，并调用 kfree
释放了所有本地高速缓存. 函数在 1933 行调用 for_each_online_node() 函数遍历
所有的 NODE 节点，并在每次遍历中释放高速缓存的 slab 链表以及共享高速缓存.
函数最后在 1941 行调用 kmem_cache_free() 函数释放了高速缓存本身.

> [kfree](#D030055)
>
> [kmem_cache_free](#D030051)

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND000100.png)

-----------------------------------------

###### <span id="D030056">drain_freelist</span>

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/TB000096.png)

drain_freelist() 函数用于从高速缓存 slabs_free 链表上移除指定数量的 slab
归还给 Buddy 分配器. cache 参数指向高速缓存, 参数 l3 指向 slab 链表，tofree
参数指明需要释放 slab 的数量.

函数在 2458 行使用 while 循环，只要 nr_freed 小于 tofree，且 slabs_free 链表
不为空，那么循环一直进行。在每次循环中，函数在 2461 行获得 slabs_free 链表最
后一个 slab，将 slab 从 slabs_free 链表上移除，并更新 slab 链表中可用缓存
对象的数量。函数接着在 2478 行调用 slab_destroy() 函数将 slab 占用的内存归还
给 Buddy 分配器. 最后返回释放 slab 的数量.

> [slab_destroy](#D030052)

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND000100.png)

-----------------------------------------

###### <span id="D030055">kfree</span>

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/TB000095.png)

kfree() 函数用于释放一段虚拟地址回 SLAB 分配器. 参数 objp 指向需要释放的虚拟
地址. 除去一些 debug 函数，函数通过 \_\_cache_free() 函数释放指定的内存.

> [\_\_cache_free](#D030045)

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND000100.png)

-----------------------------------------

###### <span id="D030046">cache_flusharray</span>

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/TB000086.png)

cache_flusharray() 函数用于将本地高速缓存里的批量缓存对象归还给共享高速缓存
或者 slab. 参数 cachep 指向高速缓存，ac 指向本地高速缓存.

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/TB000001.png)

函数在 3485 行获得释放缓存对象的数量，其可以从本地高速缓存的 batchcount 中获得。
3490 行获得高速缓存的 slab 链表，如果此时 l3 shared 不为零，及共享高速缓存
存在，那么函数执行 3493 行到 3502 行代码逻辑，在这段代码逻辑中，函数首先确认
共享高速缓存中维护可用缓存对象数量是否已经达到上限，如果没有达到上限，那么
函数将批量的缓存对象插入到共享高速缓存的缓存栈上，插入完毕之后调转到 free_done；
如果共享高速缓存已经无法容纳缓存对象了，那么直接跳转到 3504 行继续执行.
函数在 3505 行调用 free_block() 函数将批量的可用缓存对象归还给 slab，最后更新
本地高速缓存可用缓存对象的数量，并将栈定部分的缓存对象整体向栈底移动.

> [free_block](#D030053)

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND000100.png)

-----------------------------------------

###### <span id="D030054">virt_to_slab</span>

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/TB000094.png)

virt_to_slab() 函数用于通过缓存对象的虚拟地址获得对应的 slab。参数 obj 指向
缓存对象的虚拟地址. 函数首先调用 virt_to_head_page() 函数获得虚拟地址对应的
物理页，然后通过调用 page_get_slab() 函数获得物理页对应的 slab。

> [page_get_slab](#D030012)

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND000100.png)

-----------------------------------------

###### <span id="D030053">free_block</span>

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/TB000093.png)

free_block() 函数用于释放批量的缓存对象会 slab 链表里。参数 cachep 指向高速
缓存，参数 objpp 指向缓存所在为的位置，参数 nr_objects 指明释放缓存的数量，
参数 node 指明 NODE 信息.

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/TB000001.png)

SLAB 分配器为高速缓存在每个 NODE 上使用 kmem_list3 维护了所有的 slab，其通过
维护 3 种链表，将不同条件的 slab 插入到指定的 slab 链表中，其中 slabs_partial
链表上维护的 slab 包含了一定数量可用的缓存对象; slabs_full 链表上维护的 slab
的所有缓存对象已经分配; slabs_free 链表上维护的 slab 上的所有缓存对象都未分配。

函数在 3441 行遍历所有需要释放的缓存对象，每次遍历中，函数在 3445 行通过调用
virt_to_slab() 函数获得缓释放缓存对象对应的 slab，并结合 node 信息获得高速缓存
对应的 slab 链表。函数在 3450 行调用 slab_put_obj() 函数将释放对象在 slab 中
标记为可分配使用的, 并更新 slab 链表上可用缓存对象的数量。函数在 3456 行继续
检测当前 slab 的所有缓存对象是否都没有在使用，如果 slab 所有缓存对象都没有在
使用，那么函数继续检测当前 slab 链表上可用缓存对象数量是否已经超过 slab 链表
的上限，如果超过，那么 SLAB 分配器将该 slab 摧毁并释放会 Buddy 分配器，函数
3458 行更新 slab 链表上可用缓存对象数量，然后调用 slab_destory() 函数摧毁
slab; 如果该 slab 上所有缓存对象可用，当 slab 链表的可用缓存对象数量没有达到
上限，那么函数将该 slab 从插入到 slabs_free 链表里; 如果 slab 的在使用的缓存
数量不为零，那么将该 slab 插入到 slabs_partial 链表上.

> [virt_to_slab](#D030054)
>
> [slab_put_obj](#D030047)
>
> [slab_destroy](#D030052)

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND000100.png)

-----------------------------------------

###### <span id="D030052">slab_destroy</span>

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/TB000092.png)

slab_destroy() 函数用于释放一个 slab。参数 cachep 用于指向高速缓存，slabp 
指向即将释放的 slab. SLAB 分配器可以采用两种方式 slab，第一种是使用 RCU 的
方式释放 slab，第二种则是直接释放 slab。

函数在 1907 行首先获得 slab 占用物理页块的首地址。然后函数会的检测高速缓存
的标志中是否包含 SLAB_DESTROY_BY_RCU，如果包含，那么函数在使用 slab 时候需要
按 RCU 方式释放，1911 行到 1916 行函数使用数据结构 struct slab_rcu 辅助 SLAB
分配器使用 RCU 方式进行释放，函数将高速缓存和 slab 以及首地址存储在 slab_rcu
中，然后调用 call_rcu() 函数和 kmem_rcu_free() 在合适的情况下释放 slab; 反之
如果高速缓存不支持 SLAB_DESTROY_BY_RCU，那么函数通过 1918 行到 1921 行进行
slab 释放，函数通过调用 kmem_freepages() 函数将 slab 占用的物理页全部归还给
Buddy 分配器，如果此时 slab 的管理数据位于 slab 外部，那么函数继续调用
kmem_cache_free() 将 slab 管理数据释放回对应的高速缓存.

> [kmem_rcu_free](#D030050)
>
> [kmem_freepages](#D030049)
>
> [kmem_cache_free](#D030051)

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND000100.png)

-----------------------------------------

###### <span id="D030051">kmem_cache_free</span>

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/TB000091.png)

kmem_cache_free() 函数用于释放一个缓存对象. 参数 cachep 指向高速缓存，参数
objp 指向需要释放的缓存对象.

函数在 3753 行通过调用 \_\_cache_free() 函数释放一个缓存对象.

> [\_\_cache_free](#D030045)

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND000100.png)

-----------------------------------------

###### <span id="D030050">kmem_rcu_free</span>

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/TB000090.png)

kmem_rcu_free() 函数的作用是使用 RCU 的方式释放 slab. 参数 head 指向 struct
slab_rcu 结构体. 函数在 1684 行获得 slab 对应的高速缓存，并且 slab 对应的
地址位于 slab_rcu 的 addr 成员里。于是函数在 1686 行调用 kmem_freepages()
函数释放了 slab 占用的物理页。1687 行通过调用 OFF_SLAB() 函数检测 slab 的管理
数据是否位于 slab 外部，如果 slab 管理数据位于 slab 外部，那么函数调用 
kmem_cache_free() 函数，将 slab 管理数据释放会对应的高速缓存。

> [kmem_freepages](#D030049)
>
> [kmem_cache_free](#D030051)

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND000100.png)

-----------------------------------------

###### <span id="D030049">kmem_freepages</span>

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/TB000089.png)

kmem_freepages() 函数用于将 slab 占用的物理页释放会 Buddy 分配器. 参数 cachep
指向高速缓存，addr 指向物理页对应的虚拟地址. 当 SLAB 分配器为高速缓存创建 slab
的时候就会从 Buddy 分配器分配一定数量的物理页，当 SLAB 分配器根据需求，将高速
缓存的 slab 占用的内存释放回 Buddy 内核分配器。函数 1660 行调用 virt_to_page()
函数获得 addr 参数对应的物理页，如果高速缓存为 slab 分配物理页时支持
SLAB_RECLAIM_ACCOUNT 标志，也就代表申请的物理页支持回收，因此在释放的时候，函数
调用 sub_zone_page_state() 函数更新物理页对应分区可回收页的数量; 反之如果分配
时物理页不支持 SLAB_RECLAIM_ACCOUNT 标志，也就是物理页都是不可回收的，因此在
释放的时候，函数调用 sub_zone_page_state() 函数更新物理页对应分区不可回收页的
数量. 函数在 1671 行到 1675 行调用 \_\_ClearPageSlab() 函数清楚物理页的 PG_slab
标志。最后函数在 1678 行调用 free_pages() 函数将对应的物理页归还给 Buddy 分配器.

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND000100.png)

-----------------------------------------

###### <span id="D030048">obj_to_index</span>

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/TB000088.png)

obj_to_index() 函数用于获得缓存对象在 slab kmem_bufctl 数组中的索引。参数
cache 指向高速缓存，slab 参数执行那个对应的 slab，obj 参数指向缓存对象.

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/TB000008.png)

在 slab 中，slab 管理数据 kmem_bufctl 数组管理所有缓存对象的使用情况，struct
slab 管理数据的 s_mem 指向了缓存对象在 slab 中的起始地址. 函数 563 行通过减法
计算出缓存对象在 slab 缓存对象区的偏移，然后调用 reciprocal_divide() 函数计算
出了缓存对象在 kmem_bufctl 数组中的偏移.

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND000100.png)

-----------------------------------------

###### <span id="D030047">slab_put_obj</span>

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/TB000087.png)

slab_put_obj() 函数的作用是将一个缓存对象释放会 slab。参数 cachep 指向高速
缓存，slabp 参数指向 slab，参数 objp 指向缓存对象，参数 nodeid 指明了 NODE
信息.

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/TB000008.png)

在 slab 中，slab 使用 kmem_bufctl 数组管理每一个可用对象的使用情况，函数 2597 
行调用 obj_to_index() 函数获得释放缓存对象在 kmem_bufctl 数组中的索引, 然后
将对应 kmem_bufctl 对应成员的值设置为当前 slab 第一个可用缓存索引值，然后修改
slab 管理数据，将 slab 第一个可用缓存对象的索引 free 设置为 objp 对应的索引
值，最后将 inuse 减一，以此表示 slab 中正在使用的缓存对象减少一个.

> [obj_to_index](#D030048)

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND000100.png)

-----------------------------------------

###### <span id="D030045">\_\_cache_free</span>

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/TB000085.png)

\_\_cache_free() 函数用于释放一个缓存对象。参数 cachep 指向高速缓存，参数 objp
指向缓存对象.

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/TB000001.png)

SLAB 分配器使用 struct kmem_cache 维护一个高速缓存，SLAB 为了加速缓存对象的
分配，为每个 CPU 分配了本地高速缓存，通过 array 数组指向本地高速缓存。本地
高速缓存通过 struct cache_array 数据结构进行维护，其通过使用一个缓存栈维护
一定数量的缓存对象，当 SLAB 分配器分配或释放缓存对象时，不是直接与 slab 进行
交互，而是与本地高速缓存进行交互。

函数 3536 行调用 cpu_cache_get() 函数获得高速缓存对应的本地缓存。在本地高速
缓存栈上，struct array_cache 的 avail 指向缓存栈的栈顶，在将一个缓存对象释放
会本地缓存的时候，如果缓存栈中可用的缓存对象大于本地缓存容纳的最大可用缓存数
时，那么函数执行 3559 行到 3561 行的代码逻辑，调用 cache_flusharray() 函数将
一定数量的可用缓存对象归还给 slab，再将释放的缓存对象插入到缓存栈上; 反之如果
缓存栈上维护的可用缓存对象数量没有超过本地高速缓存维护的最大可用缓存对象数量，
那么函数直接将释放的缓存对象插入缓存栈的栈顶.

> [cache_flusharray](#D030046)

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND000100.png)

-----------------------------------------

###### <span id="D030044">kmem_cache_init_late</span>

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/TB000084.png)

kmem_cache_init_late() 函数用于完成 SLAB 分配器初始化后期. 在内核初始化到
一定阶段之后，内核已经支持 CPU 热插拔以及多 CPU 模式，此时 SLAB 更新所有
高速缓存的本地高速缓存. 

函数 1565 行为遍历 cache_chain 链表加上互斥锁 cache_chain_mutex. 1566 行函数
调用 list_for_each_entry() 函数遍历 cache_chain 链表上的所有高速缓存，并在
每次遍历过程中，调用 enable_cpucahe() 函数更新高速缓存的本地高速缓存。由于
该阶段内核已经支持 CPU 热插拔，那么有的 CPU 可以离线但本地高速缓存还没有释放，
那么 SLAB 分配器需要更新所有高速缓存的本地高速缓存，以此让离线 CPU 未释放的
本地高速缓存释放掉. 遍历完毕之后调用调用 mutex_unlock() 函数接触 cache_chain
的互斥锁。将 g_cpucache_up 设置为 FULL, 以此表示 SLAB 能完整使用.

> [enable_cpucache](#D030036)

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND000100.png)

-----------------------------------------

###### <span id="D030043">MAKE_ALL_LISTS</span>

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/TB000083.png)

MAKE_ALL_LISTS() 函数用于将高速缓存的 slab 链表的成员迁移到新的链表上。
参数 cachep 指向高速缓存，参数 ptr 指向新的链表，nodeid 参数用于指明 NODE 
信息. 在高速缓存中，为每个 NODE 维护了一套 slab 链表，slab 链表包含了 3 种链
表，分别是 slabs_partial、slabs_free 以及 slabs_full。函数通过调用 
MAKE_LIST() 函数将高速缓存原始的 slab 链表中的成员迁移到新的 slab 链表中.

> [MAKE_LIST](#D030042)

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND000100.png)

-----------------------------------------

###### <span id="D030042">MAKE_LIST</span>

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/TB000082.png)

MAKE_LIST() 函数用于初始化新的 slab 链表，并将原始高速缓存的指定 slab 链表
上的成员迁移到新的 slab 链表上. 参数 cachep 指向高速缓存，listp 指向新的链表，
slab 指明 slab 链表的类型，nodeid 参数用于指定 NODE 信息。

函数在 367 行初始化了 listp 对应的链表，然后调用 list_splice() 函数将高速缓存
nodeid 阶段对应的 slab 链表上的成员迁移到 listp 上面.

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND000100.png)

-----------------------------------------

###### <span id="D030041">init_list</span>

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/TB000081.png)

init_list() 用于创建一个新的 struct kmem_list3 数据结构，并将高速缓存原始的
kmem_list3 slab 链表数据迁移到新的 kmem_list3 上，并将高速缓存的 slab 链表
指向新的 kmem_list3。参数 cachep 指向高速缓存，参数 list 指向 slab 链表，参数
nodeid 指明 NODE 信息.

函数在 1342 行调用 kmalloc_node() 函数从指定的节点上分配内存，并在 1345 行将
高速缓存原始的 slab 链表信息迁移到 ptr 对应的内存上，然后初始化了对应的 
list_lock 锁，接着调用 MAKE_ALL_LISTS() 函数将 slab 链表上的数据全部迁移
到 ptr 对应的链表上，最后更新高速缓存的 slab 链表指向 ptr.

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND000100.png)

-----------------------------------------

###### <span id="D030040">kmem_cache_init</span>

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/TB000073.png)

kmem_cache_init() 函数用于初始化 SLAB 分配器. SLAB 分配器的初始化是一个 
"鸡和蛋" 问题，因此 SLAB 分配器为解决这个问题将初始化过程分作了几个阶段，
通过 g_cpucache_up 变量进行表示，g_cpucache_up 的取值如下:

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/TB000059.png)

当 g_cpucache_up 为 NONE 的时候，SLAB 分配器只能提供 cache_cache 一个高速
缓存，在这个阶段，SLAB 分配器需要通过 cache_cache 和静态数据构建 cache_array
高速缓存，当 cache_array 高速缓存构建完毕之后，SLAB 分配器进入 PARTIAL_AC
阶段，在该阶段中，SLAB 分配器基于已经存储的静态数据和几个高速缓存，构建
kmem_list3 高速缓存，该缓存构建完毕之后，SLAB 分配器位于 PARTIAL_L3 缓存。
此时 SLAB 基本可以使用 SLAB 分配器大部分功能，于是将之前使用的静态数据通过
高速缓存进行分配，并将静态数据迁移到新分配的内存，完成之后 SLAB 进入 EARLY
阶段，此时 SLAB 基本已经可以使用所有 SLAB 分配器功能，但此时由于高速缓存只能
外为 boot CPU 分配本地高速缓存。等内核进行调用 kmem_cache_init_late() 函数
对 SLAB 分配进行最后的初始化之后，SLAB 分配器进入 FULL 阶段，SLAB 分配器正式
初始化完毕.

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/TB000074.png)

SLAB 分配器初始化最早的阶段，函数调用 kmem_list3_init() 函数和 set_up_list3s()
函数为 cache_cache 高速缓存的创建构建 slab 链表. 此时 slab 链表相关的 kmem_list3
数据来自静态数据 initkmem_list3.

> [kmem_list3_init](#D030000)
>
> [set_up_list3s](#D030001)

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/TB000075.png)

SLAB 分配器首先初始化高速缓存全局链表 cahce_chain, 然后将 cache_cache 插入
到该链表上，然后 1426 行计算了 cache_cache 着色长度，并存储在高速缓存的 
colour_off 成员里，1427 行为 cache_cache 高速缓存设置本地高速缓存，其使用
静态数据 initarray_cache 提供的内存进行创建. 1428 行函数通过静态数据 
initkmem_list3 创建了 cache_cache 的 slab 链表. 1434 行计算了 struct kmem_cache
高速缓存的对象长度. 函数在 1439 行对高速缓存的对象长度进行了对齐操作.

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/TB000076.png)

函数在 1444 行使用 for 循环来确认 Buddy 分配器需要提供物理页的数量，以便
cache_cache 构建 slab。在每次循环中，函数调用 cache_estimate() 函数计算
cache_cache 高速缓存使用的 slab 占用物理页的数量，已经 slab 中维护缓存对象的
数量，最后还计算了 slab 浪费的内存长度。在每次循环中，如果当前物理页满足 slab
的构建要求，那么跳出循环。如果循环结束，Buddy 分配器提供的物理页还不能满足
cache_cache 的 slab 构建，那么 SLAB 直接调用 BUG_ON() 报错并提供初始化. 如果
找到 slab 构建的正确数据，那么将 slab 占用物理页的数据存储在 cache_cache 高速
缓存的 gfporder 成员里。1452 行函数通过 slab 浪费的内存除以着色长度，以此计算
了 cache_cache 的着色范围.最后将 slab 管理数据的长度存储在 cache_cache 的
slab_size 成员里.

> [cache_estimate](#D030002)

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/TB000077.png)

SLAB 分配器为了加速内核中通用的长度的内存分配速度，为系统提供了通用高速缓存，
这些通用高速缓存的对象的长度为 2 的幂次，这些高速缓存也称为匿名高速缓存。
SLAB 分配器初始化到图中的阶段时，通过计算 struct cache_array 的长度，建立对应
的通用高速缓存，通过调用 kmem_cache_create() 函数进行创建。函数在 1472 行进行
检测，用于判断 struct cache_array 与 struct kmem_list3 数据结构是否相同，如果
不同，那么 SLAB 分配器也会调用 kmem_cache_create() 函数为 struct kmem_list3
数据结构长度创建对应的通用高速缓存。创建完毕之后，将 slab_early_init 设置为 0.

> [kmem_cache_create](#D030039)

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/TB000078.png)

SLAB 分配器接下来为系统通用高速缓存创建对应的高速缓存，如果 CONFIG_ZONE_DMA
宏打开，那么也为 DMA 通用高速缓存创建高速缓存.

> [kmem_cache_create](#D030039)

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/TB000079.png)

SLAB 初始化到这个阶段，由于原始 cache_cache 的本地高速缓存通过静态数据
initarray_cache 进行维护。此时 struct cache_array 对应的通用高速缓存已经
可以使用，因此函数在 1514 行调用 kmalloc() 函数为 ptr 指针分配内存，并在 1517
行将 cache_cache 对应的本地高速缓存数据迁移到 ptr 对应的内存上，接着初始化了
ptr 对应的数据，其中包括 lock 锁的初始化，接着将 cache_cache 的本地高速缓存
指向了 ptr。函数继续调用 kmalloc() 分配内存，且 ptr 指向新分配的内存. 函数在
1530 行将 struct cache_array 对应的通用高速缓存的本地缓存数据迁移到 ptr 指向
的内存，然后将该通用高速缓存的本地缓存指向 ptr。至此 SLAB 分配器中原始的静态
本地缓存占用的内存全部替换成 SLAB 提供的内存.

> [kmalloc](#D030028)

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/TB000080.png)

SLAB 初始化到这个阶段，SLAB 分配器准备将静态的 slab 链表数据更新为 SLAB 提供
的内存. 函数在 1544 行调用 for_each_online_node() 函数遍历系统所有在线的 NODE，
在每次遍历中，函数将 cache_cache 高速缓存和 struct cache_array、
struct kmem_list3 对应的 slab 链表占用的内存全部替换成 SLAB 分配的内存。通过
init_list() 函数实现. 遍历完毕之后，SLAB 分配器将 g_cpucache_up 设置为 EARLY，
以此表示 SLAB 分配器的可以正常使用.

> [init_list](#D030041)

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND000100.png)

-----------------------------------------

###### <span id="D030039">kmem_cache_create</span>

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/TB000060.png)

kmem_cache_create() 函数用于创建一个高速缓存. 参数 name 指明高速缓存的名字;
参数 size 指明高速缓存对象的长度; 参数 align 指明高速缓存对象的对齐方式; 
参数 flags 包含了高速缓存创建标志; 参数 ctor 指向了高速缓存对象的构建函数.

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/TB000001.png)

SLAB 分配器使用 struct kmem_cache 表示一个高速缓存，其包含了高速缓存的基础信息，
其中也包含了高速缓存的本地缓存、共享缓存，以及 slab 链表等。高速缓存的创建过程
就是用于填充和创建高速缓存所需的数据.

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/TB000061.png)

函数首先进行基础的检测，其中包括高速缓存的名字是否存在、此时是否处在中断中、
高速缓存对象的长度是否小于 BYTES_PER_WORD 或者大于 KMALLOC_MAX_SIZE, 只要
以上条件其一满足，那么 SLAB 分配器认为这是非法的，直接内核 BUG.

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/TB000062.png)

函数 2128 行调用 slab_is_available() 函数检测当前的 SLAB 分配器是否可用，
如果可用，那么如果此时系统支持 CPU 热插拔，那么函数调用 get_online_cpus()
获得可用的 CPU，然后调用 mutex_lock() 上互斥锁. 函数在 2133 行处调用函数
list_for_each_entry() 函数遍历所有的高速缓存.

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/TB000010.png)

SLAB 分配器将所有的高速缓存维护在 cache_chain 链表上，然后函数遍历所有的
高速缓存，如果遍历到的高速缓存的名字与即将创建的高速缓存名字一致，那么 SLAB
分配器不允许名字相同的高速缓存存在，因此调用 dump_stack() 报错并跳转到 oops。

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/TB000063.png)

函数 2182 行到 2231 行，函数根据高速缓存创建时提供的标志进行对齐相关的操作，
最终会将对齐的结果存储在 ralign 变量里.

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/TB000064.png)

函数在 2240 行继续判断当前 SLAB 分配器已经正常使用，如果正常使用，那么 SLAB
分配器从 Buddy 分配器分配的物理内存使用 GFP_KERNEL 标志进行分配; 反之如果 
SLAB 分配器还不能完整使用，那么从 Buddy 分配器分配物理内存时使用 GFP_NOWAIT
标志，以便让分配不能失败.

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/TB000065.png)

函数 2246 行调用 kmem_cache_zalloc() 函数为高速缓存分配一个新的 
struct kmem_cache. 如果分配失败，则跳转到 oops。

> [kmem_cache_zalloc](#D030022)

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/TB000066.png)

函数 2287 行到 2293 行用于判断 slab 默认的管理数据是位于 slab 内部还是外部。
判断的条件之一是 slab_early_init 是否为零，这也说明了 SLAB 初始化完毕之前，
slab 的管理数据默认只能待在 slab 内部. 函数判断如果 slab 管理数据位于 slab
外部，必须满足这几个条件: 其一高速缓存对象的长度不小于 "PAGE_SIZE >> 3",
其二 SLAB 分配器已经过了基础初始化阶段，其三是 flags 标志不能包含 
SLAB_NOLEAKTRACE. 只有同时满足上面三个条件，那么 slab 的管理数据才能位于
slab 外部. 如果条件都满足了，那么函数将 CFLAGS_OFF_SLAB 标志添加到 flags 参数
里面.

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/TB000067.png)

函数 2295 行获得高速缓存对象对齐之后的长度，并在 2297 行调用 
calculate_slab_order() 函数计算出高速缓存每个 slab 维护缓存对象的数量，然后
计算每个 slab 占用物理页的数量，并计算出每个 slab 剩余的内存数量. 函数在
2306 行计算除了 slab 管理数据的长度，并存储在 slab_size 变量中.

> [calculate_slab_order](#D030023)

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/TB000068.png)

函数在 2313 行检测 slab 的管理数据是否位于 slab 外面，同时也检测 slab 剩余
的内存是否比 slab 管理数据大。如果高速缓存的 flags 参数包含了 CFLAGS_OFF_SLAB
标志，即表示想让 slab 管理数据位于 slab 外部，当如果此时 slab 管理数据的长度
小于等于 slab 剩余的内存，那么 SLAB 分配器不会让该高速缓存的 slab 管理数据
位于 slab 外部，因此将 CFLAGS_OFF_SLAB 标志从 flags 中移除，并将 slab 中剩余
的内存减去了 slab 管理数据的长度.

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/TB000021.png)

函数接着在 2318 行检测 slab 管理数据是否位于 slab 外部，如果此时 slab 管理数据
位于 slab 外部，那么 SLAB 分配器需要独立计算 slab 管理数据占用的内存数量，从
上图可以看出 slab 管理数据包括了 struct slab 以及 kmem_bufctl 数组，kmem_bufctl
数组的长度与 slab 中位于缓存对象数量有关.

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/TB000069.png)

函数 2333 行调用 cache_line_size() 计算出当前 cache line 的长度，以此作为高速
缓存着色的长度，存储在高速缓存的 colour_off 成员里，如果 cache line 的长度
小于之前计算出来的对齐长度，那么函数还是将高速缓存的着色长度设置为 align。
函数在 2337 行将之前获得的 slab 剩余的内存除以着色长度，以此获得高速缓存着色
范围，并存储在高速缓存的 colour 成员里. 接着在 2338 行将 slab 管理数据的长度
存储在高速缓存的 slab_size 成员里，2339 行将高速缓存的创建标志存在在 flags 
成员里，并在 2341 行到 2342 行计算高速缓存从 Buddy 分配器中获得物理页的标志.
2343 行将缓存对象的长度存储在高速缓存的 buffer_size 成员里.

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/TB000070.png)

函数在 2346 行检测 slab 管理数据是否位于 slab 外部，如果位于，那么函数调用
kmem_find_general_cachep() 函数根据 slab 管理数据的长度，从 SLAB 中获得一个
合适的通用高速缓存，并使用高速缓存的 slabp_cache 指向该通用高速缓存. 函数
2357 行设置了高速缓存的构造函数，2358 行设置了高速缓存的名字.

> [kmem_find_general_cachep](#D030025)

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/TB000071.png)

函数在 2360 行调用 setup_cpu_cache() 函数用于设置高速缓存的本地高速缓存、
共享高速缓存以及 slab 链表. 如果设置失败，那么函数调用 \_\_kmem_cache_destroy()
函数摧毁该高速缓存，并将高速缓存指针 cachep 设置为 NULL, 最后跳转到 oops.
如果分配成功，那么将高速缓存插入到系统高速缓存链表 cache_chain 里.

> [setup_cpu_cache](#D030037)

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/TB000072.png)

2368 行即为跳转 oops 处，函数首先判断高速缓存是否已经成功分配了，如果没有
成功分配且 flags 中包含了 SLAB_PANIC 标志，那么调用 panic() 函数. 函数
2372 行调用函数 slab_is_available() 函数检测是否 SLAB 分配器已经正常完整使用，
如果已经可以，那么函数接触 cache_chain_mutex 互斥锁，并调用 put_online_cpus()
函数。函数最后返回指向高速缓存的指针 cachep，并将该函数通过 EXPORT_SYMBOL()
导出给内核其他部分使用.

> [slab_is_available](#D030038)

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND000100.png)

-----------------------------------------

###### <span id="D030038">slab_is_available</span>

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/TB000058.png)

slab_is_available() 函数用于判断 SLAB 分配器是否可以正常使用。由于 SLAB 分配
器的初始化是一个 "鸡和蛋" 的问题，因此 SLAB 分配器将初始化分作几个阶段，通过
g_cpucache_up 进行描述，g_cpucache_up 可以取值:

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/TB000059.png)

NONE 阶段 SLAB 分配器只能分配 struct kmem_cache 对应的高速缓存，且功能不是
很完整; PARTIAL_AC 阶段 SLAB 分配器已经增加了 struct array_cache 对应的高速
缓存，因此高速函数可以为本地高速缓存分配内存了; PARTIAL_L3 阶段，SLAB 分配器
增加了 struct kmem_list3 的缓存，因此该阶段 kmalloc 等函数已经可以使用; EARLY
阶段，SLAB 分配器已经将分配器相关的所有数据都使用 SLAB 分配器分配内存，不再
使用静态数据; FULL 阶段 SLAB 分配器已经可以像一个正常的分配器为内核提供接口
进行内存分配和回收. 因此 SLAB 如果处于 EARLY 及其之后都表示 SLAB 已经准备好，
可以开始使用了.

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND000100.png)

-----------------------------------------

###### <span id="D030037">setup_cpu_cache</span>

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/TB000057.png)

setup_cpu_cache() 函数用于为高速缓存设置本地高速缓存. 参数 cachep 指向高速
缓存，参数 gfp 指向分配内存的标志.

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/TB000001.png)

在每个高速缓存中，SLAB 分配器为加速高速缓存的分配速度，为每个 CPU 创建了一个
本地高速缓存，其使用一个缓存栈维护一定数量的可用缓存对象. 由于 SLAB 分配器的
初始化分作多个阶段，每个阶段都涉及到为高速缓存创建本地高速缓存。SLAB 分配器
使用 g_cpucache_up 变量来标示 SLAB 分配器的初始化进度。函数 2028 行到 2045 行，
SLAB 分配器处于初始化的第一阶段，这个阶段 struct kmem_list3 和
struct cache_array 对应的高速缓存还没有创建，因此为该阶段的高速缓存分配本地
缓存只能使用静态变量 initarray_generic 对应的本地高速缓存，由于这个阶段只有
一个 CPU 在运行，因此函数在 2034 行设置了本地高速缓存. 并调用 set_up_list3s()
函数设置这个阶段的 kmem_list3 链表. 如果此时 INDEX_AC 的值与 INDEX_L3 的值
相等，那么函数将 SLAB 初始化进度设置为 PARTIAL_L3; 反之设置为 PARTIAL_AC。通过
源码分析可以知道，此时 INDEX_AC 与 INDEX_L3 不相等，因此将 SLAB 分配器初始化
进度设置为 PARTIAL_AC. 函数从 2048 行到 2063 行，SLAB 分配器初始化进度达到
第二阶段之后，函数使用这段代码为高速缓存分配本地高速缓存, 此时 kmalloc() 函数
已经可以使用，2047 行到 2048 行，函数调用 kmalloc() 函数为高速缓存分配内存，
此时系统还是只有一个 CPU，由于此时 SLAB 分配器初始化进度是 PARTIAL_AC，因此
函数在 2051 行将 SLAB 分配器初始化进度设置为 PARTIAL_L3; 当 SLAB 分配器初始化
进入第三阶段，此时系统中多个 CPU 已经可以使用，因此函数在 2055 行遍历所有的
在线 CPU，并调用 kmalloc_node() 函数为每个 CPU 分配本地高速缓存，并调用 
kmem_list3_init() 函数初始化高速缓存的 kmem_list3 链表. 对于 SLAB 分配器
初始化进度前三个阶段，SLAB 分配器将这些阶段的本地缓存维护的缓存对象控制在
BOOT_CPUCACHE_ENTRIES 个. 函数 2068 行到 2073 行初始化了基础的成员. SLAB
分配器初始化进度到第四阶段，即 g_cpucache_up 等于 FULL, 那么函数在 2026 行
调用 enable_cpucache() 函数初始化高速缓存的本地高速缓存、共享高速缓存，以及
kmem_list3 slab 链表.

> [enable_cpucache](#D030036)
>
> [set_up_list3s](#D030001)
>
> [kmem_list3_init](#D030000)
>
> [kmalloc](#D030028)
>
> [kmalloc_node](#D030030)

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND000100.png)

-----------------------------------------

###### <span id="D030036">enable_cpucache</span>

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/TB000056.png)

enable_cpucache() 函数用于计算一个高速缓存的本地缓存维护缓存对象的数量，以及
高速缓存对应的共享高速缓存维护缓存对象的数量，并为高速缓存分配本地缓存和 
kmem_list3 链表, 最后更新本地缓存.

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/TB000001.png)

SLAB 分配器为加快高速缓存的分配速度，为每个 CPU 创建了一个本地高速缓存，其使用
一个缓存栈维护一定数量的可用缓存对象，每个 CPU 都可以快速从缓存栈上分配可用
的缓存对象. SLAB 分配器还为高速缓存创建了一个共享高速缓存，所有 CPU 都可以从
共享高速缓存中获得可用的缓存对象。SLAB 分配器使用 kmem_list3 链表为高速缓存
在指定 NODE 上维护了 3 类链表，以此管理指定的 slab。enable_cpucache() 函数就是
用于为一个高速缓存规划本地高速缓存和共享高速缓存中维护缓存对象的数量，以及
创建并更新本地缓存，最后为高速缓存创建 kmem_list3 链表。

函数 3971 行到 3980 行用于计算一个高速缓存对应的本地高速缓存维护可用缓存对象
的数量，从代码逻辑可以推断高速缓存对象越大，本地高速缓存能够维护可用缓存对象
数量越少。3991 行到 3993 行，如果高速缓存对象的大小不大于 PAGE_SIZE, 且
num_possible_cpus() 数量大于 1，即 CPU 数量大于 1，那么将共享高速缓存维护的
缓存对象设置为 8. 最后函数在 4003 行处调用 do_tune_cpucache() 函数为高速缓存
创建了本地高速缓存和共享高速缓存，以及 kmem_list3 链表.

> [do_tune_cpucache](#D030033)

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND000100.png)

-----------------------------------------

###### <span id="D030035">kzalloc</span>

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/TB000055.png)

kzalloc() 函数用于从 SLAB 分配器中获得指定大小的内存，并且内存内容已经被清空.
参数 size 指向分配内存的大小，flags 指明分配标志. 函数 321 行通过调用
kmalloc() 函数并传入 \_\_GFP_ZERO 标志实现.

> [kmalloc](#D030028)

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND000100.png)

-----------------------------------------

###### <span id="D030034">do_ccupdate_local</span>

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/TB000054.png)

do_ccupdate_local() 函数用于更新高速缓存对应的本地缓存。info 参数指向高速缓存
相关的信息. 在支持 CPU 热插拔的系统中，如果 CPU 离线但没有释放本高速地缓存，
那么函数会使用 struct ccupdate_struct 数据结构进行高速缓存的本地高速缓存释放.

3907 行获得 CPU 热插拔之前的本地高速缓存，3909 行到 3910 行，函数将新的本地
高速缓存与 CPU 热插拔之前的本地高速缓存进行交换. 交换之后，高速缓存使用了新的
本地高速缓存，而原始本地高速缓存已经存储在 struct ccupdate_struct 数据结构里.

> [struct ccupdate_struct](#D0108)

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND000100.png)

-----------------------------------------

###### <span id="D030033">do_tune_cpucache</span>

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/TB000052.png)

do_tune_cpucache() 函数用于为缓存分配本地缓存、共享缓存和 l3 链表. 参数
cachep 指向缓存，limit 用于指明本地缓存最大缓存对象的数量。shared 参数用于
指明共享缓存中缓存对象的数量; gfp 用于分配内存时候使用的标志.

函数 3920 行调用 kzalloc() 函数为 struct ccupdate_struct 分配新的内存。3924 
行到 3933 行，函数调用 for_each_online_cpu() 函数遍历所有的 CPU，并基于
struct ccupdate_struct 结构为 CPU 分配本地缓存结构 struct cache_array, 通过
alloc_arraycache() 函数进行分配. 函数 3936 行调用 on_each_cpu() 函数与
do_ccupdate_local() 函数更新当前高速缓存的本地缓存. 接着函数设置了高速缓存
的 batchcount, limit 以及 shared 成员. 函数 3943 行到 3951 行遍历所有在线的
CPU，如果发生了 CPU 热插拔事件之后，离线的 CPU 没有释放本地缓存，那么函数并
检测高速缓存对应的本地缓存是否存在，如果存在，那么释放本地缓存. 函数最后释放
new 变量，并调用 alloc_kmemlist() 函数为高速缓存分配 kmem_list3 链表数据.

> [kzalloc](#D030035)
>
> [alloc_arraycache](#D030031)
>
> [alloc_kmemlist](#D030032)
>
> [struct ccupdate_struct](#D0108)

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND000100.png)

-----------------------------------------

###### <span id="D030032">alloc_kmemlist</span>

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/TB000051.png)

alloc_kmemlist() 函数用于为缓存分配每个 NODE 的 kmem_list3 数据结构。参数
cachep 指向缓存，参数 gfp 指向分配内存使用的标志.

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/TB000001.png)

SLAB 分配器使用 struct kmem_cache 管理一个缓存，NUMA 架构中，每个缓存在每个 
NODE 上都维护着一个 kmem_list3 数据结构，因此该函数用于为缓存的每个 NODE 
的 struct kmem_list3 分配内存。函数 3818 行调用 for_each_online_node() 函数
遍历当前系统的所有 NODE，每次遍历过程中，函数 3820 行判断 use_alien_caches
是否为真. 函数 3827 行检测缓存是否支持共享缓存，如果支持，那么函数调用 
alloc_arraycache() 函数分配一个缓存栈，new_shared 指向该缓存栈; 如果缓存不
支持共享缓存，那么跳过 3828 到 3835 行的代码. 函数 3837 行处检测遍历到的
NODE struct kmem_list3 是否已经存在，如果存在，那么函数执行 3839 行开始的
代码逻辑，在这段代码逻辑中，函数首先获得原始的共享缓存，如果共享缓存存在，
那么调用 free_block() 函数将共享缓存内的缓存对象释放会 slab 中。然后将 shared
成员指向新的共享缓存，更新 struct kmem_list3 的 free_limit 成员; 如果缓存
不支持共享缓存，那么跳过这段代码. 函数 3859 行调用 kmalloc_node() 函数为
struct kmem_list3 分配内存，并在 3866 行调用 kmem_list3_init() 函数初始化
struct kmem_list3 数据结构，函数继续将 l3 的 shared 成员设置为 new_shared,
让其指向新的共享缓存. 并将缓存在该 NODE 下的 nodelist3 成员指向新分配的 l3.
函数继续循环遍历所有的 NODE，循环完毕之后，函数直接返回 0.

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND000100.png)

-----------------------------------------

###### <span id="D030031">alloc_arraycache</span>

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/TB000050.png)

alloc_arraycache() 函数用于为缓存分配缓存栈，并初始化缓存栈。参数 node 指明
NODE ID 信息，entries 指明缓存栈最多维护缓存对象的数量. 参数 batchcount 指明
缓存栈与 slab 交互缓存对象数量. 参数 gfp 用于指明分配内存使用的标志.

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/TB000001.png)

SLAB 分配器为了加入缓存对象的分配，为每个 CPU 设置了一个缓存栈，缓存栈里面
维护一定数量的缓存对象. SLAB 分配器使用 struct kmem_cache 维护一个缓存，其
成员 array 指向一个缓存栈，缓存栈使用 struct array_cache 进行维护，其 entry
成员是一个零数组，因此函数在 896 行为 entry 数组结合 entries 的数量规划了内存，
因此 memsize 参数就表示真实 struct array_cache 的长度。函数在 899 行调用 
kmalloc_node() 函数为 struct array_cache 分配内存，如果分配成功，那么将 avail
成员设置为 0，表示缓存栈没有可用缓存对象. limit 成员设置为 entries 表示缓存
栈最大容纳缓存数量; batchcount 成员表示当缓存栈没有可用缓存对象的时候从 slab
中获得缓存对象的数量. touched 成员设置为 0，表示缓存栈没有被使用过. 最后返回
缓存栈.

> [kmalloc_node](#D030030)

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND000100.png)

-----------------------------------------

###### <span id="D030030">kmalloc_node</span>

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/TB000049.png)

kmalloc_node() 函数用于从指定的 NODE 上分配指定长度的内存。参数 size 用于指明
分配内存的长度. node 参数指明 NODE 信息. 在 UMA 架构中，函数在 246 行直接调用
kmalloc() 函数分配所需的内存.

> [kmalloc](#D030028)

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND000100.png)

-----------------------------------------

###### <span id="D030029">kmem_cache_alloc_notrace</span>

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/TB000048.png)

kmem_cache_alloc_notrace() 函数用于从指定的高速缓存中分配一个可用的缓存对象，且
不带调用者信息。参数 cachep 指向高速缓存，flags 参数指明分配的标志. 函数在 120
行通过调用 kmem_cache_alloc() 函数分配可用的缓存对象.

> [kmem_cache_alloc](#D030021)

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND000100.png)

-----------------------------------------

###### <span id="D030028">kmalloc</span>

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/TB000047.png)

kmalloc() 函数用于分配指定长度的内存。参数 size 指明分配内存的长度，参数 flags
指明分配的标志. 函数在 133 行通过 \_\_builtin_constant_p() 函数判断 size
参数是否是一个常量，如果是，那么函数执行 134 行到 161 行代码逻辑, 在这段代码
逻辑中，函数首先判断 size 参数的合法性，然后遍历 SLAB 提供的系统常用缓存的
长度，以此找到第一个满足条件的长度，并将 cachep 指向该高速缓存。函数在 155 行
调用 kmem_cache_alloc_notrace() 函数从高速缓存中分配一个可用的缓存对象. 最后在
160 行返回缓存对象; 如果 size 参数不是常量，那么函数在 162 行出调用 
\_\_kmalloc() 函数分配指定长度的内存.

> [kmem_cache_alloc_notrace](#D030029)
>
> [\_\_kmalloc](#D030027)

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND000100.png)

-----------------------------------------

###### <span id="D030027">\_\_kmalloc</span>

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/TB000046.png)

\_\_kmalloc() 函数用于分配指定长度的内存. 参数 size 指明分配内存的长度，参数
flags 指明分配参数. 函数在 3719 行调用 \_\_do_kmalloc() 函数进行实际的内存
分配.

> [\_\_do_kmalloc](#D030026)

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND000100.png)

-----------------------------------------

###### <span id="D030026">\_\_do_kmalloc</span>

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/TB000045.png)

\_\_do_kmalloc() 函数用于分配指定长度的内存。size 指明内存的长度，flags 指明
分配标志，caller 为申请者. 当使用 \_\_do_kmalloc() 函数分配指定长度的内存，
SLAB 分配器会从 malloc_sizes 缓存数组中找到一个合适的缓存对象，并从缓存对象
中分配可用缓存对象.

函数 3704 行到 3706 行调用 \_\_find_general_cachep() 函数从 malloc_sizes 缓存
数组中找到 size 参数适合的缓存对象，然后 3707 行调用 \_\_cache_alloc() 函数
从缓存对象中分配可用的缓存对象，最后返回缓存对象对应的虚拟地址.

> [\_\_find_general_cachep](#D030024)
>
> [\_\_cache_alloc](#D030020)

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND000100.png)

-----------------------------------------

###### <span id="D030025">kmem_find_general_cachep</span>

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/TB000044.png)

kmem_find_general_cachep() 函数用于通过指定长度找到内核频繁使用的缓存对象。
参数 size 指明长度，gfpflags 指明与物理内存分配相关的标志. 739 行函数调用
\_\_find_general_cachep() 函数获得内核频繁使用的缓存对象.

> [\_\_find_general_cachep](#D030024)

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND000100.png)

-----------------------------------------

###### <span id="D030024">\_\_find_general_cachep</span>

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/TB000043.png)

\_\_find_general_cachep() 函数用于通过 size 参数找到 SLAB 分配器提供的频繁
使用的缓存对象。size 参数指向需求的长度，gfpflags 参数用于指明 Buddy 分配器
相关的分配标志. SLAB 分配器向内核提供了经常使用的缓存对象，这些缓存对象如下:

{% highlight c %}
# .name  .name_dma
size-32                 size-32(DMA)
size-64                 size-64(DMA)
size-96                 size-96(DMA)
size-128                size-128(DMA)
size-192                size-192(DMA)
size-256                size-256(DMA)
size-512                size-512(DMA)
size-1024               size-1024(DMA)
size-2048               size-2048(DMA)
size-4096               size-4096(DMA)
size-8192               size-8192(DMA)
size-16384              size-16384(DMA)
size-32768              size-32768(DMA)
size-65536              size-65536(DMA)
size-131072             size-131072(DMA)
size-262144             size-262144(DMA)
size-524288             size-524288(DMA)
size-1048576            size-1048576(DMA)
size-2097152            size-2097152(DMA)
size-4194304            size-4194304(DMA)
{% endhighlight %}

SLAB 分配器可以通过该函数为指定长度找到一个符号要求的缓存对象。这些缓存对象
使用 malloc_sizes 数组进行维护，因此函数 710 行首先将 csizep 指向了 
malloc_sizes 数组，然后在 722 行到 723 行循环中找到第一个满足长度是缓存对象，
最后返回缓存对象.

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND000100.png)

-----------------------------------------

###### <span id="D030023">calculate_slab_order</span>

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/TB000042.png)

calculate_slab_order() 函数用于计算缓存对象的每个 slab 占用的物理页数量，
并返回缓存对象的着色范围. 参数 cachep 指向缓存对象, 参数 size 指向每个缓存
对象的长度; 参数 align 指明缓存对象的对齐方式; 参数 flags 指向分配物理内存
的标志.

SLAB 分配器为缓存对象从 Buddy 分配器中分配一定数量的物理页之后，将这些内存组织
成 slab，slab 基本又两部分组成，第一部分就是 slab 的管理数据部分，第二部部分
又缓存对象组成. SLAB 分配器支持 slab 管理数据位于 slab 内部，也支持 slab 管理
数据位于 slab 外部.

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/TB000008.png)

上图中，slab 管理数据位于 slab 的内部，因此从 Buddy 分配器分配内存之后，将这些
内存的起始部分规划为着色预留区，紧接着是 slab 管理数据去，包括 struct slab 和
kmem_bufctl 数组，最后是又多个缓存对象组成的区域。calculate_slab_order() 函数
的作用就是确保分配的物理内存能够合理的布局三个区域，因此函数计算了各个区域的
数据。函数 1965 行使用 for 循环从一个物理页开始进行布局，在每次布局中，函数在
1969 行处调用 cache_estimate() 函数计算当前数量的物理页是否能合理布局三个区域，
并将可用缓存对象数量存储在 num 变量里，remiander 变量则存储布局完 slab 管理
区域和缓存对象区域之后，剩余的内存数量，这些内存将作为缓存对象着色使用. 1973
行到 1984 行用于处理 slab 管理数据位于 slab 外部的情况，如下图:

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/TB000021.png)

如果 flags 支持 CFLGS_OFF_SLAB 标志，那么 slab 管理数据位于 slab 外部，此时
函数计算 kmem_bufctl 数组的成员数量是否超过 slab 中缓存对象数量. 1987 行设置
了一个 slab 中缓存对象的数量. 1988 行设置了 slab 占用物理页的数量，left_over
变量用于存储 slab 中减去 slab 管理数据和缓存对象区域之后剩余的内存长度，最后
返回该长度.

> [cache_estimate](#D030002)

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND000100.png)

-----------------------------------------

###### <span id="D030022">kmem_cache_zalloc</span>

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/TB000041.png)

kmem_cache_zalloc() 函数用于从缓存对象中分配一个可用并且清零的缓存对象。参数
k 指向缓存对象，参数 flags 指向分配物理内存使用的标志. 311 行函数通过调用
kmem_cache_alloc() 函数并传入 \_\_GFP_ZERO 标志，以此分配一个可用缓存对象，
并将缓存对象对应的内存清零.

> [kmem_cache_alloc](#D030021)

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND000100.png)

-----------------------------------------

###### <span id="D030021">kmem_cache_alloc</span>

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/TB000040.png)

kmem_cache_alloc() 函数用于从缓存对象中分配一个可用的缓存对象. 参数 cachep
指向缓存对象; 参数 flags 用于指明分配物理内存标志. 3575 行函数通过调用
\_\_cache_alloc() 函数分配一个可用缓存对象. 最后返回缓存对象.

> [\_\_cache_alloc](#D030020)

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND000100.png)

-----------------------------------------

###### <span id="D030020">\_\_cache_alloc</span>

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/TB000039.png)

\_\_cache_alloc() 函数用于从缓存对象中分配可用的缓存对象. 参数 cachep 指向
缓存对象; 参数 flags 指向物理内存分配标志; 参数 caller 用于指向申请者.

3407 行函数检测了 flags 参数，以此符合 Buddy 分配器参数要求. 3416 行函数调用
\_\_do_cache_alloc() 函数分配可用的缓存对象，并调用 prefetchw() 函数将可用
缓存对象预加载到 CACHE。3426 行函数如果检测到 flags 参数中包含了 \_\_GFP_ZERO
标志，那么函数调用 memset() 函数将缓存对象指向的内存清零. 最后返回缓存对象.

> [\_\_do_cache_alloc](#D030019)

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND000100.png)

-----------------------------------------

###### <span id="D030019">\_\_do_cache_alloc</span>

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/TB000038.png)

\_\_do_cache_alloc() 函数用于从缓存对象中分配可用的缓存对象. cachep 参数指向
缓存对象, 参数 flags 用于分配物理内存的标志. 函数通过调用 \_\_\_\_cache_alloc()
函数进行分配.

> [\_\_\_\_cache_alloc](#D030018)

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND000100.png)

-----------------------------------------

###### <span id="D030018">\_\_\_\_cache_alloc</span>

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/TB000037.png)

\_\_\_\_cache_alloc() 函数用于从缓存对象中分配可用的缓存对象. 参数 cachep
指向缓存，参数 flags 指向缓存分配内存使用的标志.

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/TB000001.png)

在 SLAB 分配器里，为了加速缓存对象的分配，SLAB 分配器为缓存对象针对每个 CPU
构建了一个缓存栈，CPU 可以快速从缓存栈上获得可用的缓存对象。其实现原理如上图，
缓存对象使用 struct kmem_cache 进行维护，其成员 array 是一个 struct cache_array
数组，数组中的每个成员对应一个缓存栈，因此 struct cache_array 维护着一个缓存栈.

3114 行函数调用 cpu_cache_get() 函数获得缓存对应的缓存栈，3115 行函数检测缓存
栈的栈顶，以此判断缓存栈里是否有可用的缓存对象，如果栈顶不为零，即 avail 成员
不为 0，那么函数执行 3116 到 2118 行代码，直接从缓存栈上获得可用的缓存对象，
并更新缓存栈的栈顶; 如果栈顶为 0，那么表示缓存栈上没有可用的缓存对象，函数
执行 3120 到 3126 行代码，调用 cache_alloc_refill() 函数将一定数量的可用缓存
对象添加到缓存栈上，并再次使用 ac 变量指向缓存栈. 3136 行函数直接返回获得的
可用缓存对象.

> [cpu_cache_get](#D030004)
>
> [cache_alloc_refill](#D030017)

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND000100.png)

-----------------------------------------

###### <span id="D030017">cache_alloc_refill</span>

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/TB000036.png)

cache_alloc_refill() 函数用于为缓存对象的缓存栈填充可用的缓存对象。参数 cachep
指向缓存对象，参数 flags 用于分配物理内存的标志.

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/TB000001.png)

在 SLAB 分配器里，为了加速缓存对象的分配，SLAB 分配器为缓存对象针对每个 CPU
构建了一个缓存栈，CPU 可以快速从缓存栈上获得可用的缓存对象。其实现原理如上图，
缓存对象使用 struct kmem_cache 进行维护，其成员 array 是一个 struct cache_array
数组，数组中的每个成员对应一个缓存栈，因此 struct cache_array 维护着一个缓存栈，
在 struct cache_array 数据结构中，avail 成员指向缓存栈的栈顶, limit 成员指定
了缓存栈的缓存对象最大数量, 超过 limit 之后，缓存栈会将一定数量的缓存对象归还
给 slabs_partial 中的 slab; batchcount 成员用于指明从 slabs_partial 一次性分配
或向 slabs_partial 一次性释放缓存对象的数量; touched 成员用于表面该缓存栈是否
被访问过; entry 数组用于存储一定数量的可用缓存对象. 

每当 SLAB 分配缓存对象的时候，如果发现缓存栈上没有可用的缓存对象时候，那么 SLAB
分配器就会检测缓存对象的 slabs_partial 上是否有可用的 slab，如果有，那么 SLAB
从 slabs_partial 的 slab 上获取 batchcount 数量的缓存对象到缓存栈里; 如果
slabs_partial 上没有可用的 slab，那么 SLAB 分配器检测 slabs_free 上时候有可用
的 slab，如果有，那么就从对应的 slab 上获得 batchcount 数量的缓存对象到缓存栈
上; 如果此时 slabs_free 上也找不到 slab，那么 SLAB 分配器需要为缓存对象扩充新
的 slab，那么 SLAB 分配器从 Buddy 分配指定数量的物理内存，并将这些内存组织成
新的 slab，并插入到 slabs_free 链表上，并重新执行之前的查找过程. 重新查找的过程
中，slabs_free 链表上有可用的 slab，那么缓存栈获得 batchcount 数量的缓存对象
之后，将 slab 从 slabs_free 上移除并插入到 slabs_partial 链表上。

2945 行到 2961 行函数获得缓存对象对应的缓存栈，通过 ac 进行指定，函数还获得
缓存对象的 kmem_list3 链表，通过 l3 进行指定。2969 行函数检测缓存栈的 batchount
是否大于 0，以此确认缓存栈是否在使用，如果小于 1，那么缓存栈不再使用; 2973 行
到 2979 行，函数检测缓存的 slabs_partial 和 slabs_free 上是否有可用的 slab，
如果最终 slabs_free 上都没有找到可用的 slab，那么函数跳转到 must_grow 出分配
新的 slab; 如果找到可用 slab，那么函数继续执行代码。2981 行到 2990 行函数
找打从 slabs_partial/slabs_free 里找到一个可用的 slab. 2992 行到 2999 行，函数
使用一个 while 循环，只要确认缓存对象正在使用的缓存对象数量小于缓存对象总数，
并且 batchcount 不为 0，那么进入循环，在循环中，函数不断从 slab 中将可用缓存
对象加入到缓存栈里，并更新缓存栈的栈顶. 3003 行到 3007 行，函数将 slab 从原先
的链表中移除，如果此时 slab 中没有可用的缓存对象，那么将 slab 插到 slabs_full
链表中，如果此时 slab 中还有部分可用缓存对象，那么将 slab 插入到 slabs_partial
链表中.

3010 行到 3017 行，如果函数直接跳转到 must_grow, 那么 SLAB 会为缓存对象分配新
的 slab，由于 3010 行到 3015 行代码是公用代码，那么函数在 3015 行处继续检测
SLAB 分配器是否在为缓存对象分配新的 slab，如果缓存栈的栈顶为 0，那么 SLAB
分配器就为缓存对象分配新的 slab，此时在 3017 行调用 cache_grow() 函数进行分配
新的 slab，分配完毕之后，函数再次检测缓存栈是否等于 0，如果为真，那么函数重新
从缓存栈上分配新的可用缓存栈. 3027 行到 3028 行，当缓存栈被访问，那么缓存栈
的 touch 标记为 1，并且从缓存栈中返回一个可用缓存对象.

> [cpu_cache_get](#D030004)
>
> [slab_get_obj](#D030005)
>
> [cache_grow](#D030016)
>
> [struct kmem_cache](#D0103)
>
> [struct kmem_list3](#D0100)
>
> [struct slab](#D0102)

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND000100.png)

-----------------------------------------

###### <span id="D030016">cache_grow</span>

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/TB000035.png)

cache_grow() 函数用于扩充缓存对象的 slab。cachep 参数指向缓存对象; flags 参数
用于从 Buddy 分配器中分配内存的标志; nodeid 参数用于指明 NODE 信息; objp 用于
指向 slab.

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/TB000001.png)

SLAB 分配器使用 struct kmem_cache 管理缓存对象，每个缓存对象在指定的 NODE
上维护着 3 个链表，通过 nodelists 成员进行指定。缓存对象的 3 个链表通过
struct kmem_list3 数据结构进行维护，该结构维护 3 种链表，第一种链表维护着
包含部分可用缓存对象的 slab, 称之为 slabs_partial; 第二种链表维护缓存对象
全部可用的 slab, 称之为 slabs_free; 第三种链表维护的 slab 的所有缓存对象都
已经使用, 称之为 slabs_full. 每当 SLAB 分配缓存对象的时候，首先检查 
slabs_partial 上是否有可用的 slab，如果不存在，那么 SLAB 分配器继续查找
slabs_free 上是否有可用的 slab，如果此时还是没有可用的 slab，那么 SLAB 分配
器就会调用 cache_grow() 函数为缓存对象分配新的 slab.

SLAB 分配器在为缓存对象分配新的 slab 时，首先从 Buddy 分配器中分配指定数量的
物理页，然后将这些物理内存进行组织，将其转换成 slab，转换之后在将 slab 对应的
物理页与缓存对象和 slab 进行绑定，最后初始化 slab 的 kmem_bufctl 数组。新的 
slab 创建完毕之后，将其插入缓存对象的 slabs_free 链表。函数 2754 到 2755 行
对分配物理内存的标志进行检查，以此确认分配的物理页是否可以回收或者不可以回收.
2758 到 2767 行，函数获得缓存对象在指定 NODE 的 kmem_list3 数据结构，从中
获得了当前着色信息，并存储在 offset 变量里。函数更新了缓存对象的下一个着色
信息，如果下一个着色信息超过了缓存支持的着色范围，那么将下一个着色信息设置为 0.
2769 行通过获得的着色信息之后计算着色预留的内存长度。2786 到 2789 行，通过
调用函数 kmem_getpages() 函数，SLAB 分配器从 Buddy 分配器中分配指定数量的物
理内存，并获得物理内存对应的虚拟地址，存储中 objp 变量里。2792 行到 2795 行，
函数通过调用 alloc_slabmgt() 函数对新分配的内存组织成一个 slab，无论 slab 采用
外部管理数据还是内部管理数据，函数都返回了 slab 管理数据对应的 struct slab
指针。2797 行函数调用 slab_map_pages() 函数，将从 Buddy 分配器分配的物理页
与缓存对象和 slab 进行绑定。2799 行函数调用 cache_init_objs() 函数初始化 slab
的 kmem_bufctl 数组。2807 到 2810 行函数将 slab 加入到缓存对象的 slabs_free
链表里，并且添加缓存对象的 kmem_list3 的 free_objects 可用对象数量.

> [alloc_slabmgmt](#D030009)
>
> [kmem_getpages](#D030008)
>
> [slab_map_pages](#D030014)
>
> [cache_init_objs](#D030015)

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND000100.png)

-----------------------------------------

###### <span id="D030015">cache_init_objs</span>

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/TB000034.png)

cache_init_objs() 函数用于 slab 创建过程中，初始化 slab 管理数据的 kmem_bufctl
数组. 参数 cachep 指向缓存对象; 参数 slabp 指向 cache。

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/TB000008.png)

SLAB 分配器为缓存对象从 Buddy 分配器分配物理内存之后，将分配的内存通过一定
数据进行组织，将组织之后的数据称为 slab，slab 主要由两部分组成，一部分就是上
图中的管理数据，其包含了 struct slab 和 kmem_bufctl 数组; 另外一部分就是对象
缓存区，区间里被分成缓存对象大小的区块。slab 使用 kmem_bufctl 数组与缓存区
关联在一起，每个缓存对象区的对象都对应 kmem_bufctl 数组里的一个成员。kmem_bufctl
的成员用于指定对应缓存对象的下一个可用缓存对象。

因此 cache_init_objs() 函数就是用于 slab 初始化的时候，初始化 kmem_bufctl 数
组。2625 行开始到 2663 行，函数使用一个 for 循环，从 slab 的第一个缓存对象开始，
遍历 slab 包含的所有缓存对象. 2626 行，函数在每次遍历中，通过 index_to_obj() 
函数获得 kmem_bufctl 数组对应的缓存对象，存储在 objp 变量里。2662 行代码，首先
通过调用 slab_bufctl() 函数获得 slab 对应的 kmem_bufctl 数组，再通过 i 获得
kmem_bufctl 对应的成员，并将其设置为 "i+1"，以此表示当前缓存对象的下一个可用
缓存对象是其后一个缓存对象. 循环结束后，2664 行代码将 slab 的最后一个缓存对象
在 kmem_bufctl 中的成员设置为 BUFCTL_END, 以此表示缓存对象的末尾。通过上面的
处理，所有的缓存初始化为一条单链表，并以 BUFCTL_END 标记结尾.

> [index_to_obj](#D030006)
>
> [slab_bufctl](#D030007)

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND000100.png)

-----------------------------------------

###### <span id="D030014">slab_map_pages</span>

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/TB000033.png)

slab_map_pages() 函数用于将缓存对象使用的物理页与缓存对象和 slab 进行双向
绑定。参数 cache 指向缓存; 参数 slab 指向 slab; 参数 addr 指向物理页对应的
虚拟地址. SLAB 分配器为缓存对象从 Buddy 分配器获得可用的物理页之后，将这些
物理页标记为 PG_slab, 被标记的物理页就不会像非 PG_slab 的物理页一样，其中
物理页对应的 struct 的 lru 成员不再使用，lru 成员包含了两个指针 prev 和 next，
因此 SLAB 将 next 指向了缓存对象，而将 prev 指向了 slab，这样物理页、缓存对象
和 slab 就进行了双向绑定。缓存对象可以通过 list3 的三个链表找到指定的 slab，
slab 也可以直接通过 list3 的三个链表反向找到对应的缓存对象。slab 可以通过其管
理数据 struct slab 的 s_mem 找到缓存对象对应的虚拟地址，由于 slab 分配的内存
都是线性区的内存，因此可以通过 virt_to_page() 获得对应的物理页，物理页对应的
struct page 的 lru 成员又可以找到对应的缓存对象和 slab。

函数的 2723 行代码调用 virt_to_page() 获得 slab 使用的物理页，然后在 2731 到
2735 行代码用于将所有的物理页的 lru prev 和 next 指向缓存对象和 slab.

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND000100.png)

-----------------------------------------

###### <span id="D030013">page_set_slab</span>

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/TB000032.png)

page_set_slab() 函数用于将物理页与 slab 进行绑定。参数 page 指向物理页; 参数
slab 指向 slab。SLAB 分配器为缓存对象从 Buddy 分配器中分配物理内存之后，将这些
物理页标记为 PG_slab, 标记之后这些物理页就不能像普通物理页一样，物理页对应的
struct page 的 lru 成员不再使用，因此将 lru 的 prev 指向了 slab, 这样 slab 和
物理页就能双向绑定，因此代码 527 将 page lru 的 prev 指向了 slab 参数.

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND000100.png)

-----------------------------------------

###### <span id="D030012">page_get_slab</span>

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/TB000031.png)

page_get_slab() 函数用于获得 SLAB 物理页对应的 slab。参数 page 指向物理页。
SLAB 分配器为缓存对象从 Buddy 分配指定的物理页之后，将这些物理页标记为 PG_slab,
被标记之后就不能像普通物理页一样使用，那么物理页对应的 struct page 的 lru 
成员将不再使用，因此 SLAB 将这个 lru 的 prev 指向了物理页对应的 slab，这样
slab 和物理页就双向绑定. 因此函数的 533 行就是从 struct page lru 的 prev 指针
中获得了物理页对应的 slab。

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND000100.png)

-----------------------------------------

###### <span id="D030011">page_get_cache</span>

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/TB000030.png)

page_get_cache() 函数用于获得 SLAB 物理页对应的缓存对象。参数 page 指向物理
页。SLAB 分配为缓存对象从 Buddy 分配器获得物理内存之后，将这些物理页标记为
PG_slab, 标记完毕之后，物理页对应的 struct page 的 lru 成员将不再使用，那么
SLAB 将物理页 struct page lru 的 next 指向了缓存对象，这样缓存和物理页就双向
绑定，因此函数在 522 行通过 lru 的 next 指针获得了缓存对象.

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND000100.png)

-----------------------------------------

###### <span id="D030010">page_set_cache</span>

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/TB000029.png)

page_set_cache() 函数用于将 SLAB 物理页与缓存对象进行关联. 参数 page 指向
物理页; 参数 cache 指向缓存对象. 当 SLAB 分配器从 Buddy 分配器中分配了物理
页之后，将这些物理页标记为 PG_slab 之后，物理对应的 struct page 的 lru 成员
不在通过的物理页逻辑使用，于是将 struct page lru 成员的 next 指向缓存对象，
这样缓存对象可以找到 slab 使用的物理页，物理页也知道自己属于哪个缓存对象.

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND000100.png)

-----------------------------------------

###### <span id="D030009">alloc_slabmgmt</span>

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/TB000028.png)

alloc_slabmgmt() 函数用于将 Buddy 分配器中获得内存构建成一个 slab。参数 cachep
指向缓存对象; 参数 objp 指向从 Buddy 分配器中获得的内存; 参数 colour_off 参数
用于给新的 slab 进行着色; local_flags 用于存储 slab 构建标志; nodeid 参数用于
指定 NODE ID 信息.

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/TB000008.png)

SLAB 分配器从 Buddy 分配器中获得可用内存之后，将这些内存根据一定的规则组建成
slab，正如上图所示，slab 包含两部分，第一部分是 slab 管理数据，由 struct slab
数据结构和 kmem_bufctl 数组构成. 第二部分是缓存对象区域，多个缓存依次排列，
并且 kmem_bufctl 数组中的每个成员对应一个缓存对象，用于指明对应缓存对象的下一
个缓存对象在 kmem_bufctl 数组中的偏移.

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/TB000021.png)

slab 的管理数据也可以位于 slab 外部，如上图。但不论 slab 管理数据位于 slab
内部还是外部，其管理逻辑都是一致的. alloc_slabmgmt() 函数就是用于将从 Buddy
分配器分配的可用内存构建成以上其中一种 slab。2589 行，如果该缓存对象支持外部
slab 管理数据，那么 2591 行就调用 kmem_cache_alloc_node() 函数从 
struct kmem_cache 的 slabp_cache 缓存中分配 slab 管理数据所使用的缓存对象，
这里值得注意的是，如果缓存对象支持 slab 外部管理数据，那么缓存对象 struct
kmem_cache 的 slabp_cache 成员指向一个缓存，用于分配 slab 管理数据内存. 如果
缓存对象支持 slab 管理数据位于 slab 内部，那么 2604 和 2605 行从 Buddy 分配
器获得的可用内存中，将起始地址处预留作为着色使用，然后将 colour_off 指向缓存
对象区的起始偏移处. 2607 行将 slab 中已经分配的缓存对象设置为 0，即 inuse 设置
为 0，然后将缓存的着色信息设置为 colour_off, 接着将管理数据 struct slab 的 
s_mem 指向了缓存对象区的起始地址，最后将 free 设置为 0，以此表示第一个可用
缓存对象的偏移值为 0. 最后返回 slabp 的地址.

> [struct slab](#D0102)

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND000100.png)

-----------------------------------------

###### <span id="D030008">kmem_getpages</span>

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/TB000027.png)

kmem_getpages() 函数用于从 Buddy 分配器指定数量的物理页并返回物理页对应的
虚拟地址. 参数 cachep 指向缓存对象; 参数 flags 用于从 Buddy 分配器分配物理
页使用的分配标志; nodeid 用于指明从哪个 NODE 上分配.

函数 1624 行到 1626 行代码用于处理分配物理页时候使用的分配标志，如果缓存对象
支持 SLAB_RECLAIM_ACCOUNT, 即支持页回收，那么 flags 添加 \_\_GFP_RECLAIMABLE.
接着调用 alloc_pages_exact_node() 函数从指定的 NODE 分配 "1 << cachep->gfporder"
个物理页，如果分配失败，则直接返回 NULL. 1632 行计算出分配的物理页数量，存储
在 nr_pages 变量里. 如果缓存对象支持 SLAB_RECLAIM_ACCOUNT 标志，那么调用函数
add_zone_page_state() 统计物理页对应的 ZONE 分区可回收页的数量; 反之如果不支持
SLAB_RECLAIM_ACCOUNT 标志，那么调用 add_zone_page_state() 函数统计物理页对应的
ZONE 分区不可回收页的数量. 代码 1639 到 1640 行，将从 Buddy 分配器中获得的物理
页标记为 PG_slab, 以此告诉系统这些物理页属于 SLAB. 函数最后滴啊用 page_address()
函数返回物理页的起始虚拟地址.

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND000100.png)

-----------------------------------------

###### <span id="D030007">slab_bufctl</span>

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/TB000026.png)

slab_bufctl() 函数用于获得 slab 管理数据之一的 kmem_bufctl 数组. slab 参数
用于指向一个 slab。

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/TB000008.png)

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/TB000021.png)

无论 slab 将 slab 管理数据放置在 slab 内部还是 slab 外部，管理数据 struct slab
和 kmem_bufctl 数组都是相连在一起的. 因此通过 slab 可以获得 kmem_bufctl 数组。
函数 2617 行就是通过获得 struct slab 的下一个 struct slab 地址就是 kmem_bufctl
数组.

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND000100.png)

-----------------------------------------

###### <span id="D030006">index_to_obj</span>

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/TB000025.png)

index_to_obj() 函数用于通过 slab 的 kmem_bufctl 数组的索引获得 slab 中缓存对象。
参数 cache 指向缓存对象; 参数 slab 指向指定的 slab; 参数 idx 指向了缓存对象
在 slab kmem_bufctl 数组中的索引.

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/TB000007.png)

SLAB 分配器使用 struct slab 和 kmem_bufctl 数组管理 slab 中缓存对象，kmem_bufctl
数组中的每个成员对应一个缓存对象. slab 的管理数据 struct slab 的 s_mem 成员
用于指定缓存对象在 slab 中的起始地址，cache 参数的 buffer_size 成员指明了每个
缓存对象的长度，因此通过 s_mem 和 buffer_size 成员，以及 idx 是可以唯一确定
一个缓存对象，因此使用 index_to_obj() 函数可以通过一个 idx 索引获得一个缓存
对象.

> [struct slab](#D0102)

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND000100.png)

-----------------------------------------

###### <span id="D030005">slab_get_obj</span>

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/TB000024.png)

slab_get_obj() 函数用于从 slab 中获得可用的缓存对象. cachep 参数指向缓存;
参数 slabp 指向 slab; 参数 nodeid 指向 NODE ID. SLAB 分配器从 Buddy 获得可用
内存之后，将这些内存构建成 slab，并使用一定的数据管理 slab，slab 里面存在一定
量的可用缓存对象。slab 通过 struct slab 数据结构和 kmem_bufctl 数组进行管理，
如下图:

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/TB000008.png)

struct slab 和 kmem_bufctl 数组可以位于 slab 内部，也可以位于 slab 外部，上图
所示的 slab 管理数据位于 slab 内部. kmem_bufctl 数组的每个成员对应 slab 中的
一个缓存对象，用于标记对应缓存对象的下一个可用缓存对象在 kmem_bufctl 数组中的
偏移.

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/TB000007.png)

管理数据 struct slab 中，free 成员用于指明 slab 第一个可用缓存对象在 kmem_bufctl
数组中的偏移. 函数 2680 行调用 index_to_obj() 函数，通过 free 指向的索引获得
slab 中第一个可用的缓存对象, 存储在 objp 变量里，2683 行更新 struct slab 的 
inuse 成员，以此表示该 slab 中缓存对象又有一个被使用. 2684 行调用 slab_bufctl()
函数获得 free 对应缓存对象的下一个可用缓存对象，并将下一个可用缓存对象在 
kmem_bufctl 数组中的索引存储在 next 变量里. 2689 行将管理数据 struct slab 的
free 成员设置为 next 指向的值，以此更新该 slab 第一个可用缓存对象在 kmem_bufctl
数组中的偏移. 2691 行返回从 slab 中获得的缓存对象.

> [slab_bufctl](#D030007)
>
> [index_to_obj](#D030006)

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND000100.png)

-----------------------------------------

###### <span id="D030004">cpu_cache_get</span>

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/TB000023.png)

cpu_cache_get() 用于获得缓存对象的缓存栈。SLAB 分配器为每个 CPU 分配了缓存对象
的缓存栈，用于从缓存栈上快速获得可用的对象. 如下图:

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/TB000001.png)

每个 CPU 都有一个缓存对象的缓存栈，缓存通过 struct kmem_cache 数据结构进行
管理，array 成员是一个数组结构，每个 CPU 对应数组中的一个成员，通过 array
数组就可以找到 CPU 对应的缓存栈，因此 704 行代码，函数通过 smp_processor_id()
函数获得当前 CPU 的 ID，然后从 struct kmem_cache 的 array 数组中获得指定的 
struct array_cache 数据结构，从而获得缓存栈.

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND000100.png)

-----------------------------------------

###### <span id="D030003">slab_mgmt_size</span>

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/TB000022.png)

slab_mgmt_size() 函数用于计数一个缓存对象的 slab 管理数据长度。nr_objs 参数
指明了缓存对象的数量; 参数 align 指明缓存对象的对齐方式.

SLAB 分配器从 Buddy 分配器分配可用内存之后，将其规划为 slab，slab 中包含了多个
缓存对象。slab 是一定的管理数据管理这些缓存对象的分配和释放，slab 使用一个 
struct slab 数据结构和一个 kmem_bufctl 数组进行管理，其中 kmem_bufctl 中的每个
成员正好对应 slab 中一个缓存对象，因此 kmem_bufctl 数组的长度由 slab 中缓存对象
的数量而定. 因此 slab 管理数据的长度由 struct slab 的长度和 kmem_bufctl 数组
长度决定，最后如 744 行定义的算法一致. 

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND000100.png)

-----------------------------------------

###### <span id="D030002">cache_estimate</span>

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/TB000020.png)

cache_estimate() 函数用于计算从 Buddy 分配器中获得指定长度内存之后，这些
内存一共可以分配多少个可用对象，以及可以支持多少中着色. 参数 gfporder 指向了
从 Buddy 分配器中获得物理页的个数; buffer_size 指明了缓存对象的长度; align
指明了缓存对象的对齐方式; flags 存储缓存对象相关的 SLAB 标志; left_over 用于
指明分配的内存减去可用对象占用的内存之后，剩余的内存长度; num 用于存储指定
长度的内存可分配对象的数量.

SLAB 分配器在分配每个缓存对象的时候，首先从 Buddy 分配器中获得可用的内存，
然后通过一定的组织方式将这些内存组织成为 slab，slab 是 SLAB 管理的单元，slab
里面包含了多个缓存对象。SLAB 分配器采用 slab 管理数据管理每个 slab，但由于
slab 管理数据支持 slab 内部管理，也支持 slab 外部管理，如下图:

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/TB000008.png)

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/TB000021.png)

无论采用那种方式管理 slab，slab 最前端都是为着色预留的内存空间，其后是多个
缓存对象，但是由于从 Buddy 分配器分配的内存都是按 PAGE_SIZE 分配的，那么以上
的结构可能会在 slab 的结尾处剩余一部分的内存区域，这里称为 Left_Over, 因此
系统会调用 cache_estimate() 函数计算出两种模式下，指定长度的内存可以划分多少
个缓存对象，以及剩余的内存长度.

756 行首先计算了从 Buddy 中获得内存的长度，存储在 slab_size 变量里。当该缓存
对象分配时支持 CFLGS_OFF_SLAB 标志，那么 slab 的管理数据将位于 slab 之外，
那么函数执行 773 行到 778 行逻辑，因此 slab 管理数据位于 slab 之外，那么
mgmt_size 的值为 0，接着计算这段内存最大容纳对象的数量，如果计算的结果超出
SLAB 分配器规定的最大缓存数量，那么将 nr_objs 设置为 SLAB_LIMIT; 如果分配
该缓存对象时不支持 CFLGS_OFF_SLAB 标志，那么 slab 管理数据位于 slab 内部，
那么在计算最大缓存对象数量的时候，也要从可用内存中减去 slab 管理数据占用的
长度。slab 管理数据包含了 struct slab 以及 kmem_bufctl 数组，因此 788 行到
789 行处计算了可分配的缓存对象数量。795 行到 797 行检测了在计算出的可分配
对象占用的内存和管理数据占用的内存是否大于从 Buddy 分配器分配的内存，如果
大于，那么将可分配缓存对象数减一. 799 到 800 行同样检测可分配缓存对象数量是否
查过 SLAB 分配器规定的最大数量, 如果超过，那么将可分配缓存对象数量设置为
SLAB_LIMIT。最后在这种模式下，将 slab 管理数据的长度存储在 mgmt_size 变量.
函数最后将可分配缓存对象数量存储在 num 变量里，并计数出剩余内存的长度，存储
在 left_over 变量里.

> [slab_mgmt_size](#D030003)

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND000100.png)

-----------------------------------------

###### <span id="D0108">struct ccupdate_struct</span>

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/TB000053.png)

struct ccupdate_struct 数据结构用于更新高速缓存对应的本地缓存.

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/TB000001.png)

每个高速缓存使用 struct kmem_cache 数据结构进行维护，为了加速每个 CPU 从高速
缓存中分配可用的缓存对象，SLAB 分配器为每个 CPU 创建了本地缓存，使用
struct cache_array 数据结构进行维护. 本地缓存中维护了一个缓存栈，CPU 可以从
这个缓存栈上快速获得可用的缓存对象.

在支持 CPU 热插拔系统里，如果 CPU 离线了但没有释放本地缓存，那么 SLAB 使用
struct ccupdate_struct 数据结构协助高速缓存更新 CPU 本地缓存. 因此 cachep 
成员用于指向需要更新的高速缓存，new 成员用于指向高速缓存新的本地缓存.

> [do_tune_cpucache](#D030033)

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND000100.png)

-----------------------------------------

###### <span id="D0107">struct arraycache_init</span>

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/TB000017.png)

SLAB 分配器初始化阶段，出现了 "鸡生蛋" 问题，为了解决这个问题，SLAB 分配器
使用 struct arraycache_init 结构静态定义了 cache_array 缓存栈提供了 SLAB 分配
器初始化使用.

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/TB000018.png)

SLAB 分配器定义了 initarray_cache 数组用于给 struct kmem_list3 和 
struct cache_array 缓存对象提供了初始化所需的 cache_array 缓存栈.

> [struct array_cache](#D0101)

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND000100.png)

-----------------------------------------

###### <span id="D0106">struct cache_names</span>

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/TB000016.png)

SLAB 分配器从 Buddy 分配器获得指定数量的物理页之后，着色完毕之后并使用 
struct slab 管理这些内存，称为 slab. 其结构如下:

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/TB000008.png)

在每个 slab 中，起始处是为着色预留的区间，接下来是 struct slab 结构，该结构
就是用于管理 slab 的数据，紧接着就是一个 kmem_bufctl 数组，该数组是由 
kmem_bufctl_t 类型的成员组成的数组。接下来是对象区，这就是用于分配的对象。
在 kmem_bufctl 数组中，每个成员按顺序与每个对象进行绑定，用于表示该对象的下
一个对象的索引，并且最后一个对象的值为 BUFCTL_END, 这样将 slab 中的所有对象
通过一个单链表进行维护。通过上面的分析，kmem_bufctl_t 数据结构构造的数据用于
指向当前对象指向的下一个对象的索引.

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND000100.png)

-----------------------------------------

###### <span id="D0105">struct cache_names</span>

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/TB000014.png)

SLAB 分配器为内核中频繁使用的特定长度分配缓存对象，由于缓存对象是有名的，
因此使用 struct cache_names 用于指明缓存对象的名字. name 成员用于指明缓存
对象的名字，name_dma 成员用于指明缓存对象 DMA 部分的名字.

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/TB000015.png)

SLAB 分配器通过上面的代码定义了多种长度的名字，包含了 DMA 部分的名字，通过
上面的定义，SLAB 为了特定长度定义的名字如下:

{% highlight c %}
# .name  .name_dma
size-32		        size-32(DMA)
size-64		        size-64(DMA)
size-96		        size-96(DMA)
size-128		size-128(DMA)
size-192		size-192(DMA)
size-256		size-256(DMA)
size-512		size-512(DMA)
size-1024		size-1024(DMA)
size-2048		size-2048(DMA)
size-4096		size-4096(DMA)
size-8192		size-8192(DMA)
size-16384		size-16384(DMA)
size-32768		size-32768(DMA)
size-65536		size-65536(DMA)
size-131072		size-131072(DMA)
size-262144		size-262144(DMA)
size-524288		size-524288(DMA)
size-1048576		size-1048576(DMA)
size-2097152		size-2097152(DMA)
size-4194304		size-4194304(DMA)
{% endhighlight %}

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND000100.png)

-----------------------------------------

###### <span id="D0104">struct cache_sizes</span>

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/TB000011.png)

SLAB 分配为系统中经常使用的特定长度分配缓存对象，使用 struct cache_sizes 进行
定义。在结构中 cs_size 成员用于指定缓存对象的长度，成员 cs_cachep 用于指向指定
长度缓存的 struct kmem_cache 指针. 如果系统支持 CONFIG_ZONE_DMA 宏，即支持
ZONE_DMA 分区，那么 DMA 分区提供了对应的缓存对象.

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/TB000012.png)

内核使用上面的代码，定义了内核经常使用的特定长度分配内存对象，在上面定义中，
通过 kmalloc_sizes.h 头文件进行转换:

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/TB000013.png)

上图即 kmalloc_sizes.h 头文件定义，通过上面的转换，内核支持经常使用指定长度
缓存对象如下:

{% highlight c %}
# SIZE
size-32
size-64
size-96
size-128
size-192
size-256
size-512
size-1024
size-2048
size-4096
size-8192
size-16384
size-32768
size-65536
size-131072
size-262144
size-524288
size-1048576
size-2097152
size-4194304
{% endhighlight %}

> [struct kmem_cache](#D0103)

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND000100.png)

-----------------------------------------

###### <span id="D0103">struct kmem_cache</span>

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/TB000009.png)

SLAB 分配器使用 struct kmem_cache 结构管理一个缓存对象. SLAB 分配器从 Buddy
分配器中分配器内存之后，使用 struct slab 管理这些内存，并称为 slab，SLAB 分配器
将 slab 维护在对应的 slabs_partial、slabs_free 或 slabs_full 链表中。slab
中存在可用的对象，但 SLAB 分配器为了加速每个 CPU 获得对象的速率，为对象针对
每个 CPU 创建了一个缓存栈，缓存栈使用 struct cache_array 进行管理，SLAB 分配器
平时从 slab 中将一定数量的可用对象加入到每个 CPU 对应的缓存栈上，那么当对应
CPU 分配对象的时候，那么 SLAB 直接从缓存栈上分配对象，反之当 CPU 使用完对象
之后，将对象直接归还给缓存栈，并不直接返回给 slab，这样有利于局部性原理，使
经常使用的对象不被换出 CACHE。为了增加对象在 CACHE 中的命中率，SLAB 为每个
slab 进行着色，这回让每个对象缓存到不同的 CACHE LINE 上，其架构如下:

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/TB000001.png)

array 成员用于为每个 CPU 指向各自的缓存栈 struct cache_array, CPU 分配对象
的时候通过该成员找到指定的缓存栈进行对象的分配和释放动作. buffer_size 用于
指明对象对齐之后的长度, 由于是对齐之后的结果，因此和对象原始长度可能存在差异;
flags 用于指明对象在 SLAB 分配器中的标志; num 成员用于指明每个 slab 上对象
的数量; gfporder 用于指明该缓存对象从 Buddy 分配器分配物理页的数量; gfpflags
用于在从 Buddy 分配器中分配物理页的分配标志; colour 用于指明该缓存对象最大
可着色范围; colour_off 成员用于指定每次着色偏移; slab_size 成员用于指定管理
数据在 slab 的长度; ctor 用于分配对象时候调用的构造函数; name 成员用于指明
缓存对象的名字; next 用于将缓存对象添加到系统的缓存链表 cache_chain 上.

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/TB000010.png)

nodelists 成员是 SLAB 分配器用于为每个 NODE 创建的 kmem_list3, 在 kmem_list3
中一共维护三种链表，分别是 slabs_partial 链表用于维护包含部分可用对象的 slab，
slabs_free 链表用于维护全部对象可用的 slab，slabs_full 链表用于维护全部对象
已经分配的 slab. SLAB 分配器为了让缓存对象从 Buddy 分配器分配的物理内存来自
同一 NODE，因此为每个 NODE 都创建了 kmem_list3 进行维护.

> [struct array_cache](#D0101)
>
> [struct kmem_list3](#D0100)

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND000100.png)

-----------------------------------------

###### <span id="D0102">struct slab</span>

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/TB000006.png)

在 SLAB 分配器中，SLAB 分配器使用 struct kmem_cache 管理一个缓存对象，SLAB
分配器从 Buddy 分配器中为对象分配内存之后，使用 struct slab 管理这些内存，
将内存分作两部分，第一部分是管理部分，第二部分就是可用对象. 如下图:

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/TB000008.png)

SLAB 分配器从 Buddy 分配器中以 PAGE_SIZE 为单位分配一定数量的内存之后，SLAB
分配器对该内存进行着色，也就是在这段内存的起始地址预留一段内存，在这段内存
之后是用于管理这段内存的 struct slab 结构, struct slab 结构包含了管理这段
内存的数据，其之后是一个 kmem_bufctl 数组，数组之后的内存，SLAB 分配器将其
分成对象大小的内存块，这些内存块就是该对象缓存。kmem_bufctl 数组中的每个成员
对应着一个缓存，用于指定该缓存的下一个缓存索引，这样就可以将这些缓存链接成
一个单链表，kmem_bufctl 数组的最后一个成员标记为 BUFCTL_END, 用于指示这段
内存的最后一个对象. 通过这样管理的内存在 SLAB 分配器中称为 slab, 具体如下图:

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/TB000007.png)

在 SLAB 分配器中，struct slab 可以位于 slab 内部，也可以位于 slab 外部，但
不论位于 slab 内部或外部，其成员的含义都是一致的。colouroff 成员用于指明该
slab 着色总长度，其包含了着色、struct slab 以及 kmem_bufctl array 的总长度，
这样可用对象加载到 CACHE 之后就会被插入到不同的 CACHE 行; s_mem 用于指向
slab 中对象的起始地址; inuse 成员用于指定 slab 中可用对象的数量; free 成员
用于指向 slab 当前可用对象在 kmem_bufctl 的索引; nodeid 成员用于指向该 slab
位于哪个 NODE; list 成员用于将 slab 加入到 struct kmem_cache 的 kmem_list3
中三个链表中的其中一个链表，如下图:

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/TB000001.png)

如果 slab 中的所有对象都是可用的，那么 slab 会被加入到 slabs_free 链表中;
如果 slab 中的所有对象都是不可用的，那么 slab 会被加入到 slabs_full 链表中;
如果 slab 中的对象部分可用，那么 slab 会被加入到 slab_partial 链表中.
slab 管理数据也可以位于 slab 外部，如下图:

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/TB000021.png)

在 SLAB 分配器中，分配对象时如果添加了 CFLGS_OFF_SLAB 标志，那么 SLAB 分配器
会将从 Buddy 分配器获得的内存制作成 slab，并将 slab 的管理数据独立与 slab。
将从 Buddy 分配器中获得的内存进行着色，着色值存储在 slab 管理数据的 colouroff
里，然后使用 slab 管理数据的 s_mem 指向 slab 对象的起始地址。kmem_bufctl 数组
对应着 slab 中的每个对象。

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND000100.png)

-----------------------------------------

###### <span id="D0101">struct array_cache</span>

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/TB000004.png)

SLAB 分配器为对象创建了每个 CPU 缓存栈，通过使用 struct array_cache 结构
进行维护。每个 CPU 都可以从各自的 array_cache 中获得可用的对象, 如下图:

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/TB000001.png)

在上图中，CPU 要从 SLAB 分配器中获得可用的对象，那么 SLAB 分配器会从对应的
struct kmem_cache array 成员中找到指定的 struct array_cache 结构，在该结构
中，avail 成员用于指定该缓存栈中存在多少个可用的对象; limit 成员用于指定该
缓存栈最多可以容纳的对象数量; batchcount 成员用于指定该缓存栈从 slabs_partial
的 slab 获得或释放对象的数量; toched 成员用于指定该缓存栈是否被访问过; lock
成员用于操作该缓存栈添加锁; entry 成员是一个零数组，数组构成了一个 LIFO 的
缓存栈，栈上的每个成员就是一个可用的对象，并且 avail 用于维护栈顶. 对于 SMP
结构，存在多个 CPU，那么 struct kmem_cache 维护多个 struct array_cache，如下图:

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/TB000005.png)

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND000100.png)

-----------------------------------------

###### <span id="D0100">struct kmem_list3</span>

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/TB000002.png)

SLAB 分配器使用 struct kmem_cache 维护一个缓存对象, 其中包含了成员
nodelist_list3, 其就是一个 struct kmem_list3 结构。SLAB 分配器为缓存
对象在每个 NODE 上都分配一个 struct kmem_list3 结构，struct kmem_list3 结构
在每个 NODE 上维护三个链表，每个链表上都维护一定数量的 slab. 如下图:

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/TB000001.png)

slabs_partial 成员用于维护一个链表，链表上的成员是包含部分可用对象的 slab，
SLAB 分配器可以从链表的成员中获得可用的对象; slabs_full 成员同样用于维护一
个链表，链表上的成员的 slab 上的所有对象全部被分配，SLAB 分配器不会从这些
slab 中查找可用的对象; slabs_free 成员也用于维护一个链表，链表上的 slab 成员
的对象全部可以使用，SLAB 在 slabs_partial 链表上找不到可用的对象之后，就从
slabs_free 链表上查找一个 slab，并从该 slab 中查找一个可用的对象返回给对象，
最后将该 slab 加入到 slabs_partial 链表上.

free_objects 成员用于指定该 NODE 上所有可用对象的数量. free_limit 成员
用于指定该 NODE 上，三个链表维护最大可用对象的数量，当可用对象数量超过
free_limit 之后，SLAB 分配器将 slabs_free 上的 slab 释放给 Buddy 分配器.
colour_next 成员用于为每个 slab 进行着色，如下图:

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/TB000003.png)

每当 SLAB 分配器从 Buddy 分配器中获得可用的内存之后，SLAB 分配器会在这段内存
的起始地址处预留 colour_next 的空间作为 SLAB 着色。每当完成一次着色之后，
colour_next 将会增加 struct kmem_cache colour_off 成员对应的长度，这样就
能将不同的 slab 着上不同的色. list_lock 成员是一个 spinlock 锁，由于对 
struct kmem_list3 操作时进行上锁操作.

> [struct array_cache](#D0101)

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND000100.png)

-----------------------------------------

###### <span id="D030000">kmem_list3_init</span>

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/TB000000.png)

kmem_list3_init() 函数用于初始化 struct kmem_list3 结构实例。在 SLAB 中，
struct kmem_cache 结构中包含了 nodelists 成员，该成员为每个 NODE 指向一个
struct kmem_list3 结构。

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/TB000001.png)

如上图所示，struct kmem_cache 为每个 NODE 准备了一个 struct kmem_list3 结构，
在 struct kmem_list3 结构中，包含了 3 个链表，每个链表维护一定数量的 SLAB。
slab_partial 链表上维护的 SLAB 中包含了一定可用数量的 object; slab_full 链表
上维护的 SLAB 中没有可用的 object; slab_free 链表上维护的 SLAB 中所有的
object 都是可用的. 

> [struct kmem_list3](#D0100)

函数中，343 行到 345 行分别初始化了 struct kmem_list3 结构体中 3 个链表.
346 行和 347 行初始化了 shared 成员和 alien 成员; 348 行初始化了 colour_next
成员，因此着色从 0 开始; 349 行初始化了 list_lock spinlock 锁; 350 行和 351 行
将 free_objects 成员设置为 0，以此表示当前没有可用的对象，free_touch 成员
设置为 0 表示 struct kmem_list3 未被访问过.

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND000100.png)

-----------------------------------------

###### <span id="D030001">set_up_list3s</span>

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/TB000019.png)

SLAB 分配器通过 struct kmem_cache 管理缓存对象，其中包含了成员 nodelists 
成员，该成员用于为系统每个 NODE 分配一个 struct kmem_list3 结构，在 struct
kmem_list3 结构中维护来自当前 NODE 的物理页构成的 slab，因此 set_up_list3s()
函数用于设置缓存对象每个 NODE 的 kmem_list3 结构.

参数 cachep 指向缓存对象的 struct kmem_cache; 参数 index 指向 NODE 偏移.
1363 行到 1368 行用于遍历当前系统所支持的 NODE 节点，在每次遍历中，将
nodelists 成员设置为 initkmem_list3 数组中的特定 struct kmem_list3 成员.
由于该函数只用于 SLAB 分配器初始化阶段，因此标记上 \_\_init.

> [struct kmem_cache](#D0103)
>
> [struct kmem_list3](#D0100)

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

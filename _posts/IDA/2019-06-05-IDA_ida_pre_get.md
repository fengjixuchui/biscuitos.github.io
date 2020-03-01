---
layout: post
title:  "ida_pre_get"
date:   2019-06-05 05:30:30 +0800
categories: [HW]
excerpt: IDA ida_pre_get().
tags:
  - Tree
---

![DTS](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND00000T.jpg)

> [Github: ida_pre_get](https://github.com/BiscuitOS/HardStack/tree/master/Algorithem/IDA/API/ida_pre_get)
>
> Email: BuddyZhang1 <buddy.zhang@aliyun.com>

# 目录

> - [源码分析](#源码分析)
>
> - [实践](#实践)
>
> - [附录](#附录)

-----------------------------------

# <span id="源码分析">源码分析</span>

{% highlight bash %}
int ida_pre_get(struct ida *ida, gfp_t gfp)
{
        /*
         * The IDA API has no preload_end() equivalent.  Instead,
         * ida_get_new() can return -EAGAIN, prompting the caller
         * to return to the ida_pre_get() step.
         */
        if (!__radix_tree_preload(gfp, IDA_PRELOAD_SIZE))
                preempt_enable();

        if (!this_cpu_read(ida_bitmap)) {
                struct ida_bitmap *bitmap = kzalloc(sizeof(*bitmap), gfp);
                if (!bitmap)
                        return 0;
                if (this_cpu_cmpxchg(ida_bitmap, NULL, bitmap))
                        kfree(bitmap);
        }

        return 1;
}
{% endhighlight %}

ida_pre_get() 函数用于预先从系统内分配一定大小的 struct ida_bittmap 给
IDA 使用。参数 ida 指向 IDA 的根；gfp 指定分配内存时使用的标志。
函数首先调用 __radix_tree_preload() 函数从 IDA 对应的 radix_tree 中获得
指定的空间用于存放 IDA 所使用节点。IDA 在 radix-tree 中使用的节点为一般节点
和 exceptional 节点。函数继续调用 this_cpu_read() 函数去获得本 CPU 对应的
ida_bitmap 变量，如果该变量为空，那么调用 kzalloc() 给 bitmap 分配指定的
内存，并初始化内存为 0. 接着调用 this_cpu_cmpxchg() 函数将 bitmap 的地址
赋值给了 ida_bitmap. 至此内核就为指定 CPU 分配了 ida_bitmap.

--------------------------------------------------

# <span id="实践">实践</span>

> - [驱动源码](#驱动源码)
>
> - [驱动安装](#驱动安装)
>
> - [驱动配置](#驱动配置)
>
> - [驱动编译](#驱动编译)
>
> - [驱动运行](#驱动运行)
>
> - [驱动分析](#驱动分析)

#### <span id="驱动源码">驱动源码</span>

{% highlight c %}
/*
 * IDA.
 *
 * (C) 2019.06.04 <buddy.zhang@aliyun.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/mm.h>

/* header of IDA/IDR */
#include <linux/idr.h>
#include <linux/radix-tree.h>

/* Root of IDA */
static DEFINE_IDA(BiscuitOS_ida);

static __init int ida_demo_init(void)
{
	int id, i;

	ida_pre_get(&BiscuitOS_ida, GFP_KERNEL);

	/* Allocate an unused ID */
	id = ida_alloc(&BiscuitOS_ida, GFP_KERNEL);
	printk("IDA-ID: %#x\n", id);

	/* Release an ID */
	ida_free(&BiscuitOS_ida, id);

	return 0;
}
device_initcall(ida_demo_init);
{% endhighlight %}

#### <span id="驱动安装">驱动安装</span>

驱动的安装很简单，首先将驱动放到 drivers/BiscuitOS/ 目录下，命名为 ida.c，
然后修改 Kconfig 文件，添加内容参考如下：

{% highlight bash %}
diff --git a/drivers/BiscuitOS/Kconfig b/drivers/BiscuitOS/Kconfig
index 4edc5a5..1a9abee 100644
--- a/drivers/BiscuitOS/Kconfig
+++ b/drivers/BiscuitOS/Kconfig
@@ -6,4 +6,14 @@ if BISCUITOS_DRV
config BISCUITOS_MISC
        bool "BiscuitOS misc driver"
+config BISCUITOS_IDA
+       bool "IDA"
+
+if BISCUITOS_IDA
+
+config DEBUG_BISCUITOS_IDA
+       bool "ida_pre_get"
+
+endif # BISCUITOS_IDA
+
endif # BISCUITOS_DRV
{% endhighlight %}

接着修改 Makefile，请参考如下修改：

{% highlight bash %}
diff --git a/drivers/BiscuitOS/Makefile b/drivers/BiscuitOS/Makefile
index 82004c9..9909149 100644
--- a/drivers/BiscuitOS/Makefile
+++ b/drivers/BiscuitOS/Makefile
@@ -1 +1,2 @@
obj-$(CONFIG_BISCUITOS_MISC)     += BiscuitOS_drv.o
+obj-$(CONFIG_BISCUITOS_IDA)     += ida.o
--
{% endhighlight %}

#### <span id="驱动配置">驱动配置</span>

驱动配置请参考下面文章中关于驱动配置一节。在配置中，勾选如下选项，如下：

{% highlight bash %}
Device Driver--->
    [*]BiscuitOS Driver--->
        [*]IDA
            [*]ida_pre_get()
{% endhighlight %}

具体过程请参考：

> [Linux 5.0 开发环境搭建 -- 驱动配置](https://biscuitos.github.io/blog/Linux-5.0-arm32-Usermanual/#%E9%A9%B1%E5%8A%A8%E9%85%8D%E7%BD%AE)

#### <span id="驱动编译">驱动编译</span>

驱动编译也请参考下面文章关于驱动编译一节：

> [Linux 5.0 开发环境搭建 -- 驱动编译](https://biscuitos.github.io/blog/Linux-5.0-arm32-Usermanual/#%E7%BC%96%E8%AF%91%E9%A9%B1%E5%8A%A8)

#### <span id="驱动运行">驱动运行</span>

驱动的运行，请参考下面文章中关于驱动运行一节：

> [Linux 5.0 开发环境搭建 -- 驱动运行](https://biscuitos.github.io/blog/Linux-5.0-arm32-Usermanual/#%E9%A9%B1%E5%8A%A8%E8%BF%90%E8%A1%8C)

启动内核，并打印如下信息：

{% highlight bash %}
usbcore: registered new interface driver usbhid
usbhid: USB HID core driver
IDA-ID: 0x0
aaci-pl041 10004000.aaci: ARM AC'97 Interface PL041 rev0 at 0x10004000, irq 24
aaci-pl041 10004000.aaci: FIFO 512 entries
oprofile: using arm/armv7-ca9
{% endhighlight %}

#### <span id="驱动分析">驱动分析</span>

创建 IDA bitmap 内存池。

-----------------------------------------------

# <span id="附录">附录</span>

> [BiscuitOS Home](https://biscuitos.github.io/)
>
> [BiscuitOS Driver](https://biscuitos.github.io/blog/BiscuitOS_Catalogue/)
>
> [BiscuitOS Kernel Build](https://biscuitos.github.io/blog/Kernel_Build/)
>
> [Linux Kernel](https://www.kernel.org/)
>
> [Bootlin: Elixir Cross Referencer](https://elixir.bootlin.com/linux/latest/source)
>
> [搭建高效的 Linux 开发环境](https://biscuitos.github.io/blog/Linux-debug-tools/)

## 赞赏一下吧 🙂

![MMU](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/HAB000036.jpg)

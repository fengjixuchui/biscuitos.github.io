---
layout: post
title:  "ida_alloc_range"
date:   2019-06-05 05:30:30 +0800
categories: [HW]
excerpt: IDA ida_alloc_range().
tags:
  - Tree
---

![DTS](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND00000T.jpg)

> [Github: ida_alloc_range](https://github.com/BiscuitOS/HardStack/tree/master/Algorithem/IDA/API/ida_alloc_range)
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
/**
 * ida_alloc_range() - Allocate an unused ID.
 * @ida: IDA handle.
 * @min: Lowest ID to allocate.
 * @max: Highest ID to allocate.
 * @gfp: Memory allocation flags.
 *
 * Allocate an ID between @min and @max, inclusive.  The allocated ID will
 * not exceed %INT_MAX, even if @max is larger.
 *
 * Context: Any context.
 * Return: The allocated ID, or %-ENOMEM if memory could not be allocated,
 * or %-ENOSPC if there are no free IDs.
 */
int ida_alloc_range(struct ida *ida, unsigned int min, unsigned int max,
                        gfp_t gfp)
{
        int id = 0;
        unsigned long flags;

        if ((int)min < 0)
                return -ENOSPC;

        if ((int)max < 0)
                max = INT_MAX;

again:
        xa_lock_irqsave(&ida->ida_rt, flags);
        id = ida_get_new_above(ida, min);
        if (id > (int)max) {
                ida_remove(ida, id);
                id = -ENOSPC;
        }
        xa_unlock_irqrestore(&ida->ida_rt, flags);

        if (unlikely(id == -EAGAIN)) {
                if (!ida_pre_get(ida, gfp))
                        return -ENOMEM;
                goto again;
        }

        return id;
}
EXPORT_SYMBOL(ida_alloc_range);
{% endhighlight %}

ida_alloc_range() 函数用于从 IDA 获得一个指定范围内未使用的 ID。
参数 ida 指向 IDA 根；参数 min 代表最小 ID 数；参数 max 代表最大
ID 树；参数 gfp 代表分配时使用的标志。函数首先检查 min 和 max，如果
min 小于 0，那么直接返回错误码 ENOSPC，如果 max 也小于 0，那么将 max
设置为 INT_MAX。函数接着调用 xa_lock_irqsave() 上锁，然后调用
ida_get_new_above() 函数从 IDA 对应的 Radix-tree 中获得一个可用
的 ID，如果获得的 ID 大于 max，那么调用 ida_remove() 函数将 ID 从
IDA 中移除，并设置 id 为 -ENOSPC；如果此时获得的 ID 有效，那么调用
xa_unlock_irqrestore() 函数解除锁。函数继续判断 id 的值，如果此时
id 的值是 EAGAIN，那么函数调用 ida_pre_get() 函数增加 IDA 的内存池，
以此增大 IDA 容纳更多的 ID。最后返回 ID。

> - [ida_get_new_above](https://biscuitos.github.io/blog/IDA_SourceAPI/#ida_get_new_above)
>
> - [ida_pre_get](https://biscuitos.github.io/blog/IDA_ida_pre_get/)
>
> - [ida_remove](https://biscuitos.github.io/blog/IDA_SourceAPI/#ida_remove)

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
	int id;

	/* Allocate an unused ID */
	id = ida_alloc_range(&BiscuitOS_ida, 0x10, INT_MAX,  GFP_KERNEL);
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
+       bool "ida_alloc_range"
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
            [*]ida_alloc_range()
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
IDA-ID: 0x10
aaci-pl041 10004000.aaci: ARM AC'97 Interface PL041 rev0 at 0x10004000, irq 24
aaci-pl041 10004000.aaci: FIFO 512 entries
oprofile: using arm/armv7-ca9
{% endhighlight %}

#### <span id="驱动分析">驱动分析</span>

分配 ID。

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

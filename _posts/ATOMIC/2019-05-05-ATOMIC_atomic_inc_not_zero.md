---
layout: post
title:  "atomic_inc_not_zero"
date:   2019-05-07 12:03:30 +0800
categories: [HW]
excerpt: ATOMIC atomic_inc_not_zero().
tags:
  - ATOMIC
---

![DTS](https://raw.githubusercontent.com/EmulateSpace/PictureSet/master/BiscuitOS/kernel/IND00000A.jpg)

> [Github: atomic_inc_not_zero](https://github.com/BiscuitOS/HardStack/tree/master/Algorithem/atomic/API/atomic_inc_not_zero)
>
> Email: BuddyZhang1 <buddy.zhang@aliyun.com>
>
> Architecture: ARMv7 Cortex A9-MP

# 目录

> - [源码分析](#源码分析)
>
> - [实践](#实践)
>
> - [附录](#附录)

-----------------------------------

# <span id="源码分析">源码分析</span>

{% highlight ruby %}
/**
 * atomic_inc_not_zero - increment unless the number is zero
 * @v: pointer of type atomic_t
 *
 * Atomically increments @v by 1, if @v is non-zero.
 * Returns true if the increment was done.
 */
#ifndef atomic_inc_not_zero
#define atomic_inc_not_zero(v)          atomic_add_unless((v), 1, 0)
#endif
{% endhighlight %}

atomic_inc_not_zero() 用于当 atomic_t 变量不为零的时候对其进行加一操作，否则
不执行任何动作。函数直接调用 atomic_add_unless() 函数，将条件设置为 0, 因此
只有 atomic_t 变量不为零才进行加一操作。

###### atomic_add_unless

> [atomic_add_unless 源码分析](https://biscuitos.github.io/blog/ATOMIC_atomic_add_unless/)

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
 * atomic
 *
 * (C) 2019.05.05 <buddy.zhang@aliyun.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

/* Memory access
 *
 *
 *      +----------+
 *      |          |
 *      | Register |                                         +--------+
 *      |          |                                         |        |
 *      +----------+                                         |        |
 *            A                                              |        |
 *            |                                              |        |
 * +-----+    |      +----------+        +----------+        |        |
 * |     |<---o      |          |        |          |        |        |
 * | CPU |<--------->| L1 Cache |<------>| L2 Cache |<------>| Memory |
 * |     |<---o      |          |        |          |        |        |
 * +-----+    |      +----------+        +----------+        |        |
 *            |                                              |        |
 *            o--------------------------------------------->|        |
 *                         volatile/atomic                   |        |
 *                                                           |        |
 *                                                           +--------+
 */

#include <linux/kernel.h>
#include <linux/init.h>

static atomic_t BiscuitOS_counter = ATOMIC_INIT(0);

/* atomic_* */
static __init int atomic_demo_init(void)
{
	/* Legacy data */
	printk("[0]Atomic: %d\n", atomic_read(&BiscuitOS_counter));

	/* increment unless the number is zero */
	atomic_inc_not_zero(&BiscuitOS_counter);

	/* New data */
	printk("[1]Atomic: %d\n", atomic_read(&BiscuitOS_counter));

	/* atomic_t is not zero. */
	atomic_add(1, &BiscuitOS_counter);
	/* atomic_t not zero */
	atomic_inc_not_zero(&BiscuitOS_counter);
	printk("[2]Atomic: %d\n", atomic_read(&BiscuitOS_counter));

	return 0;
}
device_initcall(atomic_demo_init);
{% endhighlight %}

#### <span id="驱动安装">驱动安装</span>

驱动的安装很简单，首先将驱动放到 drivers/BiscuitOS/ 目录下，命名为 atomic.c，
然后修改 Kconfig 文件，添加内容参考如下：

{% highlight bash %}
diff --git a/drivers/BiscuitOS/Kconfig b/drivers/BiscuitOS/Kconfig
index 4edc5a5..1a9abee 100644
--- a/drivers/BiscuitOS/Kconfig
+++ b/drivers/BiscuitOS/Kconfig
@@ -6,4 +6,14 @@ if BISCUITOS_DRV
config BISCUITOS_MISC
        bool "BiscuitOS misc driver"
+config BISCUITOS_ATOMIC
+       bool "atomic"
+
+if BISCUITOS_ATOMIC
+
+config DEBUG_BISCUITOS_ATOMIC
+       bool "atomic_inc_not_zero"
+
+endif # BISCUITOS_ATOMIC
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
+obj-$(CONFIG_BISCUITOS_ATOMIC)  += atomic.o
--
{% endhighlight %}

#### <span id="驱动配置">驱动配置</span>

驱动配置请参考下面文章中关于驱动配置一节。在配置中，勾选如下选项，如下：

{% highlight bash %}
Device Driver--->
    [*]BiscuitOS Driver--->
        [*]atomic
            [*]atomic_inc_not_zero()
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

{% highlight ruby %}
usbcore: registered new interface driver usbhid
usbhid: USB HID core driver
[0]Atomic: 0
[1]Atomic: 0
[2]Atomic: 2
aaci-pl041 10004000.aaci: ARM AC'97 Interface PL041 rev0 at 0x10004000, irq 24
aaci-pl041 10004000.aaci: FIFO 512 entries
oprofile: using arm/armv7-ca9
{% endhighlight %}

#### <span id="驱动分析">驱动分析</span>

在对 0 不执行动作的情况下，使用这个函数可以。

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

![MMU](https://raw.githubusercontent.com/EmulateSpace/PictureSet/master/BiscuitOS/kernel/HAB000036.jpg)

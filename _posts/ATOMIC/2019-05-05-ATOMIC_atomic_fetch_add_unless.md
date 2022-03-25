---
layout: post
title:  "atomic_fetch_add_unless"
date:   2019-05-07 09:46:30 +0800
categories: [HW]
excerpt: ATOMIC atomic_fetch_add_unless().
tags:
  - ATOMIC
---

![DTS](/assets/PDB/BiscuitOS/kernel/IND00000A.jpg)

> [Github: atomic_fetch_add_unless](https://github.com/BiscuitOS/HardStack/tree/master/Algorithem/atomic/API/atomic_fetch_add_unless)
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
static inline int atomic_fetch_add_unless(atomic_t *v, int a, int u)
{
        int oldval, newval;
        unsigned long tmp;

        smp_mb();
        prefetchw(&v->counter);

        __asm__ __volatile__ ("@ atomic_add_unless\n"
"1:     ldrex   %0, [%4]\n"
"       teq     %0, %5\n"
"       beq     2f\n"
"       add     %1, %0, %6\n"
"       strex   %2, %1, [%4]\n"
"       teq     %2, #0\n"
"       bne     1b\n"
"2:"
        : "=&r" (oldval), "=&r" (newval), "=&r" (tmp), "+Qo" (v->counter)
        : "r" (&v->counter), "r" (u), "r" (a)
        : "cc");

        if (oldval != u)
                smp_mb();

        return oldval;
}
{% endhighlight %}

atomic_fetch_add_unless() 函数用于当 atomic_t 变量值与某个值不相等是，执行加法操作。
参数 v 指向一个 atomic_t 变量；参数 a 指明需要增加的数据；参数 u 代表与原始值对比的值。
函数首先调用 smp_mb() 函数添加一个内存屏障，接着调用 prefetchw() 函数预读取 v->counter
的值到 cache，接着调用内嵌汇编。在汇编中，首先调用 ldrex 指令添加独占标志，并将
v->counter 的值从内存读取到 oldval 变量里，然后通过对比 oldval 的值是否和 参数 u 相等，
如果相等，则跳转到 2 处；如果不相等，那么就将 oldval 的值与参数 a 的值相加，并把
相加的结果存储到 newval 里，接着调用 strex 指令，如果此时独占标志还存在，那么
strex 指令就将 newval 的值写入 v->counter 对应的内存，并肩 tmp 的值设置为 0；反之
如果此时独占标志已经被清零，那么 strex 指令仅仅将 tmp 设置为 1，然后继续执行
teq 指令，如果此时 teq 检查到 tmp 不为零，那么跳转到 1 处重复执行之前的动作，知道
将 newval 写入到 v->counter 对应的内存。最后如果 oldval 不等于 u，那么代表
strex 指令已经执行过了，所以此时需要在执行一条 smp_mb() 确保数据都写入到内存。
最后返回 oldval 的值。

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

/*
 * atomic_add_* (ARMv7 Cotex-A9MP)
 *
 * static inline int atomic_fetch_add_unless(atomic_t *v, int a, int u)
 * {
 *         int oldval, newval;
 *         unsigned long tmp;
 *
 *         smp_mb();
 *         prefetchw(&v->counter);
 *
 *         __asm__ __volatile__ ("@ atomic_add_unless\n"
 * "1:     ldrex   %0, [%4]\n"
 * "       teq     %0, %5\n"
 * "       beq     2f\n"
 * "       add     %1, %0, %6\n"
 * "       strex   %2, %1, [%4]\n"
 * "       teq     %2, #0\n"
 * "       bne     1b\n"
 * "2:"
 *         : "=&r" (oldval), "=&r" (newval), "=&r" (tmp), "+Qo" (v->counter)
 *         : "r" (&v->counter), "r" (u), "r" (a)
 *         : "cc");
 *
 *         if (oldval != u)
 *                 smp_mb();
 *
 *         return oldval;
 * }
 */

#include <linux/kernel.h>
#include <linux/init.h>

static atomic_t BiscuitOS_counter = ATOMIC_INIT(8);

/* atomic_* */
static __init int atomic_demo_init(void)
{
	int val;

	/* Atomic add: Old == original */
	val = atomic_fetch_add_unless(&BiscuitOS_counter, 1, 8);

	printk("[0]Atomic: oiginal-> %d new-> %d\n", val,
			atomic_read(&BiscuitOS_counter));

	/* Atomic add: Old != original */
	val = atomic_fetch_add_unless(&BiscuitOS_counter, 1, 5);

	printk("[1]Atomic: original-> %d new-> %d\n", val,
			atomic_read(&BiscuitOS_counter));

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
+       bool "atomic_fetch_add_unless"
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
            [*]atomic_fetch_add_unless()
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
[0]Atomic: oiginal-> 8 new-> 8
[1]Atomic: original-> 8 new-> 9
aaci-pl041 10004000.aaci: ARM AC'97 Interface PL041 rev0 at 0x10004000, irq 24
aaci-pl041 10004000.aaci: FIFO 512 entries
oprofile: using arm/armv7-ca9
{% endhighlight %}

#### <span id="驱动分析">驱动分析</span>

在某些有条件的 atomic 加法中，这个函数是比较适合的。

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

![MMU](/assets/PDB/BiscuitOS/kernel/HAB000036.jpg)

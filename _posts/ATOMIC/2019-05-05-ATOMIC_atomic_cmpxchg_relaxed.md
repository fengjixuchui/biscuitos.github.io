---
layout: post
title:  "atomic_cmpxchg_relaxed"
date:   2019-05-07 09:14:30 +0800
categories: [HW]
excerpt: ATOMIC atomic_cmpxchg_relaxed().
tags:
  - ATOMIC
---

![DTS](/assets/PDB/BiscuitOS/kernel/IND00000A.jpg)

> [Github: atomic_cmpxchg_relaxed](https://github.com/BiscuitOS/HardStack/tree/master/Algorithem/atomic/API/atomic_cmpxchg_relaxed)
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
static inline int atomic_cmpxchg_relaxed(atomic_t *ptr, int old, int new)
{
        int oldval;
        unsigned long res;

        prefetchw(&ptr->counter);

        do {
                __asm__ __volatile__("@ atomic_cmpxchg\n"
                "ldrex  %1, [%3]\n"
                "mov    %0, #0\n"
                "teq    %1, %4\n"
                "strexeq %0, %5, [%3]\n"
                    : "=&r" (res), "=&r" (oldval), "+Qo" (ptr->counter)
                    : "r" (&ptr->counter), "Ir" (old), "r" (new)
                    : "cc");
        } while (res);

        return oldval;
}
{% endhighlight %}

atomic_cmpxchg_relaxed() 函数用于对 atmoic 变量的值进行对比，如果与内存中的值
相等，那么就替换成新值。参数 ptr 指向一个 atomic_t 变量；参数 old 代表与内存中对比
的值；new 代表替换的值。函数首先调用 prefetchw() 函数将 ptr->counter 值从内存预取
到 cache 里面，然后调用一个内嵌汇编。在内嵌汇编中，首先低啊用 ldrex 指令设置
独占标志，并将 ptr->counter 的值从内存读取到 oldval 变量里，接着将 res 变量清零。
接着调用 teq 指令对比 ptr->counter 的值是否和 old 参数相同，如果不相同，则直接
返回；如果相同，那么调用 strexeq 指令，如果此时独占标志还存在，那么 strexeq 指令就
将 new 参数的值写入到 ptr->counter 对应的内存中，并将 res 设置为 0；如果此时
独占标志被清零，那么 strexeq 不会将 new 值写入 ptr->counter 对应的内存，而是
仅仅将 res 设置为 1。由于 res 的值为 1，那么 while 循环继续循环重复之前的操作。

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
 * atomic_cmpxchg_* (ARMv7 Cotex-A9MP)
 *
 * static inline int atomic_cmpxchg_relaxed(atomic_t *ptr, int old, int new)
 * {
 *         int oldval;
 *         unsigned long res;
 *
 *         prefetchw(&ptr->counter);
 *
 *         do {
 *                 __asm__ __volatile__("@ atomic_cmpxchg\n"
 *                 "ldrex  %1, [%3]\n"
 *                 "mov    %0, #0\n"
 *                 "teq    %1, %4\n"
 *                 "strexeq %0, %5, [%3]\n"
 *                     : "=&r" (res), "=&r" (oldval), "+Qo" (ptr->counter)
 *                     : "r" (&ptr->counter), "Ir" (old), "r" (new)
 *                     : "cc");
 *         } while (res);
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

	/* Atomic cmpxchg: Old == original */
	val = atomic_cmpxchg_relaxed(&BiscuitOS_counter, 8, 9);

	printk("[0]Atomic: oiginal-> %d new-> %d\n", val,
			atomic_read(&BiscuitOS_counter));

	/* Atomic cmpxchg: Old != original */
	val = atomic_cmpxchg_relaxed(&BiscuitOS_counter, 1, 5);

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
+       bool "atomic_cmpxchg_relaxed"
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
            [*]atomic_cmpxchg_relaxed()
{% endhighlight %}

具体过程请参考：

> [Linux 5.0 开发环境搭建 -- 驱动配置](/blog/Linux-5.0-arm32-Usermanual/#%E9%A9%B1%E5%8A%A8%E9%85%8D%E7%BD%AE)

#### <span id="驱动编译">驱动编译</span>

驱动编译也请参考下面文章关于驱动编译一节：

> [Linux 5.0 开发环境搭建 -- 驱动编译](/blog/Linux-5.0-arm32-Usermanual/#%E7%BC%96%E8%AF%91%E9%A9%B1%E5%8A%A8)

#### <span id="驱动运行">驱动运行</span>

驱动的运行，请参考下面文章中关于驱动运行一节：

> [Linux 5.0 开发环境搭建 -- 驱动运行](/blog/Linux-5.0-arm32-Usermanual/#%E9%A9%B1%E5%8A%A8%E8%BF%90%E8%A1%8C)

启动内核，并打印如下信息：

{% highlight ruby %}
usbcore: registered new interface driver usbhid
usbhid: USB HID core driver
[0]Atomic: original-> 8 new-> 9
[1]Atomic: original-> 9 new-> 9
aaci-pl041 10004000.aaci: ARM AC'97 Interface PL041 rev0 at 0x10004000, irq 24
aaci-pl041 10004000.aaci: FIFO 512 entries
oprofile: using arm/armv7-ca9
{% endhighlight %}

#### <span id="驱动分析">驱动分析</span>

在需要交换 atomic_t 变量值的地方可以使用 atomic_cmpxchg_relaxed() 函数。

-----------------------------------------------

# <span id="附录">附录</span>

> [BiscuitOS Home](https://biscuitos.github.io/)
>
> [BiscuitOS Driver](/blog/BiscuitOS_Catalogue/)
>
> [BiscuitOS Kernel Build](/blog/Kernel_Build/)
>
> [Linux Kernel](https://www.kernel.org/)
>
> [Bootlin: Elixir Cross Referencer](https://elixir.bootlin.com/linux/latest/source)
>
> [搭建高效的 Linux 开发环境](/blog/Linux-debug-tools/)

## 赞赏一下吧 🙂

![MMU](/assets/PDB/BiscuitOS/kernel/HAB000036.jpg)

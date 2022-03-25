---
layout: post
title:  "atomic_sub_return_relaxed"
date:   2019-05-07 07:20:30 +0800
categories: [HW]
excerpt: ATOMIC atomic_sub_return_relaxed().
tags:
  - ATOMIC
---

![DTS](/assets/PDB/BiscuitOS/kernel/IND00000A.jpg)

> [Github: atomic_sub_return_relaxed](https://github.com/BiscuitOS/HardStack/tree/master/Algorithem/atomic/API/atomic_sub_return_relaxed)
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
#define ATOMIC_OPS(op, c_op, asm_op)                                    \
        ATOMIC_OP(op, c_op, asm_op)                                     \
        ATOMIC_OP_RETURN(op, c_op, asm_op)                              \
        ATOMIC_FETCH_OP(op, c_op, asm_op)

ATOMIC_OPS(sub, +=, sub)
{% endhighlight %}

atomic_sub_return_relaxed() 用于给 atomic_t 变量做减法并返回减少之后的值。在 ARMv7 中，
使用 ATOMIC_OPS 宏定义了 atomic_sub_return_relaxed() 函数。开发者可以通过编译之后的结果
查看 atomic_sub_return_relaxed() 函数的实现，
如下：

{% highlight ruby %}
static inline int atomic_sub_return_relaxed(int i, atomic_t *v)
{
        unsigned long tmp;
        int result;

        prefetchw(&v->counter);
        __asm__ volatile ("\n\t"
        "@ atomic_sub\n\t"
"1:      ldrex   %0, [%3]\n\t"        @ result, tmp115
"        sub     %0, %0, %4\n\t"      @ result,
"        strex   %1, %0, [%3]\n\t"    @ tmp, result, tmp115
"        teq     %1, #0\n\t"          @ tmp
"        bne     1b"
         : "=&r" (result), "=&r" (tmp), "+Qo" (v->counter)
         : "r" (&v->counter), "Ir" (i)
         : "cc");

         return result;
}
{% endhighlight %}

atomic_sub_return_relaxed() 函数的定义如上，参数 i 指明 atomic 变量需要减少的值；参数 v 指向
atomic_t 变量。函数首先使用 prefetchw() 函数将 v->counter 的值预读到 cache，
然后调用一个内嵌汇编，汇编首先调用 ldrex 指令首先对 v->counter 对应的内存地址
设置独占标志，同时从内存中读取 v->counter 的值到 result。接着调用 sub 指令，
将 result 中的值添加 i 对应的值。然后调用 strex 指令准备将 result 中的值写入
到 v->counter 对应的内存地址，如果此时独占标志还存在，表示写内存的操作不存在抢占
问题，可以直接写入，并将 tmp 的值设置为 0；如果此时独占标志已经被清除，那么
此时没有权限往内存写入值，那么 strex 会放弃写入值，并将 tmp 设置为 1。strex
指令执行完之后，调用 teq 指令检查 tmp 的值，如果是 0，那么表示写入成功，直接返回；
如果是 1，那么调用 bne 跳转到 1，重新执行之前的代码，直到 strex 将数据写入到
内存，最后返回 result 的值。上面的逻辑确保 SMP 模式下，多线程对共享的数据实现了
锁机制。最后返回减法之后的结果。

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
 * atomic_sub (ARMv7 Cotex-A9MP)
 *
 * static inline int atomic_sub_return_relaxed(int i, atomic_t *v)
 * {
 *         unsigned long tmp;
 *         int result;
 *
 *         prefetchw(&v->counter);
 *         __asm__ volatile ("\n\t"
 *         "@ atomic_sub\n\t"
 * "1:      ldrex   %0, [%3]\n\t"        @ result, tmp115
 * "        sub     %0, %0, %4\n\t"      @ result,
 * "        strex   %1, %0, [%3]\n\t"    @ tmp, result, tmp115
 * "        teq     %1, #0\n\t"          @ tmp
 * "        bne     1b"
 *          : "=&r" (result), "=&r" (tmp), "+Qo" (v->counter)
 *          : "r" (&v->counter), "Ir" (i)
 *          : "cc");
 *
 *         return result;
 * }
 */

#include <linux/kernel.h>
#include <linux/init.h>

static atomic_t BiscuitOS_counter = ATOMIC_INIT(8);

/* atomic_* */
static __init int atomic_demo_init(void)
{
	int val;

	/* Atomic sub */
	val = atomic_sub_return_relaxed(1, &BiscuitOS_counter);

	printk("Atomic: %d\n", val);

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
+       bool "atomic_sub_return_relaxed"
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
            [*]atomic_sub_return_relaxed()
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
Atomic: 7
aaci-pl041 10004000.aaci: ARM AC'97 Interface PL041 rev0 at 0x10004000, irq 24
aaci-pl041 10004000.aaci: FIFO 512 entries
oprofile: using arm/armv7-ca9
{% endhighlight %}

#### <span id="驱动分析">驱动分析</span>

在需要对 atomic_t 变量进行减法之后并获得结果，可以使用 atomic_sub_return_relaxed

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

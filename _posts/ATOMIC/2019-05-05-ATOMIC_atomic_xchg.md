---
layout: post
title:  "atomic_xchg"
date:   2019-05-07 10:16:30 +0800
categories: [HW]
excerpt: ATOMIC atomic_xchg().
tags:
  - ATOMIC
---

![DTS](/assets/PDB/BiscuitOS/kernel/IND00000A.jpg)

> [Github: atomic_xchg](https://github.com/BiscuitOS/HardStack/tree/master/Algorithem/atomic/API/atomic_xchg)
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
#define atomic_xchg(v, new) (xchg(&((v)->counter), new))
{% endhighlight %}

atomic_xchg() 函数用于交换 atomic_t 变量的值。参数 v 指向 atomic_t 变量；参数
new 代表需要替换的值。函数直接调用 xchg 进行交换，xchg 的核心实现如下：

{% highlight ruby %}
#define atomic_xchg(v, new) (xchg(&((v)->counter), new))
{% endhighlight %}

xchg 是核心实现如下：

{% highlight ruby %}
static inline unsigned long __xchg(unsigned long x, volatile void *ptr,
                                             int size)
{
     unsigned long ret;
     unsigned int tmp;

     asm volatile("@ __xchg4\n"
     "1:     ldrex   %0, [%3]\n"
     "       strex   %1, %2, [%3]\n"
     "       teq     %1, #0\n"
     "       bne     1b"
             : "=&r" (ret), "=&r" (tmp)
             : "r" (x), "r" (ptr)
             : "memory", "cc");

     return ret;
}
{% endhighlight %}

参数 x 代表需要交换的数值；参数 ptr 指向一个 atomic_t 变量；函数直接调用内嵌汇编。
汇编中首先调用 ldrex 指令标记独占，并且从内存中读取 ptr 对应的内容到 ret 变量里，
然后调用 strex 指令，如果此时独占标志还存在的话，那么就将 x 参数的值写入到 ptr
对应的内存地址。并将 tmp 的值设置为 0；如果此时独占标志已经被清零，那么 strex 指令
仅仅设置 tmp 的值为 1,。接着调用 teq 指令，如果 tmp 的值为 1，那么跳转到 1 处
重新执行之前的动作；如果 tmp 为 0，那么直接返回 ret 的值。因此，可以确保数据被
原子写入到内存。

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
 * atomic_xchg (ARMv7 Cotex-A9MP)
 *
 * static inline unsigned long __xchg(unsigned long x, volatile void *ptr,
 *						int size)
 * {
 * 	unsigned long ret;
 * 	unsigned int tmp;
 *
 * 	asm volatile("@ __xchg4\n"
 * 	"1:     ldrex   %0, [%3]\n"
 * 	"       strex   %1, %2, [%3]\n"
 * 	"       teq     %1, #0\n"
 * 	"       bne     1b"
 * 		: "=&r" (ret), "=&r" (tmp)
 * 		: "r" (x), "r" (ptr)
 * 		: "memory", "cc");
 *
 *	return ret;
 * }
 */

#include <linux/kernel.h>
#include <linux/init.h>

static atomic_t BiscuitOS_counter = ATOMIC_INIT(8);

/* atomic_* */
static __init int atomic_demo_init(void)
{
	/* Atomic value before exhange data  */
	printk("[0]Atomic: oiginal-> %d\n", atomic_read(&BiscuitOS_counter));

	/* Atomic xhg */
	atomic_xchg(&BiscuitOS_counter, 6);

	/* Atomic value after exhange data  */
	printk("[1]Atomic: new->     %d\n", atomic_read(&BiscuitOS_counter));

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
+       bool "atomic_xchg"
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
            [*]atomic_xchg()
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
[0]Atomic: oiginal-> 8
[1]Atomic: new->     6
aaci-pl041 10004000.aaci: ARM AC'97 Interface PL041 rev0 at 0x10004000, irq 24
aaci-pl041 10004000.aaci: FIFO 512 entries
oprofile: using arm/armv7-ca9
{% endhighlight %}

#### <span id="驱动分析">驱动分析</span>

交换 atomic_t 变量的值，可以使用 atomic_xchg() 函数。

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

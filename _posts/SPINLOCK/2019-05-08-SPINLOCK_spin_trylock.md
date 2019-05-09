---
layout: post
title:  "spin_trylock"
date:   2019-05-08 16:45:30 +0800
categories: [HW]
excerpt: SPINLOCK spin_trylock().
tags:
  - SPINLOCK
---

![DTS](https://raw.githubusercontent.com/EmulateSpace/PictureSet/master/BiscuitOS/kernel/IND00000S.jpg)

> [Github: spin_trylock](https://github.com/BiscuitOS/HardStack/tree/master/Algorithem/spinlock/API/spin_trylock)
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

{% highlight ruby %}
static __always_inline int spin_trylock(spinlock_t *lock)
{
        return raw_spin_trylock(&lock->rlock);
}
{% endhighlight %}

spin_trylock() 函数用于尝试给 spinlock 加锁，如果锁被占用，则直接返回 0 不死等；
如果获得 spinlock 锁，那么上锁并返回 1。spin_trylock() 的实现与体系有关，如下：

##### ARMv7 arm32

{% highlight ruby %}
static inline int arch_spin_trylock(arch_spinlock_t *lock)
{
        unsigned long contended, res;
        u32 slock;

        prefetchw(&lock->slock);
        do {
                __asm__ __volatile__(
                "       ldrex   %0, [%3]\n"
                "       mov     %2, #0\n"
                "       subs    %1, %0, %0, ror #16\n"
                "       addeq   %0, %0, %4\n"
                "       strexeq %2, %0, [%3]"
                : "=&r" (slock), "=&r" (contended), "=&r" (res)
                : "r" (&lock->slock), "I" (1 << TICKET_SHIFT)
                : "cc");
        } while (res);

        if (!contended) {
                smp_mb();
                return 1;
        } else {
                return 0;
        }
}
{% endhighlight %}

函数首先执行 prefetchw() 函数将 lock->slock 从内存预读到 cache 中。然后调用一段
内嵌汇编。汇编首先调用 ldrex 指令设置独占标志，同时从内存中读取 lock->slock 的值
到 slock 中，接着将 0 存储到 res 里，调用 sub 指令，比较 slock 的高 16 位于低 16
位是否相同，并将相减的结果存储在 contended 里。如果相减的结果不为 0，表示无法获得
spinlock 锁，则直接结束汇编；如果相减的结果为 0，表示 spinlock 未上锁，则调用 addeq
指令，给 slock 的高 16 位加一，然后调用 strexeq 指令，如果此时独占标志被清除，那么
仅仅将 res 置 1，然后再次循环重复之前的操作；如果此时独占标志置位，那么将 slock 的
值写入到 lock->slock 里，并将 res 设置为 0。执行完汇编之后，如果加锁成功，那么 contended
的值为零，那么执行 smp_mb() 实现一次内存屏障，并返回 1；如果加锁失败，contended 的
值不为零，那么直接返回 0.


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
 * spinlock
 *
 * (C) 2019.05.08 <buddy.zhang@aliyun.com>
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

/* spinlock */
#include <linux/spinlock.h>

/*
 * ARMv7 arch_spin_lock()
 *
 * static inline int arch_spin_trylock(arch_spinlock_t *lock)
 * {
 *         unsigned long contended, res;
 *         u32 slock;
 *
 *         prefetchw(&lock->slock);
 *         do {
 *                 __asm__ __volatile__(
 *                 "       ldrex   %0, [%3]\n"
 *                 "       mov     %2, #0\n"
 *                 "       subs    %1, %0, %0, ror #16\n"
 *                 "       addeq   %0, %0, %4\n"
 *                 "       strexeq %2, %0, [%3]"
 *                 : "=&r" (slock), "=&r" (contended), "=&r" (res)
 *                 : "r" (&lock->slock), "I" (1 << TICKET_SHIFT)
 *                 : "cc");
 *         } while (res);
 *
 *         if (!contended) {
 *                 smp_mb();
 *                 return 1;
 *         } else {
 *                 return 0;
 *         }
 * }
 */

static spinlock_t BiscuitOS_lock;

static __init int spinlock_demo_init(void)
{
	/* Initialize spinlock */
	spin_lock_init(&BiscuitOS_lock);

	/* try acquire lock */
	if (spin_trylock(&BiscuitOS_lock)) {

		__asm__ volatile ("nop");

		/* release lock */
		spin_unlock(&BiscuitOS_lock);
	} else {
		printk("Unable obtain spinlock.\n");
	}

	printk("Spinlock done.....\n");

	return 0;
}
device_initcall(spinlock_demo_init);
{% endhighlight %}

#### <span id="驱动安装">驱动安装</span>

驱动的安装很简单，首先将驱动放到 drivers/BiscuitOS/ 目录下，命名为 spinlock.c，
然后修改 Kconfig 文件，添加内容参考如下：

{% highlight bash %}
diff --git a/drivers/BiscuitOS/Kconfig b/drivers/BiscuitOS/Kconfig
index 4edc5a5..1a9abee 100644
--- a/drivers/BiscuitOS/Kconfig
+++ b/drivers/BiscuitOS/Kconfig
@@ -6,4 +6,14 @@ if BISCUITOS_DRV
config BISCUITOS_MISC
        bool "BiscuitOS misc driver"
+config BISCUITOS_SPINLOCK
+       bool "SPINLOCK"
+
+if BISCUITOS_SPINLOCK
+
+config DEBUG_BISCUITOS_SPINLOCK
+       bool "spin_trylock"
+
+endif # BISCUITOS_SPINLOCK
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
+obj-$(CONFIG_BISCUITOS_SPINLOCK)  += SPINLOCK.o
--
{% endhighlight %}

#### <span id="驱动配置">驱动配置</span>

驱动配置请参考下面文章中关于驱动配置一节。在配置中，勾选如下选项，如下：

{% highlight bash %}
Device Driver--->
    [*]BiscuitOS Driver--->
        [*]SPINLOCK
            [*]spin_trylock()
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
Spinlock done.....
input: AT Raw Set 2 keyboard as /devices/platform/smb@4000000/smb@4000000:motherboard/smb@4000000:motherboard:iofpga@7,00000000/10006000.kmi/serio0/input/input0
aaci-pl041 10004000.aaci: ARM AC'97 Interface PL041 rev0 at 0x10004000, irq 24
aaci-pl041 10004000.aaci: FIFO 512 entries
oprofile: using arm/armv7-ca9
{% endhighlight %}

#### <span id="驱动分析">驱动分析</span>

如果 spinlock 可以加上则上锁；如果加不上锁，则不死等，直接返回。那么使用
spin_trylock().

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

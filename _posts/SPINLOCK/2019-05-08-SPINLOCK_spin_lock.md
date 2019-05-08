---
layout: post
title:  "spin_lock"
date:   2019-05-08 10:27:30 +0800
categories: [HW]
excerpt: SPINLOCK spin_lock().
tags:
  - SPINLOCK
---

![DTS](https://raw.githubusercontent.com/EmulateSpace/PictureSet/master/BiscuitOS/kernel/IND00000S.jpg)

> [Github: spin_lock](https://github.com/BiscuitOS/HardStack/tree/master/Algorithem/spinlock/API/spin_lock)
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
static __always_inline void spin_lock(spinlock_t *lock)
{
        raw_spin_lock(&lock->rlock);
}
{% endhighlight %}

spin_lock() 函数的作用就是给一段临界代码上锁。其实现与体系有关，每个体系的实现
如下：

##### ARMv7: arm32

{% highlight ruby %}
static inline void arch_spin_lock(arch_spinlock_t *lock)
{
        unsigned long tmp;
        u32 newval;
        arch_spinlock_t lockval;

        prefetchw(&lock->slock);
        __asm__ __volatile__(
"1:     ldrex   %0, [%3]\n"
"       add     %1, %0, %4\n"
"       strex   %2, %1, [%3]\n"
"       teq     %2, #0\n"
"       bne     1b"
        : "=&r" (lockval), "=&r" (newval), "=&r" (tmp)
        : "r" (&lock->slock), "I" (1 << TICKET_SHIFT)
        : "cc");

        while (lockval.tickets.next != lockval.tickets.owner) {
                wfe();
                lockval.tickets.owner = READ_ONCE(lock->tickets.owner);
        }

        smp_mb();
}
{% endhighlight %}

spin_lock() 函数最终在 ARMv7 上的实现通过 arch_spin_lock() 函数，spinlock_t 变量
通过转换之后，以 arch_spinlock_t 的形式传入到 arch_spin_lock() 中，其定义指的研究，
如下：

{% highlight ruby %}
typedef struct {
        union {
                u32 slock;
                struct __raw_tickets {
                        u16 owner;
                        u16 next;
                } tickets;
        };
} arch_spinlock_t;
{% endhighlight %}

从上面的结构体可以看出，arch_spinlock_t 结构中包含了一个联合体，联合体的第一个成员
是 u32 变量 slock，联合体的第二个成员是 struct __raw_tickets 机构，结构中包含了
两个 u16 变量 owner 和 next。从 arch_spinlock_t 结构的定义可以看出 tickets.owner
代表 slock 的低 16 位，tickets.next 代表 slock 的高 16 位。再次回到
arch_spin_lock() 函数。首先调用 prefetchw() 函数从内存中将 lock->slock 预读到内存，
然后是一段内嵌汇编，如下：

{% highlight ruby %}
        __asm__ __volatile__(
"1:     ldrex   %0, [%3]\n"
"       add     %1, %0, %4\n"
"       strex   %2, %1, [%3]\n"
"       teq     %2, #0\n"
"       bne     1b"
        : "=&r" (lockval), "=&r" (newval), "=&r" (tmp)
        : "r" (&lock->slock), "I" (1 << TICKET_SHIFT)
        : "cc");
{% endhighlight %}

汇编中首先调用 ldrex 指令设置独占标志，然后将 lock->slock 的值读取到 lockval 变量里，
然后将 lockval 的值增加 (1 << TICET_SHIFT)，并将结果存储到 newval 内，此处
TICKET_SHIFT 的值是 16，可以理解为给 arch_spinlock_t 中的 slock 高 16 位加一操作，
也就是 tickets.next 加一操作。接着调用 strex 指令，如果此时独占标志被清除，那么
仅仅将 tmp 设置为 1；如果此时独占标志还存在，那么将 newval 的值写入到 lock->slock
中，并将 tmp 设置为 0. 接着调用 teq 和 bne 指令判断 tmp 值，如果为 1，代表写入失败，
跳转到 1，并重复之前的操作；如果 tmp 的值为 0，那么该段内嵌汇编就结束。通过上面的
汇编，确保每个 PE 对锁的值都正确写入。接下来执行的代码是：

{% highlight ruby %}
        while (lockval.tickets.next != lockval.tickets.owner) {
                wfe();
                lockval.tickets.owner = READ_ONCE(lock->tickets.owner);
        }

        smp_mb();
{% endhighlight %}

上面的代码就是 spinlock 的核心思想，当有线程执对该 spin lock 执行过加锁操作，
那么对 lock.tickets.next 执行加 1 操作 (解锁的时候就会对 lock.tickets.owner
执行加一操作)，此时 lockval 代表上一次 spinlock 的情况，因此只要判断
lockval.tickets.next 与 lockval.tickets.owner 的值就能判断上一次加锁的情况。
如果 lockval.tickets.next 与 lockval.tickets.owner 相等，代表没有使用 spinlock，
可以使用； 如果 lockval.tickets.next 与 lockval.tickets.owner 不相等，代表
spinlock 正在被使用，因此进入 while 循环内，调用 wfe() 函数。

对于 wfe() 函数，在没有实现 WFE 指令，那么使用 nop 指令替代，这就是所有的 “忙等”，
如果体系实现了 WFE 指令，那么该 CPU 就进入 low-power 模式，等待其他处理器调用
SEV 指令来唤醒该处理器。

通过上面的分析就可以知道 spinlock 的核心实现就是，但发现有其他进程或线程在使用
该 spinlock，那么就进入 “忙等” 或者 "low-power" 模式，直到解锁，调用 READ_ONCE()
函数从内存中再次读取 lock->tickets.owner 的值，以此判断 spinlock 锁是否真的可用。
如果可用就返回；如果不可用继续在 while 循环中执行。

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
 * static inline void arch_spin_lock(arch_spinlock_t *lock)
 * {
 *         unsigned long tmp;
 *         u32 newval;
 *         arch_spinlock_t lockval;
 *
 *         prefetchw(&lock->slock);
 *         __asm__ __volatile__(
 * "1:     ldrex   %0, [%3]\n"
 * "       add     %1, %0, %4\n"
 * "       strex   %2, %1, [%3]\n"
 * "       teq     %2, #0\n"
 * "       bne     1b"
 *         : "=&r" (lockval), "=&r" (newval), "=&r" (tmp)
 *         : "r" (&lock->slock), "I" (1 << TICKET_SHIFT)
 *         : "cc");
 *
 *         while (lockval.tickets.next != lockval.tickets.owner) {
 *                 wfe();
 *                 lockval.tickets.owner = READ_ONCE(lock->tickets.owner);
 *         }
 *
 *         smp_mb();
 * }
 */

static spinlock_t BiscuitOS_lock;

static __init int spinlock_demo_init(void)
{
	/* Initialize spinlock */
	spin_lock_init(&BiscuitOS_lock);

	/* acquire lock */
	spin_lock(&BiscuitOS_lock);

	__asm__ volatile ("nop");

	/* release lock */
	spin_unlock(&BiscuitOS_lock);

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
+       bool "spin_lock"
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
            [*]spin_lock()
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

使用 spin_lock() 函数保护临界代码段。

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

---
layout: post
title:  "spin_lock_init"
date:   2019-05-08 11:48:30 +0800
categories: [HW]
excerpt: SPINLOCK spin_lock_init().
tags:
  - SPINLOCK
---

![DTS](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND00000S.jpg)

> [Github: spin_lock_init](https://github.com/BiscuitOS/HardStack/tree/master/Algorithem/spinlock/API/spin_lock_init)
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
#define spin_lock_init(_lock)                           \
do {                                                    \
        spinlock_check(_lock);                          \
        raw_spin_lock_init(&(_lock)->rlock);            \
} while (0)
{% endhighlight %}

spin_lock_init() 函数用于初始化一个 spinlock 锁，其实现与体系结构有关，例如：

##### ARMv7 arm32

{% highlight ruby %}
# define raw_spin_lock_init(lock)                               \
        do { *(lock) = __RAW_SPIN_LOCK_UNLOCKED(lock); } while (0)

#define __RAW_SPIN_LOCK_UNLOCKED(lockname)      \
        (raw_spinlock_t) __RAW_SPIN_LOCK_INITIALIZER(lockname)

#define __RAW_SPIN_LOCK_INITIALIZER(lockname)   \
        {                                       \
        .raw_lock = __ARCH_SPIN_LOCK_UNLOCKED,  \
        SPIN_DEBUG_INIT(lockname)               \
        SPIN_DEP_MAP_INIT(lockname) }

#define __ARCH_SPIN_LOCK_UNLOCKED       { { 0 } }
{% endhighlight %}

通过上面的定义可以知道，spin_lock_init() 函数将 spinlock 锁初始化为 0.

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
+       bool "spin_lock_init"
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
            [*]spin_lock_init()
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

当使用一个 spinlock 之前应该进行初始化。

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

---
layout:             post
title:              "ARM 汇编"
date:               2019-03-20 15:19:30 +0800
categories:         [MMU]
excerpt:            ARM Assembly inline.
tags:
  - MMU
---

![LINUXP](https://gitee.com/BiscuitOS/GIFBaseX/raw/master/RPI/GIF000204.gif)

> [GitHub ARM Assembly Set](https://github.com/BiscuitOS/HardStack/tree/master/Language/Assembly/ARM-GNU-Assembly/Instruction)
>
> Email: BuddyZhang1 <buddy.zhang@aliyun.com>

# 目录

> - [ARM 汇编](#ARM 汇编index)
>
> - [ARM 汇编实践](#ARM 汇编实践)
>
> - [ARM 汇编 List](#ARM_INS_LIST)
>
> - [附录](#附录)

--------------------------------------------------------------
<span id="ARM 汇编index"></span>

![MMU](https://gitee.com/BiscuitOS/GIFBaseX/raw/master/RPI/IND00000A.jpg)

# ARM 汇编

###### ARM汇编指令集

> 指令：是机器码的助记符，经过汇编器编译后，由CPU执行。
>
> 伪指令：用来指导指令执行，是汇编器的产物，最终不会生成机器码。

###### 有两种不同风格的ARM指令

> ARM 官方的 ARM 汇编风格：指令一般用大写。
>
> GNU 风格的 ARM 汇编：指令一般用小写。

###### ARM 汇编的特点

所有运算处理都是发生通用寄存器(一般是 R0~R14 )的之中.所有存储器空间(如C语言变量
的本质就是一个存储器空间上的几个 BYTE).的值的处理,都是要传送到通用寄存器来完成.
因此代码中大量看到 LDR,STR 指令来传送值.

ARM 汇编语句中. 当前语句很多时候要隐含的使用上一句的执行结果.而且上一句的执行结
果,是放在CPSR寄存器里,(比如说进位,为 0,为负…)

--------------------------------------------------------------
<span id="ARM 汇编实践"></span>

![MMU](https://gitee.com/BiscuitOS/GIFBaseX/raw/master/RPI/IND00000H.jpg)

# ARM 汇编实践

ARM 汇编实践方法很多，这里提供在 Linux arm32 系统里进行实践。由于本文所介绍的
实践都基于 Linux 5.0 arm32，读者可以根据下文构建 Linux 5.0 arm32 的开发环境：

> [Linux Arm32 5.0 开发环境搭建教程](https://biscuitos.github.io/blog/Linux-5.0-arm32-Usermanual/)

在进行实践之前，将所用到的资源如下：

> [Arm 汇编命令实践代码 Github](https://github.com/BiscuitOS/HardStack/tree/master/Language/Assembly/ARM-GNU-Assembly/Instruction)
>
> [ARM 开发手册以及 ARMv7 架构手册](https://github.com/BiscuitOS/Documentation/tree/master/Datasheet/ARM)

##### 获得实践源码

ARM 汇编提供了很多指令，本文以 MOV 指令为例进行讲解，其他指令实践采用同样的
办法。首先从 Github 上获得 STMEA 指令的实践代码，如下：

{% highlight c %}
/*
 * Arm inline-assembly
 *
 * (C) 2019.03.15 BuddyZhang1 <buddy.zhang@aliyun.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#include <linux/init.h>
#include <linux/kernel.h>

#include <linux/mm.h>

/*
 * STMEA (Store Multiple) stores a non-empty subset (or possibly all)
 * of the general-purpose registers to sequential memory locations.
 *
 * Syntax
 *   STM{<cond>}<addressing_mode> <Rn>{!}, <registers>
 */

static unsigned long R0[10];

static int debug_stmea(void)
{
	unsigned long R1 = 0x11;
	unsigned long R2 = 0x22;
	unsigned long R3 = 0x33;
	int i;

	/*
	 * STMIA: Store Register into memory, and empty ascending stack
	 *
	 *
	 *             +-------------+
	 *             |             |
	 *             +-------------+
	 *             |             |
	 *             +-------------+
	 *             |             |
	 *             +-------------+
	 *             |             |
	 *    R0[5]--> +-------------+
	 *             |             |<----------- R1
	 *             +-------------+
	 *             |             |<----------- R2
	 *             +-------------+
	 *             |             |<----------- R3
	 *             +-------------+
	 *             |             |
	 *             +-------------+
	 *             |             |
	 *    R0[0]--> +-------------+
	 *
	 * Push register into empty ascending stack.
	 */

	/* Emulate Stack */
	for (i = 0; i < 10; i++)
		printk("R0[%d] %#lx\n", i, R0[i]);

	return 0;
}
device_initcall(debug_stmea);
{% endhighlight %}

##### 添加源码到内核

根据教程搭建好 Linux 5.0 arm32 开发环境之后，将获得的源码驱动加入到
BiscuitOS/output/linux-5.0-arm32/linux/linux/dirvers/BiscuitOS/ 目录下，
修改 Kconfig 文件，如下：

{% highlight bash %}
diff --git a/drivers/BiscuitOS/Kconfig b/drivers/BiscuitOS/Kconfig
index 4edc5a5..1a9abee 100644
--- a/drivers/BiscuitOS/Kconfig
+++ b/drivers/BiscuitOS/Kconfig
@@ -6,4 +6,14 @@ if BISCUITOS_DRV
config BISCUITOS_MISC
        bool "BiscuitOS misc driver"
+config BISCUITOS_ASM
+       bool "ARM assembly"

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
+obj-$(CONFIG_BISCUITOS_ASM)     += asm.o
--
{% endhighlight %}

##### 配置驱动

驱动配置请参考下面文章中关于驱动配置一节。在配置中，勾选如下选项，如下：

{% highlight bash %}
Device Driver--->
    [*]BiscuitOS Driver--->
        [*]ARM assembly
{% endhighlight %}

[基于 Linux 5.x 的 arm32 开发环境搭建教程](https://biscuitos.github.io/blog/Kernel_Build/#Linux_5X)

##### 驱动编译

驱动编译也请参考下面文章关于驱动编译一节：

[基于 Linux 5.x 的 arm32 开发环境搭建教程](https://biscuitos.github.io/blog/Kernel_Build/#Linux_5X)

##### 驱动运行

驱动的运行，请参考下面文章中关于驱动运行一节：

[基于 Linux 5.x 的 arm32 开发环境搭建教程](https://biscuitos.github.io/blog/Kernel_Build/#Linux_5X)

--------------------------------------------------------------
<span id="ARM_INS_LIST"></span>

![MMU](https://gitee.com/BiscuitOS/GIFBaseX/raw/master/RPI/IND00000K.jpg)

# ARM 汇编 List

> [adc](https://github.com/BiscuitOS/HardStack/tree/master/Language/Assembly/ARM-GNU-Assembly/Instruction/adc)
>
> [add](https://github.com/BiscuitOS/HardStack/tree/master/Language/Assembly/ARM-GNU-Assembly/Instruction/add)
>
> [and](https://github.com/BiscuitOS/HardStack/tree/master/Language/Assembly/ARM-GNU-Assembly/Instruction/and)
>
> [asl](https://github.com/BiscuitOS/HardStack/tree/master/Language/Assembly/ARM-GNU-Assembly/Instruction/asl)
>
> [asr](https://github.com/BiscuitOS/HardStack/tree/master/Language/Assembly/ARM-GNU-Assembly/Instruction/asr)
>
> [b](https://github.com/BiscuitOS/HardStack/tree/master/Language/Assembly/ARM-GNU-Assembly/Instruction/b)
>
> [bcc](https://github.com/BiscuitOS/HardStack/tree/master/Language/Assembly/ARM-GNU-Assembly/Instruction/bcc)
>
> [bcs](https://github.com/BiscuitOS/HardStack/tree/master/Language/Assembly/ARM-GNU-Assembly/Instruction/bcs)
>
> [beq](https://github.com/BiscuitOS/HardStack/tree/master/Language/Assembly/ARM-GNU-Assembly/Instruction/beq)
>
> [bgt](https://github.com/BiscuitOS/HardStack/tree/master/Language/Assembly/ARM-GNU-Assembly/Instruction/bgt)
>
> [bhi](https://github.com/BiscuitOS/HardStack/tree/master/Language/Assembly/ARM-GNU-Assembly/Instruction/bhi)
>
> [bhs](https://github.com/BiscuitOS/HardStack/tree/master/Language/Assembly/ARM-GNU-Assembly/Instruction/bhs)
>
> [bic](https://github.com/BiscuitOS/HardStack/tree/master/Language/Assembly/ARM-GNU-Assembly/Instruction/bic)
>
> [bl](https://github.com/BiscuitOS/HardStack/tree/master/Language/Assembly/ARM-GNU-Assembly/Instruction/bl)
>
> [ble](https://github.com/BiscuitOS/HardStack/tree/master/Language/Assembly/ARM-GNU-Assembly/Instruction/ble)
>
> [blo](https://github.com/BiscuitOS/HardStack/tree/master/Language/Assembly/ARM-GNU-Assembly/Instruction/blo)
>
> [bls](https://github.com/BiscuitOS/HardStack/tree/master/Language/Assembly/ARM-GNU-Assembly/Instruction/bls)
>
> [blt](https://github.com/BiscuitOS/HardStack/tree/master/Language/Assembly/ARM-GNU-Assembly/Instruction/blt)
>
> [bmi](https://github.com/BiscuitOS/HardStack/tree/master/Language/Assembly/ARM-GNU-Assembly/Instruction/bmi)
>
> [bne](https://github.com/BiscuitOS/HardStack/tree/master/Language/Assembly/ARM-GNU-Assembly/Instruction/bne)
>
> [bpl](https://github.com/BiscuitOS/HardStack/tree/master/Language/Assembly/ARM-GNU-Assembly/Instruction/bpl)
>
> [bvc](https://github.com/BiscuitOS/HardStack/tree/master/Language/Assembly/ARM-GNU-Assembly/Instruction/bvc)
>
> [bvs](https://github.com/BiscuitOS/HardStack/tree/master/Language/Assembly/ARM-GNU-Assembly/Instruction/bvs)
>
> [cmn](https://github.com/BiscuitOS/HardStack/tree/master/Language/Assembly/ARM-GNU-Assembly/Instruction/cmn)
>
> [cmp](https://github.com/BiscuitOS/HardStack/tree/master/Language/Assembly/ARM-GNU-Assembly/Instruction/cmp)
>
> [eor](https://github.com/BiscuitOS/HardStack/tree/master/Language/Assembly/ARM-GNU-Assembly/Instruction/eor)
>
> [ldmda](https://github.com/BiscuitOS/HardStack/tree/master/Language/Assembly/ARM-GNU-Assembly/Instruction/ldmda)
>
> [ldmdb](https://github.com/BiscuitOS/HardStack/tree/master/Language/Assembly/ARM-GNU-Assembly/Instruction/ldmdb)
>
> [ldmea](https://github.com/BiscuitOS/HardStack/tree/master/Language/Assembly/ARM-GNU-Assembly/Instruction/ldmea)
>
> [ldmed](https://github.com/BiscuitOS/HardStack/tree/master/Language/Assembly/ARM-GNU-Assembly/Instruction/ldmed)
>
> [ldmfa](https://github.com/BiscuitOS/HardStack/tree/master/Language/Assembly/ARM-GNU-Assembly/Instruction/ldmfa)
>
> [ldmfd](https://github.com/BiscuitOS/HardStack/tree/master/Language/Assembly/ARM-GNU-Assembly/Instruction/ldmfd)
>
> [ldmia](https://github.com/BiscuitOS/HardStack/tree/master/Language/Assembly/ARM-GNU-Assembly/Instruction/ldmia)
>
> [ldmib](https://github.com/BiscuitOS/HardStack/tree/master/Language/Assembly/ARM-GNU-Assembly/Instruction/ldmib)
>
> [ldr](https://github.com/BiscuitOS/HardStack/tree/master/Language/Assembly/ARM-GNU-Assembly/Instruction/ldr)
>
> [lsl](https://github.com/BiscuitOS/HardStack/tree/master/Language/Assembly/ARM-GNU-Assembly/Instruction/lsl)
>
> [lsr](https://github.com/BiscuitOS/HardStack/tree/master/Language/Assembly/ARM-GNU-Assembly/Instruction/lsr)
>
> [mcr](https://github.com/BiscuitOS/HardStack/tree/master/Language/Assembly/ARM-GNU-Assembly/Instruction/mcr)
>
> [mla](https://github.com/BiscuitOS/HardStack/tree/master/Language/Assembly/ARM-GNU-Assembly/Instruction/mla)
>
> [mov](https://github.com/BiscuitOS/HardStack/tree/master/Language/Assembly/ARM-GNU-Assembly/Instruction/mov)
>
> [mrc](https://github.com/BiscuitOS/HardStack/tree/master/Language/Assembly/ARM-GNU-Assembly/Instruction/mrc)
>
> [mul](https://github.com/BiscuitOS/HardStack/tree/master/Language/Assembly/ARM-GNU-Assembly/Instruction/mul)
>
> [mvn](https://github.com/BiscuitOS/HardStack/tree/master/Language/Assembly/ARM-GNU-Assembly/Instruction/mvn)
>
> [orr](https://github.com/BiscuitOS/HardStack/tree/master/Language/Assembly/ARM-GNU-Assembly/Instruction/orr)
>
> [ror](https://github.com/BiscuitOS/HardStack/tree/master/Language/Assembly/ARM-GNU-Assembly/Instruction/ror)
>
> [rsb](https://github.com/BiscuitOS/HardStack/tree/master/Language/Assembly/ARM-GNU-Assembly/Instruction/rsb)
>
> [rsc](https://github.com/BiscuitOS/HardStack/tree/master/Language/Assembly/ARM-GNU-Assembly/Instruction/rsc)
>
> [sbc](https://github.com/BiscuitOS/HardStack/tree/master/Language/Assembly/ARM-GNU-Assembly/Instruction/sbc)
>
> [stmda](https://github.com/BiscuitOS/HardStack/tree/master/Language/Assembly/ARM-GNU-Assembly/Instruction/stmda)
>
> [stmdb](https://github.com/BiscuitOS/HardStack/tree/master/Language/Assembly/ARM-GNU-Assembly/Instruction/stmdb)
>
> [stmea](https://github.com/BiscuitOS/HardStack/tree/master/Language/Assembly/ARM-GNU-Assembly/Instruction/stmea)
>
> [stmed](https://github.com/BiscuitOS/HardStack/tree/master/Language/Assembly/ARM-GNU-Assembly/Instruction/stmed)
>
> [stmfa](https://github.com/BiscuitOS/HardStack/tree/master/Language/Assembly/ARM-GNU-Assembly/Instruction/stmfa)
>
> [stmfd](https://github.com/BiscuitOS/HardStack/tree/master/Language/Assembly/ARM-GNU-Assembly/Instruction/stmfd)
>
> [stmia](https://github.com/BiscuitOS/HardStack/tree/master/Language/Assembly/ARM-GNU-Assembly/Instruction/stmia)
>
> [stmib](https://github.com/BiscuitOS/HardStack/tree/master/Language/Assembly/ARM-GNU-Assembly/Instruction/stmib)
>
> [str](https://github.com/BiscuitOS/HardStack/tree/master/Language/Assembly/ARM-GNU-Assembly/Instruction/str)
>
> [sub](https://github.com/BiscuitOS/HardStack/tree/master/Language/Assembly/ARM-GNU-Assembly/Instruction/sub)
>
> [swp](https://github.com/BiscuitOS/HardStack/tree/master/Language/Assembly/ARM-GNU-Assembly/Instruction/swp)
>
> [teq](https://github.com/BiscuitOS/HardStack/tree/master/Language/Assembly/ARM-GNU-Assembly/Instruction/teq)
>
> [tst](https://github.com/BiscuitOS/HardStack/tree/master/Language/Assembly/ARM-GNU-Assembly/Instruction/tst)

-----------------------------------------------

# <span id="附录">附录</span>

> [ARM inline-assembly usermanual](http://www.ethernut.de/en/documents/arm-inline-asm.html)
>
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


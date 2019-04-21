---
layout:             post
title:              "ARM Linux 5.0 -- arch/arm/kernel/head.S"
date:               2019-04-20 08:41:30 +0800
categories:         [MMU]
excerpt:            arch/arm/kernel/head.S.
tags:
  - MMU
---

> [GitHub Bootstrap ARM](https://github.com/BiscuitOS/Bootstrap_arm)
>
> Email: BuddyZhang1 <buddy.zhang@aliyun.com>
>
> Info: ARM 32 -- Linux 5.0

# 目录

> - [简介](#简介)
>
> - [实践](#实践)
>
> - [附录](#附录)

--------------------------------------------------------------
<span id="简介"></span>

![MMU](https://raw.githubusercontent.com/EmulateSpace/PictureSet/master/BiscuitOS/kernel/IND00000A.jpg)

# 简介


--------------------------------------------------------------
<span id="实践"></span>

![MMU](https://raw.githubusercontent.com/EmulateSpace/PictureSet/master/BiscuitOS/kernel/IND00000A.jpg)

# 实践

> - [vmlinux 入口：第一行运行的代码](#vmlinux first code)
>
> - [__hyp_stub_install](#__hyp_stub_install)
>
> - [__lookup_processor_type](#__lookup_processor_type)
>
> - [__proc_info](#__proc_info)
>
> - [__vet_atags](#__vet_atags)
>
> - [__fixup_smp](#__fixup_smp)
>
> - [__do_fixup_smp_on_up](#__do_fixup_smp_on_up)

### <span id="vmlinux first code">vmlinux 入口：第一行运行的代码</span>

内核源码经过编译汇编成目标文件，再通过链接脚本链接成可执行 ELF 文件 vmlinux。vmlinux
位于源码顶层目录，以及使用的链接脚本是 arch/arm/kernel/vmlinux.lds.S。首先开发者应该
找到 vmlinux 的入口地址，然后从入口地址开始调试内核。为了确定 vmlinux 内核的起始地址，
首先通过 vmlinux.lds.S 链接脚本进行分析。

##### vmlinux.lds.S

vmlinux.lds.S 用于说明内核源码的链接过程，通过链接脚本将所有目标文件链接成一个
ELF 文件，这个文件就是源码顶层目录的 vmlinux。通过分析 vmlinux 对内核的启动
和内存布局都有学习有一定帮助。vmlinux.lds.S 位于 arch/arm/kernel 目录下，经过
预处理，vmlinux.lds.S 会生成 vmlinux.lds。为了了解 ARM linux 的启动过程，需要
通过对链接脚本的分析，从而找到 vmlinux 的入口函数。具体分析如下：

{% highlight haskell %}
/* SPDX-License-Identifier: GPL-2.0 */
/* ld script to make ARM Linux kernel
 * taken from the i386 version by Russell King
 * Written by Martin Mares <mj@atrey.karlin.mff.cuni.cz>
 */

#ifdef CONFIG_XIP_KERNEL
#include "vmlinux-xip.lds.S"
#else

#include <asm-generic/vmlinux.lds.h>
#include <asm/cache.h>
#include <asm/thread_info.h>
#include <asm/memory.h>
#include <asm/mpu.h>
#include <asm/page.h>
#include <asm/pgtable.h>

#include "vmlinux.lds.h"

OUTPUT_ARCH(arm)
ENTRY(stext)
{% endhighlight %}

首先 vmlinux.lds.S 起始处的代码如上，根据链接脚本语法，可以知道 OUTPUT_ARCH 关键字
指定了链接之后的输出文件的体系结构是 ARM。ENTRY 关键字指定了输出文件 vmlinux 的入口
地址是 stext, 因此开发者只需找到 stext 的定义就可以知道 vmlinux 的入口函数。接下来
的代码是：

{% highlight haskell %}
SECTIONS
{
        /*
         * XXX: The linker does not define how output sections are
         * assigned to input sections when there are multiple statements
         * matching the same input section name.  There is no documented
         * order of matching.
         *
         * unwind exit sections must be discarded before the rest of the
         * unwind sections get included.
         */
        /DISCARD/ : {
                ARM_DISCARD
#ifndef CONFIG_SMP_ON_UP
                *(.alt.smp.init)
#endif
        }

        . = PAGE_OFFSET + TEXT_OFFSET;
        .head.text : {
                _text = .;
                HEAD_TEXT
        }
{% endhighlight %}

关键字 SECTIONS 定义了 vmlinux 内各个 section 的布局。其中关键 /DISCARD/ 定义
了 vmlinux 在生成过程了，将输入文件指定的段丢弃，这里不做过多讨论。接着链接脚本
指定了一下内存的地址，使当前地址指向了 "PAGE_OFFSET + TEXT_OFFSET", 这里先看看
两个宏的函数，首先是 TEXT_OFFSET, 其定义在 arch/arm/Makefile, 如下：

{% highlight haskell %}
# Text offset. This list is sorted numerically by address in order to
# provide a means to avoid/resolve conflicts in multi-arch kernels.
textofs-y       := 0x00008000
# We don't want the htc bootloader to corrupt kernel during resume
textofs-$(CONFIG_PM_H1940)      := 0x00108000
# SA1111 DMA bug: we don't want the kernel to live in precious DMA-able memory
ifeq ($(CONFIG_ARCH_SA1100),y)
textofs-$(CONFIG_SA1111) := 0x00208000
endif
textofs-$(CONFIG_ARCH_MSM8X60) := 0x00208000
textofs-$(CONFIG_ARCH_MSM8960) := 0x00208000
textofs-$(CONFIG_ARCH_MESON) := 0x00208000
textofs-$(CONFIG_ARCH_AXXIA) := 0x00308000

# The byte offset of the kernel image in RAM from the start of RAM.
TEXT_OFFSET := $(textofs-y)
{% endhighlight %}

因此可以知道 TEXT_OFFSET 的含义就是内核镜像在 DRAM 内的偏移，从上面的定义来看，
默认情况下，内核镜像位于 DRAM 0x00008000 处。接着看一下 PAGE_OFFSET 的定义，
定义在 arch/arm/include/asm/memory.h 里，如下：

{% highlight haskell %}
/* PAGE_OFFSET - the virtual address of the start of the kernel image */
#define PAGE_OFFSET             UL(CONFIG_PAGE_OFFSET)
{% endhighlight %}

从上面的定义可以知道，PAGE_OFFSET 用于指定内核镜像的起始虚拟地址。PAGE_OFFSET 的定
义与 CONFIG_PAGE_OFFSET 有关，该宏定义在 arch/arm/Kconfig 里，如下：

{% highlight haskell %}
choice
        prompt "Memory split"
        depends on MMU
        default VMSPLIT_3G
        help
          Select the desired split between kernel and user memory.

          If you are not absolutely sure what you are doing, leave this
          option alone!

        config VMSPLIT_3G
                bool "3G/1G user/kernel split"
        config VMSPLIT_3G_OPT
                depends on !ARM_LPAE
                bool "3G/1G user/kernel split (for full 1G low memory)"
        config VMSPLIT_2G
                bool "2G/2G user/kernel split"
        config VMSPLIT_1G
                bool "1G/3G user/kernel split"

config PAGE_OFFSET
        hex
        default PHYS_OFFSET if !MMU
        default 0x40000000 if VMSPLIT_1G
        default 0x80000000 if VMSPLIT_2G
        default 0xB0000000 if VMSPLIT_3G_OPT
        default 0xC0000000
{% endhighlight %}

根据 CONFIG_PAGE_OFFSET 宏定义可以知道，其值与 MMU，VMSPLIT_1G, VMSPLIT_2G,
VMSPLIT_3G_OPT 的定义有关，这几个宏的定义如上。开发者可以在
BiscuitOS/output/linux-5.x-arm32/linux/linux/.config 中查看当前内核配置，其
本实践平台基于 vexpress 平台，使用的是 VMSPLIT_2G，所以 PAGE_OFFSET 的值为
0x8000000。

分析完上面两个宏的定义之后，再回到之前的链接脚本源码：

{% highlight haskell %}
        . = PAGE_OFFSET + TEXT_OFFSET;
        .head.text : {
                _text = .;
                HEAD_TEXT
        }
{% endhighlight %}

将当前地址设置为 "PAGE_OFFSET + TEXT_OFFSET", 其含义就是内核镜像的起始虚拟地址
加上内核镜像在 DRAM 中的偏移。接着定义了一个输出段 .head.text, 段内容首先定义了
一个 _text 指向当前地址，然后将 HEAD_TEXT 加入到段内，HEAD_TEXT 的定义如下：

{% highlight haskell %}
/* Section used for early init (in .S files) */
#define HEAD_TEXT  KEEP(*(.head.text))
{% endhighlight %}

HEAD_TEXT 的定义使用了 KEEP 关键字，其作用就是保留所有输入文件的 .head.text 段。
更多 KEEP 使用方法，请看：

> [LD scripts 关键字 -- KEEP](https://biscuitos.github.io/blog/LD-KEEP/)

从上面的定义可以知道，输出文件的 .head.text 段包含了所有输入文件的 .head.text 段，
因此可以知道内核镜像文件中，最前面位置的代码位于 .head.text 段中，vmlinux 的入口
函数 stext 也可能位于 .head.text 段中，因此在 arch/arm/kernel 目录下查找含有
.head.text 段和 stext 的位置。通过查找，在 arch/arm/kernel/head.S 文件中找到
stext 的定义，如下：

{% highlight haskell %}
        .arm

        __HEAD
ENTRY(stext)
{% endhighlight %}

从这里虽然找到了 stext 的定义，但还需要确定 stext 是否位于 .head.text 段中，通过
上面的源码，查看 __HEAD 的定义，如下：

{% highlight haskell %}
/* For assembly routines */
#define __HEAD          .section        ".head.text","ax"
{% endhighlight %}

因此，__HEAD 宏就是用于指定段属性为 ".head.text", 因此根据此前的分析，
arch/arcm/kernel/head.S 中的 stext 就是 vmlinux 的入口函数。因此，开发者可以从
 stext 处开始调试内核。开发者可以在 stext 处添加断点，然后 GDB 在线调试，调试
情况如下：

{% highlight haskell %}
buddy@BDOS: BiscuitOS/output/linux-5.0-arm32$ BiscuitOS/output/linux-5.0-arm32/arm-linux-gnueabi/arm-linux-gnueabi/bin/arm-linux-gnueabi-gdb -x BiscuitOS/output/linux-5.0-arm32/package/gdb/gdb_Image
GNU gdb (Linaro_GDB-2019.02) 8.2.1.20190122-git
Copyright (C) 2018 Free Software Foundation, Inc.
License GPLv3+: GNU GPL version 3 or later <http://gnu.org/licenses/gpl.html>
This is free software: you are free to change and redistribute it.
There is NO WARRANTY, to the extent permitted by law.
Type "show copying" and "show warranty" for details.
This GDB was configured as "--host=x86_64-unknown-linux-gnu --target=arm-linux-gnueabi".
Type "show configuration" for configuration details.
For bug reporting instructions, please see:
<http://www.gnu.org/software/gdb/bugs/>.
Find the GDB manual and other documentation resources online at:
    <http://www.gnu.org/software/gdb/documentation/>.

For help, type "help".
Type "apropos word" to search for commands related to "word".
warning: No executable has been specified and target does not support
determining executable automatically.  Try using the "file" command.
0x60000000 in ?? ()
add symbol table from file "/xspace/OpenSource/BiscuitOS/BiscuitOS/output/linux-5.0-arm32/linux/linux/vmlinux" at
	.text_addr = 0x60100000
	.head.text_addr = 0x60008000
	.rodata_addr = 0x60800000
(gdb) b stext
Breakpoint 1 at 0x60008000: file arch/arm/kernel/head.S, line 89.
(gdb) c
Continuing.

Breakpoint 1, stext () at arch/arm/kernel/head.S:89
89		bl	__hyp_stub_install
(gdb) info reg pc
pc             0x60008000          0x60008000 <stext>
(gdb)
{% endhighlight %}

从上面的调试可以看出，stext 确实是内核所在的物理起始地址，此时 pc 正好指向内核
解压之后的起始地址 0x60008000, 因此 arch/arm/kernel/head.S 目录下的内容就是
vmlinux 的入口函数 stext 所在的位置。接下来，开发者可以从这个文件开始，一步步
调试内核源码。

首先，开发者可以查看一下这部分代码的注释介绍，如下：

{% highlight haskell %}
/*
 * Kernel startup entry point.
 * ---------------------------
 *
 * This is normally called from the decompressor code.  The requirements
 * are: MMU = off, D-cache = off, I-cache = dont care, r0 = 0,
 * r1 = machine nr, r2 = atags or dtb pointer.
 *
 * This code is mostly position independent, so if you link the kernel at
 * 0xc0008000, you call this at __pa(0xc0008000).
 *
 * See linux/arch/arm/tools/mach-types for the complete list of machine
 * numbers for r1.
 *
 * We're trying to keep crap to a minimum; DO NOT add any machine specific
 * crap here - that's what the boot loader (or in extreme, well justified
 * circumstances, zImage) is for.
 */
        .arm

        __HEAD
ENTRY(stext)
{% endhighlight %}

注释中说明了 stext 为内核启动的入口，当 zImage 解压完内核之后，这里就会被最先调用。
为了调用这个入口，MMU 必须关闭，D-cache 必须关闭，I-cache 无所谓，r0 必须为 0，
r1 必须存储体系相关的 ID，以及 r2 必须存储着 ATAG 或者 DTB 的指针。内核镜像的位置
通常都是独立的，如果需要将内核镜像的虚拟地址设置为 0xc0008000, 那么内核镜像的物理
地址可以通过 __pa(0xc0008000) 获得。r1 寄存器中包含了体系相关的 ID 信息，内核在
linux/arch/arm/tools/mach-types 文件中描述了 ID 对应的信息。这里还提示，如果
没有合适的理由，不要试图这段代码。开发者可以使用 stext 作为断点，然后使用 GDB 进行
调试，以此实践上面的内容，实践如下：

{% highlight haskell %}
.text_addr = 0x60100000
.head.text_addr = 0x60008000
.rodata_addr = 0x60800000
(gdb) b stext
Breakpoint 1 at 0x60008000: file arch/arm/kernel/head.S, line 89.
(gdb) c
Continuing.

Breakpoint 1, stext () at arch/arm/kernel/head.S:89
89		bl	__hyp_stub_install
(gdb) info reg pc
pc             0x60008000          0x60008000 <stext>
(gdb) info reg r1
r1             0x8e0               2272
(gdb) info reg r2
r2             0x69cf7000          1775202304
(gdb) info reg r0
r0             0x0                 0
(gdb)
{% endhighlight %}

首先查看当前地址是 0x60008000, 因此此时内核和用户空间划分采用的 1:1。然后 r1 寄存器的值是
0x8e0,此时查看 arch/arm/tools/mach-types 文件，查看对应的内容如下：

{% highlight haskell %}
spear300                MACH_SPEAR300           SPEAR300                2237
lilly1131               MACH_LILLY1131          LILLY1131               2239
hmt                     MACH_HMT                HMT                     2254
vexpress                MACH_VEXPRESS           VEXPRESS                2272
d2net                   MACH_D2NET              D2NET                   2282
{% endhighlight %}

从中可知，目前实践采用的是 vexpress。由于本实践采用了 DTB，因此 r2 寄存器的值指向了 DTB 在
内存中的位置。分析完上面的代码之后，接下来分析源码如下：

{% highlight haskell %}
ENTRY(stext)
 ARM_BE8(setend be )                    @ ensure we are in BE8 mode

 THUMB( badr    r9, 1f          )       @ Kernel is always entered in ARM.
 THUMB( bx      r9              )       @ If this is a Thumb-2 kernel,
 THUMB( .thumb                  )       @ switch to Thumb now.
 THUMB(1:                       )

#ifdef CONFIG_ARM_VIRT_EXT
        bl      __hyp_stub_install
#endif
            MACH_D2NET              D2NET                   2282
{% endhighlight %}

由于本实践不支持 THUMB 以及 ARM_BE8, 但支持 CONFIG_ARM_VIRT_EXT 宏，因此上面真正执行的代码
只有 "bl __hyp_stub_install" 行代码，其源码如下：

<span id="__hyp_stub_install"></span>

{% highlight haskell %}
/*
 * Hypervisor stub installation functions.
 *
 * These must be called with the MMU and D-cache off.
 * They are not ABI compliant and are only intended to be called from the kernel
 * entry points in head.S.
 */
@ Call this from the primary CPU
ENTRY(__hyp_stub_install)
        store_primary_cpu_mode  r4, r5, r6
ENDPROC(__hyp_stub_install)

        @ fall through...

@ Secondary CPUs should call here
ENTRY(__hyp_stub_install_secondary)
        mrs     r4, cpsr
        and     r4, r4, #MODE_MASK

        /*
         * If the secondary has booted with a different mode, give up
         * immediately.
         */
        compare_cpu_mode_with_primary   r4, r5, r6, r7
        retne   lr

        /*
         * Once we have given up on one CPU, we do not try to install the
         * stub hypervisor on the remaining ones: because the saved boot mode
         * is modified, it can't compare equal to the CPSR mode field any
         * more.
         *
         * Otherwise...
         */

        cmp     r4, #HYP_MODE
        retne   lr                      @ give up if the CPU is not in HYP mode
{% endhighlight %}

__hyp_stub_install 也很简单，直接调用 store_primay_cpu_mode 宏，该宏的定义与 ZIMAGE 宏的定义
有关，通过本实践可以知道，ZIMAGE 宏是支持的，因此 store_primay_cpu_mode 的函数定义如下：

{% highlight haskell %}
.macro  store_primary_cpu_mode  reg1:req, reg2:req, reg3:req
.endm
{% endhighlight %}

从上面的定义来看， store_primay_cpu_mode 不做任何动作，因此 __hyp_stub_install 直接调用
__hyp_stub_install_secondary, 通过上面的源码可以知道，__hyp_stub_install_secondary 首先
执行 "mrs r4, cpsr" 代码，以此获得当前模式的 CPSR 寄存器，然后执行 "and r4, r4, #MODE_MASK"
代码，以此从 CPSR 中独立出模式域。接着调用 compare_cpu_mode_with_primary, 与 store_primary_cpu_mode
一样，其定义与 ZIMAGE 宏有关，因此其定义如下：

{% highlight haskell %}
/*
 * The zImage loader only runs on one CPU, so we don't bother with mult-CPU
 * consistency checking:
 */
        .macro  compare_cpu_mode_with_primary mode, reg1, reg2, reg3
        cmp     \mode, \mode
        .endm
{% endhighlight %}

因此，__hyp_stub_install_secondary 宏也不做任何实际工作买就是将 CPSR ZF 置位。接下
会判断对比的结果是否为零，如果不为零，则调用 "retne lr" 返回，但是从上面的分析，那么
这里为零不可能返回，那么接下来调用 "cmp r4, #HYP_MODE" 代码，以此检查当前模式是否
是 HYP 模式，如果不是就调用 "retne lr" 命令进行返回。由于在本次实践中，当前模式不是 HYP
模式，因此直接返回。由于剩下的代码不被执行，这里不做分析，开发者可以通过 GDB 断点调试这部分
代码，调试情况如下：

{% highlight haskell %}
.text_addr = 0x60100000
.head.text_addr = 0x60008000
.rodata_addr = 0x60800000
(gdb) b stext
Breakpoint 1 at 0x60008000: file arch/arm/kernel/head.S, line 89.
(gdb) c
Continuing.

Breakpoint 1, stext () at arch/arm/kernel/head.S:89
warning: Source file is more recent than executable.
89		bl	__hyp_stub_install
(gdb) s
__hyp_stub_install () at arch/arm/kernel/hyp-stub.S:89
89		store_primary_cpu_mode	r4, r5, r6
(gdb) n
__hyp_stub_install_secondary () at arch/arm/kernel/hyp-stub.S:96
96		mrs	r4, cpsr
(gdb) info reg cpsr
cpsr           0x800001d3          -2147483181
(gdb) n
97		and	r4, r4, #MODE_MASK
(gdb) n
103		compare_cpu_mode_with_primary	r4, r5, r6, r7
(gdb) info reg r4
r4             0x13                19
(gdb) n
104		retne	lr
(gdb) n
115		cmp	r4, #HYP_MODE
(gdb) n
116		retne	lr			@ give up if the CPU is not in HYP mode
(gdb) info reg cpsr
cpsr           0x800001d3          -2147483181
(gdb) n
stext () at arch/arm/kernel/head.S:92
92		safe_svcmode_maskall r9
(gdb)
{% endhighlight %}

实践结果和预期分析一致，这里对 HYP 模式不做过多解释，接下来执行的代码如下：

{% highlight haskell %}
@ ensure svc mode and all interrupts masked
safe_svcmode_maskall r9
{% endhighlight %}

这段代码主要是进入 SVC 模式并且屏蔽所有的中断，由于此时已经是 SVC 模式了，因此
不对该代码进行讲解，接下来执行的代码如下：

{% highlight haskell %}
mrc     p15, 0, r9, c0, c0              @ get processor id
bl      __lookup_processor_type         @ r5=procinfo r9=cpuid
{% endhighlight %}

接下来通过 mrc 指令从 cp15 c0 中获得体系相关的 ID 信息，此时 CP15 C0 的布局如下：

![MMU](https://raw.githubusercontent.com/EmulateSpace/PictureSet/master/BiscuitOS/boot/BOOTCP15C0.png)

此时选中了 MIDR (Main ID Register) 寄存器，此时 MIDR 的布局如下：

![MMU](https://raw.githubusercontent.com/EmulateSpace/PictureSet/master/BiscuitOS/boot/BOOT000025.png)

此时 r9 寄存器存储这体系相关 ID 信息，并调用 __lookup_processor_type, 该函数源码如下：

<span id="__lookup_processor_type"></span>

{% highlight haskell %}
/*
 * Read processor ID register (CP#15, CR0), and look up in the linker-built
 * supported processor list.  Note that we can't use the absolute addresses
 * for the __proc_info lists since we aren't running with the MMU on
 * (and therefore, we are not in the correct address space).  We have to
 * calculate the offset.
 *
 *      r9 = cpuid
 * Returns:
 *      r3, r4, r6 corrupted
 *      r5 = proc_info pointer in physical address space
 *      r9 = cpuid (preserved)
 */
__lookup_processor_type:
        adr     r3, __lookup_processor_type_data
        ldmia   r3, {r4 - r6}
        sub     r3, r3, r4                      @ get offset between virt&phys
        add     r5, r5, r3                      @ convert virt addresses to
        add     r6, r6, r3                      @ physical address space
1:      ldmia   r5, {r3, r4}                    @ value, mask
        and     r4, r4, r9                      @ mask wanted bits
        teq     r3, r4
        beq     2f
        add     r5, r5, #PROC_INFO_SZ           @ sizeof(proc_info_list)
        cmp     r5, r6
        blo     1b
        mov     r5, #0                          @ unknown processor
2:      ret     lr
ENDPROC(__lookup_processor_type)

/*
 * Look in <asm/procinfo.h> for information about the __proc_info structure.
 */
        .align  2
        .type   __lookup_processor_type_data, %object
__lookup_processor_type_data:
        .long   .
        .long   __proc_info_begin
        .long   __proc_info_end
        .size   __lookup_processor_type_data, . - __lookup_processor_type_data
{% endhighlight %}

__lookup_processor_type 首先调用 adr 指令获得 __lookup_processor_type_data 的物理地址，
__lookup_processor_type_data 的定义如上，__lookup_processor_type_data 定义为一个 object，
内部定义了三个 long 变量，第一个 long 变量存储了 __lookup_processor_type_data 的虚拟地址；第二个
long 变量存储了 __proc_info_begin 地址；第三个 long 变量存储了 __proc_info_end 地址。

<span id="__proc_info"></span>

__proc_info_begin 和 __proc_info_end 的定义在 arch/arm/kernel/vmlinux.lds.S 中，如下：

{% highlight haskell %}
.text : {                       /* Real text segment            */
        _stext = .;             /* Text and read-only data      */
        ARM_TEXT
}
{% endhighlight %}

定义在 ARM_TEXT 中，其定义在 arch/arm/kernel/vmlinux/lds.h 里，如下：

{% highlight haskell %}
#define ARM_TEXT                                                        \
                IDMAP_TEXT                                              \
                __entry_text_start = .;                                 \
                *(.entry.text)                                          \
                __entry_text_end = .;                                   \
                IRQENTRY_TEXT                                           \
                SOFTIRQENTRY_TEXT                                       \
                TEXT_TEXT                                               \
                SCHED_TEXT                                              \
                CPUIDLE_TEXT                                            \
                LOCK_TEXT                                               \
                HYPERVISOR_TEXT                                         \
                KPROBES_TEXT                                            \
                *(.gnu.warning)                                         \
                *(.glue_7)                                              \
                *(.glue_7t)                                             \
                . = ALIGN(4);                                           \
                *(.got)                 /* Global offset table */       \
                ARM_CPU_KEEP(PROC_INFO)
{% endhighlight %}

其中 ARM_CPU_KEEP(PROC_INFO) 包含了 __proc_info 相关的段，定义如下：

{% highlight haskell %}
#ifdef CONFIG_HOTPLUG_CPU
#define ARM_CPU_DISCARD(x)
#define ARM_CPU_KEEP(x)         x
#else
#define ARM_CPU_DISCARD(x)      x
#define ARM_CPU_KEEP(x)
#endif

#define PROC_INFO                                                       \
                . = ALIGN(4);                                           \
                __proc_info_begin = .;                                  \
                *(.proc.info.init)                                      \
                __proc_info_end = .;
{% endhighlight %}

由于本实践中，宏 CONFIG_HOTPLUG_CPU 启用，因此 ARM_CPU_KEEP 宏将 PROC_INFO 段
保留下来了，从定义可以看出，PROC_INFO 链接时将所有输入文件的 .proc.info.init 段都
加在了 ARM_TEXT 段的最后，即输出文件 vmlinux 的 .text 的末尾。并且定义了两个变量
__proc_info_begin 和 __proc_info_end 指向所有 .proc_info_init 段的起始虚拟地址
和终止虚拟地址。.proc.info.init 对应的数据结构定义在 arch/arm/include/asm/procinfo.h
文件中，定义如下：

{% highlight haskell %}
/*
 * Note!  struct processor is always defined if we're
 * using MULTI_CPU, otherwise this entry is unused,
 * but still exists.
 *
 * NOTE! The following structure is defined by assembly
 * language, NOT C code.  For more information, check:
 *  arch/arm/mm/proc-*.S and arch/arm/kernel/head.S
 */
struct proc_info_list {
        unsigned int            cpu_val;
        unsigned int            cpu_mask;
        unsigned long           __cpu_mm_mmu_flags;     /* used by head.S */
        unsigned long           __cpu_io_mmu_flags;     /* used by head.S */
        unsigned long           __cpu_flush;            /* used by head.S */
        const char              *arch_name;
        const char              *elf_name;
        unsigned int            elf_hwcap;
        const char              *cpu_name;
        struct processor        *proc;
        struct cpu_tlb_fns      *tlb;
        struct cpu_user_fns     *user;
        struct cpu_cache_fns    *cache;
};
{% endhighlight %}

获得上面的定义之后，开发者继续在 arch/arm/ 目录下查找包含 .proc.info.init 的段，由于本实践
基于 ARMv7 架构，因此找到 arch/arm/mm/proc-v7.S 中包含该段，定义如下：

{% highlight haskell %}
.section ".proc.info.init", #alloc

/*
 * Standard v7 proc info content
 */
.macro __v7_proc name, initfunc, mm_mmuflags = 0, io_mmuflags = 0, hwcaps = 0, proc_fns = v7_processor_functions, cache_fns = v7_cache_fns
ALT_SMP(.long   PMD_TYPE_SECT | PMD_SECT_AP_WRITE | PMD_SECT_AP_READ | \
                PMD_SECT_AF | PMD_FLAGS_SMP | \mm_mmuflags)
ALT_UP(.long    PMD_TYPE_SECT | PMD_SECT_AP_WRITE | PMD_SECT_AP_READ | \
                PMD_SECT_AF | PMD_FLAGS_UP | \mm_mmuflags)
.long   PMD_TYPE_SECT | PMD_SECT_AP_WRITE | \
        PMD_SECT_AP_READ | PMD_SECT_AF | \io_mmuflags
initfn  \initfunc, \name
.long   cpu_arch_name
.long   cpu_elf_name
.long   HWCAP_SWP | HWCAP_HALF | HWCAP_THUMB | HWCAP_FAST_MULT | \
        HWCAP_EDSP | HWCAP_TLS | \hwcaps
.long   cpu_v7_name
.long   \proc_fns
.long   v7wbi_tlb_fns
.long   v6_user_fns
.long   \cache_fns
.endm

......

/*
 * Match any ARMv7 processor core.
 */
.type   __v7_proc_info, #object
__v7_proc_info:
.long   0x000f0000              @ Required ID value
.long   0x000f0000              @ Mask for ID
__v7_proc __v7_proc_info, __v7_setup
.size   __v7_proc_info, . - __v7_proc_info
{% endhighlight %}

里面定义了很多 struct proc_info_list 结构，每个只做与本实践有关的分析，其他分析请
参考完成。通过上面的分析之后，可以知道 ARM 内核是如何定义 __proc_info table. 再回到
__lookup_processor_type 函数，代码如下：

{% highlight haskell %}
__lookup_processor_type:
        adr     r3, __lookup_processor_type_data
        ldmia   r3, {r4 - r6}
        sub     r3, r3, r4                      @ get offset between virt&phys
        add     r5, r5, r3                      @ convert virt addresses to
        add     r6, r6, r3                      @ physical address space
1:      ldmia   r5, {r3, r4}                    @ value, mask
        and     r4, r4, r9                      @ mask wanted bits
        teq     r3, r4
        beq     2f
        add     r5, r5, #PROC_INFO_SZ           @ sizeof(proc_info_list)
        cmp     r5, r6
        blo     1b
        mov     r5, #0                          @ unknown processor
2:      ret     lr
ENDPROC(__lookup_processor_type)
{% endhighlight %}

在获得 __lookup_processor_type_data 地址之后，调用 "ldmia r3, {r4 - r6}" 代码，将
__lookup_processor_type_data 定义的 3 个 long 分别赋值到 r4,r5,r6 寄存器中，此时
r4 寄存器存储这 __lookup_processor_type_data 的虚拟地址; r5 寄存器存储着
__proc_info_begin 的虚拟地址；r6 寄存器存储着 __proc_info_end 的虚拟地址。
接着调用 "sub r3, r3, r4" 代码，此时 r3 寄存器存储着 __lookup_processor_type_data 物理地址，
r4 寄存器存储着 __lookup_processor_type_data 的虚拟地址，所以这条代码的作用就是计算虚拟地址
转换成物理地址的偏移，接着连续调用几条 add 指令就是将 __proc_info_begin 和 __proc_info_end
的虚拟地址转换成对应的物理地址。此时 r5 寄存器存储着 __proc_info_begin 的物理地址，接着调用
"ldmia r5, {r3, r4}" 代码，ldmia 指令，获得 r5 对应物理地址处两个 long 字节的内容，存储到
r3 和 r4 寄存器中。从之前的分析可以知道 arch/arm/mm/proc-v7.S 的 .proc.info.init 段中
包含了 ARMv7 家族支持的 proc_info 表，因此接下来的代码就是根据体系相关的 ID 找到对应的
proc_info。因此执行完 "ldmia r5, {r3, r4}" 代码之后，r3 寄存器存储着每个 struct proc_info_list
结构的 cpu_val 值，r4 寄存器存储着 cpu_mask 的值，由之前的代码可以知道，此时 r9 寄存器存储着
体系相关的 ID 信息，调用 "and r4, r4, r9" 指令进行掩码操作，获得预期的结果。接着调用
"teq r3, r4" 命令，以此查看是否与 r3 寄存器的值相同，如果相同，那么跳转到 2，并直接返回；
如果不相同，那么将 r5 寄存的值加上 PROC_INFO_SZ,以此遍历到下一个 struct proc_info_list
结构，接着调用 cmp 指令判断此时是不是已经到达 __proc_info_end，如果到达则将 r5 寄存器
的值设置为 0，表示没有找打对应的寄存器；如果没有达到，那么继续查找剩下的
struct proc_info_list。通过上面的代码，CPU 可以找到对应的 proc_info_list 结构。
开发者可以在适当的位置添加断点，然后使用 GDB 调试这个过程，实践情况如下：

{% highlight haskell %}
.text_addr = 0x60100000
.head.text_addr = 0x60008000
.rodata_addr = 0x60800000
(gdb) b stext
Breakpoint 1 at 0x60008000: file arch/arm/kernel/head.S, line 89.
(gdb) c
Continuing.

Breakpoint 1, stext () at arch/arm/kernel/head.S:89
warning: Source file is more recent than executable.
89		bl	__hyp_stub_install
(gdb) n
92		safe_svcmode_maskall r9
(gdb) b
Breakpoint 2 at 0x60008004: file arch/arm/kernel/head.S, line 92.
(gdb) n
94		mrc	p15, 0, r9, c0, c0		@ get processor id
(gdb) n
95		bl	__lookup_processor_type		@ r5=procinfo r9=cpuid
(gdb) s
__lookup_processor_type () at arch/arm/kernel/head-common.S:176
warning: Source file is more recent than executable.
176		adr	r3, __lookup_processor_type_data
(gdb) n
177		ldmia	r3, {r4 - r6}
(gdb) n
178		sub	r3, r3, r4			@ get offset between virt&phys
(gdb) n
179		add	r5, r5, r3			@ convert virt addresses to
(gdb) n
180		add	r6, r6, r3			@ physical address space
(gdb) n
181	1:	ldmia	r5, {r3, r4}			@ value, mask
(gdb) n
182		and	r4, r4, r9			@ mask wanted bits
(gdb) info reg r3 r4 r5 r9
r3             0x410fc050          1091551312
r4             0xff0ffff0          -15728656
r5             0x60728890          1618118800
r9             0x410fc090          1091551376
(gdb) n
183		teq	r3, r4
(gdb) n
184		beq	2f
(gdb) n
185		add	r5, r5, #PROC_INFO_SZ		@ sizeof(proc_info_list)
(gdb) n
186		cmp	r5, r6
(gdb) n
187		blo	1b
(gdb) n
181	1:	ldmia	r5, {r3, r4}			@ value, mask
(gdb) n
182		and	r4, r4, r9			@ mask wanted bits
(gdb) n
183		teq	r3, r4
(gdb) n
184		beq	2f
(gdb) info reg r3 r4 r5 r9
r3             0x410fc090          1091551376
r4             0x410fc090          1091551376
r5             0x607288c4          1618118852
r9             0x410fc090          1091551376
(gdb) n
189	2:	ret	lr
(gdb) n
stext () at arch/arm/kernel/head.S:96
96		movs	r10, r5				@ invalid processor (r5=0)?
(gdb)
{% endhighlight %}

从上面的实践可以知道，__lookup_processor_type_data 中的第二个项就是需要查找的对象，
可以通过 arch/arm/mm/proc-v7.S 查看对应的 proc_info 项如下：

{% highlight haskell %}
/*
 * ARM Ltd. Cortex A9 processor.
 */
.type   __v7_ca9mp_proc_info, #object
__v7_ca9mp_proc_info:
.long   0x410fc090
.long   0xff0ffff0
__v7_proc __v7_ca9mp_proc_info, __v7_ca9mp_setup, proc_fns = ca9mp_processor_functions
.size   __v7_ca9mp_proc_info, . - __v7_ca9mp_proc_info
{% endhighlight %}

因此，从这里实践可以知道，实践所使用的 CPU 是 Cortex A9，以及对应的 struct proc_info_list
信息。这里以后会派上用场。接着代码继续执行，如下：

{% highlight haskell %}
movs    r10, r5                         @ invalid processor (r5=0)?
THUMB( it      eq )            @ force fixup-able long branch encoding
beq     __error_p                       @ yes, error 'p'
{% endhighlight %}

从 __lookup_processor_type 返回之后，r5 寄存器存储着与体系相关的 proc_info_list 结构
的地址，如果 __lookup_processor_type 没有找到对应的 proc_info_list, 那么 r5 寄存器
的值为 0，根据这个信息，执行 "movs r10, r5" 和 "beq __error_p" 代码，如果 r5 为零，
那么跳转到 __error_p 处执行。由于实践结果可知，不会跳转到 __error_p,所以不对 __error_p
进行分析，继续执行代码：

{% highlight haskell %}
#ifdef CONFIG_ARM_LPAE
        mrc     p15, 0, r3, c0, c1, 4           @ read ID_MMFR0
        and     r3, r3, #0xf                    @ extract VMSA support
        cmp     r3, #5                          @ long-descriptor translation table format?
 THUMB( it      lo )                            @ force fixup-able long branch encoding
        blo     __error_lpae                    @ only classic page table format
#endif

#ifndef CONFIG_XIP_KERNEL
        adr     r3, 2f
        ldmia   r3, {r4, r8}
        sub     r4, r3, r4                      @ (PHYS_OFFSET - PAGE_OFFSET)
        add     r8, r8, r4                      @ PHYS_OFFSET
#else   
        ldr     r8, =PLAT_PHYS_OFFSET           @ always constant in this case
#endif
{% endhighlight %}

由于实践中宏 CONFIG_ARM_LPAE 和宏 CONFIG_XIP_KERNEL 都没有启用，因此接下来执行的代码
"adr r3, 2f", 这段代码是获得 2f 的地址，2f 处定义的内容如下：

{% highlight haskell %}
#ifndef CONFIG_XIP_KERNEL
2:      .long   .
        .long   PAGE_OFFSET
#endif
{% endhighlight %}

2f 处定义了两个 long 变量，第一个 long 存储 2 的虚拟地址；第二个存储 PAGE_OFFSET 的值，
也就是内核镜像起始虚拟地址。上面代码中，使用 "ldmia r3, {r4, r8}" 代码将 2f 的虚拟地址
存储到 r4 寄存器，并将 PAGE_OFFSET 的值存储到 r8 寄存器中。有上面的代码可以知道
r3 存储着 2f 的物理地址，r4 寄存器存储着 2f 的虚拟地址，调用 "sub r4, r3, r4"
就是将 PHYS_OFFSET 减去 PAGE_OFFSET, 然后调用 "add r8, r8, r4" 计算真实的
PHYS_OFFSET。 开发者可以在适当的位置加上断点，然后使用 GDB 进行调试，调试的情况如下：

{% highlight haskell %}
.text_addr = 0x60100000
.head.text_addr = 0x60008000
.rodata_addr = 0x60800000
(gdb) b BS_debug
Breakpoint 1 at 0x60008044: file arch/arm/kernel/head.S, line 110.
(gdb) c
Continuing.

Breakpoint 1, stext () at arch/arm/kernel/head.S:110
110		adr	r3, 2f
(gdb) n
111		ldmia	r3, {r4, r8}
(gdb) info reg r3
r3             0x60008084          1610645636
(gdb) n
112		sub	r4, r3, r4			@ (PHYS_OFFSET - PAGE_OFFSET)
(gdb) info reg r3 r4
r3             0x60008084          1610645636
r4             0x80008084          -2147450748
(gdb) n
113		add	r8, r8, r4			@ PHYS_OFFSET
(gdb) info reg r4
r4             0xe0000000          -536870912
(gdb) info reg r8
r8             0x80000000          -2147483648
(gdb) n
122		bl	__vet_atags
(gdb) info reg r3 r4 r8
r3             0x60008084          1610645636
r4             0xe0000000          -536870912
r8             0x60000000          1610612736
(gdb)
{% endhighlight %}

通过上面的计算，算出了 PHYS_OFFSET 是 0x60000000。接下来执行的代码是：

<span id="__vet_atags"></span>
{% highlight haskell %}
/*
 * r1 = machine no, r2 = atags or dtb,
 * r8 = phys_offset, r9 = cpuid, r10 = procinfo
 */
bl      __vet_atags
{% endhighlight %}

在执行 __vet_atags 之前，r1 寄存器存储着 Machine 号；r2 寄存器指向 atags 或者 dtb；
r8 存储 PHYS_OFFSET； r9 存储着体系相关的 ID； r10 寄存器存储着 proc_info。接着
执行的代码如下：

{% highlight haskell %}
/* Determine validity of the r2 atags pointer.  The heuristic requires
 * that the pointer be aligned, in the first 16k of physical RAM and
 * that the ATAG_CORE marker is first and present.  If CONFIG_OF_FLATTREE
 * is selected, then it will also accept a dtb pointer.  Future revisions
 * of this function may be more lenient with the physical address and
 * may also be able to move the ATAGS block if necessary.
 *
 * Returns:     
 *  r2 either valid atags pointer, valid dtb pointer, or zero
 *  r5, r6 corrupted
 */
__vet_atags:    
        tst     r2, #0x3                        @ aligned?
        bne     1f

        ldr     r5, [r2, #0]
#ifdef CONFIG_OF_FLATTREE
        ldr     r6, =OF_DT_MAGIC                @ is it a DTB?
        cmp     r5, r6
        beq     2f
#endif
        cmp     r5, #ATAG_CORE_SIZE             @ is first tag ATAG_CORE?
        cmpne   r5, #ATAG_CORE_SIZE_EMPTY
        bne     1f
        ldr     r5, [r2, #4]
        ldr     r6, =ATAG_CORE
        cmp     r5, r6
        bne     1f

2:      ret     lr                              @ atag/dtb pointer is ok

1:      mov     r2, #0
        ret     lr
ENDPROC(__vet_atags)
{% endhighlight %}

从注释可以知道，这段代码的主要任务就是：这里通过代码检测 ATAGS/DTB 的合法性。如果 Uboot
通过 ATAGS 方式传递参数给 kernel，ATAG_CORE 会在 RAM 的第一个 16K 处。但如果
CONFIG_OF_FLATTREE 宏启用，那么也会检查 DTB 的合法性。代码首先检查 r2 寄存器的的对齐
方式，如果没有对齐，则直接跳转到 1 处，并将 0 赋值给 r2 寄存器，然后直接返回。但如果
r2 寄存器已经对齐，那么使用 ldr 指令读取 r2 对应内存处的内容到 r5 寄存器。由于本实践
CONFIG_OF_FLATTREE 宏已经启用，那么接下来将 OF_DT_MAGIC 传递给 r6 寄存器，然后对比
r5 的内容是否与 r6 寄存器的一致。这里 OF_DT_MAGIC 的定义如下：

{% highlight haskell %}
#ifdef CONFIG_CPU_BIG_ENDIAN
#define OF_DT_MAGIC 0xd00dfeed
#else
#define OF_DT_MAGIC 0xedfe0dd0 /* 0xd00dfeed in big-endian */
#endif
{% endhighlight %}

开发者可以在上面代码适当位置添加断点，然后使用 GDB 进行调试，调试的情况如下：

{% highlight haskell %}
.text_addr = 0x60100000
.head.text_addr = 0x60008000
.rodata_addr = 0x60800000
(gdb) b BS_debug
Breakpoint 1 at 0x60008224: file arch/arm/kernel/head-common.S, line 49.
(gdb) c
Continuing.

Breakpoint 1, __vet_atags () at arch/arm/kernel/head-common.S:49
49		tst	r2, #0x3			@ aligned?
(gdb) info reg r2
r2             0x69cf7000          1775202304
(gdb) n
50		bne	1f
(gdb) n
52		ldr	r5, [r2, #0]
(gdb) n
54		ldr	r6, =OF_DT_MAGIC		@ is it a DTB?
(gdb) info reg r5
r5             0xedfe0dd0          -302117424
(gdb) n
55		cmp	r5, r6
(gdb) info reg r6
r6             0xedfe0dd0          -302117424
(gdb) n
56		beq	2f
(gdb) n
66	2:	ret	lr				@ atag/dtb pointer is ok
(gdb) n
stext () at arch/arm/kernel/head.S:123
123		bl	__fixup_smp
(gdb)
{% endhighlight %}

从上面的实践可以看出，已经检测到 DTB 的存在，并且合法。因此这里就不在继续介绍 ATAGS
相关的代码。继续执行 head.S 里面的代码，如下：

<span id="__fixup_smp"></span>
{% highlight haskell %}
#ifdef CONFIG_SMP_ON_UP
        bl      __fixup_smp
#endif
{% endhighlight %}

由于本实践启用了 CONFIG_SMP_ON_UP 宏，因此接下来执行的代码是：

{% highlight haskell %}
#ifdef CONFIG_SMP_ON_UP
        __HEAD
__fixup_smp:
        and     r3, r9, #0x000f0000     @ architecture version
        teq     r3, #0x000f0000         @ CPU ID supported?
        bne     __fixup_smp_on_up       @ no, assume UP

....
{% endhighlight %}

代码首先检查 r9 是否支持，这里先看一下 ARMv7 的 MIDR 寄存器，其布局如下：

![MMU](https://raw.githubusercontent.com/EmulateSpace/PictureSet/master/BiscuitOS/boot/BOOT000025.png)

其中 MDIR[19:16] 对应的是 Architecture 域，其域值定义如下：

![MMU](https://raw.githubusercontent.com/EmulateSpace/PictureSet/master/BiscuitOS/boot/BOOT000040.png)

此时在使用的位置加上断点，使用 GDB 调试这段代码，调试情况如下：

{% highlight haskell %}
.text_addr = 0x60100000
.head.text_addr = 0x60008000
.rodata_addr = 0x60800000
(gdb) b BS_debug
Breakpoint 1 at 0x60008144: file arch/arm/kernel/head.S, line 508.
(gdb) c
Continuing.

Breakpoint 1, __fixup_smp () at arch/arm/kernel/head.S:508
508		and	r3, r9, #0x000f0000	@ architecture version
(gdb) info reg r9
r9             0x410fc090          1091551376
(gdb) n
509		teq	r3, #0x000f0000		@ CPU ID supported?
(gdb) info reg r3 r9
r3             0xf0000             983040
r9             0x410fc090          1091551376
(gdb) n
510		bne	__fixup_smp_on_up	@ no, assume UP
(gdb) info reg cpsr
cpsr           0x400001d3          1073742291
(gdb) n
512		bic	r3, r9, #0x00ff0000
(gdb)
{% endhighlight %}

从实践可以看出，MIDR 的 Architecture 域由 CPUID 决定，因此继续执行如下代码：

{% highlight haskell %}
bic     r3, r9, #0x00ff0000
bic     r3, r3, #0x0000000f     @ mask 0xff00fff0
mov     r4, #0x41000000
orr     r4, r4, #0x0000b000
orr     r4, r4, #0x00000020     @ val 0x4100b020
teq     r3, r4                  @ ARM 11MPCore?
reteq   lr                      @ yes, assume SMP
{% endhighlight %}

代码首先将 r9 寄存器的 16 到 23 bit 都清零了，然后再清零 0 到 3 bit, 以此构造一个
掩码 0xff00fff0, 接下来使用 orr 指令，构造 r4 寄存器的值为 0x4100b020。接着将
0xff00fff0 与 0x4100b020 进行对比，以此确定 CPU 是不是 ARM11MP。如果是，那么就
直接返回。开发者可以在适当的位置加上断点，然后使用 GDB 进行调试，调试的情况如下：

{% highlight haskell %}
5.0-arm32/linux/linux/vmlinux" at
	.text_addr = 0x60100000
	.head.text_addr = 0x60008000
	.rodata_addr = 0x60800000
(gdb) b BS_debug
Breakpoint 1 at 0x60008150: file arch/arm/kernel/head.S, line 512.
(gdb) c
Continuing.

Breakpoint 1, __fixup_smp () at arch/arm/kernel/head.S:512
512		bic	r3, r9, #0x00ff0000
(gdb) n
513		bic	r3, r3, #0x0000000f	@ mask 0xff00fff0
(gdb) n
514		mov	r4, #0x41000000
(gdb) n
515		orr	r4, r4, #0x0000b000
(gdb) n
516		orr	r4, r4, #0x00000020	@ val 0x4100b020
(gdb) n
517		teq	r3, r4			@ ARM 11MPCore?
(gdb) info reg r3 r4
r3             0x4100c090          1090568336
r4             0x4100b020          1090564128
(gdb) n
518		reteq	lr			@ yes, assume SMP
(gdb) n
520		mrc	p15, 0, r0, c0, c0, 5	@ read MPIDR
(gdb)
{% endhighlight %}

从上面的调试结果看出，该 CPU 不是 ARM11MP。那么接下来执行的代码如下：

{% highlight haskell %}
mrc     p15, 0, r0, c0, c0, 5   @ read MPIDR
and     r0, r0, #0xc0000000     @ multiprocessing extensions and
teq     r0, #0x80000000         @ not part of a uniprocessor system?
bne    __fixup_smp_on_up        @ no, assume UP
{% endhighlight %}

首先调用 mrc 指令读取 MPIDR 寄存器的值，存储到 r0 寄存器中，然后将 r0 的值与 0xc0000000
相与，结果存储在 r0 寄存器中，这里 MPIDR 寄存器的布局如下：

![MMU](https://raw.githubusercontent.com/EmulateSpace/PictureSet/master/BiscuitOS/boot/BOOT000041.png)

其中 ARMv7 中不支持 Multiprocessing Extensions,因此 MPIDR[31:28] 对应的域 Reserved。
因此这里 bne 指令不会跳转。开发者在适当的位置添加断点，然后使用 GDB 进行调试，调试情况
如下：

{% highlight haskell %}
5.0-arm32/linux/linux/vmlinux" at
	.text_addr = 0x60100000
	.head.text_addr = 0x60008000
	.rodata_addr = 0x60800000
(gdb) b BS_debug
Breakpoint 1 at 0x6000816c: file arch/arm/kernel/head.S, line 520.
(gdb) c
Continuing.

Breakpoint 1, __fixup_smp () at arch/arm/kernel/head.S:520
520		mrc	p15, 0, r0, c0, c0, 5	@ read MPIDR
(gdb) n
521		and	r0, r0, #0xc0000000	@ multiprocessing extensions and
(gdb) info reg r0
r0             0x80000000          -2147483648
(gdb) n
522		teq	r0, #0x80000000		@ not part of a uniprocessor system?
(gdb) info reg r0
r0             0x80000000          -2147483648
(gdb) n
523		bne    __fixup_smp_on_up	@ no, assume UP
(gdb) info reg cpsr
cpsr           0x600001d3          1610613203
(gdb)
{% endhighlight %}

上面的实践结果符合预期，接下来继续执行代码：

{% highlight haskell %}
@ Core indicates it is SMP. Check for Aegis SOC where a single
@ Cortex-A9 CPU is present but SMP operations fault.
mov     r4, #0x41000000
orr     r4, r4, #0x0000c000
orr     r4, r4, #0x00000090
teq     r3, r4                  @ Check for ARM Cortex-A9
retne   lr                      @ Not ARM Cortex-A9,
{% endhighlight %}

这段代码主要检查 Core 是否是 SMP，检查 Aegis SOC 是否是一个单片的 Cortex-A9 但
SMP 操作是失败的。首先使用 orr 指令是 r4 寄存器的值为 0x4100c090,然后调用 teq 指令，
如果对比结果不为零，那么直接退出，代表 CPU 不是 ARM Cortex-A9。开发者在适当的位置
添加断点，然后使用 GDB 进行调试，调试结果如下：

{% highlight haskell %}
5.0-arm32/linux/linux/vmlinux" at
	.text_addr = 0x60100000
	.head.text_addr = 0x60008000
	.rodata_addr = 0x60800000
(gdb) b BS_debug
Breakpoint 1 at 0x6000817c: file arch/arm/kernel/head.S, line 527.
(gdb) c
Continuing.

Breakpoint 1, __fixup_smp () at arch/arm/kernel/head.S:527
527		mov	r4, #0x41000000
(gdb) n
528		orr	r4, r4, #0x0000c000
(gdb) n
529		orr	r4, r4, #0x00000090
(gdb) n
530		teq	r3, r4			@ Check for ARM Cortex-A9
(gdb) n
531		retne	lr			@ Not ARM Cortex-A9,
(gdb) info reg r3 r4
r3             0x4100c090          1090568336
r4             0x4100c090          1090568336
(gdb) n
536		mrc	p15, 4, r0, c15, c0	@ get SCU base address
(gdb)
{% endhighlight %}

通过实践可以知道 CPU 是 Cortex-A9, 因此继续执行代码：

{% highlight haskell %}
@ If a future SoC *does* use 0x0 as the PERIPH_BASE, then the
@ below address check will need to be #ifdef'd or equivalent
@ for the Aegis platform.
mrc     p15, 4, r0, c15, c0     @ get SCU base address
teq     r0, #0x0                @ '0' on actual UP A9 hardware
beq     __fixup_smp_on_up       @ So its an A9 UP
ldr     r0, [r0, #4]            @ read SCU Config
ARM_BE8(rev     r0, r0)                 @ byteswap if big endian
and     r0, r0, #0x3            @ number of CPUs
teq     r0, #0x0                @ is 1?
retne   lr
{% endhighlight %}

接着调用 mrc 指令读取了 p15 c15 寄存器，VMSA CP15 c15 寄存器是
IMPLEMENTATION DEFINED 寄存器。ARMv7 预留 CP15 c15 为 IMPLEMENTATION DEFINED
目的用，请具体描述可以查看手册，这里不做过多解释：

![MMU](https://raw.githubusercontent.com/EmulateSpace/PictureSet/master/BiscuitOS/boot/BOOT000042.png)

根据 Cortex-A9 手册可以知道 c15 系统控制寄存器布局如下：

![MMU](https://raw.githubusercontent.com/EmulateSpace/PictureSet/master/BiscuitOS/boot/BOOT000043.png)

通过 "mrc p15, 4, r0, c15, c0" 选中了 Configuration Base Address, 其布局如下：

![MMU](https://raw.githubusercontent.com/EmulateSpace/PictureSet/master/BiscuitOS/boot/BOOT000044.png)

回到代码，如果 Configuration Base Address 的值为 0，那么这个 CPU 是一个 A9 UP.
如果 r0 寄存器的值不为 0，那么通过 ldr 指令读取 SCU 的配置。更多 SCU 信息请查看
Cortex-A9MP 手册。SCU Configuration 寄存器的布局如下：

![MMU](https://raw.githubusercontent.com/EmulateSpace/PictureSet/master/BiscuitOS/boot/BOOT000045.png)

接着执行代码 "and r0, r0, #0x3", 获得 SCU Configuration 寄存器的 LSB 低 2bits，
低 2 位用于指定 CPU 的数量。如果此时 r0 寄存器的值为 0，那么代表系统只有一个 Cortex-A9
CPU，那么 retne 指令不执行。开发者在适当的位置添加断点，然后使用 GDB 进行调试，调试
情况如下：

{% highlight haskell %}
5.0-arm32/linux/linux/vmlinux" at
	.text_addr = 0x60100000
	.head.text_addr = 0x60008000
	.rodata_addr = 0x60800000
(gdb) b BS_debug
Breakpoint 1 at 0x60008190: file arch/arm/kernel/head.S, line 536.
(gdb) c
Continuing.

Breakpoint 1, __fixup_smp () at arch/arm/kernel/head.S:536
536		mrc	p15, 4, r0, c15, c0	@ get SCU base address
(gdb) n
537		teq	r0, #0x0		@ '0' on actual UP A9 hardware
(gdb) info reg r0
r0             0x1e000000          503316480
(gdb) n
538		beq	__fixup_smp_on_up	@ So its an A9 UP
(gdb) n
539		ldr	r0, [r0, #4]		@ read SCU Config
(gdb) n
541		and	r0, r0, #0x3		@ number of CPUs
(gdb) info reg r0
r0             0x10                16
(gdb) n
542		teq	r0, #0x0		@ is 1?
(gdb) info reg r0
r0             0x0                 0
(gdb) n
543		retne	lr
(gdb) n
546		adr	r0, 1f
(gdb)
{% endhighlight %}

通过上面的实践可以知道系统只包含一个 CPU，因此继续执行如下代码：

{% highlight haskell %}
__fixup_smp_on_up:
        adr     r0, 1f
        ldmia   r0, {r3 - r5}
        sub     r3, r0, r3
        add     r4, r4, r3
        add     r5, r5, r3
        b       __do_fixup_smp_on_up
ENDPROC(__fixup_smp)

        .align
1:      .word   .
        .word   __smpalt_begin
        .word   __smpalt_end

        .pushsection .data
        .align  2
        .globl  smp_on_up
smp_on_up:
        ALT_SMP(.long   1)
        ALT_UP(.long    0)
        .popsection
{% endhighlight %}

代码首先获得 1f 处的地址，然后使用 ldmia 指令读取 1 的虚拟地址到 r3 寄存器；
读取 __smpalt_begin 虚拟地址到 r4 寄存器；读取 __smpalt_end 的虚拟地址到
r5 寄存器。然后使用 "sub r3, r0, r3" 调整物理地址与虚拟地址之间的偏移，然后
连续调用 add 指令调整 r4,r5 寄存器，以此获得虚拟地址对应的物理地址。在调用
__do_fixup_smp_on_up 之前，这里先分析一下 __smpalt_begin 和 __smpalt_end。
其定义在 arch/arm/kernel/vmlinux.lds.S,如下：

{% highlight haskell %}
#ifdef CONFIG_SMP_ON_UP
        .init.smpalt : {
                __smpalt_begin = .;
                *(.alt.smp.init)
                __smpalt_end = .;
        }
#endif
{% endhighlight %}

从中可以看出，__smpalt_begin 指向了输出文件 vmlinux .init.smpalt section 的
起始地址，而 __smpalt_end 指向该 section 的终止地址。.init.smpalt section
是由输入文件的 .alt.smp.init section 构成的。.alt.smp.init section 存在于
arch/arm/include/asm/processor.h 和 arch/arm/include/asm/assembler.h 文件中，
如下：

{% highlight haskell %}
arch/arm/include/asm/processor.h

#ifdef CONFIG_SMP
#define __ALT_SMP_ASM(smp, up)                                          \
        "9998:  " smp "\n"                                              \
        "       .pushsection \".alt.smp.init\", \"a\"\n"                \
        "       .long   9998b\n"                                        \
        "       " up "\n"                                               \
        "       .popsection\n"
#else
#define __ALT_SMP_ASM(smp, up)  up
#endif

arch/arm/include/asm/assembler.h

#ifdef CONFIG_SMP
#define ALT_SMP(instr...)                                       \
9998:   instr
/*
 * Note: if you get assembler errors from ALT_UP() when building with
 * CONFIG_THUMB2_KERNEL, you almost certainly need to use
 * ALT_SMP( W(instr) ... )
 */
#define ALT_UP(instr...)                                        \
        .pushsection ".alt.smp.init", "a"                       ;\
        .long   9998b                                           ;\
9997:   instr                                                   ;\
        .if . - 9997b == 2                                      ;\
                nop                                             ;\
        .endif                                                  ;\
        .if . - 9997b != 4                                      ;\
                .error "ALT_UP() content must assemble to exactly 4 bytes";\
        .endif                                                  ;\
        .popsection
#define ALT_UP_B(label)                                 \
        .equ    up_b_offset, label - 9998b                      ;\
        .pushsection ".alt.smp.init", "a"                       ;\
        .long   9998b                                           ;\
        W(b)    . + up_b_offset                                 ;\
        .popsection
#else
#define ALT_SMP(instr...)
#define ALT_UP(instr...) instr
#define ALT_UP_B(label) b label
#endif
{% endhighlight %}

因此通过上面三个宏，都可以将

[link](https://blog.csdn.net/shangyaowei/article/details/17425901)


开发者可以在 arch/arm/ 目录下查找
含有 .alt.smp.init section 的源码。开发者可以在之前的代码处添加适当的断点，然后
使用 GDB 进行调试，调试的情况如下：

{% highlight haskell %}
5.0-arm32/linux/linux/vmlinux" at
	.text_addr = 0x60100000
	.head.text_addr = 0x60008000
	.rodata_addr = 0x60800000
(gdb) b BS_debug
Breakpoint 1 at 0x600081ac: file arch/arm/kernel/head.S, line 546.
(gdb) c
Continuing.

Breakpoint 1, __fixup_smp () at arch/arm/kernel/head.S:546
546		adr	r0, 1f
(gdb) n
547		ldmia	r0, {r3 - r5}
(gdb) n
548		sub	r3, r0, r3
(gdb) n
549		add	r4, r4, r3
(gdb) n
550		add	r5, r5, r3
(gdb) n
551		b	__do_fixup_smp_on_up
(gdb) info reg r3 r4 r5
r3             0xe0000000          -536870912
r4             0x60a471ec          1621389804
r5             0x60a5357c          1621439868
(gdb)
{% endhighlight %}

在准备好条件之后，函数执行 bl 指令跳转到 __do_fixup_smp_on_up 处执行，代码如下：
<span id="__do_fixup_smp_on_up"></span>

{% highlight haskell %}
5.0-arm32/linux/linux/vmlinux" at
	.text_addr = 0x60100000
	.head.text_addr = 0x60008000
	.rodata_addr = 0x60800000
(gdb) b BS_debug
Breakpoint 1 at 0x600081ac: file arch/arm/kernel/head.S, line 546.
(gdb) c
Continuing.

Breakpoint 1, __fixup_smp () at arch/arm/kernel/head.S:546
546		adr	r0, 1f
(gdb) n
547		ldmia	r0, {r3 - r5}
(gdb) n
548		sub	r3, r0, r3
(gdb) n
549		add	r4, r4, r3
(gdb) n
550		add	r5, r5, r3
(gdb) n
551		b	__do_fixup_smp_on_up
(gdb) info reg r3 r4 r5
r3             0xe0000000          -536870912
r4             0x60a471ec          1621389804
r5             0x60a5357c          1621439868
(gdb)
{% endhighlight %}

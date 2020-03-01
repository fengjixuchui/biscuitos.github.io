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

![MMU](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND00000A.jpg)

# 简介


--------------------------------------------------------------
<span id="实践"></span>

![MMU](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND00000A.jpg)

# 实践

> - [vmlinux 入口：第一行运行的代码](#vmlinux first code)
>
> - [\_\_hyp_stub_install](#__hyp_stub_install)
>
> - [\_\_lookup_processor_type](#__lookup_processor_type)
>
> - [\_\_proc_info](#__proc_info)
>
> - [\_\_vet_atags](#__vet_atags)
>
> - [\_\_fixup_smp](#__fixup_smp)
>
> - [\_\_do_fixup_smp_on_up](#__do_fixup_smp_on_up)
>
> - [\_\_fixup_pv_table](#__fixup_pv_table)
>
> - [\_\_create_page_tables](#__create_page_tables)
>
> - [swapper_pg_dir](#swapper_pg_dir)
>
> - [ARMv7 Cortex-A9 proc_info_list](#ARMv7 Cortex-A9 proc_info_list)
>
> - [\_\_v7_setup](#__v7_setup)
>
> - [v7_invalidate_l1](#v7_invalidate_l1)
>
> - [\_\_v7_setup_cont](#__v7_setup_cont)
>
> - [v7_ttb_setup](#v7_ttb_setup)
>
> - [\_\_enable_mmu](#__enable_mmu)
>
> - [\_\_mmap_switched](#__mmap_switched)
>
> - [start_kernel_last](#start_kernel_last)

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

![MMU](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/boot/BOOTCP15C0.png)

此时选中了 MIDR (Main ID Register) 寄存器，此时 MIDR 的布局如下：

![MMU](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/boot/BOOT000025.png)

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

![MMU](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/boot/BOOT000025.png)

其中 MDIR[19:16] 对应的是 Architecture 域，其域值定义如下：

![MMU](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/boot/BOOT000040.png)

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

![MMU](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/boot/BOOT000041.png)

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

![MMU](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/boot/BOOT000042.png)

根据 Cortex-A9 手册可以知道 c15 系统控制寄存器布局如下：

![MMU](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/boot/BOOT000043.png)

通过 "mrc p15, 4, r0, c15, c0" 选中了 Configuration Base Address, 其布局如下：

![MMU](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/boot/BOOT000044.png)

回到代码，如果 Configuration Base Address 的值为 0，那么这个 CPU 是一个 A9 UP.
如果 r0 寄存器的值不为 0，那么通过 ldr 指令读取 SCU 的配置。更多 SCU 信息请查看
Cortex-A9MP 手册。SCU Configuration 寄存器的布局如下：

![MMU](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/boot/BOOT000045.png)

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

首先查看 __ALT_SMP_ASM 宏，这个宏的作用就是将当前 section 指针指向 .alt.smp.init,
并在进入 .slt.smp.init section 之前就在当前 section 做好了标号 9998 指向参数 smp。
对于 ALT_UP 宏，则将指令都放到了 .alt.smp.init 中，总之上面的宏定义都使用了
.pushsection 和 .popsection 两个伪指令，用意就是让当前 section 指向 .alt.smp.init
section. 更多细节文档请参考：

> [内核动态补丁（kpatch）及 kpatch pushsection popsection previous的解释](https://www.veryarm.com/114497.html)
>
> [Linux Kernel SMP & nbsp...](https://blog.csdn.net/shangyaowei/article/details/17425901)

开发者可以在 arch/arm/ 目录下查找含有 .alt.smp.init section 的源码。开发者可以在
之前的代码处添加适当的断点，然后使用 GDB 进行调试，调试的情况如下：

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
__do_fixup_smp_on_up:
        cmp     r4, r5
        reths   lr
        ldmia   r4!, {r0, r6}
 ARM(   str     r6, [r0, r3]    )
 THUMB( add     r0, r0, r3      )
#ifdef __ARMEB__
 THUMB( mov     r6, r6, ror #16 )       @ Convert word order for big-endian.
#endif
 THUMB( strh    r6, [r0], #2    )       @ For Thumb-2, store as two halfwords
 THUMB( mov     r6, r6, lsr #16 )       @ to be robust against misaligned r3.
 THUMB( strh    r6, [r0]        )
        b       __do_fixup_smp_on_up
ENDPROC(__do_fixup_smp_on_up)
{% endhighlight %}

此时 r4 寄存器存储 __smpalt_begin, 即 .alt.smp.init section 的起始地址，而 r5
寄存器存储着 __smpalt_end，即 .alt.smp.init section 的结束地址。然后调用 cmp
指令检查 r4 是否已经等于 r5 寄存器，以此检查循环是否结束。如果不相等，那么循环继续，
继续调用 ldmia 指令将 r4 对应地址的内容拷贝到 r0 和 r6 寄存器，由于本次实践不支持
THUMB，那么接下来执行 "str r6, [r0, r3]", 将 r6 寄存器的值写入到 "r0+r3" 对应的
内存地址上。这样起到了校正 .alt.smp.init section 的作用。开发者可以在适当的位置
添加断点，然后使用 GDB 进行调试，调试情况如下：

{% highlight haskell %}
5.0-arm32/linux/linux/vmlinux" at
	.text_addr = 0x60100000
	.head.text_addr = 0x60008000
	.rodata_addr = 0x60800000
(gdb) b BS_debug
Breakpoint 1 at 0x60102560: file arch/arm/kernel/head.S, line 570.
(gdb) c
Continuing.

Breakpoint 1, __do_fixup_smp_on_up () at arch/arm/kernel/head.S:570
570		cmp	r4, r5
(gdb) info reg r4 r5
r4             0x60a471ec          1621389804
r5             0x60a5357c          1621439868
(gdb) n
571		reths	lr
(gdb) n
572		ldmia	r4!, {r0, r6}
(gdb) n
573	 ARM(	str	r6, [r0, r3]	)
(gdb) info reg r0 r3 r6
r0             0x80b0b5c0          -2135902784
r3             0xe0000000          -536870912
r6             0x0                 0
(gdb) x/1x 0x60b0b5c0
0x60b0b5c0:	0x00000001
(gdb) info reg r6
r6             0x0                 0
(gdb) n
581		b	__do_fixup_smp_on_up
(gdb) info reg r6
r6             0x0                 0
(gdb) x/1x 0x60b0b5c0
0x60b0b5c0:	0x00000000
(gdb) n

Breakpoint 1, __do_fixup_smp_on_up () at arch/arm/kernel/head.S:570
570		cmp	r4, r5
(gdb)
{% endhighlight %}

实践的结果和预期的一致，r6 寄存器的值已经更新到 "r0+r3" 对应的内存地址上了。经过
多次循环后，函数执行返回。接下来执行的代码是 arch/arm/kernel/head.S

<span id="__fixup_pv_table"></span>
{% highlight haskell %}
/* __fixup_pv_table - patch the stub instructions with the delta between
 * PHYS_OFFSET and PAGE_OFFSET, which is assumed to be 16MiB aligned and
 * can be expressed by an immediate shifter operand. The stub instruction
 * has a form of '(add|sub) rd, rn, #imm'.
 */
        __HEAD
__fixup_pv_table:
        adr     r0, 1f
        ldmia   r0, {r3-r7}
        mvn     ip, #0
        subs    r3, r0, r3      @ PHYS_OFFSET - PAGE_OFFSET
        add     r4, r4, r3      @ adjust table start address
        add     r5, r5, r3      @ adjust table end address
        add     r6, r6, r3      @ adjust __pv_phys_pfn_offset address
        add     r7, r7, r3      @ adjust __pv_offset address
        mov     r0, r8, lsr #PAGE_SHIFT @ convert to PFN
        str     r0, [r6]        @ save computed PHYS_OFFSET to __pv_phys_pfn_offset
        strcc   ip, [r7, #HIGH_OFFSET]  @ save to __pv_offset high bits
        mov     r6, r3, lsr #24 @ constant for add/sub instructions
        teq     r3, r6, lsl #24 @ must be 16MiB aligned
THUMB(  it      ne              @ cross section branch )
        bne     __error
        str     r3, [r7, #LOW_OFFSET]   @ save to __pv_offset low bits
        b       __fixup_a_pv_table
ENDPROC(__fixup_pv_table)

        .align
1:      .long   .
        .long   __pv_table_begin
        .long   __pv_table_end
2:      .long   __pv_phys_pfn_offset
        .long   __pv_offset
{% endhighlight %}

__fixup_pv_table 的主要任务也就是调整 PV table 的地址。函数首先通过 adr 指令获得
1 处的物理地址，然后通过 ldmia 指令，将 1 处的虚拟地址存储在 r3 寄存器；将
__pv_table_begin 的虚拟地址存储在 r4 寄存器；将 __pv_table_end 的虚拟地址存储在
r5 寄存器；将 __pv_phys_pfn_offset 的虚拟地址存储在 r6 寄存器；将 __pv_offset
的虚拟地址存储在 r7 寄存器。接着使用 "mvn" 指令按位取反 0，即将 0xffffffff 的值存储
到 IP 寄存器，然后通过 "subs r3, r0, r3" 代码计算物理地址与虚拟地址之间的偏移，
然后通过 add 指令，分别获得 __pv_table_begin, __pv_table_end, __pv_phys_pfn_offset,
以及 __pv_offset 的物理地址。此时 r8 寄存器存储着修正后的 PHYS_OFFSET, 这里调用
"mov r0, r8, lsr #PAGE_OFFSET" 命令获得内核镜像起始地址对应的页帧号。然后将修正后的
值存储到 __pv_phys_pfn_offset 里，接着将 0xffffffff 存储到 __pv_offset+4 处。
将 r3 寄存器的值右移 24 位并存储到 r6 寄存器，然后将 r6 寄存器载左移 24 位。以此
达到 16M 对齐，通过 teq 检查 r3 寄存器的值是否按 16M 对齐，如果没有，那么就直接跳转
到 __error；如果按 r3 按 16M 对齐，那么将使用 str 指令，将 r3 的值存储到
__pv_offset 地址处。最后跳转到 __fixup_a_pv_table. 开发者可以在适当的位置添加
断点，然后使用 GDB 进行调试，调试的情况如下：

{% highlight haskell %}
5.0-arm32/linux/linux/vmlinux" at
	.text_addr = 0x60100000
	.head.text_addr = 0x60008000
	.rodata_addr = 0x60800000
(gdb) b BS_debug
Breakpoint 1 at 0x600081d0: file arch/arm/kernel/head.S, line 610.
(gdb) c
Continuing.

Breakpoint 1, __fixup_pv_table () at arch/arm/kernel/head.S:610
610		adr	r0, 1f
(gdb) n
611		ldmia	r0, {r3-r7}
(gdb) n
612		mvn	ip, #0
(gdb) info reg r3 r4 r5 r6 r7
r3             0x80008210          -2147450352
r4             0x80a5357c          -2136656516
r5             0x80a53d34          -2136654540
r6             0x80b0b5c4          -2135902780
r7             0x80b0b5c8          -2135902776
(gdb) n
613		subs	r3, r0, r3	@ PHYS_OFFSET - PAGE_OFFSET
(gdb) n
614		add	r4, r4, r3	@ adjust table start address
(gdb) n
615		add	r5, r5, r3	@ adjust table end address
(gdb) n
616		add	r6, r6, r3	@ adjust __pv_phys_pfn_offset address
(gdb) n
617		add	r7, r7, r3	@ adjust __pv_offset address
(gdb) n
618		mov	r0, r8, lsr #PAGE_SHIFT	@ convert to PFN
(gdb) info reg r3 r4 r5 r6 r7
r3             0xe0000000          -536870912
r4             0x60a5357c          1621439868
r5             0x60a53d34          1621441844
r6             0x60b0b5c4          1622193604
r7             0x60b0b5c8          1622193608
(gdb) n
619		str	r0, [r6]	@ save computed PHYS_OFFSET to __pv_phys_pfn_offset
(gdb) info reg r0
r0             0x60000             393216
(gdb) x/x 0x60b0b5c4
0x60b0b5c4:	0x00000000
(gdb) n
620		strcc	ip, [r7, #HIGH_OFFSET]	@ save to __pv_offset high bits
(gdb) x/x 0x60b0b5c4
0x60b0b5c4:	0x00060000
(gdb) x/x 0x60b0b5cc
0x60b0b5cc:	0x00000000
(gdb) n
621		mov	r6, r3, lsr #24	@ constant for add/sub instructions
(gdb) x/x 0x60b0b5cc
0x60b0b5cc:	0xffffffff
(gdb) n
622		teq	r3, r6, lsl #24 @ must be 16MiB aligned
(gdb) info reg r3 r6
r3             0xe0000000          -536870912
r6             0xe0                224
(gdb) n
624		bne	__error
(gdb) n
625		str	r3, [r7, #LOW_OFFSET]	@ save to __pv_offset low bits
(gdb) x/x 0x60b0b5c8
0x60b0b5c8:	0x00000000
(gdb) n
626		b	__fixup_a_pv_table
(gdb) x/x 0x60b0b5c8
0x60b0b5c8:	0xe0000000
(gdb)
{% endhighlight %}

从调试结果来看，和预期分析的一致。这里继续讲解 __pv 相关的表。首先 __pv_table_begin
和 __pv_table_end, 其定义在 arch/arm/kernel/vmlinux.lds.S 里，如下：

{% highlight haskell %}
        .init.pv_table : {      
                __pv_table_begin = .;
                *(.pv_table)    
                __pv_table_end = .;
        }
{% endhighlight %}

因此 __pv_table_begin 指向输出文件 vmlinux 的 .init.pv_table section 的起始地址，
__pv_table_end 指向 .init.pv_table section 的结束地址。__pv_offset 和
__pv_phys_pfn_offset 定义在 arch/arm/kernel/head.S 里面，如下：

{% highlight haskell %}
        .data
        .align  2
        .globl  __pv_phys_pfn_offset
        .type   __pv_phys_pfn_offset, %object
__pv_phys_pfn_offset:
        .word   0
        .size   __pv_phys_pfn_offset, . -__pv_phys_pfn_offset

        .globl  __pv_offset
        .type   __pv_offset, %object
__pv_offset:    
        .quad   0
        .size   __pv_offset, . -__pv_offset
{% endhighlight %}

这些定义的具体含义以后再做介绍，这里继续分析接下来执行的代码，如下：

{% highlight haskell %}
        .text
__fixup_a_pv_table:
        adr     r0, 3f
        ldr     r6, [r0]
        add     r6, r6, r3
        ldr     r0, [r6, #HIGH_OFFSET]  @ pv_offset high word
        ldr     r6, [r6, #LOW_OFFSET]   @ pv_offset low word
        mov     r6, r6, lsr #24
        cmn     r0, #1
        moveq   r0, #0x400000   @ set bit 22, mov to mvn instruction
        b       2f
1:      ldr     ip, [r7, r3]
        bic     ip, ip, #0x000000ff
        tst     ip, #0xf00      @ check the rotation field
        orrne   ip, ip, r6      @ mask in offset bits 31-24
        biceq   ip, ip, #0x400000       @ clear bit 22
        orreq   ip, ip, r0      @ mask in offset bits 7-0
        str     ip, [r7, r3]
2:      cmp     r4, r5
        ldrcc   r7, [r4], #4    @ use branch for delay slot
        bcc     1b
        ret     lr
ENDPROC(__fixup_a_pv_table)

        .align
3:      .long __pv_offset
{% endhighlight %}

首先调用 adr 获得 3f 对应的物理地址存储到 r0 寄存器中，然后将 __pv_offset 的地址
加载到 r6 寄存器，接着再通过 r3 寄存器调整为 __pv_offset 对应的物理地址。
此时内核将 r6 寄存器高 4 字节的内容存储到 r0 寄存器，再将 r6 寄存器低 4
字节的内容存储到 r6 寄存器，并将 r6 寄存器的值向右移动 24 bit，内核继续调用
cmn 和 moveq 两条指令设置 r0 寄存器的值。接下来就是比较重点的内容，内核
首先获得 __pv_table_begin 的地址，然后使用循环遍历表中每一项，表中的每一项
就是一个 long 变量，其指向一个段指令所在的地址，内核通过 "ldr ip, [r7, r3]"
指令获得一条指令所在的地址，这里指的注意的是。__pv_table 里面的内容基本
都是 "ADD/SUB" 指令，因此内核通过修改这两条指令的立即数域，以此修改虚拟地址
和物理地址转换的偏移，如下：

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/boot/BOOT000232.png)

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/boot/BOOT000233.png)

两个指令的机器码格式中，shifter_operand 域都是一致，其定义如下：

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/boot/BOOT000234.png)

上面为 ARMv7 中立即数的表示方式，其包含了 rotate_imm 域和 immed_8 域，
ARMv7 通过将 immed_8 域的值向右循环移动 (2 * rotate_imm 域值)，即：

{% highlight c %}
立即数 = immed_8 >> (2 * rotate_imm)  "循环右移"                      
{% endhighlight %}

因此 __pv_stub 宏将指令添加到 .init.pv_table section 之后，可以通过
__pv_table_begin 遍历里面的所有指令，并可以修改每条 ADD 或 SUB
指令的指定域，其中包括立即数域。内核首先调用 "bic ip, ip, #0xff"
指令清除 ADD/SUB 指令的 immed_8 域，然后调用 "tst ip, #0xf00"
指令检查 ADD/SUB 指令的 rotate_imm 域是否有效，如果有效，那么
将 r6 寄存器的内容，即物理地址和虚拟地址之间的偏移存储到指令的
立即数域。这样就完成了内核对 .init.pv_table section 内容的修改。

<span id="__create_page_tables"></span>
{% highlight haskell %}
/*
 * Setup the initial page tables.  We only setup the barest
 * amount which are required to get the kernel running, which
 * generally means mapping in the kernel code.
 *
 * r8 = phys_offset, r9 = cpuid, r10 = procinfo
 *
 * Returns:
 *  r0, r3, r5-r7 corrupted
 *  r4 = physical page table address
 */
__create_page_tables:
        pgtbl   r4, r8                          @ page table address
{% endhighlight %}

__create_page_table 的主要作用就是初始化内核页表，但这个页表是空的，要等待内核
对页表的内容进行初始化。在指向 __create_page_table 之前，r8 指向 PHYS_OFFSET，
r9 存储着 CPUID 信息，r10 存储着 proc_info 信息。函数首先调用 pgtbl 宏，定义
如下：

{% highlight haskell %}
        .macro  pgtbl, rd, phys
        add     \rd, \phys, #TEXT_OFFSET
        sub     \rd, \rd, #PG_DIR_SIZE
        .endm
{% endhighlight %}

pgtal 宏接收两个参数 rd 和 phys。该宏首先将 phys 参数加上 TEXT_OFFSET 获得内核
镜像的物理地址，并存储在 rd 里面。 TEXT_OFFSET 代表内核镜像在 RAM 上的偏移。接着
将内核镜像的物理地址减去 PG_DIR_SIZE, 以该地址作为内核页表的起始地址，并存储
在 rd 中。如之前的代码 "pgtbl r4, r8", 此时 r8 存储 PHYS_OFFSET, 也就是 RAM
的起始地址，执行完代码之后，r4 指向内核页表的起始物理地址。开发者可以在适当
的位置添加断点，然后使用 GDB 进行调试，调试情况如下：

{% highlight haskell %}
5.0-arm32/linux/linux/vmlinux" at
	.text_addr = 0x60100000
	.head.text_addr = 0x60008000
	.rodata_addr = 0x60800000
(gdb) b BS_debug
Breakpoint 1 at 0x6000808c: file arch/arm/kernel/head.S, line 182.
(gdb) c
Continuing.

Breakpoint 1, __create_page_tables () at arch/arm/kernel/head.S:182
182		pgtbl	r4, r8				@ page table address
(gdb) info reg r4 r8
r4             0x60a53d34          1621441844
r8             0x60000000          1610612736
(gdb) n
187		mov	r0, r4
(gdb) info reg r4 r8
r4             0x60004000          1610629120
r8             0x60000000          1610612736
(gdb)
{% endhighlight %}

从上面的实践可以知道，全局页目录的物理地址是 0x60004000。在分析代码之前，这里补充
一个重要的知识点 swapp_pg_dir 变量，其定义如下：

<span id="swapper_pg_dir"></span>
{% highlight haskell %}
/*
 * swapper_pg_dir is the virtual address of the initial page table.
 * We place the page tables 16K below KERNEL_RAM_VADDR.  Therefore, we must
 * make sure that KERNEL_RAM_VADDR is correctly set.  Currently, we expect
 * the least significant 16 bits to be 0x8000, but we could probably
 * relax this restriction to KERNEL_RAM_VADDR >= PAGE_OFFSET + 0x4000.
 */
#define KERNEL_RAM_VADDR        (PAGE_OFFSET + TEXT_OFFSET)
#if (KERNEL_RAM_VADDR & 0xffff) != 0x8000
#error KERNEL_RAM_VADDR must start at 0xXXXX8000
#endif

#ifdef CONFIG_ARM_LPAE
        /* LPAE requires an additional page for the PGD */
#define PG_DIR_SIZE     0x5000
#define PMD_ORDER       3
#else
#define PG_DIR_SIZE     0x4000
#define PMD_ORDER       2
#endif

        .globl  swapper_pg_dir
        .equ    wapper_pg_dir, KERNEL_RAM_VADDR - PG_DIR_SIZE
{% endhighlight %}

swapper_pg_dir 是一个指向全局页表的指针，swapper_pg_dir 包含的是一个虚拟地址。
内核将页表放在 KERNEL_RAW_VADDR 之前的 16K 虚拟地址上。KERNEL_RAW_VADDR 的定义
是 PAGE_OFFSET + TEXT_OFFSET, PAGE_OFFSET 代表 RAM 起始物理地址对应的虚拟地址，
TEXT_OFFSET 代表内核镜像在 RAM 中的偏移，因此 KERNEL_RAW_VADDR 代表内核镜像起始
物理地址对应的虚拟地址。因此这里定义了一个变量 wapper_pg_dir 的地址就是
KERNEL_RAW_VADDR - PG_DIR_SIZE, 这就是页表起始虚拟地址。因此在
arch/arm/include/asm/pgtable.h 里面定义了 swapper_pg_dir 引用声明：

{% highlight c %}
extern pgd_t swapper_pg_dir[PTRS_PER_PGD];
{% endhighlight %}

因此，当 MMU 启用之后，可以通过引用 swapper_pg_dir 来访问全局页表。分析完
swapper_pg_dir 之后，继续分析 __create_page_table，接下来执行的代码是：

{% highlight haskell %}
__create_page_tables:
        pgtbl   r4, r8                          @ page table address

        /*
         * Clear the swapper page table
         */
        mov     r0, r4
        mov     r3, #0
        add     r6, r0, #PG_DIR_SIZE
1:      str     r3, [r0], #4
        str     r3, [r0], #4
        str     r3, [r0], #4
        str     r3, [r0], #4
        teq     r0, r6
        bne     1b
{% endhighlight %}

在使用 pgtbl 获得全局页目录的物理地址之后，将其存储在 r4 寄存器中，总共有 16K
那么大。接下来执行的任务就是清除全局页目录里面的内容，为写入新内容做准备。首先执行的
两条 mov 指令将 r0 寄存器指向了全局页目录的首地址，并将 0 存储到 r3 寄存器里。
然后调用 add 指向，将 r0 寄存器的值加上 PG_DIR_SIZE，以此获得全局目录的结束地址，
并存储到 r6 中。然后连续使用 4 条 str 指令，将 r3 寄存器的值写入到 r0 开始处的 16 个
字节里，由于 r3 寄存器里存储着 0，那么这样操作就清除了 r0 开始处 16 个字节的内容。
接着调用 teq 指令，判断 r0 是否已经到达全局页目录的结束地址，如果没有，则跳转到 1b
处继续执行，知道 16K 内容全部清理。开发者可以在适当位置添加断点，然后使用 GDB 进行
调试，调试情况如下：

{% highlight haskell %}
5.0-arm32/linux/linux/vmlinux" at
	.text_addr = 0x60100000
	.head.text_addr = 0x60008000
	.rodata_addr = 0x60800000
(gdb) b BS_debug
Breakpoint 1 at 0x60008094: file arch/arm/kernel/head.S, line 187.
(gdb) c
Continuing.

Breakpoint 1, __create_page_tables () at arch/arm/kernel/head.S:187
187		mov	r0, r4
(gdb) n
188		mov	r3, #0
(gdb) n
189		add	r6, r0, #PG_DIR_SIZE
(gdb) n
190	1:	str	r3, [r0], #4
(gdb) info reg r3 r4 r6
r3             0x0                 0
r4             0x60004000          1610629120
r6             0x60008000          1610645504
(gdb) x/16x 0x60004000
0x60004000:	0x00000c12	0x00100c12	0x00200c12	0x00300c12
0x60004010:	0x00400c12	0x00500c12	0x00600c12	0x00700c12
0x60004020:	0x00800c12	0x00900c12	0x00a00c12	0x00b00c12
0x60004030:	0x00c00c12	0x00d00c12	0x00e00c12	0x00f00c12
(gdb) b BS_IO
Breakpoint 2 at 0x600080b8: file arch/arm/kernel/head.S, line 226.
(gdb) c
Continuing.

Breakpoint 2, __create_page_tables () at arch/arm/kernel/head.S:226
226		ldr	r7, [r10, #PROCINFO_MM_MMUFLAGS] @ mm_mmuflags
(gdb) x/16x 0x60004000
0x60004000:	0x00000000	0x00000000	0x00000000	0x00000000
0x60004010:	0x00000000	0x00000000	0x00000000	0x00000000
0x60004020:	0x00000000	0x00000000	0x00000000	0x00000000
0x60004030:	0x00000000	0x00000000	0x00000000	0x00000000
(gdb)
{% endhighlight %}

从上面的调试情况可以看出，代码已经将页表清零。接下来执行的代码如下：

{% highlight haskell %}
#define PROCINFO_MM_MMUFLAGS 8 /* offsetof(struct proc_info_list, __cpu_mm_mmu_flags) */
ldr     r7, [r10, #PROCINFO_MM_MMUFLAGS] @ mm_mmuflags
{% endhighlight %}

上面的代码主要任务就是获得 ARMv7 Cortex-A9 对应的 struct proc_info_list
结构的 __cpu_mm_mmu_flags 成员。接下来用于介绍如何获得 ARMv7 Cortex-A9 对应的
struct proc_info_list 中指定成员。

<span id="ARMv7 Cortex-A9 proc_info_list"></span>
每个 ARM CPU 对对应一个 struct proc_info_list 结构，该结构维护着 mmu，pgtable
等操作函数和标志，本文档讲解基于 ARMv7 Cortex-A9, 因此接下来的内容讲解如何
获得 ARMv7 Cortex-A9 对应的 struct proc_info_list。首先 struct proc_info_list
定义在 arch/arm/include/asm/procinfo.h 中，如下：

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

然后 ARMv7 Cortex-A9 的 struct proc_info_list 的内容定义在 arch/arm/mm/proc-v7.S
中，并且通过之前 __proc_info 的分析，可以知道 ARMv7 Cortex-A9 对应的 proc_info
是 __v7_ca9mp_proc_info, 具体实践过程请看：

> [__proc_info 源码分析实践](#__proc_info)

接着查看 __v7_ca9mp_proc_info 的定义，如下：

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

通过上面的内容可以看出 struct proc_info_list 对应的内容，这其中使用了 __v7_proc 宏
来计算 struct proc_info_list 相关的成员，这里重点分析 __v7_proc 宏的实现过程，
源码如下：

{% highlight haskell %}
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
{% endhighlight %}

通过上面的分析，直接构造 ARMv7 Cortex-A9 的 struct proc_info_list 内容如下：

{% highlight c %}
__v7_ca9mp_proc_info {
  .cpu_val            = 0x410fc090,
  .cpu_mask           = 0xff0ffff0,
  .__cpu_mm_mmu_flags = PMD_TYPE_SECT | PMD_SECT_AP_WRITE | PMD_SECT_AP_READ | \
                        PMD_SECT_AF | PMD_FLAGS_UP,
  .__cpu_io_mmu_flags = PMD_TYPE_SECT | PMD_SECT_AP_WRITE | \
                        PMD_SECT_AP_READ | PMD_SECT_AF,
  .__cpu_flush        = __v7_ca9mp_setup,
  .arch_name          = cpu_arch_name,
  .elf_name           = cpu_elf_name,
  .elf_hwcap          = HWCAP_SWP | HWCAP_HALF | HWCAP_THUMB | HWCAP_FAST_MULT | \
                        HWCAP_EDSP | HWCAP_TLS,
  .cpu_name           = cpu_v7_name,
  .proc               = ca9mp_processor_functions,
  .tlb                = v7wbi_tlb_fns,
  .user               = v6_user_fns,
  .cache              = v7_cache_fns,
}
{% endhighlight %}

分析完上面的代码，继续分析 __create_page_tables，代码如下：

{% highlight haskell %}
#define PROCINFO_MM_MMUFLAGS 8 /* offsetof(struct proc_info_list, __cpu_mm_mmu_flags) */
ldr     r7, [r10, #PROCINFO_MM_MMUFLAGS] @ mm_mmuflags
{% endhighlight %}

通过上面的代码，将从 __v7_ca9mp_proc_info 中读取 __cpu_mm_mmu_flags 的值，开发者
可以在适当位置添加断点，然后使用 GDB 调试，调试情况如下：

{% highlight haskell %}
5.0-arm32/linux/linux/vmlinux" at
	.text_addr = 0x60100000
	.head.text_addr = 0x60008000
	.rodata_addr = 0x60800000
b(gdb) b BS_debug
Breakpoint 1 at 0x600080b8: file arch/arm/kernel/head.S, line 225.
(gdb) c
Continuing.

Breakpoint 1, __create_page_tables () at arch/arm/kernel/head.S:225
225		ldr	r7, [r10, #PROCINFO_MM_MMUFLAGS] @ mm_mmuflags
(gdb) info reg r7 r10
r7             0x8070f078          -2140082056
r10            0x607288c4          1618118852
(gdb) n
231		adr	r0, __turn_mmu_on_loc
(gdb) info reg r7
r7             0xc0e               3086
(gdb)
{% endhighlight %}

从实践结果来看，ARMv7 Cortex-A9 的 proc_info_list 结构中，__cpu_mm_mmu_flags
的值为 0xc0e, 正好等于上面分析的 __v7_ca9mp_proc_info.__cpu_mm_mmu_flags
的值，其由多个宏构成，定义如下：

{% highlight haskell %}
#define PMD_TYPE_SECT           (_AT(pmdval_t, 2) << 0)
#define PMD_SECT_AP_WRITE       (_AT(pmdval_t, 1) << 10)
#define PMD_SECT_AP_READ        (_AT(pmdval_t, 1) << 11)
#define PMD_SECT_AF             (_AT(pmdval_t, 0))
#define PMD_FLAGS_UP		        PMD_SECT_WB
#define PMD_SECT_WB             (_AT(pmdval_t, 3) << 2) /* normal inner write-back */
{% endhighlight %}

接着继续分析 __create_page_tables 代码，如下：

{% highlight haskell %}
        /*
         * Create identity mapping to cater for __enable_mmu.
         * This identity mapping will be removed by paging_init().
         */
        adr     r0, __turn_mmu_on_loc
        ldmia   r0, {r3, r5, r6}
        sub     r0, r0, r3                      @ virt->phys offset
        add     r5, r5, r0                      @ phys __turn_mmu_on
        add     r6, r6, r0                      @ phys __turn_mmu_on_end
        mov     r5, r5, lsr #SECTION_SHIFT
        mov     r6, r6, lsr #SECTION_SHIFT

1:      orr     r3, r7, r5, lsl #SECTION_SHIFT  @ flags + kernel base
        str     r3, [r4, r5, lsl #PMD_ORDER]    @ identity mapping
        cmp     r5, r6
        addlo   r5, r5, #1                      @ next section
        blo     1b

        .ltorg
        .align
__turn_mmu_on_loc:
        .long   .
        .long   __turn_mmu_on
        .long   __turn_mmu_on_end
{% endhighlight %}

这段代码的主要任务就是给 __enable_mmu 函数建立对应的页表。
通过 adr 指令，获得了 __turn_mmu_on_loc 的地址，并调用 ldmia 指令将
__turn_mmu_on_loc 的虚拟地址存储在 r3 寄存器中；将 __turn_mmu_on 的虚拟地址存储
在 r5 寄存器中；最后将 __turn_mmu_on_end 的地址存储在 r6 寄存器。接着按照
常用方法，首先计算出物理偏移，然后校正 r5 和 r6 为 __turn_mmu_on 和
__turn_mmu_on_end 对应的物理地址。将 r5 和 r6 寄存器的值都向右移 SECTION_SHIFT,
接着再将 r5 左移 SECTION_SHIFT，以此达到对齐的作用。将 r5 左移之后的结果与 r7
相或，通过之前的分析，r7 寄存器存储着 proc_info_list.__cpu_mm_mmu_flags 的值。
接着让 r5 寄存器左移 PMD_ORDER 位之后与 r5 相加会得到一个页表地址，此时调用 str
指令将 r3 的值写入到对应的页表里面。然后比较现在有没有到达 __turn_mmu_on，如果到达，
则调过带 lo 的指令继续执行；如果没有到达，则执行带 lo 的指令。开发者可以在适当
的位置添加断点，然后使用 GDB 进行调试，调试的情况如下：

{% highlight haskell %}
5.0-arm32/linux/linux/vmlinux" at
	.text_addr = 0x60100000
	.head.text_addr = 0x60008000
	.rodata_addr = 0x60800000
(gdb) b BS_debug
Breakpoint 1 at 0x600080bc: file arch/arm/kernel/head.S, line 231.
(gdb) c
Continuing.

Breakpoint 1, __create_page_tables () at arch/arm/kernel/head.S:231
231		adr	r0, __turn_mmu_on_loc
(gdb) n
232		ldmia	r0, {r3, r5, r6}
(gdb) n
233		sub	r0, r0, r3			@ virt->phys offset
(gdb) info reg r3 r5 r6
r3             0x80008138          -2147450568
r5             0x80100000          -2146435072
r6             0x80100020          -2146435040
(gdb) n
234		add	r5, r5, r0			@ phys __turn_mmu_on
(gdb) n
235		add	r6, r6, r0			@ phys __turn_mmu_on_end
(gdb) n
236		mov	r5, r5, lsr #SECTION_SHIFT
(gdb) n
237		mov	r6, r6, lsr #SECTION_SHIFT
(gdb) n
239	1:	orr	r3, r7, r5, lsl #SECTION_SHIFT	@ flags + kernel base
(gdb) n
240		str	r3, [r4, r5, lsl #PMD_ORDER]	@ identity mapping
(gdb) info reg r3 r5 r7
r3             0x60100c0e          1611664398
r5             0x601               1537
r7             0xc0e               3086
(gdb) info reg r4 r5
r4             0x60004000          1610629120
r5             0x601               1537
(gdb) x/x 0x60005804
0x60005804:	0x00000000
(gdb) n
241		cmp	r5, r6
(gdb) x/x 0x60005804
0x60005804:	0x60100c0e
(gdb) info reg r6
r6             0x601               1537
(gdb) info reg r5
r5             0x601               1537
(gdb) n
242		addlo	r5, r5, #1			@ next section
(gdb) info reg r5
r5             0x601               1537
(gdb) n
243		blo	1b
(gdb) info reg r5
r5             0x601               1537
(gdb) n
248		add	r0, r4, #PAGE_OFFSET >> (SECTION_SHIFT - PMD_ORDER)
(gdb)
{% endhighlight %}

从上面的实践可以看到为 __enable_mmu 函数建立页表的过程。接下来执行的代码是：

{% highlight haskell %}
        /*
         * Map our RAM from the start to the end of the kernel .bss section.
         */
        add     r0, r4, #PAGE_OFFSET >> (SECTION_SHIFT - PMD_ORDER)
        ldr     r6, =(_end - 1)
        orr     r3, r8, r7
        add     r6, r4, r6, lsr #(SECTION_SHIFT - PMD_ORDER)
1:      str     r3, [r0], #1 << PMD_ORDER
        add     r3, r3, #1 << SECTION_SHIFT
        cmp     r0, r6
        bls     1b
{% endhighlight %}

这段代码的主要任务就是：从 RAM 起始虚拟地址到内核镜像(包括 .bss 段)结束的虚拟地址
建立页表。首先将 PAGE_OFFSET 向右移 (SECTION_SHIFT - PMD_ORDER) 位，然后加上
r4 寄存器，此时 PAGE_OFFSET 代表 RAM 起始的虚拟地址，r4 存储全局页目录的物理地址，
因此 "add r0, r4, #PAGE_OFFSET >> (SECTION_SHIFT - PMD_ORDER)" 的左右就是获得
RAM 起始虚拟地址对应的全局页目录的入口地址。接着调用 "ldr r6, =(_end - 1)", 通过
这段代码获得内核镜像结束虚拟地址，这个结束地址是包含了 .bss section 的。接下来执行
"orr r3, r8, r7" 代码， 此时 r8 代表 PHYS_OFFSET，这里就是构建第一块物理内存的
的页表内容，即物理地址加上 flags，将结果存储在 r3 寄存器中。接着调用
"add r6, r4, r6, lsr #(SECTION_SHIFT - PMD_ORDER)", 通过这段代码就可以获得
内核镜像结束的虚拟地址对应的页目录入口地址，以此作为循环结束的标志。接下来就是
循环将指定的内容填入全局页表里，首先调用 "str r3, [r0], #1 << PMD_ORDER" 代码，
将 r3 寄存器的值也就是页表内容写到 r0 对应的地址，写完之后再将 r0 寄存器的值增加
(1 << PMD_ORDER), 这样就可以指向下一个页表入口。接着执行代码
"add r3, r3, #1 << SECTION_SHIFT", 通过 add 指令，将 r3 寄存器对应的值增加
(1 << SECTION_SHIFT), 也就是指向下一段内存区块。最后在调用 cmp 指令检查页表项是不是
到达最后一项，如果不是，执行 bls 指令继续循环页表写入操作。开发者可以在适当
的位置加入断点，然后使用 GDB 进行调试，调试的结果如下：

{% highlight haskell %}
5.0-arm32/linux/linux/vmlinux" at
	.text_addr = 0x60100000
	.head.text_addr = 0x60008000
	.rodata_addr = 0x60800000
(gdb) b BS_debug
Breakpoint 1 at 0x600080ec: file arch/arm/kernel/head.S, line 248.
(gdb) n
Cannot find bounds of current function
(gdb) c
Continuing.

Breakpoint 1, __create_page_tables () at arch/arm/kernel/head.S:248
248		add	r0, r4, #PAGE_OFFSET >> (SECTION_SHIFT - PMD_ORDER)
(gdb) n
249		ldr	r6, =(_end - 1)
(gdb) info reg r0
r0             0x60006000          1610637312
(gdb) x/16x 0x60006000
0x60006000:	0x00000000	0x00000000	0x00000000	0x00000000
0x60006010:	0x00000000	0x00000000	0x00000000	0x00000000
0x60006020:	0x00000000	0x00000000	0x00000000	0x00000000
0x60006030:	0x00000000	0x00000000	0x00000000	0x00000000
(gdb) n
250		orr	r3, r8, r7
(gdb) n
251		add	r6, r4, r6, lsr #(SECTION_SHIFT - PMD_ORDER)
(gdb) info reg r3 r7 r8
r3             0x60000c0e          1610615822
r7             0xc0e               3086
r8             0x60000000          1610612736
(gdb) n
252	1:	str	r3, [r0], #1 << PMD_ORDER
(gdb) info reg r6
r6             0x6000602e          1610637358
(gdb) n
253		add	r3, r3, #1 << SECTION_SHIFT
(gdb) n
254		cmp	r0, r6
(gdb) b BS_IO
Breakpoint 2 at 0x6000810c: file arch/arm/kernel/head.S, line 281.
(gdb) c
Continuing.

Breakpoint 2, __create_page_tables () at arch/arm/kernel/head.S:281
281		mov	r0, r2, lsr #SECTION_SHIFT
(gdb) x/16x 0x60006000
0x60006000:	0x60000c0e	0x60100c0e	0x60200c0e	0x60300c0e
0x60006010:	0x60400c0e	0x60500c0e	0x60600c0e	0x60700c0e
0x60006020:	0x60800c0e	0x60900c0e	0x60a00c0e	0x60b00c0e
0x60006030:	0x00000000	0x00000000	0x00000000	0x00000000
(gdb)
{% endhighlight %}

从上面的调试结果可以看出内核为 RAM 到内核镜像结束的虚拟地址创建页表的过程。接下来
执行的代码是：

{% highlight haskell %}
        /*
         * Then map boot params address in r2 if specified.
         * We map 2 sections in case the ATAGs/DTB crosses a section boundary.
         */
        mov     r0, r2, lsr #SECTION_SHIFT
        movs    r0, r0, lsl #SECTION_SHIFT
        subne   r3, r0, r8
        addne   r3, r3, #PAGE_OFFSET
        addne   r3, r4, r3, lsr #(SECTION_SHIFT - PMD_ORDER)
        orrne   r6, r7, r0
        strne   r6, [r3], #1 << PMD_ORDER
        addne   r6, r6, #1 << SECTION_SHIFT
        strne   r6, [r3]
        ret     lr
{% endhighlight %}

接下来这段代码是为 uboot 传递给内核的 ATAGS 参数或者 DTB 对应的虚拟地址建立页表。
此时 r2 寄存器存储着 ATAGS 参数物理地址或者 DTB 的物理地址；r8 寄存器存储着
PHYS_OFFSET。首先将 r2 对应的物理地址按 SECTION_SHIFT 对齐，并将对齐之后的值存
储在 r0 寄存器，然后通过 "subne r3, r0, r8" 代码进行地址校正，然后在将 r3 寄存器
的值加上 PAGE_OFFSET，以此获得 ATAGS 或者 DTB 对应的虚拟地址。有了虚拟地址之后，
就通过向右移动 (SECTION_SHIFT - PMD_ORDER) 位，然后加上 r4 页表基地址，以此获得
ATAGS 或 DTB 虚拟地址对应的页表入口地址。通过 "orrne r6, r7, r0" 构建页表内容，
上面动作都完成之后，调用 strne 指令将 r6 页表内容写 r3 页表项入口地址，写完之后
将 r3 增加 (1 << PMD_ORDER) 以此指向下一个页表项入口地址，此时调用
"addne r6, r6, #1 << SECTION_SHIFT" 构建下一个页表项的内容，并调用
"strne r6, [r3]" 代码将页表内容写入到页表项里, 最后执行返回。从上面的分析可以知道，
内核一共为 ATAGS 或者 DTB 创建了 2M 的页表。开发者可以在适当的位置添加断点，然后
使用 GDB 进行调试，调试的情况如下：

{% highlight haskell %}
5.0-arm32/linux/linux/vmlinux" at
	.text_addr = 0x60100000
	.head.text_addr = 0x60008000
	.rodata_addr = 0x60800000
(gdb) b BS_debug
Breakpoint 1 at 0x6000810c: file arch/arm/kernel/head.S, line 280.
(gdb) c
Continuing.

Breakpoint 1, __create_page_tables () at arch/arm/kernel/head.S:280
280		mov	r0, r2, lsr #SECTION_SHIFT
(gdb) info reg r2
r2             0x69cff000          1775235072
(gdb) n
281		movs	r0, r0, lsl #SECTION_SHIFT
(gdb) info reg r0
r0             0x69c               1692
(gdb) n
282		subne	r3, r0, r8
(gdb) info reg r0
r0             0x69c00000          1774190592
(gdb) n
283		addne	r3, r3, #PAGE_OFFSET
(gdb) info reg r3
r3             0x9c00000           163577856
(gdb) n
284		addne	r3, r4, r3, lsr #(SECTION_SHIFT - PMD_ORDER)
(gdb) info reg r3
r3             0x89c00000          -1983905792
(gdb) n
285		orrne	r6, r7, r0
(gdb) info reg r3
r3             0x60006270          1610637936
(gdb) n
286		strne	r6, [r3], #1 << PMD_ORDER
(gdb) info reg r0 r6 r7
r0             0x69c00000          1774190592
r6             0x69c00c0e          1774193678
r7             0xc0e               3086
(gdb) n
287		addne	r6, r6, #1 << SECTION_SHIFT
(gdb) n
288		strne	r6, [r3]
(gdb) info reg r6
r6             0x69d00c0e          1775242254
(gdb) info reg r3
r3             0x60006274          1610637940
(gdb) n
355		ret	lr
(gdb)
{% endhighlight %}

上面的实践结果和预期分析的一致，支持内核前期的页表构建完成，接下来执行的代码是：

{% highlight haskell %}
        /*
         * The following calls CPU specific code in a position independent
         * manner.  See arch/arm/mm/proc-*.S for details.  r10 = base of
         * xxx_proc_info structure selected by __lookup_processor_type
         * above.
         *
         * The processor init function will be called with:
         *  r1 - machine type
         *  r2 - boot data (atags/dt) pointer
         *  r4 - translation table base (low word)
         *  r5 - translation table base (high word, if LPAE)
         *  r8 - translation table base 1 (pfn if LPAE)
         *  r9 - cpuid
         *  r13 - virtual address for __enable_mmu -> __turn_mmu_on
         *
         * On return, the CPU will be ready for the MMU to be turned on,
         * r0 will hold the CPU control register value, r1, r2, r4, and
         * r9 will be preserved.  r5 will also be preserved if LPAE.
         */     
        ldr     r13, =__mmap_switched           @ address to jump to after
                                                @ mmu has been enabled
{% endhighlight %}

这段代码主要是对当前各个寄存器的功能做了说明。此时 r1 寄存器存储着机器类型
信息；r2 寄存器存储着 atags/dtb 的指针；r4 寄存器存储着全局页表的基地址； r5 寄
存器存储着页表的高字节 (LPAE 模式)；r8 寄存器存储着 PHYS_OFFSET；r9 寄存器
存储着 CPUID；r10 寄存器存储着 proc_info_list 的地址；r13 寄存器存储着
__enable_mmu 的虚拟地址，最终指向 __turn_mmu_on。这里使用 ldr 指令，将
r13 指向 __mmap_switched 的虚拟地址。接下来执行的代码是：

{% highlight haskell %}
        badr    lr, 1f                          @ return (PIC) address
#ifdef CONFIG_ARM_LPAE
        mov     r5, #0                          @ high TTBR0
        mov     r8, r4, lsr #12                 @ TTBR1 is swapper_pg_dir pfn
#else
        mov     r8, r4                          @ set TTBR1 to swapper_pg_dir
#endif
        ldr     r12, [r10, #PROCINFO_INITFUNC]
        add     r12, r12, r10
        ret     r12
1:      b       __enable_mmu
{% endhighlight %}

这段代码的主要作用就是调用 ARMv7 Cortex-A9 对应的 proc_info_list 结构的
__cpu_flush。首先调用 badr 指令将 1f 处的地址存储到 lr 寄存器中，以此返回时候
直接跳转到 1f 处，1f 处包含了跳转到 __enable_mmu 的代码。接下来将 r4 寄存器的值
存储到 r8 寄存器，注释上解释这行代码的主要是为了设置 TTBR1 寄存器。接着调用
ldr 寄存器，将 "r10 + PROCINFO_INITFUNC" 指向的地址存储到 r12 寄存器中，通过
查找 PROCINFO_INITFUNC 的定义为 16，此时 r10 寄存器指向 ARMv7 Cortex-A9 对应的
proc_info_list 结构，在通过 PROCINFO_INITFUNC 偏移，此时指向了 proc_info_list
结构的 __cpu_flush。在调用 "add r12, r12, r10" 校正了 r12 的地址，更多关于
r10 寄存器 proc_info_list 内容，请查看：

> [ARMv7 Cortex-A9 proc_info_list 解析](#ARMv7 Cortex-A9 proc_info_list)

接着调用 "ret r12" 跳转到指定函数处运行，通过上面的文档可以知道，此时对于的位置
是：__v7_ca9mp_setup。在适当的位置添加断点，然后使用 GDB 进行调试，调试的情况如下：

{% highlight haskell %}
5.0-arm32/linux/linux/vmlinux" at
	.text_addr = 0x60100000
	.head.text_addr = 0x60008000
	.rodata_addr = 0x60800000
(gdb) b BS_debug
Breakpoint 1 at 0x60008064: file arch/arm/kernel/head.S, line 150.
(gdb) c
Continuing.

Breakpoint 1, stext () at arch/arm/kernel/head.S:150
150		ldr	r13, =__mmap_switched		@ address to jump to after
(gdb) n
stext () at arch/arm/kernel/head.S:152
152		badr	lr, 1f				@ return (PIC) address
(gdb) info reg r13
r13            0x80a002e0          0x80a002e0 <__mmap_switched>
(gdb) n
157		mov	r8, r4				@ set TTBR1 to swapper_pg_dir
(gdb) info reg lr
lr             0x6000807c          1610645628
(gdb) n
159		ldr	r12, [r10, #PROCINFO_INITFUNC]
(gdb) n
160		add	r12, r12, r10
(gdb) s
161		ret	r12
(gdb) s
__v7_cr8mp_setup () at arch/arm/mm/proc-v7.S:283
283		mov	r10, #(1 << 0)			@ Cache/TLB ops broadcasting
(gdb)
{% endhighlight %}

调试情况和预期一致，接下里执行的代码如下：

<span id="__v7_setup"></span>
{% highlight haskell %}
/*
 *      __v7_setup
 *
 *      Initialise TLB, Caches, and MMU state ready to switch the MMU
 *      on.  Return in r0 the new CP15 C1 control register setting.
 *
 *      r1, r2, r4, r5, r9, r13 must be preserved - r13 is not a stack
 *      r4: TTBR0 (low word)
 *      r5: TTBR0 (high word if LPAE)
 *      r8: TTBR1
 *      r9: Main ID register
 *
 *      This should be able to cover all ARMv7 cores.
 *
 *      It is assumed that:
 *      - cache type register is implemented
 */
__v7_ca5mp_setup:
__v7_ca9mp_setup:
__v7_cr7mp_setup:
__v7_cr8mp_setup:
        mov     r10, #(1 << 0)                  @ Cache/TLB ops broadcasting
        b       1f
__v7_ca7mp_setup:
__v7_ca12mp_setup:
__v7_ca15mp_setup:
__v7_b15mp_setup:
__v7_ca17mp_setup:
        mov     r10, #0
1:      adr     r0, __v7_setup_stack_ptr


        .align  2
__v7_setup_stack_ptr:
        .word   PHYS_RELATIVE(__v7_setup_stack, .)
ENDPROC(__v7_setup)

        .bss
        .align  2
__v7_setup_stack:
        .space  4 * 7                           @ 7 registers


#define PHYS_RELATIVE(v_data, v_text) ((v_data) - (v_text))
{% endhighlight %}

__v7_setup 的主要作用就是初始化 TLB，Caches，以及将 MMU 的状态设置为 on。
在执行 __v7_setup 之前，将 r4 用于 TTBR0 寄存器器，r8 寄存器用于 TTBR1 寄存器；
r9 寄存器存储着 ID 信息。经过之前的分析，__v7_setup 首先执行 __v7_ca9mp_setup，
首先将 1 存储到 r10 寄存器中，然后调用 ldr 指令将 __v7_setup_stack_ptr 的物理
地址存储到 r0 寄存器里。这里分析一下 __v7_setup_stack_ptr，其定义如上，
定义一个 word，其内容是 __v7_setup_stack 与当前地址之间的差值。__v7_setup_stack
位于 .bss 段，__v7_setup_stack 只建立了 7 个寄存器大小的空间，通过
__v7_setup_stack_ptr 可以计算出堆栈的位置。接下来执行的代码是:

{% highlight haskell %}
        ldr     r12, [r0]
        add     r12, r12, r0                    @ the local stack
        stmia   r12, {r1-r6, lr}                @ v7_invalidate_l1 touches r0-r6
        bl      v7_invalidate_l1
{% endhighlight %}

这段代码的作用就是找到本地堆栈，然后在调用 v7_invalidate_l1 之前将 r1, r2, r3, r4,
r5, r6, 以及 lr 寄存器的值压入堆栈，然后调用 v7_invalidate_l1。代码首先调用
ldr 获得堆栈的偏移，然后通过 add 指令计算出堆栈的物理地址，接着调用 stmia 指令
将 r1, r2, r3, r4, r5, r6, 以及 lr 寄存器的值压入堆栈，最后调用 bl 指令跳转到
v7_invalidate_l1 处执行。开发者可以在适当的位置添加断点进行 GDB 调试，调试情况如下：

{% highlight haskell %}
5.0-arm32/linux/linux/vmlinux" at
	.text_addr = 0x60100000
	.head.text_addr = 0x60008000
	.rodata_addr = 0x60800000
b(gdb) b BS_debug
Breakpoint 1 at 0x601196b4: file arch/arm/mm/proc-v7.S, line 293.
(gdb) c
Continuing.

Breakpoint 1, BS_debug () at arch/arm/mm/proc-v7.S:293
293		ldr	r12, [r0]
(gdb) n
294		add	r12, r12, r0			@ the local stack
(gdb) info reg r0
r0             0x601197d8          1611765720
(gdb) n
295		stmia	r12, {r1-r6, lr}		@ v7_invalidate_l1 touches r0-r6
(gdb) info reg r12
r12            0x60b696f8          1622578936
(gdb) x/16x 0x60b696f8
0x60b696f8:	0x00000000	0x00000000	0x00000000	0x00000000
0x60b69708:	0x00000000	0x00000000	0x00000000	0x00000000
0x60b69718:	0x00000000	0x00000000	0x00000000	0x00000000
0x60b69728:	0x00000000	0x00000000	0x00000000	0x00000000
(gdb) info reg r1 r2 r3 r4 r5 r6 lr
r1             0x8e0               2272
r2             0x69cff000          1775235072
r3             0x60006274          1610637940
r4             0x60004000          1610629120
r5             0x601               1537
r6             0x69d00c0e          1775242254
lr             0x6000807c          1610645628
(gdb) n
296		bl      v7_invalidate_l1
(gdb) x/16x 0x60b696f8
0x60b696f8:	0x000008e0	0x69cff000	0x60006274	0x60004000
0x60b69708:	0x00000601	0x69d00c0e	0x6000807c	0x00000000
0x60b69718:	0x00000000	0x00000000	0x00000000	0x00000000
0x60b69728:	0x00000000	0x00000000	0x00000000	0x00000000
(gdb)
{% endhighlight %}

通过上面的实践，可以看出堆栈压栈之后的变化，和预期分析一直，继续执行代码如下：

<span id="v7_invalidate_l1"></span>
{% highlight haskell %}
/*
 * The secondary kernel init calls v7_flush_dcache_all before it enables
 * the L1; however, the L1 comes out of reset in an undefined state, so
 * the clean + invalidate performed by v7_flush_dcache_all causes a bunch
 * of cache lines with uninitialized data and uninitialized tags to get
 * written out to memory, which does really unpleasant things to the main
 * processor.  We fix this by performing an invalidate, rather than a
 * clean + invalidate, before jumping into the kernel.
 *
 * This function is cloned from arch/arm/mach-tegra/headsmp.S, and needs
 * to be called for both secondary cores startup and primary core resume
 * procedures.  
 */
ENTRY(v7_invalidate_l1)
       mov     r0, #0
       mcr     p15, 2, r0, c0, c0, 0
       mrc     p15, 1, r0, c0, c0, 0
{% endhighlight %}

从上面的注释可以知道，在内核第二阶段的初始化过程中，需要在使能 L1 cache 之前调用
v7_flush_dcache_all。由于 cache reset 之后处于位置状态，所以 v7_flush_dcache_all
的执行会引起 clean 和 invalidate 动作，这些动作会导致 cache 大量的未初始化的
cache line 和 cache tag 被同步到主内存，这不是内核想要发生的事。因此在跳转到
kernel 之前修正了动作，仅仅执行 invalidate 动作，而不是 clean+invalidate 动作。
代码首先将 r0 寄存器，然后执行 "mcr p15, 2, r0, c0, c0, 0" 代码，此时对应
的 CP15 C0 布局如下：

![MMU](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/boot/BOOTCP15C0.png)

此时选中的寄存器是：CSSELR, Cache Size Select Register. 其内存布局如下：

![MMU](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/boot/BOOT000026.png)

代码先将 0 写入到 CSSELR 寄存器，通过 CSSELR 寄存器的 InD 域写入 0 代表
数据或一致 cache。CSSELR 寄存器的 Level 域写入 0 代表选择 Level 1 Cache。通过
这行代码，内核选中了 Level 1 D-cache。接着调用 "mrc p15, 1, r0, c0, c0, 0",
此时选中的是 CCSIDR, Cache Size ID Register 寄存器，其布局如下：

![MMU](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/boot/BOOT000028.png)

通过上面的代码，读取了 CCSIDR 寄存器的值到 r0 寄存器。开发者可以在适当的位置添加
断点，然后使用 GDB 进行调试，调试的情况如下：

{% highlight haskell %}
5.0-arm32/linux/linux/vmlinux" at
	.text_addr = 0x60100000
	.head.text_addr = 0x60008000
	.rodata_addr = 0x60800000
(gdb) b BS_debug
Breakpoint 1 at 0x60118b4c: file arch/arm/mm/cache-v7.S, line 37.
(gdb) c
Continuing.

Breakpoint 1, v7_invalidate_l1 () at arch/arm/mm/cache-v7.S:37
37	       mov     r0, #0
(gdb) n
38	       mcr     p15, 2, r0, c0, c0, 0
(gdb) n
39	       mrc     p15, 1, r0, c0, c0, 0
(gdb) n
41	       movw    r1, #0x7fff
(gdb) info reg r0
r0             0xe00fe019          -535830503
(gdb)
{% endhighlight %}

在执行后续代码之前，这里补充一下 Cacahe 基础知识：

![MMU](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/boot/BOOT000023.png)

Cache 的最小数据单元称为 cache line；多个连续的 cache line 称为一个 cache set；
cache 被均分成多个 cache set；每个 cache set 中含有的 cache line 的数量称为
cache way；cache tag 用于管理每个 cache line；cache line + cache tag + flags
称为一个 cache frame。由于不同的 cache 替换策略，所有 cache 的 flush 方法不同，
ARMv7 采用的是 Set/Way 方法 flush cache。更多 cache 内容请看：

> [ARMv7 Cache Manual]()

对于 CCSIDR 寄存器，其布局如下：

![MMU](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/boot/BOOT000028.png)

通过 CSSELR 选中 cache level 之后，CSSIDR 寄存器里描述了对应 cache level 的信息，
根据之前的实践可以知道，此时选中的是 Cache level 0，CSSIDR 寄存器的值 0xe00fe019,
解析寄存器数据的含义，CSSIDR LineSize 域的值是 0x1，代表 cache line 的长度是
8 words；CSSIDR Associativity 域的值是 0x3， 代表 cache way 的值是 4 路；
CSSIDR NumSets 域的值是 0x7f，代表 cache sets 的值为 0x80,；因此 cache 的
大小等于： 0x80 * 0x4 * 8 = 4096 words = 4096 * 4 = 16K, 在 ARMv7 中，word
的长度是 4 字节，因此 Level 1 cache 的大小为 16K。分析完毕之后，接下来执行的代码是：

{% highlight haskell %}
       movw    r1, #0x7fff
       and     r2, r1, r0, lsr #13

       movw    r1, #0x3ff

       and     r3, r1, r0, lsr #3      @ NumWays - 1
       add     r2, r2, #1              @ NumSets

       and     r0, r0, #0x7
       add     r0, r0, #4      @ SetShift

       clz     r1, r3          @ WayShift
       add     r4, r3, #1      @ NumWays
{% endhighlight %}

代码首先将 0x7fff 的值存储在 r1 寄存器中，然后将 r0 寄存器的值右移 13 位，然后与
0x7fff，并将相与的结果存储在 r2 寄存器中，以此获得 CCSIDR 寄存器 NumSets 域的值。
接着用同样的办法，将 0x3ff 存储到 r1 寄存器中，然后将 r0 寄存器的值右移 3 位，
然后与 r1 寄存器的值相与，结果存储在 r3 寄存器中，以此获得 CCSIDR 的 Associativity
域值。将 r2 寄存器的值加 1 获得 Cache Set 的正确值。同理将 r0 寄存器的值与 0x7
相与，以此获得 cache line 的值，然后将 r0 寄存器的值加 4。使用 clz 指令计算
WayShift 的值，最后通过 add 指令，计算真实 Way 的值。开发者可以在适当的位置添加
断点，然后使用 GDB 进行调试，调试结果如下：

{% highlight haskell %}
5.0-arm32/linux/linux/vmlinux" at
	.text_addr = 0x60100000
	.head.text_addr = 0x60008000
	.rodata_addr = 0x60800000
(gdb) b BS_debug
Breakpoint 1 at 0x60118b58: file arch/arm/mm/cache-v7.S, line 41.
(gdb) c
Continuing.

Breakpoint 1, v7_invalidate_l1 () at arch/arm/mm/cache-v7.S:41
41	       movw    r1, #0x7fff
(gdb) info reg r0
r0             0xe00fe019          -535830503
(gdb) n
42	       and     r2, r1, r0, lsr #13
(gdb) n
44	       movw    r1, #0x3ff
(gdb) info reg r2
r2             0x7f                127
(gdb) n
46	       and     r3, r1, r0, lsr #3      @ NumWays - 1
(gdb) n
47	       add     r2, r2, #1              @ NumSets
(gdb) info reg r3
r3             0x3                 3
(gdb) n
49	       and     r0, r0, #0x7
(gdb) info reg r2
r2             0x80                128
(gdb) n
50	       add     r0, r0, #4      @ SetShift
(gdb) info reg r0
r0             0x1                 1
(gdb) n
52	       clz     r1, r3          @ WayShift
(gdb) info reg r1 r3
r1             0x3ff               1023
r3             0x3                 3
(gdb) n
53	       add     r4, r3, #1      @ NumWays
(gdb) info reg r1 r3
r1             0x1e                30
r3             0x3                 3
(gdb) n
54	1:     sub     r2, r2, #1      @ NumSets--
(gdb) info reg r4
r4             0x4                 4
(gdb)
{% endhighlight %}

通过上面的实践已经知道内核是如何获得 Level 1 cache 的信息，至此，内核为什么要
获得这些信息，以及内核如何 flush 所有的 cache line 呢？在讲其他代码之前，这里
科普一下 ARMv7 flush 的方法，通过之前的介绍，ARMv7 使用 Cache Set/Way 的方式
管理 cache，所以在 flush cache 的时候也是按 Set/Way 方式 flush cache。ARMv7
提供了一个寄存器 DCCISW，只要往 DCCISW 寄存器中写入 Set 和 Way 信息之后，
CPU 就会根据 DCCISW 中的信息去 flush 指定的 cache。DCCISW 的内存布局如下：

![MMU](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/boot/BOOT000029.png)

通过寄存器的描述，知道上面的代码为什么要这样处理从 CSSIDR 寄存器中值，从 DCCISW
的布局可以知道，只要往 DCCISW 最高位往地位写入 way 的信息，因此有了 "clz r1, r3"
这行代码；同理， DCCISW Set 域指定需要刷新的 set，因此需要将指定的 Set 信息写入
到这个域。因此接下来的代码如下：

{% highlight haskell %}
1:     sub     r2, r2, #1      @ NumSets--
       mov     r3, r4          @ Temp = NumWays
2:     subs    r3, r3, #1      @ Temp--
       mov     r5, r3, lsl r1
       mov     r6, r2, lsl r0
       orr     r5, r5, r6      @ Reg = (Temp<<WayShift)|(NumSets<<SetShift)
       mcr     p15, 0, r5, c7, c6, 2
       bgt     2b
       cmp     r2, #0
       bgt     1b
       dsb     st
       isb
       ret     lr
{% endhighlight %}

通过上面对 DCCISW 的描述，可以知道这段代码的主要任务就是按 Set/Way 的方式 flush
cache，因此只要循环将 cache 按 Set/Way 方式写入，cache 就可以被 flush。
首先 r2 寄存器存储着 cache set 的数量，这里可以将 set 理解为 “行” 的意思，然后
r4 寄存器存储着 cache way 的数量，这里可以将 way 理解为 “列” 的意思。首先
调用 "sub r2, r2, #1" 获得一个新的 set 号，再将 r4 寄存器的值也就是 way 的值
存储到 r3 寄存器。通过 "subs r3, r3, #1" 获得一个新列，通过接下来几条命令，
将 set 和 way 数据构造成 DCCISW 寄存器所需要的格式，然后调用
"mcr p15, 0, r5, c7, c6, 2" 命令将值写入到 DCCISW 寄存器里，此时 CP15 c7 布局如下：

![MMU](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/boot/BOOTCP15C7.png)

通过命令选中了 DCCISW 寄存器，然后通过 mcr 指令将数据写入到 DCCISW 寄存器里。
以此反复，可以总结为每个 set 一共循环 way 次写 DCCISW 操作，因此总共循环了
set * way - 1 次循环。执行完上面的命令之后，cache 已经被 flush 完毕，那么
函数直接返回。

再次回到 __v7_setup 即 __v7_ca9mp_setup, 继续执行代码如下：

{% highlight haskell %}
        ldmia   r12, {r1-r6, lr}
#ifdef CONFIG_SMP
        orr     r10, r10, #(1 << 6)             @ Enable SMP/nAMP mode
        ALT_SMP(mrc     p15, 0, r0, c1, c0, 1)
        ALT_UP(mov      r0, r10)                @ fake it for UP
        orr     r10, r10, r0                    @ Set required bits
        teq     r10, r0                         @ Were they already set?
        mcrne   p15, 0, r10, c1, c0, 1          @ No, update register
#endif
{% endhighlight %}

从调用 v7_invalidate_l1 中返回之后，调用 ldmia 指令将之前压入堆栈的寄存器
全部返回给原有的寄存器。由于内核开启了 CONFIG_SMP 宏，那么系统支持 SMP，
但也支持多核中单核心启动，因此上面代码的 ALT_SMP 代码将被丢弃，而使用 ALT_UP
对应的代码。由之前的代码可以知道 r10 此时的值为 1，那么调用 orr 指令之后，r10
寄存器变为了 0x40，再将 r10 寄存器的值存储到 r0 寄存器，接这几条命令之后使用
teq 指令检查 r10 与 r0 是否相等，如果不相等，则执行 mcrne 指。通过上面的分析，
r10 和 r0 在单 CPU 启动的时候是相等的，因此这段代码无实际意义。开发者可以在
适当的位置添加断点，然后使用 GDB 进行调试，调试的结果如下：

{% highlight haskell %}
5.0-arm32/linux/linux/vmlinux" at
	.text_addr = 0x60100000
	.head.text_addr = 0x60008000
	.rodata_addr = 0x60800000
(gdb) b BS_debug
Breakpoint 1 at 0x601196c8: file arch/arm/mm/proc-v7.S, line 299.
(gdb) c
Continuing.

Breakpoint 1, BS_debug () at arch/arm/mm/proc-v7.S:299
299		orr	r10, r10, #(1 << 6)		@ Enable SMP/nAMP mode
(gdb) info reg r10
r10            0x1                 1
(gdb) n
300		ALT_SMP(mrc	p15, 0, r0, c1, c0, 1)
(gdb) info reg r10
r10            0x41                65
(gdb) info reg r0
r0             0x5                 5
(gdb) n
302		orr	r10, r10, r0			@ Set required bits
(gdb) info reg r0 r10
r0             0x41                65
r10            0x41                65
(gdb) n
303		teq	r10, r0				@ Were they already set?
(gdb) info reg r0 r10
r0             0x41                65
r10            0x41                65
(gdb) n
304		mcrne	p15, 0, r10, c1, c0, 1		@ No, update register
(gdb)
{% endhighlight %}

实践的结果和预期的一致。接下来执行的代码是：

<span id="__v7_setup_cont"></span>
{% highlight haskell %}
__v7_setup_cont:
        and     r0, r9, #0xff000000             @ ARM?
        teq     r0, #0x41000000
        bne     __errata_finish
        and     r3, r9, #0x00f00000             @ variant
        and     r6, r9, #0x0000000f             @ revision
        orr     r6, r6, r3, lsr #20-4           @ combine variant and revision
        ubfx    r0, r9, #4, #12                 @ primary part number

        /* Cortex-A8 Errata */
        ldr     r10, =0x00000c08                @ Cortex-A8 primary part number
        teq     r0, r10
        beq     __ca8_errata

        /* Cortex-A9 Errata */
        ldr     r10, =0x00000c09                @ Cortex-A9 primary part number
        teq     r0, r10
        beq     __ca9_errata
{% endhighlight %}

r9 寄存器存储着 CPUID 信息，通过 and 指令和 teq 指令联合使用，判断当前 CPU
是不是 ARM，如果不是，直接跳转到 __errata_finish; 如果是则需要对特定的 ARM 系列进行
处理。接着从 r9 寄存器中提取 24-27 bits 和 0-3 bits，然后组合成特定的数据，使用
ubfx 继续拼凑数据并存储到 r0 寄存器。接着就是变量 Cortex-A 家族的表，使用 ldr 指令
将 r10 寄存器设置为特定的值，然后检查 r0 与 r10 是否相等，如果相等则跳转到指定的
处理函数。由于实践平台是 ARMv7 Cortex-A9MP，因此这里会跳转到 __ca9_errata，
开发者可以在适当的位置添加断点，然后使用 GDB 进行调试，调试的情况如下：

{% highlight haskell %}
5.0-arm32/linux/linux/vmlinux" at
	.text_addr = 0x60100000
	.head.text_addr = 0x60008000
	.rodata_addr = 0x60800000
(gdb) b BS_debug
Breakpoint 1 at 0x6011970c: file arch/arm/mm/proc-v7.S, line 476.
(gdb) c
Continuing.

Breakpoint 1, __v7_setup () at arch/arm/mm/proc-v7.S:476
476		and	r0, r9, #0xff000000		@ ARM?
(gdb) n
477		teq	r0, #0x41000000
(gdb) info reg r9
r9             0x410fc090          1091551376
(gdb) n
478		bne	__errata_finish
(gdb) n
479		and	r3, r9, #0x00f00000		@ variant
(gdb) n
480		and	r6, r9, #0x0000000f		@ revision
(gdb) n
481		orr	r6, r6, r3, lsr #20-4		@ combine variant and revision
(gdb) n
482		ubfx	r0, r9, #4, #12			@ primary part number
(gdb) n
485		ldr	r10, =0x00000c08		@ Cortex-A8 primary part number
(gdb) n
486		teq	r0, r10
(gdb) n
487		beq	__ca8_errata
(gdb) n
490		ldr	r10, =0x00000c09		@ Cortex-A9 primary part number
(gdb) n
491		teq	r0, r10
(gdb) s
492		beq	__ca9_errata
(gdb) s
__ca9_errata () at arch/arm/mm/proc-v7.S:368
368		b	__errata_finish
(gdb)
{% endhighlight %}

实践的结果和分析的一致，最后跳转到 __ca9_errata, 接下来的代码如下：

{% highlight haskell %}
__ca9_errata:
#ifdef CONFIG_ARM_ERRATA_742230
        cmp     r6, #0x22                       @ only present up to r2p2
        mrcle   p15, 0, r0, c15, c0, 1          @ read diagnostic register
        orrle   r0, r0, #1 << 4                 @ set bit #4
        mcrle   p15, 0, r0, c15, c0, 1          @ write diagnostic register
#endif
#ifdef CONFIG_ARM_ERRATA_742231
        teq     r6, #0x20                       @ present in r2p0
        teqne   r6, #0x21                       @ present in r2p1
        teqne   r6, #0x22                       @ present in r2p2
        mrceq   p15, 0, r0, c15, c0, 1          @ read diagnostic register
        orreq   r0, r0, #1 << 12                @ set bit #12
        orreq   r0, r0, #1 << 22                @ set bit #22
        mcreq   p15, 0, r0, c15, c0, 1          @ write diagnostic register
#endif
#ifdef CONFIG_ARM_ERRATA_743622
        teq     r3, #0x00200000                 @ only present in r2p*
        mrceq   p15, 0, r0, c15, c0, 1          @ read diagnostic register
        orreq   r0, r0, #1 << 6                 @ set bit #6
        mcreq   p15, 0, r0, c15, c0, 1          @ write diagnostic register
#endif
#if defined(CONFIG_ARM_ERRATA_751472) && defined(CONFIG_SMP)
        ALT_SMP(cmp r6, #0x30)                  @ present prior to r3p0
        ALT_UP_B(1f)
        mrclt   p15, 0, r0, c15, c0, 1          @ read diagnostic register
        orrlt   r0, r0, #1 << 11                @ set bit #11
        mcrlt   p15, 0, r0, c15, c0, 1          @ write diagnostic register
1:
#endif
        b       __errata_finish
{% endhighlight %}

由于 CONFIG_ARM_ERRATA_742230 宏，CONFIG_ARM_ERRATA_742231 宏，
CONFIG_ARM_ERRATA_743622 宏，CONFIG_ARM_ERRATA_751472 宏在本实践中都为打开，
因此 __ca9_errata 只执行了 "b __errata_finish"，那么接下来执行的代码如下：

{% highlight haskell %}
__errata_finish:
        mov     r10, #0
        mcr     p15, 0, r10, c7, c5, 0          @ I+BTB cache invalidate
#ifdef CONFIG_MMU
        mcr     p15, 0, r10, c8, c7, 0          @ invalidate I + D TLBs
        v7_ttb_setup r10, r4, r5, r8, r3        @ TTBCR, TTBRx setup
        ldr     r3, =PRRR                       @ PRRR
        ldr     r6, =NMRR                       @ NMRR
        mcr     p15, 0, r3, c10, c2, 0          @ write PRRR
        mcr     p15, 0, r6, c10, c2, 1          @ write NMRR
#endif
        dsb
{% endhighlight %}

接下来这段代码就是 MMU 设置的核心代码，在分析代码之前，先讲解涉及的相关寄存器。
"mcr p15, 0, r10, c7, c5, 0" 代码中，对 CP15 C7 中的寄存器操作，此时布局如下：

![MMU](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/boot/BOOTCP15C7.png)

CP15 C7 是与 Cache 维护和地址转换有关的寄存器，此时选中寄存器：
ICIALLU, Invalidate all instruction caches to PoU. 对该寄存器执行写操作会引起
所有指令 cache 无效(PoU)。下一个涉及的寄存器位于 CP15 C8 里，其包含了很多 TLB 相关
的寄存器，其布局如下：

![MMU](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/boot/BOOTCP15C8.png)

"mcr p15, 0, r10, c8, c7, 0" 涉及的寄存器是：TLBIALL, invalidate unified TLB.
往这个寄存器里面写操作会导致所有的 TLB 无效。下一个涉及的寄存器位于 CP15 c10 里，
其包含了内存的重映射和 TLB 控制操作，其布局如下：

![MMU](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/boot/BOOTCP15C10.png)

"mcr p15, 0, r3, c10, c2, 0" 涉及的寄存器是：PRRR, Primary Region Remap Register,
其布局如下：

![MMU](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/boot/BOOT000046.png)

接下来涉及的寄存器也在 CP15 c10 里，"mcr p15, 0, r6, c10, c2, 1" 涉及的寄存器是：
NMRR, Normal Memory Remap Register, 其内存布局是：

![MMU](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/boot/BOOT000047.png)

更多寄存器描述请看 ARMv7 Usermanual。接下来分析代码。首先将 r10 寄存器设置为 0，
然后执行 "mcr p15, 0, r10, c7, c5, 0" 代码，向 ICIALLU 寄存器，这样会让
I-BTB cache 无效。由于启用了 MMU，那么 CONFIG_MMU 宏启用，接下来执行
"mcr p15, 0, r10, c8, c7, 0" 代码，向 TLBIALL 寄存器写入 0，导致
I-TLBs 和 D-TLBs 无效。接下来执行代码 "v7_ttb_setup r10, r4, r5, r8, r3",
其中 v7_ttb_setup 的定义如下：

<span id="v7_ttb_setup"></span>
{% highlight haskell %}
        /*
         * Macro for setting up the TTBRx and TTBCR registers.
         * - \ttb0 and \ttb1 updated with the corresponding flags.
         */
        .macro  v7_ttb_setup, zero, ttbr0l, ttbr0h, ttbr1, tmp
        mcr     p15, 0, \zero, c2, c0, 2        @ TTB control register
        ALT_SMP(orr     \ttbr0l, \ttbr0l, #TTB_FLAGS_SMP)
        ALT_UP(orr      \ttbr0l, \ttbr0l, #TTB_FLAGS_UP)
        ALT_SMP(orr     \ttbr1, \ttbr1, #TTB_FLAGS_SMP)
        ALT_UP(orr      \ttbr1, \ttbr1, #TTB_FLAGS_UP)
        mcr     p15, 0, \ttbr1, c2, c0, 1       @ load TTB1
        .endm
{% endhighlight %}

v7_ttb_setup 宏主要的作用是：
v7_ttb_setup 首先执行的代码是 "mcr p15, 0, \zero, c2, c0, 2", 此时涉及 CP15 C2,
其布局如下：

![MMU](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/boot/BOOTCP15C2.png)

代码写入的寄存器是： TTBCR, Translation Table Base Control Register.
TTBCR 寄存器用于控制地址转换，其布局如下：

![MMU](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/boot/BOOT000015.png)

由于本实践平台支持 SMP 单核心启动，因此 v7_ttb_setup 宏中 ALT_SMP 代码不执行，只
执行 ALT_UP 宏包含的代码，因此 v7_ttb_setup 宏首先向 TTBCR 寄存器中写入 0，初始化
TTBCR 寄存器。接着使用两条 orr 指令对 ttbr0l 和 ttbr1 参数添加了 TTB_FLAGS_UP
标志。然后执行 "mcr p15, 0, \ttbr1, c2, c0, 1", 其涉及到 TTBR1 寄存器，
该寄存器 Translation Table Base Register 1. 所以 v7_ttb_setup 的作用就是
清零 TTBCR 寄存器，然后向 TTB1 寄存器中写入带 TTB_FLAGS_UP 标志的值。回到
__errata_finish，继续分析代码：

{% highlight haskell %}
#ifdef CONFIG_MMU
        mcr     p15, 0, r10, c8, c7, 0          @ invalidate I + D TLBs
        v7_ttb_setup r10, r4, r5, r8, r3        @ TTBCR, TTBRx setup
        ldr     r3, =PRRR                       @ PRRR
        ldr     r6, =NMRR                       @ NMRR
        mcr     p15, 0, r3, c10, c2, 0          @ write PRRR
        mcr     p15, 0, r6, c10, c2, 1          @ write NMRR
#endif
{% endhighlight %}

继续上面的分析，调用 v7_ttb_setup 设置了 TTBCR 和 TTBR1 寄存器，然后将 r3 的值
设置为 PRRR，这里可以分析对 PRRR 寄存器的设置，PRRR 宏的定义如下：

{% highlight haskell %}
        /*
         * Memory region attributes with SCTLR.TRE=1
         *
         *   n = TEX[0],C,B
         *   TR = PRRR[2n+1:2n]         - memory type
         *   IR = NMRR[2n+1:2n]         - inner cacheable property
         *   OR = NMRR[2n+17:2n+16]     - outer cacheable property
         *
         *                      n       TR      IR      OR
         *   UNCACHED           000     00
         *   BUFFERABLE         001     10      00      00
         *   WRITETHROUGH       010     10      10      10
         *   WRITEBACK          011     10      11      11
         *   reserved           110
         *   WRITEALLOC         111     10      01      01
         *   DEV_SHARED         100     01
         *   DEV_NONSHARED      100     01
         *   DEV_WC             001     10
         *   DEV_CACHED         011     10
         *
         * Other attributes:
         *
         *   DS0 = PRRR[16] = 0         - device shareable property
         *   DS1 = PRRR[17] = 1         - device shareable property
         *   NS0 = PRRR[18] = 0         - normal shareable property
         *   NS1 = PRRR[19] = 1         - normal shareable property
         *   NOS = PRRR[24+n] = 1       - not outer shareable
         */
.equ    PRRR,   0xff0a81a8
.equ    NMRR,   0x40e040e0
{% endhighlight %}

首先解析 PRRR 对于 PRRR 寄存器的设置。每个页表项同包含了 TEX[0],C,B 三个位，这
三个位按下表的方式组成一个 n 值：

![MMU](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/boot/BOOT000015.png)

通过上面的 n 值可以在 PRRR 寄存器中找到对应的 NOSn 域和 TRn 域，这些域都说明了
不同的内存类型的属性，由于这部分比较复杂，更多细节请看：

> [ARMv7 Memory Manual]()

NMRR 提供了附加的映射控制域，其也通过页表的 TEX[0],C,B 位决定，由于这部分比较复杂，
更多详细内容，请看上面提供的文档。回到 __errata_finish，继续分析代码：

{% highlight haskell %}
#ifdef CONFIG_MMU
        mcr     p15, 0, r10, c8, c7, 0          @ invalidate I + D TLBs
        v7_ttb_setup r10, r4, r5, r8, r3        @ TTBCR, TTBRx setup
        ldr     r3, =PRRR                       @ PRRR
        ldr     r6, =NMRR                       @ NMRR
        mcr     p15, 0, r3, c10, c2, 0          @ write PRRR
        mcr     p15, 0, r6, c10, c2, 1          @ write NMRR
#endif
        dsb
{% endhighlight %}

接着两条 mcr 指令将 PRRR 的值写入了 PRRR 寄存器，同理将 NMRR 的值写入到 NMRR 寄存器，
写入之后，页表映射的属性已经设置好了。通过上面的指令，内核已经设置好了页表转换需要
的基地址寄存器 TTBR 和控制器 TTBCR，以及页表映射属性 PRRR 和控制指令 NMRR，更多
页表内容请看：

[ARMv7 分页原理及实践教程]()

最后执行一条 dsb 内存屏蔽指令让上面的设置有效，接下来执行的代码是：

{% highlight haskell %}
#ifndef CONFIG_ARM_THUMBEE
        mrc     p15, 0, r0, c0, c1, 0           @ read ID_PFR0 for ThumbEE
        and     r0, r0, #(0xf << 12)            @ ThumbEE enabled field
        teq     r0, #(1 << 12)                  @ check if ThumbEE is present
        bne     1f
        mov     r3, #0
        mcr     p14, 6, r3, c1, c0, 0           @ Initialize TEEHBR to 0
        mrc     p14, 6, r0, c0, c0, 0           @ load TEECR
        orr     r0, r0, #1                      @ set the 1st bit in order to
        mcr     p14, 6, r0, c0, c0, 0           @ stop userspace TEEHBR access
1:
#endif
{% endhighlight %}

这段代码的主要任务就是设置 ThumbEE，检查当前系统是否支持 ThumbEE, 如果支持就将
TEEHBR 寄存器设置为 0，然后设置 TEECR 寄存器，使 Unprivileged access disabled。
代码 "mrc p15, 0, r0, c0, c1, 0" 首先读取 ID_PFR0 寄存器，其寄存器布局如下：

![MMU](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/boot/BOOT000051.png)

其中 State3 域 bits[15:12] 指明系统是否支持 ThumbEE, 该域为 1 代表支持 ThumbEE;
如果该域为 0， 则代表不支持 ThumbEE。代码中获得该域之后，使用 teq 指令对该域进行对比，
如果支持，则继续执行，如果不支持，则跳转到 1 处继续执行。由于 ARMv7 支持 ThumbEE，
那么继续执行下面代码，首先将 r3 寄存器设置为 0， 然后调用 "mcr p14, 6, r3, c1, c0, 0"
代码将 0 写入到 TEEHBR 寄存器，TEEHBR 寄存器的布局如下：

![MMU](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/boot/BOOT000049.png)

将 TEEHBR 寄存器设置为了 0 之后，初始化了 TEEHBR 寄存器。接着执行代码
"mrc p14, 6, r0, c0, c0, 0" 读取 TEECR 寄存器，寄存器布局如下：

![MMU](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/boot/BOOT000050.png)

接着调用 orr 指令将 TEECR 的 bit0 置位，这样就会使 Unprivileged access disabled.
最后将 r0 寄存器的写入到 TEECR 寄存器。开发者可以在适当的位置添加断点，然后使用
GDB 进行调试，调试的情况如下：

{% highlight haskell %}
5.0-arm32/linux/linux/vmlinux" at
	.text_addr = 0x60100000
	.head.text_addr = 0x60008000
	.rodata_addr = 0x60800000
(gdb) b BS_debug
Breakpoint 1 at 0x60119794: file arch/arm/mm/proc-v7.S, line 522.
(gdb) c
Continuing.

Breakpoint 1, __v7_setup () at arch/arm/mm/proc-v7.S:522
522		mrc	p15, 0, r0, c0, c1, 0		@ read ID_PFR0 for ThumbEE
(gdb) n
523		and	r0, r0, #(0xf << 12)		@ ThumbEE enabled field
(gdb) info reg r0
r0             0x1031              4145
(gdb) n
524		teq	r0, #(1 << 12)			@ check if ThumbEE is present
(gdb) n
525		bne	1f
(gdb) n
526		mov	r3, #0
(gdb) n
527		mcr	p14, 6, r3, c1, c0, 0		@ Initialize TEEHBR to 0
(gdb) info reg r3
r3             0x0                 0
(gdb) n
528		mrc	p14, 6, r0, c0, c0, 0		@ load TEECR
(gdb) n
529		orr	r0, r0, #1			@ set the 1st bit in order to
(gdb) info reg r0
r0             0x0                 0
(gdb) n
530		mcr	p14, 6, r0, c0, c0, 0		@ stop userspace TEEHBR access
(gdb) info reg r0
r0             0x1                 1
(gdb) n
533		adr	r3, v7_crval
(gdb)
{% endhighlight %}

GDB 实践的结果符合预期，接下来执行的代码如下：

{% highlight haskell %}
        adr     r3, v7_crval
        ldmia   r3, {r3, r6}
 ARM_BE8(orr    r6, r6, #1 << 25)               @ big-endian page tables
#ifdef CONFIG_SWP_EMULATE
        orr     r3, r3, #(1 << 10)              @ set SW bit in "clear"
        bic     r6, r6, #(1 << 10)              @ clear it in "mmuset"
#endif
        mrc     p15, 0, r0, c1, c0, 0           @ read control register
        bic     r0, r0, r3                      @ clear bits them
        orr     r0, r0, r6                      @ set them
 THUMB( orr     r0, r0, #1 << 30        )       @ Thumb exceptions
        ret     lr                              @ return to head.S:__ret
{% endhighlight %}

这段代码的主要任务就是构造一段数据，用于设置 SCTLR 寄存器。SCTLR 寄存器的布局如下：

![MMU](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/boot/BOOT000014.png)

其中，ARMv7 中内核想将 SCTLR 寄存器设置为指定内容，将需要设置的内容存储在 v7_crval,
其源码如下：

{% highlight haskell %}
        /*   AT
         *  TFR   EV X F   I D LR    S
         * .EEE ..EE PUI. .T.T 4RVI ZWRS BLDP WCAM
         * rxxx rrxx xxx0 0101 xxxx xxxx x111 xxxx < forced
         *   01    0 110       0011 1100 .111 1101 < we want
         */
        .align  2
        .type   v7_crval, #object
v7_crval:
        crval   clear=0x2120c302, mmuset=0x10c03c7d, ucset=0x00c01c7c
{% endhighlight %}

从上面的定义可以看出，内核想将 SCTLR 寄存器设置为指定内容，即有些 bit 位必须
设置为已知状态，那么就分析一下这些位的含义：

{% highlight haskell %}
M,bit[0] MMU enable.
        该位置位之后， PL1&0 Stage 1 MMU 使能。
A,bit[1] Alignment check enable.
        该位清零，那么 Alignment fault checking disabled.
C,bit[2] Cache enable.
        该位置位，Data and unified cache 使能。
Bits[4:3] Reserved,RAO/SBOP

CP15BEN,bit[5] CP15 barrier enable.
        该位置位，CP15 支持内存屏蔽操作。可以直接使用 isb，dsb，以及 dmb 指令。
Bit[9:6] Reserved，RAO/SBOP

SW,bit[10] SWP and SWPB enable.
        该位置位，SWP 和 SWPB 指令可以使用。
Z,bit[11] Branch prediction enable.
        该位置位，程序分支预测使能。
I,bit[12] Instruction cache enable.
        该位置位，指令 cache 启用。
V,bit[13] Vector bit.
        该位置位，High exception vector (Hivecs), 基地址是 0xFFFF0000.
RR,bit[14] Round Robin select.
        该位清零，Cache 采用正常的替换策略，例如随机替换。
Bit[15] Reserved, RAZ/SBZP

FI,bit[21] Fast interrupts configuration enable.
        该位清零，All performance features enabled。
U,bit[23:22] RAO/SBOP.

VE,bit[24], Interrupt Vector Enable.
        该位清零，User the FIQ and IRQ vector from thevector table。
TRE,bit[28] TEX remap enable.
        该位置位，TEX remap 使能。TEX[0],C,B bits, with the MMU remap register
        describe region attributes.
AFE,bit[29] Access flag enable.
        该位置位，在页表项中，AP[0] 是 Access 标志。
{% endhighlight %}

上面的代码中，就是够着了上面的 SCTLR 寄存器的描述，并存储在 r0 寄存器中，最后返回
arch/arm/kernel/head.S 处继续执行，开发者可以在适当的位置添加断点，然后使用 GDB
进行调试，调试的情况如下：

{% highlight haskell %}
5.0-arm32/linux/linux/vmlinux" at
	.text_addr = 0x60100000
	.head.text_addr = 0x60008000
	.rodata_addr = 0x60800000
(gdb) b BS_debug
Breakpoint 1 at 0x601197b8: file arch/arm/mm/proc-v7.S, line 533.
(gdb) c
Continuing.

Breakpoint 1, __v7_setup () at arch/arm/mm/proc-v7.S:533
533		adr	r3, v7_crval
(gdb) n
534		ldmia	r3, {r3, r6}
(gdb) n
537		orr     r3, r3, #(1 << 10)              @ set SW bit in "clear"
(gdb) info reg r3 r6
r3             0x2120c302          555795202
r6             0x10c03c7d          281033853
(gdb) n
538		bic     r6, r6, #(1 << 10)              @ clear it in "mmuset"
(gdb) n
540	   	mrc	p15, 0, r0, c1, c0, 0		@ read control register
(gdb) info reg r3 r6
r3             0x2120c702          555796226
r6             0x10c0387d          281032829
(gdb) n
541		bic	r0, r0, r3			@ clear bits them
(gdb) info reg r0 r3 r6
r0             0xc55070            12931184
r3             0x2120c702          555796226
r6             0x10c0387d          281032829
(gdb) n
542		orr	r0, r0, r6			@ set them
(gdb) info reg r0 r3 r6
r0             0xc51070            12914800
r3             0x2120c702          555796226
r6             0x10c0387d          281032829
(gdb) n
544		ret	lr				@ return to head.S:__ret
(gdb) info reg r0 r3 r6
r0             0x10c5387d          281360509
r3             0x2120c702          555796226
r6             0x10c0387d          281032829
(gdb)
{% endhighlight %}

通过实践可以知道最终 r0 寄存器的值是 0x10c5387d, 这正好与 v7_crval 中的设定是一
致的，处理 bit[10] 与 v7_crval 不一致，是因为内核支持 CONFIG_SWP_EMULATE 宏，
将该 bit 清零了。接着执行返回指令，返回到 arch/arm/kernel/head.S 处继续执行。

<span id="__enable_mmu"></span>
{% highlight haskell %}
/*
 * Setup common bits before finally enabling the MMU.  Essentially
 * this is just loading the page table pointer and domain access
 * registers.  All these registers need to be preserved by the
 * processor setup function (or set in the case of r0)
 *
 *  r0  = cp#15 control register
 *  r1  = machine ID
 *  r2  = atags or dtb pointer
 *  r4  = TTBR pointer (low word)
 *  r5  = TTBR pointer (high word if LPAE)
 *  r9  = processor ID
 *  r13 = *virtual* address to jump to upon completion
 */
__enable_mmu:
#if defined(CONFIG_ALIGNMENT_TRAP) && __LINUX_ARM_ARCH__ < 6
        orr     r0, r0, #CR_A
#else
        bic     r0, r0, #CR_A
#endif
#ifdef CONFIG_CPU_DCACHE_DISABLE
        bic     r0, r0, #CR_C
#endif
#ifdef CONFIG_CPU_BPREDICT_DISABLE
        bic     r0, r0, #CR_Z
#endif
#ifdef CONFIG_CPU_ICACHE_DISABLE
        bic     r0, r0, #CR_I
#endif
#ifdef CONFIG_ARM_LPAE
        mcrr    p15, 0, r4, r5, c2              @ load TTBR0
#else
        mov     r5, #DACR_INIT
        mcr     p15, 0, r5, c3, c0, 0           @ load domain access register
        mcr     p15, 0, r4, c2, c0, 0           @ load page table pointer
#endif
        b       __turn_mmu_on
ENDPROC(__enable_mmu)
{% endhighlight %}

__enable_mmu 主要任务就是打开 MMU，在这里主要就是加载页表和 domain 访问域。
在执行到 __enable_mmu 的时候，r0 寄存器存储着 SCTLR 寄存器需要写入的值；r1 寄存
器存储着 Mahine ID 信息；r2 寄存器存储着 ATAGS 或者 DTB 的地址；r4 存储着 TTBR0
将要写入的值；r5 存储着 TTBR1 将要写入的值；r9 寄存器存储着处理器相关的 ID；r13 寄
存器存储将要跳转到的位置。首先判断 CONFIG_ALIGNMENT_TRAP 宏和 __LINUX_ARM_ARCH__
是否小于 6，由于 ARMv7 __LINUX_ARM_ARCH__ 大于 6，因此执行 "bic r0, r0, #CR_A",
将 r0 寄存器 CR_A 对应的位清零，当写入 SCTLR 寄存时会影响
"bit[1] Alignment check enable", 不启用对齐检测。接着由于都没有定义宏
CONFIG_CPU_DCACHE_DISABLE，CONFIG_CPU_BPREDICT_DISABLE，CONFIG_CPU_ICACHE_DISABLE，
以及 CONFIG_ARM_LPAE，因此接下来执行的代码是 "mov r5, #DACR_INIT", DACR_INIT
定义如下：

{% highlight haskell %}
#ifdef CONFIG_CPU_SW_DOMAIN_PAN
#define DACR_INIT \
        (domain_val(DOMAIN_USER, DOMAIN_NOACCESS) | \
         domain_val(DOMAIN_KERNEL, DOMAIN_MANAGER) | \
         domain_val(DOMAIN_IO, DOMAIN_CLIENT) | \
         domain_val(DOMAIN_VECTORS, DOMAIN_CLIENT))
#else
#define DACR_INIT \
        (domain_val(DOMAIN_USER, DOMAIN_CLIENT) | \
         domain_val(DOMAIN_KERNEL, DOMAIN_MANAGER) | \
         domain_val(DOMAIN_IO, DOMAIN_CLIENT) | \
         domain_val(DOMAIN_VECTORS, DOMAIN_CLIENT))
#endif
{% endhighlight %}

由于 CONFIG_CPU_SW_DOMAIN_PAN 宏定义，所以 r5 寄存器存储了 DOMAIN_USER，
DOMAIN_NOACCESS，DOMAIN_KERNEL，DOMAIN_MANAGER，DOMAIN_IO，DOMAIN_CLIENT，
DOMAIN_VECTORS，DOMAIN_CLIENT。接着将 r5 寄存器通过代码
"mcr p15, 0, r5, c3, c0, 0" 写入了到寄存器，这里涉及到 CP15 C3 有关，其布局如下：

![MMU](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/boot/BOOTCP15C3.png)

通过代码选中了 DACR, Domain Access Control Register. 其寄存器布局如下：

![MMU](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/boot/BOOT000052.png)

向每个域中写入了访问权限。接着通过代码 "mcr p15, 0, r4, c2, c0, 0" 将 r4 寄存器
的值写入到 TTBR0 寄存器，也就是写入页表的基地址。注意此时写入的是物理地址，而不是
虚拟地址。开发者可以在适当的位置添加断点，然后使用断点调试，调试情况如下：

{% highlight haskell %}
5.0-arm32/linux/linux/vmlinux" at
	.text_addr = 0x60100000
	.head.text_addr = 0x60008000
	.rodata_addr = 0x60800000
(gdb) b BS_debug
Breakpoint 1 at 0x6010254c: file arch/arm/kernel/head.S, line 454.
(gdb) c
Continuing.

Breakpoint 1, __enable_mmu () at arch/arm/kernel/head.S:454
454		bic	r0, r0, #CR_A
(gdb) info reg r0 r1 r2 r4 r5 r9 r13
r0             0x10c5387d          281360509
r1             0x8e0               2272
r2             0x69cff000          1775235072
r4             0x60004059          1610629209
r5             0x601               1537
r9             0x410fc090          1091551376
r13            0x80a002e0          0x80a002e0 <__mmap_switched>
(gdb) n
468		mov	r5, #DACR_INIT
(gdb) n
469		mcr	p15, 0, r5, c3, c0, 0		@ load domain access register
(gdb) info reg r5
r5             0x51                81
(gdb) n
470		mcr	p15, 0, r4, c2, c0, 0		@ load page table pointer
(gdb) info reg r4
r4             0x60004059          1610629209
(gdb) n
472		b	__turn_mmu_on
(gdb)
{% endhighlight %}

调试情况和预期一致，接下来跳转到 __turn_mmu_on 处执行，代码如下：

{% highlight haskell %}
/*
 * Enable the MMU.  This completely changes the structure of the visible
 * memory space.  You will not be able to trace execution through this.
 * If you have an enquiry about this, *please* check the linux-arm-kernel
 * mailing list archives BEFORE sending another post to the list.
 *
 *  r0  = cp#15 control register
 *  r1  = machine ID
 *  r2  = atags or dtb pointer
 *  r9  = processor ID
 *  r13 = *virtual* address to jump to upon completion
 *
 * other registers depend on the function called upon completion
 */
        .align  5
        .pushsection    .idmap.text, "ax"
ENTRY(__turn_mmu_on)
        mov     r0, r0
        instr_sync
        mcr     p15, 0, r0, c1, c0, 0           @ write control reg
        mrc     p15, 0, r3, c0, c0, 0           @ read id reg
        instr_sync
        mov     r3, r3
        mov     r3, r13
        ret     r3
__turn_mmu_on_end:
ENDPROC(__turn_mmu_on)
        .popsection
{% endhighlight %}

这段代码的主要任务就是向 SCTLR 寄存器写入设置好的值，然后就可以启用 MMU。
代码首先执行了一条伪 nop 代码 "mov r0, r0", 然后调用 instr_sync 宏，这个宏
起到 isb 的作用，定义如下：

{% highlight haskell %}
/*
 * Instruction barrier
 */
        .macro  instr_sync
#if __LINUX_ARM_ARCH__ >= 7
        isb
#elif __LINUX_ARM_ARCH__ == 6
        mcr     p15, 0, r0, c7, c5, 4
#endif
        .endm
{% endhighlight %}

这里添加 isb 的作用就是在向 SCTLR 写入之前，让之前所有指令和流水线都同步完成之后
才执行 isb 之后的指令。接下来执行 "mcr p15, 0, r0, c1, c0, 0", 将之前配置好
的 SCTLR 内容都写到 SCTLR 寄存器里，然后读取 MIDR 寄存器，最后执行一条 isb 指令，
使刚刚设置的内容都生效。至此，MMU 启动，虚拟地址开始使用。接下来将 r13 寄存器
存储的虚拟地址赋值给 r3，然后直接跳转到 r3 对应的虚拟地址处执行。根据之前的实践
分析可以知道，r3 对应的地址是 __mmap_switched 的虚拟地址。开发者可以在适当的位置
添加断点，然后使用 GDB 进行调试，调试情况如下：

{% highlight haskell %}
5.0-arm32/linux/linux/vmlinux" at
	.text_addr = 0x60100000
	.head.text_addr = 0x60008000
	.rodata_addr = 0x60800000
(gdb) b BS_debug
Breakpoint 1 at 0x60100000: file arch/arm/kernel/head.S, line 492.
(gdb) c
Continuing.

Breakpoint 1, __turn_mmu_on () at arch/arm/kernel/head.S:492
492		mov	r0, r0
(gdb) n
493		instr_sync
(gdb) n
494		mcr	p15, 0, r0, c1, c0, 0		@ write control reg
(gdb) info reg r0
r0             0x10c5387d          281360509
(gdb) n
495		mrc	p15, 0, r3, c0, c0, 0		@ read id reg
(gdb) n
496		instr_sync
(gdb) info reg r3
r3             0x410fc090          1091551376
(gdb) n
497		mov	r3, r3
(gdb) n
498		mov	r3, r13
(gdb) n
499		ret	r3
(gdb) info reg r13
r13            0x80a002e0          0x80a002e0 <__mmap_switched>
(gdb) s
0x80a002e0 in __mmap_switched ()
(gdb) ni
{% endhighlight %}

通过上面的实践已经看到内核已经跳转到 __mmap_switched 处继续执行，那么接下来
执行的代码是：

<span id="__mmap_switched"></span>
{% highlight haskell %}
/*
 * The following fragment of code is executed with the MMU on in MMU mode,
 * and uses absolute addresses; this is not position independent.
 *
 *  r0  = cp#15 control register
 *  r1  = machine ID
 *  r2  = atags/dtb pointer
 *  r9  = processor ID
 */
        __INIT
__mmap_switched:
ENTRY(BS_debug)
        mov     r7, r1
        mov     r8, r2
        mov     r10, r0

        adr     r4, __mmap_switched_data
        mov     fp, #0
{% endhighlight %}

这里开始执行的代码 MMU 已经打开，所有使用的是虚拟地址，__mmap_switched 是为跳转到
start_kernel 做准备。在执行这段代码之前，r0 寄存器存储着 SCTLR 控制器的配置内容；
r1 寄存器存储着 machine ID；r2 寄存器存储着 DTB/ATAGS 的虚拟地址；r9 寄存存储着
处理器 ID。代码首先将 r1,r2,r0 寄存器的值赋值给 r7,r8,r9 寄存器，然后使用 adr 指令
获得 __mmap_switched_data 的虚拟地址，__mmap_switched_data 的定义如下：

{% highlight haskell %}
        .align  2
        .type   __mmap_switched_data, %object
__mmap_switched_data:
        .long   __bss_start                     @ r0
        .long   __bss_stop                      @ r1
        .long   init_thread_union + THREAD_START_SP @ sp

        .long   processor_id                    @ r0
        .long   __machine_arch_type             @ r1
        .long   __atags_pointer                 @ r2
        .long   cr_alignment                    @ r3
        .size   __mmap_switched_data, . - __mmap_switched_data
{% endhighlight %}

在 __mmap_switched_data 数据中，__bss_start 指向 .bss 段的起始地址；__bss_stop
指向 .bss 段的终止地址；init_thread_union 是 init_task 使用其描述的内存区域作为
该进程的堆栈空间，并且和自身的 thread_info 参数共用这部分内存空间。processor_id
存储着处理器 ID；__machine_arch_type 变量用于存储 machine type; __atags_pointer
指向 ATAGS 或者 DTB 的指针， cr_alignment 对齐信息。在回到 __mmap_switched，
继续执行代码 "mov fp, #0" 将 fp 寄存器设置为 0。开发者可以在适当的位置添加断点，
使用 GDB 进行调试，调试情况如下：

{% highlight haskell %}
5.0-arm32/linux/linux/vmlinux" at
	.text_addr = 0x80100000
	.head.text_addr = 0x80008000
	.rodata_addr = 0x80800000
	.init.text_addr = 0x80a002e0
(gdb) b BS_debug
Breakpoint 1 at 0x80a002e0: file arch/arm/kernel/head-common.S, line 83.
(gdb) c
Continuing.

Breakpoint 1, __mmap_switched () at arch/arm/kernel/head-common.S:83
83		mov	r7, r1
(gdb) n
84		mov	r8, r2
(gdb) n
85		mov	r10, r0
(gdb) n
87		adr	r4, __mmap_switched_data
(gdb) info reg r0 r1 r2 r7 r8 r9 fp
r0             0x10c5387d          281360509
r1             0x8e0               2272
r2             0x69cff000          1775235072
r7             0x8e0               2272
r8             0x69cff000          1775235072
r9             0x410fc090          1091551376
fp             0x80a002e0          0x80a002e0 <__mmap_switched>
(gdb) n
88		mov	fp, #0
(gdb) n
105	   ARM(	ldmia	r4!, {r0, r1, sp} )
(gdb) info reg r4 fp
r4             0x80a00324          -2136997084
fp             0x80a002e0          0x80a002e0 <__mmap_switched>
(gdb)
{% endhighlight %}

调试情况和预期一致，接下来执行的代码如下：

{% highlight haskell %}
   ARM( ldmia   r4!, {r0, r1, sp} )
 THUMB( ldmia   r4!, {r0, r1, r3} )
 THUMB( mov     sp, r3 )
        sub     r2, r1, r0
        mov     r1, #0
        bl      memset                          @ clear .bss
{% endhighlight %}

这段的代码主要作用就是清除内核的 .bss 段。调用 "ldmia r4!, {r0, r1, sp}" 从
__mmap_switched_data 中获得数据，并将 __bss_start 存储到 r0 寄存器，将
__bss_stop 存储到 r1 寄存器，将 init_thread_union + THREAD_START_SP
存储到 sp 寄存器，然后将 r1 寄存器减去 r0 寄存器可以获得 .bss 段的长度，
然后将 r1 设置为 0， 然后跳转到 memset 进行清除动作，对于 memset 参数，
r0 对应需要清除的起始地址；r1 代表设置的初始值；r2 代表清除字节数 ，接着调用
arch/arm/lib/memset.S 中的 memset 进行清除动作。开发者可以在适当的位置
加入断点，然后使用 GDB 进行调试，调试的结果如下：

{% highlight haskell %}
5.0-arm32/linux/linux/vmlinux" at
	.text_addr = 0x80100000
	.head.text_addr = 0x80008000
	.rodata_addr = 0x80800000
	.init.text_addr = 0x80a002e0
(gdb) b BS_debug
Breakpoint 1 at 0x80a002f4: file arch/arm/kernel/head-common.S, line 105.
(gdb) c
Continuing.

Breakpoint 1, __mmap_switched () at arch/arm/kernel/head-common.S:105
105	   ARM(	ldmia	r4!, {r0, r1, sp} )
(gdb) n
__mmap_switched () at arch/arm/kernel/head-common.S:108
108		sub	r2, r1, r0
(gdb) n
109		mov	r1, #0
(gdb) info reg r0 r1 r2
r0             0x80b69128          -2135518936
r1             0x80b90998          -2135357032
r2             0x27870             161904
(gdb) x/16x 0x80b69128
0x80b69128:	0x00000000	0x00000000	0x00000000	0x00000000
0x80b69138:	0x00000000	0x00000000	0x00000000	0x00000000
0x80b69148 <ramdisk_execute_command>:	0x00000000	0x00000000	0x00000000	0x00000000
0x80b69158 <initcall_command_line>:	0x00000000	0x00000000	0x00000000	0x00000000
(gdb) b BS_IO
Breakpoint 2 at 0x80a00304: file arch/arm/kernel/head-common.S, line 113.
(gdb) c
Continuing.

Breakpoint 2, __mmap_switched () at arch/arm/kernel/head-common.S:113
113		ldmia	r4, {r0, r1, r2, r3}
(gdb) x/16x 0x80b69128
0x80b69128:	0x00000000	0x00000000	0x00000000	0x00000000
0x80b69138:	0x00000000	0x00000000	0x00000000	0x00000000
0x80b69148 <ramdisk_execute_command>:	0x00000000	0x00000000	0x00000000	0x00000000
0x80b69158 <initcall_command_line>:	0x00000000	0x00000000	0x00000000	0x00000000
(gdb) quit
{% endhighlight %}

接下来执行的代码是：

<span id="start_kernel_last"></span>
{% highlight haskell %}
        ldmia   r4, {r0, r1, r2, r3}
        str     r9, [r0]                        @ Save processor ID
        str     r7, [r1]                        @ Save machine type
        str     r8, [r2]                        @ Save atags pointer
        cmp     r3, #0
        strne   r10, [r3]                       @ Save control register values
        mov     lr, #0
        b       start_kernel
ENDPROC(__mmap_switched)
{% endhighlight %}

这段代码就是跳转到 start_kernel 之前最后一段代码。首先调用 ldmia 指令获得
__mmap_switched_data 数据中对应的内容，然后将处理器 ID，机器类型，atags/DTB 信息
写入到 __mmap_switched_data 指定的位置。最后将确认 r3 寄存器对应的位置是否存在，
如果存在，则将 r10 寄存器的值写入里面，此时 r10 寄存器存储 SCTLR 寄存器的配置。
最后将 lr 寄存器设置为 0 后，跳转到 start_kernel 处继续执行。开发者可以在适当
位置添加断点，然后使用 GDB 进行调试，调试情况如下：

{% highlight haskell %}
5.0-arm32/linux/linux/vmlinux" at
	.text_addr = 0x80100000
	.head.text_addr = 0x80008000
	.rodata_addr = 0x80800000
	.init.text_addr = 0x80a002e0
(gdb) b BS_debug
Breakpoint 1 at 0x80a00304: file arch/arm/kernel/head-common.S, line 112.
(gdb) c
Continuing.

Breakpoint 1, __mmap_switched () at arch/arm/kernel/head-common.S:112
112		ldmia	r4, {r0, r1, r2, r3}
(gdb) n
113		str	r9, [r0]			@ Save processor ID
(gdb) n
114		str	r7, [r1]			@ Save machine type
(gdb) n
115		str	r8, [r2]			@ Save atags pointer
(gdb) n
116		cmp	r3, #0
(gdb) info reg r0 r1 r2 r3 r7 r8 r9 r10
r0             0x80b69554          -2135517868
r1             0x80b08c1c          -2135913444
r2             0x80a54a38          -2136651208
r3             0x80b0cd94          -2135896684
r7             0x8e0               2272
r8             0x69cff000          1775235072
r9             0x410fc090          1091551376
r10            0x10c5387d          281360509
(gdb) n
117		strne	r10, [r3]			@ Save control register values
(gdb) n
118		mov	lr, #0
(gdb) n
119		b	start_kernel
(gdb) n
start_kernel () at init/main.c:538
538	{
(gdb) list
533	{
534		rest_init();
535	}
536
537	asmlinkage __visible void __init start_kernel(void)
538	{
539		char *command_line;
540		char *after_dashes;
541
542		set_task_stack_end_magic(&init_task);
(gdb)
{% endhighlight %}

通过上面实践，可以看到最后调用 start_kernel 的情况。至此，kernel 汇编基础部分
初始化已经结束。接下来将进入 C 函数 start_kernel 继续执行代码。

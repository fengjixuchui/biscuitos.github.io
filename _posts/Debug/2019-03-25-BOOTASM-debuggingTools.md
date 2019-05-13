---
layout:             post
title:              "ARM Debugging Usermanual"
date:               2019-03-25 17:28:30 +0800
categories:         [MMU]
excerpt:            ARM Debugging Usermanual.
tags:
  - MMU
---

![LINUXP](https://raw.githubusercontent.com/EmulateSpace/PictureSet/master/BiscuitOS/kernel/GIF000200.gif)

> [GitHub BiscuitOS](https://github.com/BiscuitOS/BiscuitOS)
>
> Email: BuddyZhang1 <buddy.zhang@aliyun.com>

# 目录

> - [ARM zImage 重定位前 gdb 调试方法](#ARM Boot-Stage1)
>
> - [ARM zImage 重定位后 gdb 调试方法](#ARM Boot-Stage2)
>
> - [内核解压后 (MMU OFF) start_kernel 之前 gdb 调试方法](#Linux_decompress_before)
>
> - [内核解压后 (MMU ON) start_kernel 之前 gdb 调试方法](#Linux_decompress_before2)
>
> - [内核解压后 start_kernel 之后 gdb 调试方法](#Linux_decompress_after)
>
> - [动态模块 gdb 调试方法](#MODULE_GDB)
>
> - [附录](#附录)

--------------------------------------------------------------
<span id="ARM Boot-Stage1"></span>

![MMU](https://raw.githubusercontent.com/EmulateSpace/PictureSet/master/BiscuitOS/kernel/IND00000A.jpg)

# ARM zImage 重定位前 gdb 调试方法

Linux 内核源码经过编译链接生成 ELF 目标文件 vmlinux，vmlinux 使用 OBJCOPY
工具去除不必要的段之后，生成 Image 二进制文件。Image 经过压缩之后生成 piggy_data,
在把 piggy_data 压缩文件放到汇编文件 piggy.s 里面经过汇编生成 piggy.o，该目标文件
与 arch/arm/boot/compressed 目录下的多个目标文件链接生成 vmlinux ELF 文件，这里
的 vmlinux 与 Linux 源码编译链接生成的 vmlinux 不是同一个。这个阶段的 vmlinux
是带了 bootstrap 的 vmlinux，vmlinux 在经过 OBJCOPY 工具之后，生成二进制文件
zImage，这个 zImage 是可以直接加载到内存直接运行的。其原理如下图：

![MMU](https://raw.githubusercontent.com/EmulateSpace/PictureSet/master/BiscuitOS/boot/BOOT000036.png)

正如上图，Uboot 将 zImage 加载到内存之后，zImage 就开始运行 Linux 早期代码，
这个阶段，zImage 主要任务就是将压缩的内核解压到制定的位置，然后将运行权转交给
解压出来的内核。由于 zImage 运行的范围会与解压之后的内核存在重叠，所以在 zImage
完成简单的初始化之后，就重定位 zImage 到安全的位置继续完成解压内核的任务。在解压
内核之前就会存在两个阶段，第一个阶段就是 zImage 重定位之前；第二个阶段就是 zImage
从定位之后，真正内核运行之前。两个阶段 GDB 调试存在一定的差异，因此将这两个阶段
独立拆开进行介绍。本节主要介绍 zImage 重定位之前，GDB 调试方法。
本文基于 ARM 32 Linux 5.0 进行讲解，如果还没有搭建 Linux 5.0 ，可以参考下面教程：

> [Linux 5.0 arm 32 开发环境搭建手册](https://biscuitos.github.io/blog/Linux-5.0-arm32-Usermanual/)

搭建完上面的教程之后，参考 BiscuitOS/output/linux-5.0-arm32/README.md ,其中
关于 zImage 重定位之前的调试介绍如下：

{% highlight base %}
# Debugging zImage before relocated

### First Terminal

```
cd BiscuitOS/output/linux-5.0-arm32
./RunQemuKernel.sh debug
```

### Second Terminal

```
BiscuitOS/output/linux-5.0-arm32/arm-linux-gnueabi/arm-linux-gnueabi/bin/arm-linux-gnueabi-gdb -x BiscuitOS/output/linux-5.0-arm32/package/gdb/gdb_zImage

(gdb) b XXX_bk
(gdb) c
(gdb) info reg
```
{% endhighlight %}

根据上面的介绍，开发者首先打开一个终端，在中断中输入如下命令：

{% highlight bash %}
cd BiscuitOS/output/linux-5.0-arm32
./RunQemuKernel.sh debug
{% endhighlight %}

然后再打开第二个终端，第二个终端中输入如下命令：

{% highlight bash %}
BiscuitOS/output/linux-5.0-arm32/arm-linux-gnueabi/arm-linux-gnueabi/bin/arm-linux-gnueabi-gdb -x BiscuitOS/output/linux-5.0-arm32/package/gdb/gdb_zImage
{% endhighlight %}

此时第二个终端进入了 GDB 模式，开发者此时输入如下命令进行调试：

{% highlight bash %}
(gdb) b XXX_bk
(gdb) c
(gdb) info reg
{% endhighlight %}

其中 XXX_bk 是断点的名字。运行如下：

![MMU](https://raw.githubusercontent.com/EmulateSpace/PictureSet/master/BiscuitOS/boot/BOOT000066.png)

#### 打断点

在实际调试过程中需要对不同的代码段打断点，以此提高调试效率。在 zImage 重定位之前的阶段
打断点请参考如下步骤：

zImage 重定位之前的阶段的代码大多位于 arch/arm/boot/compressed/ 目录下，其中这个阶段
的入口函数位于 arch/arm/boot/compressed/head.S 里面，如下：

{% highlight bash %}
 AR_CLASS(      .arm    )
start:
                .type   start,#function
                .rept   7
                __nop
                .endr
#ifndef CONFIG_THUMB2_KERNEL
                mov     r0, r0
#else
 AR_CLASS(      sub     pc, pc, #3      )       @ A/R: switch to Thumb2 mode
  M_CLASS(      nop.w                   )       @ M: already in Thumb2 mode
                .thumb
#endif
                W(b)    1f
{% endhighlight %}

这上面的函数中，开发者可以使用 ENTRY() 宏来添加一个断点，例如：

{% highlight base %}
 AR_CLASS(      .arm    )
start:
                .type   start,#function
                .rept   7
                __nop
                .endr
ENTRY(BS_debug)
#ifndef CONFIG_THUMB2_KERNEL
                mov     r0, r0
#else
 AR_CLASS(      sub     pc, pc, #3      )       @ A/R: switch to Thumb2 mode
  M_CLASS(      nop.w                   )       @ M: already in Thumb2 mode
                .thumb
#endif
                W(b)    1f
{% endhighlight %}

在上面的代码中，添加了一个名为 BS_debug 的标签，可以再 GDB 中利用这个标签打
断点。调试方法如下所述，在进入 GDB 模式后，使用如下命令：

{% highlight bash %}
(gdb) b BS_debug
(gdb) c
(gdb) info reg
{% endhighlight %}

实际运行情况如下图：

![MMU](https://raw.githubusercontent.com/EmulateSpace/PictureSet/master/BiscuitOS/kernel/ASM000001.png)

##### 拓展

由于 zImage 阶段的汇编代码调试都需要在 GDB 中进行符号表的重定位，BiscuitOS 在该阶段
默认使用的 .gdbinit 脚本位于 BiscuitOS/output/linux-5.0-arm32/package/gdb/gdb_zImage,
其内容如下：

{% highlight bash %}
# Debug zImage after relocated zImage
#
# (C) 2019.04.15 BuddyZhang1 <buddy.zhang@aliyun.com>
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License version 2 as
# published by the Free Software Foundation.

# Remote to gdb
#
target remote :1234

# Reload vmlinux for zImage
#
add-symbol-file BiscuitOS/output/linux-5.0-arm32/linux/linux/arch/arm/boot/compressed/vmlinux 0x60010000
{% endhighlight %}

上面脚本中使用了 gdb 的 add-symbol-file 动态加载了 vmlinux 的符号表，但后面的数值
是根据实际 vmlinux 加载偏移所设置的。

--------------------------------------------------------------
<span id="ARM Boot-Stage2"></span>

![MMU](https://raw.githubusercontent.com/EmulateSpace/PictureSet/master/BiscuitOS/kernel/IND00000Q.jpg)

# ARM zImage 重定位后 gdb 调试方法

zImage 完成基本的初始化之后，由于 zImage 的运行地址空间和解压之后的内核运行空间
存在重叠的位置，因此需要将 zImage 整体拷贝到一个安全的地址上运行，这里成为 zImage
的重定位。拷贝工作在 zImage 初始化阶段已经完成，本节所讨论的阶段是 zImage 从重
定位的地址继续运行，继续完成内核解压的任务。由于重定位之后，zImage 的符号表需要
重新加载，因此对该阶段的调试，请参考本节。本节的所有内容都是基于 Linux 5.0 进行
讲解的，如果还未搭建 Linux 5.0 开发环境，请参看如下教程：

> [Linux 5.0 arm 32 开发环境搭建手册](https://biscuitos.github.io/blog/Linux-5.0-arm32-Usermanual/)

搭建完上面的教程之后，参考 BiscuitOS/output/linux-5.0-arm32/README.md ,其中
关于 zImage 重定位之后的调试介绍如下：

{% highlight bash %}
# Debugging zImage After Relocated

### First Terminal

```
cd BiscuitOS/output/linux-5.0-arm32
./RunQemuKernel.sh debug
```

### Second Terminal

```
BiscuitOS/output/linux-5.0-arm32/arm-linux-gnueabi/arm-linux-gnueabi/bin/arm-linux-gnueabi-gdb -x /BiscuitOS/output/linux-5.0-arm32/package/gdb/gdb_RzImage

(gdb) b XXX_bk
(gdb) c
(gdb) info reg
```
{% endhighlight %}

根据上面的介绍，开发者首先打开一个终端，在中断中输入如下命令：

{% highlight base %}
cd BiscuitOS/output/linux-5.0-arm32
./RunQemuKernel.sh debug
{% endhighlight %}

然后再打开第二个终端，第二个终端中输入如下命令：

{% highlight base %}
BiscuitOS/output/linux-5.0-arm32/arm-linux-gnueabi/arm-linux-gnueabi/bin/arm-linux-gnueabi-gdb -x /BiscuitOS/output/linux-5.0-arm32/package/gdb/gdb_RzImage
{% endhighlight %}

此时第二个终端进入了 GDB 模式，开发者此时输入如下命令进行调试：

{% highlight base %}
(gdb) b XXX_bk
(gdb) c
(gdb) info reg
{% endhighlight %}

其中 XXX_bk 是断点的名字。运行如下：

![MMU](https://raw.githubusercontent.com/EmulateSpace/PictureSet/master/BiscuitOS/boot/BOOT000031.png)

#### 打断点

在实际调试过程中需要对不同的代码段打断点，以此提高调试效率。在 zImage 重定位之后的阶段
打断点请参考如下步骤：

zImage 重定位之后的阶段的代码大多位于 arch/arm/boot/compressed/ 目录下，其中这个阶段
的入口函数位于 arch/arm/boot/compressed/head.S 里面，且重定位的代码从 head.S 的
restart 之后执行。如下：

{% highlight bash %}
restart:        adr     r0, LC0
                ldmia   r0, {r1, r2, r3, r6, r10, r11, r12}
                ldr     sp, [r0, #28]

                /*
                 * We might be running at a different address. We need
                 * to fix up various pointers.
                 */
                sub     r0, r0, r1              @ caclculate the delta offset
                add     r6, r6, r0              @ _edata
                add     r10, r10, r0            @ inflated kernel size location
{% endhighlight %}

这上面的函数中，开发者可以使用 ENTRY() 宏来添加一个断点，例如：

{% highlight bash %}
restart:        adr     r0, LC0
ENTRY(BS_debug)
                ldmia   r0, {r1, r2, r3, r6, r10, r11, r12}
                ldr     sp, [r0, #28]

                /*
                 * We might be running at a different address. We need
                 * to fix up various pointers.
                 */
                sub     r0, r0, r1              @ caclculate the delta offset
                add     r6, r6, r0              @ _edata
                add     r10, r10, r0            @ inflated kernel size location
{% endhighlight %}

在上面的代码中，添加了一个名为 BS_debug 的标签，可以再 GDB 中利用这个标签打
断点。调试方法如下所述，在进入 GDB 模式后，使用如下命令：

{% highlight bash %}
(gdb) b BS_debug
(gdb) c
(gdb) info reg
{% endhighlight %}

实际运行情况如下图：

![MMU](https://raw.githubusercontent.com/EmulateSpace/PictureSet/master/BiscuitOS/boot/BOOT000031.png)

##### 拓展

由于 zImage 阶段的汇编代码调试都需要在 GDB 中进行符号表的重定位，BiscuitOS 在该阶段
默认使用的 .gdbinit 脚本位于 BiscuitOS/output/linux-5.0-arm32/package/gdb/gdb_RzImage,
其内容如下：

{% highlight bash %}
# Debug zImage after relocated zImage
#
# (C) 2019.04.15 BuddyZhang1 <buddy.zhang@aliyun.com>
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License version 2 as
# published by the Free Software Foundation.

# Remote to gdb
#
target remote :1234

# Reload vmlinux for zImage
#
add-symbol-file BiscuitOS/output/linux-5.0-arm32/linux/linux/arch/arm/boot/compressed/vmlinux 0x60b6951c
{% endhighlight %}

上面脚本中使用了 gdb 的 add-symbol-file 动态加载了 vmlinux 的符号表，但后面的数值
是根据实际情况动态计算出来的，开发者不同担心这些问题，BiscuitOS 使用了如下脚本自动
计算加载地址 (脚本位于 BiscuitOS/output/linux-5.0-arm32/package/gdb/gdb.pl)：

{% highlight bash %}
#!/usr/bin/perl

# GDB helper scripts.
#
# (C) 2019.04.15 BiscuitOS <buddy.zhang@aliyun.com>
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License version 2 as
# published by the Free Software Foundation.

use strict;

my $ROOT=shift;
my $CROSS_COMP=shift;

## Default file
my $GDB_FILE="$ROOT/package/gdb/gdb_vmlinux_obj";
my $VM_FILE="$ROOT/linux/linux/arch/arm/boot/compressed/vmlinux";
my $CROSS_TOOLS="$ROOT/$CROSS_COMP/$CROSS_COMP/bin/$CROSS_COMP";

## Auto objdump vmlinux file
`$CROSS_TOOLS-objdump -x $VM_FILE > $GDB_FILE`;

my $restart=hex(`cat $GDB_FILE | grep " restart" | awk '{print \$1}'`);
my $Image=`ls -l $ROOT/linux/linux/arch/arm/boot/Image | awk '{print \$5}'`;
my $kernel_start=0x60008000;
my $load=0x60010000;
my $reloc=hex(`cat $GDB_FILE | grep "reloc_code_end" | awk '{print \$1}'`);
my $end=hex(`cat $GDB_FILE | grep " _end" | awk '{print \$1}'`);

## Remove tmpfile
`rm $GDB_FILE`;

my ($r5, $r9, $r6, $r10);
my $size;
my $final;

# Kernel end execute address
$r10 = $kernel_start + $Image;
# Protect area for relocated
$r10 = ($r10 + (($reloc - $restart + 256) & 0xFFFFFF00)) & 0xffffff00;

# Runnung address for restart.
$r5 = $restart + $load;
$r5 &= 0xffffffE0;

# Running address for end address of zImage
$r6 = $end + $load;

# Size to Relocate for zImage
$r9 = $r6 - $r5;
$r9 += 31;
$r9 &= 0xffffffe0;
$size = $r9;

# End address of zImage
$r6 = $r9 + $r5;
# end relocated address of zImage
$r9 += $r10;

# Start address for Relocated zImage
$r9 -= $size;
# Start address of zImage
$r6 = $r5;

# Relocated size
$size = $r9 - $r6;

# Call Relocated address for zIamge
$final = $size + $restart + $load;

printf "%#x\n", $final;
{% endhighlight %}

感兴趣的开发者可以自行研究脚本的原理。

--------------------------------------------------------------
<span id="Linux_decompress_before"></span>

![MMU](https://raw.githubusercontent.com/EmulateSpace/PictureSet/master/BiscuitOS/kernel/IND00000F.jpg)

# 内核解压后 (MMU OFF) start_kernel 之前 gdb 调试方法

zImage 将压缩的内核解压到指定位置之后，然后将 CPU 的执行权移交给解压之后的内核。内核
获得 CPU 之后，就开始真正的初始化内核，由于此时 MMU 并未开启，内核没有将内存映射
到 vmlinux 的链接地址，因此在运行到 start_kernel 之前，需要使用特殊的方法调试
这个阶段的代码，本节用于介绍如何调试这个阶段的代码。本节的所有内容都是基于 Linux 5.0
进行讲解的，如果还未搭建 Linux 5.0 开发环境，请参看如下教程：

> [Linux 5.0 arm 32 开发环境搭建手册](https://biscuitos.github.io/blog/Linux-5.0-arm32-Usermanual/)

搭建完上面的教程之后，参考 BiscuitOS/output/linux-5.0-arm32/README.md ,其中
关于 Image 解压运行开始到 start_kernel 之前的调试介绍如下：

{% highlight bash %}
# Debugging kernel MMU OFF before start_kernel

### First Terminal

```
cd BiscuitOS/output/linux-5.0-arm32
./RunQemuKernel.sh debug
```

### Second Terminal

```
BiscuitOS/output/linux-5.0-arm32/arm-linux-gnueabi/arm-linux-gnueabi/bin/arm-linux-gnueabi-gdb -x BiscuitOS/output/linux-5.0-arm32/package/gdb/gdb_Image

(gdb) b XXX_bk
(gdb) c
(gdb) info reg
```
{% endhighlight %}

根据上面的介绍，开发者首先打开一个终端，在中断中输入如下命令：

{% highlight base %}
cd BiscuitOS/output/linux-5.0-arm32
./RunQemuKernel.sh debug
{% endhighlight %}

然后再打开第二个终端，第二个终端中输入如下命令：

{% highlight base %}
BiscuitOS/output/linux-5.0-arm32/arm-linux-gnueabi/arm-linux-gnueabi/bin/arm-linux-gnueabi-gdb -x BiscuitOS/output/linux-5.0-arm32/package/gdb/gdb_Image
{% endhighlight %}

此时第二个终端进入了 GDB 模式，开发者此时输入如下命令进行调试：

{% highlight base %}
(gdb) b BS_debug
(gdb) c
(gdb) info reg
{% endhighlight %}

其中 BS_debug 是断点的名字。运行如下：

![MMU](https://raw.githubusercontent.com/EmulateSpace/PictureSet/master/BiscuitOS/boot/BOOT000037.png)

#### 打断点

在实际调试过程中需要对不同的代码段打断点，以此提高调试效率。在 Image 初始化的阶段
打断点请参考如下步骤：

Image 初始化阶段的代码大多位于 arch/arm/kernel/ 目录下，其中这个阶段
的入口函数位于 arch/arm/kernel/head.S 里面，且 Image 初始化的代码从 head.S 的
ENTRY(stext) 之后执行。如下：

{% highlight bash %}
        .arm

        __HEAD
ENTRY(stext)
 ARM_BE8(setend be )                    @ ensure we are in BE8 mode

 THUMB( badr    r9, 1f          )       @ Kernel is always entered in ARM.
 THUMB( bx      r9              )       @ If this is a Thumb-2 kernel,
 THUMB( .thumb                  )       @ switch to Thumb now.
 THUMB(1:                       )

#ifdef CONFIG_ARM_VIRT_EXT
        bl      __hyp_stub_install
#endif
        @ ensure svc mode and all interrupts masked
        safe_svcmode_maskall r9

        mrc     p15, 0, r9, c0, c0              @ get processor id
        bl      __lookup_processor_type         @ r5=procinfo r9=cpuid
        movs    r10, r5                         @ invalid processor (r5=0)?
 THUMB( it      eq )            @ force fixup-able long branch encoding
        beq     __error_p                       @ yes, error 'p'
{% endhighlight %}

在上面的函数中，开发者可以使用 ENTRY() 宏来添加一个断点，例如：

{% highlight bash %}
        .arm

        __HEAD
ENTRY(stext)
 ARM_BE8(setend be )                    @ ensure we are in BE8 mode

 THUMB( badr    r9, 1f          )       @ Kernel is always entered in ARM.
 THUMB( bx      r9              )       @ If this is a Thumb-2 kernel,
 THUMB( .thumb                  )       @ switch to Thumb now.
 THUMB(1:                       )

ENTRY(BS_debug)
#ifdef CONFIG_ARM_VIRT_EXT
        bl      __hyp_stub_install
#endif
        @ ensure svc mode and all interrupts masked
        safe_svcmode_maskall r9

        mrc     p15, 0, r9, c0, c0              @ get processor id
        bl      __lookup_processor_type         @ r5=procinfo r9=cpuid
        movs    r10, r5                         @ invalid processor (r5=0)?
 THUMB( it      eq )            @ force fixup-able long branch encoding
        beq     __error_p                       @ yes, error 'p'n
{% endhighlight %}

在上面的代码中，添加了一个名为 BS_debug 的标签，可以再 GDB 中利用这个标签打
断点。调试方法如下所述，在进入 GDB 模式后，使用如下命令：

{% highlight bash %}
(gdb) b BS_debug
(gdb) c
(gdb) list
{% endhighlight %}

实际运行情况如下图：

![MMU](https://raw.githubusercontent.com/EmulateSpace/PictureSet/master/BiscuitOS/boot/BOOT000037.png)

##### 拓展

由于 Image 初始化阶段，MMU 尚未开启，所有内存地址还没有与 vmlinux 链接地址关联，因此
该阶段使用 GDB 调试需要重定位符号表。 BiscuitOS 在该阶段
默认使用的 .gdbinit 脚本位于 BiscuitOS/output/linux-5.0-arm32/package/gdb/gdb_Image,
其内容如下：

{% highlight bash %}
# Debug Image before start_kernel
#
# (C) 2019.04.11 BuddyZhang1 <buddy.zhang@aliyun.com>
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License version 2 as
# published by the Free Software Foundation.

# Remote to gdb
#
target remote :1234

# Reload vmlinux for zImage
#
add-symbol-file BiscuitOS/output/linux-5.0-arm32/linux/linux/vmlinux 0x60100000 -s .head.text 0x60008000 -s .rodata 0x60800000
{% endhighlight %}

上面脚本中使用了 gdb 的 add-symbol-file 动态加载了 vmlinux 的符号表，但后面的数值
是根据实际情况动态计算出来的，其计算的基本思路是：Image 从压缩文件解压到 0x60008000
的位置开始执行，此时是 Image 最前端的位置，也就是 .head.text 的位置。开发者可以按
如下步骤进行计算。首先使用平台对应的 readelf 文件获得 vmlinux 的符号表，具体命令
如下：

{% highlight bash %}
BiscuitOS/output/linux-5.0-arm32/arm-linux-gnueabi/arm-linux-gnueabi/bin/arm-linux-gnueabi-readelf -S BiscuitOS/output/linux-5.0-arm32/linux/linux/vmlinux
{% endhighlight %}

获得数据如下：

![MMU](https://raw.githubusercontent.com/EmulateSpace/PictureSet/master/BiscuitOS/boot/BOOT000038.png)

从上面的数据可知，.head.text Addr 项对应的地址是 80008000, 但由于 Image 开始执行
地址是 0x60008000, 因此 GDB 使用 add-symbol-file 重定位 vmlinux 的时候，需要使用
-s 选项重新指定 .head.text section 的地址是 0x60008000; 同理，.rodata 的 Addr
项是 80800000, 因此需要重新指定 .rodata section 的地址是 0x60800000. 最后，也是
最关键的，在 GDB 中使用 add-symbol-file 命令重定位 vmlinux 的地址，这个地址就是
vmlinux ELF 文件的 .text section 的地址，从图中可以看出，.text Addr 项是
0x80100000, 因此调整之后的地址就是 0x60100000, 通过这样的调整之后，vmlinux
符号表就重定位到 Image 对应的位置上了。

BiscuitOS 已经自动生成 gdb_Image 文件，开发者只需按照教程 README 提示的步骤，
就可以简单完成对该阶段代码的调试。

--------------------------------------------------------------
<span id="Linux_decompress_before2"></span>

![MMU](https://raw.githubusercontent.com/EmulateSpace/PictureSet/master/BiscuitOS/kernel/IND00000F.jpg)

# 内核解压后 (MMU ON) start_kernel 之前 gdb 调试方法

Image 已经进行基础的初始化，但 MMU 并未开启，所以使用的都是物理地址。但 Image
初始化到后期，页表等寄存器设置完毕之后，内核启用 MMU 之后，开始使用虚拟地址，但
按 1:1 仅仅映射了内核镜像到 .bss 段的地址，其他地址并未映射。在启用 MMU 之后，
地址发生改变，所以之前重定位的符号表此处需要重新进行定位，因此从 MMU 启用到
start_kernel 这个阶段的调试需要按如下的步骤。本节的所有内容都是基于 Linux 5.0
进行讲解的，如果还未搭建 Linux 5.0 开发环境，请参看如下教程：

> [Linux 5.0 arm 32 开发环境搭建手册](https://biscuitos.github.io/blog/Linux-5.0-arm32-Usermanual/)

搭建完上面的教程之后，参考 BiscuitOS/output/linux-5.0-arm32/README.md ,其中
关于 Image 解压运行开始 (MMU ON) 到 start_kernel 之前的调试介绍如下：

{% highlight bash %}
# Debugging kernel MMU ON before start_kernel

### First Terminal

```
cd BiscuitOS/output/linux-5.0-arm32
./RunQemuKernel.sh debug
```

### Second Terminal

```
BiscuitOS/output/linux-5.0-arm32/arm-linux-gnueabi/arm-linux-gnueabi/bin/arm-linux-gnueabi-gdb -x BiscuitOS/output/linux-5.0-arm32/package/gdb/gdb_RImage

(gdb) b XXX_bk
(gdb) c
(gdb) info reg
```
{% endhighlight %}

根据上面的介绍，开发者首先打开一个终端，在中断中输入如下命令：

{% highlight base %}
cd BiscuitOS/output/linux-5.0-arm32
./RunQemuKernel.sh debug
{% endhighlight %}

然后再打开第二个终端，第二个终端中输入如下命令：

{% highlight base %}
BiscuitOS/output/linux-5.0-arm32/arm-linux-gnueabi/arm-linux-gnueabi/bin/arm-linux-gnueabi-gdb -x BiscuitOS/output/linux-5.0-arm32/package/gdb/gdb_RImage
{% endhighlight %}

此时第二个终端进入了 GDB 模式，开发者此时输入如下命令进行调试：

{% highlight base %}
(gdb) b BS_debug
(gdb) c
(gdb) info reg
{% endhighlight %}

其中 BS_debug 是断点的名字。运行如下：

![MMU](https://raw.githubusercontent.com/EmulateSpace/PictureSet/master/BiscuitOS/boot/BOOT000053.png)

#### 打断点

在实际调试过程中需要对不同的代码段打断点，以此提高调试效率。在 Image 初始化的阶段
打断点请参考如下步骤：

Image 初始化阶段 (MMU ON) 的代码大多位于 arch/arm/kernel/ 目录下，其中这个阶段
的入口函数位于 arch/arm/kernel/head.S 里面。如下：

{% highlight bash %}
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
        mov     r7, r1
        mov     r8, r2
        mov     r10, r0

        adr     r4, __mmap_switched_data
        mov     fp, #0
{% endhighlight %}

在上面的函数中，开发者可以使用 ENTRY() 宏来添加一个断点，例如：

{% highlight bash %}
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

在上面的代码中，添加了一个名为 BS_debug 的标签，可以再 GDB 中利用这个标签打
断点。调试方法如下所述，在进入 GDB 模式后，使用如下命令：

{% highlight bash %}
(gdb) b BS_debug
(gdb) c
(gdb) list
{% endhighlight %}

实际运行情况如下图：

![MMU](https://raw.githubusercontent.com/EmulateSpace/PictureSet/master/BiscuitOS/boot/BOOT000053.png)

##### 拓展

由于 Image 初始化阶段，MMU 已经开启，需要将内核符号表加载到指定位置。 BiscuitOS 在该阶段
默认使用的 .gdbinit 脚本位于 BiscuitOS/output/linux-5.0-arm32/package/gdb/gdb_RImage,
其内容如下：

{% highlight bash %}
# Debug Image MMU on before start_kernel
#
# (C) 2019.04.11 BuddyZhang1 <buddy.zhang@aliyun.com>
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License version 2 as
# published by the Free Software Foundation.

# Remote to gdb
#
target remote :1234

# Reload vmlinux for Image
#
add-symbol-file /xspace/OpenSource/BiscuitOS/BiscuitOS/output/linux-5.0-arm32/linux/linux/vmlinux 0x80100000 -s .head.text 0x80008000 -s .rodata 0x80800000 -s .init.text 0x80a002e0
{% endhighlight %}

获得数据如下：

![MMU](https://raw.githubusercontent.com/EmulateSpace/PictureSet/master/BiscuitOS/boot/BOOT000038.png)

MMU 启用后，内核开始使用虚拟地址。从上面的数据可知，.head.text Addr 项对应的地址
是 80008000, 因此 GDB 使用 add-symbol-file 重定位 vmlinux 的时候，需要使用
-s 选项重新指定 .head.text section 的地址是 0x80008000; 同理，.rodata 的 Addr
项是 80800000, 因此需要重新指定 .rodata section 的地址是 0x80800000. 最后，也是
最关键的，在 GDB 中使用 add-symbol-file 命令重定位 vmlinux 的地址，这个地址就是
vmlinux ELF 文件的 .text section 的地址，从图中可以看出，.text Addr 项是
0x80100000, 因此加载地址就是 0x80100000, 通过这样的调整之后，vmlinux
符号表就重定位到 Image 对应的位置上了。这里还要设计到 .init.text 的位置，和其他
section 一样的设置方法。

BiscuitOS 已经自动生成 gdb_RImage 文件，开发者只需按照教程 README 提示的步骤，
就可以简单完成对该阶段代码的调试。

--------------------------------------------------------------
<span id="Linux_decompress_after"></span>

![MMU](https://raw.githubusercontent.com/EmulateSpace/PictureSet/master/BiscuitOS/kernel/IND00000H.jpg)

# 内核解压后 start_kernel 之后 gdb 调试方法

内核运行到 start_kernel 之后，MMU 已经开启，内存地址已经与 vmlinux 映射，因此
可以直接调试内核而不需要重定位 vmlinux 的符号表。本节的所有内容都是基于 Linux 5.0
进行讲解的，如果还未搭建 Linux 5.0 开发环境，请参看如下教程：

> [Linux 5.0 arm 32 开发环境搭建手册](https://biscuitos.github.io/blog/Linux-5.0-arm32-Usermanual/)

搭建完上面的教程之后，参考 BiscuitOS/output/linux-5.0-arm32/README.md ,其中
关于 start_kernel 之后的调试介绍如下：

{% highlight bash %}
# Debugging kernel after start_kernel

### First Terminal

```
cd /xspace/OpenSource/BiscuitOS/BiscuitOS/output/linux-5.0-arm32
./RunQemuKernel.sh debug
```

### Second Terminal

```
BiscuitOS/output/linux-5.0-arm32/arm-linux-gnueabi/arm-linux-gnueabi/bin/arm-linux-gnueabi-gdb BiscuitOS/output/linux-5.0-arm32/linux/linux/vmlinux -x BiscuitOS/output/linux-5.0-arm32/package/gdb/gdb_Kernel

(gdb) b start_kernel
(gdb) c
(gdb) info reg
```
{% endhighlight %}

根据上面的介绍，开发者首先打开一个终端，在中断中输入如下命令：

{% highlight bash %}
cd BiscuitOS/output/linux-5.0-arm32
./RunQemuKernel.sh debug
{% endhighlight %}

然后再打开第二个终端，第二个终端中输入如下命令：

{% highlight bash %}
BiscuitOS/output/linux-5.0-arm32/arm-linux-gnueabi/arm-linux-gnueabi/bin/arm-linux-gnueabi-gdb BiscuitOS/output/linux-5.0-arm32/linux/linux/vmlinux -x BiscuitOS/output/linux-5.0-arm32/package/gdb/gdb_Kernel
{% endhighlight %}

此时第二个终端进入了 GDB 模式，开发者此时输入如下命令进行调试：

{% highlight base %}
(gdb) b start_kernel
(gdb) c
(gdb) info reg
{% endhighlight %}

其中在 start_kernel 处大断点。运行如下：

![MMU](https://raw.githubusercontent.com/EmulateSpace/PictureSet/master/BiscuitOS/boot/BOOT000039.png)

start_kernel 之后的 kernel GDB 调试都可以使用通用的 GDB 进行断点，函数，寄存器等
调试，开发者可以参考 GDB 手册进行调试。

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

![MMU](https://raw.githubusercontent.com/EmulateSpace/PictureSet/master/BiscuitOS/kernel/HAB000036.jpg)

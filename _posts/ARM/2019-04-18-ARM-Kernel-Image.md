---
layout:             post
title:              "Linux arm 内核镜像"
date:               2019-04-18 14:31:30 +0800
categories:         [MMU]
excerpt:            Linux Image zImage vmlinux uImage.
tags:
  - MMU
---

![LINUXP](https://raw.githubusercontent.com/EmulateSpace/PictureSet/master/BiscuitOS/kernel/GIF000203.gif)

> Email: BuddyZhang1 <buddy.zhang@aliyun.com>

# 目录

> - [内核镜像介绍](#原理)
>
> - [vmlinux 构建过程](#vmlinuxEE)
>
> - [Image 构建过程](#Image)
>
> - [piggy.gz/piggy_data 构建过程](#piggy.gz/piggy_data)
>
> - [piggy.o 构建过程](#piggy.o)
>
> - [Bootstrap ELF kernel (vmlinux) 构建过程](#Bootstrap ELF kernel)
>
> - [zImage 构建过程](#zImage)
>
> - [uImage 构建过程](#uImage)
>
> - [附录](#附录)

---------------------------------------------------------
<span id="原理"></span>

![MMU](https://raw.githubusercontent.com/EmulateSpace/PictureSet/master/BiscuitOS/kernel/IND00000L.jpg)

## 内核镜像介绍

![MMU](https://raw.githubusercontent.com/EmulateSpace/PictureSet/master/BiscuitOS/boot/BOOT000036.png)

vmlinux, Image, zImage, 以及 uImage，各种名词对各位开发者来说是内核开发中比较混淆
的地方，本文就给各位开发者理顺一下各个名词之间的关系以及构建构成，为内核学习打下扎实
基础。正如上图所示，ARM linux 内核是一个运行在 arm 平台上的 Linux 操作系统，该操作
系统由不同的源代码制作而成，这里称一个可在目标板上运行的操作系统文件为内核镜像。
内核镜像是由不同的源代码经过编译汇编，链接而成，在不同的阶段，内核镜像有不同的称呼，
因此本节介绍不同阶段内核镜像的名字。

##### vmlinux

内核镜像的第一个名字称为 vmlinux，其位于源码目录下。vmlinux 就是通过源码经过编译汇编，
链接而成的 ELF 文件，因此这个 vmlinux 文件包含了 ELF 的属性，以及各种调试信息等，因此
这个阶段的内核镜像 vmlinux 特别大，而且不能直接在 arm 上直接运行。

##### Image

由于 vmlinux 镜像体积巨大而且不能在 arm 上运行，因此需要使用 OBJCOPY 工具将不需要
的 section 从 vmlinux 里面剥离出来，最终在 arch/arm/boot/ 目录下生成了 Image 文件，
此时 Image 是可以在 arm 平台上运行的，但是由于历史原因，当年制作出 Image 的大小
正好比一个软盘大一点，为了让内核镜像能够装在一张软盘上，所以就将 Image 进行压缩，
生成 piggy.gz 或者 piggy_data.

##### piggy.gz/piggy_data

一开始只支持 gzip 压缩方法，所以将压缩之后的 Image 称为 piggy.gz，但随着内核的不断
发展，内核支持更多的压缩算法，因此把压缩之后的 Image 称为 piggy_data.

##### piggy.o

之前说过 Image 可以在 arm 上运行，当不能直接运行，因为 Image 运行前需要一些已知
初始化环境，这就需要特定功能的代码实现这些功能，这里称这些代码为 bootstrap。
于是内核在 arch/arm/boot/compressed/ 目录下增加了 bootstrap 功能的代码。和制作
vmlinux 一样，需要将这个目录下的源文件编译汇编成目标文件，然后再链接成一个文件。
为了构造这个，内核将 piggy_data 直接塞到了一个汇编文件 piggy.S 中，然后这个文件
经过汇编之后，就生成了 piggy.o

##### vmlinux (compress kernel)

为了构建能直接在 ARM 上运行的内核，压缩之后的内核与 bootstrap 功能的目标文件经过链接
生成了一个 ELF 文件 vmlinux，这个 vmlinux 位于 arch/arm/boot/compressed/ 目录下，
这个 vmlinux 与内核源码顶层目录下的 vmlinux 不是同一个文件。该目录下的 vmlinux
是包含 bootstrap 和压缩内核的内核镜像文件，是一个 ELF，所以不能在 arm 上直接运行，
于是和之前一样，使用 OBJCOPY 工具将 vmlinux 中不必要的段全部丢弃，最后生成二进制文件
zImage。

##### zImage

zImage 是可以直接在 arm 直接运行的内核镜像。zImage 的主要任务就是将被压缩的 Image
解压到指定位置，然后将控制权交给 Image 执行。因此，只要将 zImage 加载到内存指定位置
之后，内核就能正常启动。

#### uImage

uboot 为了启动内核，将 zImage 经过 mkimage 工具在 zImage 头部添加一些 uboot 加载
内核需要的信息。

---------------------------------------------------------
<span id="vmlinuxEE"></span>

![MMU](https://raw.githubusercontent.com/EmulateSpace/PictureSet/master/BiscuitOS/kernel/IND00000M.jpg)

## vmlinux 构建过程

vmlinux 文件是 Kbuild 编译系统将源码经过编译链接所获得的目标文件，所以它是一个 ELF
文件，因此 vmlinux 文件包含了各种调试信息和各种有用的 section。(注意！这里的 vmlinux
文件位于内核源码的顶层目录，可能其他目录也有名为 vmlinux 的文件)。vmlinux 文件的链接
过程由 arch/$(ARCH)/kernel/vmlinux.lds.S 链接脚本决定，可以通过该文件知道 vmlinux
文件的内部布局。vmlinux 生成的更多信息可以查看：

> [Kbuild 构建 Linux 内核]()

---------------------------------------------------------
<span id="Image"></span>

![MMU](https://raw.githubusercontent.com/EmulateSpace/PictureSet/master/BiscuitOS/kernel/IND00000K.jpg)

## Image 构建过程

Image 文件是 vmlinux 使用 objcopy 工具转换后得到的二进制文件。由于 vmlinux 不能
直接在 arm 上运行，需要丢弃一些与运行无关的 section，所以使用 objcopy 工具正好
可以完成这个任务。Image 文件相比 vmlinux，除了格式不同之外，vmlinux 的调试信息和
许多注释以及与运行无关的 section 都被移除，所以体积会变小很多。开发者可以在
arch/arm/boot/Makefile 中查看这个过程：

{% highlight bash %}
$(obj)/Image: vmlinux FORCE
  $(call if_changed,objcopy)
{% endhighlight %}

从上面可以看出 Image 就是通过 vmlinux objcopy 获得，这里 objcopy 对应的命令是
位于 scripts/Makefile.lib 文件中获得，定义如下：

{% highlight bash %}
quiet_cmd_objcopy = OBJCOPY $@
cmd_objcopy = $(OBJCOPY) $(OBJCOPYFLAGS) $(OBJCOPYFLAGS_$(@F)) $< $@
{% endhighlight %}

通过上面的代码，开发者可以在 Image 生成过程中添加打印消息，以此查看整个 object 过程，
添加调试代码如下：

{% highlight bash %}
$(obj)/Image: vmlinux FORCE
  $(warning "OBJCOPYFLAGS: $(OBJCOPYFLAGS)")
  $(warning "OBJCOPYFLAGS_$(@F): $(OBJCOPYFLAGS_$(@F))")
  $(call if_changed,objcopy)
{% endhighlight %}

然后编译内核时可以看到如下消息：

{% highlight bash %}
LD      vmlinux
SORTEX  vmlinux
SYSMAP  System.map
arch/arm/boot/Makefile:61: "OBJCOPYFLAGS: -O binary -R .comment -S"
arch/arm/boot/Makefile:61: "OBJCOPYFLAGS_Image: "
OBJCOPY arch/arm/boot/Image
Building modules, stage 2.
MODPOST 6 modules
Kernel: arch/arm/boot/Image is ready
{% endhighlight %}

从上面的调试可知，vmlinux ELF 文件使用 object 工具变成 Image 时，使用的参数
是 "-O binary -R .comment -S"，这个参数的意思是：

{% highlight bash %}
-O binary 表示生成二进制文件

-R .comment 表示移除 .comment section

-S 表示移除所有的标志以及重定位信息
{% endhighlight %}

---------------------------------------------------------
<span id="piggy.gz/piggy_data"></span>

![MMU](https://raw.githubusercontent.com/EmulateSpace/PictureSet/master/BiscuitOS/kernel/IND00000O.jpg)

## piggy.gz/piggy_data 构建过程

piggy.gz 是 Image 经过压缩之后得到的压缩文件，在高版本中，piggy.gz 被命名为
piggy_data，指代码压缩内核，Image 压缩过程位于 arch/arm/boot/compressed/Makefile
里面，具体如下：

{% highlight bash %}
$(obj)/piggy_data: $(obj)/../Image FORCE
  $(call if_changed,$(compress-y))
{% endhighlight %}

通过上面的内容可知，内核采用的压缩方法由 compress-y 变量决定，其定义在
arch/arm/boot/compressed/Makefile 里面，如下：

{% highlight bash %}
compress-$(CONFIG_KERNEL_GZIP) = gzip
compress-$(CONFIG_KERNEL_LZO)  = lzo
compress-$(CONFIG_KERNEL_LZMA) = lzma
compress-$(CONFIG_KERNEL_XZ)   = xzkern
compress-$(CONFIG_KERNEL_LZ4)  = lz4
{% endhighlight %}

因此内核支持 gzip,lzo,lzma,xzkern, 和 lz4 的压缩方法，具体使用哪种，因此开发者可以在
命令执行处添加调试代码如下：

{% highlight bash %}
$(obj)/piggy_data: $(obj)/../Image FORCE
  $(wraning "compress-y: $(compress-y)")
  $(call if_changed,$(compress-y))
{% endhighlight %}

编译内核，获得如下调试信息：

{% highlight bash %}
CALL    scripts/checksyscalls.sh
CHK     include/generated/compile.h
Kernel: arch/arm/boot/Image is ready
Building modules, stage 2.
MODPOST 6 modules
arch/arm/boot/compressed/Makefile:192: "compress-y: gzip"
Kernel: arch/arm/boot/zImage is ready
{% endhighlight %}

所以 Image 采用了 gzip 方法，因此开发者可以在 scripts/Makefile.lib 文件中获得
具体的 gzip 过程，如下：

{% highlight bash %}
quiet_cmd_gzip = GZIP    $@
      cmd_gzip = cat $(filter-out FORCE,$^) | gzip -n -f -9 > $@
{% endhighlight %}

gizp 的参数含义如下：

{% highlight bash %}
-n 压缩文件时，不保存原来文件名称以及时间戳

-f 强制压缩文件。不理会文件名称或硬链接是否存在以及文件是否为符号链接

-9 用 9 调整压缩的速度，-1 或 --fast 表示最快压缩方法 (低压缩比), -9 或者 --best 表示最慢的压缩方法 (高压缩比)
{% endhighlight %}

其他压缩方法同理分析。经过上面分析，Image 压缩成 piggy.gz 或者 piggy_data 的过程已经
分析完毕。

---------------------------------------------------------
<span id="piggy.o"></span>

![MMU](https://raw.githubusercontent.com/EmulateSpace/PictureSet/master/BiscuitOS/kernel/IND00000H.jpg)

## piggy.o 构建过程

piggy.o 文件是通过 piggy_data/piggy.gz 汇编之后的可链接的目标文件，其汇编命令位于
arch/arm/boot/compressed/Makefile,具体内容如下：

{% highlight bash %}
$(obj)/piggy.o: $(obj)/piggy_data
{% endhighlight %}

从上面的命令来看，piggy.o 是通过 piggy.S 汇编生成，其依赖 piggy_data，那么 piggy.S
的内容如下 (arch/arm/boot/compressed/piggy.S)：

{% highlight bash %}
/* SPDX-License-Identifier: GPL-2.0 */
        .section .piggydata,#alloc
        .globl  input_data
input_data:
        .incbin "arch/arm/boot/compressed/piggy_data"
        .globl  input_data_end
input_data_end:
{% endhighlight %}

从上面的汇编代码可以知道，piggy.S 汇编中调用 incbin 指令将 arch/arm/boot/compressed/piggy_data
到汇编文件中，成为汇编的一部分，这样 piggy_data 就能被汇编最后成为一个可链接的目标文件。
这里定义了两个全局符号： input_data 和 input_data_end。这两个符号标记了压缩内核在
piggy.o 中的起始地址和终止地址，对链接脚本有用。至此，piggy.o 的构建过程分析完毕，内核
到此已经被汇编成一个可链接的目标文件。

---------------------------------------------------------
<span id="Bootstrap ELF kernel"></span>

![MMU](https://raw.githubusercontent.com/EmulateSpace/PictureSet/master/BiscuitOS/kernel/IND00000J.jpg)

## Bootstrap ELF kernel (vmlinux) 构建过程

只有纯粹的内核是无法启动的，所以需要在内核的头部加入一些用于 bootstrap loader 功能的代码。
Kbuild 编译系统在 arch/arm/boot/compressed/ 目录下，将 head.S, misc.S,
compressed.S 等多个汇编文件汇编成多个可链接的 ELF 目标文件，以此作为内核的
bootstrap loader。在这个步骤，Kbuid 编译系统将这些可链接的目标文件与 piggy.o 文件按
链接脚本的内容进行链接，制作出一个带 bootstrap loader 的内核ELF 文件。对于的过程要参考
arch/arm/boot/compressed/ 目录下的 Makefile 和  vmlinux.lds.S 文件。
首先通过分析 Makefile 知道链接的文件，具体源码如下：

{% highlight bash %}
$(obj)/vmlinux: $(obj)/vmlinux.lds $(obj)/$(HEAD) $(obj)/piggy.o \
                $(addprefix $(obj)/, $(OBJS)) $(lib1funcs) $(ashldi3) \
                $(bswapsdi2) $(efi-obj-y) FORCE
        @$(check_for_multiple_zreladdr)
        $(call if_changed,ld)
        @$(check_for_bad_syms)
{% endhighlight %}

从上面的代码可知，这里将可链接之后的 ELF 目标文件也成为 vmlinux，开发者要将这个
vmlinux 与源码顶层目录的 vmlinux 区分开来。这里的 vmlinux 是添加了 bootstrap loader。
从上面的代码可以看出，vmlinux 的链接过程通过 vmlinux.lds 进行链接，这里开发者可以好好分析
一下这个链接脚本，以此知道 vmlinux 如何布局，以及系统运行之后，vmlinux 如何在内存中布局，
在下一节重点分析 vmlinux.lds 链接脚本。vmlinux 的具体构建过程，在下面的章节会详细介绍。
至此，一个带 bootstrap loader 的内核 ELF 文件已经制作完成，但由之前分析可知，ELF 文件
是不能直接在 arm 上运行的，需要制作成 bin 文件才能在 arm 上运行，所以下一步就是 zImage
的制作。

## vmlinux.lds

这里所介绍的 vmlinux.lds 是位于 arch/arm/boot/compressed/ 目录下的 vmlinux.lds，
这个 vmlinux.lds 用于将压缩内核 ELF 文件 piggy.o 与其他目标文件链接成一个带 bootstrap
loader 的 ELF 目标文件。

{% highlight bash %}
(vmlinux)
+---------+
|         |
|         |
|         |
| piggy.o |
|         |
|         |
|         |
+---------+
| misc.o  |
+---------+
| big_    |
| endian.o|
+---------+
| head-   |
| xscal.o |
+---------+
|  head.o |
+---------+
{% endhighlight %}

链接之后的 ELF 文件成为 vmlinux，用于在 arm 上解压被压缩的内核。那么接下来分析一下这个 ELF
的构建过程。由于源码比较长，这里分段解析：

{% highlight bash %}
/*
 *  Copyright (C) 2000 Russell King
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifdef CONFIG_CPU_ENDIAN_BE8
#define ZIMAGE_MAGIC(x) ( (((x) >> 24) & 0x000000ff) | \
                          (((x) >>  8) & 0x0000ff00) | \
                          (((x) <<  8) & 0x00ff0000) | \
                          (((x) << 24) & 0xff000000) )
#else
#define ZIMAGE_MAGIC(x) (x)
#endif
{% endhighlight %}

首先通过判断宏 CONFIG_CPU_ENDIAN_BE8 是否定义，以此定义了一个宏操作 ZIMAGE_MAGIC,
这个宏主要用于调整字节序。

{% highlight bash %}
OUTPUT_ARCH(arm)
ENTRY(_start)
{% endhighlight %}

链接脚本首先定义了 ELF 目标文件运行在 arm，并且 vmlinux 的入口函数为 _start。

{% highlight bash %}
/DISCARD/ : {
  *(.ARM.exidx*)
  *(.ARM.extab*)
  /*
   * Discard any r/w data - this produces a link error if we have any,
   * which is required for PIC decompression.  Local data generates
   * GOTOFF relocations, which prevents it being relocated independently
   * of the text/got segments.
   */
  *(.data)
}
{% endhighlight %}

链接脚本使用 "/DISCARD/" 关键字，将所有输入文件的 ".ARM.exidx", ".ARM.extab",
以及可读写的 ".data" 数据段。因此如果 vmlinux ELF 的目标文件的数据最好放到代码段
里作为只读数据。

{% highlight bash %}
. = TEXT_START;
_text = .;

.text : {
  _start = .;
  *(.start)
  *(.text)
  *(.text.*)
  *(.fixup)
  *(.gnu.warning)
  *(.glue_7t)
  *(.glue_7)
}
{% endhighlight %}

链接脚本定义了 vmlinux ELF 目标文件的代码段 .text，其包含了所有输入文件的 ".start",
".text", ".text.*", ".fixup", ".gnu.warning", ".glue_7t", ".glue_7" sections。
并定义了 .text section 的起始地址是 TEXT_START, 并且 _text 指向 .text section
开始的地方。

{% highlight bash %}
.table : ALIGN(4) {
  _table_start = .;
  LONG(ZIMAGE_MAGIC(2))
  LONG(ZIMAGE_MAGIC(0x5a534c4b))
  LONG(ZIMAGE_MAGIC(__piggy_size_addr - _start))
  LONG(ZIMAGE_MAGIC(_kernel_bss_size))
  LONG(0)
  _table_end = .;
}
{% endhighlight %}

接下来定义了 .table section， 按 4 字节对齐，.table section 中首先定义了 _table_start
变量，用于指向 .table section 的起始位置。 _table_end 变量指向 .table section 的结束
地址，所以在代码中使用这两个变量就可以确定 .table 的位置。这里用于创建一个 table，table
内首先定义了一个 long 字节用于存储 2，第二个 long 直接存储 MAGIC 0x5a534c4b。对于第三个
字节，首先 __piggy_size_addr 地址对应的值用于存储压缩内核的大小，也就是 piggy_data 的
大小。所以第三个 long 字节用于存储 __piggy_size_addr 与 _start 之间的偏移，用于运行
时确定 __piggy_size_addr 正确位置。第四个 long 字节用于存储内核的 bss section 的大小。
第五个 long 字节存储一个 0， 用于结尾。通过这个 table，基本确定了带 bootstrap loader
的 vmlinux 内存布局了。

{% highlight bash %}
.rodata : {
  *(.rodata)
  *(.rodata.*)
  *(.data.rel.ro)
}
{% endhighlight %}

定义了 .rodata section, 用于存储所有输入文件的 .rodata, .rodata.*, .data.rel.ro sections，
这个 section 基本就是随机数的 section。

{% highlight bash %}
.piggydata : {
  *(.piggydata)
  __piggy_size_addr = . - 4;
}
{% endhighlight %}

这里比较重要的是建立了 .piggydata section, 这个 section 就是用于存储被压缩的内核镜像，
之前 piggy_data 经过汇编生成 piggy.o 之后，由于 piggy_data 是二进制文件，所以
piggy_data 内的 section 没有被 vmlinux.lds.S 拆分。这里还定义了一个变量 __piggy_size_addr
用于存储 piggy_data 的大小。 __piggy_size_addr 指向当前地址前 4 字节。这样 piggy_data
的最后 4 个字节就是存储 piggy_data 的大小。

{% highlight bash %}
.got.plt              : { *(.got.plt) }
_got_start = .;
.got                  : { *(.got) }
_got_end = .;
{% endhighlight %}

创建了 .got.plt 和 .got section, 用于存储输入文件的 GOT 表和 PLT 表。PLT 表可以称为
内部函数表，GOT 表称为全局函数表。

{% highlight bash %}
/* ensure the zImage file size is always a multiple of 64 bits */
/* (without a dummy byte, ld just ignores the empty section) */
.pad                  : { BYTE(0); . = ALIGN(8); }

#ifdef CONFIG_EFI_STUB
.data : ALIGN(4096) {
  __pecoff_data_start = .;
  /*
   * The EFI stub always executes from RAM, and runs strictly before the
   * decompressor, so we can make an exception for its r/w data, and keep it
   */
  *(.data.efistub)
  __pecoff_data_end = .;

  /*
   * PE/COFF mandates a file size which is a multiple of 512 bytes if the
   * section size equals or exceeds 4 KB
   */
  . = ALIGN(512);
}
__pecoff_data_rawsize = . - ADDR(.data);
#endif
{% endhighlight %}

这里由于没有定义 CONFIG_EFI_STUB 宏，则不做讲解。

{% highlight bash %}
_edata = .;

/*
 * The image_end section appears after any additional loadable sections
 * that the linker may decide to insert in the binary image.  Having
 * this symbol allows further debug in the near future.
 */
.image_end (NOLOAD) : {
  /*
   * EFI requires that the image is aligned to 512 bytes, and appended
   * DTB requires that we know where the end of the image is.  Ensure
   * that both are satisfied by ensuring that there are no additional
   * sections emitted into the decompressor image.
   */
  _edata_real = .;
}
{% endhighlight %}

_edata 指向了 data 段的结束地址，由于 .image_end section 定义了 NOLOAD，所以
这个 section 不会被链接到 vmlinux。

{% highlight bash %}
_magic_sig = ZIMAGE_MAGIC(0x016f2818);
_magic_start = ZIMAGE_MAGIC(_start);
_magic_end = ZIMAGE_MAGIC(_edata);
_magic_table = ZIMAGE_MAGIC(_table_start - _start);
{% endhighlight %}

接着定义了 4 个变量。_maigc_sig 存储 0x016f2818;_magic_start 指向了 vmlinux
的起始地址 _start; _magic_end 指向了 _edata； _magic_table 指向了.table 的相对
地址。

{% highlight bash %}
. = BSS_START;
__bss_start = .;
.bss                  : { *(.bss) }
_end = .;
{% endhighlight %}

使用 BSS_START 定义了 BSS section 在内存中的地址。其定义如下：

{% highlight bash %}
#
# We now have a PIC decompressor implementation.  Decompressors running
# from RAM should not define ZTEXTADDR.  Decompressors running directly
# from ROM or Flash must define ZTEXTADDR (preferably via the config)
# FIXME: Previous assignment to ztextaddr-y is lost here. See SHARK
ifeq ($(CONFIG_ZBOOT_ROM),y)
ZTEXTADDR       := $(CONFIG_ZBOOT_ROM_TEXT)
ZBSSADDR        := $(CONFIG_ZBOOT_ROM_BSS)
else
ZTEXTADDR       := 0
ZBSSADDR        := ALIGN(8)
endif

CPPFLAGS_vmlinux.lds := -DTEXT_START="$(ZTEXTADDR)" -DBSS_START="$(ZBSSADDR)"
{% endhighlight %}

所以这里用 CONFIG_ZBOOT_ROM 来设置 BSS_START 的起始地址。以此，当 CONFIG_ZBOOT_ROM
没有使用的情况下，ZBSSADDR 指向 ALIGN(8), 所以 BSS 段的起始地址是从当前地址进行 8 字节
对齐之后，作为 BSS 的起始地址。并且这里也定义了 vmlinux 的 _end 地址以及 __bss_start
地址。

{% highlight bash %}
. = ALIGN(8);         /* the stack must be 64-bit aligned */
.stack                : { *(.stack) }

PROVIDE(__pecoff_data_size = ALIGN(512) - ADDR(.data));
PROVIDE(__pecoff_end = ALIGN(512));
{% endhighlight %}

最后定义了 .stack section, 将所有的输入文件的 .stack 存储到 .stack section。并定义
了 __pecoff_data_size 和 __pecoff_end 地址。

{% highlight bash %}
.stab 0               : { *(.stab) }
.stabstr 0            : { *(.stabstr) }
.stab.excl 0          : { *(.stab.excl) }
.stab.exclstr 0       : { *(.stab.exclstr) }
.stab.index 0         : { *(.stab.index) }
.stab.indexstr 0      : { *(.stab.indexstr) }
.comment 0            : { *(.comment) }
ASSERT(_edata_real == _edata, "error: zImage file size is incorrect");
{% endhighlight %}

这些 section 基本不需要讨论。最后使用 ASSERT 关键字确定 _edata_real 和 _edata 之间
的关系是否满足。如果不满足。则 zImage 的 size 不正确。

以此，通过上面 vmlinux.lds.S 链接脚本链接之后，生成的带 bootstrap 的 vmlinux ELF
文件。可以使用 objdump 工具查看此时的 ELF 布局：

{% highlight bash %}
vmlinux:     file format elf32-little
vmlinux
architecture: UNKNOWN!, flags 0x00000112:
EXEC_P, HAS_SYMS, D_PAGED
start address 0x00000000

Program Header:
    LOAD off    0x00010000 vaddr 0x00000000 paddr 0x00000000 align 2**16
         filesz 0x0043cf70 memsz 0x0043df88 flags rwx
   STACK off    0x00000000 vaddr 0x00000000 paddr 0x00000000 align 2**4
         filesz 0x00000000 memsz 0x00000000 flags rwx

Sections:
Idx Name          Size      VMA       LMA       File off  Algn
  0 .text         00003ae0  00000000  00000000  00010000  2**5
                  CONTENTS, ALLOC, LOAD, READONLY, CODE
  1 .table        00000014  00003ae0  00003ae0  00013ae0  2**2
                  CONTENTS, ALLOC, LOAD, DATA
  2 .rodata       00000ce4  00003af4  00003af4  00013af4  2**2
                  CONTENTS, ALLOC, LOAD, READONLY, DATA
  3 .piggydata    0043875c  000047d8  000047d8  000147d8  2**0
                  CONTENTS, ALLOC, LOAD, READONLY, DATA
  4 .got.plt      0000000c  0043cf34  0043cf34  0044cf34  2**2
                  CONTENTS, ALLOC, LOAD, DATA
  5 .got          00000028  0043cf40  0043cf40  0044cf40  2**2
                  CONTENTS, ALLOC, LOAD, DATA
  6 .pad          00000008  0043cf68  0043cf68  0044cf68  2**0
                  CONTENTS, ALLOC, LOAD, DATA
  7 .bss          00000018  0043cf70  0043cf70  0044cf70  2**2
                  ALLOC
  8 .stack        00001000  0043cf88  0043cf88  0044cf70  2**0
                  ALLOC
  9 .comment      00000074  00000000  00000000  0044cf70  2**0
                  CONTENTS, READONLY
 10 .ARM.attributes 0000002d  00000000  00000000  0044cfe4  2**0
                  CONTENTS, READONLY
{% endhighlight %}

---------------------------------------------------------
<span id="zImage"></span>

![MMU](https://raw.githubusercontent.com/EmulateSpace/PictureSet/master/BiscuitOS/kernel/IND00000F.jpg)

## zImage 构建过程

zImage 是通过带 bootstrap loader 的内核 ELF 文件经过 objcopy 命令之后制作生成
的二进制文件，用于在 arm 上直接运行，其生成过程可以查看 arch/arm/boot/Makefile:

{% highlight bash %}
$(obj)/zImage:  $(obj)/compressed/vmlinux FORCE
        $(call if_changed,objcopy)
{% endhighlight %}

同原始 vmlinux 转换为 Image 过程一致，具体细节可以参考上面章节。制作完 zImage 之后，
可以将 zImage 在 arm 上运行。

---------------------------------------------------------
<span id="uImage"></span>

![MMU](https://raw.githubusercontent.com/EmulateSpace/PictureSet/master/BiscuitOS/kernel/IND00000F.jpg)

## uImage 构建过程

uboot 为加载 zImage，会使用 mkimage 工具对 zImage 进行相应处理生成 uImage，
因此 uImage 用于 uboot 加载。其构建如下：

{% highlight bash %}
mkimage -A arm -O linux -T kernel -C none -a 60008000 -e 60008000 -n linux-5.0 -d zImage uImage
{% endhighlight %}

-----------------------------------------

# <span id="附录">附录</span>

> [Linux ARM Debugging](https://biscuitos.github.io/blog/BOOTASM-debuggingTools/#ARM%20Boot-Stage1)
>
> [Debugging ARM Linux from first code](https://biscuitos.github.io/blog/ARM-BOOT/)
>
> [BiscuitOS Home](https://biscuitos.github.io/)
>
> [BiscuitOS Kernel Build](https://biscuitos.github.io/blog/Kernel_Build/)
>
> [Linux Kernel](https://www.kernel.org/)
>
> [Bootlin: Elixir Cross Referencer](https://elixir.bootlin.com/linux/latest/source)

## 赞赏一下吧 🙂

![MMU](https://raw.githubusercontent.com/EmulateSpace/PictureSet/master/BiscuitOS/kernel/HAB000036.jpg)

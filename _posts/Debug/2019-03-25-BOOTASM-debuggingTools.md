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
> - [ARM 内核解压后 gdb 调试方法](#ARM compressed)
>
> - [Linux start_kernel 之后 gdb 调试方法](#ARM_start_kernel)
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

![MMU](https://raw.githubusercontent.com/EmulateSpace/PictureSet/master/BiscuitOS/kernel/BOOT000000.png)

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
BiscuitOS/output/linux-5.0-arm32/arm-linux-gnueabi/arm-linux-gnueabi/bin/arm-linux-gnueabi-gdb -x BiscuitOS/output/linux-5.0-arm32/gdb/gdb_zImage

(gdb) b XXX_bk
(gdb) c
(gdb) info reg
```
```
{% endhighlight %}

根据上面的介绍，开发者首先打开一个终端，在中断中输入如下命令：

{% highlight base %}
cd BiscuitOS/output/linux-5.0-arm32
./RunQemuKernel.sh debug
{% endhighlight %}

然后再打开第二个终端，第二个终端中输入如下命令：

{% highlight base %}
BiscuitOS/output/linux-5.0-arm32/arm-linux-gnueabi/arm-linux-gnueabi/bin/arm-linux-gnueabi-gdb -x BiscuitOS/output/linux-5.0-arm32/gdb/gdb_zImage
{% endhighlight %}

此时第二个终端进入了 GDB 模式，开发者此时输入如下命令进行调试：

{% highlight base %}
(gdb) b XXX_bk
(gdb) c
(gdb) info reg
{% endhighlight %}

其中 XXX_bk 是断点的名字。运行如下：

![MMU](https://raw.githubusercontent.com/EmulateSpace/PictureSet/master/BiscuitOS/kernel/ASM000000.png)

#### 打断点

在实际调试过程中需要对不同的代码段打断点，以此提高调试效率。在 zImage 重定位之前的阶段
打断点请参考如下步骤：

zImage 重定位之前的阶段的代码大多位于 arch/arm/boot/compressed/ 目录下，其中这个阶段
的入口函数位于 arch/arm/boot/compressed/head.S 里面，如下：

{% highlight base %}
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

{% highlight base %}
(gdb) b BS_debug
(gdb) c
(gdb) info reg
{% endhighlight %}

实际运行情况如下图：

![MMU](https://raw.githubusercontent.com/EmulateSpace/PictureSet/master/BiscuitOS/kernel/ASM000001.png)

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

---
layout:             post
title:              "ARM Boot-Stage Debugging Usermanual"
date:               2019-03-25 17:28:30 +0800
categories:         [MMU]
excerpt:            ARM Boot-Stage Debugging Usermanual.
tags:
  - MMU
---

![LINUXP](https://raw.githubusercontent.com/EmulateSpace/PictureSet/master/BiscuitOS/kernel/GIF000205.gif)

> [GitHub ARM Assembly Set](https://github.com/BiscuitOS/HardStack/tree/master/Language/Assembly/Inline-Assembly/ARM/Instruction)
>
> Email: BuddyZhang1 <buddy.zhang@aliyun.com>

# 目录

> - [ARM Boot Stage 1 调试方法](#ARM Boot-Stage1)
>
> - [ARM Boot Stage 2 调试方法](#ARM Boot-Stage2)
>
> - [附录](#附录)

--------------------------------------------------------------
<span id="ARM Boot-Stage1"></span>

![MMU](https://raw.githubusercontent.com/EmulateSpace/PictureSet/master/BiscuitOS/kernel/IND00000A.jpg)

# ARM Boot Stage 1 调试方法

ARM Boot Stage 1 指的是 Uboot 将压缩好的内核加载到内存之后，内核还没有解压缩之
前的这个阶段。开发者可以使用 gdb 工具在这个阶段进行单步调试，调试的步骤如下：
本文基于 ARM 32 Linux 5.0 进行讲解，如果还没有搭建 Linux 5.0 ，可以参考下面教程：

> [Linux 5.0 arm 32 开发环境搭建手册](https://biscuitos.github.io/blog/Linux-5.0-arm32-Usermanual/)

搭建完上面的教程之后，参考 BiscuitOS/output/linux-5.0-arm32/README.md ,其中
关于 Boot Stage 1 调试介绍如下：

{% highlight base %}
# Debugging Boot-Stage

### First Terminal

```
cd BiscuitOS/output/linux-5.0-arm32
./RunQemuKernel.sh debug_boot
```

### Second Terminal

```
BiscuitOS/output/linux-5.0-arm32/arm-linux-gnueabi/arm-linux-gnueabi/bin/arm-linux-gnueabi-gdb BiscuitOS/output/linux-5.0-arm32/linux/linux/arch/arm/boot/compressed/vmlinux

(gdb) target remote :1234
(gdb) b XXX_bk
(gdb) c
(gdb) info reg
```
{% endhighlight %}

根据上面的介绍，开发者首先打开一个终端，在中断中输入如下命令：

{% highlight base %}
cd BiscuitOS/output/linux-5.0-arm32
./RunQemuKernel.sh debug_boot
{% endhighlight %}

然后再打开第二个终端，第二个终端中输入如下命令：

{% highlight base %}
BiscuitOS/output/linux-5.0-arm32/arm-linux-gnueabi/arm-linux-gnueabi/bin/arm-linux-gnueabi-gdb BiscuitOS/output/linux-5.0-arm32/linux/linux/arch/arm/boot/compressed/vmlinux
{% endhighlight %}

此时第二个终端进入了 GDB 模式，开发者此时输入如下命令进行调试：

{% highlight base %}
(gdb) target remote :1234
(gdb) b XXX_bk
(gdb) c
(gdb) info reg
{% endhighlight %}

其中 XXX_bk 是断点的名字。运行如下：

![MMU](https://raw.githubusercontent.com/EmulateSpace/PictureSet/master/BiscuitOS/kernel/ASM000000.png)

#### 打断点

在实际调试过程中需要对不同的代码段打断点，以此提高调试效率。在 Boot Stage 1 阶段
打断点请参考如下步骤：

Boot Stage 1 阶段的代码大多位于 arch/arm/boot/compressed/ 目录下，其中这个阶段
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
(gdb) target remote :1234
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

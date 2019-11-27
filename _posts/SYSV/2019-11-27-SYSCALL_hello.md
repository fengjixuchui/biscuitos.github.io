---
layout: post
title:  "增加一个系统调用 (arm)"
date:   2019-11-27 08:35:30 +0800
categories: [HW]
excerpt: syscall.
tags:
  - syscall
---

![](https://raw.githubusercontent.com/EmulateSpace/PictureSet/master/BiscuitOS/kernel/IND00000L0.PNG)

> Email: BuddyZhang1 <buddy.zhang@aliyun.com>

# 目录

> - [原理分析](#A0)
>
> - [实践部署](#B0)
>
> - [附录/捐赠](#C0)

-----------------------------------

# <span id="A0"></span>

![](https://raw.githubusercontent.com/EmulateSpace/PictureSet/master/BiscuitOS/kernel/IND00000H0.PNG)

## 原理分析

Linux 5.x 之后，arm 体系增加一个系统调用已经变得很便捷，arm
内核将系统调用的重复性操作从内核移除，开发者只需简单几部就能
添加一个新的系统调用。

首先在内核源码中修改文件 "arch/arm/tools/syscall.tbl" 的底部添加
如下内容:

![](https://raw.githubusercontent.com/EmulateSpace/PictureSet/master/RPI/RPI000310.png)

如上面内容所示，在文件最后一行添加了名为 hello_BiscuitOS 的
系统调用，400 代表系统调用号，hello_BiscuitOS 为系统调用的
名字，sys_hello_BiscuitOS 为系统调用在内核的实现。添加
完毕并保存文件，接着重新编译内核。编译内核中会打印相关的
信息如下图:

![](https://raw.githubusercontent.com/EmulateSpace/PictureSet/master/RPI/RPI000306.png)

从上面的编译信息看出，编译系统调用相关的脚本自动为
hello_BiscuitOS 生成了相关的系统调用，可以查看
"arch/arm/include/generated/asm/unistd-nr.h" 已经自动
生成当前系统调用的总数:

![](https://raw.githubusercontent.com/EmulateSpace/PictureSet/master/RPI/RPI000307.png)

可以看出系统调用的总数 __NR_syscalls 变成了 404. 接着
在 "arch/arm/include/generated/uapi/asm/unistd-common.h"
文件中自动生成了系统调用号，如下图:

![](https://raw.githubusercontent.com/EmulateSpace/PictureSet/master/RPI/RPI000308.png)

从上面的文件中看到新添加系统调用的系统号为 "__NR_hello_BiscuitOS".
在 "arch/arm/include/generated/" 目录下自动生成两个文件 "calls-eabi.S"
"calls-oabi.S", 该汇编文件中包含了系统调用的入口，如下:

![](https://raw.githubusercontent.com/EmulateSpace/PictureSet/master/RPI/RPI000309.png)

从上图可以看到 hello_BiscuitOS 系统调用的入口为 
"NATIVE(400, sys_hello_BiscuitOS)".
添加完系统调用入口后，接着添加实际的系统调用接口，
可以在内核源码数任意位置，添加一个 C 源码文件，并
将源码文件编译进内核即可，其源码实现可以如下:

![](https://raw.githubusercontent.com/EmulateSpace/PictureSet/master/RPI/RPI000312.png)

将上面的文件编译进内核，重新编译内核即可。最后就是
在用户空间添加一个系统调用的函数，如下:

![](https://raw.githubusercontent.com/EmulateSpace/PictureSet/master/RPI/RPI000311.png)

用户空间通过 swi 指令可以执行一次系统调用，在执行系统调用
的时候，将系统调用号存储在 r7 寄存器，并使用 "swi 0" 指令
触发系统调用。至此一个新的系统调用就加到系统里，并可以在
用户空间进行调用。

-----------------------------------

# <span id="B0"></span>

![](https://raw.githubusercontent.com/EmulateSpace/PictureSet/master/BiscuitOS/kernel/IND00000I.jpg)

## 实践部署

> - [添加系统调用号](#B00)
>
> - [添加内核实现](#B01)
>
> - [添加用户空间实现](#B02)
>
> - [运行系统调用](#B03)

--------------------------------------------

## <span id="B00">添加系统调用号</span>

Linux 5.x arm 内核已经采用比较便捷的方式添加系统调用，
如果开发者还没有 linux 5.x arm 内核开发环境，请看考
下面文档:

> - [Linux 5.x arm 内核开发环境部署](https://biscuitos.github.io/blog/Kernel_Build/#Linux_5X)

准备好开发环境之后，在 linux 内核源码中修改文件 
"arch/arm/tools/syscall.tbl" 的底部添加如下内容:

![](https://raw.githubusercontent.com/EmulateSpace/PictureSet/master/RPI/RPI000310.png)

如上面内容所示，在文件最后一行添加了名为 hello_BiscuitOS 的
系统调用，400 代表系统调用号，hello_BiscuitOS 为系统调用的
名字，sys_hello_BiscuitOS 为系统调用在内核的实现。至此系统
号已经添加完毕。

--------------------------------------------

## <span id="B01">添加内核实现</span>

添加完系统号之后，需要在内核中添加系统调用的具体实现。
那么开发者接着在内核源码树中添加一个 c 源文件，例如
"arch/arm/kernel" 目录下添加名为 "BiscuitOS_syscall.c" 源文件:

![](https://raw.githubusercontent.com/EmulateSpace/PictureSet/master/RPI/RPI000312.png)

修改 "arch/arm/kernel/Makefile" 如下:

![](https://raw.githubusercontent.com/EmulateSpace/PictureSet/master/RPI/RPI000313.png)

上图将 BiscuitOS_syscall.o 按 obj-y 的方式编译进内核，
做好上面的步骤之后，最后就是编译内核，编译内核过程中
输出了系统调用号生成的相关信息，如下图:

![](https://raw.githubusercontent.com/EmulateSpace/PictureSet/master/RPI/RPI000306.png)

--------------------------------------------

## <span id="B02">添加用户空间实现</span>

系统调用就是在用户空间访问内核的一种方式，因此最后一步就是
在用户空间增加调用接口，可以参考下面例子编写应用程序:

![](https://raw.githubusercontent.com/EmulateSpace/PictureSet/master/RPI/RPI000315.png)

用户空间通过 swi 指令可以执行一次系统调用，在执行系统调用
的时候，将系统调用号存储在 r7 寄存器，并使用 "swi 0" 指令
触发系统调用。至此一个新的系统调用就加到系统里，并可以在
用户空间进行调用。

--------------------------------------------

## <span id="B03">运行系统调用</span>

将内核重新编译，并交叉编译应用程序到目标机器上，运行如下:

![](https://raw.githubusercontent.com/EmulateSpace/PictureSet/master/RPI/RPI000314.png)

-----------------------------------------------

## <span id="C0">附录</span>

> [BiscuitOS Home](https://biscuitos.github.io/)
>
> [BiscuitOS Driver](https://biscuitos.github.io/blog/BiscuitOS_Catalogue/)
>
> [Linux Kernel](https://www.kernel.org/)
>
> [Bootlin: Elixir Cross Referencer](https://elixir.bootlin.com/linux/latest/source)
>

## 赞赏一下吧 🙂

![MMU](https://raw.githubusercontent.com/EmulateSpace/PictureSet/master/BiscuitOS/kernel/HAB000036.jpg)

---
layout: post
title:  ".pushsection"
date:   2020-01-06 14:02:30 +0800
categories: [HW]
excerpt: ASM .pushsection.
tags:
  - ASM
---

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND00000I.jpg)

> Email: BuddyZhang1 <buddy.zhang@aliyun.com>

## 目录

> - [指令分析](#A0)
>
> - [指令实践](#B0)
>
> - [附录](#Y0)

----------------------------------

<span id="A0"></span>

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND00000P.jpg)

#### 指令分析

> - [指令介绍](#A00)
>
> - [指令使用](#A01)

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND000101.jpg)

------------------------------------------------------

#### <span id="A00">指令介绍</span>

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/HK000204.png)

.pushsection 与 .popsection 成对使用，用于在当前目标文件中插入一个 section。
.pushsection 的使用方法如上图所示

###### section_name

如果源码中包含了 .pushsection/.popsection, 那么编译器就像目标文件中插
入名为 "section_name" 的 section。开发者可以自定义一个新的 section 名字
或者一个已经存在的 section 名字。

###### permission

permission 字段用于指明 section 的权限。section 权限包括:  "a" (可分配)、
"w" (可写)、"r" (可读)、"x" (可执行)，permission 字段可以组合使用 "awrx".

###### instruction

instruction 部分用于 section 内部的代码，这里可以放入插入的代码。

------------------------------------------------------

#### <span id="A01">指令使用</span>

当需要在目标文件中加入一个 section，可以使用 .pushsection 添加，
接下来以一个实际例子分析指令如何使用:

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/HK000205.png)

这里使用内嵌汇编的方法使用 .pushsection, 开发者可以参考在汇编中
直接使用 .pushsection. 在上面的例子中，在文件对应的目标文件中定义
了一个名为 ".biscuitos" 的 section，section 属性是 "ax", 即可分配
和可执行的。接着在 .biscuitos 插入了 "mov r0, #0x80" 代码。向
.biscuitos section 添加完想要的代码之后，使用 .popsection 进行结束。
接下来就是编译、汇编操作，将该文件从源文件变为汇编文件，最后生成
目标文件，这里可以分别查看每个阶段这段代码在输出文件中如何布局

###### 预处理

开发者可以参考如下命令获得对应的预处理文件:

{% highlight bash %}
cd BiscuitOS/output/linux-5.0-arm32/linux/linux

make ARCH=arm CROSS_COMPILE=BiscuitOS/output/linux-5.0-arm32/arm-linux-gnueabi/arm-linux-gnueabi/bin/arm-linux-gnueabi- drivers/BiscuitOS/pushsection.i
or
BiscuitOS/output/linux-5.0-arm32/arm-linux-gnueabi/arm-linux-gnueabi/bin/arm-linux-gnueabi-gcc -E pushsection.c -o pushsection.i
{% endhighlight %}

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/HK000206.png)

从上图看出，预处理阶段并为对 .pushsection 有过任何处理.

###### 汇编

开发者可以参考如下命令获得对应的汇编文件:

{% highlight bash %}
cd BiscuitOS/output/linux-5.0-arm32/linux/linux

make ARCH=arm CROSS_COMPILE=BiscuitOS/output/linux-5.0-arm32/arm-linux-gnueabi/arm-linux-gnueabi/bin/arm-linux-gnueabi- drivers/BiscuitOS/pushsection.s
or
BiscuitOS/output/linux-5.0-arm32/arm-linux-gnueabi/arm-linux-gnueabi/bin/arm-linux-gnueabi-gcc -S pushsection.c
{% endhighlight %}

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/HK000207.png)

从上面的汇编结果可以看出，在源文件中的内嵌汇编被原封不动的搬到了
汇编文件中，这得力于 volatile 关键字，阻止了编译器对函数的优化。

###### 目标文件

{% highlight bash %}
cd BiscuitOS/output/linux-5.0-arm32/linux/linux

# Compiler
make ARCH=arm CROSS_COMPILE=BiscuitOS/output/linux-5.0-arm32/arm-linux-gnueabi/arm-linux-gnueabi/bin/arm-linux-gnueabi- drivers/BiscuitOS/pushsection.o
or
BiscuitOS/output/linux-5.0-arm32/arm-linux-gnueabi/arm-linux-gnueabi/bin/arm-linux-gnueabi-gcc -c pushsection.o

# Dump object-file
BiscuitOS/output/linux-5.0-arm32/arm-linux-gnueabi/arm-linux-gnueabi/bin/arm-linux-gnueabi-objdump -sSdhx pushsection.o > output.file
{% endhighlight %}

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/HK000208.png)

目标文件的 SECTION 中可以看出 .biscuitos section 的存在，section 的属性
包含了 ALLOC、LOAD、RELOC、READONLY、CODE，符合 .pushection 添加的 section
的属性。

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/HK000209.png)
![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/HK000210.png)

在目标文件中, .biscuitos section 只包含了之前添加的代码，不包含其他
代码，这里值得注意的是 .biscuitos 虽然是代码段，但不属于 .text section。

通过上面的分析，当需要向目标文件中添加一个新的 section，那么可以使用
.pushsection/.popsection 组合实现。

----------------------------------

<span id="B0"></span>

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND00000H.jpg)

#### 指令实践

> - [实践准备](#B00)
>
> - [实践安装](#B01)
>
> - [实践运行](#B02)
>
> - [实践分析](#B03)
>
> - [实践总结](#B04)

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND000101.jpg)

------------------------------------------------

###### <span id="B00">实践准备</span>

本例基于 BiscuitOS linux 5.0 进行实践，BiscuitOS linux 5.0 开发环境
部署请参考下文:

> - [BiscuitOS linux 5.0 arm32 开发环境部署](https://biscuitos.github.io/blog/Linux-5.0-arm32-Usermanual/)

------------------------------------------------

###### <span id="B01">实践安装</span>

BiscuitOS 支持 .pushsection 指令的快速实践，开发者基于 BiscuitOS 使用
如下命令:

{% highlight bash %}
cd BiscuitOS
make linux-5.0-arm32_defconfig
make menuconfig
{% endhighlight %}

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/HK000170.png)

进入 Kbuild 的配置界面之后，选择并进入 "Package --->",

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/HK000211.png)

接着选择并进入 "ARM Assembly Instruction Sets  --->"

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/HK000212.png)

接着选择 ".pushsection  --->" 选项和，此时已经选择完毕，保存并退出。
配置完毕之后，使用接下来的命令进行项目安装.

{% highlight bash %}
cd BiscuitOS
make
{% endhighlight %}

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/HK000173.png)

通过上图可以看到实践所需的项目已经安装到指定位置，从图中可以看出
源码位于 "BiscuitOS/output/linux-5.0-arm32/package" 目录下，如下:

{% highlight bash %}
BiscuitOS/output/linux-5.0-arm32/package/pushsection-0.0.1
{% endhighlight %}

BiscuitOS 已经支持完整的驱动编译机制，开发者只需要使用简单的命令
便可快速实践驱动。同理，CMA 驱动的实践步骤如下:

{% highlight bash %}
cd BiscuitOS/output/linux-5.0-arm32/package/pushsection-0.0.1
make prepare
make download
make
make install
make pack
{% endhighlight %}

至此 .pushsection 驱动程序安装完毕.

------------------------------------------------

###### <span id="B02">实践运行</span>

准备好前面几个内容之后，现在可以在 BiscuitOS 中使用 .pushsection 驱动，
具体命令如下:

{% highlight bash %}
cd BiscuitOS/output/linux-5.0-arm32/
./RunBiscuitOS.sh
{% endhighlight %}

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/HK000213.png)

运行 BiscuitOS 之后，安装对应的驱动文件，参考使用如下命令:

{% highlight bash %}
cd /lib/modules/5.0.0/extra
insmod asm.ko
lsmod
{% endhighlight %}

驱动安装成功之后，会在 /sys/devices/platform/pushsection.1 生成两个
文件节点 "aligned" 和 "unaligned", 通过读取两个节点的值可以调用到
.pushsection 所在的函数，两个节点会传入不同的值给 .pushsection 所在
的函数。支持实践运行完毕。

------------------------------------------------

###### <span id="B03">实践分析</span>

实践驱动源码位于:

{% highlight bash %}
BiscuitOS/output/linux-5.0-arm32/package/pushsection-0.0.1/pushsection-0.0.1/asm.c
{% endhighlight %}

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/HK000214.png)

驱动的基础逻辑是向系统注册一个设备，设备在 "/sys/devices/platform/pushsection.1" 目录下创建两个文件节点 "aligned" 和 "unaligned", 当使用 cat 命令读取
"aligned" 节点的时候，函数会调用驱动程序中的 "aligned_show()" 函数:

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/HK000215.png) 

aligned_show() 函数的实现很简单，调用 kzalloc() 创建一个 unsigned long
的内存给名为 addr 的 unsigned long 指针。函数并将 addr 指向的内容赋值为
0x8976, 接着函数将 addr 指针传入 load_unaligned() 函数，并肩函数的返回
值存储在 value 变量中以便打印时使用。函数最后释放之前申请的内存，最后
返回 0.

当使用 cat 命令读取 "unaligned" 节点的时候，函数会调用驱动程序中的
"unaligned_show()" 函数:

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/HK000216.png)

"unaligned_show()" 函数的实现与 "aligned_show()" 函数大体一致，只是
当向 load_unaligned() 函数传递参数的时候，"unaligned_show()" 函数传递
了 "addr" 指针地址加 1 的值给函数，这个值也就是一个未对齐的地址。其他
函数实现一致。

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/HK000217.png)

load_unaligned() 函数是驱动的核心实现，其实现逻辑是使用一个内嵌汇编，
在内嵌汇编的 1 处定义了一条指令 "ldr %0, [%1]", 这条指令的意思是从
从 addr 参数对应的地址上读取其值存储到 ret 变量里。接着内嵌汇编调用
.pushsection 在驱动的目标文件中定义了一个名为 ".text.fixup" 的 section，
在 section 内部按 2 字节对齐，并加入一行代码 "mov %0, #0x89" 用于将
立即数 0x89 存储在 ret 中，接着添加代码 "b 2b", 这句代码的意思就是
从 .text.fixup section 的当前位置跳转到 .text section 的 2b 的位置，
也就是途中 2 标号的位置。内嵌汇编继续调用 .pushsection 指令，在当前
目标文件中添加名为 "__ex_table" section，section 属性是 "ax" , 即
可分配和可执行，__ex_table section 内部按 3 字节对齐，并定义了两个
long 变量，分别存储 1b 和 3b 的地址。

如果只从代码表面是很难理解这么做的理由，这里简要介绍一些额外的内容。
在 arm 中，使用 struct exception_table_entry 结构，其定义如下:

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/HK000218.png)

struct exception_table_entry 结构定义了一个异常入口表，insn
指向一条指令地址，fixup 指向另外一条指令。系统将内核所有的
异常入口表都维护在 "__ex_table" section 内部，其逻辑是只要
insn 处指令引起 exception，那么系统就会执行 fixup 处的代码。
fixup 处的代码必须放在 ".text.fixup" section 内部，
struct exception_table_entry 结果必须在 "__ex_table" section
内部，因此这里使用 .pushsection 将代码塞到对应的 section。

再回到上图的代码，这里设计了一个很简单的逻辑，当对指针进行
访问的时候，如果指针地址没有对齐，就会触发一个不对齐的异常，
此时系统就会去找这个异常对应的异常处理程序。因此上图中的代码
就是利用这个逻辑，当在 /sys/devices/platform/pushsection.1
目录下读取 aligned 节点，那么驱动传递一个对齐的参数给
load_unaligned() 函数，1 处没有触发异常，因此函数仅仅是读取
addr 参数对应的值; 反之读取 unaligned 节点是，驱动传递了一个
不对齐的地址 addr 给 load_unaligned() 函数，当执行 1 处的代码
时，系统检查到一个不对齐指针，那么触发不对齐异常处理程序，
那么此处异常处理程序就是 3 标签对应的代码，即将 0x89 传递
给 ret 变量，因此读取 unaligned 的结果就是 0x89, 而 aligned
的结果是 0x8976。

------------------------------------------------

###### <span id="B04">实践总结</span>

经过上面步骤之后，基本也知道 .pushsection 如何使用了，如果要深入了解
更多内容，还需了解链接脚本和链接相关的内容。.pushsection 为增加
一个 section 提供了很多便捷，开发过程中可以将很多代码塞到指定的
section 之后，然后在特殊时候使用。

-----------------------------------------------

# <span id="Y0">附录</span>

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

---
layout: post
title:  "STARTUP"
date:   2018-12-29 15:11:30 +0800
categories: [MMU]
excerpt: LD scripts key STARTUP.
tags:
  - Linker
---

# STARTUP

{% highlight ruby %}
STARTUP(filename)
{% endhighlight %}

指定 filename 为第一个输入文件。在链接过程中，每个输入文件是有顺序的，此命令设
置文件 filename 为第一个输入文件。

**e.g.** 三个源文件 DemoA.c，DemoB.c 和 DemoC.c，其中 DemoA.c 引用 DemoA.c 和 
DemoB.c 里面的函数，使用 GCC 生成独立的目标文件 DemoA.o，DemoB.o 和 DemoC.o。 
ld 使用链接脚本 Demo.lds, 并且在 Demo.lds 里面使用 STARTUP 关键字，分别做三次
链接，每次 STARTUP 指定的输入文件不同：

DemoA.c

{% highlight ruby %}
extern void print();
extern void exit();

void nomain()
{
    print();
    exit();
}
{% endhighlight %}

DemoB.c

{% highlight ruby %}
char *str = "Hello World\n";

void print()
{
    __asm__ ("movl $13, %%edx\n\t"
             "movl %0, %%ecx\n\t"
             "movl $0, %%ebx\n\t"
             "movl $4, %%eax\n\r"
             "int $0x80\n\t"
             :: "r" (str) : "edx", "ecx", "ebx");
}
{% endhighlight %}

DemoC.c

{% highlight ruby %}
void exit()
{
    __asm__ ("movl $42, %ebx\n\t"
             "movl $1, %eax\n\t"
             "int $0x80\n\t");
}
{% endhighlight %}

Demo.lds : DemoA.o 为第一个输入文件

{% highlight ruby %}
ENTRY(nomain)

INPUT(DemoB.o)
INPUT(DemoC.o)
STARTUP(DemoA.o)

SECTIONS
{
    . = 0x08048000 + SIZEOF_HEADERS;

    Demotext ：{ *(.text) *(.data) *(.rodata) }
}
{% endhighlight %}

使用如下命令进行编译和链接：

{% highlight ruby %}
gcc DemoA.c -c 
gcc DemoB.c -c 
gcc DemoC.c -c -fno-builtin 
ld -static -T Demo.lds -o a.out
{% endhighlight %}

链接成功之后生成 mvDemo 可执行文件，使用 objdump 工具查看 a.out 的 ELF 文件布
局：

{% highlight ruby %}
objdump -sSdhx a.out
{% endhighlight %}

![LD](https://raw.githubusercontent.com/EmulateSpace/PictureSet/master/BiscuitOS/kernel/MMU000493.png)

从上面的数据可知，在链接脚本中指定了 DemoA.o 为第一个输入文件之后，输出文件的 
Demotext 段开始的代码就是 DemoA.o .text 的代码段。

Demo.lds : DemoB.o 为第一个输入文件

{% highlight ruby %}
ENTRY(nomain)

STARTUP(DemoB.o)
INPUT(DemoC.o)
INPUT(DemoA.o)

SECTIONS
{
    . = 0x08048000 + SIZEOF_HEADERS;

    Demotext ：{ *(.text) *(.data) *(.rodata) }
}
{% endhighlight %}

使用如下命令进行编译和链接：

{% highlight ruby %}
gcc DemoA.c -c 
gcc DemoB.c -c 
gcc DemoC.c -c -fno-builtin 
ld -static -T Demo.lds -o a.out
{% endhighlight %}

链接成功之后生成 mvDemo 可执行文件，使用 objdump 工具查看 a.out 的 ELF 文件布
局：

{% highlight ruby %}
objdump -sSdhx a.out
{% endhighlight %}

![LD](https://raw.githubusercontent.com/EmulateSpace/PictureSet/master/BiscuitOS/kernel/MMU000494.png)

从上面的数据可知，在链接脚本中指定了 DemoB.o 为第一个输入文件之后，输出文件的 
Demotext 段开始的代码就是 DemoB.o .text 的代码段。

Demo.lds : DemoC.o 为第一个输入文件

{% highlight ruby %}
ENTRY(nomain)

INPUT(DemoB.o)
STARTUP(DemoC.o)
INPUT(DemoA.o)

SECTIONS
{
    . = 0x08048000 + SIZEOF_HEADERS;

    Demotext ：{ *(.text) *(.data) *(.rodata) }
}
{% endhighlight %}

使用如下命令进行编译和链接：

{% highlight ruby %}
gcc DemoA.c -c 
gcc DemoB.c -c 
gcc DemoC.c -c -fno-builtin 
ld -static -T Demo.lds -o a.out
{% endhighlight %}

链接成功之后生成 mvDemo 可执行文件，使用 objdump 工具查看 a.out 的 ELF 文件布
局：

{% highlight ruby %}
objdump -sSdhx a.out
{% endhighlight %}

![LD](https://raw.githubusercontent.com/EmulateSpace/PictureSet/master/BiscuitOS/kernel/MMU000495.png)

从上面的数据可知，在链接脚本中指定了 DemoC.o 为第一个输入文件之后，输出文件的 
Demotext 段开始的代码就是 DemoC.o .text 的代码段。

STARTUP 指令在 bootloader 等需要明确顺序链接的的情况下适用。

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

## 赞赏一下吧 🙂

![MMU](https://raw.githubusercontent.com/EmulateSpace/PictureSet/master/BiscuitOS/kernel/HAB000036.jpg)

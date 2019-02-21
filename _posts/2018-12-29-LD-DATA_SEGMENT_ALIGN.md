---
layout: post
title:  "DATA_SEGMENT_ALIGN"
date:   2018-12-29 18:00:30 +0800
categories: [MMU]
excerpt: LD scripts key DATA_SEGMENT_ALIGN.
tags:
  - Linker
---

# DATA_SEGMENT_ALIGN

{% highlight ruby %}
DATA_SEGMENT_ALIGN(maxpagesize, commonpagesize)
{% endhighlight %}

等价于：

{% highlight ruby %}
(ALIGN(maxpagesize) + (. & (maxpagesize - 1)))
或者
(ALIGN(maxpagesize) + ((. + commonpagesize - 1) & (maxpagesize - commonpagesize)))
{% endhighlight %}

取决于后面数据段 (位于此表达式结果之后以及 DATA_SEGMENT_END 之间) 是否使用比前
面更小的 commonpagesize 大小的页。如果后面的形式被使用，表示着保存 
commonpagesize 字节的运行时内存时，花费的代价最多浪费 commonpagesize 大小的磁
盘空间。

此表达式仅仅能直接使用在 SECTIONS 命令中，不能再任何输出段描述里，且只能在链接
脚本内出现一次。commonpagesize 应当小于或等于 maxpagesize 且应当为目标希望的最
合适的系统页面大小

一个简单例子

{% highlight ruby %}
SECTIONS
{
    . = DATA_SEGMENT_ALIGN(CONSTANT(MAXPAGESIZE),CONSTANT(COMMONPAGESIZE));
    .data : { *(.data) }
}
{% endhighlight %}

e.g. 三个源文件 DemoA.c，DemoB.c 和 DemoC.c，其中 DemoA.c 引用 DemoA.c 和 
DemoB.c 里面的函数，使用 GCC 生成独立的目标文件 DemoA.o，DemoB.o 和 DemoC.o。
ld 使用链接脚本 Demo.lds, 并且在 Demo.lds 里面通过使用 DATA_SEGMENT_ALIGN 关
键字对输出段进行对齐：

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

Demo.lds 

{% highlight ruby %}
ENTRY(nomain)

INPUT(DemoA.o)
INPUT(DemoB.o)
INPUT(DemoC.o)

SECTIONS
{
    . = 0x08048000 + SIZEOF_HEADERS;

    DemoText  : { *(.text) }

    . = 0x08049001;

    . = DATA_SEGMENT_ALIGN(CONSTANT(MAXPAGESIZE),CONSTANT(COMMONPAGESIZE));

    DemoData : { *(.data) }

    DemoRD : { *(.rodata) }

    .eh_frame : { *(.eh_frame) }

    /DISCARD/ : { *(.comment) }
}
{% endhighlight %}

使用如下命令进行编译和链接：

{% highlight ruby %}
gcc DemoA.c -c
gcc DemoB.c -c
gcc DemoC.c -c -fno-builtin
ld -static -T Demo.lds -o a.out
objdump -xSsdh a.out
{% endhighlight %}

![LD](https://raw.githubusercontent.com/EmulateSpace/PictureSet/master/BiscuitOS/kernel/BUD000028.png)

通过运行的数据可知，输出 DemoData 的 VMA 地址就是 0x0804a004，而不是 
0x08049001。

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

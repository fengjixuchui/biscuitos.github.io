---
layout: post
title:  "CONSTANT"
date:   2018-12-29 17:53:30 +0800
categories: [MMU]
excerpt: LD scripts key CONSTANT.
tags:
  - Linker
---

# CONSTANT

可以使用 CONSTANT 关键字引用一个目标特定的常数。语法如下：

{% highlight ruby %}
CONSTANT(name)
{% endhighlight %}

name 可以为如下值：

> 1. MAXPAGESIZE: 目标的页大小
>
> 2. COMMONPAGESIZE：目标的默认也大小


一个简单例子

{% highlight ruby %}
SECTIONS
{
    .text ALIGN(CONSTANT(MAXPAGESIZE)) : { *(.text) }
}
{% endhighlight %}

**e.g.** 三个源文件 DemoA.c，DemoB.c 和 DemoC.c，其中 DemoA.c 引用 DemoA.c 和
DemoB.c 里面的函数，使用 GCC 生成独立的目标文件 DemoA.o，DemoB.o 和 DemoC.o。
ld 使用链接脚本 Demo.lds, 并且在 Demo.lds 里面通过使用 CONSTANT 关键字对输出段
进行对齐：

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

    . = 0x08049001;

    DemoText ALIGN(CONSTANT(MAXPAGESIZE)) : { *(.text) }

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

![LD](https://raw.githubusercontent.com/EmulateSpace/PictureSet/master/BiscuitOS/kernel/BUD000027.png)

通过运行的数据可知，输出 DemoText 的 VMA 地址对齐到 0x0804a000，而不是 
0x08049001。输出 DemoData 的 VMA 地址对齐到了 0x08051000，而不是 0x08050001。

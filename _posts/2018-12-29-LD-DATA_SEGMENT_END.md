---
layout: post
title:  "DATA_SEGMENT_END"
date:   2018-12-29 18:04:30 +0800
categories: [MMU]
excerpt: LD scripts key DATA_SEGMENT_END.
tags:
  - Linker
---

# DATA_SEGMENT_END

{% highlight ruby %}
DATA_SEGMENT_END(exp)
{% endhighlight %}

此命令为 DATA_SEGMENT_ALIGN 运算定义数据段的结尾

一个简单例子

{% highlight ruby %}
SECTIONS
{
    . = DATA_SEGMENT_ALIGN(CONSTANT(MAXPAGESIZE),CONSTANT(COMMONPAGESIZE));
    .data : { *(.data) }
    . = DATA_SEGMENT_END(.);
}
{% endhighlight %}

**e.g.** 三个源文件 DemoA.c，DemoB.c 和 DemoC.c，其中 DemoA.c 引用 DemoA.c 和
DemoB.c 里面的函数，使用 GCC 生成独立的目标文件 DemoA.o，DemoB.o 和 DemoC.o。
ld 使用链接脚本 Demo.lds, 并且在 Demo.lds 里面通过使用 DATA_SEGMENT_END 关键字
对数据段进行处理：

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

    . = DATA_SEGMENT_END(.);

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

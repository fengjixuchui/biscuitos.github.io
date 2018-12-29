---
layout: post
title:  "ONLY_IF_RW"
date:   2018-12-29 17:49:30 +0800
categories: [MMU]
excerpt: LD scripts key ONLY_IF_RW.
tags:
  - Linker
---

# ONLY_IF_RW

ONLY_IF_RW 关键字用于表示在所有输入文件的 section 为可读可写的时候，输出 
section 才能生成。语法如下：

{% highlight ruby %}
output_section  :  ONY_IF_RW { *(.rodata) }
{% endhighlight %}

一个简单例子

{% highlight ruby %}
SECTIONS
{
    .data : ONLY_IF_RW
    { 
        *(.data) 

    }
}
{% endhighlight %}

**e.g.** 三个源文件 DemoA.c，DemoB.c 和 DemoC.c，其中 DemoA.c 引用 DemoA.c 和 
DemoB.c 里面的函数，使用 GCC 生成独立的目标文件 DemoA.o，DemoB.o 和 DemoC.o。 
ld 使用链接脚本 Demo.lds, 并且在 Demo.lds 里面通过使用 ONLY_IF_RW 关键字生成
一个可读可写的输出 section：

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

    DemoText : { *(.text) }

    DemoData : ONLY_IF_RW
    { *(.data) }

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

![LD](https://raw.githubusercontent.com/EmulateSpace/PictureSet/master/BiscuitOS/kernel/BUD000026.png)

通过运行的数据可知，输出 DemoDATA section 只包含输入文件的可读可写 section。



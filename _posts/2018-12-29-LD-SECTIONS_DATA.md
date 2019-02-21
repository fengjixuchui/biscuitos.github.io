---
layout: post
title:  "SECTIONS DATA"
date:   2018-12-29 18:30:30 +0800
categories: [MMU]
excerpt: LD scripts key SECTIONS DATA.
tags:
  - Linker
---

能够在输出 section 中显式的填入指定的信息，可以使用如下关键字：

> 1. BYTE(EXPRESSION)      1 字节   
>
> 2. SHORT(EXPRESSION)     2 字节
>
> 3. LONG(EXPRESSION)      4 字节
>
> 4. QUAD(EXPRESSION)      8 字节
>
> 5. SQUAD(EXPRESSION)     64 位系统时是 8 字节

输出文件的字节顺序 big endiannesss 或 little endianness，可以由输出目标文件的
格式决定。如果输出目标文件的格式不能决定字节序，那么字节顺序与第一个输入文件的
字节序相同

一个简单的例子：

{% highlight ruby %}
SECTIONS
{
    .text : { *(.text) LONG(1) }
    .data : { *(.data) }
}
{% endhighlight %}

**e.g.** 三个源文件 DemoA.c，DemoB.c 和 DemoC.c，其中 DemoA.c 引用 DemoA.c 和
DemoB.c 里面的函数，使用 GCC 生成独立的目标文件 DemoA.o，DemoB.o 和 DemoC.o。
ld 使用链接脚本 Demo.lds, 并且在 Demo.lds 里面通过 BYTE, SHORT 等关键字在输出
文件中的特定 section 中添加数据：

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

    DemoText ： { *(.text) *(.rodata) }

    DemoData : { 
        *(.data) 
        BYTE(1) 
        SHORT(1)
        LONG(1)
        QUAD(1)
        SQUAD(1)
    }

    /DISCARD/ : { *(.comment) }
}
{% endhighlight %}

使用如下命令进行编译和链接：

{% highlight ruby %}
gcc DemoA.c -c
gcc DemoB.c -c
gcc DemoC.c -c -fno-builtin
ld -static -T Demo.lds -o a.out -M
{% endhighlight %}

![LD](https://raw.githubusercontent.com/EmulateSpace/PictureSet/master/BiscuitOS/kernel/MMU000513.png)

通过上面的运行数据可知，输出文件的 DemoData section 中已经被放入了指定的数据，
如 0x08048140 就被放入一个字节的数据。

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

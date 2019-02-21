---
layout: post
title:  "SECTIONS LMA"
date:   2018-12-29 18:24:30 +0800
categories: [MMU]
excerpt: LD scripts key SECTIONS LMA.
tags:
  - Linker
---

e.g. 三个源文件 DemoA.c，DemoB.c 和 DemoC.c，其中 DemoA.c 引用 DemoA.c 和 
DemoB.c 里面的函数，使用 GCC 生成独立的目标文件 DemoA.o，DemoB.o 和 DemoC.o。
ld 使用链接脚本 Demo.lds, 并且在 Demo.lds 里面指定多个段的 LMA 地址：

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

SECTIONS
{
    . = 0x08048000 + SIZEOF_HEADERS;

    DemoText ：AT(0x0858000) { *(.text) }

    DemoData ： AT(ADDR(DemoText) + SIZEOF(DemoText)) {
        *(.data) *(.rodata)
    }

    /DISCARD/ : { *(.comment) }
}
{% endhighlight %}

使用如下命令进行编译和链接：

{% highlight ruby %}
gcc DemoA.c -c
gcc DemoB.c -c
gcc DemoC.c -c -fno-builtin
ld -static -T Demo.lds DemoA.o DemoB.o DemoC.c -o a.out
{% endhighlight %}

链接成功之后生成 a.out 可执行文件，使用 objdump 工具查看 a.out 的 ELF 文件布局：

{% highlight ruby %}
objdump -x a.out
{% endhighlight %}

![LD](https://raw.githubusercontent.com/EmulateSpace/PictureSet/master/BiscuitOS/kernel/MMU000505.png)

通过上面数据可以知道 DemoText 段的 LMA 地址变成了 0x08058000， DemoData 段的 
LMA 地址变成了 0x08480d4。所以 AT() 函数可以改变输出段的 LMA 值。

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

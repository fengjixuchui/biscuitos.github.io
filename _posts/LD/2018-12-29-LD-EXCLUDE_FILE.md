---
layout: post
title:  "EXCLUDE_FILE"
date:   2018-12-29 16:02:30 +0800
categories: [MMU]
excerpt: LD scripts key EXCLUDE_FILE.
tags:
  - Linker
---

# EXCLUDE_FILE

{% highlight ruby %}
EXCLUDE_FILE(FILENAME1 FILENAME2)
{% endhighlight %}

这个关键字用于输入段描述中，剔除指定的输入文件。

一个简单的例子：

{% highlight ruby %}
SECTIONS
{
    DemoEH : { *(EXCLUDE_FILE(DemoA.o DemoB.o) .eh_frame) }
}
{% endhighlight %}

**e.g.** 三个源文件 DemoA.c，DemoB.c 和 DemoC.c，其中 DemoA.c 引用 DemoA.c 和 
DemoB.c 里面的函数，使用 GCC 生成独立的目标文件 DemoA.o，DemoB.o 和 DemoC.o。
ld 使用链接脚本 Demo.lds, 并且在 Demo.lds 里面通过 EXCLUDE_FILE 关键去除 
DemoB.o 和 DemoC.o 中的 .eh_frame section：

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

    DemoText ： { *(.text) *(.data) *(.rodata) }

    DemoEH : { *(EXCLUDE_FILE(DemoB.o DemoC.o) .eh_frame) }

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

![LD](/assets/PDB/BiscuitOS/kernel/MMU000512.png)

通过上面的运行数据可知，输出文件的 DemoEH section 没有包含 DemoB.o 和 DemoC.o 
的 .eh_frame section， 而是被输出文件的 .eh_frame section 包含。DemoEH 中包含
了剩余输入文件的所有 .eh_frame sections。

-----------------------------------------------

# <span id="附录">附录</span>

> [BiscuitOS Home](https://biscuitos.github.io/)
>
> [BiscuitOS Driver](/blog/BiscuitOS_Catalogue/)
>
> [BiscuitOS Kernel Build](/blog/Kernel_Build/)
>
> [Linux Kernel](https://www.kernel.org/)
>
> [Bootlin: Elixir Cross Referencer](https://elixir.bootlin.com/linux/latest/source)

## 赞赏一下吧 🙂

![MMU](/assets/PDB/BiscuitOS/kernel/HAB000036.jpg)

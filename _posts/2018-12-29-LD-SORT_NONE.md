---
layout: post
title:  "SORT_NONE"
date:   2018-12-29 17:38:30 +0800
categories: [MMU]
excerpt: LD scripts key SORT_NONE.
tags:
  - Linker
---

# SORT_NONE

{% highlight ruby %}
SORT_NONE(...)
{% endhighlight %}

SORT_NONE 关键字忽略 ld 命令行对满足字符串模式的所有名字进行递增排序的要求。

**e.g.** 三个源文件 DemoA.c，DemoB.c 和 DemoC.c，其中 DemoA.c 引用 DemoA.c 和
DemoB.c 里面的函数，使用 GCC 生成独立的目标文件 DemoA.o，DemoB.o 和 DemoC.o。
ld 使用链接脚本 Demo.lds, 并且在 Demo.lds 里面使用 SORT_NONE 对输入 section 禁
止排序：

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

INPUT(DemoB.o)
INPUT(DemoA.o)
INPUT(DemoC.o)

SECTIONS
{
    . = 0x08048000 + SIZEOF_HEADERS;

    Demotext ：{ SORT_NONE(*)(.text) *(.data) *(.rodata) }

    /DISCARD/ : { *(.comment) }
}
{% endhighlight %}

另外使用 SORT 的 DemoA.lds

{% highlight ruby %}
ENTRY(nomain)

INPUT(DemoB.o)
INPUT(DemoA.o)
INPUT(DemoC.o)

SECTIONS
{
    . = 0x08048000 + SIZEOF_HEADERS;

    Demotext ：{ SORT(*)(.text) *(.data) *(.rodata) }

    /DISCARD/ : { *(.comment) }
}
{% endhighlight %}

使用如下命令进行编译和链接：

{% highlight ruby %}
gcc DemoA.c -c
gcc DemoB.c -c
gcc DemoC.c -c -fno-builtin
ld -static -T Demo.lds -o a.out -M
ld -static -T DemoC.lds -o a.out -M
{% endhighlight %}

链接成功之后生成 a.out 可执行文件，使用 objdump 工具查看 a.out 的 ELF 文件布局：

{% highlight ruby %}
objdump -x a.out
{% endhighlight %}

![LD](https://raw.githubusercontent.com/EmulateSpace/PictureSet/master/BiscuitOS/kernel/MMU000509.png)

使用了 SORT_NONE 之后，DemoText section 的布局顺序是 DemoA.o, DemoB.o 和 
DemoC.o。而不使用 SORT_NONE 的情况如下：

![LD](https://raw.githubusercontent.com/EmulateSpace/PictureSet/master/BiscuitOS/kernel/MMU000508.png)

通过上面数据可知，不使用 SORT_NONE 函数后，DemoText 的链接顺序是 DemoB.o， 
DemoA.o 和 DemoC.o。所以通过对比，SORT 可以排数输入文件。

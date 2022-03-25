---
layout: post
title:  "SECTIONS NAME"
date:   2018-12-29 18:13:30 +0800
categories: [MMU]
excerpt: LD scripts key SECTIONS NAME.
tags:
  - Linker
---

e.g. 三个源文件 DemoA.c，DemoB.c 和 DemoC.c，其中 DemoA.c 引用 DemoA.c 和 
DemoB.c 里面的函数，使用 GCC 生成独立的目标文件 DemoA.o，DemoB.o 和 DemoC.o。 
ld 使用链接脚本 Demo.lds, 并且在 Demo.lds 里面定义一个输出段的名字是 Demotext，
这个段将所有输入文件的 .text, .data,  和 .rodata 都合入到该段里：

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

    Demotext ：{ *(.text) *(.data) *(.rodata) }
}
{% endhighlight %}

使用如下命令进行编译和链接：

{% highlight ruby %}
gcc DemoA.c -c
gcc DemoB.c -c
gcc DemoC.c -c -fno-builtin
ld -static -T Demo.lds DemoA.o DemoB.o DemoC.c -o a.out
{% endhighlight %}

链接成功之后生成 a.out 可执行文件，使用 readelf 工具查看 a.out 的 ELF 文件布局：

{% highlight ruby %}
readelf -Sl a.out
{% endhighlight %}

![LD](/assets/PDB/BiscuitOS/kernel/MMU000502.png)

通过上面的例子可知，可以通过这个方法来指定输出段的名字。

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

![MMU](/assets/PDB/BiscuitOS/kernel/HAB000036.jpg)

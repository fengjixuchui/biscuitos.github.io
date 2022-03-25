---
layout: post
title:  "OUTPUT_FORMAT3"
date:   2018-12-29 15:23:30 +0800
categories: [MMU]
excerpt: LD scripts key OUTPUT_FORMAT3.
tags:
  - Linker
---

# OUTPUT_FORMAT

{% highlight ruby %}
OUTPUT_FORMAT(DEFAULT,BIG,LITTLE)
{% endhighlight %}

定义三种输出文件的格式 (大小端)。若有命令行选项 -EB，则使用第 2 个 BFD 格式；
若有命令行选项 -EL，则使用第 3 个 BFD 格式，否则默认使用第一个 BFD 格式。

**e.g.** 三个源文件 DemoA.c，DemoB.c 和 DemoC.c，其中 DemoA.c 引用 DemoA.c 和 
DemoB.c 里面的函数，使用 GCC 生成独立的目标文件 DemoA.o，DemoB.o 和 DemoC.o。
ld 使用链接脚本 Demo.lds, 并且在 Demo.lds 里面使用 OUTPUT_FORMAT 关键字指定了
输出文件的三种 BFD 格式：

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

OUTPUT_FORMAT("elf32-i386", "elf32-little", "elf32-big")

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

链接成功之后生成 a.out 可执行文件，使用 objdump 工具查看 a.out 的 ELF 文件布
局：

{% highlight ruby %}
objdump -sSdhx a.out
{% endhighlight %}

![LD](/assets/PDB/BiscuitOS/kernel/MMU000497.png)

从上面的数据可知，可看到默认 file format 是 elf32-i386, 所以可以使用 
OUTPUT_FORMAT 指令来指定输出文件的 BFD 格式。开发者也可以使用如下命令使用另外
两种 BFD 格式：

{% highlight ruby %}
# ld -static -T Demo.lds DemoB.o DemoA.o
# file vmDemo
vmDemo: ELF 32-bit LSB executable, Intel 80386, version 1 (SYSV), statically linked, not stripped
#
# ld -static -T Demo.lds DemoB.o DemoA.o -EL
# file vmDemo
vmDemo: ELF 32-bit LSB executable, Intel 80386, version 1 (SYSV), statically linked, not stripped
#
# ld -static -T Demo.lds DemoB.o DemoA.o -EB
ld: warning: could not find any tragets that match endianness requirement
# file vmDemo
vmDemo: ELF 32-bit LSB executable, Intel 80386, version 1 (SYSV), statically linked, not stripped
{% endhighlight %}

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

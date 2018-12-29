---
layout: post
title:  "SEARCH_DIR"
date:   2018-12-29 15:06:30 +0800
categories: [MMU]
excerpt: LD scripts key SEARCH_DIR.
tags:
  - Linker
---

# SEARCH_DIR

{% highlight ruby %}
SEARCH_DIR(PATH)
{% endhighlight %}

定义输入文件的搜索路径。同 ld 的 -L 选项，不过由 -L 指定的路径要比它定义的优先
被收缩到。

**e.g.** 三个源文件 DemoA.c，DemoB.c 和 DemoC.c，其中 DemoA.c 引用 DemoA.c 和
DemoB.c 里面的函数，使用 GCC 生成独立的目标文件 DemoA.o，DemoB.o 和 DemoC.o。
但 DemoA.o 位于 /tmp/Demo 目录下，DemoB.o 和 DemoC.o 位于 /tmp 目录下。 ld 使
用链接脚本 Demo.lds, 并且在 Demo.lds 里面使用 SEARCH_DIR 关键字指定了搜索目录
为 /tmp，并使用 INPUT 关键字指定 DemoB.o 和 DemoC.o 为输入文件：

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

SEARCH_DIR(/tmp)
IUPUT(DemoB.o)
INPUT(DemoC.o)

SECTIONS
{
    . = 0x08048000 + SIZEOF_HEADERS;

    Demotext ：{ *(.text) *(.data) *(.rodata) }
}
{% endhighlight %}

使用如下命令进行编译和链接：

{% highlight ruby %}
gcc DemoA.c -c -o /tmp/DemoA.o
gcc DemoB.c -c -o /tmp/DemoB.o
gcc DemoC.c -c -fno-builtin -o /tmp/DemoC.o
ld -static -T Demo.lds DemoA.o -o a.out
{% endhighlight %}

链接成功之后生成 mvDemo 可执行文件，使用 objdump 工具查看 a.out 的 ELF 文件布
局：

{% highlight ruby %}
objdump -sSdhx a.out
{% endhighlight %}

![LD](https://raw.githubusercontent.com/EmulateSpace/PictureSet/master/BiscuitOS/kernel/MMU000492.png)

从上面的数据可知，在链接脚本中指定了输入文件的搜索路径之后，输入文件 DemoB.o 
和 DemoC.o 能够正确的被找到。因此可以用 SEARCH_DIR 指定输入文件的路径，该关键
字的作用和 ld 的 -L 选项一样，当 -L 选项的优先级更高。

{% highlight ruby %}
ld -static -T Demo.lds -L/home/buddy DemoC.o -o a.out
{% endhighlight %}


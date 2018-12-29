---
layout: post
title:  "CREATE_OBJECT_SYMBOLS"
date:   2018-12-29 16:12:30 +0800
categories: [MMU]
excerpt: LD scripts key CREATE_OBJECT_SYMBOLS.
tags:
  - Linker
---

# CREATE_OBJECT_SYMBOLS

为每个输入文件建立一个符号，符号名为输入文件的名字。每个符号所在的 section 是
出现该关键字的 section。

一个简单的例子：

{% highlight ruby %}
SECTIONS
{
    DemoData : { 
        CREATE_OBJECT_SYMBOLS
        *(.data)
        _edata = ALIGN(0x808765)
    }
}
{% endhighlight %}

**e.g.** 三个源文件 DemoA.c，DemoB.c 和 DemoC.c，其中 DemoA.c 引用 DemoA.c 和 
DemoB.c 里面的函数，使用 GCC 生成独立的目标文件 DemoA.o，DemoB.o 和 DemoC.o。
ld 使用链接脚本 Demo.lds, 并且在 Demo.lds 里面通过 CREATE_OBJECT_SYMBOLS 关键
字，在 DemoData 段内定义一个 _edata 的符号：

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
        CREATE_OBJECT_SYMBOLS
        *(.data) FILL(0x20) 
        _edata = ALIGN(0x08047000)
    }

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

![LD](https://raw.githubusercontent.com/EmulateSpace/PictureSet/master/BiscuitOS/kernel/MMU000517.png)

通过上面的运行数据可知，在输出文件中，_edata 已经被导出称为一个符号。

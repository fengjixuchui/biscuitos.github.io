---
layout: post
title:  "PROVIDE_HIDDEN"
date:   2018-12-29 17:20:30 +0800
categories: [MMU]
excerpt: LD scripts key PROVIDE_HIDDEN.
tags:
  - Linker
---

# PROVIDE_HIDDEN

PROVIDE_HIDDEN 关键字用于定义这类符号：在目标文件内被定义，被隐藏但不会被导
出。所以其它源文件无法引用这个符号。命令格式如下：

{% highlight ruby %}
PROVIDE_HIDDEN(symbol = expression)
{% endhighlight %}

一个简单例子

{% highlight ruby %}
SECTIONS
{
    .data : 
    { 
        *(.data) 
        _edata = .;
        PROVIDE(_edata);
    }
}
{% endhighlight %}

目标文件内引用 _edata 符号，却没有定义它是，_edata 符号对应的地址被定义为 
.text section 之后的第一个字节地址。

{% highlight ruby %}
extern char _edata;
int main()
{
    return 0;
}
{% endhighlight %}

**e.g.** 三个源文件 DemoA.c，DemoB.c 和 DemoC.c，其中 DemoA.c 引用 DemoA.c 和 
DemoB.c 里面的函数，使用 GCC 生成独立的目标文件 DemoA.o，DemoB.o 和 DemoC.o。
ld 使用链接脚本 Demo.lds, 并且在 Demo.lds 里面通过使用 PROVIDE_HIDDEN 关键字隐
藏一个符号在输出文件中：

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

    DemoData : 
    {
        *(.data) *(.rodata)
        PROVIDE_HIDDEN(_edata = .);
    }

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

![LD](https://raw.githubusercontent.com/EmulateSpace/PictureSet/master/BiscuitOS/kernel/MMU000524.png)

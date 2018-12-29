---
layout: post
title:  "EXPRESSION MATCH"
date:   2018-12-29 18:26:30 +0800
categories: [MMU]
excerpt: LD scripts key EXPRESSION MATCH.
tags:
  - Linker
---

**e.g.** 三个源文件 DemoA.c，DemoB.c 和 DemoC.c，其中 DemoA.c 引用 DemoA.c 和
DemoB.c 里面的函数，使用 GCC 生成独立的目标文件 DemoA.o，DemoB.o 和 DemoC.o。
ld 使用链接脚本 Demo.lds, 并且在 Demo.lds 里面对输入 section 的字符串名字进行
限定：

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

    DemoText ： { *(.text) }

    DemoData :  { [A-Z]*(.data) }

    DemoRD : { [a-zA-Z]?(.rodata) }

    DemoElf : { :A(.data) }

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

![LD](https://raw.githubusercontent.com/EmulateSpace/PictureSet/master/BiscuitOS/kernel/MMU000506.png)

通过上面数据可知，DemoDATA 的输入 section 匹配到了相应的 section，但是 DemoRD 
并没有匹配到任何输入文件的 section。开发者可以通过 ld -M 选项查看各 section 的
引用关系：

{% highlight ruby %}
ld -static -T Demo.lds DemoA.o DemoB.o DemoC.c -o a.out -M
{% endhighlight %}

![LD](https://raw.githubusercontent.com/EmulateSpace/PictureSet/master/BiscuitOS/kernel/MMU000507.png)

---
layout: post
title:  "OUTPUT_FORMAT"
date:   2018-12-29 15:19:30 +0800
categories: [MMU]
excerpt: LD scripts key OUTPUT_FORMAT.
tags:
  - Linker
---

# OUTPUT_FORMAT

{% highlight ruby %}
OUTPUT_FORMAT(BFDNAME)
{% endhighlight %}

设置输出文件使用的 BFD 格式。同 ld 选项 -o format BFDNAME, 不过 ld 选项优先级
更高。BFD 格式包括：elf32-i386, a.out-i386-linux, efi-app-ia32, elf32-little, 
elf32-big, elf64-x86-64, elf64-little, elf64-big 等

**e.g.** 三个源文件 DemoA.c，DemoB.c 和 DemoC.c，其中 DemoA.c 引用 DemoA.c 和 
DemoB.c 里面的函数，使用 GCC 生成独立的目标文件 DemoA.o，DemoB.o 和 DemoC.o。
ld 使用链接脚本 Demo.lds, 并且在 Demo.lds 里面使用 OUTPUT_FORMAT 关键字指定了
输出文件的 BFD 格式：

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

OUTPUT_FORMAT("elf32-i386")

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

![LD](https://raw.githubusercontent.com/EmulateSpace/PictureSet/master/BiscuitOS/kernel/MMU000496.png)

从上面的数据可知，可看到 file format 是 elf32-i386, 所以可以使用 
OUTPUT_FORMAT 指令来指定输出文件的 BFD 格式。

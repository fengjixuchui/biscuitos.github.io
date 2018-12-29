---
layout: post
title:  "OVERLAY"
date:   2018-12-29 17:14:30 +0800
categories: [MMU]
excerpt: LD scripts key OVERLAY.
tags:
  - Linker
---

# OVERLAY

{% highlight ruby %}
OVERLAY [START] : [NOCROSSPEFS][AT(LDADDR)]
{
    SECTION1 [ADDRESS][(TYPE)]:[AT(LMA)]
    {
        OUTPUT-SECTION-COMMAND
        OUTPUT-SECTION-COMMAND
        ....
    } [>REGION][AT>LMA_REGION][:PHDR HDR ...][=FILLEXP]
    SECTION2 [ADDRESS][(TYPE)]:[AT(LMA)]
    {
        OUTPUT-SECTION-COMMAND
        OUTPUT-SECTION-COMMAND
        ....
    } [>REGION][AT>LMA_REGION][:PHDR HDR ...][=FILLEXP]
}
{% endhighlight %}

覆盖图描述使两个或多个不同的 section 占用同一块程序地址空间。覆盖图管理代码负
责将 section 的拷入和拷出。考虑这种情况，当某存储块的访问速度比其他内存块要快
时，那么如果将 section 拷到该存储块来执行或访问，那么速度将会有所提高，覆盖图
描述很适合这种情况。

同一覆盖图内的 section 具有相同的 VMA。SECTION2 的 LMA 为 SECTION1 的 LMA 加上
SECTION1 的大小。同理计算 SECTION2,3,4... 的 LMA。 SECTION1 的 LMA 由 该输出段
的 AT() 指定，如果没有指定，那么由 START 关键字指定，如果没有 START，那么由当
前位符号的值决定。

> 1. NOCROSSREFS 关键字指定各 section 之间不能交叉引用，否则报错
>
> 2. START 指定所有 SECTION 的 VMA 地址 
>
> 3. AT 指定了第一个 SECTION 的 LMA 地址

对于 OVERLAY 描述的每个 section，链接器定义两个符号 __load_start_SECNAME 和 
__load_stop_SECNAME，这两个符号的分别代表 SECNAME section 的 LMA 地址开始和结
束。链接器处理完 OVERLAY 描述语句之后，将定位符号的值加上所有覆盖图内 section 
大小的最大值。

输出 section 的 LMA 与 SECTIONS 之间的层级关系：

{% highlight ruby %}
SECTIONS
{
    OVERLAY [START] : [NOCROSSREFS][AT(LDADDR)]
    {
        SECTION1 [ADDRESS][(TYPE)]:[AT(LMA)]
        {
            OUTPUT-SECTION-COMMAND
            OUTPUT-SECTION-COMMAND
            ....
        } [>REGION][AT>LMA_REGION][:PHDR HDR ...][=FILLEXP]
        SECTION2 [ADDRESS][(TYPE)]:[AT(LMA)]
        {
            OUTPUT-SECTION-COMMAND
            OUTPUT-SECTION-COMMAND
            ....
        } [>REGION][AT>LMA_REGION][:PHDR HDR ...][=FILLEXP]
    }
}
{% endhighlight %}

一个简单例子：

{% highlight ruby %}
SECTIONS
{
    OVERLAY 0x10000 : AT(0x4000)
    {
        DemoText  { *(.text) }

        DemoData  { *(.data) }
    } 
}
{% endhighlight %}

DemoText section 和 DemoData section 的 VMA 地址都是 0x1000, DemoText section 
LMA 地址是 0x4000，DemoData section 的 LMA 地址紧跟其后。可以在 C 代码中引用：

{% highlight ruby %}
extern char __load_start_DemoText, __load_stop_DemoText;
{% endhighlight %}

**e.g.** 三个源文件 DemoA.c，DemoB.c 和 DemoC.c，其中 DemoA.c 引用 DemoA.c 和
DemoB.c 里面的函数，使用 GCC 生成独立的目标文件 DemoA.o，DemoB.o 和 DemoC.o.
ld 使用链接脚本 Demo.lds, 并且在 Demo.lds 里面通过 OVERLAY 关键字，并规定 
section 之间不能相互引用。并在 C 代码中引用段的开始和结束位置：

DemoA.c

{% highlight ruby %}
extern void print();
extern void exit();

extern char __load_start_DemoText, __load_stop_DemoText;

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
    OVERLAY 0x8050000 : AT(0x8060000)
    {
        DemoText { *(.text) }

        DemoData { *(.data) *(.rodata) }
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

![LD](https://raw.githubusercontent.com/EmulateSpace/PictureSet/master/BiscuitOS/kernel/MMU000522.png)

通过上面的运行数据可知，DemoText 和 DemoData 的 VMA 地址是相同的，但 LVM 地址
是不同的。

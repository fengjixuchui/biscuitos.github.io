---
layout: post
title:  "EXPRESSION"
date:   2018-12-30 12:14:30 +0800
categories: [MMU]
excerpt: LD scripts key EXPRESSION.
tags:
  - Linker
---

表达式的语法和 C 语言的表达式语法一样，表达式的值都是整形，如果 ld 的运行主机
和生成文件的目标机都是 32 位，则表达式是 32 位数据，否则是 64 位数据。能够在表
达式内使用的值，设置符号的值。表达式格式如下：

{% highlight ruby %}
SYMBOL = EXPRESSION
SYMOBL += EXPRESSION
SYMBOL -= EXPRESSION
SYMBOL *= EXPRESSION
SYMBOL /= EXPRESSION
SYMBOL <<= EXPRESSION
SYMBOL >>= EXPRESSION
SYMBOL &= EXPRESSION
SYMBOL |= EXPRESSION
{% endhighlight %}

一个简单例子

{% highlight ruby %}
SECTIONS
{
    .data : { *(.data) _edata = ABSOLUTE(.); }
}
{% endhighlight %}

这个例子中， _edata 符号的值是 .data section 的末尾值（绝对值，在程序地址空间
内）

链接脚本相关的内建函数：

> 1. ABSOLUTE(EXP): 转换成绝对值
>
> 2. ADDR(SECTION): 返回某 section 的 VMA 值
> 
> 3. ALIGN(EXP): 返回定位符 . 的修调值，对齐后的值，(. + EXP - 1) & ~(EXP - 1)
>
> 4. BLOCK(EXP): 如同 ALIGN(EXP)，为了向前兼容
>
> 5. DEFINED(SYMBOL): 如果符号 SYMBOL 在全局符号表内，且被定义了，那么返回 1，
>    否则返回 0
> 6. LOADADDR(SECTION): 返回某 SECTION 的 LMA
> 
> 7. MAX(EXP1, EXP2): 返回大者
>
> 8. MIN(EXP1,EXP2): 返回小者
>
> 9. NEXT(EXP): 返回下一个能被使用的地址，该地址是 EXP 的倍数，类似于 
>    ALIGN(EXP)。除非使用 MEMORY 命令定义了一些非连续的内存块，否则 NEXT(EXP) 
>    与 ALIGN(一定相同)
>
> 10. SIZEOF(SECTION): 返回 SECTION 的大小。当 SECTION 没有被分配时，即此时 
>     SECTION 大小不能确定，链接器会报错，
>
> 11. SIZEOF_HEADERS: 返回输出文件的文件头大小，用以确定第一个 section 的开始
>     地址

**e.g.** 三个源文件 DemoA.c，DemoB.c 和 DemoC.c，其中 DemoA.c 引用 DemoA.c 和 
DemoB.c 里面的函数，使用 GCC 生成独立的目标文件 DemoA.o，DemoB.o 和 DemoC.o。
ld 使用链接脚本 Demo.lds, 并且在 Demo.lds 里面通过使用多个赋值语句关键字：

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

    DemoText ALIGN(4) : 
    { 
        __load_addr_DemoText = LOADADDR(DemoText);

        *(.text) 
        __end_text_check = MIN(., 0x08060000);
        __end_text_crc = MAX(.,0x08050000);
        __end_text_ek = NEXT(.);
        __emd_text_bk = BLOCK(.);

    }

    DemoData ADDR(DemoText) + SIZEOF(DemoText) : 
    { 
        __data_checkid = DEFINED(_edata_op)? 1: 0;
        *(.data) *(.rodata)

        _edata = ABSOLUTE(.); 
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

![LD](https://raw.githubusercontent.com/EmulateSpace/PictureSet/master/BiscuitOS/kernel/MMU000523.png)

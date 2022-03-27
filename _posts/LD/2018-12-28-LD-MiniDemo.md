---
layout: post
title:  "链接脚本最小实践"
date:   2018-12-28 17:15:30 +0800
categories: [MMU]
excerpt: 链接脚本最小实践.
tags:
  - Linker
---

为了实现最小实践，开发者应该准备好一个源码和一个链接脚本。

### 源码

一个简单的源码如下：

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

void exit()
{
    __asm__ ("movl $42, %ebx\n\t"
             "movl $1, %eax\n\t"
             "int $0x80\n\t");
}

void nomain()
{
    print();
    exit();
}
{% endhighlight %}

从源码可以知道，程序的入口函数为 nomain()，然后该函数调用 print() 函数，打印 
“Hello World”，接着调用 exit() 函数，结束进程。这里的 print 函数使用了 Linux 
的 Write 系统调用，exit() 函数使用了 EXIT 系统调用。函数的核心步骤使用 ATT 内
嵌汇编实现，相关内容可以参考附录 《asm-inline》。这里简单介绍系统调用：系统调
用通过 0x80 中断实现，其中 eax 为调用号，ebx，ecx，edx 等通用寄存器用来传递参
数，比如 WRITE 系统调用是往一个文件句柄写入数据，通过用 C 语言描述它的原型就
是：

{% highlight ruby %}
int write(int filedesc, char *buf, int size);
{% endhighlight %}

> 1. WRITE 调用号为 4， 则 eax = 4
>
> 2. filedesc 表示被写入的文件句柄，使用 ebx 寄存器传递，这里要往默认终端 
>    (stdout) 输出，它的文件句柄为 0，则 ebx = 0
>
> 3. buffer 表示要写入的缓冲区地址，使用 ecx 寄存器传递，这里输入字符串 str，
>    所以 ecx = str
> 4. size 表示要写入的字节数，使用 edx 寄存器传递，字符串“Hello World\n”长度
>    为 13 字节，所以 edx = 13

同理，EXIT 系统调用中，ebx 表示进程退出码 (Exit Code)，比如平时的 main 函数中
的 return 的数值会返回给系统库，由系统库将该数值传递给 EXIT 系统调用。这样父进
程就可以接受到子进程的退出码。EXIT 系统调用的调用号为 1，即 eax = 1。

接下来开发者将源码编译汇编成目标 ELF 文件，并使用默认的链接脚本进行链接。以下
命令均运行在 IA32 平台上，其他平台可以参考运行 (IA32 实践平台搭建方法见：
《IA32 实践平台搭建方法》)。

{% highlight ruby %}
gcc hello.c -c -fno-builtin -o hello.o
ld helloc.o -static -e nomain -o a.out
{% endhighlight %}

> 1. -fno-builtin： GCC 编译器提供了很多内置函数，它会把一些常用的 C 库函数替
>    换成编译器的内置函数，以达到优化的功能。比如 GCC 会将只有字符串参数的 
>    printf 函数替换成 puts，以省略格式解析的时间。exit() 函数也是 GCC 的内置
>    参数之一，所以开发者要使用 -fno-buildin 参数来关闭 GCC 内置函数功能。
>
> 2. -static 这个参数表示 ld 将使用静态链接的方式来链接程序，而不是使用默认的
>    动态链接方式。
>
> 3. -e nomain 表示该程序的入口函数为 nomain， 这个参数就是将 ELF 文件头的 
>    e_entry 成员赋值成 nomain 函数的地址。

通过上面的命令，开发者会得到一个可执行的 ELF 文件，运行它之后会打印 
“Hello World”。开发者也可以使用 objdump 或者 readelf 命令查看 a.out 可执行 
ELF 文件，会发现它包含了 4 个段：.text, .rodata, .data 和 .comment。

{% highlight ruby %}
objdump -sSdhx a.out
{% endhighlight %}

![LD](/assets/PDB/BiscuitOS/kernel/MMU000486.png)

> 1. .text 用于保存的是程序的指令，它是只读的
>
> 2. .rodata 保存的是字符串 "Hello World!\n"，它也是只读的
>
> 3. .data 保存的是 str 全局变量，看上去它是可读的，但开发者并没有在程序中改
>    写该变量，所以实际上它也是只读的。
>
> 4. .comment 保存的是编译器和系统版本信息，这些信息也是只读的。由于 .comment 
>    里面保存的数据并不关键，对于程序的运行没有作用，所以可以将其丢弃。

### 链接脚本

无论是输入文件还是输出文件，它们的主要的数据是在文件中的各个段，把输入文件中的
段称为输入段 (Input Sections)，输出文件中的段称为输出段 (Output Sections)。简
单来讲，控制链接过程无非是控制输入段如何变成输出段，比如那些输入段要合入一个输
出段，哪些输入段需要丢弃；指定输出段的名字，装载地址，属性，等等。下面以一个实
际的链接脚本链接上面的目标文件。(一般链接脚本都以 lds 作为拓展名 ld scripts), 
链接脚本如下：

{% highlight ruby %}
ENTRY(nomain)

SECTIONS
{
    . = 0x08048000 + SIZEOF_HEADERS;

    hellotext : { *(.text) *(.data) *(.rodata) }

    /DISCARD/ : { *(.comment) }
}
{% endhighlight %}

这是一个简单的链接脚本，第一行的 ENTRY(nomain) 指定了程序的入口为 nomain() 函
数；后面的 SECTIONS 命令是链接脚本的主体，这个命令指定了各个输入段到输出段的变
换，SECTIONS 后面紧跟着的一对大括号里面包含了 SECTIONS 变换规则，其中有三条语
句，每条语句一行。第一条语句是赋值语句，后面两条是段转换规则，含义如下：

##### .=0x0848000 + SIZEOF_HEADERS

第一条赋值语句的意思是将当前虚拟地址设置为 0x08048000 _ SIZEOF_HEADERS，
SIZEOF_HEADERS 为输出文件的文件头大小。“.”表示当前虚拟地址，因为这条语句后面紧
跟着输出段 "hellotext" ，所以 “hellotext”段的起始虚拟地址即为 0x08048000 + 
SIZEOF_HEADERS。它将当前虚拟地址设置成一个比较巧妙的值，以便于装载时页面映射更
为方便。

##### hellotext : { *(.text) *(.data) *(.rodata) }

第二条是个段转换规则，它的意思即为所有输入文件中的名字为 ".text", “.data”,
“.rodata”的段依次合并到输出文件的 hellotext 。

##### /DISCARD/ : { *(.comment) }

第三条规则为：将所有输入文件中的名字为 “.comment”的段丢弃，不保存到输出文件中

通过上述两条转换规则，就达到了 a.out 程序的第三个要求：最终输出的可执行文件只
有一个叫 hellotext 的段。开发者可以使用下面的命令来启用链接控制脚本：

{% highlight ruby %}
gcc -c -fno-builtin -o hello.o
ld -static -T hello.lds -o a.out hello.o
{% endhighlight %}

最终，可以得到一个可执行 ELF 文件，并在终端运行之后，输出 “Hello World”。开发
者也可以使用 objdump 或 readelf 命令，查看使用链接脚本生成可执行 ELF 文件的各
段分布，使用如下命令：

{% highlight ruby %}
objdump -sSdhx a.out
{% endhighlight %}

![LD](/assets/PDB/BiscuitOS/kernel/MMU000487.png)

通过对比使用默认链接脚本的 objdump 结果，可以看出：a.out 文件中只有 hellotext 
段，并且原先 .rodata 段的 “Hello World”内容都被放置到 hellotext 段里面了；

至此，最小链接脚本实践完结。

-----------------------------------------

# <span id="附录">附录</span>

> [BiscuitOS Home](https://biscuitos.github.io/)
>
> [BiscuitOS Driver](/blog/BiscuitOS_Catalogue/)
>
> [BiscuitOS Kernel Build](/blog/Kernel_Build/)
>
> [Linux Kernel](https://www.kernel.org/)
>
> [Bootlin: Elixir Cross Referencer](https://elixir.bootlin.com/linux/latest/source)

## 赞赏一下吧 🙂

![MMU](/assets/PDB/BiscuitOS/kernel/HAB000036.jpg)

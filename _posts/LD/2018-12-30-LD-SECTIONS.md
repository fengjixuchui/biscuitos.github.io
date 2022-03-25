---
layout: post
title:  "SECTIONS"
date:   2018-12-30 13:40:30 +0800
categories: [MMU]
excerpt: LD scripts key SECTIONS.
tags:
  - Linker
---

# 目录

> 1. [SECTIONS](#SECTIONS)
>
> 2. [Output section description](#Output section description)
>
> 3. [Section 叠加描述](#Section 叠加描述)

--------------------------------------

# <span id="SECTIONS">SECTIONS</span>

{% highlight ruby %}
SECTIONS
{
    SECTIONS-COMMAND
    SECTIONS-COMMAND
}
{% endhighlight %}

SECTIONS 命令告诉 ld 如何把输入文件的 sections 映射到输出文件的各个 section。
对于如何将输入 section 合为输出 section；以及如何把输出 section 放入程序地址空
间 (VMA) 和进程地址空间 (LMA)。使用方式如上。SECTIONS-COMMAND 有四种，分别为：

> 1. [ENTRY 命令](https://biscuitos.github.io/blog/LD-ENTRY/)
>
> 2. [符号赋值语句](https://biscuitos.github.io/blog/LD-EXPRESSION/)
>
> 3. [输出 section 的描述](#Output section description)
>
> 4. [Section 叠加描述](#Section 叠加描述)

如果整个链接脚本内没有 SECTIONS-COMMAND， 那么 ld 将所有同名输入 section 合为
一个输出 section 内，各输入 section 的顺序为他们被链接器发现的顺序。如果某输入
section 没有在 SECTIONS 命令中提到，那么该 section 将被直接拷贝成输出 section。

------------------------------------------------

# <span id="Output section description">输出 section 的描述</span>

输出 section 描述的具体格式如下：

{% highlight ruby %}
SECTION [ADDRESS][(TYPE)]:[AT(LMA)]
{
    OUTPUT-SECTION-COMMAND
    OUTPUT-SECTION-COMMAND
    ....
} [>REGION][AT>LMA_REGION][:PHDR HDR ...][=FILLEXP]
{% endhighlight %}

一个简单的例子：

{% highlight ruby %}
SECTIONS
{
    DemoText : { *(.text) }
}
{% endhighlight %}

输出 section 描述 与 SECTIONS 之间的关系：

{% highlight ruby %}
SECTIONS
{
    SECTION [ADDRESS][(TYPE)]:[AT(LMA)] {
        OUTPUT-SECTION-COMMAND
        OUTPUT-SECTION-COMMAND
    } [>REGION][AT>LMA_REGION][:PHDR HDR ...][=FILEEXP]
}
{% endhighlight %}

输出段描述包含如下内容：

> [输出 section 名字 : SECTION](#输出 section 名字 : SECTION)
>
> [输出 section VMA 地址 : ADDRESS](#输出 section VMA 地址 : ADDRESS)
>
> [输出 section 类型 : TYPE](#输出 section 类型 : TYPE)
>
> [输出 section LMA 地址 : AT(LMA)](#输出 section LMA 地址)
>
> [输出 section 所在的程序段](#输出 section 所在的程序段)
>
> [输出 section 的填充模板](#输出 section 的填充模板)
>
> [输出 section 描述命令](#输出 section 描述命令)

### <span id="输出 section 名字 : SECTION">输出 section 名字 (SECTION)</span>

{% highlight ruby %}
SECTION [ADDRESS][(TYPE)]:[AT(LMA)]
{
    OUTPUT-SECTION-COMMAND
    OUTPUT-SECTION-COMMAND
    ....
} [>REGION][AT>LMA_REGION][:PHDR HDR ...][=FILLEXP]
{% endhighlight %}

SECTION: section 名字。

输出 section 名字与 SECTIONS 和输出section 之间的层级关系：

{% highlight ruby %}
SECTIONS
{
    SECTION [ADDRESS][(TYPE)]:[AT(LMA)] {
        OUTPUT-SECTION-COMMAND
        OUTPUT-SECTION-COMMAND
    } [>REGION][AT>LMA_REGION][:PHDR HDR ...][=FILEEXP]
}
{% endhighlight %}

输出 section 名字简单例子如下：

{% highlight ruby %}
SECTIONS
{
    DemoText : { *(.text) }
}
{% endhighlight %}

输出 section 名字必须符合输出文件的格式要求，比如： a.out 格式的文件只允许存
在 .text， .data 和 .bss section 名。而有的格式只允许存在数字名字，那么此时应
该用引号将所有名字内核数字组合在一起；另外，还有一些格式运行任何顺序的字符存在
于 section 名字内，此时如果名字内包含特殊字符 (比如空格，逗号等)，那么需要引号
将其组合在一起。

SECTION 左右的空白，圆括号，冒号是必须的，换行符合其他空格是可选的。并且 
SECTION 和冒号之间必须有一个空格，下列就是错误的写法：

{% highlight ruby %}
SECTIONS
{
    DemoText: { *(.text) }
}
{% endhighlight %}

实践请看：[《输出 section 名字 (SECTION) 实践》](https://biscuitos.github.io/blog/LD-SECTIONS_NAME/)

### <span id="输出 section VMA 地址 : ADDRESS">输出 section VMA 地址 (ADDRESS)</span>

{% highlight ruby %}
SECTION [ADDRESS][(TYPE)]:[AT(LMA)]
{
    OUTPUT-SECTION-COMMAND
    OUTPUT-SECTION-COMMAND
    ....
} [>REGION][AT>LMA_REGION][:PHDR HDR ...][=FILLEXP]
{% endhighlight %}

ADDRESS 是一个表达式，它的值用于设置 VMA，VMA 表示可执行程序的地址空间。如果没
有该选项且有 REGION 选项，那么链接器根据 REGION 设置 VMA。如果也没有 REGION 选
项，那么链接器将根据定位符号 “.”的值设置该 section 的 VMA，将定位符号的值调整
到满足输出 section 对其要求的值，输出 section 的对其要求为：该输出 section 描
述内用到的所有输入 section 的对其要求中最严格的。

输出 section VMA 与 SECTIONS 之间的层级关系：

{% highlight ruby %}
SECTIONS
{
    SECTION [ADDRESS][(TYPE)]:[AT(LMA)] {
        OUTPUT-SECTION-COMMAND
        OUTPUT-SECTION-COMMAND
    } [>REGION][AT>LMA_REGION][:PHDR HDR ...][=FILEEXP]
}
{% endhighlight %}

简单例子：

{% highlight ruby %}
SECTIONS
{
    .text               . : { *(.text) }
    .stab        ALIGN(0) : { *(.stab) }
    .stabstr            0 : { *(.stabstr) }
    .comment   0x08058000 : { *(.comment) }
}
{% endhighlight %}

或者 

{% highlight ruby %}
SECTIONS
{
    . = 0x08048000;

    .text : { *(.text) }
}
{% endhighlight %}

这两类描述是截然不同的，第一类将 .text section 的 VMA 设置为指定的值；而第二类
则是设置成定位符的修调值，满足对其要求。注意！设置 ADDRESS 值，将更改定位符号
的值。

实践请看：[《输出 section VMA 地址 (ADDRESS) 实践》](https://biscuitos.github.io/blog/LD-SECTIONS_VMA/)

### <span id="输出 section 类型 : TYPE">输出 section 类型 : TYPE</span>

{% highlight ruby %}
SECTION [ADDRESS][(TYPE)]:[AT(LMA)]
{
    OUTPUT-SECTION-COMMAND
    OUTPUT-SECTION-COMMAND
    ....
} [>REGION][AT>LMA_REGION][:PHDR HDR ...][=FILLEXP]
{% endhighlight %}

每个输出 section 都有一个类型，如果没有指定 TYPE 类型，那么链接器根据输出 
section 引用输入 section 的类型设置该输出 section 的类型，TYPE 可以有五种值：

> 1. NOLOAD: 该 section 在程序运行时，不被载入内存
>
> 2. DESCT： 向后兼容保留下来的，这种类型的 section 必须被标记为 “不可被加载
             的”，以便在程序运行不为它们分配内存。
>
> 3. COPY：向后兼容保留下来的，这种类型的 section 必须被标记为 “不可被加载的”，
>          以便在程序运行不为它们分配内存。
>
> 4. INFO：向后兼容保留下来的，这种类型的 section 必须被标记为 “不可被加载的”，
>          以便在程序运行不为它们分配内存。
>
> 5. OVERLAY：向后兼容保留下来的，这种类型的 section 必须被标记为 “不可被加载
              的”，以便在程序运行不为它们分配内存。

输出 section 类型与 SECTIONS 的层级关系：

{% highlight ruby %}
SECTIONS
{
    SECTION [ADDRESS][(TYPE)]:[AT(LMA)] {
        OUTPUT-SECTION-COMMAND
        OUTPUT-SECTION-COMMAND
    } [>REGION][AT>LMA_REGION][:PHDR HDR ...][=FILEEXP]
}
{% endhighlight %}

一个简单例子如下：

{% highlight ruby %}
SECTIONS
{
    DemoText (NOLOAD) : { *(.comment) }
}
{% endhighlight %}

实践请看：[《输出 section 类型 : TYPE 实践》](https://biscuitos.github.io/blog/LD-SECTIONS_TYPE/)

### <span id="输出 section LMA 地址">输出 section 段 LMA 地址： AT(LMA)</span>

{% highlight ruby %}
SECTION [ADDRESS][(TYPE)]:[AT(LMA)]
{
    OUTPUT-SECTION-COMMAND
    OUTPUT-SECTION-COMMAND
    ....
} [>REGION][AT>LMA_REGION][:PHDR HDR ...][=FILLEXP]
{% endhighlight %}

LMA 为程序的加载地址，默认情况下，LMA 等于 VMA，但可以通过关键字 AT() 指定 LMA。
如果不用 AT() 关键字，那么可以用 AT>LMA_REGION 表达式设置指定该 section 加载地
址的范围。

输出 section 的 LMA 与 SECTIONS 之间的层级关系：

{% highlight ruby %}
SECTIONS
{
    SECTION [ADDRESS][(TYPE)]:[AT(LMA)] {
        OUTPUT-SECTION-COMMAND
        OUTPUT-SECTION-COMMAND
    } [>REGION][AT>LMA_REGION][:PHDR HDR ...][=FILEEXP]
}
{% endhighlight %}

一个简单例子：

{% highlight ruby %}
SECTIONS
{
    DemoText : AT(0x0848000) { *(.text) }

    MEMORY { rom : ORIGIN = 0x1000, LENGTH = 0x1000 }

    DemoData : { *(.data)} > rom
}
{% endhighlight %}

实践请看：[《输出 section 段 LMA 地址： AT(LMA) 实践》](https://biscuitos.github.io/blog/LD-SECTIONS_LMA/) [《MEMORY》](https://biscuitos.github.io/blog/LD-MEMORY/)  [《PHDRS》](https://biscuitos.github.io/blog/LD-PHDRS/)

### <span id="输出 section 所在的程序段">输出 section 所在的程序段</span>

{% highlight ruby %}
SECTION [ADDRESS][(TYPE)]:[AT(LMA)]
{
    OUTPUT-SECTION-COMMAND
    OUTPUT-SECTION-COMMAND
    ....
} [>REGION][AT>LMA_REGION][:PHDR HDR ...][=FILLEXP]
{% endhighlight %}

可以将输出 section 放入预先定义的程序段 (Program Segment) 内。如果某个输出 
section 设置了它所在的一个或多个程序段，那么接下来定义的输出 section 的默认
程序段与该输出 section 的相同。除非再次显示地指定。可以通过 :NONE 指定链接器
不把该 section 放入任何程序段内。

输出 section 的 LMA 与 SECTIONS 之间的层级关系：

{% highlight ruby %}
PHDRS
{
    NAME TYPE [FILEHDR][PHDRS][AT(ADDRESS)][FLAGS(FLAGS)]
}

SECTIONS
{
    SECTION [ADDRESS][(TYPE)]:[AT(LMA)] {
        OUTPUT-SECTION-COMMAND
        OUTPUT-SECTION-COMMAND
    } [>REGION][AT>LMA_REGION][:PHDR HDR ...][=FILEEXP]
}
{% endhighlight %}

一个简单例子：

{% highlight ruby %}
PHDRS
{
    headers PT_PHDR PHDRS;
    interp PT_INTERP;
    text PT_LOAD FILEHDR PHDRS;
    data PT_LOAD;
    dynamic PT_DYNAMIC;
}

SECTIONS
{
    . = 0x8048000 + SIZEOF_HEADERS;
    .interp : { *(.interp) } :text :interp
    .text { *(.text) } :text
   .rodata : { *(.rodata) } /* Defaults to .text */

    ....

    . = . + 0x1000;  /* mov to a new page in memory */
    .data : { *(.data) } :data
    .dynamic : { *(.dynamic) } :data : dynamic

    ....
}
{% endhighlight %}

实践请看：[《PHDRS》](https://biscuitos.github.io/blog/LD-PHDRS/)

### <span id="输出 section 的填充模板">输出 section 的填充模板</span>

{% highlight ruby %}
SECTION [ADDRESS][(TYPE)]:[AT(LMA)]
{
    OUTPUT-SECTION-COMMAND
    OUTPUT-SECTION-COMMAND
    ....
} [>REGION][AT>LMA_REGION][:PHDR HDR ...][=FILLEXP]
{% endhighlight %}

任何输出 section 描述内的未指定的内存区域内的未指定内存区域，链接器用该模板填
充该区域。用法 =FILEEXP，前两字节有效，当区域大于两个字节时，重复使用这两个字
节以其填满

输出 section 的 LMA 与 SECTIONS 之间的层级关系：

{% highlight ruby %}
SECTIONS
{
    SECTION [ADDRESS][(TYPE)]:[AT(LMA)] {
        OUTPUT-SECTION-COMMAND
        OUTPUT-SECTION-COMMAND
    } [>REGION][AT>LMA_REGION][:PHDR HDR ...][=FILEEXP]
}
{% endhighlight %}

一个简单例子：

{% highlight ruby %}
SECTIONS
{
    DemoText : { *(.text) } =0x3456

    DemoData : { *(.data)} 
}
{% endhighlight %}

实践请看：[《FILL》](https://biscuitos.github.io/blog/LD-FILL/)

### <span id="输出 section 描述命令">输出 section 描述命令</span>

{% highlight ruby %}
SECTION [ADDRESS][(TYPE)]:[AT(LMA)]
{
    OUTPUT-SECTION-COMMAND
    OUTPUT-SECTION-COMMAND
    ....
} [>REGION][AT>LMA_REGION][:PHDR HDR ...][=FILLEXP]
{% endhighlight %}

输出 section 描述命令主要包含以下几类内容：

> 1. [输入 section 描述](#输入 section 描述)
>
> 2. [垃圾回收](#垃圾回收)
>
> 3. [输入 section 描述中存放数据命令](#输入 section 描述中存放数据命令)
>
> 4. [输入 section 描述中的命令关键字](#输入 section 描述中的命令关键字)
>
> 5. [输出 section 丢弃](#输出 section 丢弃)

输出 section 描述与 SECTIONS 之间的关系：

{% highlight ruby %}
SECTIONS
{
    SECTION [ADDRESS][(TYPE)]:[AT(LMA)] {
        OUTPUT-SECTION-COMMAND
        OUTPUT-SECTION-COMMAND
    } [>REGION][AT>LMA_REGION][:PHDR HDR ...][=FILEEXP]
}
{% endhighlight %}

一个简单例子：

{% highlight ruby %}
SECTIONS
{
    DemoText : { *(.text) }
}
{% endhighlight %}

#### <span id="输入 section 描述">输出 section 描述命令之：输入 section 描述</span>

最常见的输出 section 命令就是输入 section 描述。输入 section 描述的最基本的链
接脚本描述，基本语法如下：

{% highlight ruby %}
FILENAME(EXCLUDE_FILE(FILENAME1 FILENAME2 ...) SECTION1 SECTION2 ...)
{% endhighlight %}

FILENAME 表示文件名，可以是一个特定的文件的名字，也可以是一个特定的字符串，
例如：

> *(.text): 表示所有输入文件的 .text section

{% highlight ruby %}
SECTIONS
{
    DemoText : { *(.text) }
}
{% endhighlight %}

> (*(EXCLUDE_FILE(*crtend.o *otherfile.o) .ctors)): 表示除 crtend.o 
> otherfile.o 文件外的所有输入文件的 .ctros section。具体实践请看：
> [《EXCLUDE_FILE》](https://biscuitos.github.io/blog/LD-EXCLUDE_FILE/)

{% highlight ruby %}
SECTIONS
{
    DemoCTR ： { (*(EXCLUDE_FILE(*crtend.o *otherfile.o) .ctors)) }
}
{% endhighlight %}

> data.o(.data): 表示 data.o 文件的 .data section

{% highlight ruby %}
SECTIONS
{
    DemoData : { data.o(.data) }
}
{% endhighlight %}

> data.o： 表示 data.o 文件的所有 section

{% highlight ruby %}
SECTIONS
{
    DemoData : { data.o } 
}
{% endhighlight %}

> *(.text .data): 表示所有文件的 .text section 和 .data section，顺序是第一个
> 文件的 .text section, 第一个文件的 .data section， 第二个文件的 .text 
> section, 第二个文件的 .data section, ....

{% highlight ruby %}
SECTIONS
{
    DemoData : { *(.data .data) }
}
{% endhighlight %}

> *(.text) *(.data): 表示所有文件的 .text section 和 .data section， 顺序是第
> 一个文件的 .text section， 第二个文件的 .text section ... 最后一个文件的 
> .text section第一个文件的 .data section，第二个文件的 .data section .... 最
> 后一个文件的 .data section

{% highlight ruby %}
SECTIONS
{
    DemoData : { *(.text) *(.data) }
}
{% endhighlight %}

接着是链接器找到 FILENAME，其遵循下面规则：

> 1. 当 FILENAME 是一个特定的文件名时，链接器会查看它是否在链接命令行内出现
>    或者在 INPUT 命令中出现
>
> 2. 当 FILENAME 是一个字符串模式时，链接器仅仅只查看它是否在链接命令行内出现
>
> 3. 如果链接器发现某个文件在 INPUT 命令内出现，那么优先在链接命令行 -L 指定的
>    路径内搜索该文件

字符串模式内可能存在以下通配符：

> * : 表示任意多个字符

{% highlight ruby %}
SECTIONS
{
    DemoText : { *(.text) }
}
{% endhighlight %}

> ? : 表示任意一个字符

{% highlight ruby %}
SECTIONS
{
    DemoText : { [A-Z]?_src.o(.data) }
}
{% endhighlight %}

> [CHARS] : 表示任意一个 CHARS 内的字符，可用 “-”表示方位，如 a-z

{% highlight ruby %}
SECTIONS
{
    DemoText : { [A-Z]*(.data) }
}
{% endhighlight %}

> : : 表示引用下一个紧跟的字符

{% highlight ruby %}
SECTIONS
{
    DemoText : { :A(.data) }
}
{% endhighlight %}

任何一个输入文件的任意 section 只能在 SECTIONS 命令里出现一次。

实践请看：[《字符串模式通配符实践》](https://biscuitos.github.io/blog/LD-STRING_MATCH/)

可以使用 SORT() 关键字对满足字符串模式的所有名字进行递增排序，如：

{% highlight ruby %}
SECTIONS
{
    DemoText : { SORT(*)(.text) }
}
{% endhighlight %}

实践请看：[《SORT》](https://biscuitos.github.io/blog/LD-SORT/)

通用符号 (common symbol) 的输入 section。 在许多目标文件中，通用符号并没有占用
一个 section。链接器认为输入文件的所有通用符号在名为 COMMON 的 section 内。例如

{% highlight ruby %}
SECTIONS
{
    .bss : { *(.bss) *(COMMON) }
}
{% endhighlight %}

这个例子中将所有的通用符号放入输出 .bss section 内。可以看到 COMMON section 的
使用方法跟其他 section 的使用方法一样的。有些目标文件格式把通用符号分成几类。
例如 MIPS elf 目标文件格式中，把通用符号分成 standard common symbols (标准通用
符号) 和 small common symbols (弱用符号)， 此时链接器认为所有 standard common 
symbols 在 COMMON section 内，而 small common symbols 在 .scommon section 内。
在一些以前的链接脚本内可以看到 [COMMON], 相当于 *(COMMON)， 不建议继续使用这种
陈旧的方式。

#### <span id="垃圾回收">输出 section 描述命令之：垃圾回收</span>

在链接命令行内使用了选项 --gc-section 后，链接器可能将某些它认为没有用的 
section 过滤掉，此时就有必要强制链接器保留一些 section，可用 KEEP 关键字，简
单例子如下：

{% highlight ruby %}
SECTIONS
{
    DemoText : { *(.text) }

    DemoComment : { KEEP(*(.comment)) }
}
{% endhighlight %}

具体实践看：[《KEEP》](https://biscuitos.github.io/blog/LD-KEEP/), [《DISCARD》](https://biscuitos.github.io/blog/LD-DISCARD/)

#### <span id="输入 section 描述中存放数据命令">输入 section 描述中存放数据命令</span>

能够在输出 section 中显示的填入指定的信息，可以使用如下关键字：

> BYTE(EXPRESSION)      1 字节   
>
> SHORT(EXPRESSION)     2 字节
>
> LONG(EXPRESSION)      4 字节
>
> QUAD(EXPRESSION)      8 字节
>
> SQUAD(EXPRESSION)     64 位系统时是 8 字节

输出文件的字节顺序 big endiannesss 或 little endianness，可以由输出目标文件的
格式决定。如果输出目标文件的格式不能决定字节序，那么字节顺序与第一个输入文件的
字节序相同。注意，上面的关键字只能存放在输出 section 描述内，其他地方不行。如
下：

错误：

{% highlight ruby %}
SECTIONS 
{
    .text : { *(.text) }
    LONG(1)
    .data : { *(.data) }
}
{% endhighlight %}

正确：

{% highlight ruby %}
SECTIONS
{
    .text : { *(.text) LONG(1) }
    .data : { *(.data) }
}
{% endhighlight %}

具体实践看：[《输入 section 描述中存放数据命令实践》](https://biscuitos.github.io/blog/LD-SECTIONS_DATA/)

在当输出 section 可能存在未描述的存储区域 (比如由于对齐造成的空隙)，可以用 
FILL(EXPRESSION) 命令决定这些存储区域的内容，EXPRESSION 的前两字节有效，这两字
节在必要时间可以重复被使用以填充这类存储区域。如 

{% highlight ruby %}
SECTIONS
{
    DemoData : { *(.data) FILL(0x8967) }
}
{% endhighlight %}

在输出 section 描述中可以有 =FILEEXP 属性，它的作用如同 FILL() 命令，但是 
FILL 指令只作用于该 FILL 之后的 section 区域。而 =FILEEXP 属性作用于整个输出 
section 区域，且 FILL 命令的优先级更高。

具体实践看：[《FILL》](https://biscuitos.github.io/blog/LD-FILL/)

#### <span id="输入 section 描述中的命令关键字">输入 section 描述中的命令关键字</span>

> CREATE_OBJECT_SYMBOLS
>
> CONSTRUCTORS

##### CREATE_OBJECT_SYMBOLS

为每个输入文件建立一个符号，符号名为输入文件的名字。每个符号所在的 section 是
出现该关键字的 section。

具体实践看：[《CREATE_OBJECT_SYMBOLS》](https://biscuitos.github.io/blog/LD-CREATE_OBJECT_SYMBOLS/)

##### CONSTRUCTORS

与 C++ 内的构造函数和析构函数相关，下面称它们为全局构造函数和全局析构函数。对
于 a.out 目标文件格式，链接器用一些不寻常的方法实现 C++ 的全局构造函数和全局析
构函数。当链接器生成的目标文件格式不支持任意 section 名字时，比如 ECOFF,XCOFF 
格式，链接器将通过名字来识别全局构造函数和析构函数，对于这些文件格式，链接器把
与全局构造函数和全局析构函数的相关信息放入出现 CONSTRUCTORS 关键字的输出 
section 内。

符号 __CTORS_LIST__ 表示全局构造函数信息的开始处，__CTORS_END__ 表示全局析构函
数信息的结束处。这两块信息的开始处是一字节的信息，表示该快信息有多少项数据，然
后以值为零的一字长数据结束。一般来说，GNU C++ 在函数 __main 内安排全局构造代码
运行，而 __main 函数被初始化代码 (在 main 函数调用之前执行) 调用。对于支持任意 
section 名的目标文件格式，比如 COFF, ELF 格式，GNU C++ 将全局构造和全局析构信
息分别放入 .ctros section 和 .dtors section 内，然以后在链接脚本内如下：

{% highlight ruby %}
__CTOR_LIST__ = .;
LONG((__CTOR_END__ - __CTRO_LIST__) / 4 - 2)
*(.ctors)
LONG(0)
__CTOR_END__ = .;
__DTOR_LIST__ = .;
LONG((__DTOR_END__ - __DTOR_LIST__) / 4 - 2)
*(.dtors)
LONG(0)
__DTOR_END__ = .;
{% endhighlight %}

如果使用 GNU C++ 提供的初始化优先级支持 (它能控制每个全局构造函数调用的先后顺
序), 那么请在链接脚本内把 CONSTRUCTORS 替换成 SORT(CONSTRUSTS), 把 *(.ctors) 
换成 *(SORT(.ctors)), 把 *(.dtors) 换成 *(SORT(.dtors))。 一般来说，默认的链接
脚本已经做好了这些工作。

#### <span id="输出 section 丢弃">输入 section 的丢弃</span>

例如，DemoData : { *(.rodata) }，如果没有任何一个输出文件包含 .rodata section，
那么链接器将不会在输出文件中创建 DemoData section。但是如果在这些输出 section 
描述包含了非输出 section 描述命令 (如符号，赋值语句)，那么；链接器将总是创建该
输出 section。

有一个特殊的输出 section，名为 /DISCARD/, 该 section 引用的任何输出 section 将
不会出现在输出文件内，就是 DISCARD 丢弃的意思。

具体实践请看：[《DISCARD》](https://biscuitos.github.io/blog/LD-DISCARD/)

---------------------------------------------------

# <span id="Section 叠加描述">覆盖图 (overlay) 描述</span>

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

> NOCROSSREFS 关键字指定各 section 之间不能交叉引用，否则报错
>
> START 指定所有 SECTION 的 VMA 地址 
>
> AT 指定了第一个 SECTION 的 LMA 地址

对于 OVERLAY 描述的每个 section，链接器定义两个符号 __load_start_SECNAME 和 
__load_stop_SECNAME，这两个符号的分别代表 SECNAME section 的 LMA 地址开始和结
束。链接器处理完 OVERLAY 描述语句之后，将定位符号的值加上所有覆盖图内 section 
大小的最大值。

输出 section 的 LMA 与 SECTIONS 之间的层级关系：

{% highlight ruby %}
SECTIONS
{
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

实践请看：[《OVERLAY》](https://biscuitos.github.io/blog/LD-OVERLAY/)

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

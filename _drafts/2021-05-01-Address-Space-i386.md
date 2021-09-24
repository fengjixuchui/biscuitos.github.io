---
layout: post
title:  "进程地址空间"
date:   2021-05-02 06:00:00 +0800
categories: [HW]
excerpt: Address Space.
tags:
  - MMU
---

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND00000L0.PNG)

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI100100.png)

#### 目录

> - [进程地址空间原理](#A)
>
>   - [Intel I386 Address Space Maps]()
>
>   - [Intel/AMD X86_64 Address Space Maps]()
>
>   - [Text Segment (ELF)](#A0)
>
>   - [Data Segment (ELF)](#B0)
>
> - [问题反馈及讨论交流](https://github.com/BiscuitOS/BiscuitOS/issues)
>
> - [附录/捐赠](#Z0)

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND000100.png)

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH000999.png)

----------------------------------

<span id="A0"></span>

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND00000H.jpg)

#### Text Segment (ELF)

进程的代码段 (text segment) 用于存储进程代码相关的信息，当一个进程被创建之后，其代码段会被加载到进程地址空间的指定位置，然后进程从代码段的指定位置开始执行, 在 i386 架构上经过 LD 链接器链接，程序的起始地址是 0x8048000; 在 AMD64 架构上经过 64 位 LD 链接器链接的 32/64 位程序的起始地址是 0x000000400000。那为什么代码段的起始地址会是指定的值? 那么同样的程序在不同的架构上起始地址不一样? 并且代码段里都有什么? 为了研究清楚上面的问题，开发者可以跟随下面目录进行实践研究:

> - [从一个实践例子开始进程的代码段](#A00)
>
> - [静态链接和动态链接](#A01)
>
> - [代码段的入口地址](#A02)
>
> - [代码段中的 Sections](#A03)
>
>   - [.text section](#A030)
>
>   - [.rodata section](#A031)
>
>   - [.init section](#A032)
>
>   - [.fini section](#A033)
>
>   - [.plt section](#A034)
>
>   - [.note.ABI-tag section](#A035)
>
>   - [.note.gnu.build-id section](#A036)
>
>   - [.rel.plt section](#A037)
>
>   - [.eh_frame section](#A038)
>
> - [代码段的生命周期](#A04)
>
> - 附录
>
>   - [AMD64 Arichitecture LD Scripts](#A001)
>
>   - [I386 Arichitecture LD Scripts](#A002)

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND000100.png)

------------------------------------

###### <span id="A00">从实践出发</span>

还是老规矩，先从实践来认知这个问题。BiscuitOs 已经集成并准备了一个用于实践讲解的例子，开发者可以更新最新的 BiscuitOs 之后进行实践, 其在 BiscuitOS 上的布局如下:

{% highlight bash %}
cd BiscuitOS
make menuconfig

  [*] Package  --->
      [*] Address Space Layout  --->
          [*] Text Segment: Stores ELF binary image  --->

BiscuitOS/output/linux-XXXX/package/BiscuitOS-address-space-text-segment-default
{% endhighlight %}

> [Text Segment Source Code](https://gitee.com/BiscuitOS_team/HardStack/tree/Gitee/Memory-Allocator/Address-Space/BiscuitOS-address-space-text-segment)
>
> [BiscuitOS 独立程序实践教程](https://biscuitos.github.io/blog/Human-Knowledge-Common/#C)

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH000527.png)

在上面的实践程序中，通过在 17 行到 20 行引用了链接器提供的几个变量，其中 \_\_executable_start 指向了进程的程序段入口地址，也就是进程开始执行的地址; etext 变量则表示进程程序段的结束地址; edata 变量表示进程数据段的结束地址; end 变量则表示 BSS 段的结束地址。通过上面几个变量可以知道进程的代码段的范围是 "\_\_executable_start" 到 "etext"，而进程的数据段的范围则是 "etext" 到 "edata", 最后进程的 BSS 段则是 "edata" 到 "end". 接下来在 BiscuitOS 上运行该实例 (注意! 此时 BiscuitOS 的架构是 i386):

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH000528.png)

从 BiscuitOS 上的运行可以看出每个段的具体范围，那么进程的代码段的加载地址是通过什么规则进行设置的? 或者 \_\_executable_start 的值此时为什么是 0x4fc000? 那么接下来来找出其中的原因. 通过上面的描述可以知道 \_\_executable_start 是由链接器提供的变量，那么接下来看一下链接的过程。首先一个程序从编写到运行需要经历如下几个过程:

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH000529.png)

当开发者们基于编程工具使用 C/CPP 编写的程序称为源程序，源程序包含了头文件、main() 函数和其他子函数、以及其他局部或全局变量，此时的源程序是易于开发者阅读的代码。由于源程序中定义或者头文件引用了很多宏等代码，此时需要将源程序进行**预处理**，将一些宏等代码进行替换，使其更易于编译器处理. 编译器将预处理之后的文件进行**汇编**，汇编的结果就是将高级语言转换成计算机更容易理解的低级语言，不同架构会对应不同的汇编。编译器继续将汇编程序进行编译成了**目标文件**, 当有了目标文件之后，链接器会采用动态链接或静态链接的方式，将程序所需的静态库和动态库进行**链接**, 此时链接器会基于**链接脚本**定义的规则来进行链接，链接最终生成了可执行文件. 以上便是一个简单的编译过程，实际过程会比这个复杂的多。

###### <span id="A01">静态链接与动态链接</span>

从之前的讨论可知，源程序要生成可执行文件必须经历两个阶段，即编译和链接。编译阶段不涉及链接行为，在链接过程中，链接主要有两种方式，分别是静态链接和动态链接。**静态链接**的方式是在链接阶段就已经把要链接的内容链接到可执行文件中，可执行文件可独立运行; **动态链接**的方式在链接的过程没有把需要链接的内容的打包到可执行文件中，而是在程序执行过程中再去链接所需的内容，因此动态链接的可执行文件不能独立运行。静态链接和动态链接各有优缺点，具体如下:

* 静态链接的执行的速度比动态链接的块
* 动态链接更节省内存

那么基于实践例子，分别查看一下动态链接和静态链接对代码段的影响 (假设 BiscuitOS 运行的 Host 平台是 AMD64), 首先来看一下静态链接, 在实践例子中，确保 Makefile 中采用静态链接:

{% highlight bash %}
# 实践例子目录
cd BiscuitOS/output/linux-XXXX/package/BiscuitOS-address-space-text-segment-default
vi BiscuitOS-address-space-text-segment-default/Makefile
{% endhighlight %}

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH000535.png)

在实践例子的 Makfile 中确保 59 行包含 "-static" 选项，接下来编译源程序并查看可执行文件的大小:

{% highlight bash %}
# 实践例子目录
cd BiscuitOS/output/linux-XXXX/package/BiscuitOS-address-space-text-segment-default
make
ls -l BiscuitOS-address-space-text-segment-default
{% endhighlight %}

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH000536.png)

当采用静态链接之后，可执行文件的大小为 657712，接着在 BiscuitOS 中实际运行如下:

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH000528.png)

从上图可以看出，程序的入口地址是 0x8048000, 并且每次运行都是该地址. 接着基于该实践例子查看采用动态链接方式时，可执行文件的不同之处，在实践例子中，确保 Makefile 中采用动态链接:

{% highlight bash %}
# 实践例子目录
cd BiscuitOS/output/linux-XXXX/package/BiscuitOS-address-space-text-segment-default
vi BiscuitOS-address-space-text-segment-default/Makefile
{% endhighlight %}

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH000537.png)

在实践例子中将 Makefile 中 59 行的 "-static" 选项移除，接下来编译源程序并查看可执行文件的大小:

{% highlight bash %}
# 实践例子目录
cd BiscuitOS/output/linux-XXXX/package/BiscuitOS-address-space-text-segment-default
make
ls -l BiscuitOS-address-space-text-segment-default
{% endhighlight %}

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH000538.png)

当采用动态链接的时候，可执行文件的大小只有 7300，基本为动态链接时的 "1%" 多一点，差异确实巨大，那么接下来看看其在 BiscuitOS 上的运行情况:

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH000539.png)

从上图可以看出，程序的入口是一个大于 0x400000 的值，且每次运行的时候程序入口地址的值都不是固定的。从实际的实践来看，动态链接在可执行文件占用内存大小方面确实比静态链接有很大优势，当动态链接的程序入口地址确实不一定，其根本原因是动态链接的时候程序的入口地址是动态调整的，也成为重定位，因此动态链接的可执行文件每次运行的入口地址都不一样，为了方面研究，接下来将采用静态链接的方式继续讨论其他问题。

###### <span id="A02">代码段的入口地址</span>

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH000529.png)

在 i386 架构上，为什么进程的入口地址都是 0x08048000? 由源码的编译链接过程可以知道，当源码经过编译汇编之后生成目标文件，链接器基于链接脚本将目标文件和动态库/静态库，通过静态链接或者动态链接的方式进行链接生成可知文件，可执行文件最终在目标环境中运行. 链接的决定了可执行程序运行时的入口地址，其根据链接脚本进行链接，那么接下来研究一下链接脚本是如何影响链接以及代码段的. 在研究链接脚本之前需要准备一些链接脚本的语法，开发者可以参考下文:

> [GNU 链接脚本详解](https://biscuitos.github.io/blog/LD/)

{% highlight bash %}
# Host I386 Targe I386
ld -verbose
# Host AMD64 Targe I386
ld -verbose -m elf_i386
# Host AMD64 Targe AMD64
ld -verbose
{% endhighlight %}

在 AMD64 或者 I386 架构的机器上，可以使用上述的命令获得链接脚本的具体内容，程序在编译链接时默认使用这个链接脚本，开发者也可以在链接的时候使用其他链接脚本，例如
:

{% highlight bash %}
ld -static -T BiscuitOS.lds -o a.out
{% endhighlight %}

链接器 ld 提供了 "-T" 选项用于指定私有的链接脚本进行链接，在链接过程中，链接器会将私有链接脚本的内容覆盖默认的链接脚本，并合并成一份临时的链接脚本进行链接。接下来开发者可以通过下面两个链接查看 I386 和 AMD64 架构下的链接脚本.

> [AMD64 Arichitecture LD Scripts](#A001)

> [I386 Arichitecture LD Scripts](#A002)

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH000540.png)

在 i386 架构的链接脚本中定义了一个全局变量 \_\_executable_start, 链接脚本使用 SEGMENT_START() 函数将程序的入口地址定义在 0x08048000 的位置上，同理查看一下 AMD64 上的链接脚本对程序入口地址的定义:

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH000541.png)

在 AMD64 架构上，链接脚本将程序的入口地址 \_\_executable_start 定义在 0x00400000 的位置上。因此可以知道为什么有的时候程序的入口地址是 0x08048000, 而有的时候时候是 0x00400000 了，这与链接时采用的链接脚本有直接关系。接下来查看代码段的构建逻辑:

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH000542.png)

上图是链接脚本中 ".text" section 的链接规则，根据链接规则，链接器将所有目标文件中符合要求的 section 全部放入到 ".text" section 中。链接脚本还定义了三个全局变量用于指示 ".text" section 的结束地址, 分别是 "\_\_etext"、"\_etext" 和 "etext". 那么接下来看看链接之后生成可执行文件中的代码段，此时基于实践例子进行讲解，在实践例子中才看可执行文件的代码段:

{% highlight bash %}
# 实践例子目录
cd BiscuitOS/output/linux-XXXX/package/BiscuitOS-address-space-text-segment-default/BiscuitOS-address-space-text-segment-default

# 文件通过 readelf -l BiscuitOS-address-space-text-segment-default 生成
vi BiscuitOS-address-space-text-segment-default.ebs
{% endhighlight %}

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH000543.png)

从可执行文件的 Program Headers Table 可以看出，代码段位于第一个表项，其在 ELF 文件正文中的偏移为 0，代码段加载进程地址空间的 0x08048000 处，代码段的大小为 0x8e438, 代码段具有可执行和可读权限，代码段并该 0x1000 进行对齐. 可执行文件中代码段包含了很多 section, 其中有我们熟悉的 ".text"、".rodata" 、".init" section 等。

------------------------------------------

#### <span id="A03">代码段中 Sections</span>

可执行文件的代码段最终会被整块的加载到程序的入口地址处，然后等待程序的执行。代码段中包含了多个 Sections，每个 Section 起到不同的作用，那么本段用于讨论代码段中各个 Section 相关的内容。

> - [.text section](#A030)
>
> - [.rodata section](#A031)
>
> - [.init section](#A032)
>
> - [.fini section](#A033)
>
> - [.plt section](#A034)
>
> - [.note.ABI-tag section](#A035)
>
> - [.note.gnu.build-id section](#A036)
>
> - [.rel.plt section](#A037)
>
> - [.eh_frame section](#A038)

----------------------------------

###### <span id="A030">.text segment</span>

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH000542.png)


------------------------------------------

###### <span id="A04">代码段的生命周期</span>

当一个进程创建的时候，其对应的代码段和数据段等会在进程的地址空间进行创建。代码段的创建与数据段一样，进程在其地址空间中为代码段分配一段虚拟内存，此时代码段只分配的虚拟地址，真正的代码段内容还存储在磁盘中。当进程初始化到一定阶段，CPU 开始执行进程的代码段时，首先将 CPU 指向了程序的入口地址，其位于代码段中。由于正在的代码段内容还在磁盘中，此时系统会通过缺页中断为代码段分配物理内存，并建立代码段的虚拟地址到物理内存的页表，最后再将磁盘中的代码段拷贝到指定的物理内存上，那么代码才真正被加载到代码段的虚拟内存上，待缺页中断返回，CPU 继续访问代码段的内容，并执行相应的代码逻辑。由于本文重点不再研究进程代码段的创建过程，感兴趣的童鞋请移步:

> [进程代码段创建过程](https://biscuitos.github.io/blog/Process-Address-Space/#A4)


![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND000100.png)

----------------------------------

<span id="B0"></span>

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND00000T.jpg)

#### Data Segment (ELF)

进程的数据段 (data segment) 用于存储进程运行所需的数据。当一个进程被创建之后，其数据段会被加载到进程的代码段之后，程序运行过程中会从数据段中读取或写入数据，因此数据段的属性通常为 "可读" 和 "可写"。在 C 语言中，数据种类大致可分配 "变量"、"指针"、"数组"、"结构体"、"联合体"、"枚举" 以及特殊的寄存器变量和常量，这些数据类型是否都存储在进程的数据段里? 以及数据段为什么一定要加载到代码段之后? 还有数据段为什么通常为 "可读写"，那么数据段还能包含其他属性吗? 为了研究清楚上面的问题，开发者可以跟随下面目录进行实践研究:

> - [从实践出发](#B00)
>
> - [全局变量](#B01)

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND000100.png)

------------------------------------

###### <span id="B00">从实践出发</span>

还是老规矩，先从实践来认知这个问题。BiscuitOS 已经集成并准备了一个用于实践讲解的例子，开发者可以更新最新的 BiscuitOs 之后进行实践, 其在 BiscuitOS 上的布局如下:

{% highlight bash %}
cd BiscuitOS
make menuconfig

  [*] Package  --->
      [*] Address Space Layout  --->
          [*] Data Segment: Stores Data for ELF  --->

BiscuitOS/output/linux-XXXX/package/BiscuitOS-address-space-data-segment-default
{% endhighlight %}

> [Data Segment Source Code](https://gitee.com/BiscuitOS_team/HardStack/tree/Gitee/Memory-Allocator/Address-Space/BiscuitOS-address-space-data-segment)
>
> [BiscuitOS 独立程序实践教程](https://biscuitos.github.io/blog/Human-Knowledge-Common/#C)

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH000544.png)

在上面的实践例子中，提供了一个源文件 "main.c". 虽然代码很长，但其逻辑很简单。程序一共定义了 8 大块数据区:

* 全局静态变量未初始化数据区 [34, 39]
* 全局静态变量初始化数据区 [42, 47]
* 全局变量未初始化数据区 [50, 55]
* 全局变量初始化数据区 [58, 63]
* 局部变量未初始化数据区 [82，87]
* 局部变量初始化数据区 [90, 95]
* 局部静态变量未初始化数据区 [98, 103]
* 局部静态变量初始化数据区 [106, 111]

在每个数据区内都定义了常见的数据种类 (实践例子并未覆盖所有的数据类型):

* 变量
* 指针
* 数组
* 结构体
* 联合体
* 枚举

除了上面的数据种类之外，程序在 66 行定义了宏，以及在 114 行定义了寄存器变量，最后还在 206 行对字符串常量的引用. 因此程序已经包含了常见的数据种类。程序在 70 行引用链接器定义的变量 \_\_executable_start[]，以此获得程序运行时进程的开始执行的地址. 函数接着引用链接器定义的变量 edata[]、etext[] 以及 end[], 以此获得进程运行时代码段和数据段在进程地址空间的范围。函数在 77 行通过内嵌汇编获得进程调用 main() 函数时堆栈栈顶位置，函数又在 116 行通过内嵌汇编获得 main() 函数定义完局部变量之后堆栈栈顶的位置。程序接在在 120 行到 136 行打印了进程代码段、数据段、BSS 段以及堆栈在进程地址空间的范围。程序从 139 行到 206 行将各个数据区的不同数据种类的地址打印出来。接下来在 BiscuitOS 上运行该实践例子 (实践基于 i386 架构)

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH000545.png)

从 BiscuitOS 上的运行可以看出，同样的数据种类位于不同的数据区内，其落在进程的不同段，有的数据区落在进程的数据段，有的数据区落在进程的 BSS 段，有的数据区落在了堆 heap 上，有的数据区落在了 MMAP 映射区，而有的数据区则落在进程的堆栈上。不过从运行的结果可以初步得出，数据位于进程的哪个段与数据的种类无关，而与数据定义的位置有关，那么接下来具体分析每种位置对数据落在进程段的影响。

--------------------------------------------
<span id="B01"></span>

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH000581.png)

#### 全局变量

全局变量是一种不仅可以在本文件中引用，还可以被其他文件引用。其定义为 extern 类型，但通常 extern 都会省略不写。相比与静态变量，静态变量需要显示的使用 static 关键字进行定义声明，而且静态变量只能在本文件中使用。那么接下来通过一个实践例子进行分析讲解，其在 BiscuitOS 中的部署如下:

{% highlight bash %}
cd BiscuitOS
make menuconfig

  [*] Package  --->
      [*] Address Space Layout  --->
          [*] Data Segment: Global Variable  --->

BiscuitOS/output/linux-XXXX/package/BiscuitOS-address-space-data-segment-global-default
{% endhighlight %}

> [Data Segment with Global Variable Source Code](https://gitee.com/BiscuitOS_team/HardStack/tree/Gitee/Memory-Allocator/Address-Space/BiscuitOS-address-space-data-segment-global)
>
> [BiscuitOS 独立程序实践教程](https://biscuitos.github.io/blog/Human-Knowledge-Common/#C)

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH000546.png)

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH000547.png)

在实践例子中包含了两个源文件，其中在源文件 BiscuitOs.c 中定义了两个全局变量，其中 BiscuitOS_other_global_uninit_variable 全局变量是未初始化的，而全局变量 BiscuitOS_other_global_init_variable 是初始化为 88520。在另外一个源文件 main.c 中，函数在 16 行和 17 行通过 extern 引用了 BiscuitOS.c 中的两个全局变量，程序继续在 20 行和 21 行也定义了两个全局变量，其中全局变量 BiscuitOS_current_global_uninit_variable 未初始化，而全局变量 BiscuitOS_current_global_init_variable 初始化为 88520. 在 main() 函数中，函数通过 extern 引用了链接器定义的 \_\_executable_start 等多个变量，其中 \_\_executable_start 指向了进程开始执行的虚拟地址，edata 指明了进程运行时数据段的结束地址，etext 指明了进程运行时代码段结束地址，end 则指明进程运行时 BSS 段结束的地址。函数在 30 行到 44 行打印了进程代码段、数据段、BSS 段在进程地址空间的范围。最后函数分别打印了四个全局变量的地址和值。将实践例子在 BiscuitOS 运行:

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH000548.png)

实践例子在 BiscuitOS 上运行的结果可以看出进程可以跨文件引用全局变量，然而全局变量如果没有被初始化，其被放在了 BSS 段，而被初始化的全局变量别放在了进程的数据段。那么为什么会出现这样的结果呢? 接下来从全局变量的生命周期中找找答案，依旧基于该实践例子进行分析.

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH000529.png)

全局变量的生命周期如上图，全局变量从源码阶段经过预处理、编译、汇编、链接阶段，最终进程通过加载器加载运行，直到进程结束退出，这些便是一个全局变量的完整生命周期。那么从全局变量生命周期的不同阶段来进行研究。

----------------------------------

#### 源码阶段

全局变量是被定义为 extern 属性的变量，与 static 属性的静态变量和局部变量不同，全局变量不止可以被当前文件引用，其他文件也可以引用该全局变量，因此全局变量对进程所包含的源文件都是可见的。全局变量定义在函数之外，并不使用 static 关键字进行定义，如果要在文件中使用其他文件中定义的全局变量，可以使用 "extern" 关键字在引用的文件中进行申明，那么该文件内的函数就可以使用全局变量了，如实践例子中:

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH000549.png)

全局变量如果未初始化，那么系统会将其自动初始化为 0; 而对于定义时初始化的全局变量，系统运行时为初始化的值。全局变量的种类包括:

* 各类整型变量、浮点变量以及布尔变量
* 指针
* 数组
* 结构体
* 联合体
* 枚举

----------------------------

#### 汇编阶段

无论使用的 C、C++ 还是其他高级语言，在进行链接之前都会将源代码先编译成汇编代码，汇编语言是比较接近计算机硬件而且便于开发者阅读的编程语言，使用下面命令查看实践例子对应的汇编源码:

{% highlight bash %}
cd BiscuitOS/output/linux-XXXX/package/BiscuitOS-address-space-data-segment-global-default
# gcc *.c -S
vi BiscuitOS-address-space-data-segment-global-default/BiscuitOS-address-space-data-segment-global-default_0.s
vi BiscuitOS-address-space-data-segment-global-default/BiscuitOS-address-space-data-segment-global-default_1.s
{% endhighlight %}

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH000550.png)

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH000551.png)

在 BiscuitOS.c 对应的汇编文件中，可以看到未初始化的全局变量 BiscuitOS_other_global_uninit_variable 加入到了汇编的 ".comm" section 中, 并指明了其长度为 4 字节并按 4 字节对齐; 而 BiscuitOS_other_global_init_variable 则使用 .global 关键字申明为一个全局变量。汇编程序的 5 行定义了 ".data" section 的起始位置，c程序在 9 行在汇编代码的 ".data" section 中定义了一个标号，该标号就是全局变量 BiscuitOS_other_global_init_variable，程序并在 7 行定义了 BiscuitOS_other_global_init_variable 的类型是 object，并指定了 object 的长度是 4 字节，最后程序在 10 行将 BiscuitOS_other_global_init_variable 标号对应的 ".data" section 设置值为 88520. 以上便可以看出初始化的全局变量和未初始化的全局变量在汇编阶段存在很大的差异。同理在 main.c 对应的汇编文件中，汇编源程序采用同样的方法定义了 BiscuitOS_current_global_uninit_variable，并将其加入到 ".comm" section 内; 而 BiscuitOS_current_global_init_variable 全局变量则通过 .global 关键字定义为全局变量，并加入到 ".data" section 内。

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH000552.png)

如上图所示，汇编源码在对全局变量的访问均通过 GOT 或者 GOTOFF 函数、ebx 寄存器，以及全局变量名字的方式进行。在汇编语言中，GOT 全称 "Global Offset Table" 全局偏移表，GOT 与 PLT 搭配使用，PLT 全称 Procedure Linker Table. GOT 和 PLT 是 Linux 系统下 ELF 可执行文件用于定位全局变量和过程的数据信息。在汇编程序中 "symbol@GOTOFF" 与 "symbol@GOT" 在 "System V I386 ABI" 中的解释如下:

> [SYSTEM V Application Binary Interface for I386 PDF](https://gitee.com/BiscuitOS_team/Documentation/blob/master/Tools-Book/SYSTEM_V_ABI_I386.pdf)

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH000559.png)

"symbol@GOTOFF" 表示 symbol 相对于 GOT (.got.plt) 的偏移。而 "symbol@GOT" 表示 symbol 在 GOT (.got.plt section) 中的 entry。为了更加深入理解这两个命令，接下来从 "\_GLOBAL_OFFSET_TABLE\_" 入手进行研究:

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH000560.png)

汇编语言中的 "\_GLOBAL_OFFSET_TABLE\_" 用于表示当前地址到 GOT (.got.plt section) 的偏移，也就是 "\_GLOBAL_OFFSET_TABLE\_" 所在代码段的位置到 GOT (.got.plt section) 的偏移，因此可以使用如下公式表示:

{% highlight bash %}
# Offset
_GLOBAL_OFFSET_TABLE = .got.plt - eip
{% endhighlight %}

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH000553.png)

回到汇编源代码，其中存在上图的代码。函数在 63 行调用 \_\_x86.get_pc_thunk.bx 函数将当前进程运行的地址存储到 EBX 寄存器中，然后在 64 行将 \_GLOBAL_OFFSET_TABLE\_ 的地址加上当前进程运行的地址，那么此时 EBX 寄存器存储着 GOT (.got.plt) 的地址. 那么此时存在两个问题 \_\_x86.get_pc_thunk.bx 如果获得进程当前运行的地址? 以及如何验证上面解释的正确的? 那么 接下来结合 GDB 工具和反汇编内容边实践边研究，相关文件已经准备好，开发者可以参考下面的命令进行调试:

{% highlight bash %}
cd BiscuitOS/output/linux-XXXX/package/BiscuitOS-address-space-data-segment-global-default
# 第一个窗口打开反汇编文件
# objdump -sSdhx binary -o > BiscuitOS.bs
vi BiscuitOS-address-space-data-segment-global-default/BiscuitOS-address-space-data-segment-global-default.bs
# 第二个窗口使用 GDB
gdb BiscuitOS-address-space-data-segment-global-default/BiscuitOS-address-space-data-segment-global-default
{% endhighlight %}

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH000555.png)

在反汇编文件 BiscuitOS-address-space-data-segment-global-default.bs 中查找 main() 函数的反汇编代码. 上图便是 main() 函数反汇编的结果，最左边是每行代码运行时的地址，中间部分是汇编代码对应的机器码，右侧部分则是 C 代码对应的汇编代码。反汇编的结果与体系架构有关，本文基于 i386 架构进行讲解，其他架构类似. 回到上面的反汇编代码，在地址 "0x80488b4" 处执行了汇编 "call 8048780 <\_\_x86.get_pc_thunk.bx>" 代码，那么可以从此处进行 GDB 调试，此时在另外一个窗口使用命令启动 GDB，并输入如下命令:

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH000556.png)

在 GDB 中使用 "b" 命令程序的 0x80488b4 打断点，当程序运行到 0x80488b4 的时候就会自动停止。当打完断点之后，使用 "r" 命令开始运行程序，程序运行之后会自动停止在 0x80488b4 的位置，此时使用 "layout asm" 边查看汇编边调试.

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH000557.png)

当程序停止在 0x80488b4 的位置，使用 "info reg" 命令查看此时 EIP 和 EBX 寄存器的值，此时 EIP 的值为 0x80488b4, 也就是断点的位置，EBX 的值则指向了 0x80d9000. 接着使用 "si" 命令而不是 "n" 或者 "ni" 命令，"si" 命令的作用是下一步进入函数内部，此时进入到 "\_\_x86.get_pc_thunk.bx" 函数.

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH000558.png)

执行 "si" 命令之后，此时进入到 "\_\_x86.get_pc_thunk.bx" 函数内部，此时通过 "info reg esp" 获得堆栈栈顶的位置，此时堆栈的栈顶为 0xffffcdec, 接着使用 "x/x 0xffffcdec" 命令查看栈顶的内容，结果发现堆栈存储的内容是 "0x80488b9", 也就是调用 "\_\_x86.get_pc_thunk.bx" 下一行指令的地址。函数此时将堆栈栈顶的内容存储到 EBX 寄存器。通过上面的分析可以知道汇编程序通过调用 "\_\_x86.get_pc_thunk.bx" 函数将下一行指令的地址存储到了 EBX 寄存器，当函数调用 ret 返回之后，EBX 寄存器的值和进程当前运行地址是一致的。

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH000561.png)

执行 "ni" 命令从 "\_\_x86.get_pc_thunk.bx" 函数返回之后，此时进程运行到 0x80488b9, 此时在 GDB 中使用 "info reg ebx eip" 命令查看 EBX 寄存器和 EIP 寄存器，此时两者都相同，那么为什么通过上面的指令就实现这么神奇的功能? 这里还得从 CALL 和 RET 指令说起:

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH000562.png)

在汇编指令中，当执行 CALL 指令的时候，CALL 指令的下一条指令的地址会被压入堆栈，当函数执行完毕调用 RET 指令进行返回是，RET 指令会从 ESP 寄存器的栈顶弹出一个地址作为下一个执行地址。"\_\_x86.get_pc_thunk.bx" 便利用这个原理，当进入 "\_\_x86.get_pc_thunk.bx" 函数之后，此时 ESP 指向的便是执行 CALL 指令的下一行指令，因此将栈顶的值存储到 EBX 寄存器之后 RET 返回，函数返回之后执行下一行代码执行时，此时 EBX 寄存器的值正好是进程当前执行地址.

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH000563.png)
![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH000553.png)

将汇编代码与 GDB 调试的情况进行对比，发现 \_GLOBAL_OFFSET_TABLE\_ 被替换成一个立即数。之前就说过 \_GLOBAL_OFFSET_TABLE\_ 是 GOT (.got.plt) 相对当前代码地址的偏移，那么接下来分析一下这个立即数是怎么计算出来的，首先结合反汇编文件找到 ".got.plt section" 的地址:

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH000564.png)

从反汇编文件中可以知道 ".got.plt section" 在地址是 0x80d9000, 此时 \_GLOBAL_OFFSET_TABLE\_ 所在代码行的地址是 0x80488b9, 带入公式进行计算:

{% highlight bash %}
# Offset
_GLOBAL_OFFSET_TABLE = .got.plt - eip

_GLOBAL_OFFSET_TABLE = 0x80d9000 - 0x80488b9
                     = 0x90747
{% endhighlight %}

通过上面的计算 \_GLOBAL_OFFSET_TABLE\_ 在运行的时候会被替换成一个立即数，该立即数就是 \_GLOBAL_OFFSET_TABLE\_ 所在代码位置与 GOT (.got.plt section) 的偏移。有了 \_GLOBAL_OFFSET_TABLE\_ 之后，symbol@GOTOFF 是如何定位 symbol 的地址呢? 接下来讨论 "symbol@GOTOFF" 的作用。

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH000565.png)

从 "System V I386 ABI" 中对 symbol@GOTOFF 的解释为 symbol 相对于 GOT (.got.plt section) 的偏移。那么反过来理解，symbol@GOTOFF 加上 \_GLOBAL_OFFSET_TABLE\_ 以及 EIP 就可以获得 symbol 的实际地址，那么可以获得下面公式:

{% highlight bash %}
# symbol 的相对地址
symbol - eip

# Offset
symbol@GOTOFF = _GLOBAL_OFFSET_TABLE - (symbol - eip)
              = (.got.plt - eip) - (symbol - eip)
              = .got.plt - symbol

# Symbol address
symbol        = _GLOBAL_OFFSET_TABLE + symbol@GOTOFF + eip
{% endhighlight %}

从上面的公式可以知道一个 symbol 的相对地址是相对于运行时的地址，那么相对地址表示为 "symbol - eip"。由于 symbol@GOTOFF 表示 symbol 在 \_GLOBAL_OFFSET_TABLE\_ 中的偏移，那么 symbol@GOTOFF 等于 ".got.plt - symbol", 最后要计算 symbol 的实际地址就可以通过 "\_GLOBAL_OFFSET_TABLE\_ + symbol@OFFSET + eip" 获得。接下来验证这个猜想:

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH000566.png)

在汇编代码中对 BiscuitOS_current_global_init_variable 的引用就采用了 symbol@GOTOFF 的方式，BiscuitOS_current_global_init_variable 定义在 ".data section", 那么首先计算 ".data section" 与 ".got.plt section" 之间的偏移，从反汇编文件中可以得到具体数据:

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH000567.png)

从上图可以看出 ".got.plt section" 在 ELF 文件中的偏移是 0x00090000, 而 ".data section" 在 ELF 文件中的偏移是 0x00090060, 那么两个段之间的偏移是 0x60, 接着查看 BiscuitOS_current_global_init_variable 在 ".data section" 的偏移:

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH000568.png)

BiscuitOS_current_global_init_variable 的地址是地址是 0x080b09e0, 其相对与 ".data section" 的偏移是 0x8, 那么 BiscuitOS_current_global_init_variable 相对于 ".got.plt section" 的偏移就是 0x68. 那么 BiscuitOS_current_global_init_variable 实际运行地址就是:

{% highlight bash %}
# address
address = _GLOBAL_OFFSET_TABLE + 0x68 + eip
        = _GLOBAL_OFFSET_TABLE + eip + 0x68
        = ebx + 0x68
{% endhighlight %}

这里是不是很熟悉，在 main() 的反汇编里，函数将 \_GLOBAL_OFFSET_TABLE\_ 对应的 GOT (.got.plt) 真实地址存储在 EBX 寄存器里。接下来在汇编文件中查找相应的内容:

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH000569.png)

上图代码 0x8048a9c 的位置，调用 movl 指令将立即数 9 直接存储到了 "0x68(%ebx)" 里，这和预期分析的一致，立即数 0x68 的计算即使 BiscuitOS_current_global_init_variable 相对于 GOT (.got.plt section) 的偏移值。接下来使用 GDB 进行实际验证，此时在 0x8048a9c 处打断点:

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH000570.png)

当 GDB 停止在 0x8048a9c 处，使用命令 "info reg ebx" 打印 EBX 寄存器的值，此时 EBX 寄存器的指向 GOT (.got.plt section) 实际的地址，那么将该地址加上 0x68, 等到的地址为 0x80d9068, 该地址正好是 BiscuitOS_current_global_init_variable 的地址. 有了 symbol@GOTOFF 功能之后，为什么还需要 symbol@GOT 呢? 开发者可以考虑一个问题，对于 extern 方式引用的全局变量，其被放在 BSS 段内，该段只有加载运行的时候才能知道具体的地址，只当靠 ELF 文件是无法获得其地址或基于 GOT 的偏移地址，那么对于 extern 的变量该怎么处理呢? 这里就引入了 symbol@GOT:

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH000571.png)

"symbol@GOT" 在 "System V I386 ABI" 中的定义为 symbol 在 GOT 的 entry。GOT 会为 进程运行时才能确定地址的 symbol 分配一个 entry，由于进程运行时不能去动态修改代码段中 symbol 的地址，因此在为 symbol 重定位时将重定位的地址填入到 symbol 在 GOT 的 entry 里，那么进程在引用 symbol 时先访问 GOT 的 entry 之后获得真实地址，最后在访问真实地址:

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH000572.png)

如上图，对一个 symbol 访问的时候，通过 symbol@GOT 找到 symbol 在 GOT (.got.plt) 中的 entry，该 entry 中存储着 symbol 的真实地址，最后进程顺利访问到 symbol. 由于该机制在动态链接和静态链接的场景下策略不同，因此 symbol@GOT 的实践放到链接阶段进行讲解。

###### symbol@GOT 与 symbol@GOTOFF

通过上面的实践总结可知，全局变量在汇编源程序中引用分为两种，一种是通过 symbol@GOTOFF 进行引用，另外一种是通过 symbol@GOT 进行引用的，那么什么时候使用 symbol@GOT? 那什么时候使用 symbol@GOTOFF 呢? 那么接下来先通过通过实践进行观察两者的不同.

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH000574.png)

首先在实践例子的 main.c 函数中添加两组数据，其中第一组是全局初始化的变量 BiscuitOS_init_X, 另外一组是全局未初始化变量 BiscuitOS_uninit_Y, 接着在 main() 函数中分别引用这两组的数据:

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH000575.png)

在 main() 函数中对两组数据都赋值，接下来重新编译程序，并查看其汇编代码:

{% highlight bash %}
cd BiscuitOS/output/linux-XXXX/package/BiscuitOS-address-space-data-segment-global-default
# gcc *.c -S -o *.s
gdb BiscuitOS-address-space-data-segment-global-default/BiscuitOS-address-space-data-segment-global-default-0.s
{% endhighlight %}

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH000576.png)

从汇编代码可以看出，所有的全局初始化变量都使用 symbol@GOTOFF 的方式访问, 而所有的全局未初始化的全局变量则通过 symbol@GOT 的方式访问。接着继续在实践例子的 BiscuitOS.c 文件中添加代码:

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH000577.png)
![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH000578.png)
![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH000579.png)

在实践例子的 BiscuitOS.c 中添加两组数据，其中第一组是初始化的全局变量，第二组则是未初始化的全局变量。接着在 main.c 函数通过 extern 关键字申明两组数据，最后在 main() 函数中为两组数据赋值。接下来查看对应的汇编文件:

{% highlight bash %}
cd BiscuitOS/output/linux-XXXX/package/BiscuitOS-address-space-data-segment-glob
al-default
# gcc *.c -S -o *.s
gdb BiscuitOS-address-space-data-segment-global-default/BiscuitOS-address-space-data-segment-global-default-0.s
{% endhighlight %}

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH000580.png)

查看汇编代码发现只要是 extern 声明的全局变量都使用 symbol@GOT 进行引用，而与全局变量是否初始化无关，为什么会这样呢? 其实这和链接有关系，接下来会在链接章节详细分析。

###### 汇编阶段总结

在汇编阶段，汇编源码为全局变量提供了三个工具: symbol@GOT、symbol@GOTFF 以及 \_GLOBAL_OFFSET_TABLE\_, 三者将结合链接器定位全局变量的绝对地址或相对地址。通过实践也发现:

* 全局初始化变量通过 symbol@GOTOFF 方式访问, 其存储在 ".data section"
* 全局未初始化变量通过 symbol@GOT 方式访问, 其存储在 ".comm section"
* extern 的全局初始化变量通过 symbol@GOT 方式访问, 存储在 ".comm section"
* extern 的全局未初始化变量通过 symbol@GOT 方式访问, 存储在 ".common section"

--------------------------------------

![](https://gitee.com/BiscuitOS/GIFBaseX/raw/master/RPI/GIF000210.gif)

#### 链接阶段

链接简单来说就是将输入目标文件中性质相同的 section 合并在一起形成新的 segment，例如将所有输入目标文件中全局变量所在的 .data section 最终链接合并成 .data segment. 链接器在链接过程中为全局变量分配地址和空间，这里所谈的空间分配只涉及虚拟地址空间的分配。对于全局变量，可能存在于 .data section 内，也可能存在与 .bss section 内，对于在 .data section 中的全局变量需要在文件中和虚拟地址空间中为其分配空间，而对于在 .bss section 中的全局变量只需分配虚拟地址空间而已。对于全局变量空间的分配基本采用两步链接 (Two-pass Linking) 的方法: 

* 空间与地址分配
* 符号解析与重定位

**第一步 "空间与地址分配"** 就是扫描所有的输入目标文件，获得全局变量所在 section 的长度、属性以及位置信息，并将输入目标文件中的符号表中所有全局变量的符号定义和符号引用收集起来，统一放到一个全局符号表中。这一步链接器能够获得合并之后全局变量段所在段在输出文件的长度和位置，并建立映射关系。


**第二步 "符号解析与重定位"** 就是使用上一步获得的信息，读取输入文件中全局变量所在段的数据、重定位信息，并进程符号解析与重定位、调整代码中的地址等。事实上这一步就是链接过程的核心。

为了更好的理解全局变量在链接过程中的行为，本节还是从实践出发边实践边讲解。依旧使用前面的实践例子，未部署的开发者可以参考下面命令进行部署实践:

{% highlight bash %}
cd BiscuitOS
make menuconfig

  [*] Package  --->
      [*] Address Space Layout  --->
          [*] Data Segment: Global Variable  --->

BiscuitOS/output/linux-XXXX/package/BiscuitOS-address-space-data-segment-global-default
{% endhighlight %}

> [Data Segment with Global Variable Source Code](https://gitee.com/BiscuitOS_team/HardStack/tree/Gitee/Memory-Allocator/Address-Space/BiscuitOS-address-space-data-segment-global)
>
> [BiscuitOS 独立程序实践教程](https://biscuitos.github.io/blog/Human-Knowledge-Common/#C)

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH000582.png)

在 BiscuitOS 中编译实践例子之后，其会生成多个文件，其中 main.c 和 BiscuitOS.c 为源代码，其余文件的对应关系如下:

* main.c 预处理文件 BiscuitOS-address-space-data-segment-global-default-0.i
* main.c 汇编文件 BiscuitOS-address-space-data-segment-global-default-0.s
* main.c 目标文件 BiscuitOS-address-space-data-segment-global-default-0.o
* main.c 目标文件的反汇编文件 BiscuitOS-address-space-data-segment-global-default-0.bs
* BiscuitOS.c 预处理文件 BiscuitOS-address-space-data-segment-global-default-1.i
* BiscuitOS.c 汇编文件 BiscuitOS-address-space-data-segment-global-default-1.s
* BiscuitOS.c 目标文件 BiscuitOS-address-space-data-segment-global-default-1.o
* BiscuitOS.c 反汇编文件 BiscuitOS-address-space-data-segment-global-default-1.bs
* ELF 可执行文件 BiscuitOS-address-space-data-segment-global-default
* ELF 可执行文件的反汇编文件 BiscuitOS-address-space-data-segment-global-default.bs
* ELF 可执行文件的 ELF 信息 BiscuitOS-address-space-data-segment-global-default.ebs

为了研究链接过程，接下来从目标文件进行研究，这里的目标文件就是链接过程中的输入目标文件，对应上面的 BiscuitOS-address-space-data-segment-global-default-0.o 和 BiscuitOS-address-space-data-segment-global-default-1.o。目标文件内部都是二进制数据，无法直观的阅读，因此可以使用 objdump 工具将目标文件进行解析，上图已经提供了解析完的文件，即 BiscuitOS-address-space-data-segment-global-default-0.bs 和 BiscuitOS-address-space-data-segment-global-default-1.bs。最后在分析目标文件之前，还需要准备一些 ELF 文件符号表相关的知识，那么接下来先研究一下 ELF 的符号表。在 ELF 文件中，符号表的表项使用 Elf32_Sym/Elf64_Sym 数据结构进行描述，其数据结果定义如下:

{% highlight bash %}
/* Symbol Table Entry */
typedef struct elf32_sym {
    Elf32_Word       st_name;     /* name - index into string table */
    Elf32_Addr       st_value;    /* symbol value */
    Elf32_Word       st_size;     /* symbol size */
    unsigned char    st_info;     /* type and binding */
    unsigned char    st_other;    /* 0 - no defined meaning */
    Elf32_Half       st_shndx;    /* section header index */
} Elf32_Sym;

typedef struct {
    Elf64_Half       st_name;     /* Symbol name index in str table */
    Elf_Byte         st_info;     /* type / binding attrs */
    Elf_Byte         st_other;    /* unused */
    Elf64_Quarter    st_shndx;    /* section index of symbol */
    Elf64_Xword      st_value;    /* value of symbol */
    Elf64_Xword      st_size;     /* size of symbol */
} Elf64_Sym;
{% endhighlight %}

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH000584.png)

上表描述了 ELF 文件符号表项中每个成员的含义，其中全局变量比较关心的成员是 st_info, 其中包含了符号绑定信息和符号类型信息，其值含义如下:

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH000585.png)

符号的绑定信息位于 st_info 成员的高 28 位，其字段含义如上，绑定信息包含了三种类型，其中全局变量可能是 STB_GLOBAL 和 STB_WEAK, 但不可能是 STB_LOCAL.

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH000586.png)

符号的类型 (Symbol Type) 位于 st_info 成员的低 4 位，其字段含义如上，目前包含了 5 种类型，其中全局变量属于 STT_OBJECT, 即全局变量对于的符号是个数据对象. 

**符号值 (st_value)** 每个符号都有一个对应着，如果符号是一个函数或变量的定义，那么符号的值就是这个函数或者变量的地址，更确切的讲下面几种情况需要区别对待:

* 目标文件中，符号不定义在 "COMMON 块", st_value 表示该符号在段中的偏移.
* 目标文件中，符号定义在 "COMMON 块"，st_value 表示的对齐属性.
* 可执行文件中，st_value 表示符号的虚拟地址.

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH000583.png)

接下来开始分析目标文件，上图为 BiscuitOS.c 对应的目标文件，该文件定义了两个全局变量，BiscuitOS_other_global_init_variable 是一个初始化的全局变量，因此可以在 28 行看到其存储在目标文件的 .data section 内; BiscuitOS_other_global_uninit_variable 是一个未初始化的全局变量，此时可以在 27 行看到其存储在 "COMMON" 内，接下来对比两个变量的 Symbol Table 的值:

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH000588.png)

两种类型的全局变量对比可知: 全局变量的类型都是 STT_OBJECT, 即符号是数据对象; 初始化的全局变量存储在 .data section, 且 st_value 表示全局变量在 .data section 中的偏移，此时可以在图中的 31-32 行查看 .data section 的内容，其内容正好是 BiscuitOS_other_global_init_variable 的初始值 88520; 对于未初始化的全局变量，其被放在 "COMMON" 块里，因此 st_value 则表示 BiscuitOS_other_global_uninit_variable 的对齐属性; 最后对于初始化的全局变量，其包含了 STB_GLOBAL 标志，由于其位于 COMMON 块，因此其属于弱符号. 

在 BiscuitOS.c 对于的目标文件中，一共存在 5 个 section，其中 .data section 的长度为 4，此时对于的 VMA/LMA 地址为 0，其长度为 34. 以上便是可以从目标文件中获得信息，接下来查看 main.c 对于的目标文件:

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH000587.png)

在 main.c 中定义了两个全局变量，其中 BiscuitOS_current_global_init_variable 为初始化的全局变量，而 BiscuitOS_current_global_uninit_variable 为未初始化的全局变量. 另外还使用 extern 引用了 BiscuitOs.c 中的两个全局变量。四个全局变量在目标文件的 symbol table 的值:

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH000589.png)

在目标文件 BiscuitOS-address-space-data-segment-global-default-0.o 中，BiscuitOS_current_global_init_variable 全局变量存储在 .data section 内，其 st_value 表示其在 .data section 中的偏移，其类型为 STT_OBJECT, 即符号是数据对象，其绑定类型为 STB_GLOBAL, 因此该变量可以被外部看见; BiscuitOS_current_global_uninit_variable 未初始化的全局变量存储在 "COMMON" 里，其 st_value 表示对齐属性，其类型也是 STT_OBJECT, 即符号是数据对象，但其是一个弱符号; 对于 extern 引用 BiscuitOS.c 的全局变量，由于没有被定义，因此其 st_shndx 被设置为 SHN_UNDEF，但其数据类型为 STB_GLOBAL. 之前在汇编阶段已经分析过对四个全局变量的引用通过 symbol@GOT 和 symbol@GOTOFF 的方式，如下图:

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH000590.png)

在汇编阶段得出了结论，对初始化的全局变量使用 symbol@GOTOFF 进行引用，而对于未初始化全局变量以及 extern 引用的全局变量通过 symbol@GOT 进行引用。但这个结论是存在一定先决条件的，那就是链接时的 "位置无关" 有关，那么接下来为了完善之前的讨论，接下来分别对 "位置无关" 和 非 "位置无关" 的情况分开讨论。所谓 "位置无关" 程序的 Load Address 不是绝对地址，而是意味着程序运行时可以灵活调整 Load Address, 当 Load Address 在运行时发生变化后，进程确保可以正确执行。非 "位置无关" 的情况典型包括 "静态链接"，所谓静态链接就是目标文件在链接成可执行文件时，链接器已经将所有的加载地址都计算好了，程序运行时按原先计算好的地址进行加载和运行。"位置无关" 的情况就是编译是带 "-fpic" 编译的目标文件进行 "动态链接"，所谓动态链接就是程序在运行时才计算加载地址。

###### 静态链接

静态链接是链接器在链接的过程中将进程运行时所需的内容全部链接到可执行文件中，并且计算好可执行文件中各个段的加载地址。程序运行的时不进行链接直接加载到执行虚拟地址进行运行，因此静态链接在运行之前就可以知道各个段的加载地址信息。接下来继续上面的实践例子进行讨论:

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH000590.png)

实践例子中 main.c 文件中对全局变量进行引用，面对不同类型的全局变量引用，汇编代码提供了 symbol@GOT 和 symbol@GOTOFF 机制对全局变量进行引用，那么在静态链接时如何计算全局变量的地址呢? 接下来先从 main.c 对应的目标文件入手，由于目标文件是二进制文件，不便于阅读，那么使用 objdump 工具解析目标文件，在 BiscuitOS-address-space-data-segment-global-default-0.bs 中关于全局变量的引用如下图:

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH000591.png)

从目标文件的反汇编中可以看到，在图中 340/343/346 行对三种全局变量进行访问，其访问方式都是通过 "mov 0x0(%ebx), %eax" 的方式进行的，而且在 349 行对 BiscuitOS_current_global_init_variable 的引用也是通过 "0x0(%ebx)" 进行的，我们会发现在目标文件中，全局变量清一色通过 "0x0(%ebx)" 进行引用。在汇编阶段我们已经分析过，此时 EBX 寄存器存储这 \_GLOBAL_OFFSET_TABLE\_ 的地址，那么链接器如何区分全局变量呢? 这里有个细节不知大家有没有发现: 在 240/343/346 行, MOV 指令对应的机器码是 "8b 83"，而 349 行的 MOV 指令则对应机器码 "c7 83"，接着在联系汇编代码，那么会发现 symbol@GOT 对应 MOV 指令的机器码是 "8b 83", 而 symbol@GOTOFF 对应 MOV 指令的机器码是 "c7 83". 另外还发现 341/344/347 行将全局变量描述为 R_386_GOT32X, 而 350 行则将全局变量描述为 R_386_GOTOFF. 从上面一共发现两处差异处，那么接下来分别对每种差异进行研究，首先对 MOV 机器码的差异进行研究:

> [Intel-IA32 Development Manual: Volume2 - Chapter 4 - 4,3 MOV -- Move](https://gitee.com/BiscuitOS_team/Documentation/blob/master/Datasheet/Intel/Intel-IA32_DevelopmentManual.pdf)

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH000592.png)

从 Intel 开发者手册上可以查找到 MOV 指令的机器码信息，对于 "8b 83" 表示将一个 32位寄存器或者 32 位内存地址的值存储到 EAX(83) 寄存器中; "c7 83" 则表示将一个 32 位立即数存储到 EAX(83) 寄存器中。通过之前的分析，两条机器码 "8b 83 00 00 00 00" 和 "c7 83 00 00 00 00" 地址部分都是 0, 因为编译器在编译汇编为目标文件时，有点全局变量定义在其他文件，编译器不知道其的加载地址，因此编译器暂时把全局变量的地址都设置为 0，把真正的地址计算工作留给了链接器。 那么接下里从全局变量引用时的 R_386_GOTOFF 和 R_386_GOT32X 进行分析，这里将设计到链接的重定位表。

**重定位表 (Relocation Table)** 链接器在链接目标文件时，目标文件的全局变量的地址都是 0，那么链接器是怎么知道哪些指令是要调整的? 事实上在 ELF 文件中，有一个叫重定位表的结构专门用来保存这些与重定位相关的信息，重定位表在 ELF 文件中往往是一个或多个 section。对于可重定位的 ELF 文件来说，它必须包含有重定位表，用来描述如何修改相应的 section 的内容。重定位表是 ELF 文件的一部分，因此重定位表也可以叫做重定位 section。开发者可以通过 objump 工具查看目标文件中的重定位表，使用如下命令:

{% highlight bash %}
cd BiscuitOS/output/linux-XXXX/package/BiscuitOS-address-space-data-segment-global-default
# objdump -r *.o
objdump -r BiscuitOS-address-space-data-segment-global-default/BiscuitOS-address-space-data-segment-global-default-0.o
{% endhighlight %}

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH000593.png)

上图便是 main.c 函数对于的目标文件的重定位表。每个表项称为重定位入口 (Relocation Entry)。重定位入口使用 Elf32_Rel 数据结构进行描述，其定义如下:

{% highlight bash %}
typedef struct {
    Elf32_Addr r_offset;
    Elf32_Word r_info;
} Elf32_Rel;
{% endhighlight %}

**r_offset** 重定位入口偏移。对于可重定位文件来说，这个值是该重定位入口所要修正的位置的第一个字节相对于段起始的偏移，这里值得思考由于要修正的地址位于代码段里，因此偏移是相对与 .text section 的偏移; 对于可执行文件或共享库来说，这个值是该重定位入口所需要修正的位置的第一个字节的虚拟地址。

**r_info** 重定位入口的类型和符号。这个成员的低 8 位表示重定位入口的类型，高 24 位表示重定位入口的符号表中的下表。通过符号表可以知道重定位入口所在的 section信息。

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH000594.png)

接下来以 BiscuitOS_other_global_uninit_variable 为例子讲解如何链接器如何找到要修正的位置。首先从重定位表开始找, 从上图的重定位表中可以知道 BiscuitOS_other_global_uninit_variable 的类型是 R_386_GOT32X, 其相对于 .text section 的起始位置为 0x1d5 处，此时通过反汇编文件查看 main.c 文件的目标文件 0x1df 处的内容:

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH000595.png)

上图便是 main() 函数反汇编的代码，其中在 340 行，此处距离 .text section 的偏移是 0x1d3, 接着一个字节一个字节的分析，0x1d3 对应的机器码是 0x8b, 通过前面的讨论可以知道 0x8b 是一个操作码，其对应的 MOV 指令; 0x1d4 对应的机器码是 0x83, 结合 MOV 指令，那么 0x83 对应 EAX 寄存器，那么接下来 0x1d5 到 0x1d8 四个字节正好用于存储全局变量的地址。通过上面的分析，BiscuitOS_other_global_uninit_variable 通过重定位表确实找到了要需要修改指令的地方。在重定位表里也发现同一个符号有多个需要重定位的地方。那么当链接器找到需要修改指令的地方，那么链接器在链接的时候进行修正。在重定位表中，每个重定位入口都有一个类型，该类型会影响地址修正的策略，具体类型修改策略如下:

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH000596.png)

* A 表示用于计算重定位域值的加数.
* B 表示程序运行时，共享库 ELF 装入内存的基地址.
* G 表示可重定位项在全局偏移表中的位置.
* GOT 表示全局偏移表的地址.
* L 表示一个符号的函数链接表项的所在地址，可能是节内偏移，或者内存地址.
* P 表示被重定位的存储党员在节内的偏移量或者内存地址.
* S 表示重定位项中某个所有所代表的符号值.

**R_386_GOTOFF** 这种重定位类型用于计算符号虚拟地址与全局偏移表之间的差值。其也会指示链接器构建全局偏移表。

**R_386_32** 这种重定位类型用于计算符号的虚拟地址与保存在被修正位置上的值，修正之后的值是一个立即数，因此在原始指令上，如果源操作数不是一个立即数，那么这里还涉及指令的修正。

**R_386_GOT32X** 这种重定位类型与链接模式有关，对于静态链接用于计算符号的虚拟地址与保存在被修正位置上的值，修正之后的值是一个立即数，因此在原始指令上，如果源操作数不是一个立即数，那么这里还涉及指令的修正。而对于动态链接，用于计算符号与全局符号偏移表之间的偏移值。

**R_386_GLOB_DAT** 这种重定位类型用于把符号的虚拟地址设置为一个全局偏移量表项。这种重定位类型在符号与全局偏移表项之间建立其联系.

**R_386_JMP_SLOT** 链接器创建这种重定位类型，用于动态链接。此类型相应的 offset 成员给出了函数链接表项的位置。动态链接器修改函数链接项来跳转到指定的符号地址.

**R_386_RELATIVE** 链接器创建这种重定位类型用于动态链接。此类型相应的 offset 成员给出了函数链接表项的位置。动态链接器修改函数链接表项来跳转到指定符号地址.

**R_386_GOTPC** 这种重定位类型与 R_386_PC32 很相似，只不过在计算中它使用的是全局偏移表的地址。一般来说，这种类型的重定位中所引用的符号是 \_GLOBAL_OFFSET_TABLE\_, 它还只是链接器去构建全局偏移表.

> [R_386_GOT32X and R_386_32](https://lkml.org/lkml/2016/10/20/141)

接下来继续以 BiscuitOS_other_global_uninit_variable 为例进行分析，目标文件在标记好需要重定位的地方后，链接器接下来将所有的目标文件进行链接，此时链接策略根据链接脚本进行，全局变量有的位于 .data section 内，有的位于 .bss section 内，链接策略如下:

> [I386 LD Scripts](#A001)
>
> [AMD64 LD Scripts](#A002)

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH000597.png)

在链接脚本中有上图描述，链接器会将所有输入目标文件的 .data、.data.\*、.gnu.linkonce.d.\* section 全部合并到可知文件的数据段内。并将所有输入目标文件的 .bss section 全部合并到可执行文件的 BSS 段内。由于链接脚本的起始虚拟地址是 0x8048000, 那么接下来链接器基于起始地址地址开始计算合并后各个 symbol 的虚拟地址. 此时可以使用 objdump 工具查看可执行文件的符号表，以此获得链接之后各个符号的虚拟地址，例如 BiscuitOS_other_global_uninit_variable 的地址:

{% highlight bash %}
cd BiscuitOS/output/linux-XXXX/package/BiscuitOS-address-space-data-segment-glob
al-default
# objdump -Ssdhx ELF
vi BiscuitOS-address-space-data-segment-global-default/BiscuitOS-address-space-data-segment-global-default.bs
{% endhighlight %}

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH000598.png)

从上图可执行文件的符号表可以知道 BiscuitOS_other_global_uninit_variable 的虚拟地址是 0x080dbcc8, 那么结合 main.o 目标文件的重定位表可知 BiscuitOS_other_global_uninit_variable 的重定位类型是 R_386_GOT32X, 那么链接器会使用绝对地址修正方案来修正地址，即 "S + A": 将 "符号的虚拟地址" 加上 "保存在被修正位置上的值"。

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH000595.png)
![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH000598.png)

由之前的分析可知知道, 在 main.o 目标文件中，"保存在被修改位置上的值" 为 0，如上上图 340 行 0x1d5-0x1d8 的位置即为 "保存在修改位置上的值"; 上图是符号链接之后地址, 即 "符号的虚拟地址" 0x080dbcc8, 因此修正的地址为:

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH000599.png)

通过上面的计算可知需要修正地址值之后，由于修正后的地址是一个绝地址，在汇编中就是一个立即数，因此链接器还需要修正一下指令对应的机器码:

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH000595.png)
![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH000600.png)

上上图 340 行是修正之前的机器码和汇编指令，上图 40136 行是修正之后的机器码和汇编指令，首先可以看出地址已经修正为 0x080dbcc8. 汇编指令虽然都是 MOV 指令，但对应的机器码已经从 "8b 83" 变成了 "c7 c0", 此时查阅 Intel 开发者手册:

> [Intel-IA32 Development Manual: Volume2 - Chapter 4 - 4,3 MOV -- Move](https://gitee.com/BiscuitOS_team/Documentation/blob/master/Datasheet/Intel/Intel-IA32_DevelopmentManual.pdf)

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH000592.png)

MOV 指令的源操作数由寄存器/内存变成了立即数，因此机器码也从 "8b" 变成了 "c7", 至此 BiscuitOS_other_global_uninit_variable 的地址修正和指令修正已经完成。接下来采用同样的方式分析一下 BiscuitOS_current_global_init_variable，其位于 main.o 目标文件的 .data section 内，其地址修正和指令修正如下:

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH000594.png)

使用 "objdump -r XXXXXXXXXXXXXXXXXXX.o" 命令从 main.o 目标文件的重定位表中找了 BiscuitOS_current_global_init_variable 的重定位入口，其需要重定位的位置在 main.o 目标文件 .text section 的 0x1f9，其重定位方式是 R_386_GOTOFF. 那么接下来查看 BiscuitOS_current_global_init_variable 链接之后符号的虚拟地址:

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH000601.png)

BiscuitOS_current_global_init_variable 链接之后的地址是 0x080da068, 由于其重定位类型是 R_386_GOTOFF, 因此其修正逻辑是 "S + A + GOT", 即 BiscuitOS_current_global_init_variable 的虚拟地址与全局偏移表之间的差值，在可知文件的符号表中可以知道全局偏移表的虚拟地址是:

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH000602.png)

此时可以获得全局偏移表 \_GLOBAL_OFFSET_TABLE\_ 的虚拟地址即使 .got.plt 段的虚拟地址，即 "0x080da000", 那么修正值的计算如下:

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH000603.png)

通过上面的计算可知需要修正的地址，此时 MOV 指令直接将立即数存储到目的段，那么指令不需要修正:

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH000604.png)
![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH000605.png)

上上图 349 行是修正之前的机器码和汇编指令，上图 40142 行是修正之后的机器码和汇编指令，此时对于本文件已经初始化的全局变量只修正了地址部分，将地址部分由原先的 "0x000000" 修正为 "0x00000068", 正好符合之前的猜想.

文章研究到这里还有一个地方没有说明白，例如在 main.c 文件中的未初始化全局变量 BiscuitOS_current_global_uninit_variable, 其位于 main.o 目标文件的 "COMMON" 块，但在链接之后，其位于可执行文件的 BSS 段中，那么这里是如何将其从 "COMMON" 块塞到 BSS 段中的呢? 在研究这个问题之前，需要对强符号和弱符号进行解释。

**弱符号与强符号** 在编程中经常会遇到一种情况叫符号重定义，即多个目标文件中含有相同名字的全局符号定义，那么这些目标文件在链接时就会出现重复定义的错误。这种符号定义可以被称为强符号 (Strong Symbol), 而相对强符号的则是弱符号 (Weak Symbol). 编译器默认函数和初始化的全局变量为强符号，而未初始化的全局变量则是弱符号。强符号和弱符号存在以下默认规则:

* 不允许强符号被多次定义.
* 如果一个符号在一个文件里是强符号，那么在其他文件则是弱符号.
* 如果一个符号在所有文件中都是弱符号，那么选择其中占用空间最大的一个.

**强引用和弱引用** 对外部目标文件中的符号使用 extern 进行声明，其在目标文件中最终链接成可执行文件时，其需要被正确决议，如果没有找到外部符号的定义，链接器会报符号未定义错误，这种被称为强引用 (Strong Reference). 与之相对应的是弱引用 (Weak Reference), 在处理器引用时，如果该符号有定义，则链接器将该符号的引用决议; 如果该符号未定义，则链接器不会报错，此时链接器认为它不是一个错误。一般对未定义的弱符号引用，链接器默认其为 0，或者是一个特殊的值，以便程序代码能够识别。

正如上面提到的，由于若符号机制运行一个符号的定义存在多个目标文件中，所以可能会导致一个问题: 如果一个弱符号定义在多个目标位文件中，而他们的类型又不同，怎么办呢? ，目前链接器还不支持符号的类型，即变量类型对于链接器来说都是透明的，它只是一个符号的名字，并不知道类型是否一致，那么可能会出现以下几种情况:

* 两个或两个以上强的符号类型不一致.
* 一个强符号，其余都是弱符号，出现类型不统一.
* 两个或两个以上的弱符号类型不一致.

对于上面三种情况，第一种不用处理，因为多个强符号定义本身就是非法的，因此链接器只用处理后面两种情况。事实上，现代的编译器和链接器都支持一种叫做 "COMMON" 块 (Common Block) 的机制，这种机制最早出起源于 Fortran, 早期的 Fortran 没有动态分配空间的机制，程序员必须事先声明它所需要的临时空间大小。Fortran 把这种空间叫 COMMON 块，当不同的目标文件需要的 COMMON 块空间大小不一致时，以最大的那块为准. 

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH000607.png)

链接器在处理弱符号的时候，采用了的就是与 COMMON 块一样的机制。编译器将未初始化的全局变量定义为弱符号处理，例如 BiscuitOS_current_global_uninit_variable:

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH000608.png)

BiscuitOS_current_global_uninit_variable 是一个全局的数据对象，它的类型为 SHN_COMMON, 因此他是一个弱符号，其长度为 4 个字节，如果此时在其他目标文件中也定义了一个名为 BiscuitOS_current_global_uninit_variable 未初始化的全局变量，其长度为 8 个字节，其类型也是 SHN_COMMON. 按照 COMMON 类型链接规则，原则上在最终输出文件中，BiscuitOS_current_global_uninit_variable 的大小以输入目标文件中最大的那个为准。因此此时 BiscuitOS_current_global_uninit_variable 的长度为 8 字节。

当然 COMMON 类型的链接规则都是针对符号都是弱符号的情况，如果其中有一个是强符号，那么最终输出的结果中符号所占空间与强符号相同。值得注意的是，如果链接过程中，有弱符号的长度大于强符号的情况，那么链接器会发出警告. 这种使用 COMMON 块的方法实际上是一种类似 "黑客" 的取巧方法，直接导致需要 COMMON 机制的原因是编译器和链接器允许不同类型的弱符号存在，但最本质的原因还是链接器不支持符号类型。最后，链接器在链接过程中确定了弱符号大小之后，在最终输出文件的 BSS 段中为弱符号分配空间，因此链接结束之后可以在 BSS 段中找到该弱符号，也可以知道弱符号的虚拟地址.

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND000100.png)

###### 动态链接

通过静态链接的分析可以知道，全局变量在链接阶段就确定了加载地址，程序运行时只需要通过加载地址就可以访问到全局变量。那么对于动态链接，全局变量的引用是怎么处理的呢? 基于链接基础知识可以知道，动态链接机制并不会在链接阶段对全局变量的地址进行重定位，只有程序加载的时候才会对地址进行重定位。设想一下，全局变量在链接之前的目标文件中，其地址是无法知道的，目标文件只维护了重定位表，以此告诉链接器哪些地址需要重定位，那么链接器在静态链接的时候就会根据重定位表对目标文件的代码和地址进行修正，由于此时程序没有运行，代码段是可以修正的; 但对于动态链接来说，链接器在链接阶段不会对代码和地址进行修正，只有等到程序加载的时候才进行地址重定位，可是一旦进程运行之后，进程的代码段是不能被修改的，因此动态链接对地址的重定位与静态链接有些不同。另外，全局变量可能位于共享动态库或动态链接的可执行文件中，两者都是通过动态链接的方式生成的目标文件，这里统一进行分析，那么接下来就分析全局变量在动态链接的处理流程，依旧以一个实践例子进行讲解，实践例子默认不使用地址无关机制 (其在 BiscuitOS 的部署流程如下):

{% highlight bash %}
cd BiscuitOS
make menuconfig

  [*] Package  --->
      [*] Address Space Layout  --->
          [*] Data Segment: Global Variable (Dynamic)  --->

BiscuitOS/output/linux-XXXX/package/BiscuitOS-address-space-data-segment-global-dynamic-default
{% endhighlight %}

> [Data Segment with Global Variable (Dynamic link) Source Code](https://gitee.com/BiscuitOS_team/HardStack/tree/Gitee/Memory-Allocator/Address-Space/BiscuitOS-address-space-data-segment-global-dynamic)
>
> [BiscuitOS 独立程序实践教程](https://biscuitos.github.io/blog/Human-Knowledge-Common/#C)

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH000609.png)

在 BiscuitOS 中编译实践例子之后，其会生成多个文件，其中 BiscuitOS-lib.c 为动态库的源码，main-A.c 和 main-B.c 为可执行文件 BiscuitOS-address-space-data-segment-global-dynamic-default-A 和 BiscuitOS-address-space-data-segment-global-dynamic-default-B 的源代码，其余文件的对应关系如下:

* main-A.c 预处理文件 BiscuitOS-address-space-data-segment-global-dynamic-default-0.i
* main-A.c 汇编文件 BiscuitOS-address-space-data-segment-global-dynamic-default-0.s
* main-A.c 目标文件 BiscuitOS-address-space-data-segment-global-default-dynamic-0.o
* main-A.c 目标文件反汇编文件 BiscuitOS-address-space-data-segment-global-dynamic-default-0.bs
* main-A.c 可执行文件 BiscuitOS-address-space-data-segment-global-dynamic-default-A
* main-A.c 可执行文件反汇编文件 BiscuitOS-address-space-data-segment-global-dynamic-default-A.bs
* main-A.c 可执行文件 ELF 信息 BiscuitOS-address-space-data-segment-global-dynamic-default-A.ebs
* main-B.c 预处理文件 BiscuitOS-address-space-data-segment-global-dynamic-default-1.i
* main-B.c 汇编文件 BiscuitOS-address-space-data-segment-global-dynamic-default-1.s
* main-B.c 目标文件 BiscuitOS-address-space-data-segment-global-default-dynamic-1.o
* main-B.c 目标文件反汇编文件 BiscuitOS-address-space-data-segment-global-dynamic-default-1.bs
* main-B.c 可执行文件 BiscuitOS-address-space-data-segment-global-dynamic-default-A
* main-B.c 可执行文件反汇编文件 BiscuitOS-address-space-data-segment-global-dynamic-default-B.bs
* main-B.c 可执行文件 ELF 信息 BiscuitOS-address-space-data-segment-global-dynamic-default-B.ebs
* BiscuitOS-lib.c 预处理文件 BiscuitOS-address-space-data-segment-global-dynamic-default-2.i
* BiscuitOS-lib.c 汇编文件 BiscuitOS-address-space-data-segment-global-dynamic-default-2.s
* BiscuitOS-lib.c 动态库 BiscuitOS-address-space-data-segment-global-default-dynamic-default-lib.so
* BiscuitOS-lib.c 动态库反汇编文件 BiscuitOS-address-space-data-segment-global-dynamic-default-lib.bs

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH000610.png)

BiscuitOS-lib.c 作为动态库，其源码中只定义了两个全局变量，其中未初始化的全局变量 BiscuitOS_other_global_uninit_variable，而 BiscuitOS_other_global_init_variable 为初始化的全局变量.

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH000611.png)

main-A.c 作为动态链接之后的可执行文件，其源码通过 extern 声明了动态链接库中的两个全局变量 BiscuitOS_other_global_uninit_variable 和 BiscuitOS_other_global_init_variable，并在本文件中定义了两个全局变量，其中 BiscuitOS_current_global_uninit_variable 为未初始化的全局变量，而 BiscuitOS_current_global_init_variable 为初始化的全局变量. 程序在 46 行到 50 行分别对全局变量进行赋值，最后在 66 行打印进程的 PID 之后死循环. 其余代码均是打印信息.

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH000612.png)

main-B.c 作为动态链接之后的可执行文件，其源码通过 extern 声明了动态链接库中的两个全局变量 BiscuitOS_other_global_uninit_variable 和 BiscuitOS_other_global_init_variable。程序在 44 行到 45 行对全局变量进行赋值，最后在 55 行打印进程的 PID 之后死循环, 其余代码均是打印信息 最后实践例子在 BiscuitOS 中的运行如下:

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH000613.png)

从运行效果来看，动态库中的全局变量由于运行时候才加载，因此同样的全局变量其虚拟地址是不固定的。这里不做过多解释，接下来从汇编文件开始分析整个动态链接过程。

###### 动态链接之汇编阶段

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH000614.png)

在 main-A.c 的汇编文件 BiscuitOS-address-space-data-segment-global-dynamic-default-0.s 中存在如上图的代码片段，对全局变量的引用直接采用 symbol 的方式，与之前静态链接汇编的 symbol@GOT 和 symbol@GOTOFF 方式不同。

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH000615.png)

在 main-B.c 的汇编文件 BiscuitOS-address-space-data-segment-global-dynamic-default-1.s 中存在如上图的代码片段，对全局变量的引用采用 symbol 方式。

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH000616.png)

在 BiscuitOS-lib.c 的汇编文件 BiscuitOS-address-space-data-segment-global-dynamic-default-2.s 中定义了全局变量，汇编源码中 BiscuitOS_other_global_init_variable 位于 .data section 内，而 BiscuitOS_other_global_init_variable 则位于 COMMON 块中. 

汇编阶段对全局变量的引用都是采用 symbol 的方式进行，而与静态链接给出的例子不同，静态链接的例子采用了 symbol@GOT 和 symbol@GOTOFF 方式访问全局变量。同样未初始化的全局变量都放到目标文件的 COMMON 块中，而初始化的全局变量则放到目标文件的 .data section 内.

###### 动态链接之链接前期

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH000617.png)

main-A.c 生成的目标文件 BiscuitOS-address-space-data-segment-global-dynamic-default-0.bs 中对全局变量的引用如上图，该目标文件还没有进行链接，在目标文件 247 行对全局变量 BiscuitOS_current_global_uninit_variable 进行引用，此时其对应的机器码是 "c7 05 00 00 00 00", 对应的汇编指令则是 "mov $X, 0", 结合 Intel 开发者手册可以知道:

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH000592.png)

> [Intel-IA32 Development Manual: Volume2 - Chapter 4 - 4,3 MOV – Move](https://gitee.com/BiscuitOS_team/Documentation/blob/master/Datasheet/Intel/Intel-IA32_DevelopmentManual.pdf)

从 Intel 手册可以看出机器码 c7 对应 MOV 指令，而 05 则指明 MOV 指令的源地址是一个 32 位的立即数，而目的地址则是一个寄存器或内存地址. 同理目标文件的 244 行对全局变量 BiscuitOS_other_global_init_variable 以及 250 行对 BiscuitOS_other_global_uninit_variable 引用的机器码都是 "7c 05 00 00 00 00". 从上面的三个例子可以看出目标文件并为填写全局变量的实际地址，而是全部填零。此时在全局变量的引用之后的代码存在重定位标记，例如在 248 行对 BiscuitOS_current_global_uninit_variable 进行引用，此时填入的地址全是 00， 但在 248 行的内容为 "R_386_32 BiscuitOS_current_global_uninit_variable", 该信息为重定位信息，后面详细讨论.

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH000618.png)

main-B.c 源文件生成的目标文件 BiscuitOS-address-space-data-segment-global-dynamic-default-1.bs 中对全局变量的引用如上图, 两个全局变量都是其他文件定义，通过 extern 在 C 源码中进行引用，而在汇编源码中其都通过 symbol 方式进行引用。在目标文件 224 行对全局变量的引用机器码是 "c7 05 00 00 00 00", 对应的汇编代码代码是 "mov $X, 0x0", 因此和 main-A.c 中对全局变量的引用一样，在链接之前均为对地址进行设置。

###### 动态链接之链接中期

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH000597.png)

链接中期是链接器链接的阶段，如上图，链接器将所有输入目标文件中的 section 按照链接脚本的规则链接到输出可知文件中指定的段。由之前的分析可知初始化的全局变量可以链接到可执行文件的数据段中，而未初始化的全局变量链接器通过 COMMON 块机制最终将其放置到 BSS 段中。链接的时候会将所有输入文件的符号表处理之后合并成一张统一的符号表，符号表中记录了全局变量在文件中的偏移值。链接器还会根据输入目标文件的重定向表对全局变量的地址和代码进行一定的修正。那么接下来本节重点介绍链接器如何根据重定位表修正全局变量的地址和相关的代码.

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH000619.png)

使用 "readelf -r BiscuitOS-address-space-data-segment-global-dynamic-default-0.o" 命令可以获得 main-A.o 目标文件的重定位表，每个表项称为重定位入口。每个表项称为重定位入口 (Relocation Entry)。重定位入口使用 Elf32_Rel 数据结构进行描述，其定义如下:

{% highlight bash %}
typedef struct {
    Elf32_Addr r_offset;
    Elf32_Word r_info;
} Elf32_Rel;
{% endhighlight %}

**r_offset** 重定位入口偏移。对于可重定位文件来说，这个值是该重定位入口所要修正的位置的第一个字节相对于段起始的偏移，这里值得思考由于要修正的地址位于代码段里，因此偏移是相对与 .text section 的偏移; 对于可执行文件或共享库来说，这个值是该重定位入口所需要修正的位置的第一个字节的虚拟地址。

**r_info** 重定位入口的类型和符号。这个成员的低 8 位表示重定位入口的类型，高 24 位表示重定位入口的符号表中的下表。通过符号表可以知道重定位入口所在的 section信息
。

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH000620.png)

接下来以 BiscuitOS_current_global_uninit_variable 为例子讲解如何链接器如何找到要修正的位置。首先从重定位表开始找, 从上图的重定位表 37 行可以知道未初始化全局变量 BiscuitOS_current_global_uninit_variable 的类型是 R_386_32, 其相对于 .text section 的起始位置为 0x111 处，此时通过反汇编文件查看 main-A.c 文件的目标文件 0x111 处的内容:

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH000617.png)

上图便是 main() 函数反汇编的代码，其中在 247 行，此处距离 .text section 的偏移是 0x10f, 接着一个字节一个字节的分析，0x10f 对应的机器码是 0xc7, 通过前面的讨论可以知道 0xc7 是一个操作码，其对应的 MOV 指令; 0x110 对应的机器码是 0x05, 结合 MOV 指令，那么 0x05 对应寄存器或内存地址，那么接下来 0x111 到 0x114 四个字节正好用于存储全局变量的地址。通过上面的分析，BiscuitOS_current_global_uninit_variable 通过重定位表确实找到了要需要修改指令的地方。在重定位表里也发现同一个符号有多个需要重定位的地方。那么当链接器找到需要修改指令的地方，那么链接器在链接的时候进行修正。在重定位表中，每个重定位入口都有一个类型，该类型会影响地址修正的策略，具体类型修改策略如下:

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH000596.png)

* A 表示用于计算重定位域值的加数.
* B 表示程序运行时，共享库 ELF 装入内存的基地址.
* G 表示可重定位项在全局偏移表中的位置.
* GOT 表示全局偏移表的地址.
* L 表示一个符号的函数链接表项的所在地址，可能是节内偏移，或者内存地址.
* P 表示被重定位的存储党员在节内的偏移量或者内存地址.
* S 表示重定位项中某个所有所代表的符号值.

**R_386_GOTOFF** 这种重定位类型用于计算符号虚拟地址与全局偏移表之间的差值。其也会指示链接器构建全局偏移表。

**R_386_32** 这种重定位类型用于计算符号的虚拟地址与保存在被修正位置上的值，修正之后的值是一个立即数，因此在原始指令上，如果源操作数不是一个立即数，那么这里还涉及指令的修正。

**R_386_GOT32X** 这种重定位类型与链接模式有关，对于静态链接用于计算符号的虚拟地址与保存在被修正位置上的值，修正之后的值是一个立即数，因此在原始指令上，如果源操作数不是一个立即数，那么这里还涉及指令的修正。而对于动态链接，用于计算符号与全局符号偏移表之间的偏移值。

**R_386_GLOB_DAT** 这种重定位类型用于把符号的虚拟地址设置为一个全局偏移量表项。这种重定位类型在符号与全局偏移表项之间建立其联系.

**R_386_JMP_SLOT** 链接器创建这种重定位类型，用于动态链接。此类型相应的 offset 成员给出了函数链接表项的位置。动态链接器修改函数链接项来跳转到指定的符号地址.

**R_386_RELATIVE** 链接器创建这种重定位类型用于动态链接。此类型相应的 offset 成员给出了函数链接表项的位置。动态链接器修改函数链接表项来跳转到指定符号地址.

**R_386_GOTPC** 这种重定位类型与 R_386_PC32 很相似，只不过在计算中它使用的是全局
偏移表的地址。一般来说，这种类型的重定位中所引用的符号是 \_GLOBAL_OFFSET_TABLE\_, 它还只是链接器去构建全局偏移表.

> [R_386_GOT32X and R_386_32](https://lkml.org/lkml/2016/10/20/141)

接下来继续以 BiscuitOS_current_global_uninit_variable 为例进行分析，目标文件在标记好需要重定位的地方后，链接器接下来将所有的目标文件进行链接，此时链接策略根据链接脚本进行，全局变量有的位于 .data section 内，有的位于 .bss section 内，链接策略如下:

> [I386 LD Scripts](#A001)
>
> [AMD64 LD Scripts](#A002)

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH000597.png)

在链接脚本中有上图描述，链接器会将所有输入目标文件的 .data、.data.\*、.gnu.linkonce.d.\* section 全部合并到可知文件的数据段内。并将所有输入目标文件的 .bss section 全部合并到可执行文件的 BSS 段内。在动态链接模式下，由于链接脚本的起始虚拟地址是 0x8048000, 并且链接器不会为需要全局变量进行重定位，只能等到程序加载运行阶段才会对全局变量的地址进行重定位，那么问题来了，之前分析过，目标文件中全局变量的地址都还是 0，而且位于目标文件的 .text section 内，链接之后这部分代码就链接到可执行文件的代码段，此时代码段是只读的，那么全局变量的地址如何重定位呢? 

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH000621.png)

从 main-A.c 的可执行文件的反汇编文件 BiscuitOS-address-space-data-segment-global-dynamic-default-A.bs 的符号表可以获得 BiscuitOS_current_global_uninit_variable 在可执行文件中的绝对地址是 0x0804a034, 由于其重定位类型为 R_386_32, 那么其修正地址的计算方法如下:

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH000623.png)

BiscuitOS_current_global_uninit_variable 的重定位类型是 R_396_32, 那么重定位策略就是 "S + A", 由目标文件的汇编代码可知，S 为 0，而 A 绝地地址就是 0x0804a034, 因此最后修正地址是 0x0804a034.

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH000622.png)

从 main-A.c 可执行文件链接之后的反汇编文件 BiscuitOS-address-space-data-segment-global-dynamic-default-A.bs 可以看到 646 行链接器将地址修正为 0x804a034. 链接完毕之后，当程序运行时其运行地址如下:

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH000624.png)

BiscuitOS-address-space-data-segment-global-dynamic-default-A 程序运行时，全局变量 BiscuitOS_current_global_uninit_variable 的虚拟地址正好是链接器修正之后的地址。那么开发者有没有发现此时和静态链接是不是很像，明明是动态链接，为什么地址不是加载是确认而是链接是确认呢? 这里首先应该明确一下，动态链接重定位的对象是目标文件中需要链接时重定位的对象，这个理解有点难于理解，例如例子中的 BiscuitOS_current_global_uninit_variable 全局变量，其重定位类型是 R_386_32, 该重定位类型只需在链接阶段进行地址修正，对该全局变量的访问采用绝对地址方式。

-------------------------------------------------

###### <span id="A001">AMD64 LD Scripts</span>

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH000530.png)

###### <span id="A002">I386 LD Scripts</span>

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH000531.png)

-----------------------------------------------

#### <span id="Z0">附录</span>

> [BiscuitOS Home](https://biscuitos.github.io/)
>
> [BiscuitOS Blog 2.0](https://biscuitos.github.io/blog/BiscuitOS_Catalogue/)
>
> [Linux Kernel](https://www.kernel.org/)
>
> [Bootlin: Elixir Cross Referencer](https://elixir.bootlin.com/linux/latest/source)
>

#### 捐赠一下吧 🙂

![MMU](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/HAB000036.jpg)

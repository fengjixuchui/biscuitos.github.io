---
layout: post
title:  "寄存器之函数参数约定"
date:   2022-07-01 12:00:00 +0800
categories: [MMU]
excerpt: Function Argument.
tags:
  - Register
---

![](/assets/PDB/BiscuitOS/kernel/IND00000L0.PNG)

#### 目录

> - [函数参数调用介绍](#A)
>
> - [X86_64 架构函数参数约定](#E001)
>
> - [i386 架构函数参数约定](#E002)
>
> - [ARM64 架构函数参数约定](#E003)
>
> - [ARM32 架构函数参数约定](#E004)

######  🙂🙂🙂🙂🙂🙂🙂🙂🙂🙂🙂🙂🙂🙂🙂🙂 捐赠一下吧 🙂🙂🙂🙂🙂🙂🙂🙂🙂🙂🙂🙂🙂🙂🙂🙂

![BiscuitOS](/assets/PDB/BiscuitOS/kernel/HAB000036.jpg)

-------------------------------------------

<span id="A"></span>

![](/assets/PDB/BiscuitOS/kernel/IND00000Q.jpg)

#### 函数参数调用介绍

![](/assets/PDB/HK/TH000649.png)

当使用 Crash/Kporbe/KSAN/Ftrace 等工具在排查内核问题的时候，分析工具可能会输出一堆寄存器信息，这些寄存器信息对问题的排查起到了关键作用。另外这些寄存器与函数调用有密切联系，函数调用时会将其参数存放在不同的寄存器中，并且不同架构参数存放在不同的寄存器中，如果能够通过寄存器准确获得某个参数的具体数值，这对问题的排查起到了关键作用。本文对主流架构的函数参数寄存器约定进行分析，以便帮助开发者加速问题的排查。另外本文在分析每个架构的同时，针对每个架构给出了一个实践案例，并可以在 BiscuitOS 上进行实践, Respect!

--------------------------------

###### <span id="E001">X86_64 架构</span>

![](/assets/PDB/HK/TH000650.png)

在 X86_64 架构中，寄存器的约定如上，当调用一个函数的时候，RDI 寄存器用于传递第一个参数，RSI 寄存器用于传递第二个寄存器，依次类推，R9 寄存器传递第六个参数, 函数返回值保存
在 RAX 寄存器中。那么如果函数的参数超过六个，那么多余的参数参数如何传递? 在 X86_64 架构中，函数大于 6 个参数的参数通过堆栈进行传输:

![](/assets/PDB/HK/TH000651.png)

当函数的参数超过六个之后，超过的参数从右到左存储在堆栈中，调用者在调用函数之前，将最后一个参数压缩堆栈，其次将倒数第二个压入堆栈，直到将第七个参数压入堆栈为止。参数压入>完毕之后向堆栈中压入函数的返回地址，其余的第一个参数到第六个参数都按约定存储在寄存器中。接下来通过一个实例来实践整个过程，BiscuitOS 已经支持该实例实践，其在 BiscuitOS 中
的部署如下:

{% highlight bash %}
cd BiscuitOS
make linux-5.0-x86_64_defconfig
make menuconfig

  [*] Package  --->
      [*] CRASH and BUG Example  --->
          [*] Example: Function arguments  --->

BiscuitOS/output/linux-5.0-x86_64/package/BiscuitOS-crash-register-with-func-argument-default
{% endhighlight %}

> [BiscuitOS-crash-register-with-func-argument-default Source Code](https://gitee.com/BiscuitOS_team/HardStack/tree/Gitee/Tools/Crash/BiscuitOS-crash-register-with-func-argument)
>
> [BiscuitOS 独立程序实践教程](/blog/Human-Knowledge-Common/#C)

![](/assets/PDB/HK/TH000652.png)

实践案例为一个内核模块，在实践案例中定义了一个函数 BiscuitOS_func_arguments(), 其包含了 20 个参数，并在调用该函数时会触发内核转储，因此可以通过汇编和 CRASH 工具查看函数>参数传递过程，首先使用如下命令获得汇编文件:

{% highlight bash %}
cd BiscuitOS/output/linux-5.0-x86_64/package/BiscuitOS-crash-register-with-func-argument-default
make
vi BiscuitOS-crash-register-with-func-argument-default/main.s
{% endhighlight %}

![](/assets/PDB/HK/TH000653.png)

* 114 行第一个参数通过 RDI 寄存器传递
* 112 行第二个参数通过 RSI 寄存器传递
* 110 行第三个参数通过 RDX 寄存器传递
* 108 行第四个参数通过 RCX 寄存器传递
* 106 行第五个参数通过 R8 寄存器传递
* 103 行第六个参数通过 R9 寄存器传递

{% highlight bash %}
movq    112(%rsp), %rax
movl    (%rax), %eax
pushq   %rax
{% endhighlight %}

对于第六个之后的参数，函数首先通过上面的方法获得。程序首先通过 "112(%rsp)" 的方式在堆栈中找到最后一个参数的位置，并将参数的地址存储到 RAX 寄存器，接着将 RAX 寄存器对应地
址的值进行读取，这样就获得指定参数。读取完毕之后，程序调用 "pushq" 的方式将 RSP 寄存器的值指向下一帧 (指向更低的地址)，然后程序重复上面的代码读取下一个参数，以此类推获得
第六个之后的参数. 接着在 BiscuitOS 上运行并获得内核转储文件:

{% highlight bash %}
cd BiscuitOS/output/linux-5.0-x86_64/package/BiscuitOS-crash-register-with-func-argument-default
make
make install
make pack
make run
{% endhighlight %}

![](/assets/PDB/HK/TH000654.png)

当在 BiscuitOS 中加载模块 BiscuitOS-crash-register-with-func-argument-
default.ko，由于代码中对空指针的引用，此时内核触发 panic，此时内核打印出内核转储时寄存器的信息，从运行可知 data 数组的地址为 0xffffffffc0087000, 那么此时 EDI 寄存器中存>储了第一个参数，其值也为 0xffffffffc0087000; 同理 ESI 寄存器中存储了第二个参数，其值为 ffffffffc0087004, 正好是函数第二个参数的值，依次类推，RDX/RCX/R8/R9 寄存器均存储函
数的参数。实践结果符合预期.

![](/assets/PDB/BiscuitOS/kernel/IND000100.png)

--------------------------------

###### <span id="E002">i386 架构</span>

![](/assets/PDB/HK/TH000655.png)

在 i386 架构中，寄存器的约定如上，寄存器的使用与 "\_\_attribute\_\_((regparm(n)))" 有关，当 n 为 0 时表示使用零个寄存器传递函数的参数; 当 n 为 1 时表示使用一个寄存器传递
函数参数; 以此类推，n 的范围是 0-3, 因此最多可以使用 3 个寄存器传递参数, 默认情况下，当函数参数数量超过 n 的最大值时，n 默认为 3，那么这个时候系统使用 3 个寄存器传递函数
参数。但函数使用寄存器传递参数时，默认约定是: 当调用一个函数的时候，EAX 寄存器用于传递第一个参数，EDX 寄存器用于传递第二个寄存器，ECX 寄存器传递第三个寄存器。函数返回值>保存在 EAX 寄存器中. 那么如果函数的参数超过 "\_\_attribute\_\_((regparm(n)))" 所规定的 n 个寄存器时，那么多余的参数参数如何传递? 在 i386 架构中，函数大于 n 个参数的参数>通过堆栈进行传输:

![](/assets/PDB/HK/TH000656.png)

当函数的参数超过三个之后，超过的参数从右到左存储在堆栈中，调用者在调用函数之前，将最后一个参数压缩堆栈，其次将倒数第二个压入堆栈，直到将第四个参数压入堆栈为止。参数压入>完毕之后向堆栈中压入函数的返回地址，其余的第一个参数到第三个参数都按约定存储在寄存器中。接下来通过一个实例来实践整个过程，BiscuitOS 已经支持该实例实践，其在 BiscuitOS 中
的部署如下:

{% highlight bash %}
cd BiscuitOS
make linux-5.0-i386_defconfig
make menuconfig

  [*] Package  --->
      [*] CRASH and BUG Example  --->
          [*] Example: Function arguments  --->

BiscuitOS/output/linux-5.0-i386/package/BiscuitOS-crash-register-with-func-argument-default
{% endhighlight %}

> [BiscuitOS-crash-register-with-func-argument-default Source Code](https://gitee.com/BiscuitOS_team/HardStack/tree/Gitee/Tools/Crash/BiscuitOS-crash-register-with-func-argument)
>
> [BiscuitOS 独立程序实践教程](/blog/Human-Knowledge-Common/#C)

![](/assets/PDB/HK/TH000652.png)

实践案例为一个内核模块，在实践案例中定义了一个函数 BiscuitOS_func_arguments(), 其包含了 20 个参数，并在调用该函数时会触发内核转储，因此可以通过汇编和内核转储信息查看函数
参数传递过程，首先使用如下命令获得汇编文件:

{% highlight bash %}
cd BiscuitOS/output/linux-5.0-i386/package/BiscuitOS-crash-register-with-func-argument-default
make
vi BiscuitOS-crash-register-with-func-argument-default/main.s
{% endhighlight %}

![](/assets/PDB/HK/TH000657.png)

* 58 行第一个参数通过 EAX 寄存器传递
* 57 行第二个参数通过 EDX 寄存器传递
* 56 行第三个参数通过 ECX 寄存器传递

{% highlight bash %}
movl    offset(%ebp), %ebx
{% endhighlight %}

对于第四个之后的参数，汇编直接通过 "offset(stack)" 的方式从堆栈中获得参数的内容，并将其 movl 存储到指定寄存器中。开发者也可以参数通过在函数中添加 "\_\_attribute\_\_((regparm(n)))" 来查看寄存器数量的变化情况:

![](/assets/PDB/HK/TH000661.png)
![](/assets/PDB/HK/TH000662.png)

当 "\_\_attribute\_\_((regparm(n)))" n 为 0 时，此时函数不使用寄存器传递参数，可以看出所有参数都通过堆栈传递。上图 176 行从堆栈 "8(%ebp)" 中找到了第一个参数.

![](/assets/PDB/HK/TH000663.png)
![](/assets/PDB/HK/TH000664.png)

当 "\_\_attribute\_\_((regparm(n)))" n 为 1 时，此时函数只使用 EAX 寄存器传递参数，可以看出其余参数都通过堆栈传递。上图 174 行从 EAX 寄存器中获得第一个参数，其余参数都从
堆栈 "offset(%ebp)" 中找获得.

![](/assets/PDB/HK/TH000665.png)
![](/assets/PDB/HK/TH000666.png)

当 "\_\_attribute\_\_((regparm(n)))" n 为 2 时，此时函数使用 EAX 寄存器传递第一个参数，使用 EDX 寄存器传递第二个参数，其余参数都通过堆栈传递。上图 172 行通过 EAX 传递第>一个参数，172 行通过 EDX 寄存器传递第二个参数， 其余参数从堆栈 "offset(%ebp)" 中获得.

![](/assets/PDB/HK/TH000658.png)
![](/assets/PDB/HK/TH000667.png)

当 "\_\_attribute\_\_((regparm(n)))" n 为 3 时，此时函数使用 EAX 寄存器传递第一个参数，使用 EDX 寄存器传递第二个参数，使用 ECX 寄存器传递第三个参数，其余参数通过堆栈传递
。上图 153 行通过 EAX 寄存器获得第一个参数，152 行通过 EDX 寄存器获得第二个参数，151 行通过 ECX 寄存器获得第三个寄存器，其余通过 "offset(%ebp)" 从堆栈中获得.

![](/assets/PDB/HK/TH000660.png)

在 BiscuitOS 加载实践模块之后，触发内核核心转储，此时数组的地址是 0xe0a5e000, 那么可以看到 EAX 寄存器指向第一个参数，正好是 0xe0a5e000; EDX 寄存器指向第二个参数，其值为 0xe0a5e004, 这样是数组的第二个成员地址; 同理 ECX 寄存器指向第三个参数，其值为 0xe0a5e008, 正好是数组的第三个成员地址.

另外，X86 总共有三种调用约定，Cdecl、stdcall 和 fastcall 调用规范，三者的区别:


* Cdecl 调用规范: 参数从右往左一次入栈，调用者实现栈平衡，返回值存放在 EAX
* stdcall调用规范: 参数从右往左一次入栈，被调用者实现栈平衡，返回值存放在 EAX
* fastcall 调用规范: 参数 1、参数 2 分别保存在 ECX、EDX, 剩下的参数从右往左一次入栈，被调用者实现栈平衡，返回值存放在 EAX

![](/assets/PDB/BiscuitOS/kernel/IND000100.png)

--------------------------------

###### <span id="E003">ARM64 架构</span>

![](/assets/PDB/HK/TH000668.png)

在 ARM64 架构中，使用 X0-X7 寄存器传递参数，第一个参数通过 X0 寄存器传递，第二个参数通过 X1 寄存器传递，以此类推. 返回值存储在 X0 寄存器中.

![](/assets/PDB/HK/TH000669.png)

当函数参数的数量超过 8 个之后，那么多余的参数从右到左依次入栈，被调用者平衡堆栈.接下来通过一个实例来实践整个过程，BiscuitOS 已经支持该实例实践，其在 BiscuitOS 中的部署如
下:

{% highlight bash %}
cd BiscuitOS
make linux-5.0-aarch_defconfig
make menuconfig

  [*] Package  --->
      [*] CRASH and BUG Example  --->
          [*] Example: Function arguments  --->

BiscuitOS/output/linux-5.0-aarch/package/BiscuitOS-crash-register-with-func-argument-default
{% endhighlight %}

> [BiscuitOS-crash-register-with-func-argument-default Source Code](https://gitee.com/BiscuitOS_team/HardStack/tree/Gitee/Tools/Crash/BiscuitOS-crash-register-with-func-argument)
>
> [BiscuitOS 独立程序实践教程](/blog/Human-Knowledge-Common/#C)

![](/assets/PDB/HK/TH000652.png)

实践案例为一个内核模块，在实践案例中定义了一个函数 BiscuitOS_func_arguments(), 其包含了 20 个参数，并在调用该函数时会触发内核转储，因此可以通过汇编和内核转储信息查看函数
参数传递过程，首先使用如下命令获得汇编文件:

{% highlight bash %}
cd BiscuitOS/output/linux-5.0-aarch/package/BiscuitOS-crash-register-with-func-argument-default
make
vi BiscuitOS-crash-register-with-func-argument-default/main.s
{% endhighlight %}

![](/assets/PDB/HK/TH000671.png)

* 38 行第一个参数通过 X0 寄存器传递
* 37 行第二个参数通过 X1 寄存器传递
* 36 行第三个参数通过 X2 寄存器传递
* 35 行第四个参数通过 X3 寄存器传递
* 34 行第五个参数通过 X4 寄存器传递
* 33 行第六个参数通过 X5 寄存器传递
* 29 行第七个参数通过 X6 寄存器传递
* 57 行第八个参数通过 X7 寄存器传递
{% highlight bash %}
ldp reg, reg, [x29, offset]
{% endhighlight %}

汇编程序在 12 行到 18 行通过将 x29 寄存器指向参数在堆栈中的位置，然后通过寄存器加偏移的方式将参数的内容读出. 汇编源码第 19 行从堆栈中读出第 15 和第 16 个参数; 第 20 行从
堆栈中读出第 17 和第 18 个参数; 第 21 行从堆栈中读出第 19 和第 20 个参数; 第 22 行从堆栈中读出第 9 和第 10 个参数; 第 23 行从堆栈中读出第 11 和第 12 个参数; 第 24 行从堆
栈中读出第 13 和第 14 个参数. 接着在 BiscuitOS 上运行并获得内核转储文件:

{% highlight bash %}
cd BiscuitOS/output/linux-5.0-aarch/package/BiscuitOS-crash-register-with-func-argument-default
make
make install
make pack
make run
{% endhighlight %}

![](/assets/PDB/HK/TH000672.png)

当在 BiscuitOS 中加载模块 BiscuitOS-crash-register-with-func-argument-default.ko，由于代码中对空指针的引用，此时内核触发 panic，此时内核打印出内核转储时寄存器的信息，从>运行可知 data 数组的地址为 0xffff000008b42000, 那么寄存器与数组的关系:

* X0: 0xffff000008b42000 DATA[0]: 0xffff000008b42000
* X1: 0xffff000008b42004 DATA[1]: 0xffff000008b42004
* X2: 0xffff000008b42008 DATA[1]: 0xffff000008b42008
* X3: 0xffff000008b4200c DATA[1]: 0xffff000008b4200c
* X4: 0xffff000008b42010 DATA[1]: 0xffff000008b42010
* X5: 0xffff000008b42014 DATA[1]: 0xffff000008b42014
* X6: 0xffff000008b42018 DATA[1]: 0xffff000008b42018
* X7: 0xffff000008b4201c DATA[1]: 0xffff000008b4201c

寄存器的值与分析一致，实践符合预期.

![](/assets/PDB/BiscuitOS/kernel/IND000100.png)

--------------------------------

###### <span id="E004">ARM 架构</span>

![](/assets/PDB/HK/TH000674.png)

在 ARM32 架构中，使用 R0-R3 寄存器传递参数，第一个参数通过 R0 寄存器传递，第二个参数通过 R1 寄存器传递，以此类推. 返回值存储在 R0 寄存器中.

![](/assets/PDB/HK/TH000675.png)

当函数参数的数量超过 4 个之后，那么多余的参数从右到左依次入栈，被调用者平衡堆栈.接下来通过一个实例来实践整个过程，BiscuitOS 已经支持该实例实践，其在 BiscuitOS 中的部署如
下:

{% highlight bash %}
cd BiscuitOS
make linux-5.0-arm32_defconfig
make menuconfig

  [*] Package  --->
      [*] CRASH and BUG Example  --->
          [*] Example: Function arguments  --->

BiscuitOS/output/linux-5.0-arm32/package/BiscuitOS-crash-register-with-func-argument-default
{% endhighlight %}

> [BiscuitOS-crash-register-with-func-argument-default Source Code](https://gitee.com/BiscuitOS_team/HardStack/tree/Gitee/Tools/Crash/BiscuitOS-crash-register-with-func-argument)
>
> [BiscuitOS 独立程序实践教程](/blog/Human-Knowledge-Common/#C)

![](/assets/PDB/HK/TH000652.png)

实践案例为一个内核模块，在实践案例中定义了一个函数 BiscuitOS_func_arguments(), 其包含了 20 个参数，并在调用该函数时会触发内核转储，因此可以通过汇编和内核转储信息查看函数
参数传递过程，首先使用如下命令获得汇编文件:

{% highlight bash %}
cd BiscuitOS/output/linux-5.0-arm32/package/BiscuitOS-crash-register-with-func-argument-default
make
vi BiscuitOS-crash-register-with-func-argument-default/main.s
{% endhighlight %}

![](/assets/PDB/HK/TH000673.png)

* 30 行第一个参数通过 R0 寄存器传递
* 29 行第二个参数通过 R1 寄存器传递
* 28 行第三个参数通过 R3 寄存器传递
* 65 行第四个参数通过 R4 寄存器传递

{% highlight bash %}
lsr     reg, [sp, #offset]
{% endhighlight %}

汇编程序通过在堆栈加偏移的方式将参数的内容读出. 汇编源码第 36 行从堆栈中读出第 20 个参数; 第 58 行从堆栈中读出第 13 个参数， 以此类推. 接着在 BiscuitOS 上运行并获得内核>转储文件:

{% highlight bash %}
cd BiscuitOS/output/linux-5.0-arm32/package/BiscuitOS-crash-register-with-func-argument-default
make
make install
make pack
make run
{% endhighlight %}

![](/assets/PDB/HK/TH000676.png)

当在 BiscuitOS 中加载模块 BiscuitOS-crash-register-with-func-argument-default.ko，由于代码中对空指针的引用，此时内核触发 panic，此时内核打印出内核转储时寄存器的信息，从>运行可知 data 数组的地址为 0xffff000008b42000, 那么寄存器与数组的关系:

* R0: 0x7f002000 DATA[0]: 0x7f002000
* R1: 0x7f002004 DATA[1]: 0x7f002004
* R2: 0x7f002008 DATA[2]: 0x7f002008
* R3: 0x7f00200c DATA[3]: 0x7f00200c

从上图可以看出堆栈从 "0x9dae3d10" 到 "0x9dae4000" 的内容，那么第五个参数的在堆栈中的地址是 "0x9dae3d88", 具体对应关系如下, 实践结果符合预期:

* Stack 0x9dae3d88: 0x7f002010 DATA[04]: 0x7f002010
* Stack 0x9dae3d8c: 0x7f002014 DATA[05]: 0x7f002014
* Stack 0x9dae3d90: 0x7f002018 DATA[06]: 0x7f002018
* Stack 0x9dae3d94: 0x7f00201c DATA[07]: 0x7f00201c
* Stack 0x9dae3d98: 0x7f002020 DATA[08]: 0x7f002020
* Stack 0x9dae3d9c: 0x7f002024 DATA[09]: 0x7f002024
* Stack 0x9dae3da0: 0x7f002028 DATA[10]: 0x7f002028
* Stack 0x9dae3da4: 0x7f00202c DATA[11]: 0x7f00202c
* Stack 0x9dae3da8: 0x7f002030 DATA[12]: 0x7f002030
* Stack 0x9dae3dac: 0x7f002034 DATA[13]: 0x7f002034
* Stack 0x9dae3db0: 0x7f002038 DATA[14]: 0x7f002038
* Stack 0x9dae3db4: 0x7f00203c DATA[15]: 0x7f00203c
* Stack 0x9dae3db8: 0x7f002040 DATA[16]: 0x7f002040
* Stack 0x9dae3dbc: 0x7f002044 DATA[17]: 0x7f002044
* Stack 0x9dae3dc0: 0x7f002048 DATA[18]: 0x7f002048
* Stack 0x9dae3dc4: 0x7f00204c DATA[19]: 0x7f00204c

![](/assets/PDB/BiscuitOS/kernel/IND000100.png)

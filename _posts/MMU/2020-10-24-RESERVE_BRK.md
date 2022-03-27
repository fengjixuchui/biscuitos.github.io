---
layout: post
title:  "RESERVE_BRK 内存分配器"
date:   2020-10-24 09:39:30 +0800
categories: [HW]
excerpt: MMU.
tags:
  - MMU
---

![](/assets/PDB/BiscuitOS/kernel/IND00000L0.PNG)

![](/assets/PDB/RPI/RPI100100.png)

#### 目录

> - [RESERVE_BRK 内存分配器原理](#A)
>
> - [RESERVE_BRK 内存分配器使用](#B)
>
> - [RESERVE_BRK 内存分配器实践](#C)
>
> - [RESERVE_BRK 内存分配器源码分析](#D)
>
> - [附录/捐赠](#Z0)

![](/assets/PDB/BiscuitOS/kernel/IND000100.png)

----------------------------------

<span id="A"></span>

![](/assets/PDB/BiscuitOS/kernel/IND00000T.jpg)

#### RESERVE_BRK 内存分配器原理

RESERVE_BRK 内存分配器称为动态堆内存分配器，其在 i386/x86-64 架构 boot-time 阶段分配物理内存，由于该阶段其他内存分配器都没有初始化，内核要么使用静态编译时 ".bss" 或 ".data" 的静态物理内存，要么使用 RESERVE_BRK 分配器动态的从堆中分配物理内存，因此 RESERVE_BRK 分配器是内核运行时最早的物理内存分配器。

![](/assets/PDB/HK/HK000602.png)

RESERVE_BRK 内存分配器的实现逻辑很简单，正如上图，内核使用 \_\_brk_base 和 \_\_brk_limit 描述系统堆的内存空间，RESERVE_BRK 内存分配器则使用 \_brk_start 和 \_brk_end 描述其分配的物理内存空间。当 RESERVE_BRK 分配器需要分配内存的时候，其将 \_brk_end 向高地址移动指定长度，此时 \_brk_start 到 \_brk_end 的区域转换为 RESERVE_BRK 分配器分配出去的物理内存，而系统堆则缩小到 \_brk_end 到 \_\_brk_limit，那么系统堆只能使用从 \_brk_end 开始的地方，所以 RESERVE_BRK 分配器称为动态堆分配器。

RESERVE_BRK 分配器的初始化或者构建比较简单，其使用 "RESERVE_BRK()" 函数进行完成定义，使用如下:

{% highlight c %}
RESERVE_BRK(name, size);
{% endhighlight %}

name 表示 RESERVE_BRK 分配器导出的分配接口，size 参数则表示该分配接口可分配物理内存的大小。当在内核中调用 RESERVE_BRK() 函数之后，内核会动态的插入一个名为 ".brk_reservation" 的 Section，该 Section 内部填充了 size 参数长度的数据，并且在内核链接阶段，内核将所有的 ".brk_reservation" Section 全部放到了系统 ".brk" Section 内，如下:

{% highlight c %}
arch/x86/kernel/vmlinux.lds.S

        . = ALIGN(PAGE_SIZE);
        .brk : AT(ADDR(.brk) - LOAD_OFFSET) {
                __brk_base = .;
                . += 64 * 1024;         /* 64k alignment slop space */
                *(.brk_reservation)     /* areas brk users have reserved */
                __brk_limit = .;
        }
{% endhighlight %}

因此可以推出当调用 RESERVE_BRK() 函数的时候，内核已经在堆中提前预留指定长度的内存，这样在系统运行时可以直接动态调整堆的范围，但 RESERVE_BRK 分配器不能超过自己预留区域，否则会带来未知的后果。

当 RESERVE_BRK 分配器在堆中预留好内存区域之后，接下来就是从 RESERVE_BRK 分配器中分配物理内存了，分配的核心就是动态调整 \_brk_end 的值，是系统原始堆收缩来实现分配，具体分配过程请查看:

> [RESERVE_BRK 内存分配器使用](#B)

![](/assets/PDB/HK/HK000607.png)

RESERVE_BRK 分配器只存在与内核初始化的 boot-time 阶段，在这个阶段可以自由使用 RESERVE_BRK 分配器，当内核初始化到一定阶段之后，其他内存分配器初始化完毕可以使用之后，RESERVE_BRK 分配器将其分配的物理内存在系统中预留的方式结束其生命，具体可以查看:

> [RESERVE_BRK 分配器声明周期](#D0)

![](/assets/PDB/BiscuitOS/kernel/IND000100.png)

---------------------------------

<span id="B"></span>

![](/assets/PDB/BiscuitOS/kernel/IND00000H.jpg)

#### RESERVE_BRK 内存分配器使用

RESERVE_BRK 内存分配器的使用很简单，首先从堆中划分一段内存进行分配，然后直接使用这段内存，但 RESERVE_BRK 分配器不回收这段内存，而是将其在系统里一直预留。开发者可以参考如下代码进行使用，使用时请注意不要超过 RESERVE_BRK 分配器的声明周期，RESERVE_BRK 分配器生命周期请参考如下文章:

> [RESERVE_BRK 分配器声明周期](#D0)

{% highlight c %}
#include <asm/setup.h>

RESERVE_BRK(BiscuitOS_BRK, PAGE_SIZE);

void * __init BiscuitOS_alloc(unsigned len)
{
        return extend_brk(len, sizeof(int));
}
{% endhighlight %}

正如上面的代码，定义了一个基于 RESERVE_BRK 分配器定义了一个内存分配接口 "BiscuitOS_alloc()"。对 RESERVE_BRK 分配器的使用首先使用 "RESERVE_BRK()" 函数定义一个分配接口，以及这个分配接口占用 RESERVE_BRK 分配区域的大小，其次通过调用 extend_brk() 函数实现实际的分配操作。RESERVE_BRK 分配器通过维护 \_brk_start 和 \_brk_end 来表示其管理的内存区域，而堆则使用 \_brk_end 和 \_\_brk_limit 维护堆的大小，如下图:

![](/assets/PDB/HK/HK000602.png)

当 RESERVE_BRK 分配器需要分配内存的时候，extend_brk 就会修改 \_brk_end 的值，使其指向更高的地址，换句话来说就是缩小堆的范围来实现 RESERVE_BRK 分配器的内存分配。定义完毕分配接口之后，只要在 RESERVE_BRK 分配器的生命周期之内，都可以按如下代码进行使用:

{% highlight c %}
extern void * __init BiscuitOS_alloc(unsigned len);

void __init BiscuitOS_demo(void)
{
	char *buffer;

	buffer = BiscuitOS_alloc(0x20);
	sprintf(buffer, "BiscuitOS");
	printk("=> %s [%#lx]\n", buffer, __pa(buffer));
}
{% endhighlight %}

以上代码就是简单的通过 RESERVE_BRK 分配器进行内存分配，具体实践过程请参考:

> [RESERVE_BRK 内存分配器实践](#C)

![](/assets/PDB/BiscuitOS/kernel/IND000100.png)

----------------------------------

<span id="C"></span>

![](/assets/PDB/BiscuitOS/kernel/IND00000I.jpg)

#### RESERVE_BRK 内存分配器实践

> - [实践准备](#C0000)
>
> - [实践部署](#C0001)
>
> - [实践执行](#C0002)

![](/assets/PDB/BiscuitOS/kernel/IND000100.png)

--------------------------------------------

#### <span id="C0000">实践准备</span>

RESERVE_BRK 内存分配器实践目前支持 x86_64 和 i386 架构，开发者可以自行选择，本文以 i386 架构进行讲解，并推荐使用该架构来实践 RESERVE_BRK 分配器。首先开发者基于 BiscuitOS 搭建一个 i386 架构的开发环境，请开发者参考如下文档，如果想要以 x86-64 架构进行搭建，搭建过程类似，开发者参考搭建:

> - [BiscuitOS Linux 5.0 i386 环境部署](/blog/Linux-5.0-i386-Usermanual/)
>
> - [BiscuitOS Linux 5.0 X86_64 环境部署](/blog/Linux-5.0-x86_64-Usermanual/)

![](/assets/PDB/BiscuitOS/kernel/IND000100.png)

--------------------------------------------

#### <span id="C0001">实践部署</span>

在部署完毕开发环境之后, 开发者可以在汇编代码阶段使用 RESERVE_BRK 分配器，也可以在 C 代码中使用 RESERVE_BRK 分配器，但要确保在 RESERVE_BRK 分配器寿命终结之前使用，其声明周期可以查看:

> [RESERVE_BRK 分配器声明周期](#D0)

RESERVE_BRK 分配器的实践与其他分配器实践不太相同，开发者可以参考本节的建议去实践
，首先在 "init/main.c" 函数的合适位置添加如下代码:

{% highlight c %}
#include <asm/setup.h>

RESERVE_BRK(BiscuitOS_BRK, PAGE_SIZE);

static void * __init BiscuitOS_alloc(unsigned len)
{
        return extend_brk(len, sizeof(int));
}
{% endhighlight %}

定义完上面的函数之后，接着在 "init/main.c" 文件的 start_kernel() 函数里，"setup_arch()" 函数的前一行插入如下代码:

{% highlight c %}
asmlinkage __visible void __init start_kernel(void)
{

        ...
        pr_notice("%s", linux_banner);

        {
                char *buffer;

                buffer = BiscuitOS_alloc(0x20);
                sprintf(buffer, "BiscuitOS");
                printk("=> %s [%#lx]\n", buffer, __pa(buffer));
        }

        setup_arch(&command_line);
        ...
}
{% endhighlight %}

至此，源码部署完毕，开发者可以参考上面的代码进行部署，只要 RESERVE_BRK 分配器周>期范围内都可以使用上面的代码. 接下来在 BiscuitOS 上查看运行效果.

![](/assets/PDB/BiscuitOS/kernel/IND000100.png)

--------------------------------------------

#### <span id="C0002">实践执行</span>

部署完毕之后，接下来重新编译内核并运行 BiscuitOS:

![](/assets/PDB/HK/HK000606.png)

由于 RESERVE_BRK 内存分配器位于内核初始化早期，调试手段有限，只能通过 dmesg 查看相应的信息，从图中可以看出可以正常使用 RESERVE_BRK 分配器分配的内容.

![](/assets/PDB/BiscuitOS/kernel/IND000100.png)

----------------------------------

<span id="D"></span>

![](/assets/PDB/BiscuitOS/kernel/IND00000H.jpg)

#### RESERVE_BRK 内存分配器源码解析

> - RESERVE_BRK 架构流程
>
>   - [RESERVE_BRK X86 架构流程](#D0)
>
> - RESERVE_BRK 分配器重要数据
>
>   - [\_brk_start](#D10)
>
>   - [\_brk_end](#D10)
>
>   - [\_\_brk_limit](#D10)
>
>   - [\_\_brk_base](#D10)
>
> - RESERVE_BRK 分配器 API
>
>   - [extend_brk](#302)
>
>   - [reserve_brk](#D303)
>
>   - [RESERVE_BRK](#D300)
>
>   - [RESERVE_BRK_ARRAY](#D301)

![](/assets/PDB/BiscuitOS/kernel/IND000100.png)

---------------------------------------------

#### <span id="D0">RESERVE_BRK X86 架构流程</span>

![](/assets/PDB/HK/HK000605.png)

RESERVE_BRK 内存分配器存在于 X86/X64 架构中，其生命周期如上图。在内核源码中，通过调用 RESERVE_BRK() 函数会自动插入一个指定的 section 到系统的 .brk section 内，并在编译链接的时候为其分配内存，这些内存就构成了 RESERVE_BRK 分配器管理的内存。内核镜像加载到 RAM 上运行的时候，RESERVE_BRK 分配器使用的内存与堆重叠，并且 \_brk_start 与 \_brk_end 在初始化的时候都是指向堆的基地址 \_\_brk_base. 但系统运行过程中，内核某个功能模块调用 RESERVE_BRK 分配器分配内存，那么 \_brk_end 的值就动态变大，但不能超过 \_\_brk_limit, 而 \_brk_start 则保持不变，这样 \_brk_start 到 \_brk_end 的区域就形成了 RESERVE_BRK 分配器分配并管理的内存区域。内核继续初始化，但内核初始化到 reserve_brk() 函数之后，内核将 \_brk_start 到 \_brk_end 的区域一直预留到系统关闭，函数介绍后 RESERVE_BRK 分配器不能在使用，因此完成使命终结.

![](/assets/PDB/BiscuitOS/kernel/IND000100.png)

---------------------------------------------

#### <span id="D303">reserve_brk</span>

![](/assets/PDB/HK/HK000604.png)

reserve_brk() 函数的作用是在内核初始化到一定阶段，将 RESERVE_BRK 分配器管理的内存区域进行预留. 函数如果检测到 \_brk_end 大于 \_brk_start, 那么表示 RESERVE_BRK 分配器为系统分配过内存，且这段内存需要在系统中一直预留，因此函数调用 memblock_reserve() 函数进行预留。预留完毕之后函数将 \_brk_start 设置为 0，表示 RESERVE_BRK 分配器不再分配内存。

> [> \_brk_start](#D10)
>
> [> \_brk_end](#D10)

![](/assets/PDB/BiscuitOS/kernel/IND000100.png)

---------------------------------------------

#### <span id="D302">extend_brk</span>

![](/assets/PDB/HK/HK000603.png)

extend_brk() 函数的作用是动态调整堆的范围，以此从堆中分配物理内存。参数 size 表示缩减堆的长度，align 参数表示对齐方式。

![](/assets/PDB/HK/HK000602.png)

正如上图，堆的生长是从低地址到高地址，且内核使用 \_brk_end 动态表示堆栈的基地址。如果 RESERVE_BRK 分配器需要从堆中分配内存，那么内核将 \_brk_end 往高地址移动，那么 \_brk_start 与 \_brk_end 之间的区域形成了 RESERVE_BRK 分配器管理的分配的区域。函数在 257 行检测 \_brk_start 是否为 0，\_brk_start 为 0 表示 RESERVE_BRK 分配器已经不能使用。接着函数在 258 行检测 align 是否越界. 函数在 260 行先跳转 \_brk_end 的值，使其对齐，接着检测 \_brk_end 加上 size 之后是否已经超出 \_\_brk_limit, 即堆最大地址. 如果没有，那么函数在 263 - 264 行将当前 \_brk_end 的地址存储在 ret 中，并将 \_brk_end 的值增加 size，指向新的堆基地址。最后函数将 ret 开始的区域清零并返回。以上便完整了以此 RESERVE_BRK 的物理内存分配。

> [> \_brk_start](#D10)
>
> [> \_brk_end](#D10)
>
> [> \_\_brk_limit](#D10)
>
> [> \_\_brk_base](#D10)

![](/assets/PDB/BiscuitOS/kernel/IND000100.png)

---------------------------------------------

#### <span id="D10">BRK SECTION</span>

![](/assets/PDB/HK/HK000602.png)

内核在链接阶段，将所有的 .brk_reservation Section 全部放到 .brk Section 内，并且使用 \_\_brk_base 指向该 Section 的起始地址，\_\_brk_limit 指向该 Section 的终止地址，可以在链接脚本中获得相关定义:

{% highlight bash %}
arch/x86/kernel/vmlinux.lds.S

        . = ALIGN(PAGE_SIZE);
        .brk : AT(ADDR(.brk) - LOAD_OFFSET) {
                __brk_base = .;
                . += 64 * 1024;         /* 64k alignment slop space */
                *(.brk_reservation)     /* areas brk users have reserved */
                __brk_limit = .;
        }
{% endhighlight %}

其次，在支持 RESERVE_BRK 内存分配器的情况下，内核会使用两个地址表示 RESERVE_BRK 内存分配器管理的物理内存空间，其范围是 "\_brk_start, \_brk_end", 这段物理内存区域就是专门用于为 RESERVE_BRK 内存分配器用于分配的空间，但值得注意的是，每当 RESERVE_BRK 分配一段内存之后，\_brk_end 的值就会增大，为了确保内核堆的正常使用，因此内核将 \_brk_end 作为堆的起始地址.

因此从上面的讨论可以知道 \_\_brk_base 和 \_\_brk_limit 都是在链接时候定义好的，而 \_brk_end 是可以动态改变的，因此这些是动态堆栈实现的9基础.

![](/assets/PDB/BiscuitOS/kernel/IND000100.png)

---------------------------------------------

#### <span id="D301">RESERVE_BRK_ARRAY</span>

![](/assets/PDB/HK/HK000601.png)

RESERVE_BRK_ARRAY() 函数类似于 KMEM_CACHE 分配方式，一次性为某个数据结构分配多个对象的空间。参数 type 指明数据结构的类型，参数 name 是 Section 名字的一部分，参数 entries 指明对象的数量.

RESERVE_BRK_ARRAY() 函数基于 RESERVE_BRK() 函数构建，其长度通过 sizeof(type) 和 entries 相乘的值确定，以此计算 Section 的长度.

> [RESERVE_BRK](#D300)

![](/assets/PDB/BiscuitOS/kernel/IND000100.png)

---------------------------------------------

#### <span id="D300">RESERVE_BRK</span>

![](/assets/PDB/HK/HK000599.png)

RESERVE_BRK() 函数用于向系统插入一个指定长度的 ".brk_reservation" section，并在内核编译链接的生成 vmlinux 的时候将该 section 插入到 .brk section 内。参数 name 是 section 名字的一部分, 参数 sz 表示 section 的长度。

第一种定义基于 \_\_ASSEMBLY\_\_ 宏没有定义，在这种情况下处于非汇编模式，函数在 108 - 109 行定义了一个名为 \_\_brk_reservation_fn_xx\_\_() 函数，函数名字 xx 部分就是参数 name，因此调用 RESERVE_BRK() 时，name 参数一定要是唯一的。函数内部实现中，在 111 行，函数调用 ".pushsection" 命令动态插入一个 ".brk_reservation" section，该 section 具有可读写属性，接着函数在 112 行定义了一个名为 ".brk.xx" 的 lab，xx 为 name 参数。函数通过 113 - 114 行定义了该 section 的长度为 sz。最后函数在 115 行调用 ".popsection" 完成该 section 的创建。创建完毕之后，内核会在 vmlinux 链接时将这个 section 插入到 ".brk" Section 内，并在内核启动时候分配指定长度的物理内存给 ".brk" section.
 
![](/assets/PDB/HK/HK000600.png)

第二种定义基于 \_\_ASSEMBLY\_\_ 宏已经定义，在这种情况下处于汇编模式。函数在 137 行直接调用 ".pushsection" 指令插入一个 ".brk_reservation" 的可读可写 Section。并在 138 行新增一个 lable ".brk.name"，并通过 139 - 140 行定义了 Section 的长度，最后在 141 行调用 .popsection 指令结束该 Section 的定义.

![](/assets/PDB/BiscuitOS/kernel/IND000100.png)

-----------------------------------------------

#### <span id="Z0">附录</span>

> [BiscuitOS Home](https://biscuitos.github.io/)
>
> [BiscuitOS Driver](/blog/BiscuitOS_Catalogue/)
>
> [Linux Kernel](https://www.kernel.org/)
>
> [Bootlin: Elixir Cross Referencer](https://elixir.bootlin.com/linux/latest/source)
>

#### 捐赠一下吧 🙂

![MMU](/assets/PDB/BiscuitOS/kernel/HAB000036.jpg)

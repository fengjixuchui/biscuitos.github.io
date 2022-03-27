---
layout: post
title:  "用户空间始化为零静态变量的虚拟地址"
date:   2018-12-13 11:21:30 +0800
categories: [MMU]
excerpt: 用户空间始化为零静态变量的虚拟地址.
tags:
  - MMU
  - VAS
---

**Architecture: I386 32bit Mechine Ubuntu 16.04**

**Kernel: 4.15.0-39-generic**

# 目录

> 1. 静态变量
>
> 2. 始化为零的静态变量
>
> 3. 实践
>
> 4. 分析
>
> 5. 总结
>
> 6. 附录

--------------------------------------------------------

# 静态变量

C 语言中的静态变量是一个只能被本文件可见的变量。静态变量定义在源文件的函数之外，使用 static 显式声明。静态变量定义如下：

{% highlight ruby %}
demo.c

static int glob_inta;
static int glob_intb = 1;
static int glob_intc = 0;

int fun()
{
    static int local_inta;
    static int local_intb = 1;
    static int local_intc = 0;

    return 0;
}
{% endhighlight %}

一个程序中可以定义多个静态变量。当编译器将源文件编译为目标 ELF 文件之后，全局
变量可以存储在 ELF 目标文件的 .data 段内，也可以存储在 .bss 段内。当程序执行的
时候，静态变量会被链接到程序的 .data 段内或 .bss 段内。

----------------------------------------------------------

# 始化为零静态变量

始化为零的静态变量是静态变量中的一种，其在定义之后进行显式的初始化赋值为零。
始化为零的静态变量定义如下：

{% highlight ruby %}
demo.c

static int demo_inta = 0;

int fun()
{
    return 0;
}
{% endhighlight %}

#### ELF 目标文件中的始化为零静态变量

始化为零的静态变量在经过编译汇编之后，会被存储到 ELF 目标文件的 .bss 段内，并
不占用 ELF 目标的存储空间。

#### 进程中的始化为零静态变量

当程序运行之后，始化为零的静态变量会被加载到进程的 .bss 段内，操作系统会为未初
始化静态变量分配指定的虚拟地址空间。

---------------------------------------------------------

# 实践

BiscuitOS 提供了始化为零静态变量相关的实例代码，开发者可以使用如下命令：
首先，开发者先准备 BiscuitOS 系统，内核版本 linux 1.0.1.2。开发可以参照文档构建 BiscuitOS 调试环境：

[Linux 1.0.1.2 内核构建方法](/blog/Linux1.0.1.2_ext2fs_Usermanual/)

接着，开发者配置内核，使用如下命令：

{% highlight ruby %}
cd BiscuitOS
make clean
make update
make linux_1_0_1_2_ext2_defconfig
make
cd BiscuitOS/kernel/linux_1.0.1.2/
make clean
make menuconfig
{% endhighlight %}

由于 BiscuitOS 的内核使用 Kbuild 构建起来的，在执行完 make menuconfig 之后，系
统会弹出内核配置的界面，开发者根据如下步骤进行配置：

![Menuconfig](/assets/PDB/BiscuitOS/kernel/MMU000003.png)

选择 **kernel hacking**，回车

![Menuconfig1](/assets/PDB/BiscuitOS/kernel/MMU000004.png)

选择 **Demo Code for variable subsystem mechanism**, 回车

![Menuconfig2](/assets/PDB/BiscuitOS/kernel/MMU000005.png)

选择 **MMU(Memory Manager Unit) on X86 Architecture**, 回车

![Menuconfig3](/assets/PDB/BiscuitOS/kernel/MMU000433.png)

选择 **Debug MMU(Memory Manager Unit) mechanism on X86 Architecture** 之后选择
**Addressing Mechanism**  回车

![Menuconfig3](/assets/PDB/BiscuitOS/kernel/MMU000434.png)

选择 **Virtual address**, 回车

![Menuconfig3](/assets/PDB/BiscuitOS/kernel/MMU000435.png)

选择 **Virtual address and Virtual space** 之后，接着选择 
**Choice Kernel/User Virtual Address Space**, 回车。

![Menuconfig3](/assets/PDB/BiscuitOS/kernel/MMU000436.png)

该选项用于选择程序运行在用户空间还是内核空间，这里选择用户空间。选择 
**User Virtual Address Space**. 回车之后按 Esc 退出。

![Menuconfig3](/assets/PDB/BiscuitOS/kernel/MMU000437.png)

接着选择 Choice Architecture Compiler ，回车。

![Menuconfig3](/assets/PDB/BiscuitOS/kernel/MMU000438.png)

这个选项用于指定用户程序运行的平台。开发者可以根据自己需求选择，这里推荐选择
**Intel i386 (32bit) Mechine**, 回车并按 Esc 退出。

![Menuconfig3](/assets/PDB/BiscuitOS/kernel/MMU000452.png)

最后开发者选择 **.data segment**,下拉菜单打开后，选择 
**Static Inited Zero Data** 选项，回车保存并退出。

运行实例代码，使用如下代码：

{% highlight ruby %}
cd BiscuitOS/kernel/linux_1.0.1.2/
make 
cd tools/demo/mmu/addressing/virtual_address/user/
./data.elf
{% endhighlight %}

![Menuconfig3](/assets/PDB/BiscuitOS/kernel/MMU000453.png)

-----------------------------------------------------------

# 分析

初始化为零的静态变量是静态变量中的一种，其声明之后赋值为零。在编译和汇编阶段，
初始化为零的静态变量会被放置到 ELF 文件的 .bss 段，并且不占用 ELF 的空间。在程
序运行之后，进程会将初始化为零的全局放到进程的 .bss 段并分配相应的虚拟空间。这
里我们将分析初始化为零的静态变量的生命周期。

开发者可以使用实例源码，源码位置：

{% highlight ruby %}
BiscuitOS/kernel/linux_1.0.1.2/tools/demo/mmu/addressing/virtual_address/user/data.c
{% endhighlight %}

如源码所示，始化为零静态变量定义如下：

{% highlight ruby %}
data.c

#ifdef CONFIG_DEBUG_VA_USER_DATA_SINITZERO
/* Static inited zero data */
static char  SInitZero_char  = 0;
static short SInitZero_short = 0;
static int   SInitZero_int   = 0;
static long  SInitZero_long  = 0;
static char  SInitZeroA_char[ARRAY_LEN]  = { 0, 0, 0, 0};
static short SInitZeroA_short[ARRAY_LEN] = { 0, 0, 0, 0};
static int   SInitZeroA_int[ARRAY_LEN]   = { 0, 0, 0, 0};
static long  SInitZeroA_long[ARRAY_LEN]  = { 0, 0, 0, 0};
static char  *SInitZeroP_char  = NULL;
static short *SInitZeroP_short = NULL;
static int   *SInitZeroP_int   = NULL;
static long  *SInitZeroP_long  = NULL;
#endif
{% endhighlight %}

始化为零的静态变量定义在函数之外，这里定义了各种类型的变量，包括变量，数组和指
针。接着开发者对源文件进行编译汇编，以此生成 ELF 文件，使用如下命令：

{% highlight ruby %}
cd BiscuitOS/kernel/linux_1.0.1.2/
make
cd tools/demo/mmu/addressing/virtual_address/user/
{% endhighlight %}

默认情况下，运行 make 命令之后，在目录下就会生成对应的 ELF 文件和反汇编文件 
**data.objdump.elf** 文件。开发者可以通过查看 data.objdump.elf 文件查看未初始
化静态变量在 ELF 文件中的布局。

![Menuconfig3](/assets/PDB/BiscuitOS/kernel/MMU000454.png)

从上图可以看出，始化为零的静态变量在 ELF 文件中被放置到 .bss 段中，但不占用 ELF
的空间。接下来开发者查看运行时，始化为零静态变量在进程中的布局。

用户空间程序来运行之前需要链接脚本进行链接运行，所以链接脚本影响着进程的虚拟内
存布局。开发者可以使用如下命令查看链接脚本的内容，如下：

{% highlight ruby %}
cd tools/demo/mmu/addressing/virtual_address/user/
cat ld_scripts.elf
{% endhighlight %}

![Menuconfig3](/assets/PDB/BiscuitOS/kernel/MMU000442.png)

从链接脚本中，PROVIDE 宏可以将变量导入到进程中，所以开发者可以在源码中使用这些
变量来指明进程的内存布局，这里使用了 **__executable_start**, **edata**, 
**etext**, 和 **end**。如下：

{% highlight ruby %}
data.c

int main()
{
    /* Defined on ld-scripts */
    char stack_bsp;
    extern char __executable_start[];
    extern char edata[];
    extern char etext[];
    extern char end[];
    char *heap_base;
    unsigned long *mmap_base = NULL;
    int mmap_fd;

    heap_base = (char *)malloc(sizeof(char));
    mmap_fd = open("/dev/mem", O_RDONLY);
    if (mmap_fd > 0)
        mmap_base = (unsigned long *)mmap(0, 4096, PROT_READ, MAP_SHARED,
mmap_fd, 50000000);

    printf("***************************************************************\n");
    printf("Data Segment: .data Describe\n\n");
    printf("+---+-------+-------+------+--+------+-+------+--------------+\n");
    printf("|   |       |       |      |  |      | |      |              |\n");
    printf("|   | .text | .data | .bss |  | Heap | | Mmap |        Stack |\n");
    printf("|   |       |       |      |  |      | |      |              |\n");
    printf("+---+-------+-------+------+--+------+-+------+--------------+\n");
    printf("0                                                           4G\n");
    printf("Executable start: %#08lx\n", (unsigned long)__executable_start);
    printf("Data Range:       %#08lx -- %#08lx\n", (unsigned long)etext,
                                                   (unsigned long)edata);
    printf("BSS  Range:       %#08lx -- %#08lx\n", (unsigned long)edata,
                                                   (unsigned long)end);
    printf("Heap Base:        %#08lx\n", (unsigned long)heap_base);
    printf("Mmap Base:        %#08lx\n", (unsigned long)mmap_base);
    printf("Stack:            %#08lx\n", (unsigned long)&stack_bsp);
printf("***************************************************************\n");


    if (mmap_base)
        munmap(mmap_base, 4096);
    if (mmap_fd > 0)
        close(mmap_fd);
    free(heap_base);
}
{% endhighlight %}

如上图源码，引用了很多链接脚本定义的变量和自定义了一些局部变量，这些变量分别代
表：

> 1. __executable_start 进程开始执行的虚拟地址
>
> 2. etext 指向进程代码段结束的虚拟地址，也就是进程数据段开始的位置
>
> 3. edata 指向进程数据段结束的位置，也就是 .BSS 段开始的位置
>
> 4. end 指向 BSS 段结束的位置
>
> 5. stack_bsp 指向 main 函数的局部堆栈特定位置
>
> 6. heap_base 指向堆的特定地址，这里使用 malloc 进行 main 函数的第一次堆分配
>    获得地址
> 7. mmap_base 指向进程映射某段物理内存之后的虚拟地址


编译汇编完之后，接下来开发者开始链接运行这个程序，使用如下命令：

{% highlight ruby %}
cd BiscuitOS/kernel/linux_1.0.1.2/
make 
cd tools/demo/mmu/addressing/virtual_address/user/
./data.elf
{% endhighlight %}

![Menuconfig3](/assets/PDB/BiscuitOS/kernel/MMU000453.png)

从运行结果来看，所有始化为零的静态变量地址被加载到从 0x804a040 增长到 
0x804a084，从进程的布局可以知道，BSS 段的范围从 0x804a03c 增加到 0x804a088. 所
以始化为零的静态变量都被进程加载到了 BSS 段，并分配了相应的虚拟地址。

------------------------------------------------------

# 总结

通过实践，实践的结果和预期一样，始化为零静态变量进过编译汇编之后，在 ELF 文件
中被存储到 .bss 段，并且不占用 ELF 文件的空间。当程序运行时，进程会将始化为零
的静态变量加载到进程的 .bss 段，并分配对应的虚拟内存。

------------------------------------------------------

# 附录

如有疑问，请联系 BuddyZhang1

{% highlight ruby %}
buddy.zhang@aliyun.com
{% endhighlight %}

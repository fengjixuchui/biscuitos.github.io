---
layout: post
title:  "用户空间未初始化局部变量的虚拟地址"
date:   2018-12-13 17:12:30 +0800
categories: [MMU]
excerpt: 用户空间未初始化局部变量的虚拟地址.
tags:
  - MMU
  - VAS
---

> Architecture: I386 32bit Mechine Ubuntu 16.04
>
> Kernel: 4.15.0-39-generic

# 目录

> 1. 局部变量
>
> 2. 未初始化的局部变量
>
> 3. 实践
>
> 4. 分析
>
> 5. 总结
>
> 6. 附录

--------------------------------------------------------

# 局部变量

C 语言中的局部变量是在函数里面定义的变量，只有函数内部可见。局部变量定义如下：

{% highlight ruby %}
demo.c

int fun()
{
    int local_demoa;
    int local_demob = 0;
    int local_democ = 2;

    return 0;
}
{% endhighlight %}

一个程序中可以定义多个局部变量。当编译器将源文件编译为目标 ELF 文件之后，局部
变量已经由编译器决定，也就是说，当进程调用这个函数的时候，已经规定好应该从堆栈
分配多少的空间给局部变量。

----------------------------------------------------------

# 未初始化局部变量

未初始化的局部变量是局部变量中的一种，其在定义之后未进行显式的初始化赋值，操作
系统不会对其进行赋值，所以未初始化的局部变量初始化是一个随机值。未初始化的局部
变量定义如下：

{% highlight ruby %}
demo.c

int fun()
{
    int demo_inta;

    return 0;
}
{% endhighlight %}

#### ELF 目标文件中的未初始化局部变量

未初始化的局部变量在经过编译汇编之后，已由编译器决定，也就是说编译器已经指明函
数被调用时，应该给局部变量分配多少的堆栈空间。

#### 进程中的未初始化局部变量

当程序运行之后，进程调用这个函数的时候，首先从堆栈中分配足够的空间给局部变量。

---------------------------------------------------------

# 实践

BiscuitOS 提供了未初始化局部变量相关的实例代码，开发者可以使用如下命令：
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

![Menuconfig3](/assets/PDB/BiscuitOS/kernel/MMU000467.png)

最后开发者选择 **.data segment**,下拉菜单打开后，选择 
**Local Un-Initialize Data** 选项，回车保存并退出。

运行实例代码，使用如下代码：

{% highlight ruby %}
cd BiscuitOS/kernel/linux_1.0.1.2/
make 
cd tools/demo/mmu/addressing/virtual_address/user/
./data.elf
{% endhighlight %}

![Menuconfig3](/assets/PDB/BiscuitOS/kernel/MMU000468.png)

-----------------------------------------------------------

# 分析

未初始化的局部变量是局部变量中的一种，其声明之后未被初始化，其值是一个随机值。
在编译和汇编阶段，未初始化的局部变量已经由编译器决定好分配的空间。当程序运行之
后，进程调用到这个函数时，会从堆栈分配足够的空间给未初始化的局部变量，但其初始
值是一个随机值。接下来将通过实践来认识未初始化局部变量的生命周期。

开发者可以使用实例源码，源码位置：

{% highlight ruby %}
BiscuitOS/kernel/linux_1.0.1.2/tools/demo/mmu/addressing/virtual_address/user/data.c
{% endhighlight %}

如源码所示，未初始化局部变量定义如下：

{% highlight ruby %}
data.c

int main()
{
#ifdef CONFIG_DEBUG_VA_USER_DATA_LNINIT
    /* Local un-initialize data */
    char  LUnInit_char;
    short LUnInit_short;
    int   LUnInit_int;
    long  LUnInit_long;
    char  LUnInitA_char[ARRAY_LEN];
    short LUnInitA_short[ARRAY_LEN];
    int   LUnInitA_int[ARRAY_LEN];
    long  LUnInitA_long[ARRAY_LEN];
    char  LUnInitP_char;
    short LUnInitP_short;
    int   LUnInitP_int;
    long  LUnInitP_long;
#endif

    return 0;
}
{% endhighlight %}

未初始化的局部变量定义在函数之外，这里定义了各种类型的变量，包括变量，数组和指
针。接着开发者对源文件进行编译汇编，以此生成 ELF 文件，使用如下命令：

{% highlight ruby %}
cd BiscuitOS/kernel/linux_1.0.1.2/
make
cd tools/demo/mmu/addressing/virtual_address/user/
{% endhighlight %}

默认情况下，运行 make 命令之后，在目录下就会生成对应的 ELF 文件和反汇编文件 
**data.objdump.elf** 文件。开发者可以通过查看 data.objdump.elf 文件查看未初始
化局部变量在 ELF 文件中的布局。

![Menuconfig3](/assets/PDB/BiscuitOS/kernel/MMU000469.png)

从上图可以看出，main 函数内，汇编代码 sub $0x88,%esp 指定了局部变量空间。

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

![Menuconfig3](/assets/PDB/BiscuitOS/kernel/MMU000468.png)

从运行结果来看，所有未初始化的局部变量地址被加载到从 0xff845d5e 递减到 
0xff845d70，从进程的布局可以知道，Stack 的位置在 0xff845d5d. 所以未初始化局部
变量位于进程的堆栈中。

------------------------------------------------------

# 总结

通过实践，实践的结果和预期一样，未初始化局部变量进过编译汇编之后，编译器已经决
定了未初始化局部变量在进程中的位置。程序运行之后，进程调用这个函数的时候，会从
堆栈中分配特定虚拟内存给未初始化的局部变量。

------------------------------------------------------

# 附录

如有疑问，请联系 BuddyZhang1

{% highlight ruby %}
buddy.zhang@aliyun.com
{% endhighlight %}

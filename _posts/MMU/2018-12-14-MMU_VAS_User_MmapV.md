---
layout: post
title:  "用户空间内存映射变量的虚拟地址"
date:   2018-12-14 09:26:30 +0800
categories: [MMU]
excerpt: 用户空间内存映射变量的虚拟地址.
tags:
  - MMU
  - VAS
---

> Architecture: I386 32bit Mechine Ubuntu 16.04
>
> Kernel: 4.15.0-39-generic

# 目录

> 1. 内存映射变量
>
> 2. 实践
>
> 3. 分析
>
> 4. 总结
>
> 5. 附录

--------------------------------------------------------

# 内存映射变量

C 语言中的内存映射变量是使用 mmap 函数将物理地址映射到进程虚拟地址空间，这部分
内存只能通过匿名方式，即指针方式访问。内存映射变量定义如下：

{% highlight ruby %}
demo.c

int fun()
{
    unsigned long *mmap_base = NULL;
    int mmap_fd;

    heap_base = (char *)malloc(sizeof(char));
    mmap_fd = open("/dev/mem", O_RDONLY);
    if (mmap_fd > 0)
        mmap_base = (unsigned long *)mmap(0, 4096, PROT_READ, MAP_SHARED,
                                                  mmap_fd, 50000000);

    if (mmap_base)
        munmap(mmap_base, 4096);
    if (mmap_fd > 0)
        close(mmap_fd);
    return 0;
}
{% endhighlight %}

进程可以使用 mmap 方式将物理地址映射到进程的 mmap 区，进程需要通过匿名的方式才
能访问，也就是通过指针的方式进程访问。当进程不再使用这部分虚拟内存之后，需要使
用 munmap() 函数释放掉。

---------------------------------------------------------

# 实践

BiscuitOS 提供了始化为非零内存映射变量相关的实例代码，开发者可以使用如下命令：
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

![Menuconfig3](/assets/PDB/BiscuitOS/kernel/MMU000480.png)

最后开发者选择 **.data segment**,下拉菜单打开后，选择 
**Mmap Data** 选项，回车保存并退出。

运行实例代码，使用如下代码：

{% highlight ruby %}
cd BiscuitOS/kernel/linux_1.0.1.2/
make 
cd tools/demo/mmu/addressing/virtual_address/user/
./data.elf
{% endhighlight %}

![Menuconfig3](/assets/PDB/BiscuitOS/kernel/MMU000481.png)

-----------------------------------------------------------

# 分析

内存映射变量是使用 mmap 函数将物理地址直接映射到进程的 mmap 区域，进程可以通过
匿名方式访问这段虚拟内存，也就是通过指针的方式访问。当进程不再需要访问这段虚拟
内存之后，可以使用 munmap 函数将其释放给操作系统。这里将研究内存映射变量的声明
周期。

开发者可以使用实例源码，源码位置：

{% highlight ruby %}
BiscuitOS/kernel/linux_1.0.1.2/tools/demo/mmu/addressing/virtual_address/user/data.c
{% endhighlight %}

如源码所示，始化为非零内存映射变量定义如下：

{% highlight ruby %}
data.c

int main()
{

    unsigned long *mmap_base = NULL;
    int mmap_fd;

    heap_base = (char *)malloc(sizeof(char));
    mmap_fd = open("/dev/mem", O_RDONLY);
    if (mmap_fd > 0)
        mmap_base = (unsigned long *)mmap(0, 4096, PROT_READ, MAP_SHARED,
                                                  mmap_fd, 50000000);


#ifdef CONFIG_DEBUG_VA_USER_DATA_MMAP
    /* Data on MMAP */
    char *MP_char   = NULL;
    short *MP_short = NULL;
    int   *MP_int   = NULL;
    long  *MP_long  = NULL;

    MP_char  = (char *)((unsigned long)mmap_base + 0);
    MP_short = (short *)((unsigned long)mmap_base + 2);
    MP_int   = (int *)((unsigned long)mmap_base + 4);
    MP_long  = (long *)((unsigned long)mmap_base + 8);
#endif

    if (mmap_base)
        munmap(mmap_base, 4096);
    if (mmap_fd > 0)
        close(mmap_fd);
    return 0;
}
{% endhighlight %}


内存映射变量定义在函数之内或之外，这里定义了各种类型的指针。接着开发者对源文件
进行编译汇编，以此生成 ELF 文件，使用如下命令：

{% highlight ruby %}
cd BiscuitOS/kernel/linux_1.0.1.2/
make
cd tools/demo/mmu/addressing/virtual_address/user/
{% endhighlight %}

默认情况下，运行 make 命令之后，在目录下就会生成对应的 ELF 文件和反汇编文件 
**data.objdump.elf** 文件。开发者可以通过查看 data.objdump.elf 文件查看未初始
化内存映射变量在 ELF 文件中的布局。

![Menuconfig3](/assets/PDB/BiscuitOS/kernel/MMU000482.png)

从上图可以看出，main 函数内，汇编代码从 0x6c 行调用 mmap 函数进程映射。

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

如上图源码，引用了很多链接脚本定义的变量和自定义了一些内存映射变量，这些变量分别代
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

![Menuconfig3](/assets/PDB/BiscuitOS/kernel/MMU000481.png)

从运行结果来看，进程并未成功映射物理地址，这可能与 CPU 有关。开发者可以通过修
改源码中映射的物理地址，以此可以成功 mmap 到物理内存。

------------------------------------------------------

# 总结

通过实践，由于没有 mmap 成功，验证失败。待 mmap 成功之后在讨论。

------------------------------------------------------

# 附录

如有疑问，请联系 BuddyZhang1

{% highlight ruby %}
buddy.zhang@aliyun.com
{% endhighlight %}

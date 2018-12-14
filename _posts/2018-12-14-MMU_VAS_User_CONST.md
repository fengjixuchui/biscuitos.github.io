---
layout: post
title:  "用户空间常量的虚拟地址"
date:   2018-12-14 16:22:30 +0800
categories: [MMU]
excerpt: 用户空间常量的虚拟地址.
tags:
  - MMU
  - VAS
---

> Architecture: I386 32bit Mechine Ubuntu 16.04
>
> Kernel: 4.15.0-39-generic

# 目录

> 1. 常量
>
> 2. 实践
>
> 3. 分析
>
> 4. 总结
>
> 5. 附录

--------------------------------------------------------

# 常量

C 语言中的常量是固定值，在程序执行期间不会改变，这些固定值又叫做字面量，常量可
以使任何的基础数据类型，比如整形常量，字符常量；也有枚举常量。常量就像是常规的
变量，只不过常量的值在定义以后不能修改。常量的定义如下：

{% highlight ruby %}
demo.c

#define CONST_STR    "Hello BiscuitOS"

int const const_int = 0x99;

enum CONST_DATA {
    CONST_ZERO,
    CONST_ONE,
    CONST_TWO,
    CONST_THREE
};
enum CONST_DATA CDATA;

int fun()
{
    return 0;
}
{% endhighlight %}

常量的定义有三种方法：

> 1. 使用 #define 宏定义
>
> 2. 使用 const 关键定义
>
> 3. 使用枚举定义

---------------------------------------------------------

# 实践

BiscuitOS 提供了始化为非零常量相关的实例代码，开发者可以使用如下命令：
首先，开发者先准备 BiscuitOS 系统，内核版本 linux 1.0.1.2。开发可以参照文档构建 BiscuitOS 调试环境：

[Linux 1.0.1.2 内核构建方法](https://biscuitos.github.io/blog/Linux1.0.1.2_ext2fs_Usermanual/)

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

![Menuconfig](https://raw.githubusercontent.com/EmulateSpace/PictureSet/master/BiscuitOS/kernel/MMU000003.png)

选择 **kernel hacking**，回车

![Menuconfig1](https://raw.githubusercontent.com/EmulateSpace/PictureSet/master/BiscuitOS/kernel/MMU000004.png)

选择 **Demo Code for variable subsystem mechanism**, 回车

![Menuconfig2](https://raw.githubusercontent.com/EmulateSpace/PictureSet/master/BiscuitOS/kernel/MMU000005.png)

选择 **MMU(Memory Manager Unit) on X86 Architecture**, 回车

![Menuconfig3](https://raw.githubusercontent.com/EmulateSpace/PictureSet/master/BiscuitOS/kernel/MMU000433.png)

选择 **Debug MMU(Memory Manager Unit) mechanism on X86 Architecture** 之后选择
**Addressing Mechanism**  回车

![Menuconfig3](https://raw.githubusercontent.com/EmulateSpace/PictureSet/master/BiscuitOS/kernel/MMU000434.png)

选择 **Virtual address**, 回车

![Menuconfig3](https://raw.githubusercontent.com/EmulateSpace/PictureSet/master/BiscuitOS/kernel/MMU000435.png)

选择 **Virtual address and Virtual space** 之后，接着选择 
**Choice Kernel/User Virtual Address Space**, 回车。

![Menuconfig3](https://raw.githubusercontent.com/EmulateSpace/PictureSet/master/BiscuitOS/kernel/MMU000436.png)

该选项用于选择程序运行在用户空间还是内核空间，这里选择用户空间。选择 
**User Virtual Address Space**. 回车之后按 Esc 退出。

![Menuconfig3](https://raw.githubusercontent.com/EmulateSpace/PictureSet/master/BiscuitOS/kernel/MMU000437.png)

接着选择 Choice Architecture Compiler ，回车。

![Menuconfig3](https://raw.githubusercontent.com/EmulateSpace/PictureSet/master/BiscuitOS/kernel/MMU000438.png)

这个选项用于指定用户程序运行的平台。开发者可以根据自己需求选择，这里推荐选择
**Intel i386 (32bit) Mechine**, 回车并按 Esc 退出。

![Menuconfig3](https://raw.githubusercontent.com/EmulateSpace/PictureSet/master/BiscuitOS/kernel/MMU000483.png)

最后开发者选择 **.data segment**,下拉菜单打开后，选择 
**Const Data** 选项，回车保存并退出。

运行实例代码，使用如下代码：

{% highlight ruby %}
cd BiscuitOS/kernel/linux_1.0.1.2/
make 
cd tools/demo/mmu/addressing/virtual_address/user/
./data.elf
{% endhighlight %}

![Menuconfig3](https://raw.githubusercontent.com/EmulateSpace/PictureSet/master/BiscuitOS/kernel/MMU000484.png)

-----------------------------------------------------------

# 分析

C 语言中的常量是固定值，在程序执行期间不会改变，这些固定值又叫做字面量，常量
可以使任何的基础数据类型，比如整形常量，字符常量；也有枚举常量。常量就像是常
规的变量，只不过常量的值在定义以后不能修改。这里将研究常量的声明周期。

开发者可以使用实例源码，源码位置：

{% highlight ruby %}
BiscuitOS/kernel/linux_1.0.1.2/tools/demo/mmu/addressing/virtual_address/user/data.c
{% endhighlight %}

如源码所示，始化为非零常量定义如下：

{% highlight ruby %}
data.c

#ifdef CONFIG_DEBUG_VA_USER_DATA_CONST
#define CONST_STR    "Hello BiscuitOS"

int const const_int = 0x99;

enum CONST_DATA {
    CONST_ZERO,
    CONST_ONE,
    CONST_TWO,
    CONST_THREE
};
enum CONST_DATA CDATA;
#endif

int main()
{
    return 0;
}
{% endhighlight %}


常量定义在函数之内或之外，这里定义了各种类型的指针。接着开发者对源文件
进行编译汇编，以此生成 ELF 文件，使用如下命令：

{% highlight ruby %}
cd BiscuitOS/kernel/linux_1.0.1.2/
make
cd tools/demo/mmu/addressing/virtual_address/user/
{% endhighlight %}

默认情况下，运行 make 命令之后，在目录下就会生成对应的 ELF 文件和反汇编文件 
**data.objdump.elf** 文件。开发者可以通过查看 data.objdump.elf 文件查看未初始
化常量在 ELF 文件中的布局。

![Menuconfig3](https://raw.githubusercontent.com/EmulateSpace/PictureSet/master/BiscuitOS/kernel/MMU000485.png)

从上图可以看出，常量被放置到 .rodata 段里面，且在 main 函数内，汇编代码从 
0x194 到 0x199 行调用对常量进行引用。

用户空间程序来运行之前需要链接脚本进行链接运行，所以链接脚本影响着进程的虚拟内
存布局。开发者可以使用如下命令查看链接脚本的内容，如下：

{% highlight ruby %}
cd tools/demo/mmu/addressing/virtual_address/user/
cat ld_scripts.elf
{% endhighlight %}

![Menuconfig3](https://raw.githubusercontent.com/EmulateSpace/PictureSet/master/BiscuitOS/kernel/MMU000442.png)

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

如上图源码，引用了很多链接脚本定义的变量和自定义了一些常量，这些变量分别代
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

![Menuconfig3](https://raw.githubusercontent.com/EmulateSpace/PictureSet/master/BiscuitOS/kernel/MMU000484.png)

根据链接脚本可知，.rodata 段紧跟 .text 之后，所以从实际运行的例子可知，.text 
段结束的位置是 0x8048848, 常量的地址从 0x8048a72  增加到 0x8048850，由于枚举
变量无法打印地址。所以综上所定义的常量全部位于 .rodata 段里。

------------------------------------------------------

# 总结

常量在源码就被定义为固定值，编译汇编生成 ELF 之后，常量被放置到 ELF 文件的 .
rodata 段内。当程序运行时，常量放置在进程的 .rodata 虚拟空间内，直到程序运行
结束，常量的值都不改变。

------------------------------------------------------

# 附录

如有疑问，请联系 BuddyZhang1

{% highlight ruby %}
buddy.zhang@aliyun.com
{% endhighlight %}

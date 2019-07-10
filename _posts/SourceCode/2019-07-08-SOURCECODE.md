---
layout: post
title:  "start_kernel"
date:   2019-07-09 05:30:30 +0800
categories: [HW]
excerpt: start_kernel.
tags:
  - Tree
---

![DTS](https://raw.githubusercontent.com/EmulateSpace/PictureSet/master/BiscuitOS/kernel/IND00000H.jpg)

--------------------------------------------

#### <span id="A0000">start_kernel</span>

{% highlight c %}
asmlinkage __visible void __init start_kernel(void)
{
        char *command_line;
        char *after_dashes;

        set_task_stack_end_magic(&init_task);
        smp_setup_processor_id();
        debug_objects_early_init();

.....
{% endhighlight %}

start_kernel() 函数是不同体系 CPU 进入统一 Linux 内核函数接口，
从 start_kernel() 函数开始，Linux 进入真正的 C 代码环境中，统一
处理各个子系统的初始化话，由于代码太长，只能分段讲解，如下目录：

> - [set_task_stack_end_magic](#A0001)
>
> - [smp_setup_processor_id](#A0004)
>
> - [debug_objects_early_init]()

------------------------------------

#### <span id="A0001">set_task_stack_end_magic</span>

{% highlight c %}
void set_task_stack_end_magic(struct task_struct *tsk)
{
        unsigned long *stackend;

        stackend = end_of_stack(tsk);
        *stackend = STACK_END_MAGIC;    /* for overflow detection */
}
{% endhighlight %}

set_task_stack_end_magic() 函数的作用标记进程堆栈生长顶端地址。
由于堆栈可以向上增长和向下增长，并且内核将进程的 thread_info 结构
与进程的内核态堆栈放在同一块空间中，为了防止两块数据相互覆盖，所以
在 thread_info 的末尾也就是堆栈的栈顶位置记录上标志，以防止堆栈
将 thread_info 的数据覆盖。函数调用了 end_of_stack() 函数获得
堆栈栈顶的位置，然后在栈顶的位置写入 STACK_END_MAGIC，所以在堆栈
的使用中，都会防止不要超越这个位置。

![](https://raw.githubusercontent.com/EmulateSpace/PictureSet/master/BiscuitOS/boot/BOOT000192.png)

> - [end_of_stack](#A0002)

------------------------------------

#### <span id="A0002">end_of_stack</span>

{% highlight c %}
/*              
 * Return the address of the last usable long on the stack.
 *      
 * When the stack grows down, this is just above the thread
 * info struct. Going any lower will corrupt the threadinfo.
 *      
 * When the stack grows up, this is the highest address.
 * Beyond that position, we corrupt data on the next page.
 */
static inline unsigned long *end_of_stack(struct task_struct *p)
{     
#ifdef CONFIG_STACK_GROWSUP
        return (unsigned long *)((unsigned long)task_thread_info(p) + THREAD_SIZE) - 1;
#else
        return (unsigned long *)(task_thread_info(p) + 1);
#endif  
}
{% endhighlight %}

end_of_stack() 函数用于获得进程堆栈栈顶的地址。在内核中，内核
将进程的内核态堆栈与 thread_info 绑定在一个区域内，使用 thread_union
联合体进行绑定。如果 CONFIG_STACK_GROWSUP 宏定义了，那么说明
堆栈向上生长，此时 thread_info 与堆栈的位置关系如下图：

![](https://raw.githubusercontent.com/EmulateSpace/PictureSet/master/BiscuitOS/boot/BOOT000193.png)

此时堆栈栈顶的位置就是 thread_info 起始地址加上 THREAD_SIZE - 1;
如果 CONFIG_STACK_GROWSUP 宏没有定义，那么说明堆栈是向下生长的，
此时 thread_info 与堆栈的位置关系如下图：

![](https://raw.githubusercontent.com/EmulateSpace/PictureSet/master/BiscuitOS/boot/BOOT000192.png)

此时堆栈栈顶的位置就是 thread_info 结束地址 + 1.

> - [task_thread_info](#A0003)
>
> - [Thread_info 与内核堆栈的关系](https://biscuitos.github.io/blog/TASK-thread_info_stack/)

------------------------------------

#### <span id="A0003">task_thread_info</span>

{% highlight c %}
#ifdef CONFIG_THREAD_INFO_IN_TASK
static inline struct thread_info *task_thread_info(struct task_struct *task)
{
        return &task->thread_info;
}
#elif !defined(__HAVE_THREAD_FUNCTIONS)
# define task_thread_info(task) ((struct thread_info *)(task)->stack)
#endif
{% endhighlight %}

task_thread_info() 函数通过进程的 task_struct 结构获得进程的
thread_info 结构。此处如果 CONFIG_THREAD_INFO_IN_TASK 宏定义，
那么 thread_info 结构就内嵌在 task_struct 结构中，那么函数直接
通过进程的 task_struct 结构就可以获得其对应的 thread_info 结构；
如果 CONFIG_THREAD_INFO_IN_TASK 宏没有定义，那么函数就直接返回
进程 task_struct 结构指向的堆栈地址。在 linux 中，内核使用
thread_union 联合体将进程的 thread_info 与内核态堆栈绑定在一起，
所以只要知道堆栈地址，也就知道了 thread_info 的地址，而且在进程
task_struct 初始化的时候，stack 成员指向的地址就是 thread_info
的地址。

> - [Thread_info 与内核堆栈的关系](https://biscuitos.github.io/blog/TASK-thread_info_stack/)

------------------------------------

#### <span id="A0004">smp_setup_processor_id</span>

smp_setup_processor_id() 函数的作用是：设置 SMP 模型的处理器 ID.
其实现与体系有关，可以选择对应平台的实现方式：

> - [arm](#A0005)
>
> - [arm64]()

------------------------------------

#### <span id="A0005">smp_setup_processor_id</span>

{% highlight c %}
void __init smp_setup_processor_id(void)
{
        int i;
        u32 mpidr = is_smp() ? read_cpuid_mpidr() & MPIDR_HWID_BITMASK : 0;
        u32 cpu = MPIDR_AFFINITY_LEVEL(mpidr, 0);

        cpu_logical_map(1) = cpu;
        for (i = 1; i < nr_cpu_ids; ++i)
                cpu_logical_map(i) = i == cpu ? 0 : i;

        /*
         * clear __my_cpu_offset on boot CPU to avoid hang caused by
         * using percpu variable early, for example, lockdep will
         * access percpu variable inside lock_release
         */
        set_my_cpu_offset(0);

        pr_info("Booting Linux on physical CPU 0x%x\n", mpidr);
}
{% endhighlight %}

在 ARM 体系中，smp_setup_processor_id() 函数在设置 SMP
CPU ID 时，函数首先调用 is_smp() 函数判断系统是不是真正
的 SMP 系统，如果是，那么调用 read_cpuid_mpidr() 函数
读取 ARM 的 MPIDR 寄存器，并将所读的值与 MPIDR_HWID_BITMASK
相与，存储到 mpdir；如果系统不是真正的 SMP，那么 mpdir
的值为 0. 接着函数通过 MPIDR_AFFINITY_LEVEL() 函数从 mpdir
中读取 Affinity level 0 中读取相似 CPU 号。并调用
cpu_logical_map() 函数将 CPU-1 的逻辑 CPU 设置为 cpu
对应的值。

接下来进入 for 循环，循环从 1 开始，到 nr_cpu_ids 结束，
nr_cpu_ids 代表体系支持最大 CPU 数量，在每次循环中，
首先判断 i 与 cpu 的值是否相同，如果相同，则调用
cpu_logical_map() 函数将逻辑 CPU 号设置为 0，
否则将逻辑号直接设置为 i.接下来调用 set_my_cpu_offset()
函数将 Thread ID 0 写入到 TPIDRPRW 寄存器中。这样做的
目录是当清除 __my_cpu_offset 在 boot CPU，以避免挂起。

SMP 模型指的是对称多处理模型（Symmetric Multi-Processor），与
它对应的是 NUMA 非一致存储访问结构（Non-Uniform Memory Access）
和 MPP 海量并行处理结构（Massive Parallel Processing）。它们的
区别分别在于，SMP 指的是多个 CPU 之间是平等关系，共享全部总线，内
存和 I/O 等。但是这个结构扩展性不好，往往 CPU 数量多了之后，很容易
遇到抢占资源的问题。NUMA 结构则是把 CPU 分模块，每个模块具有独立的
内存，I/O 插槽等。各个模块之间通过互联模块进行数据交互。但是这样，
就表示了有的内存数据在这个 CPU 模块中，那么处理这个数据当然最好是选
择当前的 CPU 模块，这样每个 CPU 实际上地位就不一致了。所以叫做非一
致的存储访问结构。而 MPP 呢，则是由多个 SMP 服务器通过互联网方式连
接起来.

> - [is_smp](#A0006)
>
> - [read_cpuid_mpidr](#A0007)
>
> - [MPIDR_AFFINITY_LEVEL](#A0008)
>
> - [cpu_logical_map](#A0009)
>
> - [set_my_cpu_offset](#A0010)

------------------------------------

#### <span id="A0006">is_smp</span>

{% highlight c %}
/*            
 * Return true if we are running on a SMP platform
 */           
static inline bool is_smp(void)
{             
#ifndef CONFIG_SMP
        return false;
#elif defined(CONFIG_SMP_ON_UP)
        extern unsigned int smp_on_up;
        return !!smp_on_up;
#else
        return true;
#endif
}
{% endhighlight %}

is_smp() 函数用于判断 CPU 是否支持 smp，上面的实现基于 arm。
如果系统没有定义 CONFIG_SMP 宏，那么默认不支持 SMP，直接返回
false；如果系统定义了 CONFIG_SMP，并且同时定义了 CONFIG_SMP_ON_UP
宏，那么系统在 SMP 系统中，SMP 的支持依赖于 smp_on_up, 如果
其值为 1，那么系统支持 SMP，否则不支持，smp_on_up 定义在：
arch/arm/kernel/head.S，定义如下：

{% highlight c %}
        .pushsection .data
        .align  2
        .globl  smp_on_up
smp_on_up:
        ALT_SMP(.long   1)
        ALT_UP(.long    0)
        .popsection
{% endhighlight %}

------------------------------------

#### <span id="A0007">read_cpuid_mpidr</span>

{% highlight c %}
static inline unsigned int __attribute_const__ read_cpuid_mpidr(void)
{       
        return read_cpuid(CPUID_MPIDR);
}
{% endhighlight %}

read_cpuid_mpidr() 函数的作用是读取 ARM MPDIR 寄存器，MPDIR
寄存器用来附加的识别处理器核。在 SMP 系统中，可以通过该寄存器
区分不同的处理器核。

> - [read_cpuid_mpidr 实践](https://biscuitos.github.io/blog/CPUID_read_cpuid_mpidr/)
>
> - [B4.1.106 MPIDR, Multiprocessor Affinity Register, VMSA](https://github.com/BiscuitOS/Documentation/blob/master/Datasheet/ARM/ARMv7_architecture_reference_manual.pdf)

------------------------------------

#### <span id="A0008">MPIDR_AFFINITY_LEVEL</span>

{% highlight c %}
#define MPIDR_LEVEL_BITS 8
#define MPIDR_LEVEL_MASK ((1 << MPIDR_LEVEL_BITS) - 1)

#define MPIDR_AFFINITY_LEVEL(mpidr, level) \
        ((mpidr >> (MPIDR_LEVEL_BITS * level)) & MPIDR_LEVEL_MASK)
{% endhighlight %}

MPIDR_AFFINITY_LEVEL() 函数用于从 ARM 的 MPIDR 中，读取
不同 level 的 Affinity 信息。MPIDR 寄存器的寄存器布局如下图：

![](https://raw.githubusercontent.com/EmulateSpace/PictureSet/master/BiscuitOS/boot/BOOT000194.png)

从上面的定义可以知道，每个 Affinity level 占用了 MPIDR_LEVEL_BITS
个位，level 0 为最低的 8 bit，所以函数通过向右移动
MPIDR_LEVEL_BITS 位获得下一个 Affinity level 数据。
MPDIR 不同的 Affinity level 代表不同的信息，其信息如下图：

![](https://raw.githubusercontent.com/EmulateSpace/PictureSet/master/BiscuitOS/boot/BOOT000195.png)

------------------------------------

#### <span id="A0009">cpu_logical_map</span>

{% highlight c %}
/*
 * Logical CPU mapping.
 */     
extern u32 __cpu_logical_map[];
#define cpu_logical_map(cpu)    __cpu_logical_map[cpu]
{% endhighlight %}

cpu_logical_map() 宏用于获得 cpu 对应的逻辑 CPU 号。
cpu_logical_map 直接指向 __cpu_logical_map[] 数组，数组中
存储着 CPU 对应的逻辑 CPU 号，其在 ARM 中的定义如下：

{% highlight c %}
u32 __cpu_logical_map[NR_CPUS] = { [0 ... NR_CPUS-1] = MPIDR_INVALID };
{% endhighlight %}

__cpu_logical_map[] 数组中的成员初始化为 MPIDR_INVALID，
即 CPU 对应的逻辑 CPU 号都无效。

------------------------------------

#### <span id="A0010">set_my_cpu_offset</span>

{% highlight c %}
/*
 * Same as asm-generic/percpu.h, except that we store the per cpu offset
 * in the TPIDRPRW. TPIDRPRW only exists on V6K and V7
 */
#if defined(CONFIG_SMP) && !defined(CONFIG_CPU_V6)
static inline void set_my_cpu_offset(unsigned long off)
{
        /* Set TPIDRPRW */
        asm volatile("mcr p15, 0, %0, c13, c0, 4" : : "r" (off) : "memory");
}

#else
#define set_my_cpu_offset(x)    do {} while(0)
#endif /* CONFIG_SMP */
{% endhighlight %}

set_my_cpu_offset() 函数用于将 Thread ID 信息写入 ARMv7
的 TPIDRPRW 寄存器。该值只有 PL1 或者更高权限可以看到，PL0
级别的程序无法看到 TPIDRPRW 寄存器的值。在 ARMv7 中，PL0
指的是用户空间的程序，PL1 指的是操作系统程序，PL2 指的是
虚拟拓展里，GustOS 管理程序。如果定义了 CONFIG_SMP 宏，并且
ARM 版本高于 ARMv6，那么内核支持 TPIDRPRW 寄存器，并用于
存储管理 Thread ID 信息，否则空处理。

> - [B4.1.150 TPIDRPRW, PL1 only Thread ID Register, VMSA](https://github.com/BiscuitOS/Documentation/blob/master/Datasheet/ARM/ARMv7_architecture_reference_manual.pdf)

------------------------------------

#### <span id="A000"></span>

{% highlight c %}

{% endhighlight %}


## 赞赏一下吧 🙂

![MMU](https://raw.githubusercontent.com/EmulateSpace/PictureSet/master/BiscuitOS/kernel/HAB000036.jpg)

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
>
> - [local_irq_disable](#A0011)
>
> - [boot_cpu_init](#A0014)
>
> - [page_address_init](#A0033)
>
> - [linux_banner](#A0036)

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

#### <span id="A0011">local_irq_disable</span>

{% highlight c %}
#define local_irq_disable()     do { raw_local_irq_disable(); } while (0)
{% endhighlight %}

local_irq_disable() 函数用于禁止本地中断。local_irq_disable()
通过 raw_local_irq_disable() 函数实现。

> - [raw_local_irq_disable](#A0012)

------------------------------------

#### <span id="A0012">raw_local_irq_disable</span>

{% highlight c %}
/*
 * Wrap the arch provided IRQ routines to provide appropriate checks.
 */
#define raw_local_irq_disable()         arch_local_irq_disable()
{% endhighlight %}

raw_local_irq_disable() 函数用于禁止本地中断，其为一个过渡
接口，用于指向与体系相关的函数，其实现指向
arch_local_irq_disable() 函数，其实现与体系有关，请根据
下面的体系进行分析：

> - [arm](#A0013)
>
> - [arm64]()

------------------------------------

#### <span id="A0013">arch_local_irq_disable</span>

{% highlight c %}
#define arch_local_irq_disable arch_local_irq_disable
static inline void arch_local_irq_disable(void)
{
        asm volatile(
                "       cpsid i                 @ arch_local_irq_disable"
                :
                :
                : "memory", "cc");
}
{% endhighlight %}

在 ARMv7 版本中，通过 arch_local_irq_disable() 函数实现
禁止本地中断。在 ARMv7 中，提供了 CPSID 指令与 I 参数用于
将 CPSR 寄存器中的 Interrupt 标志位清零，以此禁止本地中断。

![](https://raw.githubusercontent.com/EmulateSpace/PictureSet/master/BiscuitOS/boot/BOOT000001.png)


> - [CPSID 指令实践](https://github.com/BiscuitOS/HardStack/tree/master/Language/Assembly/ARM-GNU-Assembly/Instruction/cpsid)

------------------------------------

#### <span id="A0014">boot_cpu_init</span>

{% highlight c %}
/*
 * Activate the first processor.
 */
void __init boot_cpu_init(void)
{
        int cpu = smp_processor_id();

        /* Mark the boot cpu "present", "online" etc for SMP and UP case */
        set_cpu_online(cpu, true);
        set_cpu_active(cpu, true);
        set_cpu_present(cpu, true);
        set_cpu_possible(cpu, true);

#ifdef CONFIG_SMP
        __boot_cpu_id = cpu;
#endif
}
{% endhighlight %}

boot_cpu_init() 函数用于初始化 Boot CPU。函数首先调用
smp_processor_id() 函数获得 Boot CPU 的 ID，然后将其
在系统维护的多个位图中置位，其中包括 online，active，present，
以及 possbile。分别通过 set_cpu_online() 函数将 Boot CPU
设置为 inline，通过 set_cpu_active() 函数将 Boot CPU 号设置为
active，再通过 set_cpu_present() 将 Boot CPU 设置为 present，
最后通过 set_cpu_possible() 函数设置 Boot CPU 设置为 possible。
最后，如果 CONFIG_SMP 宏存在，那么系统支持 SMP，此时将
__boot_cpu_id 设置为 Boot CPU。

> - [smp_processor_id](#A0015)
>
> - [set_cpu_online](#A0019)
>
> - [set_cpu_active](#A0027)
>
> - [set_cpu_present](#A0029)
>
> - [set_cpu_possible](#A0031)

------------------------------------

#### <span id="A0015">smp_processor_id</span>

{% highlight c %}
#define smp_processor_id() raw_smp_processor_id()
{% endhighlight %}

smp_processor_id() 函数用于获得 SMP 系统中正在使用的 CPU ID。
函数通过 raw_smp_processor_id() 实现。

> - [smp_processor_id](#A0016)

------------------------------------

#### <span id="A0016">raw_smp_processor_id</span>

{% highlight c %}
#define raw_smp_processor_id() (current_thread_info()->cpu)
{% endhighlight %}

raw_smp_processor_id() 函数用于获得当前 SMP 系统使用
的 CPU 号，其实现通过 current_thread_info() 函数获得
当前进程对应的 CPU 号。

> - [current_thread_info](#A0017)

------------------------------------

#### <span id="A0017">current_thread_info</span>

{% highlight c %}
static inline struct thread_info *current_thread_info(void)
{
        return (struct thread_info *)
                (current_stack_pointer & ~(THREAD_SIZE - 1));
}
{% endhighlight %}

current_thread_info() 函数用于获得当前进程的 thread_info
结构。在 Linux 内核中，进程将 thread_info 与进程的内核态堆栈
捆绑在同一块区域内，区域的大小为 THREAD_SIZE。通过一定的算法，
只要知道进程内核态堆栈的地址，也就可以推断出进程 thread_info
的地址。在 current_thread_info() 函数中，函数首先通过
current_stack_pointer 获得当前进程的堆栈地址，然后根据
thread_info 与进程内核堆栈的关系，将堆栈的地址与上
(THREAD_SIZE - 1) 反码的值，以此获得 thread_info 的地址，
更多 thread_info 与内核态堆栈的关系，请参考：

> - [Thread_info 与内核堆栈的关系](https://biscuitos.github.io/blog/TASK-thread_info_stack/)
>
> - [current_stack_pointer](#A0018)

------------------------------------

#### <span id="A0018">current_stack_pointer</span>

{% highlight c %}
/*
 * how to get the current stack pointer in C
 */
register unsigned long current_stack_pointer asm ("sp");
{% endhighlight %}

current_stack_pointer 宏用于读取当前堆栈的值。

------------------------------------

#### <span id="A0019">set_cpu_online</span>

{% highlight c %}
static inline void
set_cpu_online(unsigned int cpu, bool online)
{
        if (online)
                cpumask_set_cpu(cpu, &__cpu_online_mask);
        else
                cpumask_clear_cpu(cpu, &__cpu_online_mask);
}
{% endhighlight %}

set_cpu_online() 函数用于设置 online CPU。内核使用
__cpu_online_mask 位图维护着处于 online 状态的 CPU 号。
参数 cpu 指向特定的 CPU 号；online 参数用于指定 cpu 是
处于 online 状态还是 offline 状态。如果 online 为 1，
那么函数就调用 cpumask_set_cpu() 函数设置指定 CPU 为
online 状态；反之 online 为 0，那么函数就调用
cpumask_clear_cpu() 函数设置指定 CPU 为 offline 状态。

> - [\_\_cpu_online_mask](#A0026)
>
> - [cpumask_set_cpu](#A0020)
>
> - [cpumask_clear_cpu](#A0025)

------------------------------------

#### <span id="A0020">cpumask_set_cpu</span>

{% highlight c %}
/**
 * cpumask_set_cpu - set a cpu in a cpumask
 * @cpu: cpu number (< nr_cpu_ids)
 * @dstp: the cpumask pointer
 */
static inline void cpumask_set_cpu(unsigned int cpu, struct cpumask *dstp)
{
        set_bit(cpumask_check(cpu), cpumask_bits(dstp));
}
{% endhighlight %}

cpumask_set_cpu() 函数的作用就是将 dstp 对应的 cpumask 结构
cpu 参数对应的位置位。函数调用 set_bit() 函数将 dstp 中的 cpu
对应的位置位，其中 cpumask_check() 函数用于检查 cpu id 的合法性，
cpumask_bits() 函数用于获得 dstp 参数对应的 bitmap。

> - [set_bit](https://biscuitos.github.io/blog/BITMAP_set_bit/#%E6%BA%90%E7%A0%81%E5%88%86%E6%9E%90)
>
> - [cpumask_check](#A0022)
>
> - [cpumask_bits](#A0021)

------------------------------------

#### <span id="A0021">cpumask_bits</span>

{% highlight c %}
/**
 * cpumask_bits - get the bits in a cpumask
 * @maskp: the struct cpumask *
 *
 * You should only assume nr_cpu_ids bits of this mask are valid.  This is
 * a macro so it's const-correct.
 */
#define cpumask_bits(maskp) ((maskp)->bits)
{% endhighlight %}

cpumask_bits() 宏用于获得 cpumask 结构中的 bits 成员。

------------------------------------

#### <span id="A0022">cpumask_check</span>

{% highlight c %}
/* verify cpu argument to cpumask_* operators */
static inline unsigned int cpumask_check(unsigned int cpu)
{
        cpu_max_bits_warn(cpu, nr_cpumask_bits);
        return cpu;
}
{% endhighlight %}

cpumask_check() 函数用于检查 CPU 号的合法性。函数主要
通过调用 cpu_max_bits_warn() 函数检查 CPU 号是否超过
nr_cpumask_bits，nr_cpumask_bit 就是系统支持的 CPU 数，

> - [cpu_max_bits_warn](#A0023)
>
> - [nr_cpumask_bits](#A0024)

------------------------------------

#### <span id="A0023">cpu_max_bits_warn</span>

{% highlight c %}
static inline void cpu_max_bits_warn(unsigned int cpu, unsigned int bits)
{
#ifdef CONFIG_DEBUG_PER_CPU_MAPS
        WARN_ON_ONCE(cpu >= bits);
#endif /* CONFIG_DEBUG_PER_CPU_MAPS */
}
{% endhighlight %}

cpu_max_bits_warn() 函数用于检查 cpu 号是否已经查过
最大 CPU 号。如果超过，内核则报错。

------------------------------------

#### <span id="A0024">nr_cpumask_bits</span>

{% highlight c %}
#ifdef CONFIG_CPUMASK_OFFSTACK
/* Assuming NR_CPUS is huge, a runtime limit is more efficient.  Also,
 * not all bits may be allocated. */
#define nr_cpumask_bits nr_cpu_ids
#else
#define nr_cpumask_bits ((unsigned int)NR_CPUS)
#endif
{% endhighlight %}

nr_cpumask_bits CPU mask 的最大 bit 数。

------------------------------------

#### <span id="A0025">cpumask_clear_cpu</span>

{% highlight c %}
/**
 * cpumask_clear_cpu - clear a cpu in a cpumask
 * @cpu: cpu number (< nr_cpu_ids)
 * @dstp: the cpumask pointer
 */
static inline void cpumask_clear_cpu(int cpu, struct cpumask *dstp)
{
        clear_bit(cpumask_check(cpu), cpumask_bits(dstp));
}
{% endhighlight %}

cpumask_clear_cpu() 函数用于将 dstp 对应的 cpumask 结构中，
cpu 参数对应的位清零。函数调用 clear_bit() 函数清除 dstp 对应
bitmap 的 cpu 位。cpumask_check() 函数用于检查 cpu 参数对应
的 CPUID 合法性，cpumask_bits() 函数用于获得 dtsp 对应的
bitmap。

> - [clear_bit](https://biscuitos.github.io/blog/BITMAP_clear_bit/#%E6%BA%90%E7%A0%81%E5%88%86%E6%9E%90)
>
> - [cpumask_check](#A0022)
>
> - [cpumask_bits](#A0021)

------------------------------------

#### <span id="A0026">__cpu_online_mask</span>

{% highlight c %}
/* Don't assign or return these: may not be this big! */
typedef struct cpumask { DECLARE_BITMAP(bits, NR_CPUS); } cpumask_t;

struct cpumask __cpu_online_mask __read_mostly;
EXPORT_SYMBOL(__cpu_online_mask);
{% endhighlight %}

__cpu_online_mask 定义为一个 bitmap，其长度为 NR_CPUS 个 bit。
__cpu_online_mask 用于维护系统中 CPU offline/online 信息。
如果特定 bit 置位，那么对应的 cpu 就处于 online 状态；反之
如果特定 bit 清零，那么对应的 cpu 就处于 offline 状态。

------------------------------------

#### <span id="A0027">set_cpu_active</span>

{% highlight c %}
static inline void
set_cpu_active(unsigned int cpu, bool active)
{       
        if (active)
                cpumask_set_cpu(cpu, &__cpu_active_mask);
        else
                cpumask_clear_cpu(cpu, &__cpu_active_mask);
}
{% endhighlight %}

set_cpu_active() 函数用于设置特定 CPU 为 active 或
inactive 状态。参数 cpu 指向特定的 CPU 的 ID 号；active
参数指向了设置 cpu 为 active 或 inactive 状态。内核使用
__cpu_active_mask 位图维护着系统内 CPU 的 active 和 inactive
信息。在函数中，如果 active 为真，那么函数调用 cpumask_set_cpu()
将 cpu 参数对应的 CPU 设置为 active 状态；反之如果 active 为假，
那么函数调用 cpumask_set_cpu() 将 cpu 参数对应的 CPU 设
置为 inactive 状态。

> - [\_\_cpu_active_mask](#A0028)
>
> - [cpumask_set_cpu](#A0020)
>
> - [cpumask_clear_cpu](#A0025)

------------------------------------

#### <span id="A0028">__cpu_active_mask</span>

{% highlight c %}
/* Don't assign or return these: may not be this big! */
typedef struct cpumask { DECLARE_BITMAP(bits, NR_CPUS); } cpumask_t;

struct cpumask __cpu_active_mask __read_mostly;
EXPORT_SYMBOL(__cpu_active_mask);
{% endhighlight %}

__cpu_active_mask 用于维护系统中所有 CPU 的 active 和 inactive
信息，其实现是一个 bitmap，每个 bit 对应一个 CPU。位图中，如果
一个位置位，那么位对应的 CPU 就处于 active；反之如果一个位清零，
那么对应的 CPU 就处于 inactive。

------------------------------------

#### <span id="A0029">set_cpu_present</span>

{% highlight c %}
static inline void
set_cpu_present(unsigned int cpu, bool present)
{
        if (present)
                cpumask_set_cpu(cpu, &__cpu_present_mask);
        else
                cpumask_clear_cpu(cpu, &__cpu_present_mask);
}
{% endhighlight %}

set_cpu_present() 函数用于设置特定 CPU 为 present 或
不存在状态。参数 cpu 指向特定的 CPU 的 ID 号；present
参数指向了设置 cpu 为 存在或不存在状态。内核使用
__cpu_present_mask 位图维护着系统内 CPU 的 present 和 non-present
信息。在函数中，如果 present 为真，那么函数调用 cpumask_set_cpu()
将 cpu 参数对应的 CPU 设置为 present 状态；反之如果 present 为假，
那么函数调用 cpumask_set_cpu() 将 cpu 参数对应的 CPU 设
置为不存在状态。

> - [\_\_cpu_present_mask](#A0030)
>
> - [cpumask_set_cpu](#A0020)
>
> - [cpumask_clear_cpu](#A0025)

------------------------------------

#### <span id="A0030">__cpu_present_mask</span>

{% highlight c %}
/* Don't assign or return these: may not be this big! */
typedef struct cpumask { DECLARE_BITMAP(bits, NR_CPUS); } cpumask_t;

struct cpumask __cpu_present_mask __read_mostly;
EXPORT_SYMBOL(__cpu_present_mask);
{% endhighlight %}

__cpu_present_mask 用于维护系统中所有 CPU 的 present 状态信息，
__cpu_present_mask 其实现是一个位图，每个 cpu 对应一个 bit，如果
一个 bit 置位，那么对应的 CPU 表示存在；反之如果一个 bit 清零，那么
对应的 CPU 表示不存在。

------------------------------------

#### <span id="A0031">set_cpu_possible</span>

{% highlight c %}
static inline void
set_cpu_possible(unsigned int cpu, bool possible)
{
        if (possible)
                cpumask_set_cpu(cpu, &__cpu_possible_mask);
        else
                cpumask_clear_cpu(cpu, &__cpu_possible_mask);
}
{% endhighlight %}

set_cpu_possible() 函数用于设置特定 CPU 的 possible 信息。
内核使用 __cpu_possible_mask 位图维护着 CPU 的 possible 信息。
参数 cpu 对应要设置的 CPU 号；参数 possible 对应着设置或清除
possbile 位。在函数中，如果 possbile 为真，那么函数就调用
cpumask_set_cpu() 设置对应的 bit；反之 possible 为假，那么函数
就调用 cpumask_clear_cpu() 清零对应的 bit。

> - [\_\_cpu_possible_mask](#A0032)
>
> - [cpumask_set_cpu](#A0020)
>
> - [cpumask_clear_cpu](#A0025)

------------------------------------

#### <span id="A0032">__cpu_possible_mask</span>

{% highlight c %}
/* Don't assign or return these: may not be this big! */
typedef struct cpumask { DECLARE_BITMAP(bits, NR_CPUS); } cpumask_t;

#ifdef CONFIG_INIT_ALL_POSSIBLE
struct cpumask __cpu_possible_mask __read_mostly
        = {CPU_BITS_ALL};
#else
struct cpumask __cpu_possible_mask __read_mostly;
#endif
EXPORT_SYMBOL(__cpu_possible_mask);
{% endhighlight %}

__cpu_possible_mask 用于维护系统所有的 CPU 的 possible 信息。
__cpu_possible_mask 的实现是一个位图，每个 bit 对应一个 CPU。
如果一个 bit 置位，那么该位对应的 CPU possible；反之如果一个
bit 清零，那么该位对应的 CPU possible 无效。

------------------------------------

#### <span id="A0033">page_address_init</span>

{% highlight c %}
void __init page_address_init(void)
{
        int i;

        for (i = 0; i < ARRAY_SIZE(page_address_htable); i++) {
                INIT_LIST_HEAD(&page_address_htable[i].lh);
                spin_lock_init(&page_address_htable[i].lock);
        }
}
{% endhighlight %}

page_address_init() 函数用于初始化高端内存线性地址中永久映
射的全局变量。内核使用全局数组 page_address_htable 维护高端
内存页表池的链表，并带有一个自旋锁。函数使用 for 循环，对数组
中的每个成员进行链表头的初始化以及自旋锁的初始化。

> - [page_address_htable](#A0035)
>
> - [INIT_LIST_HEAD](https://biscuitos.github.io/blog/LIST_INIT_LIST_HEAD/#%E6%BA%90%E7%A0%81%E5%88%86%E6%9E%90)
>
> - [spin_lock_init](#)
>
> - [ARRAY_SIZE](#A0034)

------------------------------------

#### <span id="A0034">ARRAY_SIZE</span>

{% highlight c %}
/**
 * ARRAY_SIZE - get the number of elements in array @arr
 * @arr: array to be sized
 */
#define ARRAY_SIZE(arr) (sizeof(arr) / sizeof((arr)[0]) + __must_be_array(arr))
{% endhighlight %}

ARRAY_SIZE 宏用于计算数组中成员的个数。

------------------------------------

#### <span id="A0035">page_address_htable</span>

{% highlight c %}
/*
 * Hash table bucket
 */
static struct page_address_slot {
        struct list_head lh;                    /* List of page_address_maps */
        spinlock_t lock;                        /* Protect this bucket's list */
} ____cacheline_aligned_in_smp page_address_htable[1<<PA_HASH_ORDER];
{% endhighlight %}

page_address_htable 是一个全局变量，维护高端内存线性地址中永久
映射的全局变量。page_address_htable 数组的每个成员为
page_address_slot 结构，结构包含了一个双链表和一个自旋锁。

------------------------------------

#### <span id="A0036"></span>

{% highlight c %}
/* FIXED STRINGS! Don't touch! */
const char linux_banner[] =
        "Linux version " UTS_RELEASE " (" LINUX_COMPILE_BY "@"
        LINUX_COMPILE_HOST ") (" LINUX_COMPILER ") " UTS_VERSION "\n"
{% endhighlight %}

linux_banner 字符串保存了 linux 版本号，编译主机，GCC 版本，编译
时间等信息。

------------------------------------

#### <span id="A0037">setup_arch</span>

setup_arch() 函数用于初始化体系相关的部分，其实现与体系和芯片有关，
具体实现如下：

> - [ARMv7: Vexpress-a9](#A0038)

------------------------------------

#### <span id="A0038">setup_arch</span>

ARMv7 Vexpress-a9 芯片上，setup_arch() 函数的实现如下，
由于函数较长，分段解析：

{% highlight c %}
        const struct machine_desc *mdesc;

        setup_processor();
        mdesc = setup_machine_fdt(__atags_pointer);
        if (!mdesc)
                mdesc = setup_machine_tags(__atags_pointer, __machine_arch_type);               
        if (!mdesc) {
                early_print("\nError: invalid dtb and unrecognized/unsupported machine ID\n");
                early_print("  r1=0x%08x, r2=0x%08x\n", __machine_arch_type,
                            __atags_pointer);
                if (__atags_pointer)
                        early_print("  r2[]=%*ph\n", 16,
                                    phys_to_virt(__atags_pointer));
                dump_machine_table();
        }
{% endhighlight %}

函数首先定义了一个局部变量 mdesc，其为 machine_desc 数据结构，用于
维护体系相关的机器信息。函数首先调用 setup_processor()
从硬件获得体系相关的信息，并将该信息用于系统的初始化。
函数接着调用 setup_machine_fdt() 函数，__atags_pointer
指向 DTB 所在的物理地址，并从 DTB 中获得 CMDLINE，可用
物理内存的信息，并使用从 DTB 中获得的信息初始化系统，函数
执行完毕之后，会返回一个指向当前可用的 machine_desc 结构，
如果该结构不存在，函数调用 setup_machine_tags() 函数，
以此从 Uboot 传递的 ATAG 参数中获得 CMDLINE，物理内存
等信息，并用于系统的初始化，最后返回一个指向当前可用的
machine_desc 结构。如果此时还是不能获得 mdsec 结构，
那么系统就打印相应的错误信息，并报错，调用 dump_machine_table()
函数打印出指定的信息。

{% highlight c %}
        machine_desc = mdesc;
        machine_name = mdesc->name;
        dump_stack_set_arch_desc("%s", mdesc->name);

        if (mdesc->reboot_mode != REBOOT_HARD)
                reboot_mode = mdesc->reboot_mode;
{% endhighlight %}

函数将获得的 mdesc 指针赋值给 machine_desc 全局变量，并
把 mdesc->name 赋值为 machine_name 全局变量，函数继续调用
dump_stack_set_arch_desc() 函数将 mdesc->name 字符串拷贝
到 dump_stack_arch_desc_str 全局变量里。

> - [setup_processor](#A0074)


------------------------------------

#### <span id="A0039">lookup_processor</span>

{% highlight c %}
/*
 * locate processor in the list of supported processor types.  The linker
 * builds this table for us from the entries in arch/arm/mm/proc-*.S
 */
struct proc_info_list *lookup_processor(u32 midr)
{
        struct proc_info_list *list = lookup_processor_type(midr);

        if (!list) {
                pr_err("CPU%u: configuration botched (ID %08x), CPU halted\n",
                       smp_processor_id(), midr);
                while (1)
                /* can't use cpu_relax() here as it may require MMU setup */;
        }

        return list;
}
{% endhighlight %}

lookup_processor() 函数用于获得体系芯片相关的 proc_info_list 结构。
参数 midr 指向了 CPU ID 信息，函数通过调用 lookup_processor_type()
函数获得对应的 proc_info_list 信息，proc_info_list 信息包含了很多
系统对内存管理以及内存基础信息。如果获取失败，函数将报错并挂起系统。

> - [lookup_processor_type](#A0040)

------------------------------------

#### <span id="A0040">lookup_processor_type</span>

{% highlight c %}
/*
 * This provides a C-API version of __lookup_processor_type
 */
ENTRY(lookup_processor_type)
        stmfd   sp!, {r4 - r6, r9, lr}
        mov     r9, r0
        bl      __lookup_processor_type
        mov     r0, r5
        ldmfd   sp!, {r4 - r6, r9, pc}
ENDPROC(lookup_processor_type)
{% endhighlight %}

lookup_processor_type() 函数用于查找体系芯片相关的
proc_info_list 结构。函数以汇编形式给出，参数通过 r0 寄存器
传入，并通过调用 __lookup_processor_type 获得需求的内容，
最后通过 r0 寄存器返回 proc_info_list 的地址。
__lookup_processor_type 获取 proc_info_list 的过程可以参考
如下文档：

> - [ARMv7 Cortex-A9 proc_info_list](https://biscuitos.github.io/blog/ARM-SCD-kernel-head.S/#ARMv7%20Cortex-A9%20proc_info_list)
>
> - [\_\_lookup_processor_type](https://biscuitos.github.io/blog/ARM-SCD-kernel-head.S/#__lookup_processor_type)

------------------------------------

#### <span id="A0041">read_cpuid_ext</span>

{% highlight c %}
static int __get_cpu_architecture(void)
{
        int cpu_arch;

        if ((read_cpuid_id() & 0x0008f000) == 0) {
                cpu_arch = CPU_ARCH_UNKNOWN;
        } else if ((read_cpuid_id() & 0x0008f000) == 0x00007000) {
                cpu_arch = (read_cpuid_id() & (1 << 23)) ? CPU_ARCH_ARMv4T : CPU_ARCH_ARMv3;
        } else if ((read_cpuid_id() & 0x00080000) == 0x00000000) {
                cpu_arch = (read_cpuid_id() >> 16) & 7;
                if (cpu_arch)
                        cpu_arch += CPU_ARCH_ARMv3;
        } else if ((read_cpuid_id() & 0x000f0000) == 0x000f0000) {
                /* Revised CPUID format. Read the Memory Model Feature
                 * Register 0 and check for VMSAv7 or PMSAv7 */
                unsigned int mmfr0 = read_cpuid_ext(CPUID_EXT_MMFR0);
                if ((mmfr0 & 0x0000000f) >= 0x00000003 ||
                    (mmfr0 & 0x000000f0) >= 0x00000030)
                        cpu_arch = CPU_ARCH_ARMv7;
                else if ((mmfr0 & 0x0000000f) == 0x00000002 ||
                         (mmfr0 & 0x000000f0) == 0x00000020)
                        cpu_arch = CPU_ARCH_ARMv6;
                else
                        cpu_arch = CPU_ARCH_UNKNOWN;
        } else
                cpu_arch = CPU_ARCH_UNKNOWN;

        return cpu_arch;
}
{% endhighlight %}

__get_cpu_architecture() 函数用于获得当前 ARM 的体系信息。在
分析源码之前，首先了解 ARM 的 ID_MMFR0 和 MIDR 寄存器。ARMv7 中
MDIR 寄存器的布局如下图：

![](https://raw.githubusercontent.com/EmulateSpace/PictureSet/master/BiscuitOS/boot/BOOT000025.png)

从 MDIR 寄存器的 Architecture 域存储着体系相关的识别码，如下图：

![](https://raw.githubusercontent.com/EmulateSpace/PictureSet/master/BiscuitOS/boot/BOOT000196
.png)

read_cpuid_id() 函数用于读取 ARM 的 MIDR 寄存器，然后
根据上面的 Architecture 域，判断当前系统 ARM 的体系版本号。
将 read_cpuid_id() 函数读取的值与 0x0008f000 相与，然后
通过相与的值可以获得真实的版本号。如果 ARMv7 ，那么 MDIR 寄
存器的 16:19 域为 0xf。当 ARM 的系统结构是 ARMv7 的时候，
__get_cpu_architecture() 函数继续调用 read_cpuid_ext()
函数判断具体 ARMv7 的版本信息。ID_MMFR0 寄存器的内存布局如下：

![](https://raw.githubusercontent.com/EmulateSpace/PictureSet/master/BiscuitOS/boot/BOOT000007
.png)

ID_MMFR0 寄存器中存储了体系内存模组相关的信息，如上图，
VMSA 域和 PMSA 域就明确指明目前体系对两者的支持情况。在
__get_cpu_architecture() 函数中，如果 VMSA 域值大于等于 3，
或者 PMSA 域大于 3，那么体系就是 ARMv7；如果 VMSA 域或者
PMSA 域等于 2，那么体系就是 ARMv6。因此 __get_cpu_architecture()
函数可以获得 ARM 的体系信息。

> - [read_cpuid_id](https://biscuitos.github.io/blog/CPUID_read_cpuid_id/)

------------------------------------

#### <span id="A0042">init_proc_vtable</span>

{% highlight c %}
static inline void init_proc_vtable(const struct processor *p)
{
        processor = *p;
}
{% endhighlight %}

init_proc_vtable() 函数用于初始化用于描述系统信息的 struct
processor 结构，全局变量 processor 用于描述系统的基础信息，与
系统有关。在 arm 中定于在 arch/arm/include/asm/proc-fns.h

------------------------------------

#### <span id="A0043">get_cr</span>

{% highlight c %}
static inline unsigned long get_cr(void)
{
        unsigned long val;
        asm("mrc p15, 0, %0, c1, c0, 0  @ get CR" : "=r" (val) : : "cc");
        return val;
}
{% endhighlight %}

get_cr() 函数用于获得 ARM 的 SCTLR (System Control Register)
系统控制寄存器。其内存布局如下：

![](https://raw.githubusercontent.com/EmulateSpace/PictureSet/master/BiscuitOS/boot/BOOT000014
.png)

------------------------------------

#### <span id="A0044">init_utsname</span>

{% highlight c %}
static inline struct new_utsname *init_utsname(void)
{
        return &init_uts_ns.name;
}
{% endhighlight %}

init_utsname() 函数用于初始化全局变量 init_uts_ns 的 name 成员，
init_uts_ns 属于 struct uts_namespace，其结构用于维护系统名称，
版本号，机器名称等。

------------------------------------

#### <span id="A0045">cpu_architecture</span>

{% highlight c %}
int __pure cpu_architecture(void)
{               
        BUG_ON(__cpu_architecture == CPU_ARCH_UNKNOWN);

        return __cpu_architecture;
}
{% endhighlight %}

cpu_architecture() 函数用于获得当前系统的 ARM 体系信息。
内核中，将体系相关的信息维护在 __cpu_architecture 里。
如果 __cpu_architecture 的值等于 CPU_ARCH_UNKNOWN，
那么是非法的，系统将会报错。

------------------------------------

#### <span id="A0046">__cpu_architecture</span>

__cpu_architecture 全局变量维护了系统体系相关的信息。

------------------------------------

#### <span id="A0047">cpuid_feature_extract_field</span>

{% highlight c %}
static inline int __attribute_const__ cpuid_feature_extract_field(u32 features,
                                                                  int field)
{
        int feature = (features >> field) & 15;

        /* feature registers are signed values */
        if (feature > 7)
                feature -= 16;

        return feature;
}  
{% endhighlight %}

cpuid_feature_extract_field() 函数用于提取 CPUID 系列寄存器
中特定的域。在 CPUID 系列寄存器中，每个域都是 4 bits，例如 MIDR
寄存器布局如下：

![](https://raw.githubusercontent.com/EmulateSpace/PictureSet/master/BiscuitOS/boot/BOOT000025.png)

函数首先将寄存器的值向右移动 field 位，然后与 0xF 相与。
如果对于的寄存器对应域的值带符号，如果此时 feature 大于 7，
那么此时值是负数，需要获得其真实的负值，将其将去 16 就可以
获得。最后返回 feature 的值。

------------------------------------

#### <span id="A0048">cpuid_feature_extract</span>

{% highlight c %}
#define cpuid_feature_extract(reg, field) \
        cpuid_feature_extract_field(read_cpuid_ext(reg), field)
{% endhighlight %}

cpuid_feature_extract() 函数用于提取 CPUID 系列寄存器
中指定的域的值。函数通过调用 cpuid_feature_extract_field()
函数实现。reg 参数指向寄存器的藐视；field 参数指向域的偏移。
函数首先调用 read_cpuid_ext() 函数获得 cp15 c0 系列寄存器值，
然后通过 cpuid_feature_extract_field() 获得 field 对应的值。

> - [cpuid_feature_extract_field](#A0047)

------------------------------------

#### <span id="A0049">cpuid_init_hwcaps</span>

{% highlight c %}
static void __init cpuid_init_hwcaps(void)
{
        int block;
        u32 isar5;

        if (cpu_architecture() < CPU_ARCH_ARMv7)
                return;

        block = cpuid_feature_extract(CPUID_EXT_ISAR0, 24);
        if (block >= 2)
                elf_hwcap |= HWCAP_IDIVA;
        if (block >= 1)
                elf_hwcap |= HWCAP_IDIVT;

        /* LPAE implies atomic ldrd/strd instructions */
        block = cpuid_feature_extract(CPUID_EXT_MMFR0, 0);
        if (block >= 5)
                elf_hwcap |= HWCAP_LPAE;

        /* check for supported v8 Crypto instructions */
        isar5 = read_cpuid_ext(CPUID_EXT_ISAR5);

        block = cpuid_feature_extract_field(isar5, 4);
        if (block >= 2)
                elf_hwcap2 |= HWCAP2_PMULL;
        if (block >= 1)
                elf_hwcap2 |= HWCAP2_AES;

        block = cpuid_feature_extract_field(isar5, 8);
        if (block >= 1)
                elf_hwcap2 |= HWCAP2_SHA1;

        block = cpuid_feature_extract_field(isar5, 12);
        if (block >= 1)
                elf_hwcap2 |= HWCAP2_SHA2;

        block = cpuid_feature_extract_field(isar5, 16);
        if (block >= 1)
                elf_hwcap2 |= HWCAP2_CRC32;
}
{% endhighlight %}

cpuid_init_hwcaps() 函数用于获得系统指定的硬件支持信息。函数
首先调用 cpu_architecture() 函数判断当前体系结构是否为 ARMv7
以上，如果不是，那么直接返回。接着函数调用 cpuid_feature_extract()
函数获得 ID_ISAR0 (Instruction Set Attribute Register) 寄存器
的 24:27，ID_ISAR0 寄存器布局如下：

![](https://raw.githubusercontent.com/EmulateSpace/PictureSet/master/BiscuitOS/boot/BOOT000197.png)

在 ID_ISAR0 寄存器中，bit 24:27 域说明如下：

![](https://raw.githubusercontent.com/EmulateSpace/PictureSet/master/BiscuitOS/boot/BOOT000198.png)

从上的定义可以知道，在 cpuid_init_hwcaps() 函数中，如果 24:27
域的值大于等于 2，那么支持 ARM 指令集的 SDIV 和 UDIV 指令，并将
这个结果通过宏 HWCAP_IDIVA 存储在全局变量 elf_hwcap 里；如果
24:27 域的值大于 1，那么系统支持 ARM 和 Thumb 指令集的 SDIV
和 UDIV 指令。接着函数继续调用 cpuid_feature_extract() 函数
读取 ID_MMFR0 (Memory Model Feature Register) 寄存器，
ID_MMFR0 寄存器用于描述系统内存模组的信息。其寄存器布局如下：

![](https://raw.githubusercontent.com/EmulateSpace/PictureSet/master/BiscuitOS/boot/BOOT000007.png)

cpuid_init_hwcaps() 函数读取了 ID_MMFR0 寄存器的低 4 bits，
ID_MMFR0 寄存器的低 4bit 域是 VMSA support 域，其定义如下：

![](https://raw.githubusercontent.com/EmulateSpace/PictureSet/master/BiscuitOS/boot/BOOT000199.png)

cpuid_init_hwcaps() 函数判断该域值是否大于 5，如果大于 5，
那么系统内存模组就支持 Long-descriptor 页表，那么系统就支持
LPAE，这样就原生支持 LPAE 模式下的 ldrd/strd 指令，此时如果
支持，函数通过 HWCAP_LPAE 宏存储在 elf_hwcap 全局变量里。
函数继续调用 read_cpuid_ext(CPUID_EXT_ISAR5) 函数获得
ID_ISAR5 寄存器，其在 ARMv7 中的内存布局如下：

![](https://raw.githubusercontent.com/EmulateSpace/PictureSet/master/BiscuitOS/boot/BOOT000200.png)

cpuid_init_hwcaps() 函数通过读取 ID_ISAR5 寄存器的值，以此
确定系统是否支持 V8 Crypto 指令。根据各个域值设置 elf_hwcap2
全局变量，通过宏 HWCAP2_PMULL，HWCAP2_AES，HWCAP2_SHA1，
HWCAP2_SHA2，HWCAP2_CRC32。

> - [cpu_architecture](#A0045)
>
> - [cpuid_feature_extract](#A0048)

------------------------------------

#### <span id="A0050">cache_policies</span>

{% highlight c %}
static struct cachepolicy cache_policies[] __initdata = {
        {
                .policy         = "uncached",
                .cr_mask        = CR_W|CR_C,
                .pmd            = PMD_SECT_UNCACHED,
                .pte            = L_PTE_MT_UNCACHED,
                .pte_s2         = s2_policy(L_PTE_S2_MT_UNCACHED),
        }, {
                .policy         = "buffered",
                .cr_mask        = CR_C,
                .pmd            = PMD_SECT_BUFFERED,
                .pte            = L_PTE_MT_BUFFERABLE,
                .pte_s2         = s2_policy(L_PTE_S2_MT_UNCACHED),
        }, {
                .policy         = "writethrough",
                .cr_mask        = 0,
                .pmd            = PMD_SECT_WT,
                .pte            = L_PTE_MT_WRITETHROUGH,
                .pte_s2         = s2_policy(L_PTE_S2_MT_WRITETHROUGH),
        }, {
                .policy         = "writeback",
                .cr_mask        = 0,
                .pmd            = PMD_SECT_WB,
                .pte            = L_PTE_MT_WRITEBACK,
                .pte_s2         = s2_policy(L_PTE_S2_MT_WRITEBACK),
        }, {
                .policy         = "writealloc",
                .cr_mask        = 0,
                .pmd            = PMD_SECT_WBWA,
                .pte            = L_PTE_MT_WRITEALLOC,
                .pte_s2         = s2_policy(L_PTE_S2_MT_WRITEBACK),
        }
};
{% endhighlight %}

cache_policies 数组是由 struct cachepolicy 数据构成的，
用于描述系统 CACHE 的策略信息。

------------------------------------

#### <span id="A0051">initial_pmd_value</span>

{% highlight c %}
static unsigned long initial_pmd_value __initdata = 0
{% endhighlight %}


------------------------------------

#### <span id="A0052">cachepolicy</span>

{% highlight c %}
static unsigned int cachepolicy __initdata = CPOLICY_WRITEBACK;
{% endhighlight %}

cachepolicy 全局变量用于指定当前系统所使用的 cache policy
在 cache_policies[] 数组中的索引。

------------------------------------

#### <span id="A0053">init_default_cache_policy</span>

{% highlight c %}
/*
 * Initialise the cache_policy variable with the initial state specified
 * via the "pmd" value.  This is used to ensure that on ARMv6 and later,
 * the C code sets the page tables up with the same policy as the head
 * assembly code, which avoids an illegal state where the TLBs can get
 * confused.  See comments in early_cachepolicy() for more information.
 */
void __init init_default_cache_policy(unsigned long pmd)
{
        int i;

        initial_pmd_value = pmd;

        pmd &= PMD_SECT_CACHE_MASK;

        for (i = 0; i < ARRAY_SIZE(cache_policies); i++)
                if (cache_policies[i].pmd == pmd) {
                        cachepolicy = i;
                        break;
                }

        if (i == ARRAY_SIZE(cache_policies))
                pr_err("ERROR: could not find cache policy\n");
}
{% endhighlight %}

init_default_cache_policy() 函数用于初始化系统 cachepolicy
信息。内核通过 cache_policies[] 数组提供了多种 cache 策略，
init_default_cache_policy() 函数通过 pmd 参数在数组中找到
指定的 cache 策略，并将当前使用的 cache 策略在 cache_policies[]
数组中的索引值存储在全局变量 cachepolicy 里。该函数又将 pmd
参数存储在全局变量 initial_pmd_value 里。如果通过 pmd 参数
传入的值不能在 cache_policies[] 数组里找到指定的值，那么系统
将提示错误信息。

> - [initial_pmd_value](#A0051)
>
> - [cache_policies](#A0050)
>
> - [cachepolicy](#A0052)
>
> - [ARRAY_SIZE](#A0034)

------------------------------------

#### <span id="A0054">read_cpuid_part</span>

{% highlight c %}
/*
 * The CPU part number is meaningless without referring to the CPU
 * implementer: implementers are free to define their own part numbers
 * which are permitted to clash with other implementer part numbers.
 */
static inline unsigned int __attribute_const__ read_cpuid_part(void)
{       
        return read_cpuid_id() & ARM_CPU_PART_MASK;
}
{% endhighlight %}

read_cpuid_part() 函数用于获得 MIDR 寄存器中 Implementer 域和
Primary part number 域。MIDR 寄存器的布局如下：

![](https://raw.githubusercontent.com/EmulateSpace/PictureSet/master/BiscuitOS/boot/BOOT000201.png)

> - [read_cpuid_id](https://biscuitos.github.io/blog/CPUID_read_cpuid_id/)

------------------------------------

#### <span id="A0055">elf_hwcap_fixup</span>

{% highlight c %}
static void __init elf_hwcap_fixup(void)
{       
        unsigned id = read_cpuid_id();

        /*
         * HWCAP_TLS is available only on 1136 r1p0 and later,
         * see also kuser_get_tls_init.
         */
        if (read_cpuid_part() == ARM_CPU_PART_ARM1136 &&
            ((id >> 20) & 3) == 0) {
                elf_hwcap &= ~HWCAP_TLS;
                return;
        }

        /* Verify if CPUID scheme is implemented */
        if ((id & 0x000f0000) != 0x000f0000)
                return;

        /*
         * If the CPU supports LDREX/STREX and LDREXB/STREXB,
         * avoid advertising SWP; it may not be atomic with
         * multiprocessing cores.
         */  
        if (cpuid_feature_extract(CPUID_EXT_ISAR3, 12) > 1 ||
            (cpuid_feature_extract(CPUID_EXT_ISAR3, 12) == 1 &&
             cpuid_feature_extract(CPUID_EXT_ISAR4, 20) >= 3))
                elf_hwcap &= ~HWCAP_SWP;
}
{% endhighlight %}

elf_hwcap_fixup() 用于修正 ARM 硬件支持的信息。函数首先调用
read_cpuid_id() 函数获得 MIDR 寄存器的值，其寄存器布局如下：

![](https://raw.githubusercontent.com/EmulateSpace/PictureSet/master/BiscuitOS/boot/BOOT000201.png)

然后调用 read_cpuid_part() 函数判断当前 ARM 是否是
ARM_CPU_PART_ARM1136, 并且判断 MIDR 寄存器的 Variant 域
是否为 0，该域值与 “An IMPLEMENTATION DEFINED variant number”
有关，如果上面的条件满足，那么设置 elf_hwcap 全局变量与
~HWCAP_TLS 相与，并直接返回。如果函数此时没有返回，那么继续
执行下面的代码，接着判断当前系统的 MIDR 寄存器的 Variant 域，
如果当前体系没有实现这个域，那么函数直接返回；反之如果 MDIR
实现了 Variant 域，那么函数调用 cpuid_feature_extract() 函数
读取体系的 ID_ISAR3 寄存器，ID_ISAR4 寄存器，其寄存器布局如下：

![](https://raw.githubusercontent.com/EmulateSpace/PictureSet/master/BiscuitOS/boot/BOOT000202.png)

![](https://raw.githubusercontent.com/EmulateSpace/PictureSet/master/BiscuitOS/boot/BOOT000203.png)

elf_hwcap_fixup() 函数判断，如果 ID_ISAR3 的 bit 12:15 域值，
该域值的含义如下：

![](https://raw.githubusercontent.com/EmulateSpace/PictureSet/master/BiscuitOS/boot/BOOT000204.png)

该域值与 ID_ISAR4 寄存器的 SynchPrim_instrs_frac 域一同
判断体系是否实现了 Synchronization Primitive 指令，其中
ID_ISAR4 寄存器的 SynchPrim_instrs_frac 域定义如下：

![](https://raw.githubusercontent.com/EmulateSpace/PictureSet/master/BiscuitOS/boot/BOOT000205.png)

elf_hwcap_fixup() 函数此时判断，如果 ID_ISAR3 寄存器的
SynchPrim_instrs 域值大于 1，或者 ID_ISAR3 寄存器的
SynchPrim_instrs 域值等于 1，且 ID_ISAR4 寄存器的
SynchPrim_instrs_frac 域大于等于 3，那么函数将全局变量 elf_hwcap
的值与宏 ~HWCAP_SWP 相与。

> - [cpuid_feature_extract](#A0048)
>
> - [read_cpuid_id](https://biscuitos.github.io/blog/CPUID_read_cpuid_id/)

------------------------------------

#### <span id="A0056">read_cpuid_cachetype</span>

{% highlight c %}
static inline unsigned int __attribute_const__ read_cpuid_cachetype(void)
{
        return read_cpuid(CPUID_CACHETYPE);
}
{% endhighlight %}

read_cpuid_cachetype() 函数用于读取 ARM 的 CTR (Cache Type Register)
寄存器，以此读取体系 CACHE 的类型信息。CTR 寄存器的内存布局如下：

![](https://raw.githubusercontent.com/EmulateSpace/PictureSet/master/BiscuitOS/boot/BOOT000206.png)

> - [read_cpuid](https://biscuitos.github.io/blog/CPUID_read_cpuid/)

------------------------------------

#### <span id="A0057">cacheid_is</span>

{% highlight c %}
#define CACHEID_VIVT                    (1 << 0)
#define CACHEID_VIPT_NONALIASING        (1 << 1)
#define CACHEID_VIPT_ALIASING           (1 << 2)
#define CACHEID_VIPT                    (CACHEID_VIPT_ALIASING|CACHEID_VIPT_NONALIASING)
#define CACHEID_ASID_TAGGED             (1 << 3)
#define CACHEID_VIPT_I_ALIASING         (1 << 4)
#define CACHEID_PIPT

/*              
 * __LINUX_ARM_ARCH__ is the minimum supported CPU architecture
 * Mask out support which will never be present on newer CPUs.
 * - v6+ is never VIVT
 * - v7+ VIPT never aliases on D-side
 */
#if __LINUX_ARM_ARCH__ >= 7
#define __CACHEID_ARCH_MIN      (CACHEID_VIPT_NONALIASING |\
                                 CACHEID_ASID_TAGGED |\
                                 CACHEID_VIPT_I_ALIASING |\
                                 CACHEID_PIPT)
#elif __LINUX_ARM_ARCH__ >= 6
#define __CACHEID_ARCH_MIN      (~CACHEID_VIVT)
#else
#define __CACHEID_ARCH_MIN      (~0)
#endif

/*              
 * Mask out support which isn't configured
 */
#if defined(CONFIG_CPU_CACHE_VIVT) && !defined(CONFIG_CPU_CACHE_VIPT)
#define __CACHEID_ALWAYS        (CACHEID_VIVT)
#define __CACHEID_NEVER         (~CACHEID_VIVT)
#elif !defined(CONFIG_CPU_CACHE_VIVT) && defined(CONFIG_CPU_CACHE_VIPT)
#define __CACHEID_ALWAYS        (0)
#define __CACHEID_NEVER         (CACHEID_VIVT)
#else
#define __CACHEID_ALWAYS        (0)
#define __CACHEID_NEVER         (0)
#endif                  

static inline unsigned int __attribute__((pure)) cacheid_is(unsigned int mask)
{                       
        return (__CACHEID_ALWAYS & mask) |
               (~__CACHEID_NEVER & __CACHEID_ARCH_MIN & mask & cacheid);
}
{% endhighlight %}

cacheid_is() 函数用于获得 ARM 系统 CTR (Cache Type Register)
寄存器中的特定域值。宏 __CACHEID_ALWAYS 的值与宏 CONFIG_CPU_CACHE_VIVT
和宏 CONFIG_CPU_CACHE_VIPT 有关，定义如上。宏 __CACHEID_ARCH_MIN 的定义
由 CACHEID_VIPT_NONALIASING，CACHEID_ASID_TAGGED，CACHEID_VIPT_I_ALIASING，
CACHEID_PIPT 定义有关。cacheid_is() 函数首先将 mask 参数
与 __CACHEID_ALWAYS 相与，然后 __CACHEID_NEVER 的反码与  
__CACHEID_ARCH_MIN, mask 参数，cacheid 相与，相与的结果与
之前的值相或，以此获得 CTR 寄存器中特定的值。

------------------------------------

#### <span id="A0058">icache_is_pipt</span>

{% highlight c %}
#define icache_is_pipt()                cacheid_is(CACHEID_PIPT)
{% endhighlight %}

icache_is_pipt() 函数判断 L1 指令 CACHE 是否是 PIPT
(Physical index, Physical tag).

------------------------------------

#### <span id="A0059">set_csselr</span>

{% highlight c %}
static inline void set_csselr(unsigned int cache_selector)
{
        asm volatile("mcr p15, 2, %0, c0, c0, 0" : : "r" (cache_selector));
}
{% endhighlight %}

set_csselr() 函数用于设置 ARM 的 CSSELR (Cache Size Selection Register)
寄存器，该寄存器用来设置 cache 的时候选择对应的 CACHE 类型以及 CACHE level 号，
例如 Dcache 或者 ICACHE，或者 Level1 cache，Level2 cache 等。
CSSELR 寄存器的寄存器布局如下：

![](https://raw.githubusercontent.com/EmulateSpace/PictureSet/master/BiscuitOS/boot/BOOT000026.png)

------------------------------------

#### <span id="A0060">read_ccsidr</span>

{% highlight c %}
static inline unsigned int read_ccsidr(void)
{
        unsigned int val;

        asm volatile("mrc p15, 1, %0, c0, c0, 0" : "=r" (val));
        return val;
}
{% endhighlight %}

read_ccsidr() 函数的作用就是读取 ARM 体系的 CCSIDR
(Cache Size ID Registers) 寄存器。CCSIDR 寄存器存储了 CSSELR
寄存器选中 cache 的 Cache Sets，LineSize 信息。
CCSIDR 寄存器的寄存器布局如下：

![](https://raw.githubusercontent.com/EmulateSpace/PictureSet/master/BiscuitOS/boot/BOOT000028.png)

------------------------------------

#### <span id="A0061">cpu_has_aliasing_icache</span>

{% highlight c %}
static int cpu_has_aliasing_icache(unsigned int arch)
{
        int aliasing_icache;
        unsigned int id_reg, num_sets, line_size;

        /* PIPT caches never alias. */
        if (icache_is_pipt())
                return 0;

        /* arch specifies the register format */
        switch (arch) {
        case CPU_ARCH_ARMv7:
                set_csselr(CSSELR_ICACHE | CSSELR_L1);
                isb();
                id_reg = read_ccsidr();
                line_size = 4 << ((id_reg & 0x7) + 2);
                num_sets = ((id_reg >> 13) & 0x7fff) + 1;
                aliasing_icache = (line_size * num_sets) > PAGE_SIZE;
                break;
        case CPU_ARCH_ARMv6:
                aliasing_icache = read_cpuid_cachetype() & (1 << 11);
                break;
        default:
                /* I-cache aliases will be handled by D-cache aliasing code */
                aliasing_icache = 0;
        }

        return aliasing_icache;
}
{% endhighlight %}

cpu_has_aliasing_icache() 函数用于判断 ICACHE 是否按页对齐。
函数首先调用 icache_is_pipt() 函数判断 ICACHE 是否属于 PIPT
类型，如果是，那么 PIPT cache 是不需要对齐的，直接返回 0；如果
CACHE 不是 PIPT 类型，那么函数通过 arch 参数进行不同的处理，
arch 如果是 ARMv7 的时候，函数首先调用 set_csselr() 设置
CSSELR 寄存器，传入 CSSELR_ICACHE 和 CSSELR_L1，以此选中
Level1 的 ICACHE。然后调用 isb() 函数执行一次内存屏障，
接着调用 read_ccsidr() 函数读取 L1 ICACHE 的 cache sets
和 cache line 信息，以此计算 L1 ICACHE 的体积是否小于一个
PAGE_SIZE，如果大于，那么表示 L1 ICACHE 没有对齐；反之
表示 L1 ICACHE 已经按页对齐。在 ARMv7 中 CSSELR 寄存器
的布局如下图：

![](https://raw.githubusercontent.com/EmulateSpace/PictureSet/master/BiscuitOS/boot/BOOT000026.png)

ARMv7 中 CCSIDR 寄存器的寄存器布局如下：

![](https://raw.githubusercontent.com/EmulateSpace/PictureSet/master/BiscuitOS/boot/BOOT000028.png)

ARMv6 的情况不讨论，最后返回比较的结果。

> - [set_csselr](#A0059)
>
> - [read_ccsidr](#A0060)

------------------------------------

#### <span id="A0062">cache_is_vivt</span>

{% highlight c %}
#define cache_is_vivt()                 cacheid_is(CACHEID_VIVT)
{% endhighlight %}

cache_is_vivt() 判断 CACHE 是否是 VIVT 类型，
Virtual index，Virtual Tag。

------------------------------------

#### <span id="A0063">cache_is_vipt</span>

{% highlight c %}
#define cache_is_vipt()                 cacheid_is(CACHEID_VIPT)
{% endhighlight %}

cache_is_vipt() 函数判断 CACHE 是否属于 VIPT 类型。
Virtual Index，Physical Tag。

------------------------------------

#### <span id="A0064">cache_is_vipt_nonaliasing</span>

{% highlight c %}
#define cache_is_vipt_nonaliasing()     cacheid_is(CACHEID_VIPT_NONALIASING)
{% endhighlight %}

cache_is_vipt_nonaliasing() 函数用于判断 CACHE 是否属于
VIPT 非别名的。Virtual Index，Physical Tag。

------------------------------------

#### <span id="A0065">cache_is_vipt_aliasing</span>

{% highlight c %}
#define cache_is_vipt_aliasing()        cacheid_is(CACHEID_VIPT_ALIASING)
{% endhighlight %}

cache_is_vipt_aliasing() 函数判断 CACHE 是否属于
VIPT aliasing，Virtual Index，Physical Tag。

------------------------------------

#### <span id="A0066">icache_is_vivt_asid_tagged</span>

{% highlight c %}
#define icache_is_vivt_asid_tagged()    cacheid_is(CACHEID_ASID_TAGGED)
{% endhighlight %}

icache_is_vivt_asid_tagged() 函数判断函数 ICACHE 是否属于
VIVT ASID tags.

------------------------------------

#### <span id="A0067">icache_is_vipt_aliasing</span>

{% highlight c %}
#define icache_is_vipt_aliasing()       cacheid_is(CACHEID_VIPT_I_ALIASING)
{% endhighlight %}

icache_is_vipt_aliasing() 函数用于判断 ICACHE 是否属于
VIPT aliasing。

------------------------------------

#### <span id="A0068">cacheid_init</span>

{% highlight c %}
static void __init cacheid_init(void)
{
        unsigned int arch = cpu_architecture();

        if (arch >= CPU_ARCH_ARMv6) {
                unsigned int cachetype = read_cpuid_cachetype();

                if ((arch == CPU_ARCH_ARMv7M) && !(cachetype & 0xf000f)) {
                        cacheid = 0;
                } else if ((cachetype & (7 << 29)) == 4 << 29) {
                        /* ARMv7 register format */
                        arch = CPU_ARCH_ARMv7;
                        cacheid = CACHEID_VIPT_NONALIASING;
                        switch (cachetype & (3 << 14)) {
                        case (1 << 14):
                                cacheid |= CACHEID_ASID_TAGGED;
                                break;
                        case (3 << 14):
                                cacheid |= CACHEID_PIPT;
                                break;
                        }
                } else {
                        arch = CPU_ARCH_ARMv6;
                        if (cachetype & (1 << 23))
                                cacheid = CACHEID_VIPT_ALIASING;
                        else    
                                cacheid = CACHEID_VIPT_NONALIASING;
                }
                if (cpu_has_aliasing_icache(arch))
                        cacheid |= CACHEID_VIPT_I_ALIASING;
        } else {
                cacheid = CACHEID_VIVT;
        }

        pr_info("CPU: %s data cache, %s instruction cache\n",
                cache_is_vivt() ? "VIVT" :
                cache_is_vipt_aliasing() ? "VIPT aliasing" :
                cache_is_vipt_nonaliasing() ? "PIPT / VIPT nonaliasing" : "unknown",            
                cache_is_vivt() ? "VIVT" :
                icache_is_vivt_asid_tagged() ? "VIVT ASID tagged" :
                icache_is_vipt_aliasing() ? "VIPT aliasing" :
                icache_is_pipt() ? "PIPT" :
                cache_is_vipt_nonaliasing() ? "VIPT nonaliasing" : "unknown");
}
{% endhighlight %}

cacheid_init() 函数用于初始化系统 CACHE 信息。内核使用
全局变量 cacheid 维护 cache 相关的信息。函数首先调用
read_cpuid_cachetype() 函数读取 ARM 体系的  CTR (Cache Type Register)
寄存器，以此读取体系 CACHE 的类型信息。CTR 寄存器的内存布局如下：

![](https://raw.githubusercontent.com/EmulateSpace/PictureSet/master/BiscuitOS/boot/BOOT000206.png)

如果 arch 参数表明当前体系是 ARMv7M 关于这个 cache type 的
0:3 域与 16:19 域为 0，这两个域分别是 IminLine 和 DminLine 域，
用于指定最小的 Line number 数。此时设置 cacheid 为 0；如果此时
cachetype 的 bit 29:31 等于 4，此时对应 CTR 寄存器的 Format 域，
其定义如下：

![](https://raw.githubusercontent.com/EmulateSpace/PictureSet/master/BiscuitOS/boot/BOOT000207.png)

如果 Format 等于 4，那么此时体系信息是 ARMv7. 此时将
cacheid 设置为 CACHEID_VIPT_NONALIASING。接着如果此时
CTR 的 bit 14:16 L1lp 域，即 Level 1 cache policy 域，
其定义为：

![](https://raw.githubusercontent.com/EmulateSpace/PictureSet/master/BiscuitOS/boot/BOOT000208.png)

如果此时域值为 1，即 AIVIVT，那么将宏 CACHEID_ASID_TAGGED
添加到 cacheid 中；如果此时域值为 3，即 PIPT，那么将宏
CACHEID_PIPT 添加到 cacheid 中；如果此时是 ARMv6，这里不做
讨论。函数接着调用 cpu_has_aliasing_icache() 函数判断当前
使用的 ICACHE 是否按页对齐，如果按页对齐，那么将宏
CACHEID_VIPT_I_ALIASING 添加到 cacheid 里面。
如果 ARM 体系的版本小于 ARMv6, 那么 cacheid 设置为
CACHE_VIVT。函数最后打印了 CACHE 的信息。

> - [cpu_architecture](#A0045)
>
> - [read_cpuid_cachetype](#A0056)
>
> - [cpu_has_aliasing_icache](#A0061)
>
> - [cache_is_vivt](#A0062)
>
> - [cache_is_vipt_aliasing](#A0065)
>
> - [cache_is_vipt_nonaliasing](#A0064)
>
> - [icache_is_vivt_asid_tagged](#A0066)
>
> - [icache_is_vipt_aliasing](#A0067)
>
> - [cacheid](#A0069)

------------------------------------

#### <span id="A0069">cacheid</span>

{% highlight c %}
unsigned int cacheid __read_mostly;
{% endhighlight %}

cacheid 全局变量存储 CACHE 类型信息。

------------------------------------

#### <span id="A0070">__per_cpu_offset</span>

{% highlight c %}
/*
 * Generic SMP percpu area setup.
 *          
 * The embedding helper is used because its behavior closely resembles
 * the original non-dynamic generic percpu area setup.  This is
 * important because many archs have addressing restrictions and might
 * fail if the percpu area is located far away from the previous
 * location.  As an added bonus, in non-NUMA cases, embedding is
 * generally a good idea TLB-wise because percpu area can piggy back
 * on the physical linear memory mapping which uses large page
 * mappings on applicable archs.
 */
unsigned long __per_cpu_offset[NR_CPUS] __read_mostly;
EXPORT_SYMBOL(__per_cpu_offset);
{% endhighlight %}

------------------------------------

#### <span id="A0071">per_cpu_offset</span>

{% highlight c %}
/*      
 * per_cpu_offset() is the offset that has to be added to a
 * percpu variable to get to the instance for a certain processor.
 *
 * Most arches use the __per_cpu_offset array for those offsets but
 * some arches have their own ways of determining the offset (x86_64, s390).
 */
#ifndef __per_cpu_offset
extern unsigned long __per_cpu_offset[NR_CPUS];

#define per_cpu_offset(x) (__per_cpu_offset[x])
#endif
{% endhighlight %}

per_cpu_offset() 函数用于获得特定 CPU 在 __per_cpu_offset[]
数组中的偏移。

------------------------------------

#### <span id="A0072">cpu_proc_init</span>

{% highlight c %}
#define PROC_VTABLE(f)                  processor.f

#define cpu_proc_init                   PROC_VTABLE(_proc_init)
{% endhighlight %}

用于调用 ARM 体系相关的 proc_init 函数，在 ARMv9 初始化
过程中，cpu_proc_init() 函数最终挂钩到 cpu_v7_proc_init()
函数。该函数实现与体系有关，例如 ARMv7 Cortex-A9mp 中的实现如下：

{% highlight c %}
ENTRY(cpu_v7_proc_init)
        ret     lr
ENDPROC(cpu_v7_proc_init)
{% endhighlight %}

> - [ARMv7 Cortex-A9mp proc_info](https://biscuitos.github.io/blog/ARM-SCD-kernel-head.S/#ARMv7%20Cortex-A9%20proc_info_list)

------------------------------------

#### <span id="A0073">cpu_init</span>

{% highlight c %}
/*
 * cpu_init - initialise one CPU.
 *
 * cpu_init sets up the per-CPU stacks.
 */
void notrace cpu_init(void)
{
#ifndef CONFIG_CPU_V7M
        unsigned int cpu = smp_processor_id();
        struct stack *stk = &stacks[cpu];

        if (cpu >= NR_CPUS) {
                pr_crit("CPU%u: bad primary CPU number\n", cpu);
                BUG();
        }       

        /*
         * This only works on resume and secondary cores. For booting on the
         * boot cpu, smp_prepare_boot_cpu is called after percpu area setup.
         */
        set_my_cpu_offset(per_cpu_offset(cpu));

        cpu_proc_init();

        /*
         * Define the placement constraint for the inline asm directive below.
         * In Thumb-2, msr with an immediate value is not allowed.
         */
#ifdef CONFIG_THUMB2_KERNEL
#define PLC     "r"
#else
#define PLC     "I"
#endif
        /*
         * setup stacks for re-entrant exception handlers
         */
        __asm__ (
        "msr    cpsr_c, %1\n\t"
        "add    r14, %0, %2\n\t"
        "mov    sp, r14\n\t"
        "msr    cpsr_c, %3\n\t"
        "add    r14, %0, %4\n\t"
        "mov    sp, r14\n\t"
        "msr    cpsr_c, %5\n\t"
        "add    r14, %0, %6\n\t"
        "mov    sp, r14\n\t"
        "msr    cpsr_c, %7\n\t"
        "add    r14, %0, %8\n\t"
        "mov    sp, r14\n\t"
        "msr    cpsr_c, %9"
            :
            : "r" (stk),
              PLC (PSR_F_BIT | PSR_I_BIT | IRQ_MODE),
              "I" (offsetof(struct stack, irq[0])),
              PLC (PSR_F_BIT | PSR_I_BIT | ABT_MODE),
              "I" (offsetof(struct stack, abt[0])),
              PLC (PSR_F_BIT | PSR_I_BIT | UND_MODE),
              "I" (offsetof(struct stack, und[0])),
              PLC (PSR_F_BIT | PSR_I_BIT | FIQ_MODE),
              "I" (offsetof(struct stack, fiq[0])),
              PLC (PSR_F_BIT | PSR_I_BIT | SVC_MODE)
            : "r14");
#endif
}
{% endhighlight %}

cpu_init() 函数用于初始化一个 CPU，并设置了 per-CPU 的堆栈。
在 ARMv7 中，没有定义宏 CONFIG_CPU_V7M，因此执行下面的代码：

{% highlight c %}
        unsigned int cpu = smp_processor_id();
        struct stack *stk = &stacks[cpu];

        if (cpu >= NR_CPUS) {
                pr_crit("CPU%u: bad primary CPU number\n", cpu);
                BUG();
        }       

        /*
         * This only works on resume and secondary cores. For booting on the
         * boot cpu, smp_prepare_boot_cpu is called after percpu area setup.
         */
        set_my_cpu_offset(per_cpu_offset(cpu));

        cpu_proc_init();
{% endhighlight %}

函数首先调用 smp_processor_id() 函数获得当前正在使用的 CPU
号，然后获得当前 CPU 对应的全局堆栈，全局堆栈通过 stacks[]
维护。如果当前使用的 CPU 号大于 NR_CPUS，那么系统将提出警告。
接着函数使用 set_my_cpu_offset() 函数将当前 CPU 在 per_cpu_offset()
中的值写入到 ARMv7 的 TPIDRPRW 寄存器里。接着函数调用
cpu_proc_init() 函数简介调用体系相关的 proc_init() 函数，
在 ARMv7 Cortex-A9mp 中，proc_init() 对应的函数是
cpu_v7_proc_init()。函数接下来使用内嵌汇编，如下：

{% highlight c %}
        /*
         * setup stacks for re-entrant exception handlers
         */
        __asm__ (
        "msr    cpsr_c, %1\n\t"
        "add    r14, %0, %2\n\t"
        "mov    sp, r14\n\t"
        "msr    cpsr_c, %3\n\t"
        "add    r14, %0, %4\n\t"
        "mov    sp, r14\n\t"
        "msr    cpsr_c, %5\n\t"
        "add    r14, %0, %6\n\t"
        "mov    sp, r14\n\t"
        "msr    cpsr_c, %7\n\t"
        "add    r14, %0, %8\n\t"
        "mov    sp, r14\n\t"
        "msr    cpsr_c, %9"
            :
            : "r" (stk),
              PLC (PSR_F_BIT | PSR_I_BIT | IRQ_MODE),
              "I" (offsetof(struct stack, irq[0])),
              PLC (PSR_F_BIT | PSR_I_BIT | ABT_MODE),
              "I" (offsetof(struct stack, abt[0])),
              PLC (PSR_F_BIT | PSR_I_BIT | UND_MODE),
              "I" (offsetof(struct stack, und[0])),
              PLC (PSR_F_BIT | PSR_I_BIT | FIQ_MODE),
              "I" (offsetof(struct stack, fiq[0])),
              PLC (PSR_F_BIT | PSR_I_BIT | SVC_MODE)
            : "r14");
{% endhighlight %}

这段内嵌汇编首先修改了 CPSR 寄存器，在分析具体的源码之前，
可以查看一下 ARMv7 的 CPSR 寄存器布局，如下：

![](https://raw.githubusercontent.com/EmulateSpace/PictureSet/master/BiscuitOS/boot/BOOT000001.png)

函数首先调用 MSR 指令，将 PSR_F_BIT，PSR_I_BIT 和
IRQ_MODE 写入到 CPSR 寄存器中，代码执行之后，CPSR
寄存器的 F 标志位和 I 标志位被置位，然后进入 IRQ 模式。
F 标志置位之后，系统将启用 FIRQ 中断，I 标志置位之后，
系统将启动 IRQ 中断。此时调用 ADD 指令，将 stk 的地址
加上 offsetof(struct stack, irq[0])，以此获得 stk->irq
的地址，此时将堆栈的地址写入到 stk->irq 内，也就是当进入
中断之后，当前 CPU 使用的中断堆栈。同理，调用 MSR 指令，
将 PSR_F_BIT，PSR_I_BIT 和 ABT_MODE 写入到 CPSR 寄存器，
此时系统处于 ABORT 模式，此时将 offsetof(struct stack, abt[0])
的值通过 ADD 指令与 stk 的地址相加，以此获得 stk->atb
地址，然后将当前堆栈的地址写入到 stk->atb 中，这样就可以
知道当系统进入 ABORT 时候，系统所使用的堆栈了。同理设置了
UND 模式和 FIQ 模式时，tsk 对应的堆栈信息。最后将
CPSR 寄存器的 F 标志和 I 标志置位，打开 FIRQ 和 IRQ，
并进入 SVC 模式。

> - [smp_processor_id](#A0015)
>
> - [set_my_cpu_offset](#A0010)
>
> - [per_cpu_offset](#A0071)
>
> - [cpu_proc_init](#A0072)
>
> - [ARMv7 Cortex-A9mp proc_info](https://biscuitos.github.io/blog/ARM-SCD-kernel-head.S/#ARMv7%20Cortex-A9%20proc_info_list)

------------------------------------

#### <span id="A0074">setup_processor</span>

{% highlight c %}
static void __init setup_processor(void)
{
        unsigned int midr = read_cpuid_id();
        struct proc_info_list *list = lookup_processor(midr);

        cpu_name = list->cpu_name;
        __cpu_architecture = __get_cpu_architecture();

        init_proc_vtable(list->proc);
#ifdef MULTI_TLB
        cpu_tlb = *list->tlb;
#endif
#ifdef MULTI_USER
        cpu_user = *list->user;
#endif
#ifdef MULTI_CACHE
        cpu_cache = *list->cache;
#endif

        pr_info("CPU: %s [%08x] revision %d (ARMv%s), cr=%08lx\n",
                list->cpu_name, midr, midr & 15,
                proc_arch[cpu_architecture()], get_cr());

        snprintf(init_utsname()->machine, __NEW_UTS_LEN + 1, "%s%c",
                 list->arch_name, ENDIANNESS);
        snprintf(elf_platform, ELF_PLATFORM_SIZE, "%s%c",
                 list->elf_name, ENDIANNESS);
        elf_hwcap = list->elf_hwcap;

        cpuid_init_hwcaps();
        patch_aeabi_idiv();

#ifndef CONFIG_ARM_THUMB
        elf_hwcap &= ~(HWCAP_THUMB | HWCAP_IDIVT);
#endif
#ifdef CONFIG_MMU
        init_default_cache_policy(list->__cpu_mm_mmu_flags);
#endif
        erratum_a15_798181_init();

        elf_hwcap_fixup();

        cacheid_init();
        cpu_init();
}
{% endhighlight %}

setup_processor() 函数用于初始化体系相关的处理器。函数较长，
分段解析，

{% highlight c %}
        unsigned int midr = read_cpuid_id();
        struct proc_info_list *list = lookup_processor(midr);

        cpu_name = list->cpu_name;
        __cpu_architecture = __get_cpu_architecture();

        init_proc_vtable(list->proc);
{% endhighlight %}

函数首先调用 read_cpuid_id() 函数获得 ARM 的 ID_MMFR0 寄存器，
存储在 midr 局部变量，然后调用 lookup_processor() 函数获得
体系相关的 proc_info_list 结构，该结构包含了系统内存模组等
基础信息。函数接着将获得的 proc_info_list 的 cpu_name 成员
信息存储到全局 cpu_name 中，以此维护当前系统名字。函数
继续调用 __get_cpu_architecture() 函数，将体系信息存储到
全局变量 __cpu_architecture 中，接着调用
init_proc_vtable() 函数设置系统的 proc_info vtable.

{% highlight c %}
#ifdef MULTI_TLB
        cpu_tlb = *list->tlb;
#endif
#ifdef MULTI_USER
        cpu_user = *list->user;
#endif
#ifdef MULTI_CACHE
        cpu_cache = *list->cache;
#endif
{% endhighlight %}

如果定义了 MULTI_TLB 宏，那么将全局变量 cpu_tlb 指向
proc_info 的 tlb 成员。如果定义了 MULTI_USER，那么将
cpu_user 指向 proc_info 的 user 成员。如果定义了
MULTI_CACHE，那么将 cpu_cache 指向 proc_info 的 cache
成员。

{% highlight c %}
        pr_info("CPU: %s [%08x] revision %d (ARMv%s), cr=%08lx\n",
                list->cpu_name, midr, midr & 15,
                proc_arch[cpu_architecture()], get_cr());

        snprintf(init_utsname()->machine, __NEW_UTS_LEN + 1, "%s%c",
                 list->arch_name, ENDIANNESS);
        snprintf(elf_platform, ELF_PLATFORM_SIZE, "%s%c",
                 list->elf_name, ENDIANNESS);
        elf_hwcap = list->elf_hwcap;

        cpuid_init_hwcaps();
        patch_aeabi_idiv();
{% endhighlight %}

函数打印了 CPU 相关的版本信息，并设置了 UTS 的 machine，
elf_platform 也被设置为 proc_info 的 elf_name。函数
将体系的 proc_info 中 elf_hwcap 成员存储到 elf_hwcap
全局变量中进行维护，并调用 cpuid_init_hwcaps() 函数，
该函数根据实际体系寄存器的值，设置了全局变量 elf_hwcap,
最后调用 patch_aeabi_idiv() 获得当前体系支持的硬件
DIV 指令。本段代码主要获得体系的硬件信息。

{% highlight c %}
#ifndef CONFIG_ARM_THUMB
        elf_hwcap &= ~(HWCAP_THUMB | HWCAP_IDIVT);
#endif
#ifdef CONFIG_MMU
        init_default_cache_policy(list->__cpu_mm_mmu_flags);
#endif
        erratum_a15_798181_init();

        elf_hwcap_fixup();

        cacheid_init();
        cpu_init();
{% endhighlight %}

这段代码中，函数设置默认的 cache 策略，并调用 elf_hwcap_fixup()
函数修正了硬件支持信息。调用 cacheid_init() 函数获得当前系统
时候的 cache 信息，并维护在指定的全局变量里。最后调用 cpu_init()
函数初始化了一个 CPU，设置其各种模式下的堆栈信息，维护在系统的
stacks[] 数组结构中。至此函数初始化硬件完毕，函数将相应的硬件信息
维护到系统的全局变量里。

> - [read_cpuid_id](https://biscuitos.github.io/blog/CPUID_read_cpuid_id/)
>
> - [lookup_processor](#A0039)
>
> - [\_\_get_cpu_architecture](#A0041)
>
> - [get_cr](#A0043)
>
> - [cpuid_init_hwcaps](#A0049)
>
> - [init_default_cache_policy](#A0053)
>
> - [elf_hwcap_fixup](#A0055)
>
> - [cacheid_init](#A0068)
>
> - [cpu_init](#A0072)

------------------------------------

#### <span id="A0075">fdt32_ld</span>

{% highlight c %}
static inline uint32_t fdt32_ld(const fdt32_t *p)
{
        const uint8_t *bp = (const uint8_t *)p;

        return ((uint32_t)bp[0] << 24)
                | ((uint32_t)bp[1] << 16)
                | ((uint32_t)bp[2] << 8)
                | bp[3];
}
{% endhighlight %}

fdt32_ld() 函数的作用是将一个 32 位的大端数据转换为一个 32 位
的小端数据。函数首先获得参数 p 的起始地址，然后按字节进行大小
段转换。

------------------------------------

#### <span id="A0076">fdt_get_header</span>

{% highlight c %}
#define fdt_get_header(fdt, field) \
        (fdt32_ld(&((const struct fdt_header *)(fdt))->field))
{% endhighlight %}

fdt_get_header() 函数用于从 DTB 的 header 读出指定的信息。
DTB 二进制文件起始的位置是一个 struct fdt_header 结构，
函数通过从 DTB 的头部读出指定数据之后，由于 DTB 内的数据
采用大端模式存储，而 ARM 是小端模式，因此需要调用 fdt32_ld()
函数将大端数据转换成小端数据。

> - [fdt32_ld](#A0075)
>
> - [DTB 二进制文件结构](https://biscuitos.github.io/blog/DTS/#M00)
>
> - [fd_header 数据结构解析](https://biscuitos.github.io/blog/DTS/#B010)

------------------------------------

#### <span id="A0077">fdt_magic</span>

{% highlight c %}
#define fdt_magic(fdt)                  (fdt_get_header(fdt, magic))
{% endhighlight %}

fdt_magic() 函数用于读取 DTB header 的 MAGIC 信息。
函数通过调用 fdt_get_header() 函数获得 DTB 二进制文件
struct fdt_header 的 magic 成员。

> - [fdt_get_header](#A0076)
>
> - [fd_header 数据结构解析](https://biscuitos.github.io/blog/DTS/#B010)

------------------------------------

#### <span id="A0078">fdt_totalsize</span>

{% highlight c %}
#define fdt_totalsize(fdt)              (fdt_get_header(fdt, totalsize))
{% endhighlight %}

fdt_totalsize() 函数用于读取 DTB header 的 totalsize 信息。
函数通过调用 fdt_get_header() 函数获得 DTB 二进制文件
struct fdt_header 的 fdt_totalsize 成员，该成员存储 DTB
二进制文件的大小。

> - [fdt_get_header](#A0076)
>
> - [fd_header 数据结构解析](https://biscuitos.github.io/blog/DTS/#B010)

------------------------------------

#### <span id="A0079">fdt_off_dt_struct</span>

{% highlight c %}
#define fdt_off_dt_struct(fdt)          (fdt_get_header(fdt, off_dt_struct))
{% endhighlight %}

fdt_off_dt_struct() 函数用于读取 DTB header 的 off_dt_struct 信息。
函数通过调用 fdt_get_header() 函数获得 DTB 二进制文件
struct fdt_header 的 off_dt_struct 成员，该成员指向 DTB
二进制文件中 DeviceTree struct 的偏移地址。

> - [fdt_get_header](#A0076)
>
> - [fd_header 数据结构解析](https://biscuitos.github.io/blog/DTS/#B010)

------------------------------------

#### <span id="A0080">fdt_off_dt_strings</span>

{% highlight c %}
#define fdt_off_dt_strings(fdt)         (fdt_get_header(fdt, off_dt_strings))
{% endhighlight %}

fdt_off_dt_strings() 函数用于读取 DTB header 的 off_dt_strings 信息。
函数通过调用 fdt_get_header() 函数获得 DTB 二进制文件
struct fdt_header 的 off_dt_strings 成员，该成员指向 DTB
二进制文件中 DeviceTree string 的偏移地址。

> - [fdt_get_header](#A0076)
>
> - [fd_header 数据结构解析](https://biscuitos.github.io/blog/DTS/#B010)

------------------------------------

#### <span id="A0081">fdt_off_mem_rsvmap</span>

{% highlight c %}
#define fdt_off_mem_rsvmap(fdt)         (fdt_get_header(fdt, off_mem_rsvmap))
{% endhighlight %}

fdt_off_mem_rsvmap() 函数用于读取 DTB header 的 off_mem_revmap 信息。
函数通过调用 fdt_get_header() 函数获得 DTB 二进制文件
struct fdt_header 的 off_mem_revmap 成员，该成员指向 DTB
二进制文件中 Reserved memory 的偏移地址。

> - [fdt_get_header](#A0076)
>
> - [fd_header 数据结构解析](https://biscuitos.github.io/blog/DTS/#B010)

------------------------------------

#### <span id="A0082">fdt_version</span>

{% highlight c %}
#define fdt_version(fdt)                (fdt_get_header(fdt, version))
{% endhighlight %}

fdt_version() 函数用于读取 DTB header 的 version 信息。
函数通过调用 fdt_get_header() 函数获得 DTB 二进制文件
struct fdt_header 的 version 成员，该成员指向 DeviceTree
的版本信息。

> - [fdt_get_header](#A0076)
>
> - [fd_header 数据结构解析](https://biscuitos.github.io/blog/DTS/#B010)

------------------------------------

#### <span id="A0083">fdt_last_comp_version</span>

{% highlight c %}
#define fdt_last_comp_version(fdt)      (fdt_get_header(fdt, last_comp_version))
{% endhighlight %}

fdt_last_comp_version() 函数用于读取 DTB header 的 last_comp_version 信息。
函数通过调用 fdt_get_header() 函数获得 DTB 二进制文件
struct fdt_header 的 last_comp_version 成员，该成员指向 DeviceTree
上一版本的兼容信息。

> - [fdt_get_header](#A0076)
>
> - [fd_header 数据结构解析](https://biscuitos.github.io/blog/DTS/#B010)

------------------------------------

#### <span id="A0085">fdt_boot_cpuid_phys</span>

{% highlight c %}
#define fdt_boot_cpuid_phys(fdt)        (fdt_get_header(fdt, boot_cpuid_phys))
{% endhighlight %}

fdt_boot_cpuid_phys() 函数用于读取 DTB header 的 boot_cpuid_phys 信息。
函数通过调用 fdt_get_header() 函数获得 DTB 二进制文件
struct fdt_header 的 boot_cpuid_phys 成员，该成员指向 DeviceTree
使用 boot CPU 信息。

> - [fdt_get_header](#A0076)
>
> - [fd_header 数据结构解析](https://biscuitos.github.io/blog/DTS/#B010)

------------------------------------

#### <span id="A0086">fdt_size_dt_strings</span>

{% highlight c %}
#define fdt_size_dt_strings(fdt)        (fdt_get_header(fdt, size_dt_strings))
{% endhighlight %}

fdt_boot_cpuid_phys() 函数用于读取 DTB header 的 dt_strings_size 信息。
函数通过调用 fdt_get_header() 函数获得 DTB 二进制文件
struct fdt_header 的 dt_strings_size 成员，该成员指明了 DTB 文件
中 string 的长度。

> - [fdt_get_header](#A0076)
>
> - [fd_header 数据结构解析](https://biscuitos.github.io/blog/DTS/#B010)

------------------------------------

#### <span id="A0087">fdt_size_dt_struct</span>

{% highlight c %}
#define fdt_size_dt_struct(fdt)         (fdt_get_header(fdt, size_dt_struct))
{% endhighlight %}

fdt_size_dt_struct() 函数用于读取 DTB header 的 dt_struct_size 信息。
函数通过调用 fdt_get_header() 函数获得 DTB 二进制文件
struct fdt_header 的 dt_struct_size 成员，该成员指明了 DTB 文件
中 DeviceTree structure 的长度。

> - [fdt_get_header](#A0076)
>
> - [fd_header 数据结构解析](https://biscuitos.github.io/blog/DTS/#B010)

------------------------------------

#### <span id="A0088">fdt_header_size_</span>

{% highlight c %}
#define FDT_V1_SIZE     (7*sizeof(fdt32_t))
#define FDT_V2_SIZE     (FDT_V1_SIZE + sizeof(fdt32_t))
#define FDT_V3_SIZE     (FDT_V2_SIZE + sizeof(fdt32_t))
#define FDT_V16_SIZE    FDT_V3_SIZE
#define FDT_V17_SIZE    (FDT_V16_SIZE + sizeof(fdt32_t)


size_t fdt_header_size_(uint32_t version)
{               
        if (version <= 1)
                return FDT_V1_SIZE;
        else if (version <= 2)
                return FDT_V2_SIZE;
        else if (version <= 3)
                return FDT_V3_SIZE;
        else if (version <= 16)
                return FDT_V16_SIZE;
        else
                return FDT_V17_SIZE;
}
{% endhighlight %}

fdt_header_size_() 函数用于获得 DTB header 结构的
长度。不同版本的 DTB 二进制文件 header 长度不一样。
函数根据 version 参数获得对应的 header 长度，每个版本
 header 长度都通过宏 FDT_VX_SIZE 定义。

------------------------------------

#### <span id="A0089">fdt_header_size</span>

{% highlight c %}
static inline size_t fdt_header_size(const void *fdt)
{       
        return fdt_header_size_(fdt_version(fdt));
}
{% endhighlight %}

fdt_header_size() 函数用于获得 DTB 二进制文件 header
的长度。函数首先通过 fdt_version() 获得 DTB 的版本号，
然后通过 fdt_header_size_() 函数计算出版本号对应的
DTB header 长度。

> - [fdt_header_size_](#A0088)
>
> - [fdt_version](#A0082)

------------------------------------

#### <span id="A0090">check_off_</span>

{% highlight c %}
static int check_off_(uint32_t hdrsize, uint32_t totalsize, uint32_t off)
{
        return (off >= hdrsize) && (off <= totalsize);
}
{% endhighlight %}

check_off_() 函数用于检测参数 off 是在 hdrsize 和 totalsize 之间。

------------------------------------

#### <span id="A0091">check_block_</span>

{% highlight c %}
static int check_block_(uint32_t hdrsize, uint32_t totalsize,
                        uint32_t base, uint32_t size)
{
        if (!check_off_(hdrsize, totalsize, base))
                return 0; /* block start out of bounds */
        if ((base + size) < base)
                return 0; /* overflow */
        if (!check_off_(hdrsize, totalsize, base + size))
                return 0; /* block end out of bounds */
        return 1;
}
{% endhighlight %}

check_block_() 函数检查 "base+size" 的和是否在 hdrsize 与
totalsize 之间。函数首先调用 check_off_() 函数判断 base
是否在 hdrsize 与 totalsize 之间，如果不在，则返回 0；如果在，
继续判断 base+size 是否溢出，如果溢出，直接返回 0,；反之，
继续检查 base+size 的和是否在 hdrsize 和 totalsize 之间。

> - [check_off_](#A0090)

------------------------------------

#### <span id="A0092">fdt_check_header</span>

{% highlight c %}
int fdt_check_header(const void *fdt)
{
        size_t hdrsize;

        if (fdt_magic(fdt) != FDT_MAGIC)
                return -FDT_ERR_BADMAGIC;
        hdrsize = fdt_header_size(fdt);
        if ((fdt_version(fdt) < FDT_FIRST_SUPPORTED_VERSION)
            || (fdt_last_comp_version(fdt) > FDT_LAST_SUPPORTED_VERSION))
                return -FDT_ERR_BADVERSION;
        if (fdt_version(fdt) < fdt_last_comp_version(fdt))
                return -FDT_ERR_BADVERSION;

        if ((fdt_totalsize(fdt) < hdrsize)
            || (fdt_totalsize(fdt) > INT_MAX))
                return -FDT_ERR_TRUNCATED;

        /* Bounds check memrsv block */
        if (!check_off_(hdrsize, fdt_totalsize(fdt), fdt_off_mem_rsvmap(fdt)))
                return -FDT_ERR_TRUNCATED;

        /* Bounds check structure block */
        if (fdt_version(fdt) < 17) {
                if (!check_off_(hdrsize, fdt_totalsize(fdt),
                                fdt_off_dt_struct(fdt)))
                        return -FDT_ERR_TRUNCATED;
        } else {
                if (!check_block_(hdrsize, fdt_totalsize(fdt),
                                  fdt_off_dt_struct(fdt),
                                  fdt_size_dt_struct(fdt)))
                        return -FDT_ERR_TRUNCATED;
        }

        /* Bounds check strings block */
        if (!check_block_(hdrsize, fdt_totalsize(fdt),
                          fdt_off_dt_strings(fdt), fdt_size_dt_strings(fdt)))
                return -FDT_ERR_TRUNCATED;

        return 0;
}
{% endhighlight %}

fdt_check_header() 函数用于检查 DTB 二进制文件是否有效。
由于函数太长，分段解析：

{% highlight c %}
        if (fdt_magic(fdt) != FDT_MAGIC)
                return -FDT_ERR_BADMAGIC;
        hdrsize = fdt_header_size(fdt);
        if ((fdt_version(fdt) < FDT_FIRST_SUPPORTED_VERSION)
            || (fdt_last_comp_version(fdt) > FDT_LAST_SUPPORTED_VERSION))
                return -FDT_ERR_BADVERSION;
        if (fdt_version(fdt) < fdt_last_comp_version(fdt))
                return -FDT_ERR_BADVERSION;
{% endhighlight %}

函数首先调用 fdt_magic() 函数检查 DTB 文件的 MAGIC 是否
与 FDT_MAGIC 一致，如果不一致，直接返回 FDT_ERR_BADMAGIC；
如果一致，然后调用 fdt_header_size() 函数获得 DTB header
的长度。接着通过 fdt_version() 函数判断 DTB 的版本如果小于
FDT_FIRST_SUPPORTED_VERSION，或则调用 fdt_last_comp_version()
函数判断 DTB 上一个兼容的版本大于 FDT_LAST_SUPPORTED_VERSION，
那么判断 DTB 是无效的，直接返回 FDT_ERR_BADVERSION。函数继续
通过调用 fdt_version() 与 fdt_last_comp_version() 进行判断，
如果 DTB 当前的版本小于上一个兼容版本，那么判定 DTB 无效，直接
返回 FDT_ERR_BADVERSION.

{% highlight c %}
        if ((fdt_totalsize(fdt) < hdrsize)
            || (fdt_totalsize(fdt) > INT_MAX))
                return -FDT_ERR_TRUNCATED;

        /* Bounds check memrsv block */
        if (!check_off_(hdrsize, fdt_totalsize(fdt), fdt_off_mem_rsvmap(fdt)))
                return -FDT_ERR_TRUNCATED;
{% endhighlight %}

函数继续检查，如果 DTB 的长度比 DTB header 小，或者
DTB 的长度比 INT_MAX 大，那么函数判定 DTB 无效，直接返回
FDT_ERR_TRUNCATED；函数接着调用 check_off_() 检查
DTB 的 reserved 区域是否在大于 DTB header 而小于 DTB
二进制文件大小的区域内，如果不在，则返回 FDT_ERR_TRUNCATED。

{% highlight c %}
        /* Bounds check structure block */
        if (fdt_version(fdt) < 17) {
                if (!check_off_(hdrsize, fdt_totalsize(fdt),
                                fdt_off_dt_struct(fdt)))
                        return -FDT_ERR_TRUNCATED;
        } else {
                if (!check_block_(hdrsize, fdt_totalsize(fdt),
                                  fdt_off_dt_struct(fdt),
                                  fdt_size_dt_struct(fdt)))
                        return -FDT_ERR_TRUNCATED;
        }

        /* Bounds check strings block */
        if (!check_block_(hdrsize, fdt_totalsize(fdt),
                          fdt_off_dt_strings(fdt), fdt_size_dt_strings(fdt)))
                return -FDT_ERR_TRUNCATED;

{% endhighlight %}

函数继续检查，如果 DTB 的版本小于 17，如果此时 DTB 文件中
DeviceTree struct 的位置不在 hdrsize 和 totalsize 之间，那么
认定 DTB 无效，直接返回 FDT_ERR_TRUNCATED；反之，如果 DTB 版本
大于等于 17，那么 DeviceTree struct 的长度不在 hdrsize
和 totalsize 之间，那么函数认定 DTB 无效，直接返回
FDT_ERR_TRUNCATED。函数最后检查 DeviceTree string 的
长度是否在 hdrsize 与 totalsize 之间，如果不在，那么
函数认定 DTB 无效，直接返回 FDT_ERR_TRUNCATED。

> - [fdt_magic](#A0077)
>
> - [fdt_header_size](#A0089)
>
> - [fdt_version](#A0082)
>
> - [fdt_totalsize](#A0078)
>
> - [check_off_](#A0090)
>
> - [check_block_](#A0091)
>
> - [fdt_off_mem_rsvmap](#A0081)
>
> - [fdt_off_dt_struct](#A0079)
>
> - [fdt_size_dt_struct](#A0087)
>
> - [fdt_lst_comp_version](#A0083)

------------------------------------

#### <span id="A0093">early_init_dt_verify</span>

{% highlight c %}
bool __init early_init_dt_verify(void *params)
{
        if (!params)
                return false;

        /* check device tree validity */
        if (fdt_check_header(params))
                return false;

        /* Setup flat device-tree pointer */
        initial_boot_params = params;
        of_fdt_crc32 = crc32_be(~0, initial_boot_params,
                                fdt_totalsize(initial_boot_params));
        return true;
}
{% endhighlight %}

early_init_dt_verify() 函数用于检查 DTB 的有效性。参数 params
指向 DTB 所在的虚拟地址。函数首先检查 params 的有效性，如果无效，
则直接返回 false；如果有效，函数调用 fdt_check_header() 检查 DTB
header 的有效性，如果无效，则返回 false；反之如果有效，则将 params
参数赋值给 initial_boot_params。最后函数调用 crc32_be() 函数
生成 DTB 的 CRC。

> - [fdt_check_header](#A0092)

------------------------------------

#### <span id="A0094">arch_get_next_mach</span>

{% highlight c %}
static const void * __init arch_get_next_mach(const char *const **match)
{       
        static const struct machine_desc *mdesc = __arch_info_begin;
        const struct machine_desc *m = mdesc;   

        if (m >= __arch_info_end)
                return NULL;

        mdesc++;
        *match = m->dt_compat;  
        return m;
}
{% endhighlight %}

arch_get_next_mach() 函数用于获得下一个 machine_desc 结构的
DeviceTree compatible strings 地址。函数首先使用局部变量 mdesc
和 m 指向 __arch_info_begin。然后每次调用 arch_get_next_mach(),
函数都会将 __arch_info_begin 指向下一个 machine_desc，以此获得
下一个 machine_desc，最后函数返回下一个 machine_desc 的 dt_compat
成员。

------------------------------------

#### <span id="A0095">fdt_offset_ptr_</span>

{% highlight c %}
static inline const void *fdt_offset_ptr_(const void *fdt, int offset)
{
        return (const char *)fdt + fdt_off_dt_struct(fdt) + offset;
}
{% endhighlight %}

fdt_offset_ptr_() 函数的作用是通过 device-node 在 DTB structure 内
的偏移得到指向 device-node 的指针。参数 fdt 指向 DTB，参数 offset
为 device-node 在 DTB structure 区块内的偏移。函数调用
fdt_off_dt_struct() 函数获得 structure 区块在 DTB 的位置，然后
将该值加上 DTB 的位置，最后一同加上 offset 偏移就可以获得
device-node 的地址。DTB 的结构设计如下图：

![](https://raw.githubusercontent.com/EmulateSpace/PictureSet/master/BiscuitOS/boot/BOOT000213.png)

> - [fdt_off_dt_struct](#A0079)
>
> - [DTB format describe](https://biscuitos.github.io/blog/DTS/#M00)

------------------------------------

#### <span id="A0096">fdt_offset_ptr</span>

{% highlight c %}
const void *fdt_offset_ptr(const void *fdt, int offset, unsigned int len)
{
        unsigned absoffset = offset + fdt_off_dt_struct(fdt);

        if ((absoffset < offset)
            || ((absoffset + len) < absoffset)
            || (absoffset + len) > fdt_totalsize(fdt))
                return NULL;

        if (fdt_version(fdt) >= 0x11)
                if (((offset + len) < offset)
                    || ((offset + len) > fdt_size_dt_struct(fdt)))
                        return NULL;

        return fdt_offset_ptr_(fdt, offset);
}
{% endhighlight %}

fdt_offset_ptr() 函数通过 device-node 在 DTB structure 内的
偏移获得指向 device-node 的指针。参数 fdt 指向 DTB，参数 offset
为 device-node 在 DTB structure 区域内的偏移，len 参数表示
device-node 的长度。函数首先通过 offset 参数和 fdt_off_dt_struct()
函数获得 device-node 在 DTB 内的偏移，其值存储在 absoffset 内，
如果 absoffset 小于 offset，表示 absoffset 溢出，或者
"absoffset+len" 的值小于 absoffset 表示溢出，或者大于
fdt_totalsize, 这几种情况都是不符合要求，不能获得正确的值，
因此直接返回 NULL。

函数调用 fdt_version() 函数获得 DTB 的版本，如果当前版本大于
等于 17，那么在这种情况下，"offset+len" 小于 offset 表示
"offset+len" 已经溢出。或者 "offset+len" 的值大于 DTB
structure 区块的长度，那么上述情况都不满足条件，直接返回 NULL。
如果上面的检查都通过，那么函数调用 fdt_offset_ptr_() 函数
获得 device-node 的指针。

> - [fdt_off_dt_struct](#A0079)
>
> - [fdt_totalsize](#A0078)
>
> - [fdt_size_dt_struct](#A0087)
>
> - [fdt_offset_ptr_](#A0095)

------------------------------------

#### <span id="A0097">CPU_TO_FDT32</span>

{% highlight c %}
#define CPU_TO_FDT32(x) ((EXTRACT_BYTE(x, 0) << 24) | (EXTRACT_BYTE(x, 1) << 16) | \    
                         (EXTRACT_BYTE(x, 2) << 8) | EXTRACT_BYTE(x, 3))
{% endhighlight %}

CPU_TO_FDT32() 函数用于将数据的大小端模式进行翻转。如果 x
代表的数据是大端模式，那么 CPU_TO_FDT32() 函数将转换为小端
模式，反之亦然。

------------------------------------

#### <span id="A0098">fdt32_to_cpu</span>

{% highlight c %}
static inline uint32_t fdt32_to_cpu(fdt32_t x)
{       
        return (FDT_FORCE uint32_t)CPU_TO_FDT32(x);
}
{% endhighlight %}

fdt32_to_cpu() 函数用于将 DTB/DTS 里的大端数据转换
为小端数据。函数通过调用 CPU_TO_FDT32() 函数实现。

> - [CPU_TO_FDT32](#A0097)

------------------------------------

#### <span id="A0099">fdt_next_tag</span>

{% highlight c %}
uint32_t fdt_next_tag(const void *fdt, int startoffset, int *nextoffset)
{
        const fdt32_t *tagp, *lenp;
        uint32_t tag;
        int offset = startoffset;
        const char *p;

        *nextoffset = -FDT_ERR_TRUNCATED;
        tagp = fdt_offset_ptr(fdt, offset, FDT_TAGSIZE);
        if (!tagp)
                return FDT_END; /* premature end */
        tag = fdt32_to_cpu(*tagp);
        offset += FDT_TAGSIZE;

        *nextoffset = -FDT_ERR_BADSTRUCTURE;
        switch (tag) {
        case FDT_BEGIN_NODE:
                /* skip name */
                do {
                        p = fdt_offset_ptr(fdt, offset++, 1);
                } while (p && (*p != '\0'));
                if (!p)
                        return FDT_END; /* premature end */
                break;

        case FDT_PROP:
                lenp = fdt_offset_ptr(fdt, offset, sizeof(*lenp));
                if (!lenp)
                        return FDT_END; /* premature end */
                /* skip-name offset, length and value */
                offset += sizeof(struct fdt_property) - FDT_TAGSIZE
                        + fdt32_to_cpu(*lenp);
                if (fdt_version(fdt) < 0x10 && fdt32_to_cpu(*lenp) >= 8 &&
                    ((offset - fdt32_to_cpu(*lenp)) % 8) != 0)
                        offset += 4;
                break;

        case FDT_END:
        case FDT_END_NODE:
        case FDT_NOP:
                break;

        default:
                return FDT_END;
        }

        if (!fdt_offset_ptr(fdt, startoffset, offset - startoffset))
                return FDT_END; /* premature end */

        *nextoffset = FDT_TAGALIGN(offset);
        return tag;
}
{% endhighlight %}

fdt_next_tag() 函数用于获得下一个 tag 在 DTB DeviceTree structure
内的偏移。在解析函数之前，开发者应该先了解 DTB 中 DeviceTree struct
区块的内存布局如下图：

![](https://raw.githubusercontent.com/EmulateSpace/PictureSet/master/BiscuitOS/boot/BOOT000214.png)

从上图可知，每个节点或子节点都是以 FDT_BEGIN_NODE 开始，到
FDT_END_NODE 结束。节点中可以嵌套节点，嵌套的节点称为父节点，
被嵌套的节点称为子节点。

{% highlight bash %}
FDT_BEGIN_NODE         .......... 节点 N
|
|---FDT_BEGIN_NODE     .......... 子节点 0
|   |
|   FDT_END_NODE
|
|---FDT_BEGIN_NODE     .......... 子节点 1
|   |
|   FDT_END_NODE
|
FDT_END_NODE
{% endhighlight %}

从上图可知，属性位于节点内部，并且每个属性以 FDT_PROP 开始，到下一个
FDT_PROP 结束。属性内包含了属性字符串大小，属性字符串在 device-tree strings
中的偏移，以及属性数值等信息。

{% highlight bash %}
FDT_BEGIN_NODE
|
|----FDT_PROP
|    |--- Property string size
|    |--- Property string offset
|    |--- Property value
|    
|----FDT_PROP
|    |--- Property string size
|    |--- Property string offset
|    |--- Property value
|
FDT_END_NODE
{% endhighlight %}

因此 fdt_next_tag() 函数在解析 DTB 下一个 tag 的信息按如下
逻辑进行分析：

{% highlight c %}
        const fdt32_t *tagp, *lenp;
        uint32_t tag;
        int offset = startoffset;
        const char *p;

        *nextoffset = -FDT_ERR_TRUNCATED;
        tagp = fdt_offset_ptr(fdt, offset, FDT_TAGSIZE);
        if (!tagp)
                return FDT_END; /* premature end */
        tag = fdt32_to_cpu(*tagp);
        offset += FDT_TAGSIZE;

        *nextoffset = -FDT_ERR_BADSTRUCTURE;
{% endhighlight %}

fdt_next_tag() 函数具有三个参数，fdt 参数指向 DTB；
参数 startoffset 指明当前 tag 在 DTB DeviceTree Structure 区域
内的偏移，参数 nextoffset 用于存储下一个 tag 在 DTB DeviceTree
Structure 区域内的偏移。函数首先调用 fdt_offset_ptr() 函数获得
当前 tag 的虚拟地址存储在 tagp 中，如果 tagp 为空，那么代表到达
不正确的末尾，为非正常值；如果 tagp 不为空，调用  fdt32_to_cpu()
函数读取 tagp 正确形式的值，此时 offset 指向一个 tag 的起始位置，
其值可能是一个 device node 的起始 tag (FDT_BEGIN_NODE), 也可能
是一个属性的起始 tag (FDT_PROP)。此时函数将 offset 的地址增加
FDT_TAGSIZE。并将 nextoffset 的值设置为 -FDT_ERR_BADSTRUCTURE.

{% highlight c %}
        switch (tag) {
        case FDT_BEGIN_NODE:
                /* skip name */
                do {
                        p = fdt_offset_ptr(fdt, offset++, 1);
                } while (p && (*p != '\0'));
                if (!p)
                        return FDT_END; /* premature end */
                break;

        case FDT_PROP:
                lenp = fdt_offset_ptr(fdt, offset, sizeof(*lenp));
                if (!lenp)
                        return FDT_END; /* premature end */
                /* skip-name offset, length and value */
                offset += sizeof(struct fdt_property) - FDT_TAGSIZE
                        + fdt32_to_cpu(*lenp);
                if (fdt_version(fdt) < 0x10 && fdt32_to_cpu(*lenp) >= 8 &&
                    ((offset - fdt32_to_cpu(*lenp)) % 8) != 0)
                        offset += 4;
                break;

        case FDT_END:
        case FDT_END_NODE:
        case FDT_NOP:
                break;

        default:
                return FDT_END;
        }
{% endhighlight %}

tag 里面存储着 DTB DeviceTree structure 区域 tag 相关的信息，
此时使用 switch 进行处理，如果 tag 的值是 FDT_BEGIN_NODE，那么
代表 startoffset 指向了一个 device node 的起始地址，由于 DTB
中定义一个 device node 的数据结果如下：

{% highlight bash %}
struct fdt_node_header {
    uint32_t tag;
    char name[0];
};
{% endhighlight %}

那么函数就找到 device node 结尾的位置，其结尾是一个字符串，
因此只要找到 `\0` 就可以找到 device node 的结尾，如果此时
找到的末尾为空，那么找到末尾，没必要继续查找下一个 tag，
直接返回 FDT_END；如果 tag 的值是 FDT_PROP，那么代表
startoffset 指向的是一个属性开始的偏移，在 DTB 中，属性的
定义如下：

{% highlight bash %}
struct fdt_property {
    uint32_t tag;
    uint32_t len;
    uint32_t nameoff;
    char data[0];
};
{% endhighlight %}

函数首先获得属性值 len 的长度，然后跳过属性的 nameoff，
最后再将 offset 的值加上属性值的长度，这样就可以到达下一个
tag 的起始位置。如果 DTB 的版本小于 17 且属性值长度大于 8
个字节，并且此时 offset 减去属性长度的值没有按 8 字节对齐，
那么需要将 offset 的值加上 4。至此，其余 tag 的值都不进行
处理。

{% highlight c %}
        if (!fdt_offset_ptr(fdt, startoffset, offset - startoffset))
                return FDT_END; /* premature end */

        *nextoffset = FDT_TAGALIGN(offset);
        return tag;
}
{% endhighlight %}

函数最后调用 fdt_offset_ptr() 函数，并传入 `offset-startoffset` 的
值以此定位到下一个 tag 的起始地址，此时对新活动地址进行判断，如果
地址有效，那么将 *nextoffset 指向下一个 tag 的偏移地址。最后返回 tag
的值，以此返回下一个 tag 类型。

> - [fdt_offset_ptr](#A00)
>
> - [fdt32_to_cpu](#A0098)
>
> - [fdt_version](#A0082)
>
> - [FDT_TAGALIGN](#A0101)

------------------------------------

#### <span id="A0100">FDT_ALIGN</span>

{% highlight c %}
#define FDT_ALIGN(x, a)         (((x) + (a) - 1) & ~((a) - 1))
{% endhighlight %}

FDT_ALIGN() 函数用于将 x 参数按 a 对齐。

------------------------------------

#### <span id="A0101">FDT_TAGALIGN</span>

{% highlight c %}
#define FDT_TAGALIGN(x)         (FDT_ALIGN((x), FDT_TAGSIZE))
{% endhighlight %}

FDT_TAGALIGN() 函数用于 DTB 中 tag 的对齐方式。其通过
FDT_ALIGN() 函数实现。

------------------------------------

#### <span id="A0102">fdt_check_node_offset_</span>

{% highlight c %}
int fdt_check_node_offset_(const void *fdt, int offset)
{
        if ((offset < 0) || (offset % FDT_TAGSIZE)
            || (fdt_next_tag(fdt, offset, &offset) != FDT_BEGIN_NODE))
                return -FDT_ERR_BADOFFSET;

        return offset;
}
{% endhighlight %}

fdt_check_node_offset_() 函数用于判断 offset 对应的
的值是否为 FDT device node。参数 fdt 指向 DTB，参数 offset
为 device node 在 DTB DeviceTree structure 内的偏移。
函数首先判断 offset 如果小于 0 则无效，或者 offset 没有按
FDT_TAGSIZE 对齐则无效，或者调用 fdt_next_tag() 函数
获得 offset 对应的下一个 tag 不是 FDT_BEGIN_NODE，
那么也是无效的。如果 offset 都不满足上面的情况，那么
offset 是一个有效的值。

> - [fdt_next_tag](#A0099)

------------------------------------

#### <span id="A0103">fdt_check_prop_offset_</span>

{% highlight c %}
int fdt_check_prop_offset_(const void *fdt, int offset)
{       
        if ((offset < 0) || (offset % FDT_TAGSIZE)
            || (fdt_next_tag(fdt, offset, &offset) != FDT_PROP))
                return -FDT_ERR_BADOFFSET;

        return offset;                            
}  
{% endhighlight %}

fdt_check_prop_offset_() 函数用于判断 offset 对应的
tag 是否为 FDT property。参数 fdt 指向 DTB，参数 offset
为 property 在 DTB DeviceTree structure 内的偏移。
如果 offset 小于 0，或者 offset 没有按 FDT_TAGSIZE 对齐，
或者调用 fdt_next_tag() 函数获得下一个 tag 不是 FDT_PROP，
那么 offset 是无效的 property tag；如果 offset 不满足
上面的条件，那么 offset 就是一个有效的 property tag。

> - [fdt_next_tag](#A0099)

------------------------------------

#### <span id="A0104">nextprop_</span>

{% highlight c %}
static int nextprop_(const void *fdt, int offset)
{
        uint32_t tag;
        int nextoffset;

        do {
                tag = fdt_next_tag(fdt, offset, &nextoffset);

                switch (tag) {
                case FDT_END:
                        if (nextoffset >= 0)
                                return -FDT_ERR_BADSTRUCTURE;
                        else
                                return nextoffset;

                case FDT_PROP:
                        return offset;
                }
                offset = nextoffset;
        } while (tag == FDT_NOP);

        return -FDT_ERR_NOTFOUND;
}
{% endhighlight %}

nextprop_() 函数用于获得下一个属性 tag 在 DTB DeviceTree
structure 内的偏移。参数 fdt 指向 DTB，offset 指向属性在
DeviceTree structure 内的偏移。函数首先调用 do while()
循环，通过调用 fdt_next_tag() 函数获得 offset 的下一个 tag，
将下一个 tag 的值返回，并将下一个 tag 在 DTB DeviceTree
structure 区域内的偏移存储在 nextoffset 内。此时，如果
tag 的值是 FDT_END，且 nextoffset 大于零，那么这不是一个
正常的 property 结尾，直接返回 FDT_ERRR_BADSTRUCTURE；
反之返回 nextoffset 的值。如果 tag 的值是 FDT_PROP，那么
检测到了 property 的结尾，则返回 offset。否则 tag 为
FDT_NOP 的时候，那么继续循环。循环结束还没找到对应的 tag，
那么直接返回 FDT_ERR_NOTFOUND。

> - [fdt_next_tag](#A0099)

------------------------------------

#### <span id="A0105">fdt_first_property_offset</span>

{% highlight c %}
int fdt_first_property_offset(const void *fdt, int nodeoffset)
{
        int offset;

        if ((offset = fdt_check_node_offset_(fdt, nodeoffset)) < 0)
                return offset;

        return nextprop_(fdt, offset);
}
{% endhighlight %}

fdt_first_property_offset() 函数用于获得 device node 的第一个
属性在 DTB DeviceTree structure 区域内的偏移。参数 fdt
指向 DTB，nodeoffset 代表 device-node 在 DTB DeviceTree structure 区域
内的偏移。函数首先调用 fdt_check_node_offset_() 函数检查 nodeoffset
对应位置是否是一个 device-node，如果不是直接返回错误值；如果是
device-node，那么 nodeoffset 的下一个 tag 可能是属性的 tag，
其偏移值是 offset，此时调用 nextprop_() 函数获得 device-node 的
第一个属性的偏移值；如果 device-node 无法通过 nextprop_() 函数找到
属性的值，那么函数返回错误码。

> - [fdt_check_node_offset_](#A0102)
>
> - [nextprop_](#A0104)

------------------------------------

#### <span id="A0106">fdt_next_property_offset</span>

{% highlight c %}
int fdt_next_property_offset(const void *fdt, int offset)
{
        if ((offset = fdt_check_prop_offset_(fdt, offset)) < 0)
                return offset;

        return nextprop_(fdt, offset);
}
{% endhighlight %}

fdt_next_property_offset() 函数用于获得 offset 对应 property
的下一个 property 的偏移。参数 fdt 指向 DTB，参数 offset 指向
当前 property 在 DTB DeviceTree Structure 区域内的偏移。
函数首先调用 fdt_check_prop_offset_() 函数下一个 tag 是不是
property，如果不是则返回错误，如果那么调用 nextprop_() 函数获得
下一个属性在 DTB DeviceTree structure 内的偏移。

> - [fdt_check_prop_offset_](#A0103)
>
> - [nextprop_](#A0104)

------------------------------------

#### <span id="A0107">fdt_get_property_by_offset_</span>

{% highlight c %}
static const struct fdt_property *fdt_get_property_by_offset_(const void *fdt,
                                                              int offset,
                                                              int *lenp)
{
        int err;
        const struct fdt_property *prop;

        if ((err = fdt_check_prop_offset_(fdt, offset)) < 0) {
                if (lenp)
                        *lenp = err;
                return NULL;
        }

        prop = fdt_offset_ptr_(fdt, offset);

        if (lenp)
                *lenp = fdt32_ld(&prop->len);

        return prop;
}
{% endhighlight %}

fdt_get_property_by_offset_() 函数的左右就是通过属性在
DTB DeviceTree Structure 区域内的偏移获得属性值的长度。
参数 fdt 指向 DTB，参数 offset 指向 proprty 在 DTB
DeviceTree Structure 区域内的偏移，参数 lenp 用于存储
property 属性值长度。函数首先通过 fdt_check_prop_offset_()
函数检查 offset 对应的 property 是否是一个 property，
如果不是，将 lenp 指向的变量设置为错误码。如果 offset
对于的是一个有效的 property，那么调用 fdt_offset_ptr_()
计算出 perperty 的虚拟地址，接着 lenp 不为空的情况下，
调用 fd32_ld() 函数读取 property 属性值长度。在 DTB
中，property 定义如下：

{% highlight bash %}
struct fdt_property {
    uint32_t tag;
    uint32_t len;
    uint32_t nameoff;
    char data[0];
};
{% endhighlight %}

因此获得 len 长度位于 tag 之后，通过 tag 就可以获得 len
的值，也就是知道 property 属性值占用了多少个 u32.

> - [fdt_check_prop_offset_](#A0103)
>
> - [fdt_offset_ptr_](#A0095)
>
> - [fdt32_ld](#A0075)

------------------------------------

#### <span id="A0108">fdt_ro_probe_</span>

{% highlight c %}
/*
 * Minimal sanity check for a read-only tree. fdt_ro_probe_() checks
 * that the given buffer contains what appears to be a flattened
 * device tree with sane information in its header.
 */
int fdt_ro_probe_(const void *fdt)
{
        if (fdt_magic(fdt) == FDT_MAGIC) {
                /* Complete tree */
                if (fdt_version(fdt) < FDT_FIRST_SUPPORTED_VERSION)
                        return -FDT_ERR_BADVERSION;
                if (fdt_last_comp_version(fdt) > FDT_LAST_SUPPORTED_VERSION)
                        return -FDT_ERR_BADVERSION;
        } else if (fdt_magic(fdt) == FDT_SW_MAGIC) {
                /* Unfinished sequential-write blob */
                if (fdt_size_dt_struct(fdt) == 0)
                        return -FDT_ERR_BADSTATE;
        } else {
                return -FDT_ERR_BADMAGIC;
        }

        return 0;
}
{% endhighlight %}

fdt_ro_probe_() 函数用于一个只读 DTB 的最小健全性检测。参数 fdt
指向 DTB。函数首先调用 fdt_magic() 检查 DTB 的 MAGIC 信息，如果
MAGIC 为 FDT_MAGIC，那么 fdt 参数指向的是一个 FDT，在这个 MAGIC
模式下，函数调用 fdt_version() 函数检查 DTB 版本如果小于
FDT_FIRST_SUPPORTED_VERSION，那么 DTB 是无效的，直接返回
FDT_ERR_BADVERSION；如果没有返回，函数继续调用
fdt_last_comp_version() 函数判断 DTB 上一个兼容版本是否大于
FDT_LAST_SUPPORTED_VERSION，如果大于则 DTB 是无效的，直接
返回 FDT_ERR_BADVERSION。如果 DTB 的 MAGIC 是 FDT_SW_MAGIC，
那么函数调用 fdt_size_dt_struct() 函数获得 DTB DeviceTree
Structure 区域的长度，如果长度为 0，那么直接返回 FDT_ERR_BADSTATE；
反之如果 MAGIC 为其他，那么 DTB 也是无效的。至此，DTB 最小
健全性检测完毕，函数返回 0 表示通过。

> - [fdt_magic](#A0077)
>
> - [fdt_last_comp_version](#A0083)
>
> - [fdt_size_dt_struct](#A0087)

------------------------------------

#### <span id="A0109">fdt_get_string</span>

{% highlight c %}
const char *fdt_get_string(const void *fdt, int stroffset, int *lenp)
{
        uint32_t absoffset = stroffset + fdt_off_dt_strings(fdt);
        size_t len;
        int err;
        const char *s, *n;

        err = fdt_ro_probe_(fdt);
        if (err != 0)
                goto fail;

        err = -FDT_ERR_BADOFFSET;
        if (absoffset >= fdt_totalsize(fdt))
                goto fail;
        len = fdt_totalsize(fdt) - absoffset;

        if (fdt_magic(fdt) == FDT_MAGIC) {
                if (stroffset < 0)
                        goto fail;
                if (fdt_version(fdt) >= 17) {
                        if (stroffset >= fdt_size_dt_strings(fdt))
                                goto fail;
                        if ((fdt_size_dt_strings(fdt) - stroffset) < len)
                                len = fdt_size_dt_strings(fdt) - stroffset;
                }
        } else if (fdt_magic(fdt) == FDT_SW_MAGIC) {
                if ((stroffset >= 0)
                    || (stroffset < -fdt_size_dt_strings(fdt)))
                        goto fail;
                if ((-stroffset) < len)
                        len = -stroffset;
        } else {
                err = -FDT_ERR_INTERNAL;
                goto fail;
        }

        s = (const char *)fdt + absoffset;
        n = memchr(s, '\0', len);
        if (!n) {
                /* missing terminating NULL */
                err = -FDT_ERR_TRUNCATED;
                goto fail;
        }

        if (lenp)
                *lenp = n - s;
        return s;

fail:
        if (lenp)
                *lenp = err;
        return NULL;
}
{% endhighlight %}

fdt_get_string() 函数通过字符串在 DTB DeviceTree strings 区域内
的偏移，获得对应的字符串。由于函数较长，分段解析。

{% highlight c %}
const char *fdt_get_string(const void *fdt, int stroffset, int *lenp)
{
        uint32_t absoffset = stroffset + fdt_off_dt_strings(fdt);
        size_t len;
        int err;
        const char *s, *n;

        err = fdt_ro_probe_(fdt);
        if (err != 0)
                goto fail;

        err = -FDT_ERR_BADOFFSET;
        if (absoffset >= fdt_totalsize(fdt))
                goto fail;
        len = fdt_totalsize(fdt) - absoffset;
{% endhighlight %}

参数 fdt 指向 DTB，参数 stroffset 是字符串在 DTB DeviceTree
Strings 区域内的偏移，参数 lenp 用于存储字符串的长度。
函数首先调用 fdt_off_dt_strings() 函数加上 stroffset，
以此计算字符串在 DTB 内的偏移。函数接着调用 fdt_ro_probe()
函数对 DTB 做了最小健全性检查，如果不通过，则跳转到 fail 处。
如果字符串在 DTB 内的偏移查过了 DTB 的大小，那么函数也跳转
到 fail 处。函数将字符串到 DTB 结尾的长度存储到 len 局部变量里。

{% highlight c %}
        if (fdt_magic(fdt) == FDT_MAGIC) {
                if (stroffset < 0)
                        goto fail;
                if (fdt_version(fdt) >= 17) {
                        if (stroffset >= fdt_size_dt_strings(fdt))
                                goto fail;
                        if ((fdt_size_dt_strings(fdt) - stroffset) < len)
                                len = fdt_size_dt_strings(fdt) - stroffset;
                }
        } else if (fdt_magic(fdt) == FDT_SW_MAGIC) {
                if ((stroffset >= 0)
                    || (stroffset < -fdt_size_dt_strings(fdt)))
                        goto fail;
                if ((-stroffset) < len)
                        len = -stroffset;
        } else {
                err = -FDT_ERR_INTERNAL;
                goto fail;
        }
{% endhighlight %}

函数调用 fdt_magic() 函数获得 DTB 的 MAGIC，然后根据不同的
MAGIC 进行相应的处理。如果当前 DTB 的版本大于等于 17，且
字符串在 DTB DeviceTree strings 内的偏移小于 DTB DeviceTree
strings 区域的长度，且该偏移到 DTB DeviceTree strings 区域
结束的长度小于 len，那么 len 的值就是 DTB DeviceTree strings
区域的长度减去 stroffset 的值。如果 DTB 的 MAGIC 是
FDT_SW_MAGIC，那么函数计算出 len 的长度，如果不符合要求，
直接跳转到 fail 处。如果 MAGIC 为其他情况，那么函数直接跳转到
fail 处。

{% highlight c %}
        s = (const char *)fdt + absoffset;
        n = memchr(s, '\0', len);
        if (!n) {
                /* missing terminating NULL */
                err = -FDT_ERR_TRUNCATED;
                goto fail;
        }

        if (lenp)
                *lenp = n - s;
        return s;

fail:
        if (lenp)
                *lenp = err;
        return NULL;
{% endhighlight %}

经过上面的检测处理之后，通过 absoffset 获得字符串的虚拟
地址，然后通过 memchr() 函数找到字符串结束的虚拟地址，
将字符串结束地址减去起始其值就可以计算出字符串的长度，
最后将字符串的长度存储在 lenp 参数里。

> - [fdt_ro_probe_](#A0108)
>
> - [fdt_totalsize](#A0078)
>
> - [fdt_size_dt_strings](#A0086)
>
> - [fdt_magic](#A0077)

------------------------------------

#### <span id="A0110">fdt_string_eq_</span>

{% highlight c %}
static int fdt_string_eq_(const void *fdt, int stroffset,
                          const char *s, int len)
{
        int slen;
        const char *p = fdt_get_string(fdt, stroffset, &slen);

        return p && (slen == len) && (memcmp(p, s, len) == 0);
}
{% endhighlight %}

fdt_string_eq_() 用于对比 offset 对应 DTB DeviceTree strings
区域内字符串与参数 s 给定的字符串是否相等。参数 fdt 指向 DTB，
stroffset 指向字符串在 DTB DeviceTree strings 内偏移。参数 s
指向需要对比的字符串，len 是需要对比的长度。函数首先调用
fdt_get_string() 函数获得 stroffset 对应的字符串，然后使用
memcmp() 函数对比指定长度内字符串是否相等，如果相等，则返回 1，
返回返回 0.

> - [fdt_get_string](#A0109)

------------------------------------

#### <span id="A0111">fdt_get_property_namelen_</span>

{% highlight c %}
static const struct fdt_property *fdt_get_property_namelen_(const void *fdt,
                                                            int offset,
                                                            const char *name,
                                                            int namelen,
                                                            int *lenp,
                                                            int *poffset)
{
        for (offset = fdt_first_property_offset(fdt, offset);
             (offset >= 0);
             (offset = fdt_next_property_offset(fdt, offset))) {
                const struct fdt_property *prop;

                if (!(prop = fdt_get_property_by_offset_(fdt, offset, lenp))) {
                        offset = -FDT_ERR_INTERNAL;
                        break;
                }
                if (fdt_string_eq_(fdt, fdt32_ld(&prop->nameoff),
                                   name, namelen)) {
                        if (poffset)
                                *poffset = offset;
                        return prop;
                }
        }

        if (lenp)
                *lenp = offset;
        return NULL;
}
{% endhighlight %}

fdt_get_property_namelen_() 函数通过 name 参数获得 DTB
中 property。参数 fdt 指向 DTB，参数 offset 指向 property
在 DTB DeviceTree Structure 中的偏移，name 参数为需要查找
的 property 名字，参数 namelen 代表名字的长度，lenp 用于
存储 property 的偏移。
函数首先调用 fdt_first_property_offset() 获得 offset 参数
开始之后的第一个 property，然后通过 fdt_next_property_offset()
用于获得下一个 property，只要 offset 大于零，循环不停止。
每次遍历一个 property，函数都会通过遍历到的 offset 与
fdt_get_property_by_offset_() 函数获得 property，并将
属性值长度存储在 lenp 里。接着调用 fdt_string_eq_()
函数对比 property 属性的名字是否和 name 参数一直，如果一致，
那么将 property 在 DTB DeviceTree Structure 内的偏移
存储在 poffset 里，并返回 property；反之返回 NULL。

> - [fdt_first_property_offset](#A0105)
>
> - [fdt_next_property_offset](#A0106)
>
> - [fdt_get_property_by_offset_](#A0107)
>
> - [fdt_string_eq_](#A0110)
>
> - [fdt32_ld](#A0075)

------------------------------------

#### <span id="A0112">fdt_getprop_namelen</span>

{% highlight c %}
const void *fdt_getprop_namelen(const void *fdt, int nodeoffset,
                                const char *name, int namelen, int *lenp)
{
        int poffset;
        const struct fdt_property *prop;

        prop = fdt_get_property_namelen_(fdt, nodeoffset, name, namelen, lenp,
                                         &poffset);
        if (!prop)
                return NULL;

        /* Handle realignment */
        if (fdt_version(fdt) < 0x10 && (poffset + sizeof(*prop)) % 8 &&
            fdt32_ld(&prop->len) >= 8)
                return prop->data + 4;
        return prop->data;
}
{% endhighlight %}

fdt_getprop_namelen() 函数用于通过名字获得 device-node 内部
属性的值。参数 fdt 指向 DTB，参数 nodeoffset 是节点在 DTB
DeviceTree Structure 区域内的偏移，参数 name 为需要查找属性的
名字，参数 lenp 用于存储属性值的长度。函数首先通过调用
fdt_get_property_namelen_() 函数获得 name 对应的 property，
然后如果当前 DTB 版本小于 17，且 property 属性值长度大于 8，
那么属性值加 4；反之直接返回属性值。

> - [fdt_get_property_namelen_](#A0111)
>
> - [fdt_version](#A0082)
>
> - [fdt32_ld](#A0075)

------------------------------------

#### <span id="A0113">fdt_getprop</span>

{% highlight c %}
const void *fdt_getprop(const void *fdt, int nodeoffset,
                        const char *name, int *lenp)
{
        return fdt_getprop_namelen(fdt, nodeoffset, name, strlen(name), lenp);
}

{% endhighlight %}

fdt_getprop() 函数通过名字获得 device-node 中的属性值。参数 fdt
指向 DTB，参数 nodeoffset 是节点 DTB DeviceTree Structure 区域
内的偏移，name 参数是需要查找的 property 名字，参数 lenp 用于
存储属性值的长度。函数直接通过调用 fdt_getprop_namelen() 函数实现。

> - [fdt_getprop](#A0112)

------------------------------------

#### <span id="A0114">of_compat_cmp</span>

{% highlight c %}
#define of_compat_cmp(s1, s2, l)        strcasecmp((s1), (s2))
{% endhighlight %}

of_compat_cmp() 函数用于对比 device-node 的 compatible
属性字符串是否相同，如果相同则返回 0.

------------------------------------

#### <span id="A0115">of_fdt_is_compatible</span>

{% highlight c %}
/**                 
 * of_fdt_is_compatible - Return true if given node from the given blob has
 * compat in its compatible list
 * @blob: A device tree blob
 * @node: node to test
 * @compat: compatible string to compare with compatible list.
 *      
 * On match, returns a non-zero value with smaller values returned for more
 * specific compatible values.
 */
static int of_fdt_is_compatible(const void *blob,
                      unsigned long node, const char *compat)
{
        const char *cp;
        int cplen;
        unsigned long l, score = 0;

        cp = fdt_getprop(blob, node, "compatible", &cplen);
        if (cp == NULL)
                return 0;
        while (cplen > 0) {
                score++;
                if (of_compat_cmp(cp, compat, strlen(compat)) == 0)
                        return score;
                l = strlen(cp) + 1;
                cp += l;
                cplen -= l;
        }

        return 0;
}
{% endhighlight %}

of_fdt_is_compatible() 函数用于检查节点的 compatible 属性值
是否与参数 compat 相同。参数 blob 指向 DTB，参数 node 代表
device-node 在 DTB DeviceTree Structure 内的偏移值，参数
compat 指向需要查找的节点 compatible。函数首先通过函数
fdt_getprop() 函数读取节点的 "compatible" 属性值，然后在
循环中通过 of_compat_cmp() 对比节点的 "compatible" 属性值
中是否包含参数 compat，知道找到才返回，否则一直循环到属性值
的末尾。那么返回 0 表示没有找到。

> - [fdt_getprop](#A0113)
>
> - [of_compat_cmp](#A0114)

------------------------------------

#### <span id="A0116">of_flat_dt_is_compatible</span>

{% highlight c %}
/**
 * of_flat_dt_is_compatible - Return true if given node has compat in compatible list
 * @node: node to test
 * @compat: compatible string to compare with compatible list.
 */
int __init of_flat_dt_is_compatible(unsigned long node, const char *compat)
{
        return of_fdt_is_compatible(initial_boot_params, node, compat);
}
{% endhighlight %}

of_flat_dt_is_compatible() 函数用于判断节点的 compatible 属性是否
包含 compat 参数。如果包含，则返回 true，反之返回 false。函数通过
调用 of_flat_dt_is_compatible() 函数进行查找。

> - [of_fdt_is_compatible](#A0115)

------------------------------------

#### <span id="A0117">of_get_flat_dt_prop</span>

{% highlight c %}
/**
 * of_get_flat_dt_prop - Given a node in the flat blob, return the property ptr
 *
 * This function can be used within scan_flattened_dt callback to get
 * access to properties
 */
const void *__init of_get_flat_dt_prop(unsigned long node, const char *name,
                                       int *size)
{
        return fdt_getprop(initial_boot_params, node, name, size);
}
{% endhighlight %}

of_get_flat_dt_prop() 函数用于从 DTB 中获得 device-node name 参数
对应的属性值。参数 node 指向 device-node 在 DTB DeviceTree Structure
区域内的偏移，参数 name 指向属性的名字，参数 size 用于存储属性值的长度。
函数通过调用 fdt_getprop() 函数获得属性值。

> - [fdt_getprop](#A0113)

------------------------------------

#### <span id="A0118">of_get_flat_dt_root</span>

{% highlight c %}
/**
 * of_get_flat_dt_root - find the root node in the flat blob
 */
unsigned long __init of_get_flat_dt_root(void)
{
        return 0;
}
{% endhighlight %}

of_get_flat_dt_root() 函数用于获得 DTS 根节点在 DTB
DeviceTree Structure 中的偏移。root 的偏移为 0.

------------------------------------

#### <span id="A0119">arch_get_next_mach</span>

{% highlight c %}
static const void * __init arch_get_next_mach(const char *const **match)
{
        static const struct machine_desc *mdesc = __arch_info_begin;
        const struct machine_desc *m = mdesc;

        if (m >= __arch_info_end)
                return NULL;

        mdesc++;
        *match = m->dt_compat;
        return m;
}
{% endhighlight %}

arch_get_next_mach() 函数用于读取系统下一个 machine_desc
结构体中的 dt_compat 成员，其用于指明 DTB 根节点 compatible
属性值。函数首先获得 __arch_info_begin 指向的 machine_desc
结构，然后将其指向下一个 machine_desc，并从下一个 machine_desc
中读取 dt_compat 成员的值存储到 match 中。内核将 machine_desc
结构都维护到一个 section 内，并使用 __arch_info_begin 指向
区间的起始地址。

------------------------------------

#### <span id="A0120">of_flat_dt_match_machine</span>

{% highlight c %}
/**             
 * of_flat_dt_match_machine - Iterate match tables to find matching machine.
 *                               
 * @default_match: A machine specific ptr to return in case of no match.
 * @get_next_compat: callback function to return next compatible match table.
 *
 * Iterate through machine match tables to find the best match for the machine
 * compatible string in the FDT.
 */
const void * __init of_flat_dt_match_machine(const void *default_match,
                const void * (*get_next_compat)(const char * const**))
{       
        const void *data = NULL;
        const void *best_data = default_match;
        const char *const *compat;
        unsigned long dt_root;
        unsigned int best_score = ~1, score = 0;

        dt_root = of_get_flat_dt_root();
        while ((data = get_next_compat(&compat))) {
                score = of_flat_dt_match(dt_root, compat);
                if (score > 0 && score < best_score) {
                        best_data = data;
                        best_score = score;
                }       
        }       
        if (!best_data) {
                const char *prop;
                int size;

                pr_err("\n unrecognized device tree list:\n[ ");

                prop = of_get_flat_dt_prop(dt_root, "compatible", &size);
                if (prop) {
                        while (size > 0) {
                                printk("'%s' ", prop);
                                size -= strlen(prop) + 1;
                                prop += strlen(prop) + 1;
                        }
                }
                printk("]\n\n");
                return NULL;
        }

        pr_info("Machine model: %s\n", of_flat_dt_get_machine_name());

        return best_data;
}

{% endhighlight %}

of_flat_dt_match_machine() 函数用于对比 DTB 根节点的 compatible
属性值与 machine_desc 数据结构中 dt_compat 成员是否相同。函数首先
通过调用 of_get_flat_dt_root() 函数获得 DTS 根节点在 DTB
DeviceTree Structure 区域内的偏移，然后使用 while() 循环，每次
循环，函数通过调用 get_next_compat() 函数获得 machine 对应的
compatible 属性，在每次遍历中，函数通过调用 of_flat_dt_match()
对比 machine 提供的 compatible 与 DTB 根节点的 compatible 是否
相同，如果找到相同的，那么将 best_data 设置为 data，并将 best_score
设置为 score。如果 best_data 为 0，那么 DTB 不是有效的 DTB，
此时通过 of_get_flat_dt_prop() 获得 DTB 根节点的 compatible，
并打印在错误信息里面，然后返回 NULL。如果 machine 提供的
compatible 与 DTB 根节点的 compatible 一致，那么打印相关
的提示信息，最后返回 best_data.

------------------------------------

#### <span id="A0121">fdt_next_node</span>

{% highlight c %}
int fdt_next_node(const void *fdt, int offset, int *depth)
{
        int nextoffset = 0;
        uint32_t tag;

        if (offset >= 0)
                if ((nextoffset = fdt_check_node_offset_(fdt, offset)) < 0)
                        return nextoffset;

        do {
                offset = nextoffset;
                tag = fdt_next_tag(fdt, offset, &nextoffset);

                switch (tag) {
                case FDT_PROP:
                case FDT_NOP:
                        break;

                case FDT_BEGIN_NODE:
                        if (depth)
                                (*depth)++;
                        break;

                case FDT_END_NODE:
                        if (depth && ((--(*depth)) < 0))
                                return nextoffset;
                        break;

                case FDT_END:
                        if ((nextoffset >= 0)
                            || ((nextoffset == -FDT_ERR_TRUNCATED) && !depth))
                                return -FDT_ERR_NOTFOUND;
                        else
                                return nextoffset;
                }
        } while (tag != FDT_BEGIN_NODE);

        return offset;
}
{% endhighlight %}

fdt_next_node() 函数用于获得当前 device-node 的下一个 device-node。
参数 fdt 指向 DTB，参数 offset 指向当前 device-node 在 DTB
DeviceTree Structure 区域中的偏移，depth 参数用于标识当前节点
的深度。函数首先判断 offset 大于等于 0 是，判断 offset 对应的
的 device-node 是否为有效的 device-node，如果无效，则返回
下一个 tag。如果当前 device-node 合法，那么函数使用 do while
进行循环，每次循环中，函数首先通过 fdt_next_tag() 获得当前 tag
的下一个 tag 以及偏移，如果下一个 tag 的值是 FDT_BEGIN_NODE，
那么当前节点包含了一个子节点，将 depath 值加一，然后继续循环；
如果下一个 tag 是 FDT_END_NODE，那么表示下一个 tag 是一个节点
的结尾，如果此时 depth 存在，并且 depth 此时为零，即不包含任何
子节点，那么函数直接返回下一个 tag 的偏移值；反之表示下一个 tag
是一个子节点结束的位置，那么结束本次循环；如果下一个 tag 的值
是 FDT_END，那么如果 nextoffset 大于零，或者 nextoffset 不是
`-FDT_ERR_TRUNCATED` 且 depth 为零，那么这是一种无效的偏移，
因为 FDT_END 已经是 DTB 的末尾，此时如果 nextoffset 小于 0，
那么函数直接返回 nextoffset。循环直到 tag 的值为 FDT_BEGIN_NODE.

> - [fdt_check_node_offset_](#A0102)
>
> - [fdt_next_tag](#A0099)

------------------------------------

#### <span id="A0122">fdt_get_name</span>

{% highlight c %}
const char *fdt_get_name(const void *fdt, int nodeoffset, int *len)
{
        const struct fdt_node_header *nh = fdt_offset_ptr_(fdt, nodeoffset);
        const char *nameptr;             
        int err;

        if (((err = fdt_ro_probe_(fdt)) != 0)
            || ((err = fdt_check_node_offset_(fdt, nodeoffset)) < 0))
                        goto fail;       

        nameptr = nh->name;

        if (fdt_version(fdt) < 0x10) {
                /*
                 * For old FDT versions, match the naming conventions of V16:
                 * give only the leaf name (after all /). The actual tree
                 * contents are loosely checked.
                 */
                const char *leaf;
                leaf = strrchr(nameptr, '/');
                if (leaf == NULL) {
                        err = -FDT_ERR_BADSTRUCTURE;
                        goto fail;
                }
                nameptr = leaf+1;
        }

        if (len)
                *len = strlen(nameptr);

        return nameptr;

 fail:
        if (len)
                *len = err;
        return NULL;
}
{% endhighlight %}

fdt_get_name() 函数通过节点的偏移获得节点的名字。参数 fdt 指向 DTB，
参数 nodeoffset 指 device-node 在 DTB DeviceTree Structure 区域
内的偏移，参数 len 用于存储名字的长度。函数首先调用 fdt_offset_ptr_()
函数获得参数 nodeoffset 对应的指针，然后调用 fdt_ro_probe_() 函数
对 fdt 进行最小健全性检测，以及调用 fdt_check_offset_() 函数检查
device-node 的合法性，如果上面两个检测其中一个失败，那么函数跳转
到 fail，并返回 NULL；反之，如果检测通过，那么函数将
fdt_node_header 数据结构中的 name 成员读取到 nameptr 中。
函数调用 fdt_version() 获得当前 DTB 的版本，如果当前版本
小于 17，那么所有节点的名字前面都会加上字符 '\', 所以对于这些
版本的名字，函数通过 strrchr 在 nameptr 中找到 `\` 的位置，
然后将 nameptr 指向 `\` 的下一个字节。如果 len 参数指向的空间
有效，那么函数调用 strlen() 函数获得 nameptr 的长度，并存储到
len 指向的空间，最后返回 nameptr。

> - [fdt_offset_ptr_](#A0095)
>
> - [fdt_check_node_offset_](#A0102)

------------------------------------

#### <span id="A0123">of_read_number</span>

{% highlight c %}
/* Helper to read a big number; size is in cells (not bytes) */
static inline u64 of_read_number(const __be32 *cell, int size)
{
        u64 r = 0;
        while (size--)
                r = (r << 32) | be32_to_cpu(*(cell++));
        return r;
}
{% endhighlight %}

of_read_number() 函数的作用就是从大端的属性值中以
小端模式读出 size 个 u32 整数。参数 cell 指向大端数据
的地址，size 参数代表读出 u32 的个数。函数使用 while
循环，每次循环一个 u32，并调用 be32_to_cpu() 函数获得
大端模式中一个 u32 整数，并将获得的整数放在小端地址的
低地址，将原先的数据放在小端地址的高地址上。循环结束
后返回小端模式的属性值。

------------------------------------

#### <span id="A0124">__early_init_dt_declare_initrd</span>

{% highlight c %}
static void __early_init_dt_declare_initrd(unsigned long start,
                                           unsigned long end)
{
        /* ARM64 would cause a BUG to occur here when CONFIG_DEBUG_VM is
         * enabled since __va() is called too early. ARM64 does make use
         * of phys_initrd_start/phys_initrd_size so we can skip this
         * conversion.
         */
        if (!IS_ENABLED(CONFIG_ARM64)) {
                initrd_start = (unsigned long)__va(start);
                initrd_end = (unsigned long)__va(end);
                initrd_below_start_ok = 1;
        }
}
{% endhighlight %}

__early_init_dt_declare_initrd() 函数用于在在系统启动早期，
设置 INITRD 的范围。参数 start 指向 initrd 的起始物理地址，
end 指向 initrd 终止的物理地址。函数首先判断当前内核有没有
定义 CONFIG_ARM64 宏，如果没有定义，那么函数将 initrd_start
指向 start 参数对应的虚拟地址，将 initrd_end 指向 end 参数
对应的虚拟地址，最后将全局变量 initrd_below_start_ok
设置为 1.

------------------------------------

#### <span id="A0125">early_init_dt_check_for_initrd</span>

{% highlight c %}
/**
 * early_init_dt_check_for_initrd - Decode initrd location from flat tree
 * @node: reference to node containing initrd location ('chosen')
 */
static void __init early_init_dt_check_for_initrd(unsigned long node)
{
        u64 start, end;
        int len;
        const __be32 *prop;

        pr_debug("Looking for initrd properties... ");

        prop = of_get_flat_dt_prop(node, "linux,initrd-start", &len);
        if (!prop)
                return;
        start = of_read_number(prop, len/4);

        prop = of_get_flat_dt_prop(node, "linux,initrd-end", &len);
        if (!prop)
                return;
        end = of_read_number(prop, len/4);

        __early_init_dt_declare_initrd(start, end);
        phys_initrd_start = start;
        phys_initrd_size = end - start;

        pr_debug("initrd_start=0x%llx  initrd_end=0x%llx\n",
                 (unsigned long long)start, (unsigned long long)end);
}
{% endhighlight %}

early_init_dt_check_for_initrd() 函数的作用是从 DTB 中读出
INITRD 的信息，并设置 INITRD 相关的全局变量。参数 node 代表
`chosen` 节点在 DTB DeviceTree Struct 区域内的偏移。函数首先
通过 of_get_flat_dt_prop() 函数获得 "linux,initrd-start"
的属性值，如果属性不存在，直接返回；反之调用 of_read_number()
函数将 "linux,initrd-start" 属性值转换成小端模式值。
函数继续调用 of_get_flat_dt_prop() 函数读取 DTB 中
"linux,initrd-end" 的属性值，如果该属性值存在，则调用
of_read_number() 函数将其转换成小端模式，至此，从 DTB 中
获得了 INITRD 的起始物理地址和终止物理地址，函数调用
__early_init_dt_declare_initrd() 函数将 INITRD 的起始
虚拟地址存储到全局变量 initrd_start，将 INITRD 的终止
虚拟地址存储到全局变量 initrd_end。最后将 INITRD 起始
物理地址存储到全局变量 phys_initrd_start, 并将 INITRD
的长度存储到 phys_initrd_size 里。

> - [of_get_flat_dt_prop](#A0117)
>
> - [of_read_number](#A0123)
>
> - [\_\_early_init_dt_declare_initrd](#A0124)

------------------------------------

#### <span id="A0126">early_init_dt_scan_chosen</span>

{% highlight c %}
int __init early_init_dt_scan_chosen(unsigned long node, const char *uname,
                                     int depth, void *data)
{
        int l;
        const char *p;

        pr_debug("search \"chosen\", depth: %d, uname: %s\n", depth, uname);

        if (depth != 1 || !data ||
            (strcmp(uname, "chosen") != 0 && strcmp(uname, "chosen@0") != 0))
                return 0;

        early_init_dt_check_for_initrd(node);

        /* Retrieve command line */
        p = of_get_flat_dt_prop(node, "bootargs", &l);
        if (p != NULL && l > 0)
                strlcpy(data, p, min((int)l, COMMAND_LINE_SIZE));

        /*
         * CONFIG_CMDLINE is meant to be a default in case nothing else
         * managed to set the command line, unless CONFIG_CMDLINE_FORCE
         * is set in which case we override whatever was found earlier.
         */     
#ifdef CONFIG_CMDLINE
#if defined(CONFIG_CMDLINE_EXTEND)
        strlcat(data, " ", COMMAND_LINE_SIZE);
        strlcat(data, CONFIG_CMDLINE, COMMAND_LINE_SIZE);
#elif defined(CONFIG_CMDLINE_FORCE)
        strlcpy(data, CONFIG_CMDLINE, COMMAND_LINE_SIZE);
#else                   
        /* No arguments from boot loader, use kernel's  cmdl*/
        if (!((char *)data)[0])
                strlcpy(data, CONFIG_CMDLINE, COMMAND_LINE_SIZE);
#endif  
#endif /* CONFIG_CMDLINE */

        pr_debug("Command line is: %s\n", (char*)data);

        /* break now */
        return 1;
}
{% endhighlight %}

early_init_dt_scan_chosen() 函数用于从 DTB 的 chosen 节点中
读取 bootargs 参数并与内核提供的 CMDLINE，最终合成系统启动时
需要的 cmdline。参数 node 代表 DTB 中 chosen 节点在 DTB
DeviceTree Structure 区域内的偏移，参数 uname 为 chosen
节点的节点名字；参数 depth 指向 chosen 子节点的前套数，
参数 data 指向 cmdline。函数首先检查 depth， data 参数
的有效性，以及当 uname 是否为 "chosen" 与 "chosen@0",
如果无效，则返回；如果检查通过，那么函数调用
early_init_dt_check_for_initrd() 函数从 DTB 的 chosen
节点中，读取并设置 INITRD 相关的数据。接着调用
of_get_flat_dt_prop() 读取 chosen 节点的 "bootargs"
属性值，如果该属性存在，那么将属性值拷贝到 data 指向的空间。
此时函数通过宏进行不同的处理，如果此时定义了 CONFIG_CMDLINE，
那么内核配置中存在一些 cmdline 数据，此时如果宏
CONFIG_CMDLINE_EXTEND 定义，那么函数将内核配置中的
cmdline 拼接到 data 后面，此时 cmdline 就包含了 DTB 和 kernel
提供的 cmdline；反之，如果此时宏 CONFIG_CMDLINE_FORCE
定义，那么函数丢弃 DTB 中的 cmdline 而只保留 kernel
提供的 cmdline。

> - [early_init_dt_check_for_initrd](#A0125)
>
> - [of_get_flat_dt_prop](#A0117)

------------------------------------

#### <span id="A0127">of_scan_flat_dt</span>

{% highlight c %}
/**
 * of_scan_flat_dt - scan flattened tree blob and call callback on each.
 * @it: callback function
 * @data: context data pointer
 *
 * This function is used to scan the flattened device-tree, it is
 * used to extract the memory information at boot before we can
 * unflatten the tree
 */
int __init of_scan_flat_dt(int (*it)(unsigned long node,
                                     const char *uname, int depth,
                                     void *data),
                           void *data)
{
        const void *blob = initial_boot_params;
        const char *pathp;
        int offset, rc = 0, depth = -1;

        if (!blob)
                return 0;

        for (offset = fdt_next_node(blob, -1, &depth);
             offset >= 0 && depth >= 0 && !rc;
             offset = fdt_next_node(blob, offset, &depth)) {

                pathp = fdt_get_name(blob, offset, NULL);
                if (*pathp == '/')
                        pathp = kbasename(pathp);
                rc = it(offset, pathp, depth, data);
        }
        return rc;
}
{% endhighlight %}

of_scan_flat_dt() 函数用于遍历 DTB 中所有节点，
节点以其在 DTB DeviceTree Structure 区域内的偏移给出，
函数将每次遍历的到的节点传递给函数 it() 执行。参数 node
为开始查找的节点偏移值，参数 uname 为需要查找节点的名字，
参数 depth 代表嵌套节点的深度；参数 data 用于传递私有数据。
函数使用 for 循环遍历从 node 参数开始的节点，函数每次通过
fdt_next_node() 函数获得下一个节点的偏移值，只要下一个节点
的偏移值不小于 0，且 depth 不小于 0，且 rc 等于 0，那么
循环继续执行，每次遍历一个节点的时候，函数首先调用
fdt_get_name() 函数获得节点对应的名字，如果节点名字
第一个字符是 `/`, 那么函数调用 kbasename() 函数将第一个
字符从名字中去除。接着函数将获得的节点偏移，和节点名字等信息
传递给 it() 函数。

> - [fdt_next_node](#A0121)
>
> - [fdt_get_name](#A0122)

------------------------------------

#### <span id="A0128">early_init_dt_scan_root</span>

{% highlight c %}
/**
 * early_init_dt_scan_root - fetch the top level address and size cells
 */
int __init early_init_dt_scan_root(unsigned long node, const char *uname,
                                   int depth, void *data)
{
        const __be32 *prop;

        if (depth != 0)
                return 0;

        dt_root_size_cells = OF_ROOT_NODE_SIZE_CELLS_DEFAULT;
        dt_root_addr_cells = OF_ROOT_NODE_ADDR_CELLS_DEFAULT;

        prop = of_get_flat_dt_prop(node, "#size-cells", NULL);
        if (prop)
                dt_root_size_cells = be32_to_cpup(prop);
        pr_debug("dt_root_size_cells = %x\n", dt_root_size_cells);

        prop = of_get_flat_dt_prop(node, "#address-cells", NULL);
        if (prop)
                dt_root_addr_cells = be32_to_cpup(prop);
        pr_debug("dt_root_addr_cells = %x\n", dt_root_addr_cells);

        /* break now */
        return 1;
}
{% endhighlight %}

early_init_dt_scan_root() 函数用于获得根节点 `#size_cells` 与
`#address-cells` 对应的属性值。参数 node 指向节点的偏移，参数 uname
指向节点的名字，参数 depth 代表节点的深度，参数 data 代表
私有数据。函数首先判断 depth 的值如果不为 0，那么直接返回，因此
此时针对的节点是根节点，根节点不可能嵌套在其他节点之下。
函数首先接着将全局变量 dt_root_size_cells 与 dt_root_addr_cells
设置为默认值。接着函数在根节点中查找 `#size-cells` 属性，如果存在，
将该属性值赋值到 dt_root_size_cells 里面，同理在根节点中查找
`#address-cells` 属性，如果该属性存在，那么将属性值存储到
dt_root_addr_cells 里面，最后返回 1.

> - [of_get_flat_dt_prop](#A0117)

------------------------------------

#### <span id="A0129">dt_mem_next_cell</span>

{% highlight c %}
u64 __init dt_mem_next_cell(int s, const __be32 **cellp)
{
        const __be32 *p = *cellp;

        *cellp = p + s;
        return of_read_number(p, s);
}
{% endhighlight %}

dt_mem_next_cell() 函数用于从 DTB memory 节点中读取
reg 属性的值。函数通过调用 of_read_number() 获得对应的
属性值。

> - [of_read_number](#A0123)

------------------------------------

#### <span id="A0130">early_init_dt_add_memory_arch</span>

{% highlight c %}
void __init __weak early_init_dt_add_memory_arch(u64 base, u64 size)
{
        const u64 phys_offset = MIN_MEMBLOCK_ADDR;

        if (size < PAGE_SIZE - (base & ~PAGE_MASK)) {
                pr_warn("Ignoring memory block 0x%llx - 0x%llx\n",
                        base, base + size);
                return;
        }

        if (!PAGE_ALIGNED(base)) {
                size -= PAGE_SIZE - (base & ~PAGE_MASK);
                base = PAGE_ALIGN(base);
        }
        size &= PAGE_MASK;

        if (base > MAX_MEMBLOCK_ADDR) {
                pr_warning("Ignoring memory block 0x%llx - 0x%llx\n",
                                base, base + size);
                return;
        }

        if (base + size - 1 > MAX_MEMBLOCK_ADDR) {
                pr_warning("Ignoring memory range 0x%llx - 0x%llx\n",
                                ((u64)MAX_MEMBLOCK_ADDR) + 1, base + size);
                size = MAX_MEMBLOCK_ADDR - base + 1;
        }

        if (base + size < phys_offset) {
                pr_warning("Ignoring memory block 0x%llx - 0x%llx\n",
                           base, base + size);
                return;
        }
        if (base < phys_offset) {
                pr_warning("Ignoring memory range 0x%llx - 0x%llx\n",
                           base, phys_offset);
                size -= phys_offset - base;
                base = phys_offset;
        }
        memblock_add(base, size);
}
{% endhighlight %}

early_init_dt_add_memory_arch() 函数用于从 DTB 中获得内存的
基地址和长度之后，进行对齐和检测之后，将基地址对应长度的物理内存
信息添加到 MEMBLOCK 内存管理器里。参数 base 指向物理内存的基地址，
size 参数指向物理内存的长度。函数首先将基地址和长度与 MEMBLOCK
管理的最大与最小物理内存信息做检测，然后进行页对齐检测，如果上面
的检测中，如果检查不通过，那么就修改基地址与长度的值，如果修改
之后还是无法通过，那么直接返回。如果上面的检测都通过，那么
函数直接调用 memblock_add() 函数将物理内存信息更新到
MEMBLOCK 内存管理器里。

> - [memblock_add](#A0166)

------------------------------------

#### <span id="A0131">early_init_dt_scan_memory</span>

{% highlight c %}
/**
 * early_init_dt_scan_memory - Look for and parse memory nodes
 */
int __init early_init_dt_scan_memory(unsigned long node, const char *uname,
                                     int depth, void *data)
{
        const char *type = of_get_flat_dt_prop(node, "device_type", NULL);
        const __be32 *reg, *endp;
        int l;
        bool hotpluggable;

        /* We are scanning "memory" nodes only */
        if (type == NULL || strcmp(type, "memory") != 0)
                return 0;

        reg = of_get_flat_dt_prop(node, "linux,usable-memory", &l);
        if (reg == NULL)
                reg = of_get_flat_dt_prop(node, "reg", &l);
        if (reg == NULL)
                return 0;

        endp = reg + (l / sizeof(__be32));
        hotpluggable = of_get_flat_dt_prop(node, "hotpluggable", NULL);

        pr_debug("memory scan node %s, reg size %d,\n", uname, l);

        while ((endp - reg) >= (dt_root_addr_cells + dt_root_size_cells)) {
                u64 base, size;

                base = dt_mem_next_cell(dt_root_addr_cells, &reg);
                size = dt_mem_next_cell(dt_root_size_cells, &reg);

                if (size == 0)
                        continue;
                pr_debug(" - %llx ,  %llx\n", (unsigned long long)base,
                    (unsigned long long)size);

                early_init_dt_add_memory_arch(base, size);

                if (!hotpluggable)
                        continue;

                if (early_init_dt_mark_hotplug_memory_arch(base, size))
                        pr_warn("failed to mark hotplug range 0x%llx - 0x%llx\n",
                                base, base + size);
        }

        return 0;
}
{% endhighlight %}

early_init_dt_scan_memory() 函数用于获得 DTB memory 节点的信息，即
物理内存的信息，并将物理内存的信息更新到 MEMBLOCK 内存分配器中。
参数 node 指向节点的偏移，uname 指向节点的名字，参数 depth 代表
节点的深度，data 指向私有数据。函数首先通过 of_get_flat_dt_prop()
函数获得当前节点的 "device_type" 属性值，如果该属性不存在，或者
该属性值不为 "memory", 那么函数直接返回 0；如果检查到的属性符合要求，
那么当前节点就是 "memory", 函数调用 of_get_flat_dt_prop() 函数从
"memory" 节点中读取 "linux,usable-memory" 属性的值，如果该值不存在，
那么函数读取 "memory" 节点的 "reg" 属性值，如果属性值还是不存在，
那么函数直接返回 0；如果上诉条件都满足，那么函数将 endp 指向属性
值末尾的地方。函数接着从 "memory" 节点中读取 "hotplugable" 属性
值。函数接着使用一个 while 循环，循环只要 "endp-reg" 的值大于
"dt_root_addr_cells+dt_root_size_cells" 的值，那么 while
循环继续循环。每次循环过程中，函数调用 dt_mem_next_cell()
函数读取 reg 属性里面的信息，以此获得物理内存的起始地址与
长度信息。如果此时 size 的值为 0，那么结束本次循环，继续下一次
循环。函数调用 early_init_dt_add_memory_arch() 函数将
获得物理内存信息更新到 MEMBLOCK 内存管理器。此时如果
hotpluggable 为零，即系统不支持热插拔的内存条，则直接
结束本次循环。否则调用 early_init_dt_mark_hotplug_memory_arch()
函数进行处理。

> - [of_get_flat_dt_prop](#A0117)
>
> - [dt_mem_next_cell](#A0129)
>
> - [early_init_dt_add_memory_arch](#A0130)

------------------------------------

#### <span id="A0132">setup_machine_fdt</span>

{% highlight c %}
/**
 * setup_machine_fdt - Machine setup when an dtb was passed to the kernel
 * @dt_phys: physical address of dt blob
 *
 * If a dtb was passed to the kernel in r2, then use it to choose the
 * correct machine_desc and to setup the system.
 */
const struct machine_desc * __init setup_machine_fdt(unsigned int dt_phys)
{
        const struct machine_desc *mdesc, *mdesc_best = NULL;

#if defined(CONFIG_ARCH_MULTIPLATFORM) || defined(CONFIG_ARM_SINGLE_ARMV7M)
        DT_MACHINE_START(GENERIC_DT, "Generic DT based system")
                .l2c_aux_val = 0x0,
                .l2c_aux_mask = ~0x0,
        MACHINE_END

        mdesc_best = &__mach_desc_GENERIC_DT;
#endif

        if (!dt_phys || !early_init_dt_verify(phys_to_virt(dt_phys)))
                return NULL;

        mdesc = of_flat_dt_match_machine(mdesc_best, arch_get_next_mach);

        if (!mdesc) {
                const char *prop;
                int size;
                unsigned long dt_root;

                early_print("\nError: unrecognized/unsupported "
                            "device tree compatible list:\n[ ");

                dt_root = of_get_flat_dt_root();
                prop = of_get_flat_dt_prop(dt_root, "compatible", &size);
                while (size > 0) {
                        early_print("'%s' ", prop);
                        size -= strlen(prop) + 1;
                        prop += strlen(prop) + 1;
                }
                early_print("]\n\n");

                dump_machine_table(); /* does not return */
        }

        /* We really don't want to do this, but sometimes firmware provides buggy data */
        if (mdesc->dt_fixup)
                mdesc->dt_fixup();

        early_init_dt_scan_nodes();

        /* Change machine number to match the mdesc we're using */
        __machine_arch_type = mdesc->nr;

        return mdesc;
}
{% endhighlight %}

setup_machine_fdt() 用于获得系统的 DTB，并对 DTB 的合法性和有效性
做检测，检测通过后，从 DTB 中获得内核初始化相关的内存信息和 cmdline
信息。参数 dt_phys 指向 DTB 所在的物理地址。函数首先定义了一个
machine_desc 结构，然后调用 early_init_dt_verify() 函数检查 DTB
的有效性，如果 DTB 的物理地址无效，或者 DTB 二进制文件无效，那么
函数直接返回 NULL。函数继续调用 of_flat_dt_match_machine() 函数
获得 DTB 根节点的 compatible 信息与 machine_desc 里给出的 dt_compat
信息是否一致，如果遍历了所有的 machine_desc 都不能找到一致的信息，
那么内核此时就报出一些错误信息。如果检测通过，那么如果 mdesc 的 dt_fixup()
函数存在，那么执行 dt_fixup() 函数。函数接着执行 early_init_dt_scan_nodes()
函数，该函数从 DTB 中读取 cmdline 信息，并设置全局的 CMDLINE 信息，
函数还从 DTB 中获得物理内存的基本信息，并将信息更新到 MEMBLOCK
内存器里。

> - [early_init_dt_verify](#A0093)
>
> - [of_get_flat_dt_root](#A0118)
>
> - [early_init_dt_scan_nodes](#A0133)

------------------------------------

#### <span id="A0133">early_init_dt_scan_nodes</span>

{% highlight c %}
void __init early_init_dt_scan_nodes(void)
{               
        int rc = 0;                    

        /* Retrieve various information from the /chosen node */
        rc = of_scan_flat_dt(early_init_dt_scan_chosen, boot_command_line);
        if (!rc)
                pr_warn("No chosen node found, continuing without\n");

        /* Initialize {size,address}-cells info */
        of_scan_flat_dt(early_init_dt_scan_root, NULL);

        /* Setup memory, calling early_init_dt_add_memory_arch */
        of_scan_flat_dt(early_init_dt_scan_memory, NULL);
}
{% endhighlight %}

early_init_dt_scan_nodes() 函数用于从 DTB 总获得 cmdline 信息，
以及物理内存信息。函数首先调用 "of_scan_flat_dt(early_init_dt_scan_chosen, boot_command_line)"
获得 cmdline 信息，接着调用 "of_scan_flat_dt(early_init_dt_scan_root, NULL)"
函数获得根节点 "#address-cells" 与 "#size-cells" 信息，最后
调用 "of_scan_flat_dt(early_init_dt_scan_memory, NULL)" 函数
获得物理内存信息。

> - [of_scan_flat_dt](#A0127)
>
> - [early_init_dt_scan_chosen](#A0126)
>
> - [early_init_dt_scan_root](#A0128)
>
> - [early_init_dt_scan_memory](#A0131)

------------------------------------

#### <span id="A0134">for_each_memblock_type</span>

{% highlight c %}
#define for_each_memblock_type(i, memblock_type, rgn)                   \
        for (i = 0, rgn = &memblock_type->regions[0];                   \
             i < memblock_type->cnt;                                    \
             i++, rgn = &memblock_type->regions[i])
{% endhighlight %}

for_each_memblock_type() 函数用于遍历 MEMBLOCK 指定内存区的
所有 region。参数 i 指 region 索引，参数 memblock_type 指向
MEMBLOCK region 的类型，参数 rgn 指向特定的 region。函数通过
使用 for 循环，从指定的 MEMBLOCK 类型的第一块 region 开始，
遍历所有的 region，直到遍历完该类型的 region。

------------------------------------

#### <span id="A0135">memblock_set_region_node</span>

{% highlight c %}
static inline void memblock_set_region_node(struct memblock_region *r, int nid)
{
        r->nid = nid;
}
{% endhighlight %}

memblock_set_region_node() 函数用于设置 memblock_region
的 nid 信息，指定 NUMA ID。

------------------------------------

#### <span id="A0136">memblock_get_region_node</span>

{% highlight c %}
static inline int memblock_get_region_node(const struct memblock_region *r)
{
        return r->nid;
}
{% endhighlight %}

memblock_get_region_node() 函数用于获得当前 memblock_region
所在的 NUMA CPU 信息。

------------------------------------

#### <span id="A0137">memblock_insert_region</span>

{% highlight c %}
/**
 * memblock_insert_region - insert new memblock region
 * @type:       memblock type to insert into
 * @idx:        index for the insertion point
 * @base:       base address of the new region
 * @size:       size of the new region
 * @nid:        node id of the new region
 * @flags:      flags of the new region
 *
 * Insert new memblock region [@base, @base + @size) into @type at @idx.
 * @type must already have extra room to accommodate the new region.
 */
static void __init_memblock memblock_insert_region(struct memblock_type *type,
                                                   int idx, phys_addr_t base,
                                                   phys_addr_t size,
                                                   int nid,
                                                   enum memblock_flags flags)
{
        struct memblock_region *rgn = &type->regions[idx];

        BUG_ON(type->cnt >= type->max);
        memmove(rgn + 1, rgn, (type->cnt - idx) * sizeof(*rgn));
        rgn->base = base;
        rgn->size = size;
        rgn->flags = flags;
        memblock_set_region_node(rgn, nid);
        type->cnt++;
        type->total_size += size;
}
{% endhighlight %}

memblock_insert_region() 函数用于将一个新的 memblock_region
插入到 MEMBLOCK 指定类型里。参数 memblock_type 指定插入的类型，
参数 idx 为插入的位置，base 参数为 region 的基地址，size 为 region
的长度，参数 nid 指向 region 所在的 NUMA 上，参数 falgs 指向
region 的 flags 信息。函数首先 idx 参数获得 type 类型的物理内存
的 region 块，如果该类型物理内存包含的 region 数大于或等于
该类型物理内存能包含最大 region 数，那么函数通过 BUG_ON()
函数进行报错。函数继续通过 memmove() 函数将 rgn 对应的 region
都拷贝到 rgn+1 下一个 region 的位置，并确保重叠数据在覆盖之前，
memmove() 函数都能正确拷贝，这样 memblock_type regions 成员
的 rgn 位置空闲出一个位置，然后将该 region 用于给新的 region
使用，设置 rgn 的 base，size，flags 与参数一致，最后增加该
类型物理内存 regions 的数量和总物理内存数。

------------------------------------

#### <span id="A0138">memblock_bottom_up</span>

{% highlight c %}
/*
 * Check if the allocation direction is bottom-up or not.
 * if this is true, that said, memblock will allocate memory
 * in bottom-up direction.
 */             
static inline bool memblock_bottom_up(void)
{
        return memblock.bottom_up;
}
{% endhighlight %}

memblock_bottom_up() 函数用于获得当前 MEMBLOCK 分配的方向。
如果 memblock.bottom_up 为 true，那么 MEMBLOCK 从低地址
向高地址分配；反之，如果 memblock.bottom_up 为 false，那么
MEMBLOCK 从高地址向低地址分配。

------------------------------------

#### <span id="A0139">__next__mem_range</span>

{% highlight c %}
/**
 * __next__mem_range - next function for for_each_free_mem_range() etc.
 * @idx: pointer to u64 loop variable
 * @nid: node selector, %NUMA_NO_NODE for all nodes
 * @flags: pick from blocks based on memory attributes
 * @type_a: pointer to memblock_type from where the range is taken
 * @type_b: pointer to memblock_type which excludes memory from being taken
 * @out_start: ptr to phys_addr_t for start address of the range, can be %NULL
 * @out_end: ptr to phys_addr_t for end address of the range, can be %NULL
 * @out_nid: ptr to int for nid of the range, can be %NULL
 *
 * Find the first area from *@idx which matches @nid, fill the out
 * parameters, and update *@idx for the next iteration.  The lower 32bit of
 * *@idx contains index into type_a and the upper 32bit indexes the
 * areas before each region in type_b.  For example, if type_b regions
 * look like the following,
 *
 *      0:[0-16), 1:[32-48), 2:[128-130)
 *
 * The upper 32bit indexes the following regions.
 *
 *      0:[0-0), 1:[16-32), 2:[48-128), 3:[130-MAX)
 *
 * As both region arrays are sorted, the function advances the two indices
 * in lockstep and returns each intersection.
 */
void __init_memblock __next_mem_range(u64 *idx, int nid,
                                      enum memblock_flags flags,
                                      struct memblock_type *type_a,
                                      struct memblock_type *type_b,
                                      phys_addr_t *out_start,
                                      phys_addr_t *out_end, int *out_nid)
{
        int idx_a = *idx & 0xffffffff;
        int idx_b = *idx >> 32;

        if (WARN_ONCE(nid == MAX_NUMNODES,
        "Usage of MAX_NUMNODES is deprecated. Use NUMA_NO_NODE instead\n"))
                nid = NUMA_NO_NODE;

        for (; idx_a < type_a->cnt; idx_a++) {
                struct memblock_region *m = &type_a->regions[idx_a];

                phys_addr_t m_start = m->base;
                phys_addr_t m_end = m->base + m->size;
                int         m_nid = memblock_get_region_node(m);

                /* only memory regions are associated with nodes, check it */
                if (nid != NUMA_NO_NODE && nid != m_nid)
                        continue;

                /* skip hotpluggable memory regions if needed */
                if (movable_node_is_enabled() && memblock_is_hotpluggable(m))
                        continue;

                /* if we want mirror memory skip non-mirror memory regions */
                if ((flags & MEMBLOCK_MIRROR) && !memblock_is_mirror(m))
                        continue;

                /* skip nomap memory unless we were asked for it explicitly */
                if (!(flags & MEMBLOCK_NOMAP) && memblock_is_nomap(m))
                        continue;

                if (!type_b) {
                        if (out_start)
                                *out_start = m_start;
                        if (out_end)
                                *out_end = m_end;
                        if (out_nid)
                                *out_nid = m_nid;
                        idx_a++;
                        *idx = (u32)idx_a | (u64)idx_b << 32;
                        return;
                }

                /* scan areas before each reservation */
                for (; idx_b < type_b->cnt + 1; idx_b++) {
                        struct memblock_region *r;
                        phys_addr_t r_start;
                        phys_addr_t r_end;

                        r = &type_b->regions[idx_b];
                        r_start = idx_b ? r[-1].base + r[-1].size : 0;
                        r_end = idx_b < type_b->cnt ?
                                r->base : PHYS_ADDR_MAX;

                        /*
                         * if idx_b advanced past idx_a,
                         * break out to advance idx_a
                         */
                        if (r_start >= m_end)
                                break;
                        /* if the two regions intersect, we're done */
                        if (m_start < r_end) {
                                if (out_start)
                                        *out_start =
                                                max(m_start, r_start);
                                if (out_end)
                                        *out_end = min(m_end, r_end);
                                if (out_nid)
                                        *out_nid = m_nid;
                                /*
                                 * The region which ends first is
                                 * advanced for the next iteration.
                                 */
                                if (m_end <= r_end)
                                        idx_a++;
                                else
                                        idx_b++;
                                *idx = (u32)idx_a | (u64)idx_b << 32;
                                return;
                        }
                }
        }

        /* signal end of iteration */
        *idx = ULLONG_MAX;
}
{% endhighlight %}

__next__mem_range() 函数的作用是在指定 regions 内找到
一块空闲的 region。由于函数太长，分段解析。

{% highlight c %}
void __init_memblock __next_mem_range(u64 *idx, int nid,
                                      enum memblock_flags flags,
                                      struct memblock_type *type_a,
                                      struct memblock_type *type_b,
                                      phys_addr_t *out_start,
                                      phys_addr_t *out_end, int *out_nid)
{
        int idx_a = *idx & 0xffffffff;
        int idx_b = *idx >> 32;

        if (WARN_ONCE(nid == MAX_NUMNODES,
        "Usage of MAX_NUMNODES is deprecated. Use NUMA_NO_NODE instead\n"))
                nid = NUMA_NO_NODE;
{% endhighlight %}

参数 idx 用于存储在 memblock_type regions 中的索引，参数 nid 指向
NUMA 信息，参数 flags 指向 regions 的 flags 信息，参数 type_a，type_b
指向特定的 memblock_type, out_start 参数指向找到物理地址的起始地址，
out_end 参数指向找到物理地址的终止地址。参数 nid 存储找到 NID 信息。
函数将 64 位的 idx 拆分成高 32 位的 idx_b 与低 32 位的 idx_a. 如果
nid 等于 MAX_NUMNODES, 那么函数将报错。

{% highlight c %}
        for (; idx_a < type_a->cnt; idx_a++) {
                struct memblock_region *m = &type_a->regions[idx_a];

                phys_addr_t m_start = m->base;
                phys_addr_t m_end = m->base + m->size;
                int         m_nid = memblock_get_region_node(m);

                /* only memory regions are associated with nodes, check it */
                if (nid != NUMA_NO_NODE && nid != m_nid)
                        continue;

                /* skip hotpluggable memory regions if needed */
                if (movable_node_is_enabled() && memblock_is_hotpluggable(m))
                        continue;

                /* if we want mirror memory skip non-mirror memory regions */
                if ((flags & MEMBLOCK_MIRROR) && !memblock_is_mirror(m))
                        continue;

                /* skip nomap memory unless we were asked for it explicitly */
                if (!(flags & MEMBLOCK_NOMAP) && memblock_is_nomap(m))
                        continue;

                if (!type_b) {
                        if (out_start)
                                *out_start = m_start;
                        if (out_end)
                                *out_end = m_end;
                        if (out_nid)
                                *out_nid = m_nid;
                        idx_a++;
                        *idx = (u32)idx_a | (u64)idx_b << 32;
                        return;
                }
{% endhighlight %}

函数使用 for 循环，在 type_a 对应的物理区内遍历所有的 region，
每次遍历到一个 memblock_region, 将该 memblock_region 的起始物理
地址存储在局部变量 m_start, 终止物理地址存储在局部遍历 m_end 中，
并从当前 region 中，通过函数 memblock_get_region_node() 获得
NUMA 信息。接下来函数对刚获得的三个数据进行检测，首先检查 NUMA
信息是否符合要求，如果 nid 即不等于 NUMA_NO_NODE，也不等于 m_nid,
那么函数直接调用 continue；函数通过 movable_node_is_enabled()
函数检测热插拔节点功能是否支持，以及 memblock_is_hotpluggable()
确定当前的 region 是否属于热插拔，如果属于，那么调用 continue；
如果此时 flags 参数中没有包含 MEMBLOCK_NOMAP，但 memblock_is_nomap()
函数返回为 true，那么不符合条件，直接返回。如果此时 type_b 参数
为空，那么函数将检测通过 region 信息存储到参数 out_* 中，增加
idx_a 的值，并更新 idx 的信息后返回。

{% highlight c %}
                /* scan areas before each reservation */
                for (; idx_b < type_b->cnt + 1; idx_b++) {
                        struct memblock_region *r;
                        phys_addr_t r_start;
                        phys_addr_t r_end;

                        r = &type_b->regions[idx_b];
                        r_start = idx_b ? r[-1].base + r[-1].size : 0;
                        r_end = idx_b < type_b->cnt ?
                                r->base : PHYS_ADDR_MAX;

                        /*
                         * if idx_b advanced past idx_a,
                         * break out to advance idx_a
                         */
                        if (r_start >= m_end)
                                break;
                        /* if the two regions intersect, we're done */
                        if (m_start < r_end) {
                                if (out_start)
                                        *out_start =
                                                max(m_start, r_start);
                                if (out_end)
                                        *out_end = min(m_end, r_end);
                                if (out_nid)
                                        *out_nid = m_nid;
                                /*
                                 * The region which ends first is
                                 * advanced for the next iteration.
                                 */
                                if (m_end <= r_end)
                                        idx_a++;
                                else
                                        idx_b++;
                                *idx = (u32)idx_a | (u64)idx_b << 32;
                                return;
                        }
                }
        }
{% endhighlight %}

如果此时 type_b 也不为空，那么函数使用一个 for 循环，遍历 type_b
中的 region， idx_b 作为索引遍历 regions，每次循环中，函数都会将
r_start 指向上一块的结束地址，然后将 r_end 指向当前 region 的结束
地址。如果 r_start 的值大于等于 r_end, 那么没有找到满足需求的
region，那么直接返回。函数继续执行，如果 m_start < r_end, 那么
可以从该区域内找到一块空闲的物理内存，其范围从 m_start 与
r_start 之间最大的地址开始，到 m_end 与 r_end 之间最小的地址
停止，最终将找到的地址存储到参数中。在函数返回前更新 idx 的值。

> - [movable_node_is_enabled](#A0140)
>
> - [memblock_is_hotpluggable](#A0141)
>
> - [memblock_is_mirror](#A0142)
>
> - [memblock_is_nomap](#A0143)
>
> - [\_\_next_mem_range 内核实践](https://biscuitos.github.io/blog/MMU-ARM32-MEMBLOCK-__next_mem_range/)

------------------------------------

#### <span id="A0140">movable_node_is_enabled</span>

{% highlight c %}
static inline bool movable_node_is_enabled(void)
{
        return movable_node_enabled;
}
{% endhighlight %}

movable_node_is_enabled() 函数判断 movable_node_enabled
是否使能，如果为 true，那么 movable_node_enabled 使能。

------------------------------------

#### <span id="A0141">memblock_is_hotpluggable</span>

{% highlight c %}
static inline bool memblock_is_hotpluggable(struct memblock_region *m)
{
        return m->flags & MEMBLOCK_HOTPLUG;
}
{% endhighlight %}

memblock_is_hotpluggable() 函数判断当前 region 是否
具有热插拔属性，如果有则返回 true；反之返回 false。

------------------------------------

#### <span id="A0142">memblock_is_mirror</span>

{% highlight c %}
static inline bool memblock_is_mirror(struct memblock_region *m)
{
        return m->flags & MEMBLOCK_MIRROR;
}
{% endhighlight %}

memblock_is_mirror() 判断当前 regions 是否是 MIRROR。
如果是则返回 true；反之返回 false

------------------------------------

#### <span id="A0143">memblock_is_nomap</span>

{% highlight c %}
static inline bool memblock_is_nomap(struct memblock_region *m)
{
        return m->flags & MEMBLOCK_NOMAP;
}
{% endhighlight %}

memblock_is_nomap() 函数判断当前 region 是否不需要
映射页表，如果是则返回 true，反之返回 false。

------------------------------------

#### <span id="A0144">for_each_mem_range</span>

{% highlight c %}
/**             
 * for_each_mem_range - iterate through memblock areas from type_a and not
 * included in type_b. Or just type_a if type_b is NULL.
 * @i: u64 used as loop variable
 * @type_a: ptr to memblock_type to iterate
 * @type_b: ptr to memblock_type which excludes from the iteration
 * @nid: node selector, %NUMA_NO_NODE for all nodes
 * @flags: pick from blocks based on memory attributes
 * @p_start: ptr to phys_addr_t for start address of the range, can be %NULL
 * @p_end: ptr to phys_addr_t for end address of the range, can be %NULL
 * @p_nid: ptr to int for nid of the range, can be %NULL
 */                     
#define for_each_mem_range(i, type_a, type_b, nid, flags,               \
                           p_start, p_end, p_nid)                       \
        for (i = 0, __next_mem_range(&i, nid, flags, type_a, type_b,    \
                                     p_start, p_end, p_nid);            \
             i != (u64)ULLONG_MAX;                                      \
             __next_mem_range(&i, nid, flags, type_a, type_b,           \
                              p_start, p_end, p_nid))
{% endhighlight %}

for_each_mem_range() 函数的作用就是遍历指定范围内的所有 region。
参数 i 用于遍历索引，参数 type_a 指向特定的 regions，同理参数 type_b
也指向特定的 regions，nid 参数存储 NUMA 信息，flags 参数包含 region
相关的 flags。函数通过 for 循环，i 从 0 开始，调用 __next_mem_range()
函数遍历 regions 内的所有 region。知道 i 等于 ULLONG_MAX 停止。

> - [\_\_next_mem_range](#A0139)
>
> - [\_\_next_mem_range 内核实践](https://biscuitos.github.io/blog/MMU-ARM32-MEMBLOCK-for_each_mem_range/)

------------------------------------

#### <span id="A0145">for_each_free_mem_range</span>

{% highlight c %}
/**
 * for_each_free_mem_range - iterate through free memblock areas
 * @i: u64 used as loop variable
 * @nid: node selector, %NUMA_NO_NODE for all nodes
 * @flags: pick from blocks based on memory attributes
 * @p_start: ptr to phys_addr_t for start address of the range, can be %NULL
 * @p_end: ptr to phys_addr_t for end address of the range, can be %NULL
 * @p_nid: ptr to int for nid of the range, can be %NULL
 *
 * Walks over free (memory && !reserved) areas of memblock.  Available as
 * soon as memblock is initialized.
 */
#define for_each_free_mem_range(i, nid, flags, p_start, p_end, p_nid)   \
        for_each_mem_range(i, &memblock.memory, &memblock.reserved,     \
                           nid, flags, p_start, p_end, p_nid)
{% endhighlight %}

for_each_free_mem_range() 函数用于遍历指定 regions 内所有空闲的
region。参数 i 左右遍历索引，参数 nid 指向 NUMA 信息，参数 p_start
存储起始物理地址，p_end 参数存储终止物理地址，p_nid 指向 NUMA 信息。
函数通过 for_each_mem_range() 函数遍历 memblock.memory 内的所有
regions，并确保遍历到的 region 不在 memblock.reserved 里面。

> - [for_each_mem_range](#A0144)
>
> - [for_each_free_mem_range 内核实践](https://biscuitos.github.io/blog/MMU-ARM32-MEMBLOCK-for_each_free_mem_range/)

------------------------------------

#### <span id="A0146">clamp</span>

{% highlight c %}
/**
 * clamp - return a value clamped to a given range with strict typechecking
 * @val: current value          
 * @lo: lowest allowable value  
 * @hi: highest allowable value
 *
 * This macro does strict typechecking of @lo/@hi to make sure they are of the
 * same type as @val.  See the unnecessary pointer comparisons.
 */
#define clamp(val, lo, hi) min((typeof(val))max(val, lo), hi)
{% endhighlight %}

clamp() 函数用于从 val 和 lo 中找到一个最大的值，再将该值与 hi 中
找到最小的值。

------------------------------------

#### <span id="A0147">__memblock_find_range_bottom_up</span>

{% highlight c %}
/**
 * __memblock_find_range_bottom_up - find free area utility in bottom-up
 * @start: start of candidate range
 * @end: end of candidate range, can be %MEMBLOCK_ALLOC_ANYWHERE or
 *       %MEMBLOCK_ALLOC_ACCESSIBLE
 * @size: size of free area to find
 * @align: alignment of free area to find
 * @nid: nid of the free area to find, %NUMA_NO_NODE for any node
 * @flags: pick from blocks based on memory attributes
 *
 * Utility called from memblock_find_in_range_node(), find free area bottom-up.
 *
 * Return:
 * Found address on success, 0 on failure.
 */
static phys_addr_t __init_memblock
__memblock_find_range_bottom_up(phys_addr_t start, phys_addr_t end,
                                phys_addr_t size, phys_addr_t align, int nid,
                                enum memblock_flags flags)
{
        phys_addr_t this_start, this_end, cand;
        u64 i;

        for_each_free_mem_range(i, nid, flags, &this_start, &this_end, NULL) {
                this_start = clamp(this_start, start, end);
                this_end = clamp(this_end, start, end);

                cand = round_up(this_start, align);
                if (cand < this_end && this_end - cand >= size)
                        return cand;
        }

        return 0;
}
{% endhighlight %}

__memblock_find_range_bottom_up() 函数用于 MEMBLOCK 的底部
开始，查找一块可用的物理内存 region。参数 start 指向查找起始
物理地址，参数 end 指向查找终止物理地址，参数 size 指向 region
大小，参数 align 代表对齐方式，参数 nid 指向 NUMA 信息，参数
flags 包含 region flags。函数首先调用 for_each_free_mem_range()
函数遍历了所有可用物理内存 region，在每次遍历中，调用 clamp()
函数找到符合要求的范围，函数接着调用 round_up() 对 this_start
进行对齐，对齐之后的值小于终止地址，并且找到的长度大于等于 size，
那么函数就找到一个空闲的 region。

> - [for_each_free_mem_range](#A0)
>
> - [clamp](#A0)
>
> - [round_up](#A0149)

------------------------------------

#### <span id="A0148">__round_mask</span>

{% highlight c %}
/*
 * This looks more complex than it should be. But we need to
 * get the type for the ~ right in round_down (it needs to be
 * as wide as the result!), and we want to evaluate the macro
 * arguments just once each.
 */
#define __round_mask(x, y) ((__typeof__(x))((y)-1))
{% endhighlight %}

__round_mask() 用于生成 (y-1) 的掩码。

------------------------------------

#### <span id="A0149">round_up</span>

{% highlight c %}
/**
 * round_up - round up to next specified power of 2
 * @x: the value to round       
 * @y: multiple to round up to (must be a power of 2)
 *      
 * Rounds @x up to next multiple of @y (which must be a power of 2).
 * To perform arbitrary rounding up, use roundup() below.
 */     
#define round_up(x, y) ((((x)-1) | __round_mask(x, y))+1)
{% endhighlight %}

round_up() 函数用于向上对齐。参数 x 为需要对齐的数据，y 为
对齐的方式。

> - [\_\_round_mask](#A0148)

------------------------------------

#### <span id="A0150">__next_mem_range_rev</span>

{% highlight c %}
/**
 * __next_mem_range_rev - generic next function for for_each_*_range_rev()
 *
 * @idx: pointer to u64 loop variable
 * @nid: node selector, %NUMA_NO_NODE for all nodes
 * @flags: pick from blocks based on memory attributes
 * @type_a: pointer to memblock_type from where the range is taken
 * @type_b: pointer to memblock_type which excludes memory from being taken
 * @out_start: ptr to phys_addr_t for start address of the range, can be %NULL
 * @out_end: ptr to phys_addr_t for end address of the range, can be %NULL
 * @out_nid: ptr to int for nid of the range, can be %NULL
 *
 * Finds the next range from type_a which is not marked as unsuitable
 * in type_b.
 *
 * Reverse of __next_mem_range().
 */
void __init_memblock __next_mem_range_rev(u64 *idx, int nid,
                                          enum memblock_flags flags,
                                          struct memblock_type *type_a,
                                          struct memblock_type *type_b,
                                          phys_addr_t *out_start,
                                          phys_addr_t *out_end, int *out_nid)
{
        int idx_a = *idx & 0xffffffff;
        int idx_b = *idx >> 32;

        if (WARN_ONCE(nid == MAX_NUMNODES, "Usage of MAX_NUMNODES is deprecated. Use NUMA_NO_NODE instead\n"))
                nid = NUMA_NO_NODE;

        if (*idx == (u64)ULLONG_MAX) {
                idx_a = type_a->cnt - 1;
                if (type_b != NULL)
                        idx_b = type_b->cnt;
                else
                        idx_b = 0;
        }

        for (; idx_a >= 0; idx_a--) {
                struct memblock_region *m = &type_a->regions[idx_a];

                phys_addr_t m_start = m->base;
                phys_addr_t m_end = m->base + m->size;
                int m_nid = memblock_get_region_node(m);

                /* only memory regions are associated with nodes, check it */
                if (nid != NUMA_NO_NODE && nid != m_nid)
                        continue;

                /* skip hotpluggable memory regions if needed */
                if (movable_node_is_enabled() && memblock_is_hotpluggable(m))
                        continue;

                /* if we want mirror memory skip non-mirror memory regions */
                if ((flags & MEMBLOCK_MIRROR) && !memblock_is_mirror(m))
                        continue;

                /* skip nomap memory unless we were asked for it explicitly */
                if (!(flags & MEMBLOCK_NOMAP) && memblock_is_nomap(m))
                        continue;

                if (!type_b) {
                        if (out_start)
                                *out_start = m_start;
                        if (out_end)
                                *out_end = m_end;
                        if (out_nid)
                                *out_nid = m_nid;
                        idx_a--;
                        *idx = (u32)idx_a | (u64)idx_b << 32;
                        return;
                }

                /* scan areas before each reservation */
                for (; idx_b >= 0; idx_b--) {
                        struct memblock_region *r;
                        phys_addr_t r_start;
                        phys_addr_t r_end;

                        r = &type_b->regions[idx_b];
                        r_start = idx_b ? r[-1].base + r[-1].size : 0;
                        r_end = idx_b < type_b->cnt ?
                                r->base : PHYS_ADDR_MAX;
                        /*
                         * if idx_b advanced past idx_a,
                         * break out to advance idx_a
                         */

                        if (r_end <= m_start)
                                break;
                        /* if the two regions intersect, we're done */
                        if (m_end > r_start) {
                                if (out_start)
                                        *out_start = max(m_start, r_start);
                                if (out_end)
                                        *out_end = min(m_end, r_end);
                                if (out_nid)
                                        *out_nid = m_nid;
                                if (m_start >= r_start)
                                        idx_a--;
                                else
                                        idx_b--;
                                *idx = (u32)idx_a | (u64)idx_b << 32;
                                return;
                        }
                }
        }
        /* signal end of iteration */
        *idx = ULLONG_MAX;
}
{% endhighlight %}

__next_mem_range_rev() 函数从指定的区域之后查找一块可用的物理内存区块。
函数代码较长，分段解析

{% highlight c %}
/**
 * __next_mem_range_rev - generic next function for for_each_*_range_rev()
 *
 * @idx: pointer to u64 loop variable
 * @nid: node selector, %NUMA_NO_NODE for all nodes
 * @flags: pick from blocks based on memory attributes
 * @type_a: pointer to memblock_type from where the range is taken
 * @type_b: pointer to memblock_type which excludes memory from being taken
 * @out_start: ptr to phys_addr_t for start address of the range, can be %NULL
 * @out_end: ptr to phys_addr_t for end address of the range, can be %NULL
 * @out_nid: ptr to int for nid of the range, can be %NULL
 *
 * Finds the next range from type_a which is not marked as unsuitable
 * in type_b.
 *
 * Reverse of __next_mem_range().
 */
void __init_memblock __next_mem_range_rev(u64 *idx, int nid,
                                          enum memblock_flags flags,
                                          struct memblock_type *type_a,
                                          struct memblock_type *type_b,
                                          phys_addr_t *out_start,
                                          phys_addr_t *out_end, int *out_nid)
{
        int idx_a = *idx & 0xffffffff;
        int idx_b = *idx >> 32;
{% endhighlight %}

参数 idx 是一个 64 位变量，其高 32 位作为 type_a 参数的索引，低 32位作为
type_b 参数的索引。flags 指向内存区的标志；type_a 指向可用物理内存区；
type_b 指向预留内存区；out_start 参数用于存储查找到区域的起始地址；
out_end 参数用于存储找到区域的终止地址; out_nid 指向 nid 域; idx_a 变量
用于存储可用物理内存区块的索引; idx_b 变量用于存储预留区块的索引。

{% highlight c %}
if (WARN_ONCE(nid == MAX_NUMNODES, "Usage of MAX_NUMNODES is deprecated. Use NUMA_NO_NODE instead\n"))
        nid = NUMA_NO_NODE;

if (*idx == (u64)ULLONG_MAX) {
        idx_a = type_a->cnt - 1;
        if (type_b != NULL)
                idx_b = type_b->cnt;
        else
                idx_b = 0;
}
{% endhighlight %}

函数首先检查 nid 参数，如果参数值不是 MAX_NUMNODES， 那么函数就会报错。接着函数
将 idx 参数进行检查，如果参数的值是 ULLONG_MAX,那么就将 idx_a 设置为 type_a
内存区含有的内存区块数，此时，如果 type_b 参数如果存在，那么将 idx_b 设置为
预留区的函数的内存区块数。如果 type_b 参数不存在，那么函数认为 MEMBLOCK 不存在
预留区，可以从有用的物理内存区开始查找，如果 idx_b 设置为 0.

{% highlight c %}
for (; idx_a >= 0; idx_a--) {
        struct memblock_region *m = &type_a->regions[idx_a];

        phys_addr_t m_start = m->base;
        phys_addr_t m_end = m->base + m->size;
        int m_nid = memblock_get_region_node(m);

        /* only memory regions are associated with nodes, check it */
        if (nid != NUMA_NO_NODE && nid != m_nid)
                continue;

        /* skip hotpluggable memory regions if needed */
        if (movable_node_is_enabled() && memblock_is_hotpluggable(m))
                continue;

        /* if we want mirror memory skip non-mirror memory regions */
        if ((flags & MEMBLOCK_MIRROR) && !memblock_is_mirror(m))
                continue;

        /* skip nomap memory unless we were asked for it explicitly */
        if (!(flags & MEMBLOCK_NOMAP) && memblock_is_nomap(m))
                continue;
{% endhighlight %}

这段代码首先使用 for 循环遍历 MEMBLOCK 中可用的物理内存区块，从最后一块开始
遍历，每次遍历一块内存区块，都是用 struct memblock_region 结构体维护，以此
计算出这块内存区块的起始地址和终止地址。接下来进行内存区块的检测，函数是要找到
一块符合要求的的内存区块，内存区块要满足以下几点：

> 1. 内存区块必须属于指定的 node
>
> 2. 内存区块不是热插拔的
>
> 3. 内存区块不是 non-mirror 的
>
> 4. 内存区块不能是 MEMBLOCK_NOMAP 的

只有符合上述的内存区块才是可以继续查找的部分。

{% highlight c %}
        if (!type_b) {
                if (out_start)
                        *out_start = m_start;
                if (out_end)
                        *out_end = m_end;
                if (out_nid)
                        *out_nid = m_nid;
                idx_a--;
                *idx = (u32)idx_a | (u64)idx_b << 32;
                return;
        }
{% endhighlight %}

经过上面的检测之后，如果预留区不存在，那么直接就从可用物理内存区中返回可用的地址，
out_start 参数指向可用物理内存区块的起始地址；out_end 参数指向可用物理内存区块
的终止地址，将 idx_a 指向前一块可用的物理内存块，并更新 idx 参数的值，使其指向
新的索引。最后返回

{% highlight c %}
/* scan areas before each reservation */
for (; idx_b >= 0; idx_b--) {
        struct memblock_region *r;
        phys_addr_t r_start;
        phys_addr_t r_end;

        r = &type_b->regions[idx_b];
        r_start = idx_b ? r[-1].base + r[-1].size : 0;
        r_end = idx_b < type_b->cnt ?
                r->base : PHYS_ADDR_MAX;
        /*
         * if idx_b advanced past idx_a,
         * break out to advance idx_a
         */

        if (r_end <= m_start)
                break;
{% endhighlight %}

如果预留区存在，那么从 idx_b 指向的预留区开始查找。同样使用 memblock_region
结构维护一个预留区块。接下来判断，如果该预留区块的前一块预留区块也存在，那么函数
将上一块预留区块的结束地址到该预留区块的起始地址作为一块可用的物理内存区块；如果
不存在上一块预留区块，那么可用物理内存区块的起始地址设置为 0；如果 idx_b 的索引
不小于预留区所具有的内存区块数，那么将可用物理内存区块的终止地址设置为
PHYS_ADDR_MAX.通过上面的判断可以找到一块可用的物理内存区块范围，如果找到的内存
区块的起始地址大于或等于该预留区块，那么就跳出循环，表示在该可用的物理内存区段中
找不到一块满足条件的内存区块。

{% highlight c %}
/* scan areas before each reservation */
/* if the two regions intersect, we're done */
if (m_end > r_start) {
        if (out_start)
                *out_start = max(m_start, r_start);
        if (out_end)
                *out_end = min(m_end, r_end);
        if (out_nid)
                *out_nid = m_nid;
        if (m_start >= r_start)
                idx_a--;
        else
                idx_b--;
        *idx = (u32)idx_a | (u64)idx_b << 32;
        return;
}
{% endhighlight %}

如果符合之前的条件，那么接下来只要找到的物理内存区块的终止地址比预留区的大，但找到的
物理内存区块的起始地址小于预留区块的终止地址，至少保证可用物理内存区在预留区块之前有
交集。接下来，将 out_start 参数指向最大的起始地址；将 out_end 指向最小的终止地
址，这样处理能确保找到的物理内存区块不与预留区块重叠。接着如果找到的可用物理内存区块
的起始地址大于预留区块的起始地址，那么增加 idx_a 的引用计数，这样可以在循环中指向
下一块可用的物理内存；反之增加 idx_b 的引用计数，这样可以指向下一块预留区块。

{% highlight c %}
/* signal end of iteration */
*idx = ULLONG_MAX;
{% endhighlight %}

如果循环遍历之后，还找不到，那么直接设置 idx 为 ULLONG_MAX.

> - [memblock_get_region_node](#A0136)
>
> - [movable_node_is_enabled](#A0140)
>
> - [memblock_is_hotpluggable](#A0141)
>
> - [memblock_is_mirror](#A0142)
>
> - [memblock_is_nomap](#A0143)
>
> - [__next_mem_range_rev 内核实践](https://biscuitos.github.io/blog/MMU-ARM32-MEMBLOCK-__next_mem_range_rev/#header)

------------------------------------

#### <span id="A0151">for_each_mem_range_rev</span>

{% highlight c %}
/**
 * for_each_mem_range_rev - reverse iterate through memblock areas from
 * type_a and not included in type_b. Or just type_a if type_b is NULL.
 * @i: u64 used as loop variable
 * @type_a: ptr to memblock_type to iterate
 * @type_b: ptr to memblock_type which excludes from the iteration
 * @nid: node selector, %NUMA_NO_NODE for all nodes
 * @flags: pick from blocks based on memory attributes
 * @p_start: ptr to phys_addr_t for start address of the range, can be %NULL
 * @p_end: ptr to phys_addr_t for end address of the range, can be %NULL
 * @p_nid: ptr to int for nid of the range, can be %NULL
 */
#define for_each_mem_range_rev(i, type_a, type_b, nid, flags,           \
                               p_start, p_end, p_nid)                   \
        for (i = (u64)ULLONG_MAX,                                       \
                     __next_mem_range_rev(&i, nid, flags, type_a, type_b,\
                                          p_start, p_end, p_nid);       \
             i != (u64)ULLONG_MAX;                                      \
             __next_mem_range_rev(&i, nid, flags, type_a, type_b,       \
                                  p_start, p_end, p_nid))
{% endhighlight %}

for_each_mem_range_rev() 函数的作用就是遍历所有可用的物理内存区块。
参数 i 用于循环；type_a 参数指向可用物理内存区块; type_b 参数指向预留物理内存
区块。 nid 指向节点信息；参数 flags 指向内存区标志； p_start 参数用于存储查找
到的内存区块起始地址； p_end 参数用于存储查找到的内存区块的终止地址

函数首先调用 for 循环，将参数 i 设置为 ULLONG_MAX, 以此遍历所有预留区，然后调用
__next_mem_range_rev() 函数查找一块可用的物理内存。每遍历一次，循环都检查 i 的
值，如果 i 不等于 ULLONG_MAX, 那么继续循环。每次调用完 __next_mem_range_rev()
函数之后，i 都会指向前一个预留区。通过循环，每个找到的可用物理内存区块都会将其起始
地址存储到 p_start 参数里，将终止地址存储到 p_end 参数里。直到遍历完所有可用物理
内存区块之后终止循环。

> - [\_\_next_mem_range_rev](#A0150)
>
> - [for_each_mem_range_rev 内核实践](https://biscuitos.github.io/blog/MMU-ARM32-MEMBLOCK-for_each_mem_range_rev/)

------------------------------------

#### <span id="A0152">for_each_free_mem_range_reverse</span>

{% highlight c %}
/**
 * for_each_free_mem_range_reverse - rev-iterate through free memblock areas
 * @i: u64 used as loop variable
 * @nid: node selector, %NUMA_NO_NODE for all nodes
 * @flags: pick from blocks based on memory attributes
 * @p_start: ptr to phys_addr_t for start address of the range, can be %NULL
 * @p_end: ptr to phys_addr_t for end address of the range, can be %NULL
 * @p_nid: ptr to int for nid of the range, can be %NULL
 *
 * Walks over free (memory && !reserved) areas of memblock in reverse
 * order.  Available as soon as memblock is initialized.
 */
#define for_each_free_mem_range_reverse(i, nid, flags, p_start, p_end,  \
                                        p_nid)                          \
        for_each_mem_range_rev(i, &memblock.memory, &memblock.reserved, \
                               nid, flags, p_start, p_end, p_nid)
{% endhighlight %}

for_each_free_mem_range_reverse() 函数的作用就是倒叙遍历所有 free
物理内存区块。参数 i 用于循环； nid 指向节点信息；参数 flags 指向内
存区标志； p_start 参数用于存储查找到的内存区块起始地址； p_end 参数
用于存储查找到的内存区块的终止地址。函数直接调用 for_each_mem_range_rev()
函数。

> - [for_each_free_mem_range_reverse](#A0151)
>
> - [for_each_free_mem_range_reverse 内核实践](https://biscuitos.github.io/blog/MMU-ARM32-MEMBLOCK-for_each_free_mem_range_reverse/)

------------------------------------

#### <span id="A0153">__memblock_find_range_top_down</span>

{% highlight c %}
/**
 * __memblock_find_range_top_down - find free area utility, in top-down
 * @start: start of candidate range
 * @end: end of candidate range, can be %MEMBLOCK_ALLOC_ANYWHERE or
 *       %MEMBLOCK_ALLOC_ACCESSIBLE
 * @size: size of free area to find
 * @align: alignment of free area to find
 * @nid: nid of the free area to find, %NUMA_NO_NODE for any node
 * @flags: pick from blocks based on memory attributes
 *
 * Utility called from memblock_find_in_range_node(), find free area top-down.
 *
 * Return:
 * Found address on success, 0 on failure.
 */
static phys_addr_t __init_memblock
__memblock_find_range_top_down(phys_addr_t start, phys_addr_t end,
                               phys_addr_t size, phys_addr_t align, int nid,
                               enum memblock_flags flags)
{
        phys_addr_t this_start, this_end, cand;
        u64 i;

        for_each_free_mem_range_reverse(i, nid, flags, &this_start, &this_end,
                                        NULL) {
                this_start = clamp(this_start, start, end);
                this_end = clamp(this_end, start, end);

                if (this_end < size)
                        continue;

                cand = round_down(this_end - size, align);
                if (cand >= this_start)
                        return cand;
        }

        return 0;
}
{% endhighlight %}

__memblock_find_range_top_down() 函数的作用是从顶端往低端，查找满足
需求的空闲物理块。函数调用 for_each_free_mem_range_reverse()
倒序遍历所有的空闲物理区块，在这些空闲的物理区块中，使用
clamp() 函数找到符合要求的起始物理地址和终止物理地址，如果
找到的终止地址小于 size，那么继续查找下一块；反之如果从
终止地址往低端地址，长度为 size 并按 align 方式对齐，那么
此时地址大于等于起始地址，那么函数就找到一块符合要求的物理
区块。

> - [for_each_free_mem_range_reverse](#A0152)
>
> - [clamp](#A0146)
>
> - [round_down](#A0154)

------------------------------------

#### <span id="A0154">round_down</span>

{% highlight c %}
/**
 * round_down - round down to next specified power of 2
 * @x: the value to round
 * @y: multiple to round down to (must be a power of 2)
 *
 * Rounds @x down to next multiple of @y (which must be a power of 2).
 * To perform arbitrary rounding down, use rounddown() below.
 */
#define round_down(x, y) ((x) & ~__round_mask(x, y))
{% endhighlight %}

round_down() 函数用于向下按 y 方式对齐。函数通过 __round_mask()
函数获得相应的掩码之后，再反码。最后相与，这样 x 就按向下
按 y 方式对齐。

------------------------------------

#### <span id="A0155">memblock_find_in_range_node</span>

{% highlight c %}
/**
 * memblock_find_in_range_node - find free area in given range and node
 * @size: size of free area to find
 * @align: alignment of free area to find
 * @start: start of candidate range
 * @end: end of candidate range, can be %MEMBLOCK_ALLOC_ANYWHERE or
 *       %MEMBLOCK_ALLOC_ACCESSIBLE
 * @nid: nid of the free area to find, %NUMA_NO_NODE for any node
 * @flags: pick from blocks based on memory attributes
 *
 * Find @size free area aligned to @align in the specified range and node.
 *
 * When allocation direction is bottom-up, the @start should be greater
 * than the end of the kernel image. Otherwise, it will be trimmed. The
 * reason is that we want the bottom-up allocation just near the kernel
 * image so it is highly likely that the allocated memory and the kernel
 * will reside in the same node.
 *
 * If bottom-up allocation failed, will try to allocate memory top-down.
 *
 * Return:
 * Found address on success, 0 on failure.
 */
phys_addr_t __init_memblock memblock_find_in_range_node(phys_addr_t size,
                                        phys_addr_t align, phys_addr_t start,
                                        phys_addr_t end, int nid,
                                        enum memblock_flags flags)
{
        phys_addr_t kernel_end, ret;

        /* pump up @end */
        if (end == MEMBLOCK_ALLOC_ACCESSIBLE ||
            end == MEMBLOCK_ALLOC_KASAN)
                end = memblock.current_limit;

        /* avoid allocating the first page */
        start = max_t(phys_addr_t, start, PAGE_SIZE);
        end = max(start, end);
        kernel_end = __pa_symbol(_end);

        /*
         * try bottom-up allocation only when bottom-up mode
         * is set and @end is above the kernel image.
         */
        if (memblock_bottom_up() && end > kernel_end) {
                phys_addr_t bottom_up_start;

                /* make sure we will allocate above the kernel */
                bottom_up_start = max(start, kernel_end);

                /* ok, try bottom-up allocation first */
                ret = __memblock_find_range_bottom_up(bottom_up_start, end,
                                                      size, align, nid, flags);
                if (ret)
                        return ret;

                /*
                 * we always limit bottom-up allocation above the kernel,
                 * but top-down allocation doesn't have the limit, so
                 * retrying top-down allocation may succeed when bottom-up
                 * allocation failed.
                 *
                 * bottom-up allocation is expected to be fail very rarely,
                 * so we use WARN_ONCE() here to see the stack trace if
                 * fail happens.
                 */
                WARN_ONCE(IS_ENABLED(CONFIG_MEMORY_HOTREMOVE),
                          "memblock: bottom-up allocation failed, memory hotremove may be affected\n");
        }

        return __memblock_find_range_top_down(start, end, size, align, nid,
                                              flags);
}
{% endhighlight %}

memblock_find_in_range_node() 函数的作用是在指定的节点区间
内查找一块可用的物理内存。代码较长，分段解析

{% highlight c %}
/**
 * memblock_find_in_range_node - find free area in given range and node
 * @size: size of free area to find
 * @align: alignment of free area to find
 * @start: start of candidate range
 * @end: end of candidate range, can be %MEMBLOCK_ALLOC_ANYWHERE or
 *       %MEMBLOCK_ALLOC_ACCESSIBLE
 * @nid: nid of the free area to find, %NUMA_NO_NODE for any node
 * @flags: pick from blocks based on memory attributes
 *
 * Find @size free area aligned to @align in the specified range and node.
 *
 * When allocation direction is bottom-up, the @start should be greater
 * than the end of the kernel image. Otherwise, it will be trimmed. The
 * reason is that we want the bottom-up allocation just near the kernel
 * image so it is highly likely that the allocated memory and the kernel
 * will reside in the same node.
 *
 * If bottom-up allocation failed, will try to allocate memory top-down.
 *
 * Return:
 * Found address on success, 0 on failure.
 */
phys_addr_t __init_memblock memblock_find_in_range_node(phys_addr_t size,
					phys_addr_t align, phys_addr_t start,
					phys_addr_t end, int nid,
					enum memblock_flags flags)
{
	phys_addr_t kernel_end, ret;
{% endhighlight %}

参数 size 代表需要查找内存的大小；align 代表对齐方式；start 代表需要查找
内存区域的起始地址；end 参数代表需要查找内存区域的终止地址；nid 代表 NUMA
节点信息；flags 代表内存区的标志。

{% highlight c %}
/* pump up @end */
if (end == MEMBLOCK_ALLOC_ACCESSIBLE ||
    end == MEMBLOCK_ALLOC_KASAN)
  end = memblock.current_limit;

/* avoid allocating the first page */
start = max_t(phys_addr_t, start, PAGE_SIZE);
end = max(start, end);
kernel_end = __pa_symbol(_end);
{% endhighlight %}

函数首先对 end 参数进行检测，只要参数属于 MEMBLOCK_ALLOC_ACCESSIBLE
或 MEMBLOCK_ALLOC_KASAN 中的一种，那么函数就会将 end 参数设置为
MEMBLOCK 最大限制地址。接着函数调用 max_t() 函数和 max() 函数对
start 参数和 end 参数进行简单的处理。最后通过 __pa_symbol() 函数获得
kernel 镜像的终止物理地址。

{% highlight c %}
/*
 * try bottom-up allocation only when bottom-up mode
 * is set and @end is above the kernel image.
 */
if (memblock_bottom_up() && end > kernel_end) {
  phys_addr_t bottom_up_start;

  /* make sure we will allocate above the kernel */
  bottom_up_start = max(start, kernel_end);

  /* ok, try bottom-up allocation first */
  ret = __memblock_find_range_bottom_up(bottom_up_start, end,
                size, align, nid, flags);
  if (ret)
    return ret;

  /*
   * we always limit bottom-up allocation above the kernel,
   * but top-down allocation doesn't have the limit, so
   * retrying top-down allocation may succeed when bottom-up
   * allocation failed.
   *
   * bottom-up allocation is expected to be fail very rarely,
   * so we use WARN_ONCE() here to see the stack trace if
   * fail happens.
   */
  WARN_ONCE(IS_ENABLED(CONFIG_MEMORY_HOTREMOVE),
      "memblock: bottom-up allocation failed, memory hotremove may be affected\n");
}
{% endhighlight %}

接下来，参数做了一个判断，如果 memblock_bottom_up() 函数返回 true，表示
MEMBLOCK 支持从低向上的分配，以及查找的终止地址大于内核的终止物理地址，
那么函数将执行从低地址开始查找符合要求的内存区块。采用这种分配的一定要
从 kernel 的终止地址之后开始分配，接着调用 __memblock_find_range_bottom_up()
函数进行分配，如果分配成功，则返回获得的起始地址。由于 bottom-top 的分配
要从 kernel 结束地址之后开始分配，但 top-down 分配则没有这个限制，所以
bottom-top 的分配很容易失败，所以当分配失败之后，函数会调用 WARN_ONCE 进行
提示。

{% highlight c %}
return __memblock_find_range_top_down(start, end, size, align, nid,
					      flags);
{% endhighlight %}

如果函数没有采用 bottom-up 的分配方式，那么函数就采用 top-down 的方式进行
分配，函数调用 __memblock_find_range_top_down() 函数，并直接返回查找到的
值。

> - [memblock_bottom_up](#A0138)
>
> - [\_\_memblock_find_range_bootm_up](#A0147)
>
> - [\_\_memblock_find_range_top_down](#A0153)
>
> - [memblock_find_in_range_node 内核实践](https://biscuitos.github.io/blog/MMU-ARM32-MEMBLOCK-memblock_find_in_range_node/)

------------------------------------

#### <span id="A0156">memblock_find_in_range</span>

{% highlight c %}
/**
 * memblock_find_in_range - find free area in given range
 * @start: start of candidate range
 * @end: end of candidate range, can be %MEMBLOCK_ALLOC_ANYWHERE or
 *       %MEMBLOCK_ALLOC_ACCESSIBLE
 * @size: size of free area to find
 * @align: alignment of free area to find
 *
 * Find @size free area aligned to @align in the specified range.
 *
 * Return:
 * Found address on success, 0 on failure.
 */
phys_addr_t __init_memblock memblock_find_in_range(phys_addr_t start,
                                        phys_addr_t end, phys_addr_t size,
                                        phys_addr_t align)
{
        phys_addr_t ret;
        enum memblock_flags flags = choose_memblock_flags();

again:
        ret = memblock_find_in_range_node(size, align, start, end,
                                            NUMA_NO_NODE, flags);

        if (!ret && (flags & MEMBLOCK_MIRROR)) {
                pr_warn("Could not allocate %pap bytes of mirrored memory\n",
                        &size);
                flags &= ~MEMBLOCK_MIRROR;
                goto again;
        }

        return ret;
}
{% endhighlight %}

memblock_find_in_range() 函数的作用是在指定的区间内查找
一块可用的物理内存。参数 start 查找区域的起始地址；参数 end
查找区域的结束地址；参数 size 表示要查找可用物理内存的大小；
align 参数用于对齐操作。

函数调用 memblock_find_in_range_node() 函数在指定的节点和指定的区域内找到
一块 size 大小的可用物理内存块，并将起始地址存储在 ret 中。如果 ret 为 0，
那么该标志无法获得想要的物理内存，改变物理内存标志，然后跳到 again 继续查找。

> - [memblock_find_in_range_node](#A0)
>
> - [choose_memblock_flags](#A0157)
>
> - [memblock_find_in_range 内核实践](https://biscuitos.github.io/blog/MMU-ARM32-MEMBLOCK-memblock_find_in_range/)

------------------------------------

#### <span id="A0157">choose_memblock_flags</span>

{% highlight c %}
enum memblock_flags __init_memblock choose_memblock_flags(void)
{
        return system_has_some_mirror ? MEMBLOCK_MIRROR : MEMBLOCK_NONE;
}
{% endhighlight %}

choose_memblock_flags() 函数用于获得 MEMBLOCK 内存区间的 flags。
如果全局变量 system_has_some_mirror 为 ture，那么直接返回
MEMBLOCK_MIRROR；反之返回 MEMBLOCK_NONE。

------------------------------------

#### <span id="A0158">memblock_isolate_range</span>

{% highlight c %}
/**
 * memblock_isolate_range - isolate given range into disjoint memblocks
 * @type: memblock type to isolate range for
 * @base: base of range to isolate
 * @size: size of range to isolate
 * @start_rgn: out parameter for the start of isolated region
 * @end_rgn: out parameter for the end of isolated region
 *
 * Walk @type and ensure that regions don't cross the boundaries defined by
 * [@base, @base + @size).  Crossing regions are split at the boundaries,
 * which may create at most two more regions.  The index of the first
 * region inside the range is returned in *@start_rgn and end in *@end_rgn.
 *
 * Return:
 * 0 on success, -errno on failure.
 */
static int __init_memblock memblock_isolate_range(struct memblock_type *type,
                                        phys_addr_t base, phys_addr_t size,
                                        int *start_rgn, int *end_rgn)
{
        phys_addr_t end = base + memblock_cap_size(base, &size);
        int idx;
        struct memblock_region *rgn;

        *start_rgn = *end_rgn = 0;

        if (!size)
                return 0;

        /* we'll create at most two more regions */
        while (type->cnt + 2 > type->max)
                if (memblock_double_array(type, base, size) < 0)
                        return -ENOMEM;


        for_each_memblock_type(idx, type, rgn) {
                phys_addr_t rbase = rgn->base;
                phys_addr_t rend = rbase + rgn->size;

                if (rbase >= end)
                        break;
                if (rend <= base)
                        continue;

                if (rbase < base) {
                        /*
                         * @rgn intersects from below.  Split and continue
                         * to process the next region - the new top half.
                         */
                        rgn->base = base;
                        rgn->size -= base - rbase;
                        type->total_size -= base - rbase;
                        memblock_insert_region(type, idx, rbase, base - rbase,
                                               memblock_get_region_node(rgn),
                                               rgn->flags);
                } else if (rend > end) {
                        /*
                         * @rgn intersects from above.  Split and redo the
                         * current region - the new bottom half.
                         */
                        rgn->base = end;
                        rgn->size -= end - rbase;
                        type->total_size -= end - rbase;
                        memblock_insert_region(type, idx--, rbase, end - rbase,
                                               memblock_get_region_node(rgn),
                                               rgn->flags);
                } else {
                        /* @rgn is fully contained, record it */
                        if (!*end_rgn)
                                *start_rgn = idx;
                        *end_rgn = idx + 1;
                }
        }

        return 0;
}
{% endhighlight %}

memblock_isolate_range() 函数用于是将指定范围的物理内存从内存区块
中孤立出来。原先的物理内存区块可能被拆分做多块。函数较长，分段解析：

{% highlight c %}
static int __init_memblock memblock_isolate_range(struct memblock_type *type,
                                        phys_addr_t base, phys_addr_t size,
                                        int *start_rgn, int *end_rgn)
{
        phys_addr_t end = base + memblock_cap_size(base, &size);
        int idx;
        struct memblock_region *rgn;

        *start_rgn = *end_rgn = 0;

        if (!size)
                return 0;

        /* we'll create at most two more regions */
        while (type->cnt + 2 > type->max)
                if (memblock_double_array(type, base, size) < 0)
                        return -ENOMEM;
{% endhighlight %}

参数 type 指向特定的 memblock_type，base 指向孤立物理内存块的
起始物理地址，size 指向孤立物理内存区块的长度。
函数首先调用 memblock_cap_size() 函数获得一个孤立物理块安全
的长度，基于这个长度，计算出孤立物理块的终止物理地址。此时将
参数 start_rgn 与 end_rgn 都设置为 0. 如果当前 memblock_type
所具有的 regions 数加上 2 之后大于该 memblock_type 支持的最大
regions 数，那么函数调用 memblock_double_array() 将 memblock_type
拓展为原先两倍大。如果拓展失败，函数返回 -ENOMEM。

{% highlight c %}
        for_each_memblock_type(idx, type, rgn) {
                phys_addr_t rbase = rgn->base;
                phys_addr_t rend = rbase + rgn->size;

                if (rbase >= end)
                        break;
                if (rend <= base)
                        continue;
{% endhighlight %}

函数调用 for_each_memblock_type() 遍历 type 对应的所有
物理内存 region， 将每次遍历到的 region 起始地址存储到
rbase，并将终止地址存储到 rend。接着将查找的 region 与
end 和 base 做检测，确保找到符合要求的 region。

{% highlight c %}
                if (rbase < base) {
                        /*
                         * @rgn intersects from below.  Split and continue
                         * to process the next region - the new top half.
                         */
                        rgn->base = base;
                        rgn->size -= base - rbase;
                        type->total_size -= base - rbase;
                        memblock_insert_region(type, idx, rbase, base - rbase,
                                               memblock_get_region_node(rgn),
                                               rgn->flags);
{% endhighlight %}

如果 rbase 小于 base，那么需要孤立的内存区块可能可遍历到的 region
存在重叠的情况，此时将重叠的部分从原先的 region 中移除，并调用
memblock_insert_region() 函数将剩余的 region 继续插入到 memblock_type
的 regions 里面。

{% highlight c %}
                } else if (rend > end) {
                        /*
                         * @rgn intersects from above.  Split and redo the
                         * current region - the new bottom half.
                         */
                        rgn->base = end;
                        rgn->size -= end - rbase;
                        type->total_size -= end - rbase;
                        memblock_insert_region(type, idx--, rbase, end - rbase,
                                               memblock_get_region_node(rgn),
                                               rgn->flags);
{% endhighlight %}

如果 rend 大于 end，那么需要孤立的物理内存块和遍历到的 region 存在
前部重叠，此时将重叠部分从 region 中移除，并将剩余的 region 重新
插入到 memblock_type 的 regions 里面。

{% highlight c %}
                } else {
                        /* @rgn is fully contained, record it */
                        if (!*end_rgn)
                                *start_rgn = idx;
                        *end_rgn = idx + 1;
                }
{% endhighlight %}

最后如果需要孤立的物理内存区块与遍历到 region 不存在重叠或完全包含，
那么函数只更新 start_rgn 与 end_rgn 的值，以此代表移除的内存区块
占用了多个 regions。

> - [memblock_cap_size](#A0159)
>
> - [memblock_double_array](#A0163)
>
> - [for_each_memblock_type](#A0134)
>
> - [memblock_insert_region](#A0137)

------------------------------------

#### <span id="A0159">memblock_cap_size</span>

{% highlight c %}
/* adjust *@size so that (@base + *@size) doesn't overflow, return new size */
static inline phys_addr_t memblock_cap_size(phys_addr_t base, phys_addr_t *size)
{
        return *size = min(*size, PHYS_ADDR_MAX - base);
}
{% endhighlight %}

memblock_cap_size() 函数用于获得一个有效的长度值。函数确保 size
不会超出系统所支持的物理内存。函数通过 min() 函数获得 size 与
"PHYS_ADDR_MAX - base" 中最小值，以此确保 size 不会超出物理
内存的范围。

------------------------------------

#### <span id="A0160">memblock_remove_range</span>

{% highlight c %}
static int __init_memblock memblock_remove_range(struct memblock_type *type,
                                          phys_addr_t base, phys_addr_t size)
{
        int start_rgn, end_rgn;
        int i, ret;

        ret = memblock_isolate_range(type, base, size, &start_rgn, &end_rgn);
        if (ret)
                return ret;

        for (i = end_rgn - 1; i >= start_rgn; i--)
                memblock_remove_region(type, i);
        return 0;
}
{% endhighlight %}

memblock_remove_range() 函数用于从指定的 memblock_type 中
移除一定范围的内存区块。参数 type 执行特定的 memblock_type,
参数 base 指向移除的起始物理地址，参数 size 指向移除的终止
物理地址。函数首先调用 memblock_isolate_range() 函数将
指定的内存区块从指定的 memblock_type 中移除，如果移除的
内存区块包含了多个 region，那么函数使用 for 循环将每个
region 通过 memblock_remove_region() 函数进行移除。

> - [memblock_isolate_range](#A0158)
>
> - [memblock_remove_region](#A0161)

------------------------------------

#### <span id="A0161">memblock_remove_region</span>

{% highlight c %}
static void __init_memblock memblock_remove_region(struct memblock_type *type, unsigned long r)
{
        type->total_size -= type->regions[r].size;
        memmove(&type->regions[r], &type->regions[r + 1],
                (type->cnt - (r + 1)) * sizeof(type->regions[r]));
        type->cnt--;

        /* Special case for empty arrays */
        if (type->cnt == 0) {
                WARN_ON(type->total_size != 0);
                type->cnt = 1;
                type->regions[0].base = 0;
                type->regions[0].size = 0;
                type->regions[0].flags = 0;
                memblock_set_region_node(&type->regions[0], MAX_NUMNODES);
        }
}
{% endhighlight %}

memblock_remove_region() 函数用于从 memblock_type regions 中，
移除指定的 region。参数 type 指向特定的 memblock_type，参数 r
指向要移除 region 在 regions 中的索引。函数首先更新 memblock_type
的 total_size，以此减少该 memblock_type 占用的物理内存。接着调用
memmove() 函数将 regions 中 r+1 的 region 及其之后的所有 region
都向前移动一个 region。接着将 memblock_type 的 cnt 数减一。
如果移除完毕之后，memblock_type 的 cnt 为 0，那么代表该
memblock_type 内不包含任何物理内存，此时重新初始化
memblock_type 结构。

------------------------------------

#### <span id="A0162">memblock_free</span>

{% highlight c %}
/**
 * memblock_free - free boot memory block
 * @base: phys starting address of the  boot memory block
 * @size: size of the boot memory block in bytes
 *
 * Free boot memory block previously allocated by memblock_alloc_xx() API.
 * The freeing memory will not be released to the buddy allocator.
 */
int __init_memblock memblock_free(phys_addr_t base, phys_addr_t size)
{
        phys_addr_t end = base + size - 1;

        memblock_dbg("   memblock_free: [%pa-%pa] %pF\n",
                     &base, &end, (void *)_RET_IP_);

        kmemleak_free_part_phys(base, size);
        return memblock_remove_range(&memblock.reserved, base, size);
}
{% endhighlight %}

memblock_free() 函数的作用是释放物理内存区块。参数 base 指向
释放的起始物理地址，参数 size 指向释放物理区块的长度。
函数首先计算出释放物理内存区块的终止物理地址，然后调用
memblock_remove_range() 函数从保留区内移除 base 到 end
之间的物理内存区块。

> - [memblock_remove_range](#A0160)
>
> - [memblock_free 内核实践](https://biscuitos.github.io/blog/MMU-ARM32-MEMBLOCK-memblock_free/)

------------------------------------

#### <span id="A0163">memblock_double_array</span>

{% highlight c %}
/**
 * memblock_double_array - double the size of the memblock regions array
 * @type: memblock type of the regions array being doubled
 * @new_area_start: starting address of memory range to avoid overlap with
 * @new_area_size: size of memory range to avoid overlap with
 *
 * Double the size of the @type regions array. If memblock is being used to
 * allocate memory for a new reserved regions array and there is a previously
 * allocated memory range [@new_area_start, @new_area_start + @new_area_size]
 * waiting to be reserved, ensure the memory used by the new array does
 * not overlap.
 *
 * Return:
 * 0 on success, -1 on failure.
 */
static int __init_memblock memblock_double_array(struct memblock_type *type,
                                                phys_addr_t new_area_start,
                                                phys_addr_t new_area_size)
{
        struct memblock_region *new_array, *old_array;
        phys_addr_t old_alloc_size, new_alloc_size;
        phys_addr_t old_size, new_size, addr, new_end;
        int use_slab = slab_is_available();
        int *in_slab;

        /* We don't allow resizing until we know about the reserved regions
         * of memory that aren't suitable for allocation
         */
        if (!memblock_can_resize)
                return -1;

        /* Calculate new doubled size */
        old_size = type->max * sizeof(struct memblock_region);
        new_size = old_size << 1;
        /*
         * We need to allocated new one align to PAGE_SIZE,
         *   so we can free them completely later.
         */
        old_alloc_size = PAGE_ALIGN(old_size);
        new_alloc_size = PAGE_ALIGN(new_size);

        /* Retrieve the slab flag */
        if (type == &memblock.memory)
                in_slab = &memblock_memory_in_slab;
        else
                in_slab = &memblock_reserved_in_slab;

        /* Try to find some space for it.
         *
         * WARNING: We assume that either slab_is_available() and we use it or
         * we use MEMBLOCK for allocations. That means that this is unsafe to
         * use when bootmem is currently active (unless bootmem itself is
         * implemented on top of MEMBLOCK which isn't the case yet)
         *
         * This should however not be an issue for now, as we currently only
         * call into MEMBLOCK while it's still active, or much later when slab
         * is active for memory hotplug operations
         */
        if (use_slab) {
                new_array = kmalloc(new_size, GFP_KERNEL);
                addr = new_array ? __pa(new_array) : 0;
        } else {
                /* only exclude range when trying to double reserved.regions */
                if (type != &memblock.reserved)
                        new_area_start = new_area_size = 0;

                addr = memblock_find_in_range(new_area_start + new_area_size,
                                                memblock.current_limit,
                                                new_alloc_size, PAGE_SIZE);
                if (!addr && new_area_size)
                        addr = memblock_find_in_range(0,
                                min(new_area_start, memblock.current_limit),
                                new_alloc_size, PAGE_SIZE);

                new_array = addr ? __va(addr) : NULL;
        }
        if (!addr) {
                pr_err("memblock: Failed to double %s array from %ld to %ld entries !\n",
                       type->name, type->max, type->max * 2);
                return -1;
        }

        new_end = addr + new_size - 1;
        memblock_dbg("memblock: %s is doubled to %ld at [%pa-%pa]",
                        type->name, type->max * 2, &addr, &new_end);

        /*
         * Found space, we now need to move the array over before we add the
         * reserved region since it may be our reserved array itself that is
         * full.
         */
        memcpy(new_array, type->regions, old_size);
        memset(new_array + type->max, 0, old_size);
        old_array = type->regions;
        type->regions = new_array;
        type->max <<= 1;

        /* Free old array. We needn't free it if the array is the static one */
        if (*in_slab)
                kfree(old_array);
        else if (old_array != memblock_memory_init_regions &&
                 old_array != memblock_reserved_init_regions)
                memblock_free(__pa(old_array), old_alloc_size);

        /*
         * Reserve the new array if that comes from the memblock.  Otherwise, we
         * needn't do it
         */
        if (!use_slab)
                BUG_ON(memblock_reserve(addr, new_alloc_size));

        /* Update slab flag */
        *in_slab = use_slab;

        return 0;
}
{% endhighlight %}

memblock_double_array() 函数的作用是将 memblock_type 支持的 regions
数扩大两倍。由于函数较长，分段解析：

{% highlight c %}
static int __init_memblock memblock_double_array(struct memblock_type *type,
                                                phys_addr_t new_area_start,
                                                phys_addr_t new_area_size)
{
        struct memblock_region *new_array, *old_array;
        phys_addr_t old_alloc_size, new_alloc_size;
        phys_addr_t old_size, new_size, addr, new_end;
        int use_slab = slab_is_available();
        int *in_slab;

        /* We don't allow resizing until we know about the reserved regions
         * of memory that aren't suitable for allocation
         */
        if (!memblock_can_resize)
                return -1;
{% endhighlight %}

参数 type 指向需要扩编的 memblock_type, 参数 new_area_start
代表新物理内存区的起始物理地址，参数 new_area_size 代表新物理
内存区的长度。函数首先调用 slab_is_available() 函数判断当前
slab 内存分配器是否可用，如果当前系统 memblock_can_resize 为
零，那么 MEMBLOCK 不支持动态拓展，那么直接返回 -1.

{% highlight c %}
        /* Calculate new doubled size */
        old_size = type->max * sizeof(struct memblock_region);
        new_size = old_size << 1;
        /*
         * We need to allocated new one align to PAGE_SIZE,
         *   so we can free them completely later.
         */
        old_alloc_size = PAGE_ALIGN(old_size);
        new_alloc_size = PAGE_ALIGN(new_size);

        /* Retrieve the slab flag */
        if (type == &memblock.memory)
                in_slab = &memblock_memory_in_slab;
        else
                in_slab = &memblock_reserved_in_slab;
{% endhighlight %}

函数首先计算原先 memblock_type 支持的最大物理内存数，然后将
长度扩大一倍存储到 new_size. 函数继续使用 PAGE_ALIGN() 函数
对 old_size 和 new_size 进行页对齐。如果 type 对应的类型是
可用物理内存区，那么读取 memblock_memory_in_slab 的值到
in_slab 中；如果 type 对应的类型是预留区物理内存，那么读取
memblock_reserved_in_slab 的值到 in_slab.

{% highlight c %}
        /* Try to find some space for it.
         *
         * WARNING: We assume that either slab_is_available() and we use it or
         * we use MEMBLOCK for allocations. That means that this is unsafe to
         * use when bootmem is currently active (unless bootmem itself is
         * implemented on top of MEMBLOCK which isn't the case yet)
         *
         * This should however not be an issue for now, as we currently only
         * call into MEMBLOCK while it's still active, or much later when slab
         * is active for memory hotplug operations
         */
        if (use_slab) {
                new_array = kmalloc(new_size, GFP_KERNEL);
                addr = new_array ? __pa(new_array) : 0;
        } else {
                /* only exclude range when trying to double reserved.regions */
                if (type != &memblock.reserved)
                        new_area_start = new_area_size = 0;

                addr = memblock_find_in_range(new_area_start + new_area_size,
                                                memblock.current_limit,
                                                new_alloc_size, PAGE_SIZE);
                if (!addr && new_area_size)
                        addr = memblock_find_in_range(0,
                                min(new_area_start, memblock.current_limit),
                                new_alloc_size, PAGE_SIZE);

                new_array = addr ? __va(addr) : NULL;
        }
{% endhighlight %}

如果此时 slab 内存分配器已经可以使用，那么 usb_slab 为 ture，
函数调用 kmalloc() 函数分配内存给 new_array, 如果分配成功，
则将 new_array 的物理地址存储在 addr。如果此时 slab 内存分配器
还不能使用，那么如果 type 指向预留区，那么 new_area_start 和
new_area_size 都设置为 0. 函数此时调用 memblock_find_in_range()
函数查找一块空闲的物理内存，如果没有找到，那么函数继续从 0，地址
重新查找物理内存，最后，如果找到，那么将找到的物理内存的起始地址
对应的虚拟地址存储在 new_arary 内；反之没有找到则将 new_array
设置为 NULL。

{% highlight c %}
        if (!addr) {
                pr_err("memblock: Failed to double %s array from %ld to %ld entries !\n",
                       type->name, type->max, type->max * 2);
                return -1;
        }

        new_end = addr + new_size - 1;
        memblock_dbg("memblock: %s is doubled to %ld at [%pa-%pa]",
                        type->name, type->max * 2, &addr, &new_end);

        /*
         * Found space, we now need to move the array over before we add the
         * reserved region since it may be our reserved array itself that is
         * full.
         */
        memcpy(new_array, type->regions, old_size);
        memset(new_array + type->max, 0, old_size);
        old_array = type->regions;
        type->regions = new_array;
        type->max <<= 1;
{% endhighlight %}

如果 addr 为 NULL，代表系统目前没有空闲的物理内存，则
系统打印错误信息之后返回 -1；更新 new_end 指向新分配的
物理内存起始地址加上 new_size 的地址上。函数首先调用
memcpy() 将原始的 regions 信息拷贝到新分配的物理内存
上，然后清零的 new_array+type->max 之后的物理内存，
继续更新 type 的 regions，使其指向新地址 new_array,
type->max 增大一倍。原始的 regions 存储在 old_array.

{% highlight c %}
        /* Free old array. We needn't free it if the array is the static one */
        if (*in_slab)
                kfree(old_array);
        else if (old_array != memblock_memory_init_regions &&
                 old_array != memblock_reserved_init_regions)
                memblock_free(__pa(old_array), old_alloc_size);

        /*
         * Reserve the new array if that comes from the memblock.  Otherwise, we
         * needn't do it
         */
        if (!use_slab)
                BUG_ON(memblock_reserve(addr, new_alloc_size));

        /* Update slab flag */
        *in_slab = use_slab;

        return 0;
{% endhighlight %}

接下来是释放原始 regions 占用的物理内存，如果 in_slab 不为 NULL，
即此时 slab 已经可以使用，那么直接调用 kfree() 函数释放 old_array;
反之函数调用 memblock_free() 函数释放原始的 regions 所占用的物理
内存。最后更新 in_slab 的值。

> - [memblock_find_in_range](#A0156)
>
> - [memblock_free](#A0162)

------------------------------------

#### <span id="A0164">memblock_merge_regions</span>

{% highlight c %}
/**
 * memblock_merge_regions - merge neighboring compatible regions
 * @type: memblock type to scan
 *
 * Scan @type and merge neighboring compatible regions.
 */
static void __init_memblock memblock_merge_regions(struct memblock_type *type)
{
        int i = 0;

        /* cnt never goes below 1 */
        while (i < type->cnt - 1) {
                struct memblock_region *this = &type->regions[i];
                struct memblock_region *next = &type->regions[i + 1];

                if (this->base + this->size != next->base ||
                    memblock_get_region_node(this) !=
                    memblock_get_region_node(next) ||
                    this->flags != next->flags) {
                        BUG_ON(this->base + this->size > next->base);
                        i++;
                        continue;
                }

                this->size += next->size;
                /* move forward from next + 1, index of which is i + 2 */
                memmove(next, next + 1, (type->cnt - (i + 2)) * sizeof(*next));
                type->cnt--;
        }
}
{% endhighlight %}

memblock_merge_regions() 函数的作用用于合并特定 memblock_type
内的 regions。参数 type 指向特定的 memblock_type。函数使用一个
while() 循环，使用 i 作为循环索引，遍历所有 region。每次遍历到一个
新的 region，将其存储到 this 指针，并将其下一个 region 存储到
next 指针。通过判断这两个是否能够合并成一个 region，如果两个
region 正好相连，并且 flags 和 NUMA 都相同，那么两个 region
可以合并成一个 region，函数通过更新 this 对应 region 的信息
实现合并。合并完之后，调用 memmove() 函数将 next+1 region 及其
之后的 region 都向前移动一个 region。最后将 type 的 cnt 减一，
并进入下一次循环。

> - [memblock_get_region_node](#A0136)

------------------------------------

#### <span id="A0165">memblock_add_range</span>

{% highlight c %}
/**
 * memblock_add_range - add new memblock region
 * @type: memblock type to add new region into
 * @base: base address of the new region
 * @size: size of the new region
 * @nid: nid of the new region
 * @flags: flags of the new region
 *
 * Add new memblock region [@base, @base + @size) into @type.  The new region
 * is allowed to overlap with existing ones - overlaps don't affect already
 * existing regions.  @type is guaranteed to be minimal (all neighbouring
 * compatible regions are merged) after the addition.
 *
 * Return:
 * 0 on success, -errno on failure.
 */
int __init_memblock memblock_add_range(struct memblock_type *type,
                                phys_addr_t base, phys_addr_t size,
                                int nid, enum memblock_flags flags)
{
        bool insert = false;
        phys_addr_t obase = base;
        phys_addr_t end = base + memblock_cap_size(base, &size);
        int idx, nr_new;
        struct memblock_region *rgn;

        if (!size)
                return 0;

        /* special case for empty array */
        if (type->regions[0].size == 0) {
                WARN_ON(type->cnt != 1 || type->total_size);
                type->regions[0].base = base;
                type->regions[0].size = size;
                type->regions[0].flags = flags;
                memblock_set_region_node(&type->regions[0], nid);
                type->total_size = size;
                return 0;
        }
repeat:
        /*
         * The following is executed twice.  Once with %false @insert and
         * then with %true.  The first counts the number of regions needed
         * to accommodate the new area.  The second actually inserts them.
         */
        base = obase;
        nr_new = 0;

        for_each_memblock_type(idx, type, rgn) {
                phys_addr_t rbase = rgn->base;
                phys_addr_t rend = rbase + rgn->size;

                if (rbase >= end)
                        break;
                if (rend <= base)
                        continue;
                /*
                 * @rgn overlaps.  If it separates the lower part of new
                 * area, insert that portion.
                 */
                if (rbase > base) {
#ifdef CONFIG_HAVE_MEMBLOCK_NODE_MAP
                        WARN_ON(nid != memblock_get_region_node(rgn));
#endif
                        WARN_ON(flags != rgn->flags);
                        nr_new++;
                        if (insert)
                                memblock_insert_region(type, idx++, base,
                                                       rbase - base, nid,
                                                       flags);
                }
                /* area below @rend is dealt with, forget about it */
                base = min(rend, end);
        }

        /* insert the remaining portion */
        if (base < end) {
                nr_new++;
                if (insert)
                        memblock_insert_region(type, idx, base, end - base,
                                               nid, flags);
        }

        if (!nr_new)
                return 0;
        /*
         * If this was the first round, resize array and repeat for actual
         * insertions; otherwise, merge and return.
         */
        if (!insert) {
                while (type->cnt + nr_new > type->max)
                        if (memblock_double_array(type, obase, size) < 0)
                                return -ENOMEM;
                insert = true;
                goto repeat;
        } else {
                memblock_merge_regions(type);
                return 0;
        }
}
{% endhighlight %}

memblock_add_range() 函数的作用是将一块物理内存块插入到内存区里面，
内存区可以是物理内存区，也可以是预留区。代码很长，分段解析：

{% highlight c %}
/**
 * memblock_add_range - add new memblock region
 * @type: memblock type to add new region into
 * @base: base address of the new region
 * @size: size of the new region
 * @nid: nid of the new region
 * @flags: flags of the new region
 *
 * Add new memblock region [@base, @base + @size) into @type.  The new region
 * is allowed to overlap with existing ones - overlaps don't affect already
 * existing regions.  @type is guaranteed to be minimal (all neighbouring
 * compatible regions are merged) after the addition.
 *
 * Return:
 * 0 on success, -errno on failure.
 */
int __init_memblock memblock_add_range(struct memblock_type *type,
                phys_addr_t base, phys_addr_t size,
                int nid, enum memblock_flags flags)
{
    bool insert = false;
    phys_addr_t obase = base;
    phys_addr_t end = base + memblock_cap_size(base, &size);
    int idx, nr_new;
    struct memblock_region *rgn;

    if (!size)
        return 0;
{% endhighlight %}

参数 type 指向了内存区，由上面调用的函数可知，这里指向预留内存区；base 指向新加入的
内存块的基地址; size 指向新加入的内核块的长度； nid 指向 NUMA 节点; flags 指向新加
入内存块对应的 flags。

函数首先调用 memblock_cap_size() 函数与 base 参数相加，以此计算新加入内存块的最后
后的物理地址。如果 size 参数为零，那么函数不做任何操作直接返回 0.

{% highlight c %}
/* special case for empty array */
if (type->regions[0].size == 0) {
	WARN_ON(type->cnt != 1 || type->total_size);
	type->regions[0].base = base;
	type->regions[0].size = size;
	type->regions[0].flags = flags;
	memblock_set_region_node(&type->regions[0], nid);
	type->total_size = size;
	return 0;
}
{% endhighlight %}

函数首先检查参数 type->regions[0].size，以此判断该内存区内是不是不包含其他内存区块，
由于内存区内的所有内存区块都是按其首地址从低到高排列，如果第一个内存区块的长度为 0，
那么函数基本认为这个内存区可能为空，但不能确定。函数继续检查内存区的 cnt 变量，这个变
量统计内存区内内存块的数量，有内存区的初始化可知，内存区的 cnt 为 1 时，表示内存区内
不含任何内存区块；函数也会检查，如果内存区的 total_size 不为零，那么内存区函数内存区
块，但是函数期望的是不含有任何内存区块，如果含有，内核就会报错。

但检查到的该内存区内不包含任何内存区块是，新加入的内存区块就是第一块，函数直接将新的
内存区块放到数组的首成员，如上述代码，执行完之后，函数就返回 0.

{% highlight c %}
repeat:
	/*
	 * The following is executed twice.  Once with %false @insert and
	 * then with %true.  The first counts the number of regions needed
	 * to accommodate the new area.  The second actually inserts them.
	 */
	base = obase;
	nr_new = 0;

	for_each_memblock_type(idx, type, rgn) {
		phys_addr_t rbase = rgn->base;
		phys_addr_t rend = rbase + rgn->size;

		if (rbase >= end)
			break;
		if (rend <= base)
			continue;
		/*
		 * @rgn overlaps.  If it separates the lower part of new
		 * area, insert that portion.
		 */
		if (rbase > base) {
#ifdef CONFIG_HAVE_MEMBLOCK_NODE_MAP
			WARN_ON(nid != memblock_get_region_node(rgn));
#endif
			WARN_ON(flags != rgn->flags);
			nr_new++;
			if (insert)
				memblock_insert_region(type, idx++, base,
						       rbase - base, nid,
						       flags);
		}
		/* area below @rend is dealt with, forget about it */
		base = min(rend, end);
	}

	/* insert the remaining portion */
	if (base < end) {
		nr_new++;
		if (insert)
			memblock_insert_region(type, idx, base, end - base,
					       nid, flags);
	}
{% endhighlight %}

如果内存区内已经包含其他的内存区块，那么函数就会继续执行如下代码。函数首先调用
for_each_memblock_type() 函数遍历该内存区内的所有内存区块，每遍历到一个内存区块，
函数会将新的内存区块和该内存区块进行比较，这两个内存区块一共会出现 11 种情况，但函数
将这么多的情况分作三种进行统一处理：

#### 遍历到的内存区块的起始地址大于或等于新内存区块的结束地址，新的内存区块位于遍历到内存区块的前端

对于这类，会存在两种情况，分别为：

{% highlight bash %}
1） rbase > end

 base                    end        rbase               rend
 +-----------------------+          +-------------------+
 |                       |          |                   |
 | New region            |          | Exist regions     |
 |                       |          |                   |
 +-----------------------+          +-------------------+

2）rbase == endi

                         rbase                      rend
                        | <----------------------> |
 +----------------------+--------------------------+
 |                      |                          |
 | New region           | Exist regions            |
 |                      |                          |
 +----------------------+--------------------------+
 | <------------------> |
 base                   end

{% endhighlight %}

对于这类情况，函数会直接退出 for_each_memblock() 循环，直接进入下一个判断，此时新内
存块的基地址都小于其结束地址，这样函数就会将新的内存块加入到内存区的链表中去

#### 遍历到的内存区块的终止地址小于或等于新内存区块的起始地址, 新的内存区块位于遍历到内存区块的后面

对于这类情况，会存在两种情况，分别为：

{% highlight bash %}
1） base > rend
 rbase                rend         base                  end
 +--------------------+            +---------------------+
 |                    |            |                     |
 |   Exist regions    |            |      new region     |
 |                    |            |                     |
 +--------------------+            +---------------------+

2) base == rend
                      base
 rbase                rend                     end
 +--------------------+------------------------+
 |                    |                        |
 |   Exist regions    |       new region       |
 |                    |                        |
 +--------------------+------------------------+
{% endhighlight %}

对于这类情况，函数会在 for_each_memblock() 中继续循环遍历剩下的节点，直到找到新加的
内存区块与已遍历到的内存区块存在其他类情况。也可能出现遍历的内存区块是内存区最后一块
内存区块，那么函数就会结束 for_each_memblock() 的循环，这样的话新内存区块还是和最后
一块已遍历的内存区块保持这样的关系。接着函数检查到新的内存区块的基地址小于其结束地址，
那么函数就将这块内存区块加入到内存区链表内。

#### 其他情况,两个内存区块存在重叠部分

剩余的情况中，新的内存区块都与已遍历到的内存区块存在重叠部分，但可以分做两种情况进行处
理：

> 新内存区块不重叠部分位于已遍历内存区块的前部
>
> 新内存区块不重叠部分位于已遍历内存区块的后部

对于第一种情况，典型的模型如下：

{% highlight bash %}
                 rbase     Exist regions        rend
                 | <--------------------------> |
 +---------------+--------+---------------------+
 |               |        |                     |
 |               |        |                     |
 |               |        |                     |
 +---------------+--------+---------------------+
 | <--------------------> |
 base   New region        end
{% endhighlight %}

当然还有其他几种几种也满足这种情况，但这种情况的显著特征就是不重叠部分位于已遍历的内存
区块的前部。对于这种情况，函数在调用 for_each_memblock() 循环的时候，只要探测到这种
情况的时候，函数就会直接调用 memblock_insert_region() 函数将不重叠部分直接加入到内存
区链表里，新加入的部分在内存区链表中位于已遍历内存区块的前面。执行完上面的函数之后，
调用 min 函数重新调整新内存区块的基地址，新调整的内存区块可能 base 与 end 也可能出现
两种情况：

> base < end
>
> base == end

如果 base == end 情况，那么新内存区块在这部分代码段已经执行完成。对于 base 小于 end
的情况，函数继续调用 memblock_insert_region() 函数将剩下的内存区块加入到内存区块
链表内。

对于第二种情况，典型的模型如下图：

{% highlight bash %}
* rbase                     rend
* | <---------------------> |
* +----------------+--------+----------------------+
* |                |        |                      |
* | Exist regions  |        |                      |
* |                |        |                      |
* +----------------+--------+----------------------+
*                  | <---------------------------> |
*                  base      new region            end
{% endhighlight %}

对于这种情况，函数会继续在 for_each_memblock() 中循环，并且每次循环中，都调用 min
函数更新新内存区块的基地址，并不断循环，使其不予已存在的内存区块重叠或出现其他位置。
如果循环结束时，新的内存区块满足 base < end 的情况，那么就调用
memblock_insert_region() 函数将剩下的内存区块加入到内存区块链表里。

{% highlight c %}
if (!nr_new)
	return 0;

/*
 * If this was the first round, resize array and repeat for actual
 * insertions; otherwise, merge and return.
 */
if (!insert) {
	while (type->cnt + nr_new > type->max)
		if (memblock_double_array(type, obase, size) < 0)
			return -ENOMEM;
	insert = true;
	goto repeat;
} else {
	memblock_merge_regions(type);
	return 0;
}
{% endhighlight %}

接下来的代码片段首先检查 nr_new 参数，这个参数用于指定有没有新的内存区块需要加入到内
存区块链表。到这里大家通过实践运行发现有几个参数会很困惑：nr_new 和 insert，以及为什
么要 repeat？其实设计这部分代码的开发者的基本思路就是：第一次通过 insert 和 nr_new
变量只检查新的内存区块是否加入到内存区块以及要加入几个内存区块(在有的一个内存区块由于
与已经存在的内存区块存在重叠被分成了两块，所以这种情况下，一块新的内存区块加入时就需
要向内存区块链表中加入两块内存区块)，通过这样的检测之后，函数就在上面的代码中检测现
有的内存区是否能存储下这么多的内存区块，如果不能，则调用 memblock_double_array() 函
数增加现有内存区块链表的长度。检测完毕之后，函数就执行真正的加入工作，将新的内存区块
都加入到内存区块链表内。执行完以上操作之后，函数最后调用 memblock_merge_regions()
函数将内存区块链表中可以合并的内存区块进行合并。

> - [memblock_cap_size](#A0159)
>
> - [memblock_set_region_node](#A0135)
>
> - [for_each_memblock_type](#A0134)
>
> - [memblock_get_region_node](#A0136)
>
> - [memblock_insert_region](#A0137)
>
> - [memblock_double_array](#A0163)
>
> - [memblock_merge_region](#A0164)
>
> - [memblock_add_range 内核实践](https://biscuitos.github.io/blog/MMU-ARM32-MEMBLOCK-memblock_add_range/)

------------------------------------

#### <span id="A0166">memblock_add</span>

{% highlight c %}
/**
 * memblock_add - add new memblock region
 * @base: base address of the new region
 * @size: size of the new region
 *
 * Add new memblock region [@base, @base + @size) to the "memory"
 * type. See memblock_add_range() description for mode details
 *
 * Return:
 * 0 on success, -errno on failure.
 */
int __init_memblock memblock_add(phys_addr_t base, phys_addr_t size)
{
        phys_addr_t end = base + size - 1;

        memblock_dbg("memblock_add: [%pa-%pa] %pF\n",
                     &base, &end, (void *)_RET_IP_);

        return memblock_add_range(&memblock.memory, base, size, MAX_NUMNODES, 0);
}
{% endhighlight %}

memblock_add() 函数的作用是入一块可用的物理内存。
参数 base 指向要添加内存区块的起始物理地址；size 指向要添加内存
区块的大小。函数直接调用 memblock_add_range() 函数将内存区块添加到
memblock.memory 内存区。

> - [memblock_add_range](#A0165)
>
> - [memblock_add 内核实践](https://biscuitos.github.io/blog/MMU-ARM32-MEMBLOCK-memblock_add/)

------------------------------------

#### <span id="A0167">dump_stack_set_arch_desc</span>

{% highlight c %}
/**             
 * dump_stack_set_arch_desc - set arch-specific str to show with task dumps
 * @fmt: printf-style format string
 * @...: arguments for the format string
 *              
 * The configured string will be printed right after utsname during task
 * dumps.  Usually used to add arch-specific system identifiers.  If an
 * arch wants to make use of such an ID string, it should initialize this
 * as soon as possible during boot.
 */
void __init dump_stack_set_arch_desc(const char *fmt, ...)
{       
        va_list args;

        va_start(args, fmt);
        vsnprintf(dump_stack_arch_desc_str, sizeof(dump_stack_arch_desc_str),
                  fmt, args);
        va_end(args);
}
{% endhighlight %}

dump_stack_set_arch_desc() 函数用于将体系名字信息写入全局变量
dump_stack_arch_desc_str 里。dump_stack_arch_desc_str 变量用于
内核 dump stack 信息的时候，输出体系相关的信息。函数通过格式化输入，
将参数 fmt 的值存储到 dump_stack_arch_desc_str 指针指向的地址。

------------------------------------

#### <span id="A0168">pgd_index</span>

{% highlight c %}
/* to find an entry in a page-table-directory */
#define pgd_index(addr)         ((addr) >> PGDIR_SHIFT)
{% endhighlight %}

pgd_index() 函数用于获得虚拟地址 x 在也目录中的索引。
例如在二级页表的 32 为虚拟地址上，页目录和页表的布局如下：

![](https://raw.githubusercontent.com/EmulateSpace/PictureSet/master/BiscuitOS/boot/BOOT000222.png)

页目录的偏移值位于虚拟地址的最高端位置，可以通过将
虚拟地址向右移动 PGDIR_SHIFT 位后获得虚拟地址在
页目录中的偏移。

------------------------------------

#### <span id="A0169">pgd_offset</span>

{% highlight c %}
#define pgd_offset(mm, addr)    ((mm)->pgd + pgd_index(addr))
{% endhighlight %}

pgd_offset() 函数用于获得虚拟地址对应的页目录内容。
参数 mm 指向进程对应的 mm_struct, 也就是进程的页目录。
参数 addr 指向虚拟地址。例如在二级页表中，页目录索引与
进程页目录的关系如下图：

![](https://raw.githubusercontent.com/EmulateSpace/PictureSet/master/BiscuitOS/boot/BOOT000223.png)

页目录索引通过 pgd_index() 函数获得，mm 参数的 pgd
指向了进程的页目录, 然后将页目录起始地址加上虚拟地址
的页目录索引，就可以得到虚拟地址在页目录中的内容。

> - [pgd_index](#A0168)

------------------------------------

#### <span id="A0170">pgd_offset_k</span>

{% highlight c %}
/* to find an entry in a kernel page-table-directory */
#define pgd_offset_k(addr)      pgd_offset(&init_mm, addr)
{% endhighlight %}

pgd_offset_k() 函数的作用是获得内核空间虚拟地址对应的页目录入口地址。
参数 addr 是一个内核空间的虚拟地址。函数通过 pgd_offset() 函数
实现，其中 init_mm 就是内核进程内存管理数据，其中包含了内核进程
所使用的页目录。例如在二级页表中，内核虚拟地址对应的页目录关系
如下：

![](https://raw.githubusercontent.com/EmulateSpace/PictureSet/master/BiscuitOS/boot/BOOT000224.png)

内核进程 task_struct 的 mm 成员指向了 init_mm 结构，
init_mm 包含了内核进程所使用的页目录，然后内核通过内核
提供的页目录与内核虚拟地址在页目录中的索引，就可以找到
内核虚拟地址对应的内核页目录入口地址。

------------------------------------

#### <span id="A0171">pud_offset</span>

{% highlight c %}
static inline pud_t *pud_offset(p4d_t *p4d, unsigned long address)
{
        return (pud_t *)p4d;
}
{% endhighlight %}

pud_offset() 函数用于从页目录中获得 PUD 入口地址，在没有四级页表
的设计中，PUD 的入口地址就是虚拟地址对应页目录项的值。

------------------------------------

#### <span id="A0172">pmd_offset</span>

{% highlight c %}
static inline pmd_t * pmd_offset(pud_t * pud, unsigned long address)
{
        return (pmd_t *)pud;
}
{% endhighlight %}

pmd_offset() 函数用于从 PUD 页表中获得 PMD 入口地址。在没有
四级页表的设计中，虚拟地址对应 PMD 值就是虚拟地址对应的
页目录值。

------------------------------------

#### <span id="A0173">fixmap_pmd</span>

{% highlight c %}
static inline pmd_t * __init fixmap_pmd(unsigned long addr)
{
        pgd_t *pgd = pgd_offset_k(addr);
        pud_t *pud = pud_offset(pgd, addr);
        pmd_t *pmd = pmd_offset(pud, addr);     

        return pmd;
}
{% endhighlight %}

fixmap_pmd() 函数的作用就是获得 FIXMAP 区间虚拟地址对应
的 PMD 入口地址。参数 addr 是 FIXMAP 区间的虚拟地址。
函数调用 pgd_offset_k() 函数获得 addr 虚拟地址对应的
内核页目录入口地址，基于页目录入口地址，调用 pud_offset()
函数获得虚拟地址对应的 PUD 入口地址，基于 PMD 入口地址，
调用 pmd_offset() 函数获得 PMD 入口地址。最后返回 pmd
入口地址。

> - [pgd_offset_k](#A0170)
>
> - [pud_offset](#A0171)
>
> - [pmd_offset](#A0172)

------------------------------------

#### <span id="A0174">flush_pmd_entry</span>

{% highlight c %}
/*
 *      flush_pmd_entry
 *
 *      Flush a PMD entry (word aligned, or double-word aligned) to
 *      RAM if the TLB for the CPU we are running on requires this.
 *      This is typically used when we are creating PMD entries.
 *
 *      clean_pmd_entry
 *
 *      Clean (but don't drain the write buffer) if the CPU requires
 *      these operations.  This is typically used when we are removing
 *      PMD entries.
 */
static inline void flush_pmd_entry(void *pmd)
{
        const unsigned int __tlb_flag = __cpu_tlb_flags;

        tlb_op(TLB_DCLEAN, "c7, c10, 1  @ flush_pmd", pmd);
        tlb_l2_op(TLB_L2CLEAN_FR, "c15, c9, 1  @ L2 flush_pmd", pmd);

        if (tlb_flag(TLB_WB))
                dsb(ishst);
}
{% endhighlight %}

flush_pmd_entry() 函数的目的是:
"Clean data or unified cache line by MVA to Poc"。在 cache 中，
每个 cache line 是 32 字节，而一个 pmd entry 是 4 字节，因此，
对于两个 pmd (pmd1, pmd2), 刷新 pmd2， pmd1 没必要重复刷新了。
这里可能会问： pmd1 和 pmd2 如果不在同一个 cache line 中，怎么办？
一个页表所占页也是 4KB，因此，如果成对刷新页表项的话，这个页表
对一定落在同一个 cache line 中。

------------------------------------

#### <span id="A0175">__pmd_populate</span>

{% highlight c %}
static inline void __pmd_populate(pmd_t *pmdp, phys_addr_t pte,
                                  pmdval_t prot)
{
        pmdval_t pmdval = (pte + PTE_HWTABLE_OFF) | prot;
        pmdp[0] = __pmd(pmdval);
#ifndef CONFIG_ARM_LPAE
        pmdp[1] = __pmd(pmdval + 256 * sizeof(pte_t));
#endif
        flush_pmd_entry(pmdp);
}
{% endhighlight %}

__pmd_populate() 函数的作用是向指定的 PMD 入口上填充 PTE
页表的物理地址与标志。参数 pmdp 指向 PMD 入口地址，参数 pte
是 PTE 页表的物理地址，参数 prot 为 PTE 页表在 PMD 入口中的
标志。在 Linux 的 PMD 入口中，PMD 具有两个入口 (pmd0 和 pmd1)，
如下图：

![](https://raw.githubusercontent.com/EmulateSpace/PictureSet/master/BiscuitOS/boot/BOOT000225.png)

如上图，一个 PTE 页表正好占用 4K 的页，每个 pte 入口占用 4 字节，
所以一个 PTE 页表可以包含 1024 个 pte 入口。在 Linux 中，PTE 页表
的后 512 个入口给硬件使用，前 512 个入口给软件使用，Linux 对上面
的设计给出的解释如下：

{% highlight c %}
Hardware-wise, we have a two level page table structure, where the first
level has 4096 entries, and the second level has 256 entries.  Each entry
is one 32-bit word.  Most of the bits in the second level entry are used
by hardware, and there aren't any "accessed" and "dirty" bits.

Linux on the other hand has a three level page table structure, which can
be wrapped to fit a two level page table structure easily - using the PGD
and PTE only.  However, Linux also expects one "PTE" table per page, and
at least a "dirty" bit.

Therefore, we tweak the implementation slightly - we tell Linux that we
have 2048 entries in the first level, each of which is 8 bytes (iow, two
hardware pointers to the second level.)  The second level contains two
hardware PTE tables arranged contiguously, preceded by Linux versions
which contain the state information Linux needs.  We, therefore, end up
with 512 entries in the "PTE" level.

This leads to the page tables having the following layout:

   pgd             pte
|        |
+--------+
|        |       +------------+ +0
+- - - - +       | Linux pt 0 |
|        |       +------------+ +1024
+--------+ +0    | Linux pt 1 |
|        |-----> +------------+ +2048
+- - - - + +4    |  h/w pt 0  |
|        |-----> +------------+ +3072
+--------+ +8    |  h/w pt 1  |
|        |       +------------+ +4096

See L_PTE_xxx below for definitions of bits in the "Linux pt", and
PTE_xxx for definitions of bits appearing in the "h/w pt".

PMD_xxx definitions refer to bits in the first level page table.
{% endhighlight %}

因此在上面的讨论之后，__pmd_populate() 函数首先定义了一个
局部变量 pmdval, 其值为 pte 页表的物理地址加上 PTE_HWTABLE_OFF，
使其指向 PTE 页表 h/w 的起始地址，然后在 pmdval 上加上 prot
的值。函数将 PMD 第一个入口的值设置为 pmdval，也就是指向
"PTE h/w pt 0", 如果系统没有定义 CONFIG_ARM_LPAE, 那么函数
继续将 PMD 第二个入口值设置为 "PTE h/w pt 1"。这样的操作实现了
上图所定义规则。设置完毕之后，函数调用 flush_pmd_entry() 进行
刷新新页表，是页表生效。

------------------------------------

#### <span id="A0176">pte_offset_early_fixmap</span>

{% highlight c %}
static pte_t * __init pte_offset_early_fixmap(pmd_t *dir, unsigned long addr)
{
        return &bm_pte[pte_index(addr)];
}
{% endhighlight %}

pte_offset_early_fixmap() 函数用于在系统初始化早期，通过虚拟地址获得
fixmap 区域内的 PTE 入口地址。bm_pte[] 数组存储着 FIXMAP 区域的 PTE
页表。

------------------------------------

#### <span id="A0177">early_fixmap_init</span>

{% highlight c %}
void __init early_fixmap_init(void)
{
        pmd_t *pmd;

        /*
         * The early fixmap range spans multiple pmds, for which
         * we are not prepared:
         */
        BUILD_BUG_ON((__fix_to_virt(__end_of_early_ioremap_region) >> PMD_SHIFT)
                     != FIXADDR_TOP >> PMD_SHIFT);

        pmd = fixmap_pmd(FIXADDR_TOP);
        pmd_populate_kernel(&init_mm, pmd, bm_pte);

        pte_offset_fixmap = pte_offset_early_fixmap;
}
{% endhighlight %}

early_fixmap_init() 函数的作用是初始化早期的 FIXMAP 内存。
函数首先调用 __fix_to_virt() 函数获得 FIXMAP 最后一个项的
虚拟地址对应的 PMD 入口，然后与 FIXADDR_TOP() 对应的 PMD 入口，
如果两者不相等，代表 FIXMAP 内存并按 FIXADDR_TOP 对齐。在
FIXMAP 内存区内，使用 FIXADDR_TOP 作为 FIXMAP 最后一个可以
分配的地址，此时调用 BUILD_BUG_ON() 函数进行报错。函数接着
调用 fixmap_pmd() 函数获得 FIXADDR_TOP 虚拟地址对应的 PMD
入口地址，存储在 pmd 变量里，接着函数调用 pmd_populate_kernel()
函数将 PMD 对应的入口指向 bm_pte, 以此建立 PMD 对应的 PTE
页表。最后将 pte_offset_fixmap 函数指向 pte_offset_early_fixmap(),
以此用于获得 FIXMAP 区域内虚拟地址对应的 PTE 入口地址。

> - [fixmap_pmd](#A0173)
>
> - [pmd_populate_kernel](#A0178)
>
> - [pte_offset_early_fixmap](#A0176)

------------------------------------

#### <span id="A0178">pmd_populate_kernel</span>

{% highlight c %}
/*
 * Populate the pmdp entry with a pointer to the pte.  This pmd is part
 * of the mm address space.
 *
 * Ensure that we always set both PMD entries.
 */
static inline void   
pmd_populate_kernel(struct mm_struct *mm, pmd_t *pmdp, pte_t *ptep)
{
        /*
         * The pmd must be loaded with the physical address of the PTE table
         */
        __pmd_populate(pmdp, __pa(ptep), _PAGE_KERNEL_TABLE);
}
{% endhighlight %}

pmd_populate_kernel() 函数的作用是为内核 PMD 入口填充 PTE 页表信息。
参数 mm 指向内核进程的 mm_struct, 参数 pmdp 指向 PMD 入口地址，参数
ptep 指向 PTE 页表。函数通过调用 __pmd_populate() 设置 PMD 入口的 PTE
页表，其中 PTE 页表必须是物理地址，_PAGE_KERNEL_TABLE 为内核对应的
PMD 入口标志。

> - [\_\_pmd_populate](#A0175)

------------------------------------

#### <span id="A0179">early_ioremap_setup</span>

{% highlight c %}
void __init early_ioremap_setup(void)
{       
        int i;

        for (i = 0; i < FIX_BTMAPS_SLOTS; i++)
                if (WARN_ON(prev_map[i]))
                        break;

        for (i = 0; i < FIX_BTMAPS_SLOTS; i++)
                slot_virt[i] = __fix_to_virt(FIX_BTMAP_BEGIN - NR_FIX_BTMAPS*i);
}
{% endhighlight %}

early_ioremap_setup() 函数的作用就是设置早期 I/O 映射的虚拟地址。
函数在 FIXMAP 内存区域分配了一块虚拟地址给早期 I/O 使用，一个包含
FIX_BTMAPS_SLOTS 个 slot，每个 slot 包含了 NR_FIX_BTMAPS，函数
调用 for 循环，通过 __fix_to_virt() 函数将所有 SLOT 对应的虚拟
地址存储在 slot_virt[] 数组里。

------------------------------------

#### <span id="A0180">early_ioremap_init</span>

{% highlight c %}
/*
 * Must be called after early_fixmap_init
 */     
void __init early_ioremap_init(void)
{       
        early_ioremap_setup();
}
{% endhighlight %}

early_ioremap_init() 函数用于初始化早期 I/O 使用的虚拟地址。
函数通过 early_ioremap_setup() 函数实现。

------------------------------------

#### <span id="A0181">isspace</span>

{% highlight c %}
#define isspace(c)      ((__ismask(c)&(_S)) != 0)
{% endhighlight %}

isspace() 函数判断字符 c 是否是一个空格。

------------------------------------

#### <span id="A0182">skip_spaces</span>

{% highlight c %}
/**             
 * skip_spaces - Removes leading whitespace from @str.
 * @str: The string to be stripped.
 *                      
 * Returns a pointer to the first non-whitespace character in @str.
 */             
char *skip_spaces(const char *str)
{                       
        while (isspace(*str))
                ++str;
        return (char *)str;
}       
EXPORT_SYMBOL(skip_spaces);
{% endhighlight %}

skip_spaces() 函数用于调过字符串 str 开头多个空格。
函数在 while() 循环中调用 isspace() 函数找到字符串
中的空格，然后加一跳过，直到字符串遇到非空格后停止循环。

> - [isspace](#A0181)

------------------------------------

#### <span id="A0183">next_arg</span>

{% highlight c %}
/*
 * Parse a string to get a param value pair.
 * You can use " around spaces, but can't escape ".
 * Hyphens and underscores equivalent in parameter names.
 */
char *next_arg(char *args, char **param, char **val)
{
        unsigned int i, equals = 0;
        int in_quote = 0, quoted = 0;
        char *next;

        if (*args == '"') {
                args++;
                in_quote = 1;
                quoted = 1;
        }

        for (i = 0; args[i]; i++) {
                if (isspace(args[i]) && !in_quote)
                        break;
                if (equals == 0) {
                        if (args[i] == '=')
                                equals = i;
                }
                if (args[i] == '"')
                        in_quote = !in_quote;
        }

        *param = args;
        if (!equals)
                *val = NULL;
        else {
                args[equals] = '\0';
                *val = args + equals + 1;

                /* Don't include quotes in value. */
                if (**val == '"') {
                        (*val)++;
                        if (args[i-1] == '"')
                                args[i-1] = '\0';
                }
        }
        if (quoted && args[i-1] == '"')
                args[i-1] = '\0';

        if (args[i]) {
                args[i] = '\0';
                next = args + i + 1;
        } else
                next = args + i;

        /* Chew up trailing spaces. */
        return skip_spaces(next);
}
{% endhighlight %}

next_arg() 函数用于从 cmdline 中读取一个项目，包括项目的名
字以及项目的值。代码较长，分段解析：

{% highlight c %}
char *next_arg(char *args, char **param, char **val)
{
        unsigned int i, equals = 0;
        int in_quote = 0, quoted = 0;
        char *next;

        if (*args == '"') {
                args++;
                in_quote = 1;
                quoted = 1;
        }
{% endhighlight %}

参数 args 指向 cmdline 字符串，参数 param 用于存储解析
项目的名字，val 参数用于存储解析项目的值。函数首先判断 args
字符串的第一个字符是否为 `"`, 如果是则将 args 指向下一个
字符，并将 in_quote 和 quoted 都设置为 1，代表目前字符串
位于双引号之间。

{% highlight c %}
        for (i = 0; args[i]; i++) {
                if (isspace(args[i]) && !in_quote)
                        break;
                if (equals == 0) {
                        if (args[i] == '=')
                                equals = i;
                }
                if (args[i] == '"')
                        in_quote = !in_quote;
        }
{% endhighlight %}

函数从头遍历 args 字符串所指的字符串，如果遍历到的字符串是
一个空格，且不在双引号里面，那么找到一个完整的项目，结束循环；
反之如果 equals 为 0，且遍历到的字符是 `=`，那么将 i 的值
赋值给 equals。如果遍历到的字符是 `"`，那么将 in_quote 取反。

{% highlight c %}
        *param = args;
        if (!equals)
                *val = NULL;
        else {
                args[equals] = '\0';
                *val = args + equals + 1;

                /* Don't include quotes in value. */
                if (**val == '"') {
                        (*val)++;
                        if (args[i-1] == '"')
                                args[i-1] = '\0';
                }
        }
{% endhighlight %}

将 *param 指向 args 所指的字符串，如果此时 equals 为 0，那么
表示该项目没有值，所以将 *val 设置为 NULL；反之将 args 字符串
从 `=` 处截断，args 只指向项目的名字，将项目的值存储到 *val
里面。如果 *val 的值为 `"`, 那么将 *val 去掉，即不包含 `"`，

{% highlight c %}
        if (quoted && args[i-1] == '"')
                args[i-1] = '\0';

        if (args[i]) {
                args[i] = '\0';
                next = args + i + 1;
        } else
                next = args + i;

        /* Chew up trailing spaces. */
        return skip_spaces(next);
{% endhighlight %}

最后对 args 指向字符串的位置进行更新，使其指向下一个项目，
并且调用 skip_spaces() 函数过略掉 args 开头的空格。

> - [isspace](#A0181)
>
> - [skip_spaces](#A0182)

------------------------------------

#### <span id="A0184">dash2underscore</span>

{% highlight c %}
static char dash2underscore(char c)
{        
        if (c == '-')
                return '_';
        return c;
}
{% endhighlight %}

dash2underscore() 函数的作用是将 `-` 字符转换成 `_`。
如果参数 c 是字符 `-`， 那么返回 `_`；反之直接返回字符。

------------------------------------

#### <span id="A0185">parameqn</span>

{% highlight c %}
bool parameqn(const char *a, const char *b, size_t n)
{
        size_t i;

        for (i = 0; i < n; i++) {
                if (dash2underscore(a[i]) != dash2underscore(b[i]))
                        return false;
        }
        return true;
}
{% endhighlight %}

parameqn() 函数的作用就是对比两个字符串的 n 个字符，如果
两个字符串在 n 个字符内不相等，那么返回 false；反之返回
true。

> - [dash2underscore](#A0184)

------------------------------------

#### <span id="A0186">parameq</span>

{% highlight c %}
bool parameq(const char *a, const char *b)
{
        return parameqn(a, b, strlen(a)+1);
}
{% endhighlight %}

parameq() 用于对比 a 字符串是否与 b 字符串相等。
如果相等，则返回 true；反之返回 false。函数通过
调用 parameqn() 函数对比字符串 a 是否与字符串 b
在 strlen(a)+1 个字符是否相等。

> - [parameqn](#A0185)

------------------------------------

#### <span id="A0187">parse_one</span>

{% highlight c %}
static int parse_one(char *param,
                     char *val,
                     const char *doing,
                     const struct kernel_param *params,
                     unsigned num_params,
                     s16 min_level,
                     s16 max_level,
                     void *arg,
                     int (*handle_unknown)(char *param, char *val,
                                     const char *doing, void *arg))
{
        unsigned int i;
        int err;

        /* Find parameter */
        for (i = 0; i < num_params; i++) {
                if (parameq(param, params[i].name)) {
                        if (params[i].level < min_level
                            || params[i].level > max_level)
                                return 0;
                        /* No one handled NULL, so do it here. */
                        if (!val &&
                            !(params[i].ops->flags & KERNEL_PARAM_OPS_FL_NOARG))
                                return -EINVAL;
                        pr_debug("handling %s with %p\n", param,
                                params[i].ops->set);
                        kernel_param_lock(params[i].mod);
                        param_check_unsafe(&params[i]);
                        err = params[i].ops->set(val, &params[i]);
                        kernel_param_unlock(params[i].mod);
                        return err;
                }
        }

        if (handle_unknown) {
                pr_debug("doing %s: %s='%s'\n", doing, param, val);
                return handle_unknown(param, val, doing, arg);
        }

        pr_debug("Unknown argument '%s'\n", param);
        return -ENOENT;
}
{% endhighlight %}

parse_one() 用于解析 cmdline 里面的一个参数，并在内核启动过程
中设置这个内核参数。由于代码太长，分段解析:

{% highlight c %}
static int parse_one(char *param,
                     char *val,
                     const char *doing,
                     const struct kernel_param *params,
                     unsigned num_params,
                     s16 min_level,
                     s16 max_level,
                     void *arg,
                     int (*handle_unknown)(char *param, char *val,
                                     const char *doing, void *arg))
{
{% endhighlight %}

参数 param 指向 cmdline 参数的名字，val 参数指向 cmdline 参数
的值，params 参数指向内核启动参数 section，num_params 参数代表
含有参数的个数。

{% highlight c %}
        /* Find parameter */
        for (i = 0; i < num_params; i++) {
                if (parameq(param, params[i].name)) {
                        if (params[i].level < min_level
                            || params[i].level > max_level)
                                return 0;
{% endhighlight %}

内核将所有启动参数的钩子函数存放在 "__param" section 里，
并且 params 指向该 section 开始的位置。在 "__param" section
内，每个成员按 struct kernel_param 格式存储。函数通过 for
循环遍历所有的 kernel_param, 函数调用 parmaeq() 函数判断
遍历到的 kernel_param name 成员是否和参数 param 相同，如果
相同，那么找到一个指定的 kernel_param.

{% highlight c %}
                        /* No one handled NULL, so do it here. */
                        if (!val &&
                            !(params[i].ops->flags & KERNEL_PARAM_OPS_FL_NOARG))
                                return -EINVAL;
                        pr_debug("handling %s with %p\n", param,
                                params[i].ops->set);
                        kernel_param_lock(params[i].mod);
                        param_check_unsafe(&params[i]);
                        err = params[i].ops->set(val, &params[i]);
                        kernel_param_unlock(params[i].mod);
                        return err;
                }
{% endhighlight %}

函数接下来对 kermel_param 里的成员与参数进行检测，如果检查
通过，那么函数上锁，然后执行 kernel_param 内包含的 set() 函数，
该函数就是用 cmdline 中的参数设置内核中启动的参数。设置完毕之后
解锁。最后返回。

{% highlight c %}
        if (handle_unknown) {
                pr_debug("doing %s: %s='%s'\n", doing, param, val);
                return handle_unknown(param, val, doing, arg);
        }

        pr_debug("Unknown argument '%s'\n", param);
        return -ENOENT;
{% endhighlight %}

如果没有从内核的 `__param` section 中找到指定的 kernel_param,
那么如果此时 handle_unknow() 存在，那么函数直接调用
handle_unknown() 函数并返回。如果 handle_unknown 也不
存在，那么函数直接返回 ENOENT。

> - [parameq](#A0186)

------------------------------------

#### <span id="A0188">parse_args</span>

{% highlight c %}
/* Args looks like "foo=bar,bar2 baz=fuz wiz". */
char *parse_args(const char *doing,
                 char *args,
                 const struct kernel_param *params,
                 unsigned num,
                 s16 min_level,
                 s16 max_level,
                 void *arg,
                 int (*unknown)(char *param, char *val,
                                const char *doing, void *arg))
{
        char *param, *val, *err = NULL;

        /* Chew leading spaces */
        args = skip_spaces(args);

        if (*args)
                pr_debug("doing %s, parsing ARGS: '%s'\n", doing, args);

        while (*args) {
                int ret;
                int irq_was_disabled;

                args = next_arg(args, &param, &val);
                /* Stop at -- */
                if (!val && strcmp(param, "--") == 0)
                        return err ?: args;
                irq_was_disabled = irqs_disabled();
                ret = parse_one(param, val, doing, params, num,
                                min_level, max_level, arg, unknown);
                if (irq_was_disabled && !irqs_disabled())
                        pr_warn("%s: option '%s' enabled irq's!\n",
                                doing, param);

                switch (ret) {
                case 0:
                        continue;
                case -ENOENT:
                        pr_err("%s: Unknown parameter `%s'\n", doing, param);
                        break;
                case -ENOSPC:
                        pr_err("%s: `%s' too large for parameter `%s'\n",
                               doing, val ?: "", param);
                        break;
                default:
                        pr_err("%s: `%s' invalid for parameter `%s'\n",
                               doing, val ?: "", param);
                        break;
                }

                err = ERR_PTR(ret);
        }

        return err;
}
{% endhighlight %}

parse_args() 函数用于从一个字符串中解析参数，并设置对应的内核
参数。由于代码太长，分段解析：

{% highlight c %}
char *parse_args(const char *doing,
                 char *args,
                 const struct kernel_param *params,
                 unsigned num,
                 s16 min_level,
                 s16 max_level,
                 void *arg,
                 int (*unknown)(char *param, char *val,
                                const char *doing, void *arg))
{
        char *param, *val, *err = NULL;

        /* Chew leading spaces */
        args = skip_spaces(args);

        if (*args)
                pr_debug("doing %s, parsing ARGS: '%s'\n", doing, args);
{% endhighlight %}

参数 doing 指向标识字符串，参数 args 指向包含启动参数的字符串，
参数 params 指向内核参数列表，参数 num 代表查找内核参数的数量，
参数 unknown 参数指向私有函数。函数首先调用 skip_space() 函数
将 args 参数开始处的空格去掉。

{% highlight c %}
        while (*args) {
                int ret;
                int irq_was_disabled;

                args = next_arg(args, &param, &val);
                /* Stop at -- */
                if (!val && strcmp(param, "--") == 0)
                        return err ?: args;
                irq_was_disabled = irqs_disabled();
                ret = parse_one(param, val, doing, params, num,
                                min_level, max_level, arg, unknown);
                if (irq_was_disabled && !irqs_disabled())
                        pr_warn("%s: option '%s' enabled irq's!\n",
                                doing, param);
{% endhighlight %}

函数使用 while 循环，只要 args 有效，循环不停止。函数首先
调用 next_arg() 函数从 args 字符串中获得一个参数的名字，
将其存储到 param 里，并从 args 字符串中获得参数名字对应的
参数值，并存储在 val 里面。函数接着判断当前中断是否使能，
函数将参数名字和参数值传递到 parse_one() 函数里，该函数
会在内核参数中找到与参数名字相同的内核参数，并将内核参数
的值设置成 val 的值。

{% highlight c %}
                switch (ret) {
                case 0:
                        continue;
                case -ENOENT:
                        pr_err("%s: Unknown parameter `%s'\n", doing, param);
                        break;
                case -ENOSPC:
                        pr_err("%s: `%s' too large for parameter `%s'\n",
                               doing, val ?: "", param);
                        break;
                default:
                        pr_err("%s: `%s' invalid for parameter `%s'\n",
                               doing, val ?: "", param);
                        break;
                }

                err = ERR_PTR(ret);
        }

        return err;
{% endhighlight %}

函数最后对 ret 处理结果进行说明。如果 ret 是错误值，那么函数
返回错误信息。

> - [skip_spaces](#A0182)
>
> - [next_arg](#A0183)
>
> - [parse_one](#A0187)

------------------------------------

#### <span id="A0189"></span>

{% highlight c %}
                switch (ret) {
                case 0:
                        continue;
                case -ENOENT:
                        pr_err("%s: Unknown parameter `%s'\n", doing, param);
                        break;
                case -ENOSPC:
                        pr_err("%s: `%s' too large for parameter `%s'\n",
                               doing, val ?: "", param);
                        break;
                default:
                        pr_err("%s: `%s' invalid for parameter `%s'\n",
                               doing, val ?: "", param);
                        break;
                }

                err = ERR_PTR(ret);
        }

        return err;
{% endhighlight %}

------------------------------------

#### <span id="A0190">do_early_param</span>

{% highlight c %}
/* Check for early params. */
static int __init do_early_param(char *param, char *val,
                                 const char *unused, void *arg)
{
        const struct obs_kernel_param *p;

        for (p = __setup_start; p < __setup_end; p++) {
                if ((p->early && parameq(param, p->str)) ||
                    (strcmp(param, "console") == 0 &&
                     strcmp(p->str, "earlycon") == 0)
                ) {
                        if (p->setup_func(val) != 0)
                                pr_warn("Malformed early option '%s'\n", param);
                }
        }
        /* We accept everything at this stage. */
        return 0;
}
{% endhighlight %}

do_early_param() 函数的作用是从 cmdline 中设置内核早期启动需要的
参数。cmdline 对应的参数在内核源码中可以通过 __setup() 函数进行
设置，函数会将设置的参数加入到 ".init.setup" section 内，并且
__setup_start 指向该 section 开始的位置，__setup_end 指向
该 section 结束的位置。参数 param 指向参数名字，参数 val
指向参数的值，参数 unused 代表是否使用过。函数使用 for() 循环，
从 __setup_start 开始遍历 ".init.setup" section 内的所有
obs_kernel_param 结构。每次遍历到的 obs_kernel_param 结构，
如果其 early 成员不为零，且 param 与 str 成员相同，或者
parma 与 "console" 相同，且 str 成员与 "earlycon" 相同，
那么此时执行 obs_kernel_param 的 setup_func 成员对应的
函数，如果函数执行完毕之后返回错误值，那么函数打印错误信息，

> - [parameq](#A0186)

------------------------------------

#### <span id="A0191">parse_early_options</span>

{% highlight c %}
void __init parse_early_options(char *cmdline)
{
        parse_args("early options", cmdline, NULL, 0, 0, 0, NULL,
                   do_early_param);
}
{% endhighlight %}

parse_early_options() 函数用于初始化 cmdline 内早期启动参数。
参数 cmdline 指向 CMDLINE，函数通过 parse_args() 函数实现。

> - [parse_args](#A0188)
>
> - [do_early_param](#A0190)

------------------------------------

#### <span id="A0192">parse_early_param</span>

{% highlight c %}
/* Arch code calls this early on, or if not, just before other parsing. */
void __init parse_early_param(void)
{
        static int done __initdata;
        static char tmp_cmdline[COMMAND_LINE_SIZE] __initdata;

        if (done)
                return;

        /* All fall through to do_early_param. */
        strlcpy(tmp_cmdline, boot_command_line, COMMAND_LINE_SIZE);
        parse_early_options(tmp_cmdline);
        done = 1;
}
{% endhighlight %}

parse_early_param() 函数用于系统启动早期，设置指定的启动参数。
函数定义了两个静态变量。函数首先判断 done 的值，以防止该函数被
二次执行。函数继续调用 strlcpy() 函数将 CMDLINE 字符串拷贝到
tmp_cmdline, 然后调用 parase_early_options() 函数解析系统
早期启动需要的参数，执行完毕之后，函数将 done 设置为 1.

> - [parse_early_options](#A0191)

------------------------------------

#### <span id="A0190"></span>

{% highlight c %}

{% endhighlight %}

------------------------------------

#### <span id="A0190"></span>

{% highlight c %}

{% endhighlight %}

------------------------------------

#### <span id="A0190"></span>

{% highlight c %}

{% endhighlight %}

------------------------------------

#### <span id="A0190"></span>

{% highlight c %}

{% endhighlight %}

------------------------------------

#### <span id="A0190"></span>

{% highlight c %}

{% endhighlight %}

------------------------------------

#### <span id="A0190"></span>

{% highlight c %}

{% endhighlight %}

------------------------------------

#### <span id="A0190"></span>

{% highlight c %}

{% endhighlight %}

------------------------------------

#### <span id="A0190"></span>

{% highlight c %}

{% endhighlight %}

------------------------------------

#### <span id="A0190"></span>

{% highlight c %}

{% endhighlight %}

------------------------------------

#### <span id="A0190"></span>

{% highlight c %}

{% endhighlight %}

------------------------------------

#### <span id="A0190"></span>

{% highlight c %}

{% endhighlight %}

------------------------------------

#### <span id="A0190"></span>

{% highlight c %}

{% endhighlight %}

------------------------------------

#### <span id="A0190"></span>

{% highlight c %}

{% endhighlight %}

------------------------------------

#### <span id="A0190"></span>

{% highlight c %}

{% endhighlight %}

------------------------------------

#### <span id="A0190"></span>

{% highlight c %}

{% endhighlight %}

------------------------------------

#### <span id="A0190"></span>

{% highlight c %}

{% endhighlight %}

------------------------------------

#### <span id="A0190"></span>

{% highlight c %}

{% endhighlight %}

------------------------------------

#### <span id="A0190"></span>

{% highlight c %}

{% endhighlight %}

------------------------------------

#### <span id="A0190"></span>

{% highlight c %}

{% endhighlight %}

------------------------------------

#### <span id="A0190"></span>

{% highlight c %}

{% endhighlight %}

------------------------------------

#### <span id="A0190"></span>

{% highlight c %}

{% endhighlight %}

------------------------------------

#### <span id="A0190"></span>

{% highlight c %}

{% endhighlight %}

------------------------------------

#### <span id="A0190"></span>

{% highlight c %}

{% endhighlight %}

------------------------------------

#### <span id="A0190"></span>

{% highlight c %}

{% endhighlight %}

------------------------------------

#### <span id="A0190"></span>

{% highlight c %}

{% endhighlight %}

------------------------------------

#### <span id="A0190"></span>

{% highlight c %}

{% endhighlight %}

------------------------------------

#### <span id="A0190"></span>

{% highlight c %}

{% endhighlight %}

------------------------------------

#### <span id="A0190"></span>

{% highlight c %}

{% endhighlight %}

------------------------------------

#### <span id="A0190"></span>

{% highlight c %}

{% endhighlight %}

------------------------------------

#### <span id="A0190"></span>

{% highlight c %}

{% endhighlight %}

------------------------------------

#### <span id="A0190"></span>

{% highlight c %}

{% endhighlight %}

------------------------------------

#### <span id="A0190"></span>

{% highlight c %}

{% endhighlight %}

------------------------------------

#### <span id="A0190"></span>

{% highlight c %}

{% endhighlight %}

------------------------------------

#### <span id="A0190"></span>

{% highlight c %}

{% endhighlight %}

------------------------------------

#### <span id="A0190"></span>

{% highlight c %}

{% endhighlight %}

------------------------------------

#### <span id="A0190"></span>

{% highlight c %}

{% endhighlight %}

------------------------------------

#### <span id="A0190"></span>

{% highlight c %}

{% endhighlight %}



## 赞赏一下吧 🙂

![MMU](https://raw.githubusercontent.com/EmulateSpace/PictureSet/master/BiscuitOS/kernel/HAB000036.jpg)

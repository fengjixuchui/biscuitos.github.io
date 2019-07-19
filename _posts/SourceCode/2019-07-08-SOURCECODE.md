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

{% highlight c %}
void __init setup_arch(char **cmdline_p)
{       
        const struct machine_desc *mdesc;

        setup_processor();
        mdesc = setup_machine_fdt(__atags_pointer);
{% endhighlight %}

ARMv7 Vexpress-a9 芯片上，setup_arch() 函数的实现如下，
由于函数较长，分段解析：

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

#### <span id="A00"></span>

{% highlight c %}

{% endhighlight %}

------------------------------------

#### <span id="A00"></span>

{% highlight c %}

{% endhighlight %}

------------------------------------

#### <span id="A00"></span>

{% highlight c %}

{% endhighlight %}

------------------------------------

#### <span id="A00"></span>

{% highlight c %}

{% endhighlight %}

------------------------------------

#### <span id="A00"></span>

{% highlight c %}

{% endhighlight %}

------------------------------------

#### <span id="A00"></span>

{% highlight c %}

{% endhighlight %}

------------------------------------

#### <span id="A00"></span>

{% highlight c %}

{% endhighlight %}

------------------------------------

#### <span id="A00"></span>

{% highlight c %}

{% endhighlight %}

------------------------------------

#### <span id="A00"></span>

{% highlight c %}

{% endhighlight %}

------------------------------------

#### <span id="A00"></span>

{% highlight c %}

{% endhighlight %}

------------------------------------

#### <span id="A00"></span>

{% highlight c %}

{% endhighlight %}

------------------------------------

#### <span id="A00"></span>

{% highlight c %}

{% endhighlight %}

------------------------------------

#### <span id="A00"></span>

{% highlight c %}

{% endhighlight %}

------------------------------------

#### <span id="A00"></span>

{% highlight c %}

{% endhighlight %}

------------------------------------

#### <span id="A00"></span>

{% highlight c %}

{% endhighlight %}

------------------------------------

#### <span id="A00"></span>

{% highlight c %}

{% endhighlight %}

------------------------------------

#### <span id="A00"></span>

{% highlight c %}

{% endhighlight %}

------------------------------------

#### <span id="A00"></span>

{% highlight c %}

{% endhighlight %}

------------------------------------

#### <span id="A00"></span>

{% highlight c %}

{% endhighlight %}



## 赞赏一下吧 🙂

![MMU](https://raw.githubusercontent.com/EmulateSpace/PictureSet/master/BiscuitOS/kernel/HAB000036.jpg)

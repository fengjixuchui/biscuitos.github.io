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

#### <span id="A000"></span>

{% highlight c %}

{% endhighlight %}

------------------------------------

#### <span id="A000"></span>

{% highlight c %}

{% endhighlight %}


## 赞赏一下吧 🙂

![MMU](https://raw.githubusercontent.com/EmulateSpace/PictureSet/master/BiscuitOS/kernel/HAB000036.jpg)

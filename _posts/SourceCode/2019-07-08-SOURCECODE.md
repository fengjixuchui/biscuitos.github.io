---
layout: post
title:  "start_kernel"
date:   2019-07-09 05:30:30 +0800
categories: [HW]
excerpt: start_kernel.
tags:
  - Tree
---

![DTS](/assets/PDB/BiscuitOS/kernel/IND00000H.jpg)

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

![](/assets/PDB/BiscuitOS/boot/BOOT000192.png)

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

![](/assets/PDB/BiscuitOS/boot/BOOT000193.png)

此时堆栈栈顶的位置就是 thread_info 起始地址加上 THREAD_SIZE - 1;
如果 CONFIG_STACK_GROWSUP 宏没有定义，那么说明堆栈是向下生长的，
此时 thread_info 与堆栈的位置关系如下图：

![](/assets/PDB/BiscuitOS/boot/BOOT000192.png)

此时堆栈栈顶的位置就是 thread_info 结束地址 + 1.

> - [task_thread_info](#A0003)
>
> - [Thread_info 与内核堆栈的关系](/blog/TASK-thread_info_stack/)

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

> - [Thread_info 与内核堆栈的关系](/blog/TASK-thread_info_stack/)

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

> - [read_cpuid_mpidr 实践](/blog/CPUID_read_cpuid_mpidr/)
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

![](/assets/PDB/BiscuitOS/boot/BOOT000194.png)

从上面的定义可以知道，每个 Affinity level 占用了 MPIDR_LEVEL_BITS
个位，level 0 为最低的 8 bit，所以函数通过向右移动
MPIDR_LEVEL_BITS 位获得下一个 Affinity level 数据。
MPDIR 不同的 Affinity level 代表不同的信息，其信息如下图：

![](/assets/PDB/BiscuitOS/boot/BOOT000195.png)

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

![](/assets/PDB/BiscuitOS/boot/BOOT000001.png)


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

> - [Thread_info 与内核堆栈的关系](/blog/TASK-thread_info_stack/)
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

> - [set_bit](/blog/BITMAP_set_bit/#%E6%BA%90%E7%A0%81%E5%88%86%E6%9E%90)
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

> - [clear_bit](/blog/BITMAP_clear_bit/#%E6%BA%90%E7%A0%81%E5%88%86%E6%9E%90)
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
> - [INIT_LIST_HEAD](/blog/LIST_INIT_LIST_HEAD/#%E6%BA%90%E7%A0%81%E5%88%86%E6%9E%90)
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

> - [ARMv7 Cortex-A9 proc_info_list](/blog/ARM-SCD-kernel-head.S/#ARMv7%20Cortex-A9%20proc_info_list)
>
> - [\_\_lookup_processor_type](/blog/ARM-SCD-kernel-head.S/#__lookup_processor_type)

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

![](/assets/PDB/BiscuitOS/boot/BOOT000025.png)

从 MDIR 寄存器的 Architecture 域存储着体系相关的识别码，如下图：

![](/assets/PDB/BiscuitOS/boot/BOOT000196
.png)

read_cpuid_id() 函数用于读取 ARM 的 MIDR 寄存器，然后
根据上面的 Architecture 域，判断当前系统 ARM 的体系版本号。
将 read_cpuid_id() 函数读取的值与 0x0008f000 相与，然后
通过相与的值可以获得真实的版本号。如果 ARMv7 ，那么 MDIR 寄
存器的 16:19 域为 0xf。当 ARM 的系统结构是 ARMv7 的时候，
__get_cpu_architecture() 函数继续调用 read_cpuid_ext()
函数判断具体 ARMv7 的版本信息。ID_MMFR0 寄存器的内存布局如下：

![](/assets/PDB/BiscuitOS/boot/BOOT000007
.png)

ID_MMFR0 寄存器中存储了体系内存模组相关的信息，如上图，
VMSA 域和 PMSA 域就明确指明目前体系对两者的支持情况。在
__get_cpu_architecture() 函数中，如果 VMSA 域值大于等于 3，
或者 PMSA 域大于 3，那么体系就是 ARMv7；如果 VMSA 域或者
PMSA 域等于 2，那么体系就是 ARMv6。因此 __get_cpu_architecture()
函数可以获得 ARM 的体系信息。

> - [read_cpuid_id](/blog/CPUID_read_cpuid_id/)

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

![](/assets/PDB/BiscuitOS/boot/BOOT000014
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

![](/assets/PDB/BiscuitOS/boot/BOOT000025.png)

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

![](/assets/PDB/BiscuitOS/boot/BOOT000197.png)

在 ID_ISAR0 寄存器中，bit 24:27 域说明如下：

![](/assets/PDB/BiscuitOS/boot/BOOT000198.png)

从上的定义可以知道，在 cpuid_init_hwcaps() 函数中，如果 24:27
域的值大于等于 2，那么支持 ARM 指令集的 SDIV 和 UDIV 指令，并将
这个结果通过宏 HWCAP_IDIVA 存储在全局变量 elf_hwcap 里；如果
24:27 域的值大于 1，那么系统支持 ARM 和 Thumb 指令集的 SDIV
和 UDIV 指令。接着函数继续调用 cpuid_feature_extract() 函数
读取 ID_MMFR0 (Memory Model Feature Register) 寄存器，
ID_MMFR0 寄存器用于描述系统内存模组的信息。其寄存器布局如下：

![](/assets/PDB/BiscuitOS/boot/BOOT000007.png)

cpuid_init_hwcaps() 函数读取了 ID_MMFR0 寄存器的低 4 bits，
ID_MMFR0 寄存器的低 4bit 域是 VMSA support 域，其定义如下：

![](/assets/PDB/BiscuitOS/boot/BOOT000199.png)

cpuid_init_hwcaps() 函数判断该域值是否大于 5，如果大于 5，
那么系统内存模组就支持 Long-descriptor 页表，那么系统就支持
LPAE，这样就原生支持 LPAE 模式下的 ldrd/strd 指令，此时如果
支持，函数通过 HWCAP_LPAE 宏存储在 elf_hwcap 全局变量里。
函数继续调用 read_cpuid_ext(CPUID_EXT_ISAR5) 函数获得
ID_ISAR5 寄存器，其在 ARMv7 中的内存布局如下：

![](/assets/PDB/BiscuitOS/boot/BOOT000200.png)

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

![](/assets/PDB/BiscuitOS/boot/BOOT000201.png)

> - [read_cpuid_id](/blog/CPUID_read_cpuid_id/)

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

![](/assets/PDB/BiscuitOS/boot/BOOT000201.png)

然后调用 read_cpuid_part() 函数判断当前 ARM 是否是
ARM_CPU_PART_ARM1136, 并且判断 MIDR 寄存器的 Variant 域
是否为 0，该域值与 “An IMPLEMENTATION DEFINED variant number”
有关，如果上面的条件满足，那么设置 elf_hwcap 全局变量与
~HWCAP_TLS 相与，并直接返回。如果函数此时没有返回，那么继续
执行下面的代码，接着判断当前系统的 MIDR 寄存器的 Variant 域，
如果当前体系没有实现这个域，那么函数直接返回；反之如果 MDIR
实现了 Variant 域，那么函数调用 cpuid_feature_extract() 函数
读取体系的 ID_ISAR3 寄存器，ID_ISAR4 寄存器，其寄存器布局如下：

![](/assets/PDB/BiscuitOS/boot/BOOT000202.png)

![](/assets/PDB/BiscuitOS/boot/BOOT000203.png)

elf_hwcap_fixup() 函数判断，如果 ID_ISAR3 的 bit 12:15 域值，
该域值的含义如下：

![](/assets/PDB/BiscuitOS/boot/BOOT000204.png)

该域值与 ID_ISAR4 寄存器的 SynchPrim_instrs_frac 域一同
判断体系是否实现了 Synchronization Primitive 指令，其中
ID_ISAR4 寄存器的 SynchPrim_instrs_frac 域定义如下：

![](/assets/PDB/BiscuitOS/boot/BOOT000205.png)

elf_hwcap_fixup() 函数此时判断，如果 ID_ISAR3 寄存器的
SynchPrim_instrs 域值大于 1，或者 ID_ISAR3 寄存器的
SynchPrim_instrs 域值等于 1，且 ID_ISAR4 寄存器的
SynchPrim_instrs_frac 域大于等于 3，那么函数将全局变量 elf_hwcap
的值与宏 ~HWCAP_SWP 相与。

> - [cpuid_feature_extract](#A0048)
>
> - [read_cpuid_id](/blog/CPUID_read_cpuid_id/)

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

![](/assets/PDB/BiscuitOS/boot/BOOT000206.png)

> - [read_cpuid](/blog/CPUID_read_cpuid/)

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

![](/assets/PDB/BiscuitOS/boot/BOOT000026.png)

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

![](/assets/PDB/BiscuitOS/boot/BOOT000028.png)

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

![](/assets/PDB/BiscuitOS/boot/BOOT000026.png)

ARMv7 中 CCSIDR 寄存器的寄存器布局如下：

![](/assets/PDB/BiscuitOS/boot/BOOT000028.png)

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

![](/assets/PDB/BiscuitOS/boot/BOOT000206.png)

如果 arch 参数表明当前体系是 ARMv7M 关于这个 cache type 的
0:3 域与 16:19 域为 0，这两个域分别是 IminLine 和 DminLine 域，
用于指定最小的 Line number 数。此时设置 cacheid 为 0；如果此时
cachetype 的 bit 29:31 等于 4，此时对应 CTR 寄存器的 Format 域，
其定义如下：

![](/assets/PDB/BiscuitOS/boot/BOOT000207.png)

如果 Format 等于 4，那么此时体系信息是 ARMv7. 此时将
cacheid 设置为 CACHEID_VIPT_NONALIASING。接着如果此时
CTR 的 bit 14:16 L1lp 域，即 Level 1 cache policy 域，
其定义为：

![](/assets/PDB/BiscuitOS/boot/BOOT000208.png)

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

> - [ARMv7 Cortex-A9mp proc_info](/blog/ARM-SCD-kernel-head.S/#ARMv7%20Cortex-A9%20proc_info_list)

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

![](/assets/PDB/BiscuitOS/boot/BOOT000001.png)

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
> - [ARMv7 Cortex-A9mp proc_info](/blog/ARM-SCD-kernel-head.S/#ARMv7%20Cortex-A9%20proc_info_list)

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

> - [read_cpuid_id](/blog/CPUID_read_cpuid_id/)
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
> - [DTB 二进制文件结构](/blog/DTS/#M00)
>
> - [fd_header 数据结构解析](/blog/DTS/#B010)

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
> - [fd_header 数据结构解析](/blog/DTS/#B010)

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
> - [fd_header 数据结构解析](/blog/DTS/#B010)

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
> - [fd_header 数据结构解析](/blog/DTS/#B010)

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
> - [fd_header 数据结构解析](/blog/DTS/#B010)

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
> - [fd_header 数据结构解析](/blog/DTS/#B010)

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
> - [fd_header 数据结构解析](/blog/DTS/#B010)

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
> - [fd_header 数据结构解析](/blog/DTS/#B010)

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
> - [fd_header 数据结构解析](/blog/DTS/#B010)

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
> - [fd_header 数据结构解析](/blog/DTS/#B010)

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
> - [fd_header 数据结构解析](/blog/DTS/#B010)

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

![](/assets/PDB/BiscuitOS/boot/BOOT000213.png)

> - [fdt_off_dt_struct](#A0079)
>
> - [DTB format describe](/blog/DTS/#M00)

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

![](/assets/PDB/BiscuitOS/boot/BOOT000214.png)

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
> - [\_\_next_mem_range 内核实践](/blog/MMU-ARM32-MEMBLOCK-__next_mem_range/)

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
> - [\_\_next_mem_range 内核实践](/blog/MMU-ARM32-MEMBLOCK-for_each_mem_range/)

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
> - [for_each_free_mem_range 内核实践](/blog/MMU-ARM32-MEMBLOCK-for_each_free_mem_range/)

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
> - [__next_mem_range_rev 内核实践](/blog/MMU-ARM32-MEMBLOCK-__next_mem_range_rev/#header)

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
> - [for_each_mem_range_rev 内核实践](/blog/MMU-ARM32-MEMBLOCK-for_each_mem_range_rev/)

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
> - [for_each_free_mem_range_reverse 内核实践](/blog/MMU-ARM32-MEMBLOCK-for_each_free_mem_range_reverse/)

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
> - [memblock_find_in_range_node 内核实践](/blog/MMU-ARM32-MEMBLOCK-memblock_find_in_range_node/)

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
> - [memblock_find_in_range 内核实践](/blog/MMU-ARM32-MEMBLOCK-memblock_find_in_range/)

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
> - [memblock_free 内核实践](/blog/MMU-ARM32-MEMBLOCK-memblock_free/)

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
> - [memblock_add_range 内核实践](/blog/MMU-ARM32-MEMBLOCK-memblock_add_range/)

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
> - [memblock_add 内核实践](/blog/MMU-ARM32-MEMBLOCK-memblock_add/)

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

![](/assets/PDB/BiscuitOS/boot/BOOT000222.png)

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

![](/assets/PDB/BiscuitOS/boot/BOOT000223.png)

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

![](/assets/PDB/BiscuitOS/boot/BOOT000224.png)

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

![](/assets/PDB/BiscuitOS/boot/BOOT000225.png)

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

#### <span id="A0193">build_mem_type_table</span>

{% highlight c %}
/*
 * Adjust the PMD section entries according to the CPU in use.
 */
static void __init build_mem_type_table(void)
{
        struct cachepolicy *cp;
        unsigned int cr = get_cr();
        pteval_t user_pgprot, kern_pgprot, vecs_pgprot;
        pteval_t hyp_device_pgprot, s2_pgprot, s2_device_pgprot;
        int cpu_arch = cpu_architecture();
        int i;

        if (cpu_arch < CPU_ARCH_ARMv6) {
#if defined(CONFIG_CPU_DCACHE_DISABLE)
                if (cachepolicy > CPOLICY_BUFFERED)
                        cachepolicy = CPOLICY_BUFFERED;
#elif defined(CONFIG_CPU_DCACHE_WRITETHROUGH)
                if (cachepolicy > CPOLICY_WRITETHROUGH)
                        cachepolicy = CPOLICY_WRITETHROUGH;
#endif
        }
        if (cpu_arch < CPU_ARCH_ARMv5) {
                if (cachepolicy >= CPOLICY_WRITEALLOC)
                        cachepolicy = CPOLICY_WRITEBACK;
                ecc_mask = 0;
        }

        if (is_smp()) {
                if (cachepolicy != CPOLICY_WRITEALLOC) {
                        pr_warn("Forcing write-allocate cache policy for SMP\n");
                        cachepolicy = CPOLICY_WRITEALLOC;
                }
                if (!(initial_pmd_value & PMD_SECT_S)) {
                        pr_warn("Forcing shared mappings for SMP\n");
                        initial_pmd_value |= PMD_SECT_S;
                }
        }

        /*
         * Strip out features not present on earlier architectures.
         * Pre-ARMv5 CPUs don't have TEX bits.  Pre-ARMv6 CPUs or those
         * without extended page tables don't have the 'Shared' bit.
         */
        if (cpu_arch < CPU_ARCH_ARMv5)
                for (i = 0; i < ARRAY_SIZE(mem_types); i++)
                        mem_types[i].prot_sect &= ~PMD_SECT_TEX(7);
        if ((cpu_arch < CPU_ARCH_ARMv6 || !(cr & CR_XP)) && !cpu_is_xsc3())
                for (i = 0; i < ARRAY_SIZE(mem_types); i++)
                        mem_types[i].prot_sect &= ~PMD_SECT_S;

        /*
         * ARMv5 and lower, bit 4 must be set for page tables (was: cache
         * "update-able on write" bit on ARM610).  However, Xscale and
         * Xscale3 require this bit to be cleared.
         */
        if (cpu_is_xscale_family()) {
                for (i = 0; i < ARRAY_SIZE(mem_types); i++) {
                        mem_types[i].prot_sect &= ~PMD_BIT4;
                        mem_types[i].prot_l1 &= ~PMD_BIT4;
                }
        } else if (cpu_arch < CPU_ARCH_ARMv6) {
                for (i = 0; i < ARRAY_SIZE(mem_types); i++) {
                        if (mem_types[i].prot_l1)
                                mem_types[i].prot_l1 |= PMD_BIT4;
                        if (mem_types[i].prot_sect)
                                mem_types[i].prot_sect |= PMD_BIT4;
                }
        }

        /*
         * Mark the device areas according to the CPU/architecture.
         */
        if (cpu_is_xsc3() || (cpu_arch >= CPU_ARCH_ARMv6 && (cr & CR_XP))) {
                if (!cpu_is_xsc3()) {
                        /*
                         * Mark device regions on ARMv6+ as execute-never
                         * to prevent speculative instruction fetches.
                         */
                        mem_types[MT_DEVICE].prot_sect |= PMD_SECT_XN;
                        mem_types[MT_DEVICE_NONSHARED].prot_sect |= PMD_SECT_XN;
                        mem_types[MT_DEVICE_CACHED].prot_sect |= PMD_SECT_XN;
                        mem_types[MT_DEVICE_WC].prot_sect |= PMD_SECT_XN;

                        /* Also setup NX memory mapping */
                        mem_types[MT_MEMORY_RW].prot_sect |= PMD_SECT_XN;
                }
                if (cpu_arch >= CPU_ARCH_ARMv7 && (cr & CR_TRE)) {
                        /*
                         * For ARMv7 with TEX remapping,
                         * - shared device is SXCB=1100
                         * - nonshared device is SXCB=0100
                         * - write combine device mem is SXCB=0001
                         * (Uncached Normal memory)
                         */
                        mem_types[MT_DEVICE].prot_sect |= PMD_SECT_TEX(1);
                        mem_types[MT_DEVICE_NONSHARED].prot_sect |= PMD_SECT_TEX(1);
                        mem_types[MT_DEVICE_WC].prot_sect |= PMD_SECT_BUFFERABLE;
                } else if (cpu_is_xsc3()) {
                        /*
                         * For Xscale3,
                         * - shared device is TEXCB=00101
                         * - nonshared device is TEXCB=01000
                         * - write combine device mem is TEXCB=00100
                         * (Inner/Outer Uncacheable in xsc3 parlance)
                         */
                        mem_types[MT_DEVICE].prot_sect |= PMD_SECT_TEX(1) | PMD_SECT_BUFFERED;
                        mem_types[MT_DEVICE_NONSHARED].prot_sect |= PMD_SECT_TEX(2);
                        mem_types[MT_DEVICE_WC].prot_sect |= PMD_SECT_TEX(1);
                } else {
                        /*
                         * For ARMv6 and ARMv7 without TEX remapping,
                         * - shared device is TEXCB=00001
                         * - nonshared device is TEXCB=01000
                         * - write combine device mem is TEXCB=00100
                         * (Uncached Normal in ARMv6 parlance).
                         */
                        mem_types[MT_DEVICE].prot_sect |= PMD_SECT_BUFFERED;
                        mem_types[MT_DEVICE_NONSHARED].prot_sect |= PMD_SECT_TEX(2);
                        mem_types[MT_DEVICE_WC].prot_sect |= PMD_SECT_TEX(1);
                }
        } else {
                /*
                 * On others, write combining is "Uncached/Buffered"
                 */
                mem_types[MT_DEVICE_WC].prot_sect |= PMD_SECT_BUFFERABLE;
        }

        /*
         * Now deal with the memory-type mappings
         */
        cp = &cache_policies[cachepolicy];
        vecs_pgprot = kern_pgprot = user_pgprot = cp->pte;
        s2_pgprot = cp->pte_s2;
        hyp_device_pgprot = mem_types[MT_DEVICE].prot_pte;
        s2_device_pgprot = mem_types[MT_DEVICE].prot_pte_s2;

#ifndef CONFIG_ARM_LPAE
        /*
         * We don't use domains on ARMv6 (since this causes problems with
         * v6/v7 kernels), so we must use a separate memory type for user
         * r/o, kernel r/w to map the vectors page.
         */
        if (cpu_arch == CPU_ARCH_ARMv6)
                vecs_pgprot |= L_PTE_MT_VECTORS;

        /*
         * Check is it with support for the PXN bit
         * in the Short-descriptor translation table format descriptors.
         */
        if (cpu_arch == CPU_ARCH_ARMv7 &&
                (read_cpuid_ext(CPUID_EXT_MMFR0) & 0xF) >= 4) {
                user_pmd_table |= PMD_PXNTABLE;
        }
#endif


        /*
         * ARMv6 and above have extended page tables.
         */
        if (cpu_arch >= CPU_ARCH_ARMv6 && (cr & CR_XP)) {
#ifndef CONFIG_ARM_LPAE
                /*
                 * Mark cache clean areas and XIP ROM read only
                 * from SVC mode and no access from userspace.
                 */
                mem_types[MT_ROM].prot_sect |= PMD_SECT_APX|PMD_SECT_AP_WRITE;
                mem_types[MT_MINICLEAN].prot_sect |= PMD_SECT_APX|PMD_SECT_AP_WRITE;
                mem_types[MT_CACHECLEAN].prot_sect |= PMD_SECT_APX|PMD_SECT_AP_WRITE;
#endif

                /*
                 * If the initial page tables were created with the S bit
                 * set, then we need to do the same here for the same
                 * reasons given in early_cachepolicy().
                 */
                if (initial_pmd_value & PMD_SECT_S) {
                        user_pgprot |= L_PTE_SHARED;
                        kern_pgprot |= L_PTE_SHARED;
                        vecs_pgprot |= L_PTE_SHARED;
                        s2_pgprot |= L_PTE_SHARED;
                        mem_types[MT_DEVICE_WC].prot_sect |= PMD_SECT_S;
                        mem_types[MT_DEVICE_WC].prot_pte |= L_PTE_SHARED;
                        mem_types[MT_DEVICE_CACHED].prot_sect |= PMD_SECT_S;
                        mem_types[MT_DEVICE_CACHED].prot_pte |= L_PTE_SHARED;
                        mem_types[MT_MEMORY_RWX].prot_sect |= PMD_SECT_S;
                        mem_types[MT_MEMORY_RWX].prot_pte |= L_PTE_SHARED;
                        mem_types[MT_MEMORY_RW].prot_sect |= PMD_SECT_S;
                        mem_types[MT_MEMORY_RW].prot_pte |= L_PTE_SHARED;
                        mem_types[MT_MEMORY_DMA_READY].prot_pte |= L_PTE_SHARED;
                        mem_types[MT_MEMORY_RWX_NONCACHED].prot_sect |= PMD_SECT_S;
                        mem_types[MT_MEMORY_RWX_NONCACHED].prot_pte |= L_PTE_SHARED;
                }
        }


        /*
         * Non-cacheable Normal - intended for memory areas that must
         * not cause dirty cache line writebacks when used
         */
        if (cpu_arch >= CPU_ARCH_ARMv6) {
                if (cpu_arch >= CPU_ARCH_ARMv7 && (cr & CR_TRE)) {
                        /* Non-cacheable Normal is XCB = 001 */
                        mem_types[MT_MEMORY_RWX_NONCACHED].prot_sect |=
                                PMD_SECT_BUFFERED;
                } else {
                        /* For both ARMv6 and non-TEX-remapping ARMv7 */
                        mem_types[MT_MEMORY_RWX_NONCACHED].prot_sect |=
                                PMD_SECT_TEX(1);
                }
        } else {
                mem_types[MT_MEMORY_RWX_NONCACHED].prot_sect |= PMD_SECT_BUFFERABLE;
        }

#ifdef CONFIG_ARM_LPAE
        /*
         * Do not generate access flag faults for the kernel mappings.
         */
        for (i = 0; i < ARRAY_SIZE(mem_types); i++) {
                mem_types[i].prot_pte |= PTE_EXT_AF;
                if (mem_types[i].prot_sect)
                        mem_types[i].prot_sect |= PMD_SECT_AF;
        }
        kern_pgprot |= PTE_EXT_AF;
        vecs_pgprot |= PTE_EXT_AF;

        /*
         * Set PXN for user mappings
         */
        user_pgprot |= PTE_EXT_PXN;
#endif


        for (i = 0; i < 16; i++) {
                pteval_t v = pgprot_val(protection_map[i]);
                protection_map[i] = __pgprot(v | user_pgprot);
        }

        mem_types[MT_LOW_VECTORS].prot_pte |= vecs_pgprot;
        mem_types[MT_HIGH_VECTORS].prot_pte |= vecs_pgprot;

        pgprot_user   = __pgprot(L_PTE_PRESENT | L_PTE_YOUNG | user_pgprot);
        pgprot_kernel = __pgprot(L_PTE_PRESENT | L_PTE_YOUNG |
                                 L_PTE_DIRTY | kern_pgprot);
        pgprot_s2  = __pgprot(L_PTE_PRESENT | L_PTE_YOUNG | s2_pgprot);
        pgprot_s2_device  = __pgprot(s2_device_pgprot);
        pgprot_hyp_device  = __pgprot(hyp_device_pgprot);

        mem_types[MT_LOW_VECTORS].prot_l1 |= ecc_mask;
        mem_types[MT_HIGH_VECTORS].prot_l1 |= ecc_mask;
        mem_types[MT_MEMORY_RWX].prot_sect |= ecc_mask | cp->pmd;
        mem_types[MT_MEMORY_RWX].prot_pte |= kern_pgprot;
        mem_types[MT_MEMORY_RW].prot_sect |= ecc_mask | cp->pmd;
        mem_types[MT_MEMORY_RW].prot_pte |= kern_pgprot;
        mem_types[MT_MEMORY_DMA_READY].prot_pte |= kern_pgprot;
        mem_types[MT_MEMORY_RWX_NONCACHED].prot_sect |= ecc_mask;
        mem_types[MT_ROM].prot_sect |= cp->pmd;

        switch (cp->pmd) {
        case PMD_SECT_WT:
                mem_types[MT_CACHECLEAN].prot_sect |= PMD_SECT_WT;
                break;
        case PMD_SECT_WB:
        case PMD_SECT_WBWA:
                mem_types[MT_CACHECLEAN].prot_sect |= PMD_SECT_WB;
                break;
        }
        pr_info("Memory policy: %sData cache %s\n",
                ecc_mask ? "ECC enabled, " : "", cp->policy);

        for (i = 0; i < ARRAY_SIZE(mem_types); i++) {
                struct mem_type *t = &mem_types[i];
                if (t->prot_l1)
                        t->prot_l1 |= PMD_DOMAIN(t->domain);
                if (t->prot_sect)
                        t->prot_sect |= PMD_DOMAIN(t->domain);
        }
}
{% endhighlight %}

build_mem_type_table() 函数的作用就是，由于函数较长并且兼容多个 ARM
版本，本节只分析与 ARMv7 相关的代码：

{% highlight c %}
        /*
         * Mark the device areas according to the CPU/architecture.
         */
        if (cpu_is_xsc3() || (cpu_arch >= CPU_ARCH_ARMv6 && (cr & CR_XP))) {
                if (!cpu_is_xsc3()) {
                        /*
                         * Mark device regions on ARMv6+ as execute-never
                         * to prevent speculative instruction fetches.
                         */
                        mem_types[MT_DEVICE].prot_sect |= PMD_SECT_XN;
                        mem_types[MT_DEVICE_NONSHARED].prot_sect |= PMD_SECT_XN;
                        mem_types[MT_DEVICE_CACHED].prot_sect |= PMD_SECT_XN;
                        mem_types[MT_DEVICE_WC].prot_sect |= PMD_SECT_XN;

                        /* Also setup NX memory mapping */
                        mem_types[MT_MEMORY_RW].prot_sect |= PMD_SECT_XN;
                }
{% endhighlight %}

cpu_arch 变量存储 Architecture 信息，ARMv7 此时对应的 cpu_arch
大于 CPU_ARCH_ARMv6，且 SCTR 寄存器的 XP 位置位。ARMv7 中 SCTR 寄存器
的 XP 位恒为 1，SCTLR 寄存器的布局如下：

![](/assets/PDB/BiscuitOS/boot/BOOT000226.png)

因此上面的函数得以执行。函数首先调用
cpu_is_xsc3() 函数判断如果当前不是 XSC3 CPU，那么函数设置
mem_types[] 数组各个成员的 prot_sect 标志，增加对 PMD_SECT_XN
的支持。XN 的意思是 Execute-Never，即用于指明处理器能否在该区域上执行
程序。例如在支持二级页表的页目录项中，XN 的定义如下：

![](/assets/PDB/BiscuitOS/boot/BOOT000227.png)

![](/assets/PDB/BiscuitOS/boot/BOOT000228.png)

{% highlight c %}
                if (cpu_arch >= CPU_ARCH_ARMv7 && (cr & CR_TRE)) {
                        /*
                         * For ARMv7 with TEX remapping,
                         * - shared device is SXCB=1100
                         * - nonshared device is SXCB=0100
                         * - write combine device mem is SXCB=0001
                         * (Uncached Normal memory)
                         */
                        mem_types[MT_DEVICE].prot_sect |= PMD_SECT_TEX(1);
                        mem_types[MT_DEVICE_NONSHARED].prot_sect |= PMD_SECT_TEX(1);
                        mem_types[MT_DEVICE_WC].prot_sect |= PMD_SECT_BUFFERABLE;
                }
{% endhighlight %}

在 ARMv7 体系中，如果 SCTLR 寄存器的 "TEX remap enable" 位置位，
例如在 SCTLR 寄存器布局中：

![](/assets/PDB/BiscuitOS/boot/BOOT000226.png)

其中 TRE 位用于 SCTLR 寄存器的 bit 28，用于页表的 TEX remap 使能。
如果该位置位，那么在页表中，TEX[2:1] 被分配给操作系统管理，而 TEX[0],
C 位，B 位，以及 MMU remap 寄存器用于描述内存区域的属性；反之如果该
位清零，那么在页表中，TEX[2:0] 和 C 位，B 位一起用于描述内存区域的
属性，如下图页目录项中：

![](/assets/PDB/BiscuitOS/boot/BOOT000227.png)

![](/assets/PDB/BiscuitOS/boot/BOOT000229.png)

根据 SCTLR 寄存器 CR_TRE 的置位清零情况，如果置位，函数将
PMD_SECT_TEX(1) 同步到页目录项里。

{% highlight c %}
        /*
         * Now deal with the memory-type mappings
         */
        cp = &cache_policies[cachepolicy];
        vecs_pgprot = kern_pgprot = user_pgprot = cp->pte;
        s2_pgprot = cp->pte_s2;
        hyp_device_pgprot = mem_types[MT_DEVICE].prot_pte;
        s2_device_pgprot = mem_types[MT_DEVICE].prot_pte_s2;
{% endhighlight %}

内核在启动过程中，曾调用过函数 init_default_cache_policy() 函数
设置了全局变量 cachepolicy，该变量是 cache_policies[] 数组中的
一个索引，用于指明当前系统 CACHE采用的策略。函数从 cache_policies[]
数组中获得当前系统 CACHE 采用的策略，并存储到 cp 变量里。函数将
cp->pte 赋值为 vecs_pgprot, kern_pgprot 和 user_pgrot. 这段函数
还设置了几个全局变量。

{% highlight c %}
#ifndef CONFIG_ARM_LPAE
        /*
         * We don't use domains on ARMv6 (since this causes problems with
         * v6/v7 kernels), so we must use a separate memory type for user
         * r/o, kernel r/w to map the vectors page.
         */
        if (cpu_arch == CPU_ARCH_ARMv6)
                vecs_pgprot |= L_PTE_MT_VECTORS;

        /*
         * Check is it with support for the PXN bit
         * in the Short-descriptor translation table format descriptors.
         */
        if (cpu_arch == CPU_ARCH_ARMv7 &&
                (read_cpuid_ext(CPUID_EXT_MMFR0) & 0xF) >= 4) {
                user_pmd_table |= PMD_PXNTABLE;
        }
#endif
{% endhighlight %}

如果此时没有定义 CONFIG_ARM_LPAE 宏，函数将执行上面的代码，由于
cpu_arch 在 ARMv7 里面是 CPU_ARCH_ARMv7，如果此时 ID_MMFR0
寄存器的最低 4 bit 的值大于等于 4。ID_MMFR0 寄存器的布局如下：

![](/assets/PDB/BiscuitOS/boot/BOOT000007.png)

最低的 4 bit 是 VMSA support 域，其定义如下：

![](/assets/PDB/BiscuitOS/boot/BOOT000230.png)

此时，当 VMSA support,bits[3:0] 大于等于 4，那么将 PXN 添加到
Short-descriptor 页表里面。函数将 user_pmd_table 里面添加了
PMD_PXNTABLE.

{% highlight c %}
        /*
         * ARMv6 and above have extended page tables.
         */
        if (cpu_arch >= CPU_ARCH_ARMv6 && (cr & CR_XP)) {
#ifndef CONFIG_ARM_LPAE
                /*
                 * Mark cache clean areas and XIP ROM read only
                 * from SVC mode and no access from userspace.
                 */
                mem_types[MT_ROM].prot_sect |= PMD_SECT_APX|PMD_SECT_AP_WRITE;
                mem_types[MT_MINICLEAN].prot_sect |= PMD_SECT_APX|PMD_SECT_AP_WRITE;
                mem_types[MT_CACHECLEAN].prot_sect |= PMD_SECT_APX|PMD_SECT_AP_WRITE;
#endif

                /*
                 * If the initial page tables were created with the S bit
                 * set, then we need to do the same here for the same
                 * reasons given in early_cachepolicy().
                 */
                if (initial_pmd_value & PMD_SECT_S) {
                        user_pgprot |= L_PTE_SHARED;
                        kern_pgprot |= L_PTE_SHARED;
                        vecs_pgprot |= L_PTE_SHARED;
                        s2_pgprot |= L_PTE_SHARED;
                        mem_types[MT_DEVICE_WC].prot_sect |= PMD_SECT_S;
                        mem_types[MT_DEVICE_WC].prot_pte |= L_PTE_SHARED;
                        mem_types[MT_DEVICE_CACHED].prot_sect |= PMD_SECT_S;
                        mem_types[MT_DEVICE_CACHED].prot_pte |= L_PTE_SHARED;
                        mem_types[MT_MEMORY_RWX].prot_sect |= PMD_SECT_S;
                        mem_types[MT_MEMORY_RWX].prot_pte |= L_PTE_SHARED;
                        mem_types[MT_MEMORY_RW].prot_sect |= PMD_SECT_S;
                        mem_types[MT_MEMORY_RW].prot_pte |= L_PTE_SHARED;
                        mem_types[MT_MEMORY_DMA_READY].prot_pte |= L_PTE_SHARED;
                        mem_types[MT_MEMORY_RWX_NONCACHED].prot_sect |= PMD_SECT_S;
                        mem_types[MT_MEMORY_RWX_NONCACHED].prot_pte |= L_PTE_SHARED;
                }
        }
{% endhighlight %}

在 ARMv7 中 SCTLR 寄存器的 23 bit 恒为 1，那么 "cr&CR_XP"
为真，那么函数执行上面的代码，并且此时宏 CONFIG_ARM_LPAE 没有
拓展，那么函数首先标记 MT_CACHECLEAN 与 MT_ROM, MT_MINICLEAN
的 prot_sect 成员 PMD_SECT_APX,PMD_SECT_AP_WRITE. 函数接下来
解析 initial_pmd_value 是否包含 PMD_SECT_S,initial_pmd_val
是通过 init_default_cache_policy() 函数传递的参数设置，该参数
来自 ARMv7 proc_info_list 的 __cpu_mm_mmu_flags, 在 ARMv7 中，
该值的定义为：

{% highlight c %}
.__cpu_mm_mmu_flags = PMD_TYPE_SECT | PMD_SECT_AP_WRITE | PMD_SECT_AP_READ | \
                        PMD_SECT_AF | PMD_FLAGS_UP
{% endhighlight %}

从上面的定义可以知道 initial_pmd_value 不包含 PMD_SECT_S 标志，
因此这段代码不执行。

{% highlight c %}
        /*
         * Non-cacheable Normal - intended for memory areas that must
         * not cause dirty cache line writebacks when used
         */
        if (cpu_arch >= CPU_ARCH_ARMv6) {
                if (cpu_arch >= CPU_ARCH_ARMv7 && (cr & CR_TRE)) {
                        /* Non-cacheable Normal is XCB = 001 */
                        mem_types[MT_MEMORY_RWX_NONCACHED].prot_sect |=
                                PMD_SECT_BUFFERED;
                } else {
                        /* For both ARMv6 and non-TEX-remapping ARMv7 */
                        mem_types[MT_MEMORY_RWX_NONCACHED].prot_sect |=
                                PMD_SECT_TEX(1);
                }
        } else {
                mem_types[MT_MEMORY_RWX_NONCACHED].prot_sect |= PMD_SECT_BUFFERABLE;
        }
{% endhighlight %}

在 ARMv7 体系中，如果 SCTLR 寄存器的 "TEX remap enable" 位置位，
例如在 SCTLR 寄存器布局中：

![](/assets/PDB/BiscuitOS/boot/BOOT000226.png)

其中 TRE 位用于 SCTLR 寄存器的 bit 28，用于页表的 TEX remap 使能。
如果该位置位，那么在页表中，TEX[2:1] 被分配给操作系统管理，而 TEX[0],
C 位，B 位，以及 MMU remap 寄存器用于描述内存区域的属性；反之如果该
位清零，那么在页表中，TEX[2:0] 和 C 位，B 位一起用于描述内存区域的
属性，如下图页目录项中：

![](/assets/PDB/BiscuitOS/boot/BOOT000227.png)

![](/assets/PDB/BiscuitOS/boot/BOOT000229.png)

根据 SCTLR 寄存器 CR_TRE 的置位清零情况，如果置位，
此时将 MT_MEMORY_RWX_NOCACHED 的 prot_sect 加上
PMD_SECT_BUFFERED。

{% highlight c %}
        for (i = 0; i < 16; i++) {
                pteval_t v = pgprot_val(protection_map[i]);
                protection_map[i] = __pgprot(v | user_pgprot);
        }
{% endhighlight %}

函数接下来就是从 protection_map[] 数组中添加对 user_pgprot 的支持。

{% highlight c %}
        mem_types[MT_LOW_VECTORS].prot_pte |= vecs_pgprot;
        mem_types[MT_HIGH_VECTORS].prot_pte |= vecs_pgprot;

        pgprot_user   = __pgprot(L_PTE_PRESENT | L_PTE_YOUNG | user_pgprot);
        pgprot_kernel = __pgprot(L_PTE_PRESENT | L_PTE_YOUNG |
                                 L_PTE_DIRTY | kern_pgprot);
        pgprot_s2  = __pgprot(L_PTE_PRESENT | L_PTE_YOUNG | s2_pgprot);
        pgprot_s2_device  = __pgprot(s2_device_pgprot);
        pgprot_hyp_device  = __pgprot(hyp_device_pgprot);

        mem_types[MT_LOW_VECTORS].prot_l1 |= ecc_mask;
        mem_types[MT_HIGH_VECTORS].prot_l1 |= ecc_mask;
        mem_types[MT_MEMORY_RWX].prot_sect |= ecc_mask | cp->pmd;
        mem_types[MT_MEMORY_RWX].prot_pte |= kern_pgprot;
        mem_types[MT_MEMORY_RW].prot_sect |= ecc_mask | cp->pmd;
        mem_types[MT_MEMORY_RW].prot_pte |= kern_pgprot;
        mem_types[MT_MEMORY_DMA_READY].prot_pte |= kern_pgprot;
        mem_types[MT_MEMORY_RWX_NONCACHED].prot_sect |= ecc_mask;
        mem_types[MT_ROM].prot_sect |= cp->pmd;
{% endhighlight %}

接下来函数将之前计算好的标记更新到指定的遍历和数组里。每种
mem_types 成员都有了新的标志。

{% highlight c %}
        switch (cp->pmd) {
        case PMD_SECT_WT:
                mem_types[MT_CACHECLEAN].prot_sect |= PMD_SECT_WT;
                break;
        case PMD_SECT_WB:
        case PMD_SECT_WBWA:
                mem_types[MT_CACHECLEAN].prot_sect |= PMD_SECT_WB;
                break;
        }
        pr_info("Memory policy: %sData cache %s\n",
                ecc_mask ? "ECC enabled, " : "", cp->policy);

        for (i = 0; i < ARRAY_SIZE(mem_types); i++) {
                struct mem_type *t = &mem_types[i];
                if (t->prot_l1)
                        t->prot_l1 |= PMD_DOMAIN(t->domain);
                if (t->prot_sect)
                        t->prot_sect |= PMD_DOMAIN(t->domain);
        }
{% endhighlight %}

函数的最后就是判断 CACHE 的类型，如果是 write-through，
那么给 MT_CACHECLEAN 添加 PMD_SECT_WT；如果是 write-back，
那么给 MT_CACHECLEAN 添加 PMD_SECT_WB。最后函数调用 for
循环，如果遍历到 mem_types[] 成员的 prot_l1 有效，那么函数
对 prot_l1 添加 PMD_DOMAIN() 的支持。如果成员 port_sect
有效，那么添加对 PMD_DOMAIN() 的支持。

------------------------------------

#### <span id="A0194">early_mm_init</span>

{% highlight c %}
void __init early_mm_init(const struct machine_desc *mdesc)
{
        build_mem_type_table();
        early_paging_init(mdesc);
}
{% endhighlight %}

early_mm_init() 函数用于在初始化简单的页表项 PTE 和 PMD。
参数 mdesc 指向 machine_desc 结构，函数首先调用
build_mem_type_table() 函数根据体系设置了 PTE 和 PMD 在
mem_types[] 数组中的标志，然后调用 early_paging_init()
函数初始化了早期的页表，不过该函数的具体执行过程与 mdesc 参数
有关，如果 mdesc 不做特殊处理，该函数不做任何实质性的工作。

> - [build_mem_type_table](#A0193)

------------------------------------

#### <span id="A0195">setup_dma_zone</span>

{% highlight c %}
void __init setup_dma_zone(const struct machine_desc *mdesc)
{
#ifdef CONFIG_ZONE_DMA
        if (mdesc->dma_zone_size) {
                arm_dma_zone_size = mdesc->dma_zone_size;
                arm_dma_limit = PHYS_OFFSET + arm_dma_zone_size - 1;
        } else
                arm_dma_limit = 0xffffffff;
        arm_dma_pfn_limit = arm_dma_limit >> PAGE_SHIFT;
#endif
}
{% endhighlight %}

setup_dma_zone() 函数的作用就是设置 DMA 域值。参数 mdesc 指向
体系相关的信息。如果系统支持 CONFIG_ZONE_DMA, 此时 mdesc 包含了
dma_zone_size 的信息，那么函数设置 arm_dma_zone_size 的值为
mdesc->dma_zone_size, 且 arm_dma_limit 为
"PHYS_OFFSET + arm_dma_zone_size - 1"；反之将 arm_dma_limit
设置为 0xffffffff. 这是完毕之后，计算 arm_dma_limit 的最大页帧
号，存储在 arm_dma_pfn_limit. 如果系统不支持 CONFIG_ZONE_DMA,
那么函数不做任何实质性动作。

------------------------------------

#### <span id="A0196">vmalloc_min</span>

{% highlight c %}
static void * __initdata vmalloc_min =
        (void *)(VMALLOC_END - (240 << 20) - VMALLOC_OFFSET);
{% endhighlight %}

vmalloc_min 用于指向 VMALLOC 内存区域最低地址。该地址通过
将 VMALLOC_END VMALLOC 区域结束地址减去 240M，再减去
VMALLOC 区域隔离区 VMLLOC_OFFSET (8M)，因此计算出
VMALLOC 内存区最低地址。

------------------------------------

#### <span id="A0197">adjust_lowmem_bounds</span>

{% highlight c %}
void __init adjust_lowmem_bounds(void)
{
        phys_addr_t memblock_limit = 0;
        u64 vmalloc_limit;
        struct memblock_region *reg;
        phys_addr_t lowmem_limit = 0;

        /*
         * Let's use our own (unoptimized) equivalent of __pa() that is
         * not affected by wrap-arounds when sizeof(phys_addr_t) == 4.
         * The result is used as the upper bound on physical memory address
         * and may itself be outside the valid range for which phys_addr_t
         * and therefore __pa() is defined.
         */
        vmalloc_limit = (u64)(uintptr_t)vmalloc_min - PAGE_OFFSET + PHYS_OFFSET;

        for_each_memblock(memory, reg) {
                phys_addr_t block_start = reg->base;
                phys_addr_t block_end = reg->base + reg->size;

                if (reg->base < vmalloc_limit) {
                        if (block_end > lowmem_limit)
                                /*
                                 * Compare as u64 to ensure vmalloc_limit does
                                 * not get truncated. block_end should always
                                 * fit in phys_addr_t so there should be no
                                 * issue with assignment.
                                 */
                                lowmem_limit = min_t(u64,
                                                         vmalloc_limit,
                                                         block_end);

                        /*
                         * Find the first non-pmd-aligned page, and point
                         * memblock_limit at it. This relies on rounding the
                         * limit down to be pmd-aligned, which happens at the
                         * end of this function.
                         *
                         * With this algorithm, the start or end of almost any
                         * bank can be non-pmd-aligned. The only exception is
                         * that the start of the bank 0 must be section-
                         * aligned, since otherwise memory would need to be
                         * allocated when mapping the start of bank 0, which
                         * occurs before any free memory is mapped.
                         */
                        if (!memblock_limit) {
                                if (!IS_ALIGNED(block_start, PMD_SIZE))
                                        memblock_limit = block_start;
                                else if (!IS_ALIGNED(block_end, PMD_SIZE))
                                        memblock_limit = lowmem_limit;
                        }

                }
        }


        arm_lowmem_limit = lowmem_limit;

        high_memory = __va(arm_lowmem_limit - 1) + 1;

        if (!memblock_limit)
                memblock_limit = arm_lowmem_limit;

        /*
         * Round the memblock limit down to a pmd size.  This
         * helps to ensure that we will allocate memory from the
         * last full pmd, which should be mapped.
         */
        memblock_limit = round_down(memblock_limit, PMD_SIZE);

        if (!IS_ENABLED(CONFIG_HIGHMEM) || cache_is_vipt_aliasing()) {
                if (memblock_end_of_DRAM() > arm_lowmem_limit) {
                        phys_addr_t end = memblock_end_of_DRAM();

                        pr_notice("Ignoring RAM at %pa-%pa\n",
                                  &memblock_limit, &end);
                        pr_notice("Consider using a HIGHMEM enabled kernel.\n");

                        memblock_remove(memblock_limit, end - memblock_limit);
                }
        }

        memblock_set_current_limit(memblock_limit);
}
{% endhighlight %}

adjust_lowmem_bounds() 函数用于调整获得低端物理内存的信息。
由于代码太长，分段解析。

{% highlight c %}
void __init adjust_lowmem_bounds(void)
{
        phys_addr_t memblock_limit = 0;
        u64 vmalloc_limit;
        struct memblock_region *reg;
        phys_addr_t lowmem_limit = 0;

        /*
         * Let's use our own (unoptimized) equivalent of __pa() that is
         * not affected by wrap-arounds when sizeof(phys_addr_t) == 4.
         * The result is used as the upper bound on physical memory address
         * and may itself be outside the valid range for which phys_addr_t
         * and therefore __pa() is defined.
         */
        vmalloc_limit = (u64)(uintptr_t)vmalloc_min - PAGE_OFFSET + PHYS_OFFSET;
{% endhighlight %}

函数定义了局部变量 memblock_limit 用于存储 MEMBLOCK 内存分配器的限制值，
可以上上限也可以是下限。局部变量 vmalloc_limit 用于存储 VMALLOC 内存区的
极值，lowmem_limit 局部变量用于存储低端物理内存的限制。函数首先将 vmalloc_min
的值 (vmalloc_min 为 VMALLOC 内存区最低地址) 减去 PAGE_OFFSET (内核虚拟地址
起始地址), 然后在加上 PHYS_OFFSET (DRAM 在地址总线上的偏移，即 DRAM 的物
理地址)，通过计算可以获得 VMALLOC 内存区的起始物理地址。

{% highlight c %}
        for_each_memblock(memory, reg) {
                phys_addr_t block_start = reg->base;
                phys_addr_t block_end = reg->base + reg->size;

                if (reg->base < vmalloc_limit) {
                        if (block_end > lowmem_limit)
                                lowmem_limit = min_t(u64,
                                                         vmalloc_limit,
                                                         block_end);

                        if (!memblock_limit) {
                                if (!IS_ALIGNED(block_start, PMD_SIZE))
                                        memblock_limit = block_start;
                                else if (!IS_ALIGNED(block_end, PMD_SIZE))
                                        memblock_limit = lowmem_limit;
                        }

                }
        }
{% endhighlight %}

函数接着调用 for_each_memblock() 函数遍历 MEMBLOCK 内存分配器的
memory 可用物理内存区域内的所有 regions。每遍历一块内存区，函数
首先获得该内存区的起始物理地址和终止地址。如果该内存区的起始物理
地址小于 vmalloc_limit，即 VMALLOC 内存区起始物理地址，那么函数
继续判断，如果此时遍历到的内存区终止物理地址大于 lowmem_limit,
那么函数调用 min_t() 函数获得 vmalloc_limit 与遍历到内存区块
的终止物理地址中最小值，这样 lowmem_limit 的最大值不会超过
vmalloc_limit. 如果此时 memblock_limit 等于 0，那么函数继续
判断，如果遍历到的内存区块的起始地址没有按 PMD_SIZE 对齐，那么
函数将 memblock_limit 设置为遍历到内存区块的起始地址；反之
如果遍历到的内存区块的起始地址按 PMD_SIZE 对齐，但遍历到的内存
区块的终止地址没有按 PMD_SIZE 对齐，那么 memblock_limit 设置
为 lowmem_limit，即最大低端物理内存地址；如果两个条件都不满足，
那么 memblock_limit 依旧是 0。

{% highlight c %}
        arm_lowmem_limit = lowmem_limit;

        high_memory = __va(arm_lowmem_limit - 1) + 1;

        if (!memblock_limit)
                memblock_limit = arm_lowmem_limit;

        /*
         * Round the memblock limit down to a pmd size.  This
         * helps to ensure that we will allocate memory from the
         * last full pmd, which should be mapped.
         */
        memblock_limit = round_down(memblock_limit, PMD_SIZE);
{% endhighlight %}

遍历完所有的可用物理内存区块之后，已经找到可用低端物理内存最大物理
地址，其存储在 lowmem_limit 内，此时将该值赋值给 arm_lowmem_limit,
因此全局变量 arm_lowmem_limit 指向低端物理内存的结束地址。因此
低端物理内存结束地址之后便是高端物理内存的起始地址，函数此时调用
__va() 函数获得高端物理内存的起始虚拟地址。如果此时 memblock_limit
还是零，那么函数将 memblock_limit 设置为 arm_lowmem_limit, 即
低端物理内存结束地址。函数使用 round_down() 函数向下将
memblock_limit 按 PMD_SIZE 对齐。

{% highlight c %}
        if (!IS_ENABLED(CONFIG_HIGHMEM) || cache_is_vipt_aliasing()) {
                if (memblock_end_of_DRAM() > arm_lowmem_limit) {
                        phys_addr_t end = memblock_end_of_DRAM();

                        pr_notice("Ignoring RAM at %pa-%pa\n",
                                  &memblock_limit, &end);
                        pr_notice("Consider using a HIGHMEM enabled kernel.\n");

                        memblock_remove(memblock_limit, end - memblock_limit);
                }
        }

        memblock_set_current_limit(memblock_limit);
{% endhighlight %}

如果此时系统没有定义 CONFIG_HIGMEM 宏或者 cache_is_vipt_aliasing()
函数返回 1，即 CACHE 是 VIPT 的，那么函数继续判断，如果此时调用
memblock_end_of_DRAM() 函数获得 DRAM 的终止地址大于 arm_lowmem_limit,
那么函数将会提示，并调用 memblock_remove() 函数将
[memblock_limit, end-memblock_limit] 区域从 MEMBLOCK 内存分配器
的 memory 区域内移除。函数最后调用 memblock_set_current_limit()
函数设置了 MEMBLOCK 内存分配器的限制值。

> - [PHYS_OFFSET](#A0198)
>
> - [PAGE_OFFSET](#A0199)
>
> - [memblock_remove](#A0200)
>
> - [memblock_end_of_DRAM](#A0201)
>
> - [memblock_set_current_limit](#A0202)
>
> - [round_down](#A0154)

------------------------------------

#### <span id="A0198">PHYS_OFFSET</span>

{% highlight c %}
config PHYS_OFFSET
        hex "Physical address of main memory" if MMU
        depends on !ARM_PATCH_PHYS_VIRT
        default DRAM_BASE if !MMU
        default 0x00000000 if ARCH_EBSA110 || \
                        ARCH_FOOTBRIDGE || \
                        ARCH_INTEGRATOR || \
                        ARCH_IOP13XX || \                
                        ARCH_KS8695 || \
                        ARCH_REALVIEW
        default 0x10000000 if ARCH_OMAP1 || ARCH_RPC
        default 0x20000000 if ARCH_S5PV210
        default 0xc0000000 if ARCH_SA1100
        help
          Please provide the physical address corresponding to the
          location of main memory in your system.
{% endhighlight %}

PHYS_OFFSET 宏用于指明 DRAM 在地址总线上的偏移。其定义在
"arch/arm/Kconfig"，从其定义可以看出 PHYS_OFFSET 的大小
与体系有关。

------------------------------------

#### <span id="A0199">PAGE_OFFSET</span>

{% highlight c %}
config PAGE_OFFSET
        hex
        default PHYS_OFFSET if !MMU
        default 0x40000000 if VMSPLIT_1G
        default 0x80000000 if VMSPLIT_2G
        default 0xB0000000 if VMSPLIT_3G_OPT
        default 0xC0000000
{% endhighlight %}

PAGE_OFFSET 宏用于指明内核空间虚拟地址的起始地址。其值与用户空间
和内核空间虚拟地址划分有关，如果用户空间与内核空间按 1:3，那么
内核空间虚拟地址从 0x40000000 开始；如果用户空间与内核空间按 2:2，
那么内核空间虚拟地址从 0x80000000 开始；如果用户空间与内核空间按
3:1，那么内核空间虚拟地址从 0xB0000000 开始。如果都不是，那么
内核空间虚拟地址从 0xC0000000 开始。

------------------------------------

#### <span id="A0200">memblock_remove</span>

{% highlight c %}
int __init_memblock memblock_remove(phys_addr_t base, phys_addr_t size)
{               
        phys_addr_t end = base + size - 1;

        memblock_dbg("memblock_remove: [%pa-%pa] %pS\n",
                     &base, &end, (void *)_RET_IP_);

        return memblock_remove_range(&memblock.memory, base, size);
}
{% endhighlight %}

memblock_remove() 函数的作用是从 MEMBLOCK 内存分配器的 memory
区域内移除指定范围的内存区块。参数 base 指向要移除内存区块的起始
物理地址，size 参数指向要移除内存区的长度。函数首先计算出要移除
内存区块的结束地址。函数通过调用 memblock_remove_range() 函数
从 MEMBLOCK 内存分配器的 memory 区域内移除指定大小的内存区块。

> - [memblock_remove_range](#A0160)

------------------------------------

#### <span id="A0201">memblock_end_of_DRAM</span>

{% highlight c %}
phys_addr_t __init_memblock memblock_end_of_DRAM(void)
{                       
        int idx = memblock.memory.cnt - 1;

        return (memblock.memory.regions[idx].base + memblock.memory.regions[idx].size);                 
}
{% endhighlight %}

memblock_end_of_DRAM() 函数用于获得 MEMBLOCK 内存分配器 memory
区域的最大物理地址。函数首先获得 memory 区域内最大内存区块的索引，
然后返回该内存区块的终止地址，以此代表 DRAM 的终止物理地址。

------------------------------------

#### <span id="A0202">memblock_set_current_limit</span>

{% highlight c %}
void __init_memblock memblock_set_current_limit(phys_addr_t limit)
{                       
        memblock.current_limit = limit;
}
{% endhighlight %}

memblock_set_current_limit() 函数用于设置 MEMBLOCK 内存分配器的限制值，
其可以是上限也可以是下限。MEMBLOCK 的现在存储在 current_limit 成员里。

------------------------------------

#### <span id="A0203">memblock_reserve</span>

{% highlight c %}
int __init_memblock memblock_reserve(phys_addr_t base, phys_addr_t size)
{       
        phys_addr_t end = base + size - 1;

        memblock_dbg("memblock_reserve: [%pa-%pa] %pF\n",
                     &base, &end, (void *)_RET_IP_);

        return memblock_add_range(&memblock.reserved, base, size, MAX_NUMNODES, 0);     
}
{% endhighlight %}

memblock_reserve() 函数用于将内存区块加入到 MEMBLOCK 的保留区内。
加入保留区内之后，该内存区块不能在被 MEMBLOCK 分配器使用。参数 base
指向需要预留内存区块的起始物理地址，size 参数表示区块的大小。
函数通过 memblock_add_range() 函数将内存区块加入到 MEMBLOCK 内存
分配器的 reserved 内存区。

> - [memblock_add_range](#A0165)

------------------------------------

#### <span id="A0204">KERNEL_START</span>

{% highlight c %}
#ifdef CONFIG_XIP_KERNEL
#define KERNEL_START            _sdata
#else
#define KERNEL_START            _stext
#endif
{% endhighlight %}

KERNEL_START 宏代表内核镜像的起始虚拟地址。其定义与
CONFIG_XIP_KERNEL 有关，如果该宏定义，那么 KERNEL_START
指向 _sdata; 反之指向 _stext.

------------------------------------

#### <span id="A0205">KERNEL_END</span>

{% highlight c %}
#define KERNEL_END              _end
{% endhighlight %}

KERNEL_END 宏指向内核镜像结束虚拟地址。其指向 _end.

------------------------------------

#### <span id="A0206">memblock_addrs_overlap</span>

{% highlight c %}
/*
 * Address comparison utilities
 */
static unsigned long __init_memblock memblock_addrs_overlap(phys_addr_t base1, phys_addr_t size1,
                                       phys_addr_t base2, phys_addr_t size2)
{
        return ((base1 < (base2 + size2)) && (base2 < (base1 + size1)));
}
{% endhighlight %}

memblock_addrs_overlap() 函数用于确认两块内存区块是否存在重叠。
如果存在，那么函数返回 1；反之返回 0.

------------------------------------

#### <span id="A0207">memblock_overlaps_region</span>

{% highlight c %}
bool __init_memblock memblock_overlaps_region(struct memblock_type *type,
                                        phys_addr_t base, phys_addr_t size)
{
        unsigned long i;

        for (i = 0; i < type->cnt; i++)
                if (memblock_addrs_overlap(base, size, type->regions[i].base,
                                           type->regions[i].size))
                        break;
        return i < type->cnt;
}
{% endhighlight %}

memblock_overlaps_region() 函数用于判断内存区块在 MEMBLOCK 内存分配器
指定 type 的 regions 内是否存在重叠部分，如果存在，那么函数返回第一个
重叠区块的索引。函数使用 for 循环遍历 type 的所有 regions，每次遍历
一个 region，通过函数 memblock_addrs_overlap() 确认该区块是否和参数
对应的区块存在重合部分，如果存在，那么函数终止循环；反之继续查找。
如果找到，那么返回 true；反之返回 false。

> - [memblock_addrs_overlap](#A0206)

------------------------------------

#### <span id="A0208">memblock_is_region_memory</span>

{% highlight c %}
/**
 * memblock_is_region_memory - check if a region is a subset of memory
 * @base: base of region to check
 * @size: size of region to check
 *
 * Check if the region [@base, @base + @size) is a subset of a memory block.
 *
 * Return:
 * 0 if false, non-zero if true
 */
bool __init_memblock memblock_is_region_memory(phys_addr_t base, phys_addr_t size)
{
        int idx = memblock_search(&memblock.memory, base);
        phys_addr_t end = base + memblock_cap_size(base, &size);

        if (idx == -1)
                return false;
        return (memblock.memory.regions[idx].base +
                 memblock.memory.regions[idx].size) >= end;
}
{% endhighlight %}

memblock_is_region_memory() 函数的作用是确认内存区块是否在
MEMBLOCK 内存分配器 memory regions 里。参数 base 指向内存区块
的起始物理地址，参数 size 代表内存区块的长度。函数首先调用
memblock_search() 函数查找 base 参数在 memory regions 的
索引，如果该索引为 -1，那么代表所要查找的内存区不在 memory
regions 里。反之如果此时找到的内存区的结束地址大于需要查找的
结束地址，那么函数直接返回 true，代表需要查找的区域都在
MEMBLOCK memory 里；反之返回 false。

> - [memblock_search](#A0209)
>
> - [memblock_cap_size](#A0159)

------------------------------------

#### <span id="A0209">memblock_search</span>

{% highlight c %}
static int __init_memblock memblock_search(struct memblock_type *type, phys_addr_t addr)
{
        unsigned int left = 0, right = type->cnt;

        do {
                unsigned int mid = (right + left) / 2;

                if (addr < type->regions[mid].base)
                        right = mid;
                else if (addr >= (type->regions[mid].base +
                                  type->regions[mid].size))
                        left = mid + 1;
                else
                        return mid;
        } while (left < right);
        return -1;
}
{% endhighlight %}

memblock_search() 函数的作用是根据起始地址，在 MEMBLOCK 指定
type regions 内找到与起始地址最靠近的内存区块。函数采用二分法，
在 MEMBLOCK 内存分配器 type regions 中查找内存区块，只要参数
addr 的值大于内存区块的起始地址但又小于终止地址，那么直接返回
该内存区块的索引。否则循环结束后没有找到，则直接返回 -1.

------------------------------------

#### <span id="A0210">memblock_is_region_reserved</span>

{% highlight c %}
/**
 * memblock_is_region_reserved - check if a region intersects reserved memory
 * @base: base of region to check
 * @size: size of region to check
 *
 * Check if the region [@base, @base + @size) intersects a reserved
 * memory block.
 *
 * Return:
 * True if they intersect, false if not.
 */     
bool __init_memblock memblock_is_region_reserved(phys_addr_t base, phys_addr_t size)    
{               
        memblock_cap_size(base, &size);
        return memblock_overlaps_region(&memblock.reserved, base, size);
}
{% endhighlight %}

memblock_is_region_reserved() 函数的作用是检查参数对应的内存区块
是否在 MEMBLOCK 内存分配器的预留区内，如果在则返回 true；反之返回
false。参数 base 指向查找区块的起始地址，参数 size 指向查找区块的
长度。函数首先调用 memblock_cap_size() 函数得到了查找区域的安全
长度，然后调用 memblock_overlaps_region() 函数判断查找的区域是否
在 MEMBLOCK reserved 区域内重合，如果重合则返回 ture，反之返回
true。

------------------------------------

#### <span id="A0211">arm_initrd_init</span>

{% highlight c %}
static void __init arm_initrd_init(void)
{
#ifdef CONFIG_BLK_DEV_INITRD
        phys_addr_t start;
        unsigned long size;

        initrd_start = initrd_end = 0;

        if (!phys_initrd_size)
                return;

        /*      
         * Round the memory region to page boundaries as per free_initrd_mem()
         * This allows us to detect whether the pages overlapping the initrd
         * are in use, but more importantly, reserves the entire set of pages
         * as we don't want these pages allocated for other purposes.
         */
        start = round_down(phys_initrd_start, PAGE_SIZE);
        size = phys_initrd_size + (phys_initrd_start - start);
        size = round_up(size, PAGE_SIZE);

        if (!memblock_is_region_memory(start, size)) {
                pr_err("INITRD: 0x%08llx+0x%08lx is not a memory region - disabling initrd\n",
                       (u64)start, size);
                return;
        }

        if (memblock_is_region_reserved(start, size)) {
                pr_err("INITRD: 0x%08llx+0x%08lx overlaps in-use memory region - disabling initrd\n",
                       (u64)start, size);
                return;
        }

        memblock_reserve(start, size);

        /* Now convert initrd to virtual addresses */
        initrd_start = __phys_to_virt(phys_initrd_start);
        initrd_end = initrd_start + phys_initrd_size;
#endif   
}
{% endhighlight %}

arm_initrd_init() 函数的作用是将 INITRD 所占用的物理内存加入到
MEMBLOCK 内存分配器的预留区内。如果定义了宏 CONFIG_BLK_DEV_INITRD，
那么系统启动的过程中，uboot 将 INITRD 占用的物理地址信息存储在 DTB
的 chosen 节点内，在 early_init_dt_check_for_initrd() 函数里，就
是从 DTB 中解析出 INITRD 的物理信息存储在 phys_initrd_start 和
phys_initrd_size 两个全局变量里。在该函数内，首先判断
phys_initrd_size, 如果该值为 0，那么函数直接返回。接着函数调用
round_down() 函数和 round_up() 函数对 phys_initrd_start 和
phys_initrd_size 进行对齐操作，对齐后的结果存储在 start 和 size
局部变量里。函数接下来检查 start 与 size 对应的内存区域是否
已经在 MEMBLOCK reserved 和 memory 区域内，如果调用
memblock_is_region_memory() 函数发现 INITRD 对应的物理区块
不全在 MEMBLOCK memory regions 内，那么函数就报错，并返回。
函数接着调用 memblock_is_region_reserved() 函数发现 start
和 size 对应的区域已经在 MEMBLOCK reserved 区域内，那么
函数也同样报错并直接返回。

函数通过检查之后，调用 memblock_reserve() 函数将 start 和
size 对应的区域加入到 MEMBLOCK 的预留区内，并将 phys_initrd_start
对应的虚拟地址赋值给 initrd_start, 并将 INITRD 结束的虚拟地址
存储在 initrd_end 全局变量里。

> - [round_down](#A0154)
>
> - [round_up](#A0149)
>
> - [memblock_is_region_memory](#A0208)
>
> - [memblock_is_region_reserved](#A0210)
>
> - [memblock_reserve](#A0203)

------------------------------------

#### <span id="A0212">arm_mm_memblock_reserve</span>

{% highlight c %}
/*
 * Reserve the special regions of memory
 */
void __init arm_mm_memblock_reserve(void)
{
        /*
         * Reserve the page tables.  These are already in use,
         * and can only be in node 0.
         */            
        memblock_reserve(__pa(swapper_pg_dir), SWAPPER_PG_DIR_SIZE);

#ifdef CONFIG_SA1111
        /*
         * Because of the SA1111 DMA bug, we want to preserve our
         * precious DMA-able memory...
         */
        memblock_reserve(PHYS_OFFSET, __pa(swapper_pg_dir) - PHYS_OFFSET);
#endif
}
{% endhighlight %}

arm_mm_memblock_reserve() 函数的作用就是将 ARM 所使用的页目录占用
的物理内存加入到 MEMBLOCK 预留区内。内核中 swapper_pg_dir 指向内核
页目录所在的虚拟地址，函数调用 memblock_reserve() 函数将
swapper_pg_dir 对应的物理内存区块加入到 MEMBLOCK 内存分配的
预留区内。此处不讨论 SA1111.

------------------------------------

#### <span id="A0213">early_init_dt_reserve_memory_arch</span>

{% highlight c %}
int __init __weak early_init_dt_reserve_memory_arch(phys_addr_t base,
                                        phys_addr_t size, bool nomap)
{                                         
        if (nomap)
                return memblock_remove(base, size);
        return memblock_reserve(base, size);
}
{% endhighlight %}

early_init_dt_reserve_memory_arch() 函数的作用就是根据不同条件，
将 DTB 占用的物理内存加入到 MEMBLOCK 的指定区域，参数 base 指向
DTB 占用物理内存区的起始地址，size 代表大小，nomap 参数是否给
DTB 所占用的物理内存建立页表。函数首先判断 nomap 参数，如果该
参数为真，那么函数不给 DTB 占用的物理内存建立页表，并将这段物理
内存区域从 MEMBLOCK memory 区域内移除；反之函数直接调用
memblock_reserve() 将 DTB 加入到 MEMBLOCK 的预留区内。

> - [memblock_remove](#A0200)
>
> - [memblock_reserve](#A0203)

------------------------------------

#### <span id="A0214">early_init_fdt_reserve_self</span>

{% highlight c %}
/**
 * early_init_fdt_reserve_self() - reserve the memory used by the FDT blob
 */
void __init early_init_fdt_reserve_self(void)
{
        if (!initial_boot_params)
                return;

        /* Reserve the dtb region */
        early_init_dt_reserve_memory_arch(__pa(initial_boot_params),
                                          fdt_totalsize(initial_boot_params),
                                          0);
}
{% endhighlight %}

early_init_fdt_reserve_self() 函数的作用就是将 DTB 加入到
MEMBLOCK 的预留区内。initial_boot_params 指向 DTB 所在的虚拟地址，
函数首先判断 initial_boot_params 的有效性，如果无效，则直接返回。
函数接着调用 early_init_dt_reserve_memory_arch() 函数将
DTB 所在的物理地址加入到 MEMBLOCK 预留区内。

> - [early_init_dt_reserve_memory_arch](#A0213)
>
> - [fdt_totalsize](#A0078)

------------------------------------

#### <span id="A0215">fdt_mem_rsv_</span>

{% highlight c %}
static inline const struct fdt_reserve_entry *fdt_mem_rsv_(const void *fdt, int n)
{
        const struct fdt_reserve_entry *rsv_table =
                (const struct fdt_reserve_entry *)
                ((const char *)fdt + fdt_off_mem_rsvmap(fdt));

        return rsv_table + n;
}
{% endhighlight %}

fdt_mem_rsv_() 函数的作用是从 DTB 的 memory reserve map 区域
读取指定保留区数据。在 memory reserve map 区域，每个保留区使用
struct fdt_erserve_entry 数据结构维护一个保留区，其定义如下：

{% highlight c %}
struct fdt_reserve_entry {
        fdt64_t address;
        fdt64_t size;
};
{% endhighlight %}

参数 fdt 指向 DTB 所在的虚拟地址，通过 fdt_off_mem_rsvmap()
函数可以获得 DTB memory reserve map 区域的起始地址，然后
通过 n 做索引获得指定的 fdt_reserve_entry 结构。

> - [fdt_off_mem_rsvmap](#A0081)

------------------------------------

#### <span id="A0216">fdt_mem_rsv_w_</span>

{% highlight c %}
static inline struct fdt_reserve_entry *fdt_mem_rsv_w_(void *fdt, int n)
{
        return (void *)(uintptr_t)fdt_mem_rsv_(fdt, n);
}
{% endhighlight %}

fdt_mem_rsv_w_() 用于获得 DTB memory reserve map 表第
n 个 fdt_reserve_entry 结构。参数 fdt 指向 DTB 所在的虚拟
地址，n 参数指向索引。函数通过调用 fdt_mem_rsw_() 函数
做索引获得指定的保留项。

> - [fdt_mem_rsv_](#A0215)

------------------------------------

#### <span id="A0217">fdt_mem_rsv</span>

{% highlight c %}
static const struct fdt_reserve_entry *fdt_mem_rsv(const void *fdt, int n)
{
        int offset = n * sizeof(struct fdt_reserve_entry);
        int absoffset = fdt_off_mem_rsvmap(fdt) + offset;

        if (absoffset < fdt_off_mem_rsvmap(fdt))
                return NULL;
        if (absoffset > fdt_totalsize(fdt) - sizeof(struct fdt_reserve_entry))
                return NULL;
        return fdt_mem_rsv_(fdt, n);
}
{% endhighlight %}

fdt_mem_rsv() 函数的作用是从 DTB memory reserve mapping
中读出特定的表项目。参数 fdt 指向 DTB，参数 n 指向表项的偏移。
函数首先计算表项在 DTB memory reserve mapping 中的偏移，
然后根据这个偏移，调用 fdt_off_mem_rswmap() 函数计算出表项
的虚拟地址，函数对表项的虚拟地址进行检测，如果检测通过，那么函数
直接调用 fdt_mem_rsv_() 函数获得该表项。

> - [fdt_off_mem_rsvmap](#A0081)
>
> - [fdt_totalsize](#A0078)
>
> - [fdt_mem_rsv_](#A0215)

------------------------------------

#### <span id="A0218">fdt_get_mem_rsv</span>

{% highlight c %}
int fdt_get_mem_rsv(const void *fdt, int n, uint64_t *address, uint64_t *size)
{       
        const struct fdt_reserve_entry *re;

        FDT_RO_PROBE(fdt);
        re = fdt_mem_rsv(fdt, n);
        if (!re)
                return -FDT_ERR_BADOFFSET;

        *address = fdt64_ld(&re->address);
        *size = fdt64_ld(&re->size);
        return 0;
}
{% endhighlight %}

fdt_get_mem_rsv() 函数用于获得 DTB 保留区中指定区域的地址
和长度。参数 fdt 指向 DTB，n 指向 DTB memory reserve mapping
的偏移，参数 address 用于存储找到的地址，参数 size 用于存储
找到的长度。函数首先调用 FDT_RO_PROBE() 函数检查 DTB 的健全性，
然后调用 fdt_mem_rsv() 函数获得保留区的入口地址，如果此时入口
地址为空，那么直接返回 -FDT_ERR_BADOFFSET；反之如果找到保留区
入口地址，那么函数通过 fdt64_ld() 函数将保留区的长度和起始地址
存储到参数 address 和 size 里。

> - [FDT_RO_PROBE](#A0219)
>
> - [fdt_mem_rsv](#A0217)
>
> - [fdt64_ld](#A0220)

------------------------------------

#### <span id="A0219">FDT_RO_PROBE</span>

{% highlight c %}
#define FDT_RO_PROBE(fdt)                       \
        { \
                int err_; \
                if ((err_ = fdt_ro_probe_(fdt)) != 0)   \
                        return err_; \
        }
{% endhighlight %}

FDT_RO_PROBE() 函数用于检查 DTB 的完整性，如果 DTB
不满足最小健全性检查，那么函数直接返回错误。

> - [fdt_ro_probe_](#A0108)

------------------------------------

#### <span id="A0220">fdt64_ld</span>

{% highlight c %}
static inline uint64_t fdt64_ld(const fdt64_t *p)
{       
        const uint8_t *bp = (const uint8_t *)p;

        return ((uint64_t)bp[0] << 56)
                | ((uint64_t)bp[1] << 48)
                | ((uint64_t)bp[2] << 40)
                | ((uint64_t)bp[3] << 32)
                | ((uint64_t)bp[4] << 24)
                | ((uint64_t)bp[5] << 16)
                | ((uint64_t)bp[6] << 8)
                | bp[7];
}
{% endhighlight %}

fdt64_ld() 函数用于从 DTB 的属性中读取一个 64 位
的属性值。参数 p 指向 64 位属性值所在的地址。由于
DTB 是按大端模式存储，所以需要将数据转换成小端模式
进行存储。

------------------------------------

#### <span id="A0221">__reserved_mem_check_root</span>

{% highlight c %}
/**
 * __reserved_mem_check_root() - check if #size-cells, #address-cells provided
 * in /reserved-memory matches the values supported by the current implementation,
 * also check if ranges property has been provided
 */
static int __init __reserved_mem_check_root(unsigned long node)
{
        const __be32 *prop;

        prop = of_get_flat_dt_prop(node, "#size-cells", NULL);
        if (!prop || be32_to_cpup(prop) != dt_root_size_cells)
                return -EINVAL;

        prop = of_get_flat_dt_prop(node, "#address-cells", NULL);
        if (!prop || be32_to_cpup(prop) != dt_root_addr_cells)
                return -EINVAL;

        prop = of_get_flat_dt_prop(node, "ranges", NULL);
        if (!prop)
                return -EINVAL;
        return 0;
}
{% endhighlight %}

__reserved_mem_check_root() 函数的作用是检查指定 node 的
"#size-cells" 属性值和 "#address-cells" 属性值是否和根节点的
属性值一致，并确保节点包含 "ranges" 属性。函数通过调用
of_get_flat_dt_prop() 函数读取节点指定属性的属性值，然后与
全局变量 dt_root_size_cells 和 dt_root_addr_cells 进行
对比，如果不相同则直接返回 -EINVAL；反之相同，那么函数继续
检查节点是否包含 "ranges" ，如果不包含则返回 -EINVAL；反之
返回 0. 改函数通常用于检查 "reserved-memory" 节点。

> - [of_get_flat_dt_prop](#A0117)

------------------------------------

#### <span id="A0222">of_fdt_device_is_available</span>

{% highlight c %}
static bool of_fdt_device_is_available(const void *blob, unsigned long node)
{
        const char *status = fdt_getprop(blob, node, "status", NULL);

        if (!status)
                return true;

        if (!strcmp(status, "ok") || !strcmp(status, "okay"))
                return true;

        return false;
}
{% endhighlight %}

of_fdt_device_is_available() 函数的作用是检查节点的 "status"
属性值是否为 "okay" 或者 "ok", 以此确保节点对应的驱动是否启用。
参数 blob 指向 DTB，node 参数指向节点的偏移。函数首先调用
fdt_getprop() 函数获得节点的 "status" 属性值，如果属性值为
空，那么函数直接返回 true，代表节点可用；反之如果获得属性值不是 "ok"
或者 "okay"，那么函数返回直接返回 false，代表节点对应的驱动
不启用；反之返回 true。

> - [fdt_getprop](#A0113)

------------------------------------

#### <span id="A0223">fdt_reserved_mem_save_node</span>

{% highlight c %}
#define MAX_RESERVED_REGIONS    32
static struct reserved_mem reserved_mem[MAX_RESERVED_REGIONS];
static int reserved_mem_count;

/**     
 * res_mem_save_node() - save fdt node for second pass initialization
 */     
void __init fdt_reserved_mem_save_node(unsigned long node, const char *uname,
                                      phys_addr_t base, phys_addr_t size)
{
        struct reserved_mem *rmem = &reserved_mem[reserved_mem_count];

        if (reserved_mem_count == ARRAY_SIZE(reserved_mem)) {
                pr_err("not enough space all defined regions.\n");
                return;
        }

        rmem->fdt_node = node;
        rmem->name = uname;
        rmem->base = base;
        rmem->size = size;

        reserved_mem_count++;
        return;
}
{% endhighlight %}

fdt_reserved_mem_save_node() 函数用于将 DTS 中 "reserved-memory"
添加到系统的 reserved_mem[] 数组中。内核使用一个 struct reserved_mem
数组维护着系统中所有的保留区。参数 node 指向节点偏移，参数 uname
指向预留区的名字，参数 base 指向预留区的起始地址，参数 size 指向
预留区的长度。函数首先从全局变量 reserved_mem[] 数组中获得一个空闲的
reserved_mem 结构，如果此时 reserved_mem_count 的数量超出
reserved_mem[] 数组的范围，那么函数将报错，并直接返回；如果检查通过，
函数将新的保留区加入到 reserved_mem[] 内。

> - [ARRAY_SIZE](#A0034)

------------------------------------

#### <span id="A0224">__reserved_mem_reserve_reg</span>

{% highlight c %}
/**
 * res_mem_reserve_reg() - reserve all memory described in 'reg' property
 */
static int __init __reserved_mem_reserve_reg(unsigned long node,
                                             const char *uname)
{
        int t_len = (dt_root_addr_cells + dt_root_size_cells) * sizeof(__be32);
        phys_addr_t base, size;
        int len;
        const __be32 *prop;
        int nomap, first = 1;

        prop = of_get_flat_dt_prop(node, "reg", &len);
        if (!prop)
                return -ENOENT;

        if (len && len % t_len != 0) {
                pr_err("Reserved memory: invalid reg property in '%s', skipping node.\n",              
                       uname);
                return -EINVAL;
        }

        nomap = of_get_flat_dt_prop(node, "no-map", NULL) != NULL;

        while (len >= t_len) {
                base = dt_mem_next_cell(dt_root_addr_cells, &prop);
                size = dt_mem_next_cell(dt_root_size_cells, &prop);

                if (size &&
                    early_init_dt_reserve_memory_arch(base, size, nomap) == 0)
                        pr_debug("Reserved memory: reserved region for node '%s': base %pa, size %ld MiB\n",
                                uname, &base, (unsigned long)size / SZ_1M);
                else    
                        pr_info("Reserved memory: failed to reserve memory for node '%s': base %pa, size %ld MiB\n",
                                uname, &base, (unsigned long)size / SZ_1M);

                len -= t_len;
                if (first) {
                        fdt_reserved_mem_save_node(node, uname, base, size);
                        first = 0;
                }
        }
        return 0;
}
{% endhighlight %}

__reserved_mem_reserve_reg() 函数用于将 DTS 中 "reserved-memory"
节点内的保留区加入到系统中进行维护。参数 node 指向预留区对应的节点，
参数 uname 指向节点的名字。函数首先调用 of_get_flat_dt_prop() 函数
从节点中读取 "reg" 属性的值，该值包含了基地址和长度两部分。然后对属性值
进行检测，如果不符合条件，那么函数直接返回错误码。函数接着从节点中读取
"no-map" 属性值。函数接着使用了一个 while() 循环，在每次循环中，函数
从 "reg" 属性值读取一对数据，其包含了保留区的基地址以及长度，将其存储
在局部变量 base 和 size 里。函数将预留区通过调用
early_init_dt_reserve_memory_arch() 函数加入到 MEMBLOCK 分配器
的预留区内，如果加入失败或者 size 参数有问题，那么函数打印相应的错误
信息，函数接着减少 len 的值，由于 reg 属性可能包含多对 "基地址+长度"
的数据，所以需要遍历所有的数据对。如果 first 为 1，那么函数调用
fdt_reserved_mem_save_node() 函数将保留区加入系统保留区数组内，
并将 first 设置为 0，以此只加入节点的第一个预留区到系统预留区内。

> - [of_get_flat_dt_prop](#A0117)
>
> - [dt_mem_next_cell](#A0129)
>
> - [early_init_dt_reserve_memory_arch](#A0213)
>
> - [fdt_reserved_mem_save_node](#A0223)

------------------------------------

#### <span id="A0225">__fdt_scan_reserved_mem</span>

{% highlight c %}
/**
 * fdt_scan_reserved_mem() - scan a single FDT node for reserved memory
 */
static int __init __fdt_scan_reserved_mem(unsigned long node, const char *uname,
                                          int depth, void *data)
{
        static int found;
        int err;

        if (!found && depth == 1 && strcmp(uname, "reserved-memory") == 0) {
                if (__reserved_mem_check_root(node) != 0) {
                        pr_err("Reserved memory: unsupported node format, ignoring\n");
                        /* break scan */
                        return 1;
                }
                found = 1;
                /* scan next node */
                return 0;
        } else if (!found) {
                /* scan next node */
                return 0;
        } else if (found && depth < 2) {
                /* scanning of /reserved-memory has been finished */
                return 1;
        }

        if (!of_fdt_device_is_available(initial_boot_params, node))
                return 0;

        err = __reserved_mem_reserve_reg(node, uname);
        if (err == -ENOENT && of_get_flat_dt_prop(node, "size", NULL))
                fdt_reserved_mem_save_node(node, uname, 0, 0);

        /* scan next node */
        return 0;
}
{% endhighlight %}

__fdt_scan_reserved_mem() 函数用于将 DTS 中 "reserved-memory" 节点
包含的子节点预留区加入到系统内进行维护。参数 node 指向子节点的
索引，参数 uname 指向子节点的名字，参数 depth 代表子节点的深度，
参数 data 代表私有数据。函数首先判断节点是 "reseved-memory" 节点内的
子节点，如果是再调用 of_fdt_device_is_available() 子节点是否启动，
即几点的 status 是否为 "okay", 如果启用，那么函数调用
__reserved_mem_reserve_reg() 函数将节点的 "reg" 属性对应的预留区
加入到 MEMBLOCK 内存分配器的保留区和系统 reserved_mem[]
数组进行维护。如果 __reserved_mem_reserve_reg() 函数返回 -ENOENT，
且节点的 "size" 属性不为空，那么函数调用 fdt_reserved_mem_save_node()
函数将节点的保留区加入系统 reserved_mem[] 数组进行维护。

> - [\_\_reserved_mem_check_root](#A0221)
>
> - [of_fdt_device_is_available](#A0222)
>
> - [\_\_reserved_mem_reserve_reg](#A0224)
>
> - [of_get_flat_dt_prop](#A0117)
>
> - [fdt_reserved_mem_save_node](#A0223)

------------------------------------

#### <span id="A0226">__rmem_check_for_overlap</span>

{% highlight c %}
static void __init __rmem_check_for_overlap(void)
{
        int i;

        if (reserved_mem_count < 2)
                return;

        sort(reserved_mem, reserved_mem_count, sizeof(reserved_mem[0]),
             __rmem_cmp, NULL);
        for (i = 0; i < reserved_mem_count - 1; i++) {
                struct reserved_mem *this, *next;

                this = &reserved_mem[i];
                next = &reserved_mem[i + 1];
                if (!(this->base && next->base))
                        continue;
                if (this->base + this->size > next->base) {
                        phys_addr_t this_end, next_end;

                        this_end = this->base + this->size;
                        next_end = next->base + next->size;
                        pr_err("OVERLAP DETECTED!\n%s (%pa--%pa) overlaps with %s (%pa--%pa)\n",
                               this->name, &this->base, &this_end,
                               next->name, &next->base, &next_end);
                }
        }
}
{% endhighlight %}

__rmem_check_for_overlap() 函数的作用是检查 reserved_mem[] 维护
的预留区是否存在重叠部分。函数首先判断当前 reserved_mem[] 中预留区
的数量，如果小于 2，那么没必要检测；反之函数首调用 sort() 函数
将 reserved_mem[] 数组的中预留区按基地址地址进行排序。接着函数使用
for 循环遍历 reserved_mem[] 数组中的所有预留区，每次遍历到一个预留区，
如果当前预留区的基地址和下一个预留区的基地址相与为 0，那么结束
本次循环进行下一次循环。如果本次循环在此时不结束，那么函数判断
如果当前预留区的结束地址大于下一个预留区的起始地址，那么存在重叠，
函数此时就会进行报错。

------------------------------------

#### <span id="A0227">memblock_alloc_range_nid</span>

{% highlight c %}
static phys_addr_t __init memblock_alloc_range_nid(phys_addr_t size,
                                        phys_addr_t align, phys_addr_t start,
                                        phys_addr_t end, int nid,
                                        enum memblock_flags flags)
{
        phys_addr_t found;

        if (!align) {
                /* Can't use WARNs this early in boot on powerpc */
                dump_stack();
                align = SMP_CACHE_BYTES;
        }

        found = memblock_find_in_range_node(size, align, start, end, nid,
                                            flags);
        if (found && !memblock_reserve(found, size)) {
                /*
                 * The min_count is set to 0 so that memblock allocations are
                 * never reported as leaks.
                 */
                kmemleak_alloc_phys(found, size, 0, 0);
                return found;
        }
        return 0;
}
{% endhighlight %}

memblock_alloc_range_nid() 函数用于从指定 NUMA 上分配物理内存。
参数 size 指定分配物理内存的长度，参数 align 指定分配物理内存对齐
方式，参数 start 指定分配物理内存的起始物理地址，参数 end 指向
分配物理内存的终止地址，参数 nid 指向 NUMA 信息，参数 flags 指向
分配物理内存的 flags 信息。函数首先检查 align 参数是否有效，如果
无效，那么函数就 dump 堆栈并设置 align 为 SMP_CACHE_BYTES。接着
函数调用 memblock_find_in_range_node() 查找符合要求的物理内存块，
如果找到，那么函数返回起始物理地址，反之返回 0.

> - [memblock_find_in_range_node](#A0155)

------------------------------------

#### <span id="A0228">memblock_alloc_base_nid</span>

{% highlight c %}
phys_addr_t __init memblock_alloc_base_nid(phys_addr_t size,
                                        phys_addr_t align, phys_addr_t max_addr,
                                        int nid, enum memblock_flags flags)
{
        return memblock_alloc_range_nid(size, align, 0, max_addr, nid, flags);
}
{% endhighlight %}

memblock_alloc_base_nid() 用于从指定 NUMA 节点上分配物理内存。
参数 size 指定分配的长度，align 参数指定对齐方式，max_addr 指向
最大可分配物理地址，nid 指定 NUMA 信息，参数 flags 指向分配
物理内存的 flags 信息。函数通过调用 memblock_alloc_range_nid()
函数获得指定物理内存。

> - [memblock_alloc_base_nid](#A0227)

------------------------------------

#### <span id="A0229">__memblock_alloc_base</span>

{% highlight c %}
phys_addr_t __init __memblock_alloc_base(phys_addr_t size, phys_addr_t align, phys_addr_t max_addr)
{
        return memblock_alloc_base_nid(size, align, max_addr, NUMA_NO_NODE,
                                       MEMBLOCK_NONE);
}
{% endhighlight %}

__memblock_alloc_base() 函数用于分配指定大小的物理内存。参数 size
指向分配物理内存的大小，参数 align 指向对齐方式，参数 max_addr 指向
可分配的最大物理地址。函数通过调用 memblock_alloc_base_nid 实现。

> - [memblock_alloc_base_nid](#A0228)

------------------------------------

#### <span id="A0230">early_init_dt_alloc_reserved_memory_arch</span>

{% highlight c %}
int __init __weak early_init_dt_alloc_reserved_memory_arch(phys_addr_t size,
        phys_addr_t align, phys_addr_t start, phys_addr_t end, bool nomap,
        phys_addr_t *res_base)
{
        phys_addr_t base;
        /*
         * We use __memblock_alloc_base() because memblock_alloc_base()
         * panic()s on allocation failure.
         */
        end = !end ? MEMBLOCK_ALLOC_ANYWHERE : end;
        align = !align ? SMP_CACHE_BYTES : align;
        base = __memblock_alloc_base(size, align, end);
        if (!base)
                return -ENOMEM;

        /*
         * Check if the allocated region fits in to start..end window
         */
        if (base < start) {
                memblock_free(base, size);
                return -ENOMEM;
        }

        *res_base = base;
        if (nomap)
                return memblock_remove(base, size);
        return 0;
}
{% endhighlight %}

early_init_dt_alloc_reserved_memory_arch() 函数用于为 DTS 中
"reserved-memory" 节点中的特定预留区分配物理内存，该类预留区
节点不包含 "reg" 属性，只包含 "size" 属性，因此需要对这里预留区
分配物理内存。参数 size 指向分配的大小，参数 align 指向对齐方式，
参数 start 指向可以分配的起始地址，end 参数指向可以分配的终止
地址，参数 nomap 指向是否为预留区内存建立页表，参数 res_base
用于存储分配物理内存的起始地址。函数首先处理 end 参数和 align
参数，使其符合要求，然后调用 __memblock_alloc_base() 函数从
指定的物理内存中分配所需要大小的物理内存，并返回分配成功的物理
起始地址。如果此时分配失败，直接返回 -ENOMEM；反之检查分配成功
的物理块的起始地址是否小于指定的起始物理地址，如果小于，则调用
memblock_free() 函数释放掉刚刚分配的物理内存；反之即分配的物理
内存符合要求，然后将分配成功的起始物理地址存储在 res_base 里面。
如果此时 nomap 参数为真，那么表示不需要对新分配的物理内存建立
页表，于是调用 memblock_remove() 函数将该段物理内存块从
MEMBLOCK 内存分配器 memory regions 中移除。最后返回 0.

> - [\_\_memblock_alloc_base](#A0229)
>
> - [memblock_free](#A0162)
>
> - [memblock_remove](#A0200)

------------------------------------

#### <span id="A0231">__reserved_mem_alloc_size</span>

{% highlight c %}
/**
 * res_mem_alloc_size() - allocate reserved memory described by 'size', 'align'
 *                        and 'alloc-ranges' properties
 */
static int __init __reserved_mem_alloc_size(unsigned long node,
        const char *uname, phys_addr_t *res_base, phys_addr_t *res_size)
{
        int t_len = (dt_root_addr_cells + dt_root_size_cells) * sizeof(__be32);
        phys_addr_t start = 0, end = 0;
        phys_addr_t base = 0, align = 0, size;
        int len;
        const __be32 *prop;
        int nomap;
        int ret;

        prop = of_get_flat_dt_prop(node, "size", &len);
        if (!prop)
                return -EINVAL;

        if (len != dt_root_size_cells * sizeof(__be32)) {
                pr_err("invalid size property in '%s' node.\n", uname);
                return -EINVAL;
        }
        size = dt_mem_next_cell(dt_root_size_cells, &prop);

        nomap = of_get_flat_dt_prop(node, "no-map", NULL) != NULL;

        prop = of_get_flat_dt_prop(node, "alignment", &len);
        if (prop) {
                if (len != dt_root_addr_cells * sizeof(__be32)) {
                        pr_err("invalid alignment property in '%s' node.\n",
                                uname);
                        return -EINVAL;
                }
                align = dt_mem_next_cell(dt_root_addr_cells, &prop);
        }

        /* Need adjust the alignment to satisfy the CMA requirement */
        if (IS_ENABLED(CONFIG_CMA)
            && of_flat_dt_is_compatible(node, "shared-dma-pool")
            && of_get_flat_dt_prop(node, "reusable", NULL)
            && !of_get_flat_dt_prop(node, "no-map", NULL)) {
                unsigned long order =
                        max_t(unsigned long, MAX_ORDER - 1, pageblock_order);

                align = max(align, (phys_addr_t)PAGE_SIZE << order);
        }

        prop = of_get_flat_dt_prop(node, "alloc-ranges", &len);
        if (prop) {

                if (len % t_len != 0) {
                        pr_err("invalid alloc-ranges property in '%s', skipping node.\n",
                               uname);
                        return -EINVAL;
                }

                base = 0;

                while (len > 0) {
                        start = dt_mem_next_cell(dt_root_addr_cells, &prop);
                        end = start + dt_mem_next_cell(dt_root_size_cells,
                                                       &prop);

                        pr_info("\n\nStart: %#lx END: %#lx\n", start, end);
                        ret = early_init_dt_alloc_reserved_memory_arch(size,
                                        align, start, end, nomap, &base);
                        if (ret == 0) {
                                pr_debug("allocated memory for '%s' node: base %pa, size %ld MiB\n",
                                        uname, &base,
                                        (unsigned long)size / SZ_1M);
                                break;
                        }
                        len -= t_len;
                }

        } else {
                ret = early_init_dt_alloc_reserved_memory_arch(size, align,
                                                        0, 0, nomap, &base);
                if (ret == 0)
                        pr_debug("allocated memory for '%s' node: base %pa, size %ld MiB\n",
                                uname, &base, (unsigned long)size / SZ_1M);
        }

        if (base == 0) {
                pr_info("failed to allocate memory for node '%s'\n", uname);
                return -ENOMEM;
        }

        *res_base = base;
        *res_size = size;

        return 0;
}
{% endhighlight %}

__reserved_mem_alloc_size() 用于为 DTS "reserved-memory" 节点
中特定子节点分配内存，该类节点包含 "size" 属性，但不包含 "reg"
属性，因此需要对这类子节点分配物理内存，这类子节点例如：

{% highlight bash %}
        reserved-memory {
                #address-cells = <1>;
                #size-cells = <1>;
                ranges;

                default-pool {
                        compatible = "shared-dma-pool";
                        size = <0x2000000>;
                        alloc-ranges = <0x61000000 0x4000000>;
                        alignment = <0x100000>;
                        reusable;
                        linux,cma-default;
                };
        };
{% endhighlight %}

由于代码太长，分段解析：

{% highlight c %}
static int __init __reserved_mem_alloc_size(unsigned long node,
        const char *uname, phys_addr_t *res_base, phys_addr_t *res_size)
{
        int t_len = (dt_root_addr_cells + dt_root_size_cells) * sizeof(__be32);
        phys_addr_t start = 0, end = 0;
        phys_addr_t base = 0, align = 0, size;
        int len;
        const __be32 *prop;
        int nomap;
        int ret;

        prop = of_get_flat_dt_prop(node, "size", &len);
        if (!prop)
                return -EINVAL;

        if (len != dt_root_size_cells * sizeof(__be32)) {
                pr_err("invalid size property in '%s' node.\n", uname);
                return -EINVAL;
        }
        size = dt_mem_next_cell(dt_root_size_cells, &prop);

        nomap = of_get_flat_dt_prop(node, "no-map", NULL) != NULL;

        prop = of_get_flat_dt_prop(node, "alignment", &len);
        if (prop) {
                if (len != dt_root_addr_cells * sizeof(__be32)) {
                        pr_err("invalid alignment property in '%s' node.\n",
                                uname);
                        return -EINVAL;
                }
                align = dt_mem_next_cell(dt_root_addr_cells, &prop);
        }
{% endhighlight %}

参数 node 指向需要预留的节点，参数 uname 指向节点的名字，参数
res_base 用于存储分配成功的物理内存的起始物理地址，参数 res_size
用于存储分配到物理内存的长度。函数首先从节点中读取 "size"
属性的值，并对属性值进行检测。接着调用 of_get_flat_dt_prop()
函数获得 "no-map" 属性值。函数继续获得 "alignment" 属性值，
并对属性值进行检测。

{% highlight c %}
        /* Need adjust the alignment to satisfy the CMA requirement */
        if (IS_ENABLED(CONFIG_CMA)
            && of_flat_dt_is_compatible(node, "shared-dma-pool")
            && of_get_flat_dt_prop(node, "reusable", NULL)
            && !of_get_flat_dt_prop(node, "no-map", NULL)) {
                unsigned long order =
                        max_t(unsigned long, MAX_ORDER - 1, pageblock_order);

                align = max(align, (phys_addr_t)PAGE_SIZE << order);
        }
{% endhighlight %}

函数测试内核是否打开 CONFIG_CMA 宏，即系统是否支持 CMA，如果系统支持
CMA，那么函数同时检测节点是否包含 "shared-dma-pool" 和 "reusable" 属性，
以及不包含 "no-map" 属性，如果满足上面的条件，此时调整 align 参数。

{% highlight c %}
        prop = of_get_flat_dt_prop(node, "alloc-ranges", &len);
        if (prop) {

                if (len % t_len != 0) {
                        pr_err("invalid alloc-ranges property in '%s', skipping node.\n",
                               uname);
                        return -EINVAL;
                }

                base = 0;

                while (len > 0) {
                        start = dt_mem_next_cell(dt_root_addr_cells, &prop);
                        end = start + dt_mem_next_cell(dt_root_size_cells,
                                                       &prop);

                        pr_info("\n\nStart: %#lx END: %#lx\n", start, end);
                        ret = early_init_dt_alloc_reserved_memory_arch(size,
                                        align, start, end, nomap, &base);
                        if (ret == 0) {
                                pr_debug("allocated memory for '%s' node: base %pa, size %ld MiB\n",
                                        uname, &base,
                                        (unsigned long)size / SZ_1M);
                                break;
                        }
                        len -= t_len;
                }

        } else {
                ret = early_init_dt_alloc_reserved_memory_arch(size, align,
                                                        0, 0, nomap, &base);
                if (ret == 0)
                        pr_debug("allocated memory for '%s' node: base %pa, size %ld MiB\n",
                                uname, &base, (unsigned long)size / SZ_1M);
        }
{% endhighlight %}

函数继续调用 of_get_flat_dt_prop() 函数读取节点 "alloc-ranges" 属性
值，以此确定节点预留内存的地址范围。其中 start 指向可用物理地址的起始地址，
end 指向可用物理地址的终止地址。函数在这段地址范围内查找 size 大小的物理
内存块，通过调用 early_init_dt_alloc_reserved_memory_arch() 函数进行
分配。如果此时分配失败则报错；如果节点不包含 "alloc-ranges" 属性，那么
函数就从系统可用物理内存中找一块满足要求的物理内存块，通过调用
early_init_dt_alloc_reserved_memory_arch() 函数实现，分配失败则报错。

{% highlight c %}
        if (base == 0) {
                pr_info("failed to allocate memory for node '%s'\n", uname);
                return -ENOMEM;
        }

        *res_base = base;
        *res_size = size;

        return 0;
{% endhighlight %}

需要的物理内存分配成功之后，函数将获得物理内存起始物理地址存储在
res_base 中，将长度存储在 res_size 中，最后返回 0.

> - [of_get_flat_dt_prop](#A0117)
>
> - [dt_mem_next_cell](#A0129)
>
> - [of_flat_dt_is_compatible](#A0115)
>
> - [early_init_dt_alloc_reserved_memory_arch](#A0230)

------------------------------------

#### <span id="A0232">_OF_DECLARE</span>

{% highlight c %}
#if defined(CONFIG_OF) && !defined(MODULE)
#define _OF_DECLARE(table, name, compat, fn, fn_type)                   \
        static const struct of_device_id __of_table_##name              \
                __used __section(__##table##_of_table)                  \
                 = { .compatible = compat,                              \
                     .data = (fn == (fn_type)NULL) ? fn : fn  }
#else    
#define _OF_DECLARE(table, name, compat, fn, fn_type)                   \
        static const struct of_device_id __of_table_##name              \
                __attribute__((unused))                                 \
                 = { .compatible = compat,                              \
                     .data = (fn == (fn_type)NULL) ? fn : fn }
#endif
{% endhighlight %}

_OF_DECLARE() 宏用于定义一个 DTS compatible 表。宏定义了
一个 struct of_device_id 结构，并将该改数据结构存储在私有
section 内，宏为数据结构提供了 compat 和 fn 两个初始参数。

------------------------------------

#### <span id="A0233">RESERVEDMEM_OF_DECLARE</span>

{% highlight c %}
#define RESERVEDMEM_OF_DECLARE(name, compat, init)                      \
        _OF_DECLARE(reservedmem, name, compat, init, reservedmem_of_init_fn)
{% endhighlight %}

RESERVEDMEM_OF_DECLARE 宏用在 __reservedmem_of_table section
内建立一个 struct of_device_id, 其 compat 成员就是 compat 参数，
并且该数据结构的 data 成员是就是 init 参数。该宏通过 _OF_DECLARE
宏实现。在内核源码中，通过这个宏可以自定私有 DTS "reserved-memory"
节点中，其子节点 compatile 为特定值的初始化函数。

------------------------------------

#### <span id="A0234">cma_init_reserved_mem</span>

{% highlight c %}
/**
 * cma_init_reserved_mem() - create custom contiguous area from reserved memory
 * @base: Base address of the reserved area
 * @size: Size of the reserved area (in bytes),
 * @order_per_bit: Order of pages represented by one bit on bitmap.
 * @name: The name of the area. If this parameter is NULL, the name of
 *        the area will be set to "cmaN", where N is a running counter of
 *        used areas.
 * @res_cma: Pointer to store the created cma region.
 *
 * This function creates custom contiguous area from already reserved memory.
 */
int __init cma_init_reserved_mem(phys_addr_t base, phys_addr_t size,
                                 unsigned int order_per_bit,
                                 const char *name,
                                 struct cma **res_cma)
{
        struct cma *cma;
        phys_addr_t alignment;

        /* Sanity checks */
        if (cma_area_count == ARRAY_SIZE(cma_areas)) {
                pr_err("Not enough slots for CMA reserved regions!\n");
                return -ENOSPC;
        }

        if (!size || !memblock_is_region_reserved(base, size))
                return -EINVAL;

        /* ensure minimal alignment required by mm core */
        alignment = PAGE_SIZE <<
                        max_t(unsigned long, MAX_ORDER - 1, pageblock_order);

        /* alignment should be aligned with order_per_bit */
        if (!IS_ALIGNED(alignment >> PAGE_SHIFT, 1 << order_per_bit))
                return -EINVAL;

        if (ALIGN(base, alignment) != base || ALIGN(size, alignment) != size)
                return -EINVAL;

        /*
         * Each reserved area must be initialised later, when more kernel
         * subsystems (like slab allocator) are available.
         */
        cma = &cma_areas[cma_area_count];
        if (name) {
                cma->name = name;
        } else {
                cma->name = kasprintf(GFP_KERNEL, "cma%d\n", cma_area_count);
                if (!cma->name)
                        return -ENOMEM;
        }
        cma->base_pfn = PFN_DOWN(base);
        cma->count = size >> PAGE_SHIFT;
        cma->order_per_bit = order_per_bit;
        *res_cma = cma;
        cma_area_count++;
        totalcma_pages += (size / PAGE_SIZE);

        return 0;
}
{% endhighlight %}

cma_init_reserved_mem() 函数用于添加并初始化一个 CMA region。内核支持多块
CMA，每一个块 CMA 称为 CMA region。CMA 将所有的 CMA region 维护
在 cma_areas[] 数组里，使用全局变量 cma_area_count 指明当前 CMA
region 的数量。函数代码较长，分段解析：

{% highlight c %}
int __init cma_init_reserved_mem(phys_addr_t base, phys_addr_t size,
                                 unsigned int order_per_bit,
                                 const char *name,
                                 struct cma **res_cma)
{
        struct cma *cma;
        phys_addr_t alignment;

        /* Sanity checks */
        if (cma_area_count == ARRAY_SIZE(cma_areas)) {
                pr_err("Not enough slots for CMA reserved regions!\n");
                return -ENOSPC;
        }
{% endhighlight %}

参数 base 指向 CMA region 的基地址，size 参数指向其长度，参数
order_per_bit 指向 bitmap，参数 name 指向 CMA region 的名字，
参数 res_cma 指向新分配的 cma 结构。函数首先判断当前 CMA regions
数量是否已经达到 cma_areas[] 数组支持的最大值，如果已经达到，
那么函数提示错误消息之后，返回错误码。

{% highlight c %}
        if (!size || !memblock_is_region_reserved(base, size))
                return -EINVAL;

        /* ensure minimal alignment required by mm core */
        alignment = PAGE_SIZE <<
                        max_t(unsigned long, MAX_ORDER - 1, pageblock_order);

        /* alignment should be aligned with order_per_bit */
        if (!IS_ALIGNED(alignment >> PAGE_SHIFT, 1 << order_per_bit))
                return -EINVAL;

        if (ALIGN(base, alignment) != base || ALIGN(size, alignment) != size)
                return -EINVAL;
{% endhighlight %}

函数接着调用 memblock_is_region_reserved() 函数判断新增加
的 CMA region 对应的物理块是否在 MEMBLOCK 分配器的预留区内，
如果不属于且长度为 0，那么函数直接返回错误码。如果对齐并未按
order_per_bit 的方式对齐，那么函数直接返回错误码。

{% highlight c %}
        /*
         * Each reserved area must be initialised later, when more kernel
         * subsystems (like slab allocator) are available.
         */
        cma = &cma_areas[cma_area_count];
        if (name) {
                cma->name = name;
        } else {
                cma->name = kasprintf(GFP_KERNEL, "cma%d\n", cma_area_count);
                if (!cma->name)
                        return -ENOMEM;
        }
        cma->base_pfn = PFN_DOWN(base);
        cma->count = size >> PAGE_SHIFT;
        cma->order_per_bit = order_per_bit;
        *res_cma = cma;
        cma_area_count++;
        totalcma_pages += (size / PAGE_SIZE);

        return 0;
{% endhighlight %}

cma_area_count 代表目前系统含有 CMA region 的数量，函数
从 cma_areas[] 数组中获得一个 cma 结构，如果此时 name 参数
有效，那么将新 CMA 名字设置为 name；反之使用 ksprintf()
函数分配一个新名字。接下来初始化新的 cma 结构，base_pfn
成员指向 CMA region 基地址的页帧，count 成员设置为
CMA region 包含的 page 数。order_per_bit 设置为参数
order_per_bit，将新 cma 结构存储到 res_cma 参数，
更新 cma_area_count，最后增加 cma totalcma_pages
引用计数。

> - [ARRAY_SIZE](#A0034)
>
> - [memblock_is_region_reserved](#A0210)

------------------------------------

#### <span id="A0235">dma_contiguous_early_fixup</span>

{% highlight c %}
void __init dma_contiguous_early_fixup(phys_addr_t base, unsigned long size)
{
        dma_mmu_remap[dma_mmu_remap_num].base = base;
        dma_mmu_remap[dma_mmu_remap_num].size = size;
        dma_mmu_remap_num++;
}
{% endhighlight %}

dma_contiguous_early_fixup() 函数的作用就是将一块物理内存添加到
dma_mmu_remap[] 数组进行维护，函数使用 dma_mmu_remap_num 变量
dma_mmu_remap[] 数组成员的数量。

------------------------------------

#### <span id="A0236">dma_contiguous_set_default</span>

{% highlight c %}
static inline void dma_contiguous_set_default(struct cma *cma)
{
        dma_contiguous_default_area = cma;
}
{% endhighlight %}

dma_contiguous_set_default() 函数用于设置默认的 CMA region。
当系统支持多个 CMA region，该函数就用于设置默认的 CMA region。
dma_contiguous_default_area 用于维护默认的 CMA region。

------------------------------------

#### <span id="A0237">dev_set_cma_area</span>

{% highlight c %}
static inline void dev_set_cma_area(struct device *dev, struct cma *cma)
{
        if (dev)
                dev->cma_area = cma;
}
{% endhighlight %}

dev_set_cma_area() 函数用于设置设备使用的 CMA region。参数
dev 指向设备，cma 指向 CMA region。函数首先判断 dev 是否
存在，如果存在，则将 cma_area 成员指向 cma，这样该设备就从
这个 CMA region 获得物理内存。

------------------------------------

#### <span id="A0238">rmem_cma_device_init</span>

{% highlight c %}
static int rmem_cma_device_init(struct reserved_mem *rmem, struct device *dev)
{
        dev_set_cma_area(dev, rmem->priv);
        return 0;
}
{% endhighlight %}

rmem_cma_device_init() 函数用于在 CMA region 初始化过程中，
设置 CMA region 对应的 device。参数 rmem 指向 CMA region，
参数 dev 指向特定设备。

> - [dev_set_cma_area](#A0237)

------------------------------------

#### <span id="A0239">rmem_cma_device_release</span>

{% highlight c %}
static void rmem_cma_device_release(struct reserved_mem *rmem,
                                    struct device *dev)
{
        dev_set_cma_area(dev, NULL);
}
{% endhighlight %}

rmem_cma_device_release() 解除 device 的 CMA region。
函数通过 dev_set_cma_area() 函数将 dev 对应的设备解除
绑定。

------------------------------------

#### <span id="A0240">rmem_cma_setup</span>

{% highlight c %}
static int __init rmem_cma_setup(struct reserved_mem *rmem)
{
        phys_addr_t align = PAGE_SIZE << max(MAX_ORDER - 1, pageblock_order);
        phys_addr_t mask = align - 1;
        unsigned long node = rmem->fdt_node;
        struct cma *cma;
        int err;

        if (!of_get_flat_dt_prop(node, "reusable", NULL) ||
            of_get_flat_dt_prop(node, "no-map", NULL))
                return -EINVAL;

        if ((rmem->base & mask) || (rmem->size & mask)) {
                pr_err("Reserved memory: incorrect alignment of CMA region\n");
                return -EINVAL;
        }

        err = cma_init_reserved_mem(rmem->base, rmem->size, 0, rmem->name, &cma);
        if (err) {
                pr_err("Reserved memory: unable to setup CMA region\n");
                return err;
        }
        /* Architecture specific contiguous memory fixup. */
        dma_contiguous_early_fixup(rmem->base, rmem->size);

        if (of_get_flat_dt_prop(node, "linux,cma-default", NULL))
                dma_contiguous_set_default(cma);

        rmem->ops = &rmem_cma_ops;
        rmem->priv = cma;

        pr_info("Reserved memory: created CMA memory pool at %pa, size %ld MiB\n",
                &rmem->base, (unsigned long)rmem->size / SZ_1M);

        return 0;
}
RESERVEDMEM_OF_DECLARE(cma, "shared-dma-pool", rmem_cma_setup);
{% endhighlight %}

rmem_cma_setup() 函数的作用就是作为 DTS 中，"reserved-memory" 节点
的子节点，其 compatible 属性值为 "shared-dma-pool" ，函数用于初始化
该节点为 CMA region。参数 rmem 指向 reserved_mem[] 数组中的一个成员。
函数首先从 rmem 对应的节点中查找 "reusable" 和 "no-map" 属性，如果
节点包含了 "no-map" 属性或不包含 "reusable" 属性，那么函数直接返回
错误码。接着函数检查 rmem 对应的起始物理地址和长度是否按 mask 方式对齐，
如果不对齐直接返回错误码。函数接着调用 cma_iit_reserved_mem() 函数将
该区域作为一个新的 CMA region 加入到 CMA 内存分配器内，如果添加失败，
则打印错误信息并返回错误码。函数继续调用 dma_contiguous_early_fixup()
函数将该 CMA region 加入到 dma_mmu_remap[] 数组里面。函数从 rmem
对应的节点中读取 "linux,cma-default" 属性，如果该属性存在，那么该
CMA region 作为 CMA 内存管理器默认的 CMA region (内核可以同时支持
多个 CMA regions)。最后将 rmem 的 ops 成员设置为 rmem_cma_ops,
其包含了两个具体的实现接口 rmem_cma_device_init() 函数用于设置
设备使用的 CMA region，以及 rmem_cma_device_release() 函数用于
解除对特定 CMA region 的绑定。函数最后将 rmem 的 priv 成员指向了
CMA region，即 cma_init_reserved_mem() 函数返回的 cma。

函数通过 RESERVEDMEM_OF_DECLARE() 宏将 rmem_cma_setup 加入到
__reservedmem_of_table section 内并建立一个 struct of_device_id,
该结构的 compatible 指向 "shared-dma-pool".

> - [of_get_flat_dt_prop](#A0117)
>
> - [cma_init_reserved_mem](#A0234)
>
> - [dma_contiguous_early_fixup](#A0235)
>
> - [dma_contiguous_set_default](#A0236)
>
> - [rmem_cma_device_init](#A0238)
>
> - [rmem_cma_device_release](#A0239)
>
> - [RESERVEDMEM_OF_DECLARE](#A0233)

------------------------------------

#### <span id="A0241">__reserved_mem_init_node</span>

{% highlight c %}
/**
 * res_mem_init_node() - call region specific reserved memory init code
 */
static int __init __reserved_mem_init_node(struct reserved_mem *rmem)
{
        extern const struct of_device_id __reservedmem_of_table[];
        const struct of_device_id *i;

        for (i = __reservedmem_of_table; i < &__rmem_of_table_sentinel; i++) {
                reservedmem_of_init_fn initfn = i->data;
                const char *compat = i->compatible;

                if (!of_flat_dt_is_compatible(rmem->fdt_node, compat))
                        continue;

                if (initfn(rmem) == 0) {
                        pr_info("initialized node %s, compatible id %s\n",
                                rmem->name, compat);
                        return 0;
                }
        }
        return -ENOENT;
}
{% endhighlight %}

__reserved_mem_init_node() 用于初始化 rmem 对应的节点。参数 rmem 为
reserved_mem[] 数组中的成员。函数首先获得 __reservedmem_of_table 表
基地址，表里都是 __reservedmem_of_table section 内成员，每个成员都
包含特定的初始化程序。函数遍历表中的所有成员，如果 rmem 对应节点的
compatible 属性值与遍历到成员的 compatible 相同，那么 rmem 就使用
成员的初始化函数初始化该节点。该函数会将节点进行 DMA 或者 CMA 初始化，
关键看 rmem 对应节点是 CMA 还是 DMA，如果是 CMA，那么初始化函数就是
rmem_cma_setup()；如果节点是 DMA，那么初始化函数就是 rmem_dma_setup().

> - [rmem_cma_setup](#A0240)
>
> - [rmem_dma_setup](#A0242)

------------------------------------

#### <span id="A0242">rmem_dma_setup</span>

{% highlight c %}
static int __init rmem_dma_setup(struct reserved_mem *rmem)
{
        unsigned long node = rmem->fdt_node;

        if (of_get_flat_dt_prop(node, "reusable", NULL))
                return -EINVAL;

#ifdef CONFIG_ARM
        if (!of_get_flat_dt_prop(node, "no-map", NULL)) {
                pr_err("Reserved memory: regions without no-map are not yet supported\n");
                return -EINVAL;
        }

        if (of_get_flat_dt_prop(node, "linux,dma-default", NULL)) {
                WARN(dma_reserved_default_memory,
                     "Reserved memory: region for default DMA coherent area is redefined\n");
                dma_reserved_default_memory = rmem;
        }
#endif

        rmem->ops = &rmem_dma_ops;
        pr_info("Reserved memory: created DMA memory pool at %pa, size %ld MiB\n",
                &rmem->base, (unsigned long)rmem->size / SZ_1M);
        return 0;
}
{% endhighlight %}

rmem_dma_setup() 函数用于初始化 DMA 的预留区。参数 rmem 是
reserved_mem[] 数组中的成员，并且 rmem 对应的节点是一个 DMA。
函数首先判断 rmem 对应的节点是否包含 reusable 属性，如果包含，
则返回错误，由于 DMA 的物理内存域是不能给其他程序使用，所以要
将 DMA 设置为独占，但 "reusable" 属性的存在代表不独占重复使用，
这正好和 DMA 的定义相反，所以直接返回错误码。如果宏 CONFIG_ARM
定义，那么函数从节点读取属性 "no-map", 如果节点没有改属性，那么
函数返回错误码，DMA 节点必须带 "no-map" 属性。如果节点包含属性
"linux,dma-default" 属性，那么函数将该节点设置为 DMA 默认分配
的 DMA region (DMA 支持多个 DMA region). 函数最后将 rmem_dma_ops
赋值到 rmem->ops 内，其包含了两个具体的实现: rmem_dma_device_init()
函数用于 DMA device 的初始化，rmem_dma_device_release() 函数
用于 DMA device release.

------------------------------------

#### <span id="A0243">fdt_init_reserved_mem</span>

{% highlight c %}
/**
 * fdt_init_reserved_mem - allocate and init all saved reserved memory regions
 */
void __init fdt_init_reserved_mem(void)
{
        int i;

        /* check for overlapping reserved regions */
        __rmem_check_for_overlap();

        for (i = 0; i < reserved_mem_count; i++) {
                struct reserved_mem *rmem = &reserved_mem[i];
                unsigned long node = rmem->fdt_node;
                int len;
                const __be32 *prop;
                int err = 0;

                prop = of_get_flat_dt_prop(node, "phandle", &len);
                if (!prop)
                        prop = of_get_flat_dt_prop(node, "linux,phandle", &len);
                if (prop)
                        rmem->phandle = of_read_number(prop, len/4);

                if (rmem->size == 0)
                        err = __reserved_mem_alloc_size(node, rmem->name,
                                                 &rmem->base, &rmem->size);
                if (err == 0)
                        __reserved_mem_init_node(rmem);
        }
}
{% endhighlight %}

fdt_init_reserved_mem() 函数的作用为 reserved_mem[] 数组内的保留区
分配内存并初始化预留区。函数首先调用 __rmem_check_for_overlap() 函数
检查 reserved_mem[] 数组对应的预留区是否存在重叠部分，如果存在则报错。
函数接着使用 for 循环遍历 reserved_mem[] 数组内的所有预留区，每个
预留区对应一个节点，函数首先获得节点的 "phandle" 或 "linux,phandle"
属性，并将该属性对应的属性值存储在预留区 rmem 的 phandle 内。如果
预留区 rmem 的 size 为 0，那么函数调用 __reserved_mem_alloc_size()
函数为预留区分配内存，如果分配成功，函数继续调用
__reserved_mem_init_node() 函数初始化预留区，预留区可能是 DMA，也可能
是 CMA。

> - [\_\_rmem_check_for_overlap](#A0226)
>
> - [of_get_flat_dt_prop](#A0117)
>
> - [of_read_number](#A0123)
>
> - [\_\_reserved_mem_alloc_size](#A0231)
>
> - [\_\_reserved_mem_init_node](#A0241)

------------------------------------

#### <span id="A0244">early_init_fdt_scan_reserved_mem</span>

{% highlight c %}
/**
 * early_init_fdt_scan_reserved_mem() - create reserved memory regions
 *
 * This function grabs memory from early allocator for device exclusive use
 * defined in device tree structures. It should be called by arch specific code
 * once the early allocator (i.e. memblock) has been fully activated.
 */
void __init early_init_fdt_scan_reserved_mem(void)
{
        int n;
        u64 base, size;

        if (!initial_boot_params)
                return;

        /* Process header /memreserve/ fields */
        for (n = 0; ; n++) {
                fdt_get_mem_rsv(initial_boot_params, n, &base, &size);
                if (!size)
                        break;
                early_init_dt_reserve_memory_arch(base, size, 0);
        }

        of_scan_flat_dt(__fdt_scan_reserved_mem, NULL);
        fdt_init_reserved_mem();
}
{% endhighlight %}

early_init_fdt_scan_reserved_mem() 函数用于将 DTB 中
"memory reserved mapping" 区域内的预留区加入到 MEMBLOCK
内存分配器的预留区，并将 DTS "reserved-memory" 节点内的子节点
加入到系统预留区，并初始化节点。函数首先判断 initial_boot_params
的有效性，其指向 DTB 所在的虚拟地址。如果有效，函数继续调用 for
循环，遍历 DTB "memory reserved mapping" 区域内的所有预留区，
并调用 fdt_get_mem_rsv() 函数将这些预留区的起始地址和长度传递给
early_init_dt_reserve_memory_arch() 函数，该函数将预留区加入到
MEMBLOCK 分配器的预留区内。遍历完 DTB 中的预留区，函数继续调用
of_scan_flat_dt() 函数和 __fdt_scan_reserved_mem() 函数将
DTS "reserved-memory" 节点中的子节点对应的预留区加入到系统
的 reserved_mem[] 数组，并在 fdt_init_reserved_mem() 函数
中为 reserved_mem[] 数组成员分配内存，并初始化这些预留区。
至此，系统 DTS/DTB 提供的预留区初始化完毕。

> - [fdt_get_mem_rsv](#A0218)
>
> - [early_init_dt_reserve_memory_arch](#A0213)
>
> - [of_scan_flat_dt](#A0127)
>
> - [\_\_fdt_scan_reserved_mem](#A0225)
>
> - [fdt_init_reserved_mem](#A0243)

------------------------------------

#### <span id="A0245">cma_early_percent_memory</span>

{% highlight c %}
static phys_addr_t __init __maybe_unused cma_early_percent_memory(void)
{
        struct memblock_region *reg;
        unsigned long total_pages = 0;

        /*
         * We cannot use memblock_phys_mem_size() here, because
         * memblock_analyze() has not been called yet.
         */
        for_each_memblock(memory, reg)
                total_pages += memblock_region_memory_end_pfn(reg) -
                               memblock_region_memory_base_pfn(reg);

        return (total_pages * CONFIG_CMA_SIZE_PERCENTAGE / 100) << PAGE_SHIFT;
}
{% endhighlight %}

cma_early_percent_memory() 函数的作用是当启用宏 CONFIG_CMA_SIZE_PERCENTAGE,
并且 CMDLINE 中不包含 "cma=" 参数，那么函数从可用物理内存中获得指定
百分比的物理内存，并返回指定百分比内存的长度。函数首先调用 for_each_memblock()
函数计算 MEMBLOCK 分配器目前可用物理内存的数量，然后将可用物理内存
乘与指定的百分比，之后返回该物理内存的大小。在 Linux 中，通过 Kbuild
可以配置 CONFIG_CMA_SIZE_PERCENTAGE 的值。

> - [for_each_memblock](#A0246)

------------------------------------

#### <span id="A0246">for_each_memblock</span>

{% highlight c %}
#define for_each_memblock(memblock_type, region)                                        \
        for (region = memblock.memblock_type.regions;                                   \
             region < (memblock.memblock_type.regions + memblock.memblock_type.cnt);    \       
             region++)
{% endhighlight %}

for_each_memblock() 函数用于遍历 MEMBLOCK memory regions
上所有物理内存区块。函数使用 for 循环遍历 memory 内部所有
region。

------------------------------------

#### <span id="A0247">cma_declare_contiguous</span>

{% highlight c %}
/**
 * cma_declare_contiguous() - reserve custom contiguous area
 * @base: Base address of the reserved area optional, use 0 for any
 * @size: Size of the reserved area (in bytes),
 * @limit: End address of the reserved memory (optional, 0 for any).
 * @alignment: Alignment for the CMA area, should be power of 2 or zero
 * @order_per_bit: Order of pages represented by one bit on bitmap.
 * @fixed: hint about where to place the reserved area
 * @name: The name of the area. See function cma_init_reserved_mem()
 * @res_cma: Pointer to store the created cma region.
 *
 * This function reserves memory from early allocator. It should be
 * called by arch specific code once the early allocator (memblock or bootmem)
 * has been activated and all other subsystems have already allocated/reserved
 * memory. This function allows to create custom reserved areas.
 *      
 * If @fixed is true, reserve contiguous area at exactly @base.  If false,
 * reserve in range from @base to @limit.
 */                     
int __init cma_declare_contiguous(phys_addr_t base,
                        phys_addr_t size, phys_addr_t limit,
                        phys_addr_t alignment, unsigned int order_per_bit,
                        bool fixed, const char *name, struct cma **res_cma)
{
        phys_addr_t memblock_end = memblock_end_of_DRAM();
        phys_addr_t highmem_start;
        int ret = 0;            

        /*
         * We can't use __pa(high_memory) directly, since high_memory
         * isn't a valid direct map VA, and DEBUG_VIRTUAL will (validly)
         * complain. Find the boundary by adding one to the last valid
         * address.
         */
        highmem_start = __pa(high_memory - 1) + 1;
        pr_debug("%s(size %pa, base %pa, limit %pa alignment %pa)\n",
                __func__, &size, &base, &limit, &alignment);

        if (cma_area_count == ARRAY_SIZE(cma_areas)) {
                pr_err("Not enough slots for CMA reserved regions!\n");
                return -ENOSPC;
        }

        if (!size)
                return -EINVAL;        

        if (alignment && !is_power_of_2(alignment))
                return -EINVAL;


        /*
         * Sanitise input arguments.
         * Pages both ends in CMA area could be merged into adjacent unmovable
         * migratetype page by page allocator's buddy algorithm. In the case,
         * you couldn't get a contiguous memory, which is not what we want.
         */
        alignment = max(alignment,  (phys_addr_t)PAGE_SIZE <<
                          max_t(unsigned long, MAX_ORDER - 1, pageblock_order));
        base = ALIGN(base, alignment);
        size = ALIGN(size, alignment);
        limit &= ~(alignment - 1);

        if (!base)
                fixed = false;

        /* size should be aligned with order_per_bit */
        if (!IS_ALIGNED(size >> PAGE_SHIFT, 1 << order_per_bit))
                return -EINVAL;

        /*
         * If allocating at a fixed base the request region must not cross the
         * low/high memory boundary.
         */
        if (fixed && base < highmem_start && base + size > highmem_start) {
                ret = -EINVAL;
                pr_err("Region at %pa defined on low/high memory boundary (%pa)\n",
                        &base, &highmem_start);
                goto err;
        }

        /*
         * If the limit is unspecified or above the memblock end, its effective
         * value will be the memblock end. Set it explicitly to simplify further
         * checks.
         */
        if (limit == 0 || limit > memblock_end)
                limit = memblock_end;

        /* Reserve memory */
        if (fixed) {
                if (memblock_is_region_reserved(base, size) ||
                    memblock_reserve(base, size) < 0) {
                        ret = -EBUSY;
                        goto err;
                }
        } else {
                phys_addr_t addr = 0;

                /*
                 * All pages in the reserved area must come from the same zone.
                 * If the requested region crosses the low/high memory boundary,
                 * try allocating from high memory first and fall back to low
                 * memory in case of failure.
                 */
                if (base < highmem_start && limit > highmem_start) {
                        addr = memblock_alloc_range(size, alignment,
                                                    highmem_start, limit,
                                                    MEMBLOCK_NONE);
                        limit = highmem_start;
                }

                if (!addr) {
                        addr = memblock_alloc_range(size, alignment, base,
                                                    limit,
                                                    MEMBLOCK_NONE);
                        if (!addr) {
                                ret = -ENOMEM;
                                goto err;
                        }
                }

                /*
                 * kmemleak scans/reads tracked objects for pointers to other
                 * objects but this address isn't mapped and accessible
                 */
                kmemleak_ignore_phys(addr);
                base = addr;
        }

        ret = cma_init_reserved_mem(base, size, order_per_bit, name, res_cma);
        if (ret)
                goto err;

        pr_info("Reserved %ld MiB at %pa\n", (unsigned long)size / SZ_1M,
                &base);
        return 0;

err:
        pr_err("Failed to reserve %ld MiB\n", (unsigned long)size / SZ_1M);
        return ret;
}
{% endhighlight %}

cma_declare_contiguous() 函数用于给 CMA 分配指定大小的物理内存。
通过 CMDLINE 或内核配置方式传递给 CMA 参数，那么系统就调用该函数
进行 CMA 物理内存的分配。由于函数比较长，分段解析：

{% highlight c %}
int __init cma_declare_contiguous(phys_addr_t base,
                        phys_addr_t size, phys_addr_t limit,
                        phys_addr_t alignment, unsigned int order_per_bit,
                        bool fixed, const char *name, struct cma **res_cma)
{
        phys_addr_t memblock_end = memblock_end_of_DRAM();
        phys_addr_t highmem_start;
        int ret = 0;
{% endhighlight %}

参数 base 指定 CMA 的基地址，size 参数指定 CMA 的长度，limit
参数指定最大 CMA 分配地址，参数 alignment 代表 CMA 对齐方式，
参数 order_per_bit 指定 CMA 分配器使用的 bitmap 信息，fixed
代表 CMA 是否需要 fixed，参数 name 指定 CMA region 的名字，
参数 res_cma 指向 struct cma 结构。函数首先调用
memblock_end_of_DRAM() 函数获得 MEMBLOCK 物理内存分配器
支持的最大可用物理内存地址。

{% highlight c %}
        /*
         * We can't use __pa(high_memory) directly, since high_memory
         * isn't a valid direct map VA, and DEBUG_VIRTUAL will (validly)
         * complain. Find the boundary by adding one to the last valid
         * address.
         */
        highmem_start = __pa(high_memory - 1) + 1;
        pr_debug("%s(size %pa, base %pa, limit %pa alignment %pa)\n",
                __func__, &size, &base, &limit, &alignment);

        if (cma_area_count == ARRAY_SIZE(cma_areas)) {
                pr_err("Not enough slots for CMA reserved regions!\n");
                return -ENOSPC;
        }

        if (!size)
                return -EINVAL;        

        if (alignment && !is_power_of_2(alignment))
                return -EINVAL;
{% endhighlight %}

函数首先对参数进行检测，其中包括 cma_areas[] 数组是否已经
分配完了，如果 cma_area_count 等于 cma_areas[] 数组的长度，
那么代表系统 CMA regions 数组 cma_areas[] 已经分配完，没有
可用的 cma 结构，那么函数打印错误信息，并返回错误码。如果 size
参数无效，获得 alignment 无效，那么函数直接返回错误码。

{% highlight c %}
        /*
         * Sanitise input arguments.
         * Pages both ends in CMA area could be merged into adjacent unmovable
         * migratetype page by page allocator's buddy algorithm. In the case,
         * you couldn't get a contiguous memory, which is not what we want.
         */
        alignment = max(alignment,  (phys_addr_t)PAGE_SIZE <<
                          max_t(unsigned long, MAX_ORDER - 1, pageblock_order));
        base = ALIGN(base, alignment);
        size = ALIGN(size, alignment);
        limit &= ~(alignment - 1);

        if (!base)
                fixed = false;

        /* size should be aligned with order_per_bit */
        if (!IS_ALIGNED(size >> PAGE_SHIFT, 1 << order_per_bit))
                return -EINVAL;

        /*
         * If allocating at a fixed base the request region must not cross the
         * low/high memory boundary.
         */
        if (fixed && base < highmem_start && base + size > highmem_start) {
                ret = -EINVAL;
                pr_err("Region at %pa defined on low/high memory boundary (%pa)\n",
                        &base, &highmem_start);
                goto err;
        }
{% endhighlight %}

函数接下来调整 CMA region 的基地址和长度，以及最大分配地址，最后当
fixed 有效，且基地址小于 highmem_start，且 CMA region 的结束地址
大于 highmem_start，那么系统将会打印错误消息并返回错误码。

{% highlight c %}
        /*
         * If the limit is unspecified or above the memblock end, its effective
         * value will be the memblock end. Set it explicitly to simplify further
         * checks.
         */
        if (limit == 0 || limit > memblock_end)
                limit = memblock_end;

        /* Reserve memory */
        if (fixed) {
                if (memblock_is_region_reserved(base, size) ||
                    memblock_reserve(base, size) < 0) {
                        ret = -EBUSY;
                        goto err;
                }
{% endhighlight %}

函数检查到 limit 为 0 或则 limit 大于 memblock_end，那么
函数将 limit 设置为 memblock_end。如果 fixed 为 true，
那么函数此时调用 memblock_is_region_reserved() 检查申请的
CMA region 是否在 MEMBLOCK 的预留区内，如果在则返回错误代码，
如果不在，那么调用 memblock_reserve() 函数将申请的 CMA region
加入到 MEMBLOCK 的预留区内，如果添加失败，那么函数返回错误码。

{% highlight c %}
        } else {
                phys_addr_t addr = 0;

                /*
                 * All pages in the reserved area must come from the same zone.
                 * If the requested region crosses the low/high memory boundary,
                 * try allocating from high memory first and fall back to low
                 * memory in case of failure.
                 */
                if (base < highmem_start && limit > highmem_start) {
                        addr = memblock_alloc_range(size, alignment,
                                                    highmem_start, limit,
                                                    MEMBLOCK_NONE);
                        limit = highmem_start;
                }

                if (!addr) {
                        addr = memblock_alloc_range(size, alignment, base,
                                                    limit,
                                                    MEMBLOCK_NONE);
                        if (!addr) {
                                ret = -ENOMEM;
                                goto err;
                        }
                }

                /*
                 * kmemleak scans/reads tracked objects for pointers to other
                 * objects but this address isn't mapped and accessible
                 */
                kmemleak_ignore_phys(addr);
                base = addr;
        }
{% endhighlight %}

如果 fixed 为 false，那么函数判断如果 base 小于 highmen_start 且 limit
大小 highmem_start，那么函数就调用 memblock_alloc_range() 函数
分配指定大小的物理内存区块，并肩 limit 设置为 highmem_start。如果
分配失败，函数继续调用 memblock_alloc_rnage() 函数再次分配，如果
分配还是失败，那么函数直接返回错误码。

{% highlight c %}
        ret = cma_init_reserved_mem(base, size, order_per_bit, name, res_cma);
        if (ret)
                goto err;

        pr_info("Reserved %ld MiB at %pa\n", (unsigned long)size / SZ_1M,
                &base);
        return 0;

err:
        pr_err("Failed to reserve %ld MiB\n", (unsigned long)size / SZ_1M);
        return ret;
{% endhighlight %}

函数在分配物理内存之后，调用 cma_init_reserved_mem() 函数
添加进 CMA 分配器内，最后打印相应的信息。

> - [memblock_end_of_DRAM](#A0201)
>
> - [memblock_is_region_reserved](#A0210)
>
> - [memblock_reserve](#A0203)
>
> - [memblock_alloc_range](#A0248)
>
> - [cma_init_reserved_mem](#A0234)

------------------------------------

#### <span id="A0248">memblock_alloc_range</span>

{% highlight c %}
phys_addr_t __init memblock_alloc_range(phys_addr_t size, phys_addr_t align,
                                        phys_addr_t start, phys_addr_t end,
                                        enum memblock_flags flags)
{                       
        return memblock_alloc_range_nid(size, align, start, end, NUMA_NO_NODE,
                                        flags);
}
{% endhighlight %}

memblock_alloc_range() 函数用于从 MEMBLOCK 可用物理内存指定范围
内分配物理内存。参数 size 指向申请物理内存的长度，align 参数指向
对齐方式，start 参数指向分配范围的起始物理地址，参数 end 指向分配
范围的终止物理地址，flags 指向分配标志。函数通过调用
memblock_alloc_range_nid() 函数实现。

> - [memblock_alloc_range_nid](#A0227)

------------------------------------

#### <span id="A0249">dma_contiguous_reserve_area</span>

{% highlight c %}
/**
 * dma_contiguous_reserve_area() - reserve custom contiguous area
 * @size: Size of the reserved area (in bytes),
 * @base: Base address of the reserved area optional, use 0 for any
 * @limit: End address of the reserved memory (optional, 0 for any).
 * @res_cma: Pointer to store the created cma region.
 * @fixed: hint about where to place the reserved area
 *
 * This function reserves memory from early allocator. It should be
 * called by arch specific code once the early allocator (memblock or bootmem)
 * has been activated and all other subsystems have already allocated/reserved
 * memory. This function allows to create custom reserved areas for specific
 * devices.
 *
 * If @fixed is true, reserve contiguous area at exactly @base.  If false,
 * reserve in range from @base to @limit.
 */
int __init dma_contiguous_reserve_area(phys_addr_t size, phys_addr_t base,
                                       phys_addr_t limit, struct cma **res_cma,
                                       bool fixed)
{
        int ret;

        ret = cma_declare_contiguous(base, size, limit, 0, 0, fixed,
                                        "reserved", res_cma);
        if (ret)
                return ret;

        /* Architecture specific contiguous memory fixup. */
        dma_contiguous_early_fixup(cma_get_base(*res_cma),
                                cma_get_size(*res_cma));

        return 0;
}
{% endhighlight %}

dma_contiguous_reserve_area() 函数的作用就是申请一块新的物理内存
给 CMA region，并将 CMA region 加入到 CMA 分配器。最后调用将
CMA region 加入到 dma_mmu_remap[] 数组。参数 size 指定申请 CMA
region 的大小，base 指向申请的基地址，limit 参数指定最大申请
地址，参数 res_cma 用于存储 cma 结构。函数首先调用
cma_declare_contiguous() 函数申请一个新的 CMA region，并添加
到 CMA 内存分配器里，然后调用 dma_contiguous_early_fixup()
函数将新的 CMA region 加入到 dma_mmu_remap[] 数组。

> - [cma_declare_contiguous](#A0247)
>
> - [dma_contiguous_early_fixup](#A0235)

------------------------------------

#### <span id="A0250">dma_contiguous_reserve</span>

{% highlight c %}
/**
 * dma_contiguous_reserve() - reserve area(s) for contiguous memory handling
 * @limit: End address of the reserved memory (optional, 0 for any).
 *              
 * This function reserves memory from early allocator. It should be
 * called by arch specific code once the early allocator (memblock or bootmem)
 * has been activated and all other subsystems have already allocated/reserved
 * memory.
 */     
void __init dma_contiguous_reserve(phys_addr_t limit)
{                               
        phys_addr_t selected_size = 0;
        phys_addr_t selected_base = 0;
        phys_addr_t selected_limit = limit;
        bool fixed = false;

        pr_debug("%s(limit %08lx)\n", __func__, (unsigned long)limit);

        if (size_cmdline != -1) {
                selected_size = size_cmdline;
                selected_base = base_cmdline;
                selected_limit = min_not_zero(limit_cmdline, limit);
                if (base_cmdline + size_cmdline == limit_cmdline)
                        fixed = true;
        } else {
#ifdef CONFIG_CMA_SIZE_SEL_MBYTES
                selected_size = size_bytes;
#elif defined(CONFIG_CMA_SIZE_SEL_PERCENTAGE)
                selected_size = cma_early_percent_memory();
#elif defined(CONFIG_CMA_SIZE_SEL_MIN)
                selected_size = min(size_bytes, cma_early_percent_memory());
#elif defined(CONFIG_CMA_SIZE_SEL_MAX)
                selected_size = max(size_bytes, cma_early_percent_memory());
#endif  
        }

        if (selected_size && !dma_contiguous_default_area) {
                pr_debug("%s: reserving %ld MiB for global area\n", __func__,
                         (unsigned long)selected_size / SZ_1M);

                dma_contiguous_reserve_area(selected_size, selected_base,
                                            selected_limit,
                                            &dma_contiguous_default_area,
                                            fixed);
        }
}
{% endhighlight %}

dma_contiguous_reserve() 函数的作用是定义默认的 CMA region。
CMA 可以通过 DTS，CMDLINE 或则 Kbuild 进行定义，内核如果没有
使用 DTS 定义默认的 CMA region 的话，该函数就定义申请一块物理
内存用于 CMA，并定义这块 CMA 为默认的 CMA。参数 limit 指向
DMA 的限制地址。函数首先判断 size_cmdline 是否有效，size_cmdline
的作用是判断当前系统是否通过 CMDLINE 方式传递 cma 的信息，
如果在 CMDLINE 中已经传递了 cma 的信息，那么 size_cmdline
不为 -1. 如果系统使用 CMDLINE 传递 cma 参数，内核启动过程中，
解析 cmdline 中的 cma 会调用 early_cma() 函数，该函数用于解析
cmdline 中 cma 的参数，包括 cma 的大小，cma 可用的起始物理地址
和终止地址。CMDLINE 中定义了 cma 参数之后，会将长度存储在
selected_size 变量里，将基地址存储在 selected_base, 将 cma
最大物理地址存储在 selected_limit 里；如果 CMDLINE 中不存在
cma 参数，那么可以通过 Kbuild 内核配置实现，如果
CONFIG_CMA_SIZE_SEL_MBYTES 宏打开，那么 CMA 的长度由
size_bytes 指定；如果 CONFIG_CMA_SIZE_SEL_PERCENTAGE 宏
打开，那么 CMA 大小由可用物理内存的百分比决定，其通过函数
cma_early_percent_memory() 实现；如果宏 CONFIG_CMA_SIZE_SEL_MIN
定义，那么 CMA 的大小由 size_bytes 和 cma_early_percent_memory()
返回值中选择最小的值；如果宏 CONFIG_CMA_SIZE_SEL_MAX
定义，那么 CMA 的大小由 size_bytes 和 cma_early_percent_memory()
之中最大的值。

函数获得 selected_size 的值之后，如果此时 DTS 并未包含默认
CMA 配置，并且 selected_size 不为空，那么函数调用
dma_contiguous_reserve_area() 函数为 CMA region 分配内存，
并加入 CMA 分配器里；如果 DTS 已经有默认 CMA 的配置，那么
函数不做任何实质性的动作，直接使用 DTS 默认的 CMA。

> - [cma_early_percent_memory](#A0245)
>
> - [dma_contiguous_reserve_area](#A0249)
>
> - [early_cma](#A0251)

------------------------------------

#### <span id="A0251">early_cma</span>

{% highlight c %}
static int __init early_cma(char *p)
{
        if (!p) {
                pr_err("Config string not provided\n");
                return -EINVAL;
        }

        size_cmdline = memparse(p, &p);
        if (*p != '@')
                return 0;
        base_cmdline = memparse(p + 1, &p);
        if (*p != '-') {
                limit_cmdline = base_cmdline + size_cmdline;
                return 0;
        }
        limit_cmdline = memparse(p + 1, &p);

        return 0;
}
early_param("cma", early_cma);
{% endhighlight %}

early_cma() 函数用于从 CMDLINE 中解析 "cma=" 参数，
cma 参数支持的格式如下：

{% highlight c %}
cma=nn[MG]@[start[MG][-end[MG]]]

例如：

"cma=20M@0x68000000-0x70000000"
{% endhighlight %}

上面的例子就是 CMA 的大小为 20M, 这 20M 物理内存必须在
0x68000000-0x7000000 范围内查找。函数首先调用 memparse()
解析出 "cma" 字符串中长度域，存储在 size_cmdline，然后检查
字符串中是否含有 "@" 符号，如果有继续调用 memparse() 函数
解析基地址，并存储在 base_cmdline 里；如果字符串中还含有
"-" 字符，那么函数继续调用 memparse() 函数解析终止地址，
并存储在 limit_cmdline 里。early_cma() 函数通过
early_param() 函数将 "cma" 参数于 early_cma()
函数挂钩。

------------------------------------

#### <span id="A0252">arm_memblock_init</span>

{% highlight c %}
void __init arm_memblock_init(const struct machine_desc *mdesc)
{
        /* Register the kernel text, kernel data and initrd with memblock. */
        memblock_reserve(__pa(KERNEL_START), KERNEL_END - KERNEL_START);

        arm_initrd_init();

        arm_mm_memblock_reserve();

        /* reserve any platform specific memblock areas */
        if (mdesc->reserve)
                mdesc->reserve();

        early_init_fdt_reserve_self();
        early_init_fdt_scan_reserved_mem();

        /* reserve memory for DMA contiguous allocations */
        dma_contiguous_reserve(arm_dma_limit);

        arm_memblock_steal_permitted = false;
        memblock_dump_all();
}
{% endhighlight %}

arm_memblock_init() 函数用于将系统需要预留的内存加入到
MEMBLOCK 内存分配器的预留区里。参数 mdesc 指向体系相关的
结构。函数首先将内核镜像所在物理块加入到保留区里，然后将
INITRD 所占用的物理内存区加入到保留区内，arm_mm_memblock_reserve()
函数将内核全局页目录所在的物理地址加入都保留区内。接下来。
函数将 DTB memory reserved mapping 中指定的区域加入到
保留区，并将 DTS "reserved-memory" 节点的子节点对应的
区域加入到保留区。函数接着调用 dma_contiguous_reserve()
函数设置内核使用的默认 CMA。最后将 arm_memblock_steal_permitted
设置为 false，如果 MEMBLOCK 分配器 debug 功能启用，那么
函数 memblock_dump_all() 函数将会打印相应的信息。

> - [memblock_reserve](#A0203)
>
> - [arm_initrd_init](#A0211)
>
> - [arm_mm_memblock_reserve](#A0212)
>
> - [early_init_fdt_reserve_self](#A0214)
>
> - [early_init_fdt_scan_reserved_mem](#A0244)
>
> - [dma_contiguous_reserve](#A0250)

------------------------------------

#### <span id="A0253">pmd_clear</span>

{% highlight c %}
#define pmd_clear(pmdp)                 \
        do {                            \
                pmdp[0] = __pmd(0);     \
                pmdp[1] = __pmd(0);     \
                clean_pmd_entry(pmdp);  \
        } while (0)
{% endhighlight %}

pmd_clear() 函数用于清除一个 PMD 入口项。函数将 PMD 入口的
值都设置为 __pmd(0), 然后使用 clean_pmd_entry() 函数刷新
对应的 TLB 和 cache。

------------------------------------

#### <span id="A0254">__pmd</span>

{% highlight c %}
#define __pmd(x)        ((pmd_t) { (x) } )
{% endhighlight %}

__pmd() 函数用于制作 PMD 入口项的值。

------------------------------------

#### <span id="A0255">pmd_off_k</span>

{% highlight c %}
static inline pmd_t *pmd_off_k(unsigned long virt)
{
        return pmd_offset(pud_offset(pgd_offset_k(virt), virt), virt);
}
{% endhighlight %}

pmd_off_k() 函数用于获得内核虚拟地址对应的 PMD 入口。
函数首先通过 pgd_offset_k() 函数获得内核虚拟地址的全局页目录
入口地址，基于这个地址调用 pud_offset() 获得对应的 PUD 入口
地址，最后调用 pmd_offset() 函数获得对应的 PMD 入口地址。

> - [pgd_offset_k](#A0170)
>
> - [pud_offset](#A0171)
>
> - [pmd_offset](#A0172)

------------------------------------

#### <span id="A0256">prepare_page_table</span>

{% highlight c %}
static inline void prepare_page_table(void)
{
        unsigned long addr;
        phys_addr_t end;

        /*
         * Clear out all the mappings below the kernel image.
         */
        for (addr = 0; addr < MODULES_VADDR; addr += PMD_SIZE)
                pmd_clear(pmd_off_k(addr));

#ifdef CONFIG_XIP_KERNEL
        /* The XIP kernel is mapped in the module area -- skip over it */
        addr = ((unsigned long)_exiprom + PMD_SIZE - 1) & PMD_MASK;
#endif  
        for ( ; addr < PAGE_OFFSET; addr += PMD_SIZE)
                pmd_clear(pmd_off_k(addr));

        /*
         * Find the end of the first block of lowmem.
         */
        end = memblock.memory.regions[0].base + memblock.memory.regions[0].size;
        if (end >= arm_lowmem_limit)
                end = arm_lowmem_limit;

        /*
         * Clear out all the kernel space mappings, except for the first
         * memory bank, up to the vmalloc region.
         */
        for (addr = __phys_to_virt(end);
             addr < VMALLOC_START; addr += PMD_SIZE)
                pmd_clear(pmd_off_k(addr));
}
{% endhighlight %}

prepare_page_table() 函数的作用是在建立页表之前，清除未使用的
页表。函数分别找了三段虚拟地址，分别为： 1）0 地址到 MODULE_VADDR
2）MODULE_VADDR 到 PAGE_OFFSET 3）第一块 DRM 大于 arm_lowmem_limit
部分。函数对于这三段地址分贝使用 pmd_off_k() 算出对于的 PMD 入口之后，
使用 pmd_clear() 函数将 PMD 项清除，并刷新 TLB 和 Cache。

> - [pmd_off_k](#A0255)
>
> - [pmd_clear](#A0253)

------------------------------------

#### <span id="A0257">vectors_high</span>

{% highlight c %}
#if __LINUX_ARM_ARCH__ >= 4
#define vectors_high()  (get_cr() & CR_V)
#else
#define vectors_high()  (0)
#endif
{% endhighlight %}

vectors_high() 用于指明 Exception Vector 所在的位置，由 SCTLR
寄存器的 "bit-13 V" 决定，如果该 bit 为 0，那么 Exception Vector
位于 0x00000000；反之如果为 1，那么 Exception Vector 位于
0xFFFF0000.

------------------------------------

#### <span id="A0258">vectors_base</span>

{% highlight c %}
#define vectors_base()  (vectors_high() ? 0xffff0000 : 0)
{% endhighlight %}

vectors_base() 用于返回 Exception Vector 所在的虚拟地址，其
通过 vectors_high() 函数定义，最终由 SCTLR寄存器的 "bit-13 V"
决定，如果该 bit 为 0，那么 Exception Vector 位于 0x00000000；
反之如果为 1，那么 Exception Vector 位于 0xFFFF0000.

> - [vectors_high](#A0257)

------------------------------------

#### <span id="A0259">pgd_addr_end</span>

{% highlight c %}
#define pgd_addr_end(addr, end)                                         \
({      unsigned long __boundary = ((addr) + PGDIR_SIZE) & PGDIR_MASK;  \
        (__boundary - 1 < (end) - 1)? __boundary: (end);                \
})
{% endhighlight %}

pgd_addr_end() 函数用于获得 addr 下一个 PGDIR_SIZE 地址。
函数首先将 addr 加上 PGDIR_SIZE，然后判断该值是否比 end 参数
小，如果小，那么函数返回 addr 加上 PGDIR_SIZE 的值；反之返回
end 参数值。函数确保获得下一个 PGDIR_SIZE 地址是安全的。

------------------------------------

#### <span id="A0260">pud_addr_end</span>

{% highlight c %}
#define pud_addr_end(addr, end)                                         \
({      unsigned long __boundary = ((addr) + PUD_SIZE) & PUD_MASK;      \
        (__boundary - 1 < (end) - 1)? __boundary: (end);                \
})
{% endhighlight %}

pud_addr_end() 函数用于获得 addr 下一个 PUD_SIZE 地址。
函数首先将 addr 加上 PUD_SIZE，然后判断该值是否比 end 参数
小，如果小，那么函数返回 addr 加上 PUD_SIZE 的值；反之返回
end 参数值。函数确保获得下一个 PUD_SIZE 地址是安全的。

------------------------------------

#### <span id="A0261">pmd_addr_end</span>

{% highlight c %}
#define pmd_addr_end(addr, end)                                         \
({      unsigned long __boundary = ((addr) + PMD_SIZE) & PMD_MASK;      \
        (__boundary - 1 < (end) - 1)? __boundary: (end);                \
})
{% endhighlight %}

pmd_addr_end() 函数用于获得 addr 下一个 PMD_SIZE 地址。
函数首先将 addr 加上 PMD_SIZE，然后判断该值是否比 end 参数
小，如果小，那么函数返回 addr 加上 PMD_SIZE 的值；反之返回
end 参数值。函数确保获得下一个 PMD_SIZE 地址是安全的。

------------------------------------

#### <span id="A0262">__map_init_section</span>

{% highlight c %}
static void __init __map_init_section(pmd_t *pmd, unsigned long addr,
                        unsigned long end, phys_addr_t phys,
                        const struct mem_type *type, bool ng)
{
        pmd_t *p = pmd;

#ifndef CONFIG_ARM_LPAE
        /*
         * In classic MMU format, puds and pmds are folded in to
         * the pgds. pmd_offset gives the PGD entry. PGDs refer to a
         * group of L1 entries making up one logical pointer to
         * an L2 table (2MB), where as PMDs refer to the individual
         * L1 entries (1MB). Hence increment to get the correct
         * offset for odd 1MB sections.
         * (See arch/arm/include/asm/pgtable-2level.h)
         */
        if (addr & SECTION_SIZE)
                pmd++;
#endif
        do {
                *pmd = __pmd(phys | type->prot_sect | (ng ? PMD_SECT_nG : 0));
                phys += SECTION_SIZE;
        } while (pmd++, addr += SECTION_SIZE, addr != end);

        flush_pmd_entry(p);
}
{% endhighlight %}

__map_init_section() 函数用于建立一个 "Section" 一级页表。
在 ARMv7 中，页目录项支持多种方式，包括 "Page table",
"Section", 以及 "Supersection". ARMv7 默认使用了 "Section" 方式
作为页目录项，如下图：

![](/assets/PDB/BiscuitOS/boot/BOOT000231.png)

在 PGD 中，一个 PGD 入口地址指向了一个 2M 的地址空间，且一个
PGD 由两个 PMD 组成，每个 PMD 各指向 1M 的地址空间，在 ARM
中，PMD 入口与 PTE 页表的关系如图所示：

![](/assets/PDB/BiscuitOS/boot/BOOT000225.png)

PMD 入口所指的 PTE 页表大小为 4K，每个 PTE 入口占 4 个字节，
PTE 页表被分作两个部分，第一个部分供 Linux 使用，第二个部分
供硬件使用。因此一个 PMD 指向 1M 的地址空间，如果参数 addr
没有按 SECTION_SIZE 对齐，也就是 addr 指向了第二个 1M 地址，
因此需要使用第二个 pmd，第一个 PMD 则设置为空。函数调用 __pmd()
函数将起始物理地址 phys，prot_sect 以及 ng 的信息写到 PMD 项里，
这里 prot_sect 就是一级页表即页目录的值，即 "Section" 页目录
的值。设置完毕之后，将 phys 增加 SECTION_SIZE 的值，addr 的
值也增加 SECTION_SIZE，如果此时 addr 等于 end，那么函数停止
循环，并调用 flush_pmd_entry() 刷新 TLB 页表。

> - [\_\_pmd](#A0254)
>
> - [flush_pmd_entry](#A0174)

------------------------------------

#### <span id="A0263">arm_pte_alloc</span>

{% highlight c %}
static pte_t * __init arm_pte_alloc(pmd_t *pmd, unsigned long addr,
                                unsigned long prot,
                                void *(*alloc)(unsigned long sz))
{
        if (pmd_none(*pmd)) {
                pte_t *pte = alloc(PTE_HWTABLE_OFF + PTE_HWTABLE_SIZE);
                __pmd_populate(pmd, __pa(pte), prot);
        }
        BUG_ON(pmd_bad(*pmd));
        return pte_offset_kernel(pmd, addr);
}
{% endhighlight %}

arm_pte_alloc() 函数用于分配并安装一个新的 PTE 页表。参数 pmd 指向
PMD 入口，addr 指向虚拟地址，prot 指向页表属性，alloc 指向内存分配函数。
函数调用 pmd_none() 函数 PMD 入口是否为空，如果不为空，函数调用
alloc() 对应的函数进行内存分配，分配长度为 "PTE_HWTABLE_OFF+PTE_HWTABLE_SIZE"
的内存给 pte，然后调用 __pmd_populate() 函数将 PTE 填充到 PMD
入口项里。执行成功之后，函数调用 pmd_bad() 函数检测 pmd
入口项的有效性，如果无效则报错。函数最后调用
pte_offset_kernel() 函数返回 addr 虚拟地址对应的 pte 入口。

> - [pmd_none](#A0265)
>
> - [__pmd_populate](#A0175)
>
> - [pmd_bad](#A0)
>
> - [pte_offset_kernel](#A0)

------------------------------------

#### <span id="A0264">pmd_val</span>

{% highlight c %}
#define pmd_val(x)      ((x).pmd)
{% endhighlight %}

pmd_val() 函数的作用是获得 PMD 入口项的值。

------------------------------------

#### <span id="A0265">pmd_none</span>

{% highlight c %}
#define pmd_none(pmd)           (!pmd_val(pmd))
{% endhighlight %}

pmd_none() 函数的作用是判断 PMD 入口项是否为空。函数通过
pmd_val() 函数读取 PMD 入口项的值，如果值为 0，那么函数返回
true 表示 PMD 入口项是空的；反之返回 0，代表 PMD 入口
项目是非空的。

> - [pmd_val](#A0264)

------------------------------------

#### <span id="A0266">pmd_bad</span>

{% highlight c %}
#define pmd_bad(pmd)            (pmd_val(pmd) & 2)
{% endhighlight %}

pmd_bad() 函数判断 PMD 入口项是否有效，如果无效则返回 true，
反之返回 false。

> - [pmd_val](#A0264)

------------------------------------

#### <span id="A0267">arm_pte_alloc</span>

{% highlight c %}
static pte_t * __init arm_pte_alloc(pmd_t *pmd, unsigned long addr,
                                unsigned long prot,
                                void *(*alloc)(unsigned long sz))
{
        if (pmd_none(*pmd)) {
                pte_t *pte = alloc(PTE_HWTABLE_OFF + PTE_HWTABLE_SIZE);
                __pmd_populate(pmd, __pa(pte), prot);
        }
        BUG_ON(pmd_bad(*pmd));
        return pte_offset_kernel(pmd, addr);
}
{% endhighlight %}

arm_pte_alloc() 函数用于分配并安装一个 PTE 页表。参数 pmd 指向 PMD
入口，addr 指向虚拟地址，参数 prot 指向页表标志，参数 alloc 用于分配
内存的函数。函数首先调用 pmd_none() 判断 PMD 是否已经指向一个 PTE 页表，
如果没有指向，那么函数调用 alloc() 对应的函数分配一个 PTE 页表，PTE 页表
有两部分组成，两部分总共 1024 个 PTE 入口，如下图：

![](/assets/PDB/BiscuitOS/boot/BOOT000225.png)

如上图，函数将 PMD 的两个入口指向了 PTE 页表 PTE_HWTABLE_OFF 处，
每个 PMD 入口指向 1M 的地址空间，函数通过调用 __pmd_populate()
函数实现 PMD 入口与 PTE 页表的绑定。安装好页表之后，函数调用
pmd_bad() 函数将查 PMD 入口对应的入口项是否有效，如果无效，则
报错，最后函数调用 pte_offset_kernel() 函数返回虚拟地址 addr 对应的
PTE 入口。

> - [pte_offset_kernel](#A0270)

------------------------------------

#### <span id="A0268">pte_index</span>

{% highlight c %}
#define pte_index(addr)         (((addr) >> PAGE_SHIFT) & (PTRS_PER_PTE - 1))
{% endhighlight %}

pte_index() 函数的作用是获得虚拟地址对应的 PTE 入口。函数从
虚拟地址中截取 PTE 入口的偏移。如下图：

![](/assets/PDB/BiscuitOS/boot/BOOT000222.png)

函数先将虚拟地址向右移 PAGE_SHIFT 位，然后与 "PTRS_PER_PTE-1"
相与，以此获得 PTE 页表的偏移。

------------------------------------

#### <span id="A0269">pmd_page_vaddr</span>

{% highlight c %}
static inline pte_t *pmd_page_vaddr(pmd_t pmd)
{
        return __va(pmd_val(pmd) & PHYS_MASK & (s32)PAGE_MASK);
}
{% endhighlight %}

pmd_page_vaddr() 函数的作用是获得 PMD 入口对应的 PTE 页表
基地址，pmd 指向 PMD 入口。函数首先调用 pmd_val() 获得
PTE 页表基地址，此时是物理地址，然后进行掩码操作，将标志去掉，
获得 PTE 页表原始物理地址，最后通过 __va() 函数获得物理地址
对应的虚拟地址。

> - [pmd_val](#A0264)

------------------------------------

#### <span id="A0270">pte_offset_kernel</span>

{% highlight c %}
#define pte_offset_kernel(pmd,addr)     (pmd_page_vaddr(*(pmd)) + pte_index(addr))
{% endhighlight %}

pte_offset_kernel() 函数的作用就是获得虚拟地址对应的 PTE 入口。
参数 pmd 指向 PMD 入口，addr 参数指向虚拟地址。函数通过调用
pmd_page_vaddr() 函数获得 PMD 入口指向的 PTE 页表的基地址，此时
地址是虚拟地址，函数在通过 pte_index() 函数获得虚拟地址对应的
PTE 偏移，最后 PTE 页表基地址加上 PTE 偏移就获得了虚拟地址对应
的 PTE 入口。

> - [pmd_page_vaddr](#A0269)
>
> - [pte_index](#A0268)

------------------------------------

#### <span id="A0271">alloc_init_pte</span>

{% highlight c %}
static void __init alloc_init_pte(pmd_t *pmd, unsigned long addr,
                                  unsigned long end, unsigned long pfn,
                                  const struct mem_type *type,
                                  void *(*alloc)(unsigned long sz),
                                  bool ng)
{
        pte_t *pte = arm_pte_alloc(pmd, addr, type->prot_l1, alloc);
        do {
                set_pte_ext(pte, pfn_pte(pfn, __pgprot(type->prot_pte)),
                            ng ? PTE_EXT_NG : 0);
                pfn++;
        } while (pte++, addr += PAGE_SIZE, addr != end);
}
{% endhighlight %}

alloc_init_pte() 函数用于分配一个 PTE 页表，并设置 PTE 页表中指定
PTE 入口。参数 pmd 指向 PMD 入口，addr 参数指向虚拟地址，参数 end
指向终止地址，参数 pfn 指向物理页的页帧号，参数 type 指向内存分配
类型，alloc 指向内存分配函数，ng 参数。函数首先调用
arm_pte_alloc() 函数分配一个内存给 PTE，并将 PMD 指向 PTE，
然后使用 do while() 循环给指定范围的虚拟地址建立 PTE 入口
项的映射。函数通过 pfn_pte() 函数将一个页帧与 __pgprot() 生成
的 PTE 页表标志合成一个 PTE 入口项，然后通过 set_pte_ext()
函数将 set_pte_ext() 函数设置 pte 页表，最后将 pte, pfn 加一操作，
将 addr 加上 PAGE_SIZE，进入下一次循环，当 addr 等于 end 的
时候，循环结束。

> - [arm_pte_alloc](#A0267)
>
> - [pfn_pte](#A0276)
>
> - [__pgprot](#A0277)

------------------------------------

#### <span id="A0272">__pte</span>

{% highlight c %}
#define __pte(x)        ((pte_t) { (x) } )
{% endhighlight %}

__pte() 函数的作用是将参数 x 转换为 pte_t 类型。

------------------------------------

#### <span id="A0273">PFN_PHYS</span>

{% highlight c %}
#define PFN_PHYS(x)     ((phys_addr_t)(x) << PAGE_SHIFT)
{% endhighlight %}

PFN_PHYS() 函数的作用是将物理页帧转换为物理地址。参数 x
指向物理页帧号。函数将物理页帧左移 PAGE_SHIFT 位，以此
获得物理地址。

------------------------------------

#### <span id="A0274">__pfn_to_phys</span>

{% highlight c %}
#define __pfn_to_phys(pfn)      PFN_PHYS(pfn)
{% endhighlight %}

__pfn_to_phys() 函数用于获得物理页帧对应的物理地址。
函数通过 PFN_PHYS() 函数实现。

> - [PFN_PHYS](#A0273)

------------------------------------

#### <span id="A0275">pgprot_val</span>

{% highlight c %}
#define pgprot_val(x)   ((x).pgprot)
{% endhighlight %}

pgprot_val() 函数用于获得 PTE 入口标志。

------------------------------------

#### <span id="A0276">pfn_pte</span>

{% highlight c %}
#define pfn_pte(pfn,prot)       __pte(__pfn_to_phys(pfn) | pgprot_val(prot))
{% endhighlight %}

pfn_pte() 函数用于制作一个 PTE 入口项。pfn 指向物理页帧，prot 指向
PTE 页表项的标志。

> - [\_\_pte](#A0272)
>
> - [\_\_pfn_to_phys](#A0274)
>
> - [pgprot_val](#A0275)

------------------------------------

#### <span id="A0277">__pgprot</span>

{% highlight c %}
#define __pgprot(x)     ((pgprot_t) { (x) } )
{% endhighlight %}

__pgprot() 函数将参数 x 转换成 pgprot_t 类型。

------------------------------------

#### <span id="A0278">alloc_init_pmd</span>

{% highlight c %}
static void __init alloc_init_pmd(pud_t *pud, unsigned long addr,
                                      unsigned long end, phys_addr_t phys,
                                      const struct mem_type *type,
                                      void *(*alloc)(unsigned long sz), bool ng)
{
        pmd_t *pmd = pmd_offset(pud, addr);
        unsigned long next;

        do {
                /*
                 * With LPAE, we must loop over to map
                 * all the pmds for the given range.
                 */
                next = pmd_addr_end(addr, end);

                /*
                 * Try a section mapping - addr, next and phys must all be
                 * aligned to a section boundary.
                 */
                if (type->prot_sect &&
                                ((addr | next | phys) & ~SECTION_MASK) == 0) {
                        __map_init_section(pmd, addr, next, phys, type, ng);
                } else {
                        alloc_init_pte(pmd, addr, next,
                                       __phys_to_pfn(phys), type, alloc, ng);
                }

                phys += next - addr;

        } while (pmd++, addr = next, addr != end);
}
{% endhighlight %}

alloc_init_pmd() 函数用于分配或初始化 PMD 入口。参数 pud 指向
PUD 入口，参数 addr 指向虚拟地址，参数 end 指向结束虚拟地址，phys
参数指向物理地址，type 参数指向内存类型，参数 alloc 指向分配函数。
函数首先调用 pmd_offset() 函数获得 PMD 入口。然后调用 do while()
循环，函数在每次循环中，首先调用 pmd_addr_end() 函数获得 addr 对应
的下一个 PMD 入口。函数此时判断 type->prot_sect，如果该值有效，
代表仅仅设置 PMD 入口项, 并调用 __map_init_section() 函数初始化
PMD 入口项；反之调用 alloc_init_pte() 函数分配一个 PTE 页并
初始化 PMD 页表指向 PTE 页表。函数最后增加 phys 的值和 pmd 的
值，只要 addr 等于 end，那么函数结束循环。

> - [pmd_offset](#A0172)
>
> - [pmd_addr_end](#A0261)
>
> - [\_\_map_init_section](#A0262)
>
> - [alloc_init_pte](#A0271)
>
> - [\_\_phys_to_pfn](#A0274)

------------------------------------

#### <span id="A0279">alloc_init_pud</span>

{% highlight c %}
static void __init alloc_init_pud(pgd_t *pgd, unsigned long addr,
                                  unsigned long end, phys_addr_t phys,
                                  const struct mem_type *type,
                                  void *(*alloc)(unsigned long sz), bool ng)
{
        pud_t *pud = pud_offset(pgd, addr);
        unsigned long next;

        do {
                next = pud_addr_end(addr, end);
                alloc_init_pmd(pud, addr, next, phys, type, alloc, ng);
                phys += next - addr;
        } while (pud++, addr = next, addr != end);
}
{% endhighlight %}

alloc_init_pud() 函数用于分配或初始化 PUD 入口。参数 pgd 指向 PGD
入口，addr 参数指向起始虚拟地址，end 参数指向终止虚拟地址，参数
phys 指向起始物理地址，参数 type 指向内存类型，参数 alloc 指向分配
函数。函数首先调用过 pud_offset() 函数获得 PUD 入口，然后调用 do while()
循环，每次循环，函数首先调用 pud_addr_end() 获得下一个 PUD 对应的虚拟
地址，然后调用 alloc_init_pmd() 函数初始化 PUD 入口项，初始化
完毕，函数增加 phys 的值和 pud 的值，如果 addr 等于 end，那么
停止循环。

> - [pud_offset](#A0171)
>
> - [pud_addr_end](#A0260)
>
> - [alloc_init_pmd](#A0278)

------------------------------------

#### <span id="A0280">__create_mapping</span>

{% highlight c %}
static void __init __create_mapping(struct mm_struct *mm, struct map_desc *md,
                                    void *(*alloc)(unsigned long sz),
                                    bool ng)
{
        unsigned long addr, length, end;
        phys_addr_t phys;
        const struct mem_type *type;
        pgd_t *pgd;

        type = &mem_types[md->type];

#ifndef CONFIG_ARM_LPAE
        /*
         * Catch 36-bit addresses
         */
        if (md->pfn >= 0x100000) {
                create_36bit_mapping(mm, md, type, ng);
                return;
        }
#endif

        addr = md->virtual & PAGE_MASK;
        phys = __pfn_to_phys(md->pfn);
        length = PAGE_ALIGN(md->length + (md->virtual & ~PAGE_MASK));

        if (type->prot_l1 == 0 && ((addr | phys | length) & ~SECTION_MASK)) {
                pr_warn("BUG: map for 0x%08llx at 0x%08lx can not be mapped using pages, ignoring.\n",
                        (long long)__pfn_to_phys(md->pfn), addr);
                return;
        }

        pgd = pgd_offset(mm, addr);
        end = addr + length;
        do {
                unsigned long next = pgd_addr_end(addr, end);

                alloc_init_pud(pgd, addr, next, phys, type, alloc, ng);

                phys += next - addr;
                addr = next;
        } while (pgd++, addr != end);
}
{% endhighlight %}

__create_mapping() 函数的作用就是建立页表。参数 mm 指向进程的 mm_struct
结构，map_desc 参数指向映射关系，alloc 指向分配参数。由于代码较长，分段解析。

{% highlight c %}
        type = &mem_types[md->type];

#ifndef CONFIG_ARM_LPAE
        /*
         * Catch 36-bit addresses
         */
        if (md->pfn >= 0x100000) {
                create_36bit_mapping(mm, md, type, ng);
                return;
        }
#endif
{% endhighlight %}

函数首先通过 md->type 参数获得对应的内存类型，其存储在 mem_types[]
数组内。如果此时系统没有定义 CONFIG_ARM_LPAE 宏，且当前映射关系结构
md->pfn 大于 0x100000, 那么此时 32 位物理地址已经无法存储现有
的物理内存，函数调用 create_36bit_mapping() 建立 36 位页表，并
直接返回。

{% highlight c %}
        addr = md->virtual & PAGE_MASK;
        phys = __pfn_to_phys(md->pfn);
        length = PAGE_ALIGN(md->length + (md->virtual & ~PAGE_MASK));

        if (type->prot_l1 == 0 && ((addr | phys | length) & ~SECTION_MASK)) {
                pr_warn("BUG: map for 0x%08llx at 0x%08lx can not be mapped using pages, ignoring.\n",
                        (long long)__pfn_to_phys(md->pfn), addr);
                return;
        }
{% endhighlight %}

函数接着从映射关系结构中获得虚拟地址，并调用 __pfn_to_phys() 函数
获得物理地址，函数调用 PAGE_ALIGN() 函数获得映射关系对齐的终止虚拟地址，
如果 type->prot_l1 等于 0 且 addr,phys,length 没有按 SECTION_MASK
对齐，那么函数报错并返回。

{% highlight c %}
        pgd = pgd_offset(mm, addr);
        end = addr + length;
        do {
                unsigned long next = pgd_addr_end(addr, end);

                alloc_init_pud(pgd, addr, next, phys, type, alloc, ng);

                phys += next - addr;
                addr = next;
        } while (pgd++, addr != end);
{% endhighlight %}

函数继续调用 pgd_offset() 函数获得虚拟地址 addr 对应的 PGD 入口，
并计算终止虚拟地址，接着使用 do while() 循环，在循环中，函数调用
pgd_addr_end() 函数获得下一个 PGD 入口对应的虚拟地址。并调用
alloc_init_pud() 函数分配获得初始化一个 PUD 入口地址。建立完毕之后
增加 phys 和 addr，以及 pgd 的值，只有 addr 等于 end 的时候，
循环才停止。

> - [\_\_pfn_to_phys](#A0274)
>
> - [pgd_offset](#A0169)
>
> - [alloc_init_pud](#A0279)

------------------------------------

#### <span id="A0281">create_mapping</span>

{% highlight c %}
/*
 * Create the page directory entries and any necessary
 * page tables for the mapping specified by `md'.  We
 * are able to cope here with varying sizes and address
 * offsets, and we take full advantage of sections and
 * supersections.
 */
static void __init create_mapping(struct map_desc *md)
{
        if (md->virtual != vectors_base() && md->virtual < TASK_SIZE) {
                pr_warn("BUG: not creating mapping for 0x%08llx at 0x%08lx in user region\n",
                        (long long)__pfn_to_phys((u64)md->pfn), md->virtual);
                return;
        }

        if ((md->type == MT_DEVICE || md->type == MT_ROM) &&
            md->virtual >= PAGE_OFFSET && md->virtual < FIXADDR_START &&
            (md->virtual < VMALLOC_START || md->virtual >= VMALLOC_END)) {
                pr_warn("BUG: mapping for 0x%08llx at 0x%08lx out of vmalloc space\n",
                        (long long)__pfn_to_phys((u64)md->pfn), md->virtual);
        }

        __create_mapping(&init_mm, md, early_alloc, false);
}
{% endhighlight %}

create_mapping() 函数用于建立一段页表。参数 md 指向映射关系结构。
函数首先判断当虚拟地址小于 TASK_SIZE，即用户空间堆栈的栈底，且
虚拟地址不等于 Exception Verctor Table，此时系统报错并直接返回。
如果此时映射关系对应的是 Device 或 ROM，且虚拟地址大于内核空间的起始
虚拟地址，但小于 FIXADDR_START，也小于 VMALLOC_START，也或大于
VMALLOC_END, 那么函数提示存错，即函数只映射低端内核虚拟地址。
检测通过后，函数调用 __create_mapping() 函数建立页表。

> - [vectors_base](#A0258)
>
> - [\_\_pfn_to_phys](#A0274)
>
> - [\_\_create_mapping](#A0280)
>
> - [early_alloc](#A0286)

------------------------------------

#### <span id="A0282">TASK_SIZE</span>

{% highlight c %}
/*      
 * TASK_SIZE - the maximum size of a user space task.
 * TASK_UNMAPPED_BASE - the lower boundary of the mmap VM area
 */                     
#define TASK_SIZE               (UL(CONFIG_PAGE_OFFSET) - UL(SZ_16M))
{% endhighlight %}

TASK_SIZE 宏指向了用户空间堆栈的最大长度。CONFIG_PAGE_OFFSET
指向内核虚拟空间的起始地址。

> - [PAGE_OFFSET](#A0199)

------------------------------------

#### <span id="A0283">memblock_phys_alloc</span>

{% highlight c %}
phys_addr_t __init memblock_phys_alloc(phys_addr_t size, phys_addr_t align)
{
        return memblock_alloc_base(size, align, MEMBLOCK_ALLOC_ACCESSIBLE);
}
{% endhighlight %}

memblock_phys_alloc() 函数用于获得指定长度的物理内存。参数 size
指向需求物理内存的长度，align 参数指向对齐方式。函数通过调用
memblock_alloc_base() 函数获得需求的物理内存。

> - [memblock_alloc_base](#A0284)

------------------------------------

#### <span id="A0284">memblock_alloc_base</span>

{% highlight c %}
phys_addr_t __init memblock_alloc_base(phys_addr_t size, phys_addr_t align, phys_addr_t max_addr)
{               
        phys_addr_t alloc;

        alloc = __memblock_alloc_base(size, align, max_addr);

        if (alloc == 0)
                panic("ERROR: Failed to allocate %pa bytes below %pa.\n",
                      &size, &max_addr);

        return alloc;
}
{% endhighlight %}

memblock_alloc_base() 函数用于获得指定长度的物理内存。参数 size 代表
需求物理内存的长度，参数 align 代表对齐方式，参数 max_addr 代表最大
可分配物理地址。函数首先调用 __memblock_alloc_base() 函数获得指定
长度的物理内存，然后判断分配是否成功，如果失败，则调用 panic() 函数
报错；反之返回 alloc 地址。

> - [\_\_memblock_alloc_base](#A0229)

------------------------------------

#### <span id="A0285">early_alloc_aligned</span>

{% highlight c %}
static void __init *early_alloc_aligned(unsigned long sz, unsigned long align)
{           
        void *ptr = __va(memblock_phys_alloc(sz, align));
        memset(ptr, 0, sz);
        return ptr;
}
{% endhighlight %}

early_alloc_aligned() 函数用于早期分配指定长度的物理内存。参数 sz 指向
需求物理内存的长度，align 参数指向对齐方式。函数首先通过
memblock_phys_alloc() 函数分配指定长度的物理内存，然后通过 __va()
函数获得物理内存对应的虚拟地址，然后使用 memset() 函数初始化内存，最后
返回内存的指针。

> - [memblock_phys_alloc](#A0283)

------------------------------------

#### <span id="A0286">early_alloc</span>

{% highlight c %}
static void __init *early_alloc(unsigned long sz)
{
        return early_alloc_aligned(sz, sz);
}
{% endhighlight %}

early_alloc() 函数用早期分配物理指定长度物理内存。参数 sz 指向
需求物理内存的长度。函数通过调用 early_alloc_aligned() 函数
分配指定长度的物理内存。

> - [early_alloc_aligned](#A0285)

------------------------------------

#### <span id="A0287">map_lowmem</span>

{% highlight c %}
static void __init map_lowmem(void)
{
        struct memblock_region *reg;
        phys_addr_t kernel_x_start = round_down(__pa(KERNEL_START), SECTION_SIZE);
        phys_addr_t kernel_x_end = round_up(__pa(__init_end), SECTION_SIZE);

        /* Map all the lowmem memory banks. */
        for_each_memblock(memory, reg) {
                phys_addr_t start = reg->base;
                phys_addr_t end = start + reg->size;
                struct map_desc map;

                if (memblock_is_nomap(reg))
                        continue;

                if (end > arm_lowmem_limit)
                        end = arm_lowmem_limit;
                if (start >= end)
                        break;

                if (end < kernel_x_start) {
                        map.pfn = __phys_to_pfn(start);
                        map.virtual = __phys_to_virt(start);
                        map.length = end - start;
                        map.type = MT_MEMORY_RWX;

                        create_mapping(&map);
                } else if (start >= kernel_x_end) {
                        map.pfn = __phys_to_pfn(start);
                        map.virtual = __phys_to_virt(start);
                        map.length = end - start;
                        map.type = MT_MEMORY_RW;

                        create_mapping(&map);
                } else {
                        /* This better cover the entire kernel */
                        if (start < kernel_x_start) {
                                map.pfn = __phys_to_pfn(start);
                                map.virtual = __phys_to_virt(start);
                                map.length = kernel_x_start - start;
                                map.type = MT_MEMORY_RW;

                                create_mapping(&map);
                        }

                        map.pfn = __phys_to_pfn(kernel_x_start);
                        map.virtual = __phys_to_virt(kernel_x_start);
                        map.length = kernel_x_end - kernel_x_start;
                        map.type = MT_MEMORY_RWX;

                        create_mapping(&map);

                        if (kernel_x_end < end) {
                                map.pfn = __phys_to_pfn(kernel_x_end);
                                map.virtual = __phys_to_virt(kernel_x_end);
                                map.length = end - kernel_x_end;
                                map.type = MT_MEMORY_RW;

                                create_mapping(&map);
                        }
                }
        }
}
{% endhighlight %}

map_lowmem() 用于映射低端物理内存。低端物理内存的起始地址为 DRM，
低端物理内存的结束地址是 arm_lowmem_limit。代码较长，分段解析：

{% highlight c %}
        struct memblock_region *reg;
        phys_addr_t kernel_x_start = round_down(__pa(KERNEL_START), SECTION_SIZE);
        phys_addr_t kernel_x_end = round_up(__pa(__init_end), SECTION_SIZE);
{% endhighlight %}

函数首先调用 round_doun() 函数和 round_up() 函数分别计算
内核镜像的起始物理地址和内核镜像的终止物理地址。

{% highlight c %}
        for_each_memblock(memory, reg) {
                phys_addr_t start = reg->base;
                phys_addr_t end = start + reg->size;
                struct map_desc map;

                if (memblock_is_nomap(reg))
                        continue;

                if (end > arm_lowmem_limit)
                        end = arm_lowmem_limit;
                if (start >= end)
                        break;
{% endhighlight %}

for_each_memblock() 用于遍历系统中所有可用物理内存，每次遍历一块
可用的物理内存，函数首先通过函数 memblock_is_nomap() 判断这段物理
内存块是否建立页表，如果不建立，那么函数直接结束这次循环。如果当前
物理内存的终止地址已经大于低端物理内存，那么函数将 end 设置为
arm_lowmem_limit, 以此只映射低端物理内存，如果遍历到的物理内存区块
的起始地址已经大于低端物理内存区块，那么直接跳出循环。

{% highlight c %}
                if (end < kernel_x_start) {
                        map.pfn = __phys_to_pfn(start);
                        map.virtual = __phys_to_virt(start);
                        map.length = end - start;
                        map.type = MT_MEMORY_RWX;

                        create_mapping(&map);
                } else if (start >= kernel_x_end) {
                        map.pfn = __phys_to_pfn(start);
                        map.virtual = __phys_to_virt(start);
                        map.length = end - start;
                        map.type = MT_MEMORY_RW;

                        create_mapping(&map);
                }
{% endhighlight %}

如果物理内存小于内核镜像的起始地址，那么这段物理内存建立一个映射关系，
起始页帧设置为物理内存区块的起始地址对应的页帧，再使用 __phys_to_virt()
函数获得起始物理地址对应的虚拟地址，物理区块的长度存储在 length 成员里，
映射的类型设置为 MT_MEMORY_RWX, 即可读可写可执行；如果物理内存区块
的起始地址大于内核镜像的起始物理地址，那么特意将内存类型设置为 MT_MEMORY_RW,
即只能读写不能执行。最后都调用 create_mapping() 函数建立页表。

{% highlight c %}
                } else {
                        /* This better cover the entire kernel */
                        if (start < kernel_x_start) {
                                map.pfn = __phys_to_pfn(start);
                                map.virtual = __phys_to_virt(start);
                                map.length = kernel_x_start - start;
                                map.type = MT_MEMORY_RW;

                                create_mapping(&map);
                        }

                        map.pfn = __phys_to_pfn(kernel_x_start);
                        map.virtual = __phys_to_virt(kernel_x_start);
                        map.length = kernel_x_end - kernel_x_start;
                        map.type = MT_MEMORY_RWX;

                        create_mapping(&map);

                        if (kernel_x_end < end) {
                                map.pfn = __phys_to_pfn(kernel_x_end);
                                map.virtual = __phys_to_virt(kernel_x_end);
                                map.length = end - kernel_x_end;
                                map.type = MT_MEMORY_RW;

                                create_mapping(&map);
                        }
                }
{% endhighlight %}

如果遍历到的物理内存块将内核镜像包含在其中，那么物理地址就包含了三个部分，
第一个部分为小于内核镜像的物理内核，第二部分为内核镜像占用的物理内存，第三
部分为大于内核镜像但小于低端物理内存结束地址的物理内存。其中特殊之处是
第一和第三部分的内存类型都设置为 MT_MEMORY_RW, 而第二部分则设置为
MT_MEMORY_RWX. 最终调用 create_mapping() 函数进行页表创建。

> - [create_mapping](#A0281)

------------------------------------

#### <span id="A0288">__pv_stub</span>

{% highlight c %}
#define __pv_stub(from,to,instr,type)                   \
        __asm__("@ __pv_stub\n"                         \
        "1:     " instr "       %0, %1, %2\n"           \
        "       .pushsection .pv_table,\"a\"\n"         \
        "       .long   1b\n"                           \
        "       .popsection\n"                          \
        : "=r" (to)                                     \
        : "r" (from), "I" (type))
{% endhighlight %}


首先来看下面三行代码的含义：

{% highlight c %}
               .pushsection .pv_table,\"a\"\n"         
               .long   1b\n"                           
               .popsection\n"                          
{% endhighlight %}

在一个文件中出现这三行代码，那么在这个编译生成的 .o 文件中，
会插入一个名为 ".pv_table" 的 section，段的内容是一个 long
型的地址，其指向标号 1b 处的地址，对于标号 1，1f 表示
forword 1 表示在本句之后的标号 1，1b 表示 backword 1 就是
本句之前的标号 1. 指令 ".pushsection" 的作用就是向当前目标
文件的 .text section 中插入一个名为指定名字的 section。
".popsection" 指令表示 section 结束之后仍然归于 .text section。
内核在编译过程中，根据链接脚本: arch/arm/kernel/vmlinux.lds.S

{% highlight c %}
        .init.pv_table : {
                __pv_table_begin = .;
                *(.pv_table)
                __pv_table_end = .;
        }                        
{% endhighlight %}

编译系统会将所有目标文件的 .pv_table section 加入到 vmlinux
的 .init.pv_table section 内，该 section 的起始虚拟地址是
__pv_table_begin, 终止地址是 __pv_table_end. 再回到 __pv_stub
宏，这个 section 中每一个 long 型都是一个地址，位于这些地址处的
内容就是各个标号 1 处的指令，于是在内核启动时就能够通过 __pv_table_begin
找到所有这些标号 1 处的指令，然后就可以修改这些指令的操作数。
例如在 ARMv7 内核启动过程中，在汇编阶段就通过 __fixup_pv_table
对 .init.pv_table section 进行修改，具体可以参考：

> - [__fixup_pv_table](/blog/ARM-SCD-kernel-head.S/#__fixup_pv_table)

这里还涉及一个比较有意思的地方，1 标志处的代码为：

{% highlight c %}
        "1:     " instr "       %0, %1, %2\n"                       
{% endhighlight %}

通过内核源码统计之后可知，instr 可以是 ADD 指令，也可以是 SUB
指令，两条指令机器码格式如下：

![](/assets/PDB/BiscuitOS/boot/BOOT000232.png)

![](/assets/PDB/BiscuitOS/boot/BOOT000233.png)

两个指令的机器码格式中，shifter_operand 域都是一致，其定义如下：

![](/assets/PDB/BiscuitOS/boot/BOOT000234.png)

上面为 ARMv7 中立即数的表示方式，其包含了 rotate_imm 域和 immed_8 域，
ARMv7 通过将 immed_8 域的值向右循环移动 (2 * rotate_imm 域值)，即：

{% highlight c %}
立即数 = immed_8 >> (2 * rotate_imm)  "循环右移"                      
{% endhighlight %}

因此 __pv_stub 宏将指令添加到 .pv_table section 之后，可以通过
__pv_table_begin 遍历里面的所有指令，并可以修改每条 ADD 或 SUB
指令的指定域，其中包括立即数域。

------------------------------------

#### <span id="A0289">__phys_to_virt</span>

{% highlight c %}
static inline unsigned long __phys_to_virt(phys_addr_t x)
{
        unsigned long t;

        /*
         * 'unsigned long' cast discard upper word when
         * phys_addr_t is 64 bit, and makes sure that inline
         * assembler expression receives 32 bit argument
         * in place where 'r' 32 bit operand is expected.
         */
        __pv_stub((unsigned long) x, t, "sub", __PV_BITS_31_24);
        return t;
}                      
{% endhighlight %}

__phys_to_virt() 函数的作用是通过物理地址获得对应的虚拟地址。
在内核空间，通过物理地址获得对应的虚拟地址，可以通过一个线性
运算获得，即加上或减去指定的偏移。内核就通过 __pv_stub() 函数
实现物理地址到虚拟地址的转换，其通过调用 sub 指令，将物理地址
减去一个偏移就可以得到虚拟地址，这里内核增加了一个技巧，内核
将这条减法指令添加到了 vmlinux 内核镜像的 .init.pv_table section
内，并在内核启动的过程中，根据实际运行情况，校正了偏移值，
这样内核就可以获得正确的偏移，而不是使用 __PV_BITS_31_24 使用的
偏移值。

> - [\_\_pv_stub](#A0288)

------------------------------------

#### <span id="A0290">__va</span>

{% highlight c %}
#define __va(x)                 ((void *)__phys_to_virt((phys_addr_t)(x)))
{% endhighlight %}

__va() 函数用于将一个物理地址转换成虚拟地址。函数通过调用
__phys_to_virt() 函数实现。

> - [\_\_phys_to_virt](#A0289)

------------------------------------

#### <span id="A0291">vm_area_add_early</span>

{% highlight c %}
static struct vm_struct *vmlist __initdata;
/**
 * vm_area_add_early - add vmap area early during boot
 * @vm: vm_struct to add
 *
 * This function is used to add fixed kernel vm area to vmlist before
 * vmalloc_init() is called.  @vm->addr, @vm->size, and @vm->flags
 * should contain proper values and the other fields should be zero.
 *
 * DO NOT USE THIS FUNCTION UNLESS YOU KNOW WHAT YOU'RE DOING.
 */
void __init vm_area_add_early(struct vm_struct *vm)
{
        struct vm_struct *tmp, **p;

        BUG_ON(vmap_initialized);
        for (p = &vmlist; (tmp = *p) != NULL; p = &tmp->next) {
                if (tmp->addr >= vm->addr) {
                        BUG_ON(tmp->addr < vm->addr + vm->size);
                        break;
                } else
                        BUG_ON(tmp->addr + tmp->size > vm->addr);
        }
        vm->next = *p;
        *p = vm;
}
{% endhighlight %}

vm_area_add_early() 函数的作用是将一个 vm_struct 加入到内核
早期的 vmlist 中。函数首先判断 vmap_initialized 是否为 true，
如果为，那么函数直接报错；反之，函数使用 for 循环，从 vmlist 的
第一个 vm_struct 开始遍历所有的成员，当找到 vmlist 中的成员 addr
大于参数 vm 的 addr，那么函数将 vm 插入到 vmlist 的当前位置。

------------------------------------

#### <span id="A0292">add_static_vm_early</span>

{% highlight c %}
void __init add_static_vm_early(struct static_vm *svm)
{
        struct static_vm *curr_svm;
        struct vm_struct *vm;
        void *vaddr;

        vm = &svm->vm;
        vm_area_add_early(vm);
        vaddr = vm->addr;

        list_for_each_entry(curr_svm, &static_vmlist, list) {
                vm = &curr_svm->vm;

                if (vm->addr > vaddr)
                        break;
        }
        list_add_tail(&svm->list, &curr_svm->list);
}
{% endhighlight %}

add_static_vm_early() 函数用于内核启动早期，将一个 static_vm 加入
到内核维护的 vmlist 和 static_vmlist 里。函数首先获得参数 svm 对应
的 vm，并通过函数 vm_area_add_early() 函数将该 vm 加入到系统早期的
vmlist 里。函数接着使用 list_for_each_entry() 函数遍历系统的
static_vmlist 链表，直到在链表中找到一个成员的地址大于参数 svm 对应
的地址，然后函数调用 list_add_tail() 函数添加到链表的当前位置。

> - [vm_area_add_early](#A0291)
>
> - [list_for_each_entry](/blog/LIST_list_for_each_entry/#%E6%BA%90%E7%A0%81%E5%88%86%E6%9E%90)
>
> - [list_add_tail](/blog/LIST_list_add_tail/#%E6%BA%90%E7%A0%81%E5%88%86%E6%9E%90)

------------------------------------

#### <span id="A0293">iotable_init</span>

{% highlight c %}
/*
 * Create the architecture specific mappings
 */
void __init iotable_init(struct map_desc *io_desc, int nr)
{        
        struct map_desc *md;
        struct vm_struct *vm;
        struct static_vm *svm;

        if (!nr)
                return;

        svm = early_alloc_aligned(sizeof(*svm) * nr, __alignof__(*svm));

        for (md = io_desc; nr; md++, nr--) {
                create_mapping(md);

                vm = &svm->vm;
                vm->addr = (void *)(md->virtual & PAGE_MASK);
                vm->size = PAGE_ALIGN(md->length + (md->virtual & ~PAGE_MASK));
                vm->phys_addr = __pfn_to_phys(md->pfn);
                vm->flags = VM_IOREMAP | VM_ARM_STATIC_MAPPING;
                vm->flags |= VM_ARM_MTYPE(md->type);
                vm->caller = iotable_init;
                add_static_vm_early(svm++);
        }
}
{% endhighlight %}

iotable_init() 函数的作用是建立体系 IO 映射表。函数首先调用
early_alloc_aligned() 函数给 static_vm 分配了内存，然后使用
for 循环，给参数 io_desc 映射结构建立指定的页表，每次遍历过程
中，函数通过调用 create_mapping() 执行真正的页表建立，建立完毕
之后，更新 svm 的 vm 结构，其中包含起始虚拟地址，长度，映射的物理
地址，虚拟地址标志设置，VM_IOREMAP 表示指明了该映射是 IO，以及
VM_ARM_STATIC_MAPPING 指令了该映射是静态映射。函数最后调用
add_static_vm_early() 函数将静态映射结构 svm 加入到系统早期
的 vmlist 和 static_vmlist 数据结构里。循环知道 nr 为零停止。

> - [early_alloc_aligned](#A0285)
>
> - [\_\_pfn_to_phys](#A0274)
>
> - [add_static_vm_early](#A0292)

------------------------------------

#### <span id="A0294">dma_contiguous_remap</span>

{% highlight c %}
void __init dma_contiguous_remap(void)
{
        int i;
        for (i = 0; i < dma_mmu_remap_num; i++) {
                phys_addr_t start = dma_mmu_remap[i].base;
                phys_addr_t end = start + dma_mmu_remap[i].size;
                struct map_desc map;
                unsigned long addr;

                if (end > arm_lowmem_limit)
                        end = arm_lowmem_limit;
                if (start >= end)
                        continue;

                map.pfn = __phys_to_pfn(start);
                map.virtual = __phys_to_virt(start);
                map.length = end - start;
                map.type = MT_MEMORY_DMA_READY;

                /*
                 * Clear previous low-memory mapping to ensure that the
                 * TLB does not see any conflicting entries, then flush
                 * the TLB of the old entries before creating new mappings.
                 *
                 * This ensures that any speculatively loaded TLB entries
                 * (even though they may be rare) can not cause any problems,
                 * and ensures that this code is architecturally compliant.
                 */
                for (addr = __phys_to_virt(start); addr < __phys_to_virt(end);
                     addr += PMD_SIZE)
                        pmd_clear(pmd_off_k(addr));

                flush_tlb_kernel_range(__phys_to_virt(start),
                                       __phys_to_virt(end));

                iotable_init(&map, 1);
        }
}
{% endhighlight %}

dma_contiguous_remap() 函数的作用就是个连续物理内存 CMA
重新建立页表。函数使用 for 循环，遍历 dma_mmu_remap[] 中
的所有成员，每个成员代表一块物理地址分配成功的 CMA，每遍历
一块 CMA，函数首先获得块 CMA 的起始物理地址和长度，以此计算
该块 CMA 的终止物理地址。函数接下来检查终止物理地址是否已经
超出低端物理地址的范围，如果查出，则将终止物理地址 end 设置
为 arm_lowmem_limit. 如果此时 CMA 物理块的起始地址已经大于
终止地址，那么函数停止本次循环，继续遍历其他 CMA 物理块。
如果遍历到的 CMA 块有效，那么函数设置映射结构体 map，设置
起始物理页帧，设置 CMA 物理块的起始虚拟地址，设置长度，设置
映射类型为 MT_MEMORY_DMA_READ。设置完映射关系之后，函数将
CMA 物理块原先占用的页表都清除调用，并调用 flush_tlb_kernel_range()
函数清除对应虚拟地址的 TLB。最后调用 iotable_init() 函数
建立指定的 CMA 页表。

> - [\_\_phys_to_pfn](#A0296)
>
> - [\_\_phys_to_virt](#A0289)
>
> - [pmd_clear](#A0253)
>
> - [iotable_init](#A0293)

------------------------------------

#### <span id="A0295">PHYS_PFN</span>

{% highlight c %}
#define PHYS_PFN(x)     ((unsigned long)((x) >> PAGE_SHIFT))
{% endhighlight %}

PHYS_PFN() 宏用于将物理地址转换成物理页帧号。函数通过将
物理地址向右移动 PAGE_SHIFT 位实现。

------------------------------------

#### <span id="A0296">__phys_to_pfn</span>

{% highlight c %}
#define __phys_to_pfn(paddr)    PHYS_PFN(paddr)
{% endhighlight %}

__phys_to_pfn() 函数用于将物理地址转换成物理页帧。函数通过
PHYS_PFN() 函数实现。

> - [PHYS_PFN](#A0295)

------------------------------------

#### <span id="A0297">__fix_to_virt</span>

{% highlight c %}
#define __fix_to_virt(x)        (FIXADDR_TOP - ((x) << PAGE_SHIFT))
{% endhighlight %}

__fix_to_virt() 用于 FIXMAP 索引获得对应的虚拟地址。首先获得 x 对应
也偏移，然后将 FIXADDR_TOP 减去也偏移就是 x 对应的虚拟地址。

------------------------------------

#### <span id="A0298">fix_to_virt</span>

{% highlight c %}
/*      
 * 'index to address' translation. If anyone tries to use the idx
 * directly without translation, we catch the bug with a NULL-deference
 * kernel oops. Illegal ranges of incoming indices are caught too.
 */     
static __always_inline unsigned long fix_to_virt(const unsigned int idx)
{
        BUILD_BUG_ON(idx >= __end_of_fixed_addresses);
        return __fix_to_virt(idx);
}
{% endhighlight %}

fix_to_virt() 函数通过 FIXMAP 索引获得对应的虚拟地址。函数
首先判断参数 idx 索引是否大于等于 __end_of_fixed_addresses,
如果为真，那么函数将会报错；反之函数调用 __fix_to_virt()
函数获得 FIXMAP 索引对应的虚拟地址。

> - [\_\_fix_to_virt](#A0297)

------------------------------------

#### <span id="A0299">pte_val</span>

{% highlight c %}
#define pte_val(x)      ((x).pte)
{% endhighlight %}

pte_val() 函数用于获得 PTE 入口对应的值。

------------------------------------

#### <span id="A0300">pte_none</span>

{% highlight c %}
#define pte_none(pte)           (!pte_val(pte))
{% endhighlight %}

pte_none() 函数用于判读 PTE 入口指向的内容为空。函数通过
pte_val() 获得函数的入口项值，然后判断该值是否为空，如果为
空则返回 ture；反之返回 false。

> - [pte_val](#A0299)

------------------------------------

#### <span id="A0301">pte_pfn</span>

{% highlight c %}
#define pte_pfn(pte)            ((pte_val(pte) & PHYS_MASK) >> PAGE_SHIFT)
{% endhighlight %}

pte_pfn() 函数用于获得 PTE 入口对应的物理页帧号。函数通过
pte_val() 函数获得 PTE 入口对应的值，然后使用掩码获得对应
的物理页帧号。

> - [pte_val](#A0299)

------------------------------------

#### <span id="A0302">early_fixmap_shutdown</span>

{% highlight c %}
static void __init early_fixmap_shutdown(void)
{
        int i;
        unsigned long va = fix_to_virt(__end_of_permanent_fixed_addresses - 1);

        pte_offset_fixmap = pte_offset_late_fixmap;
        pmd_clear(fixmap_pmd(va));
        local_flush_tlb_kernel_page(va);

        for (i = 0; i < __end_of_permanent_fixed_addresses; i++) {
                pte_t *pte;
                struct map_desc map;

                map.virtual = fix_to_virt(i);
                pte = pte_offset_early_fixmap(pmd_off_k(map.virtual), map.virtual);

                /* Only i/o device mappings are supported ATM */
                if (pte_none(*pte) ||
                    (pte_val(*pte) & L_PTE_MT_MASK) != L_PTE_MT_DEV_SHARED)
                        continue;

                map.pfn = pte_pfn(*pte);
                map.type = MT_DEVICE;
                map.length = PAGE_SIZE;

                create_mapping(&map);
        }
}
{% endhighlight %}

early_fixmap_shutdown() 函数的作用是为 FIXMAP 区域建立页表。
函数首先调用 fix_to_virt() 获得 __end_of_permanent_fixed_addresses
对应的虚拟地址，然后使用 pmd_clear() 函数清除虚拟地址对应的 PMD
入口。接着调用 local_flush_tlb_kernel_page() 刷新 TLB。函数接着
使用 for 循环将遍历所欲的 __end_of_permanent_fixed_addresses，
每编译一个 FIXMAP 项，函数通过 fix_to_virt() 函数获得 FIXMAP
区域内的虚拟地址，然后通过调用 pte_offset_early_fixmap() 获得
虚拟地址对应的 PTE 入口，如果该 PTE 为空，或者 PTE 表项不包含
L_PTE_MT_DEV_SHARD, 那么函数不会为其建立页表，结束本次循环；反之
如果 PTE 页表检测通过，那么函数设置映射结构体，将 pfn 指向 pte
页表指向的页，长度设置为 PAGE_SIZE，最后调用 create_mapping()
函数建立页表。

> - [fix_to_virt](#A0298)
>
> - [pmd_clear](#A0253)
>
> - [fixmap_pmd](#A0173)
>
> - [pte_offset_early_fixmap](#A0176)
>
> - [pmd_off_k](#A0255)
>
> - [pte_none](#A0300)
>
> - [pte_val](#A0299)
>
> - [pte_pfn](#A0301)
>
> - [create_mapping](#A0281)

------------------------------------

#### <span id="A0303">kuser_init</span>

{% highlight c %}
static void __init kuser_init(void *vectors)
{
        extern char __kuser_helper_start[], __kuser_helper_end[];
        int kuser_sz = __kuser_helper_end - __kuser_helper_start;

        memcpy(vectors + 0x1000 - kuser_sz, __kuser_helper_start, kuser_sz);

        /*
         * vectors + 0xfe0 = __kuser_get_tls
         * vectors + 0xfe8 = hardware TLS instruction at 0xffff0fe8
         */
        if (tls_emu || has_tls_reg)
                memcpy(vectors + 0xfe0, vectors + 0xfe8, 4);
}
{% endhighlight %}

kuser_init() 函数用于将 __kuser_helper_start 到 __kuser_helper_end 之间
的内容拷贝到异常向量表的指定位置，该区域包含了多个有空的函数。
函数首先计算 __kuser_helper_start 到 __kuser_helper_end 的大小，然后
使用 memcpy() 函数将该区域拷贝到 vector + 0x1000 处。接下来的
函数是体系特殊处理，需要具体分析。

------------------------------------

#### <span id="A0304">early_trap_init</span>

{% highlight c %}
void __init early_trap_init(void *vectors_base)
{
#ifndef CONFIG_CPU_V7M
        unsigned long vectors = (unsigned long)vectors_base;
        extern char __stubs_start[], __stubs_end[];
        extern char __vectors_start[], __vectors_end[];
        unsigned i;

        vectors_page = vectors_base;

        /*
         * Poison the vectors page with an undefined instruction.  This
         * instruction is chosen to be undefined for both ARM and Thumb
         * ISAs.  The Thumb version is an undefined instruction with a
         * branch back to the undefined instruction.
         */
        for (i = 0; i < PAGE_SIZE / sizeof(u32); i++)
                ((u32 *)vectors_base)[i] = 0xe7fddef1;

        /*
         * Copy the vectors, stubs and kuser helpers (in entry-armv.S)
         * into the vector page, mapped at 0xffff0000, and ensure these
         * are visible to the instruction stream.
         */
        memcpy((void *)vectors, __vectors_start, __vectors_end - __vectors_start);      
        memcpy((void *)vectors + 0x1000, __stubs_start, __stubs_end - __stubs_start);

        kuser_init(vectors_base);

        flush_icache_range(vectors, vectors + PAGE_SIZE * 2);
#else /* ifndef CONFIG_CPU_V7M */
        /*
         * on V7-M there is no need to copy the vector table to a dedicated
         * memory area. The address is configurable and so a table in the kernel
         * image can be used.
         */
#endif
}
{% endhighlight %}

early_trap_init() 函数用于早期异常向量表的建立。参数 vectors_base
指向新异常向量表所在的虚拟地址。函数首先将 vectors_base 赋值给
vectors_page, 然后使用 for 循环将异常向量表中的所有项设置
为 0xe7fddef1，即未定义指令。函数接着调用 memcpy() 函数将
__vectors_start 到 __vectors_end 区域拷贝到 vectors 处，
同样也调用 memcpy() 函数将 __stubs_start 到 __stubs_end
区域拷贝到 vectors + 0x1000 处，接着调用 kuser_init() 函数
安装 kuser 相关的函数到 vector 指定的位置。最后调用
flush_icache_range() 函数刷新指定的 cache。

> - [kuser_init](#A0303)

------------------------------------

#### <span id="A0305">pmd_empty_section_gap</span>

{% highlight c %}
static void __init pmd_empty_section_gap(unsigned long addr)
{
        vm_reserve_area_early(addr, SECTION_SIZE, pmd_empty_section_gap);
}

{% endhighlight %}

pmd_empty_section_gap() 函数的作用是将 addr 对应的虚拟空间
加入到系统静态映射中预留。函数通过调用 vm_reserve_area_early()
函数实现。

> - [vm_reserve_area_early](#A0306)

------------------------------------

#### <span id="A0306">vm_reserve_area_early</span>

{% highlight c %}
void __init vm_reserve_area_early(unsigned long addr, unsigned long size,
                                  void *caller)
{
        struct vm_struct *vm;
        struct static_vm *svm;

        svm = early_alloc_aligned(sizeof(*svm), __alignof__(*svm));

        vm = &svm->vm;
        vm->addr = (void *)addr;
        vm->size = size;
        vm->flags = VM_IOREMAP | VM_ARM_EMPTY_MAPPING;
        vm->caller = caller;
        add_static_vm_early(svm);
}
{% endhighlight %}

vm_reserve_area_early() 函数的作用是给指定虚拟地址建立静态预留映射，
但不对应实际的物理地址，只做预留作用。参数 addr 指向预留虚拟地址，
参数 size 指向预留的长度，caller 指代静态预留的调用函数。
函数首先调用 early_alloc_aligned() 函数分配指定大小的内存，
然后设置静态映射对应的结构，最后调用 add_static_vm_early()
函数将该静态映射添加到系统的静态映射链表里。

> - [early_alloc_aligned](#A0285)
>
> - [add_static_vm_early](#A0292)

------------------------------------

#### <span id="A0307">fill_pmd_gaps</span>

{% highlight c %}
static void __init fill_pmd_gaps(void)
{
        struct static_vm *svm;
        struct vm_struct *vm;
        unsigned long addr, next = 0;
        pmd_t *pmd;

        list_for_each_entry(svm, &static_vmlist, list) {
                vm = &svm->vm;
                addr = (unsigned long)vm->addr;
                if (addr < next)
                        continue;

                /*
                 * Check if this vm starts on an odd section boundary.
                 * If so and the first section entry for this PMD is free
                 * then we block the corresponding virtual address.
                 */
                if ((addr & ~PMD_MASK) == SECTION_SIZE) {
                        pmd = pmd_off_k(addr);
                        if (pmd_none(*pmd))
                                pmd_empty_section_gap(addr & PMD_MASK);
                }

                /*
                 * Then check if this vm ends on an odd section boundary.
                 * If so and the second section entry for this PMD is empty
                 * then we block the corresponding virtual address.
                 */
                addr += vm->size;
                if ((addr & ~PMD_MASK) == SECTION_SIZE) {
                        pmd = pmd_off_k(addr) + 1;
                        if (pmd_none(*pmd))
                                pmd_empty_section_gap(addr);
                }

                /* no need to look at any vm entry until we hit the next PMD */
                next = (addr + PMD_SIZE - 1) & PMD_MASK;
        }
}
{% endhighlight %}

fill_pmd_gaps() 函数的作用是检查静态映射区内的虚拟地址，是否存在 PMD
入口值占用了一个，并且是基数 PMD 入口。在 linux 中，PMD 入口包含了两个
连续的 SECTION 入口用于指向 2M 地址空间。然而调用 create_mapping() 函数
可以通过使用单独 1M 地址空间来优化静态映射。如果不使用顶部或底部的 SECTION
入口，实际上 PMD 入口会减半。如果 ioremap() 获得 vmalloc() 函数视图使用
这些未使用的 PMD 入口，那么会引发问题。该问题可以通过插入虚假的 vm 入口
来覆盖未使用的 PMD 入口。函数首先使用 list_for_each_entry() 函数遍历
系统所有静态映射，如果遍历到的静态映射的起始虚拟地址小于 next，那么函数
继续遍历下一个静态映射；反之如果函数检测到当前虚拟地址使用了奇数的 PMD
入口，并调用函数 pmd_none() 检测到虚拟对应的 pmd 是空的，那么函数调用
pmd_empty_section_gap() 函数填充该 PMD 入口，同理检查静态映射的终止
地址对应的 PMD 入口是否也存在其中一个 PMD 入口也是空的情况。最后函数
将 next 指向该静态映射的结尾。

> - [list_for_each_entry](/blog/LIST_list_for_each_entry/#%E6%BA%90%E7%A0%81%E5%88%86%E6%9E%90)
>
> - [pmd_off_k](#A0255)
>
> - [pmd_none](#A0265)
>
> - [pmd_empty_section_gap](#A0305)

------------------------------------

#### <span id="A0308">devicemaps_init</span>

{% highlight c %}
/*
 * Set up the device mappings.  Since we clear out the page tables for all
 * mappings above VMALLOC_START, except early fixmap, we might remove debug
 * device mappings.  This means earlycon can be used to debug this function
 * Any other function or debugging method which may touch any device _will_
 * crash the kernel.
 */
static void __init devicemaps_init(const struct machine_desc *mdesc)
{
        struct map_desc map;
        unsigned long addr;
        void *vectors;

        /*
         * Allocate the vector page early.
         */
        vectors = early_alloc(PAGE_SIZE * 2);

        early_trap_init(vectors);

        /*
         * Clear page table except top pmd used by early fixmaps
         */
        for (addr = VMALLOC_START; addr < (FIXADDR_TOP & PMD_MASK); addr += PMD_SIZE)
                pmd_clear(pmd_off_k(addr));

        /*
         * Map the kernel if it is XIP.
         * It is always first in the modulearea.
         */
#ifdef CONFIG_XIP_KERNEL
        map.pfn = __phys_to_pfn(CONFIG_XIP_PHYS_ADDR & SECTION_MASK);
        map.virtual = MODULES_VADDR;
        map.length = ((unsigned long)_exiprom - map.virtual + ~SECTION_MASK) & SECTION_MASK;
        map.type = MT_ROM;
        create_mapping(&map);
#endif

        /*
         * Map the cache flushing regions.
         */
#ifdef FLUSH_BASE
        map.pfn = __phys_to_pfn(FLUSH_BASE_PHYS);
        map.virtual = FLUSH_BASE;
        map.length = SZ_1M;
        map.type = MT_CACHECLEAN;
        create_mapping(&map);
#endif
#ifdef FLUSH_BASE_MINICACHE
        map.pfn = __phys_to_pfn(FLUSH_BASE_PHYS + SZ_1M);
        map.virtual = FLUSH_BASE_MINICACHE;
        map.length = SZ_1M;
        map.type = MT_MINICLEAN;
        create_mapping(&map);
#endif

        /*
         * Create a mapping for the machine vectors at the high-vectors
         * location (0xffff0000).  If we aren't using high-vectors, also
         * create a mapping at the low-vectors virtual address.
         */
        map.pfn = __phys_to_pfn(virt_to_phys(vectors));
        map.virtual = 0xffff0000;
        map.length = PAGE_SIZE;
#ifdef CONFIG_KUSER_HELPERS
        map.type = MT_HIGH_VECTORS;
#else
        map.type = MT_LOW_VECTORS;
#endif
        create_mapping(&map);

        if (!vectors_high()) {
                map.virtual = 0;
                map.length = PAGE_SIZE * 2;
                map.type = MT_LOW_VECTORS;
                create_mapping(&map);
        }

        /* Now create a kernel read-only mapping */
        map.pfn += 1;
        map.virtual = 0xffff0000 + PAGE_SIZE;
        map.length = PAGE_SIZE;
        map.type = MT_LOW_VECTORS;
        create_mapping(&map);


        /*
         * Ask the machine support to map in the statically mapped devices.
         */
        if (mdesc->map_io)
                mdesc->map_io();
        else
                debug_ll_io_init();
        fill_pmd_gaps();

        /* Reserve fixed i/o space in VMALLOC region */
        pci_reserve_io();

        /*
         * Finally flush the caches and tlb to ensure that we're in a
         * consistent state wrt the writebuffer.  This also ensures that
         * any write-allocated cache lines in the vector page are written
         * back.  After this point, we can start to touch devices again.
         */
        local_flush_tlb_all();
        flush_cache_all();

        /* Enable asynchronous aborts */
        early_abt_enable();
}
{% endhighlight %}

devicemaps_init() 函数用于 IO 设备建立映射。由于函数太长，分段解析：

{% highlight c %}
static void __init devicemaps_init(const struct machine_desc *mdesc)
{
        struct map_desc map;
        unsigned long addr;
        void *vectors;

        /*
         * Allocate the vector page early.
         */
        vectors = early_alloc(PAGE_SIZE * 2);

        early_trap_init(vectors);

        /*
         * Clear page table except top pmd used by early fixmaps
         */
        for (addr = VMALLOC_START; addr < (FIXADDR_TOP & PMD_MASK); addr += PMD_SIZE)
                pmd_clear(pmd_off_k(addr));
{% endhighlight %}

函数首先调用 early_alloc() 函数为新的异常向量表分配内存空间，然后
调用 early_trap_init() 将系统的异常向量表和特定的函数拷贝到新
的异常向量表所在的空间。函数接着使用 for 循环，接着 pmd_clear() 函数，
将 VMALLOC_START 到 FIXMAP 之间的虚拟地址的 PMD 入口都清零了。

{% highlight c %}
#ifdef CONFIG_XIP_KERNEL
        map.pfn = __phys_to_pfn(CONFIG_XIP_PHYS_ADDR & SECTION_MASK);
        map.virtual = MODULES_VADDR;
        map.length = ((unsigned long)_exiprom - map.virtual + ~SECTION_MASK) & SECTION_MASK;
        map.type = MT_ROM;
        create_mapping(&map);
#endif

        /*
         * Map the cache flushing regions.
         */
#ifdef FLUSH_BASE
        map.pfn = __phys_to_pfn(FLUSH_BASE_PHYS);
        map.virtual = FLUSH_BASE;
        map.length = SZ_1M;
        map.type = MT_CACHECLEAN;
        create_mapping(&map);
#endif
#ifdef FLUSH_BASE_MINICACHE
        map.pfn = __phys_to_pfn(FLUSH_BASE_PHYS + SZ_1M);
        map.virtual = FLUSH_BASE_MINICACHE;
        map.length = SZ_1M;
        map.type = MT_MINICLEAN;
        create_mapping(&map);
#endif
{% endhighlight %}

如果系统定义了 CONFIG_XIP_KERNEL 宏，FLUSH_BASE 宏，或则
FLUSH_BASE_MINICACHE 宏，那么系统会为这几个 IO 或设备建立
页表。

{% highlight c %}
        /*
         * Create a mapping for the machine vectors at the high-vectors
         * location (0xffff0000).  If we aren't using high-vectors, also
         * create a mapping at the low-vectors virtual address.
         */
        map.pfn = __phys_to_pfn(virt_to_phys(vectors));
        map.virtual = 0xffff0000;
        map.length = PAGE_SIZE;
#ifdef CONFIG_KUSER_HELPERS
        map.type = MT_HIGH_VECTORS;
#else
        map.type = MT_LOW_VECTORS;
#endif
        create_mapping(&map);

        if (!vectors_high()) {
                map.virtual = 0;
                map.length = PAGE_SIZE * 2;
                map.type = MT_LOW_VECTORS;
                create_mapping(&map);
        }
{% endhighlight %}

函数接下来为新的异常向量表建立页表，新页表的虚拟地址
设置为 0xffff0000, 如果此时定义了 CONFIG_KUSER_HELPER
宏，那么将异常向量表的映射类型设置为 MT_HIGH_VECTOR,
反之设置为 MT_LOW_VECTORS, 最后调用 create_mapping()
函数建立页表。如果函数通过调用 vectors_high() 函数发现
异常向量表不在高地址，那么函数为低端的异常向量表建立
映射。

{% highlight c %}
        /* Now create a kernel read-only mapping */
        map.pfn += 1;
        map.virtual = 0xffff0000 + PAGE_SIZE;
        map.length = PAGE_SIZE;
        map.type = MT_LOW_VECTORS;
        create_mapping(&map);
{% endhighlight %}

接下来函数建立了一个只读的页表，虚拟地址位于 0xffff0000+PAGE_SZIE
处，该映射的物理地址紧跟异常向量的物理地址。映射的类型是
MT_LOW_VECTORS.

{% highlight c %}
        /*
         * Ask the machine support to map in the statically mapped devices.
         */
        if (mdesc->map_io)
                mdesc->map_io();
        else
                debug_ll_io_init();
        fill_pmd_gaps();

        /* Reserve fixed i/o space in VMALLOC region */
        pci_reserve_io();
{% endhighlight %}

如果此时包含了体系相关的 IO 映射，则调用体系对应的 map_io()
函数进行映射。函数通过调用 fill_pmd_gaps() 函数将 IO 或 设备
建立的映射中 PMD 只使用了其中一个入口，将另外一个空的 PMD
入口填充。函数最后调用 pci_reserve_io() 为 PCI 设备建立映射。

{% highlight c %}
        /*
         * Finally flush the caches and tlb to ensure that we're in a
         * consistent state wrt the writebuffer.  This also ensures that
         * any write-allocated cache lines in the vector page are written
         * back.  After this point, we can start to touch devices again.
         */
        local_flush_tlb_all();
        flush_cache_all();

        /* Enable asynchronous aborts */
        early_abt_enable();
{% endhighlight %}

函数最后调用 local_flush_tabl_all() 函数和 flush_cache_all()
函数刷新 TLB 和 cache，最后函数调用 early_abt_enable()
函数使能同步 ABORT 异常处理。

> - [early_alloc](#A0286)
>
> - [early_trap_init](#A0304)
>
> - [pmd_off_k](#A0255)
>
> - [pmd_clear](#A0253)
>
> - [\_\_phys_to_pfn](#A0296)
>
> - [create_mapping](#A0281)
>
> - [fill_reserve_io](#A0307)

------------------------------------

#### <span id="A0309">early_pte_alloc</span>

{% highlight c %}
static pte_t * __init early_pte_alloc(pmd_t *pmd, unsigned long addr,
                                      unsigned long prot)
{
        return arm_pte_alloc(pmd, addr, prot, early_alloc);
}
{% endhighlight %}

early_pte_alloc() 函数的作用是内核启动早期，分配并初始化一个 PTE 页表。
pmd 参数指向 PMD 入口，addr 参数指向虚拟地址，prot 参数指向页表标志。
函数通过 arm_pte_alloc() 函数分配了一个 PTE 页表，并将 pmd 入口指向
该 PTE 页表。最后返回参数 addr 虚拟地址对应的 PTE 入口。

> - [arm_pte_alloc](#A0263)

------------------------------------

#### <span id="A0310">kmap_init</span>

{% highlight c %}
static void __init kmap_init(void)
{
#ifdef CONFIG_HIGHMEM
        pkmap_page_table = early_pte_alloc(pmd_off_k(PKMAP_BASE),
                PKMAP_BASE, _PAGE_KERNEL_TABLE);
#endif

        early_pte_alloc(pmd_off_k(FIXADDR_START), FIXADDR_START,
                        _PAGE_KERNEL_TABLE);
}
{% endhighlight %}

kmap_init() 函数用于建立 PMD 对应的 PTE 页表。如果内核启用了
CONFIG_HIGHMEM 宏，即高端物理内存，那么系统调用 early_pte_alloc()
函数建立一个页表给 pkmap_page_table, 其起始虚拟地址是 PKMAP_BASE,
函数同样调用 early_pte_alloc() 函数，为 FIXADDR_START 建立
PTE 页表。

> - [early_pte_alloc](#A0309)
>
> - [pmd_off_k](#A0255)

------------------------------------

#### <span id="A0311">memblock_allow_resize</span>

{% highlight c %}
void __init memblock_allow_resize(void)
{       
        memblock_can_resize = 1;
}
{% endhighlight %}

memblock_allow_resize() 函数用于设置 MEMBLOCK 内存分配器的
memblock_can_resize 变量，设置其为 1.

------------------------------------

#### <span id="A0312">PFN_UP</span>

{% highlight c %}
#define PFN_UP(x)       (((x) + PAGE_SIZE-1) >> PAGE_SHIFT)
{% endhighlight %}

PFN_UP() 函数用于将参数 x 对应的下一个页帧号。

------------------------------------

#### <span id="A0313">PFN_DOWN</span>

{% highlight c %}
#define PFN_DOWN(x)     ((x) >> PAGE_SHIFT)
{% endhighlight %}

PFN_DOWN() 函数用于获得参数 x 对应的页帧号。

------------------------------------

#### <span id="A0314">memblock_get_current_limit</span>

{% highlight c %}
phys_addr_t __init_memblock memblock_get_current_limit(void)
{       
        return memblock.current_limit;
}
{% endhighlight %}

memblock_get_current_limit() 函数用于获得 MEMBLOCK 内存分配器
当前最大可分配物理地址。MEMBLOCK 内存分配器中，最大可分配物理地址
存储在 memblock.current_limit 成员里。

------------------------------------

#### <span id="A0315">memblock_start_of_DRAM</span>

{% highlight c %}
/* lowest address */
phys_addr_t __init_memblock memblock_start_of_DRAM(void)
{       
        return memblock.memory.regions[0].base;
}
{% endhighlight %}

memblock_start_of_DRAM() 函数用于获得 MEMBLOCK 内存分配器
里，第一块 DRAM 的起始物理地址。在 MEMBLOCK 内存分配器中，可用
物理内存存储在 memblock.memory 里，其按起始地址从低到高排列，
因此 DRAM 的起始地址就是第一个 region 的起始物理地址。

------------------------------------

#### <span id="A0316">find_limits</span>

{% highlight c %}
static void __init find_limits(unsigned long *min, unsigned long *max_low,
                               unsigned long *max_high)
{
        *max_low = PFN_DOWN(memblock_get_current_limit());
        *min = PFN_UP(memblock_start_of_DRAM());
        *max_high = PFN_DOWN(memblock_end_of_DRAM());
}
{% endhighlight %}

find_limits() 函数的作用是获得物理地址低端物理内存的起始页帧
和终止页帧，以及 MEMBLOCK 内存分配器支持的最大页帧。
memblock_get_current_limit() 函数获得 MEMBLOCK 内存分配器支持
的最大物理地址，然后通过 PFN_DOWN() 函数获得对应的页帧。
memblock_start_of_DRAM() 函数获得第一块 DRM 对应的物理地址，
然后使用 PFN_UP() 函数获得其对应的页帧，memblock_end_of_DRAM()
函数获得 MEMBLOCK 内存分配器可用物理内存的终止物理地址，再通过
PFN_DOWN() 函数获得对应的页帧号。

> - [memblock_get_current_limit](#A0314)
>
> - [memblock_start_of_DRAM](#A0315)
>
> - [memblock_end_of_DRAM](#A0201)
>
> - [PFN_DOWN](#A0313)
>
> - [PFN_UP](#A0312)

------------------------------------

#### <span id="A0317">memblock_region_memory_base_pfn</span>

{% highlight c %}
/**     
 * memblock_region_memory_base_pfn - get the lowest pfn of the memory region
 * @reg: memblock_region structure
 *
 * Return: the lowest pfn intersecting with the memory region
 */
static inline unsigned long memblock_region_memory_base_pfn(const struct memblock_region *reg)
{
        return PFN_UP(reg->base);
}
{% endhighlight %}

memblock_region_memory_base_pfn() 函数的作用是获得 memblock_region
起始物理地址对应的页帧。参数 reg 对应指定的 memblock_region。函数
调用 PFN_UP() 函数获得起始页帧号。

> - [PFN_UP](#A0312)

------------------------------------

#### <span id="A0318">memblock_region_memory_end_pfn</span>

{% highlight c %}
/**
 * memblock_region_memory_end_pfn - get the end pfn of the memory region
 * @reg: memblock_region structure
 *
 * Return: the end_pfn of the reserved region
 */     
static inline unsigned long memblock_region_memory_end_pfn(const struct memblock_region *reg)   
{
        return PFN_DOWN(reg->base + reg->size);
}
{% endhighlight %}

memblock_region_memory_end_pfn() 函数用于获得 memblock_region
终止物理地址对应的页帧号。参数 reg 对应指定的 memblock_region。函数
调用 PFN_DOWN() 函数获得终止页帧号。

> - [PFN_DOWN](#A0313)

------------------------------------

#### <span id="A0319">arm_adjust_dma_zone</span>

{% highlight c %}
static void __init arm_adjust_dma_zone(unsigned long *size, unsigned long *hole,
        unsigned long dma_size)
{
        if (size[0] <= dma_size)
                return;

        size[ZONE_NORMAL] = size[0] - dma_size;
        size[ZONE_DMA] = dma_size;
        hole[ZONE_NORMAL] = hole[0];
        hole[ZONE_DMA] = 0;
}
{% endhighlight %}

arm_adjust_dma_zone() 函数的作用是调整 DMA_ZONE 和 NORMAL_ZONE
统计数组。参数 size 指向可用物理页帧数组，hole 参数指向碎片物理页帧
数组，dma_size 参数指向 DMA_ZONE 占用的物理页帧。函数首先检查
NORMAL_ZONE 包含的可用物理页帧数是否比 dna_size 小，如果小，那么
函数直接返回；反之函数跳转 size[] 数组和 hole[] 数组的数据，使
DMA_ZONE 和 NORMAL_ZONE 有正确的统计数据。

------------------------------------

#### <span id="A0320">zone_spanned_pages_in_node</span>

{% highlight c %}
static inline unsigned long __init zone_spanned_pages_in_node(int nid,
                                        unsigned long zone_type,
                                        unsigned long node_start_pfn,
                                        unsigned long node_end_pfn,
                                        unsigned long *zone_start_pfn,
                                        unsigned long *zone_end_pfn,
                                        unsigned long *zones_size)
{
        unsigned int zone;

        *zone_start_pfn = node_start_pfn;
        for (zone = 0; zone < zone_type; zone++)
                *zone_start_pfn += zones_size[zone];

        *zone_end_pfn = *zone_start_pfn + zones_size[zone_type];

        return zones_size[zone_type];
}

{% endhighlight %}

zone_spanned_pages_in_node() 用于计算每个 zone 的起始页帧和结束页帧。
参数 nid 指向 NUMA 信息，参数 zone_type 指向 zones_size[] 数组的偏移，
node_start_pfn 参数指向 ZONE 的起始页帧，node_end_pfn 参数指向 ZONE
的结束页帧，参数 zone_start_pfn 用于存储 ZONE 的起始页帧，zone_end_pfn
用于存储 ZONE 的结束页帧，zones_size[] 数组用于存储 ZONE 信息。函数
通过遍历 zones_size[] 中的信息获得该 ZONE 的起始页帧和结束页帧，并返回
该 zone 的长度。

------------------------------------

#### <span id="A0321">zone_absent_pages_in_node</span>

{% highlight c %}
static inline unsigned long __init zone_absent_pages_in_node(int nid,
                                                unsigned long zone_type,
                                                unsigned long node_start_pfn,
                                                unsigned long node_end_pfn,
                                                unsigned long *zholes_size)
{
        if (!zholes_size)
                return 0;

        return zholes_size[zone_type];
}
{% endhighlight %}

zone_absent_pages_in_node() 函数用于计算两个 ZONE 之间的
hole 占用的页帧数。hole 的信息都存储在 zholes_size[] 数组
里，函数首先判断 zholes_size[] 数组的有效性，如果无效，直接
返回；如果有效，那么函数返回 zholes_size[] 对应的页帧数。

------------------------------------

#### <span id="A0322">calculatode_totalpages</span>

{% highlight c %}
static void __init calculate_node_totalpages(struct pglist_data *pgdat,
                                                unsigned long node_start_pfn,
                                                unsigned long node_end_pfn,
                                                unsigned long *zones_size,
                                                unsigned long *zholes_size)
{
        unsigned long realtotalpages = 0, totalpages = 0;
        enum zone_type i;

        for (i = 0; i < MAX_NR_ZONES; i++) {
                struct zone *zone = pgdat->node_zones + i;
                unsigned long zone_start_pfn, zone_end_pfn;
                unsigned long size, real_size;

                size = zone_spanned_pages_in_node(pgdat->node_id, i,
                                                  node_start_pfn,
                                                  node_end_pfn,
                                                  &zone_start_pfn,
                                                  &zone_end_pfn,
                                                  zones_size);
                real_size = size - zone_absent_pages_in_node(pgdat->node_id, i,
                                                  node_start_pfn, node_end_pfn,
                                                  zholes_size);
                if (size)
                        zone->zone_start_pfn = zone_start_pfn;
                else
                        zone->zone_start_pfn = 0;
                zone->spanned_pages = size;
                zone->present_pages = real_size;

                totalpages += size;
                realtotalpages += real_size;
        }

        pgdat->node_spanned_pages = totalpages;
        pgdat->node_present_pages = realtotalpages;
        printk(KERN_DEBUG "On node %d totalpages: %lu\n", pgdat->node_id,
                                                        realtotalpages);
}
{% endhighlight %}

calculate_node_totalpages() 函数的作用是用于计算系统占用的物理页帧
和实际物理页帧的数量，所谓占用物理页帧指的是多块物理内存中间存在 hole，
但这些 hole 是不可用的物理页帧，因此系统占用的物理页帧就是 hole 加上
实际物理页帧数量。如下图，存在多块物理内存块，内存块之间存在 hole，因此
"spanned pages" 指的就是第一块物理内存块到最后一块物理内存块横跨了多少
物理页帧，而 "real pages" 指的就是实际物理内存块占用了多少个物理页帧。

![](/assets/PDB/BiscuitOS/boot/BOOT000353.png)

对于 ZONE 来讲，spanned 和 present 的关系如下：

![](/assets/PDB/BiscuitOS/boot/BOOT000354.png)

函数使用 for() 循环，遍历了所有的 ZONE，每遍历一个 zone，
函数调用 zone_spanned_pages_in_node() 函数计算了该 ZONE
横跨了多少个物理页帧，然后调用 zone_absent_pages_in_node()
函数计算 hole 的大小，以此计算该 ZONE 真正占用了多少个物理
页帧。计算完毕后，进行统计，更新 zone 对应的 spanned_pages
和 present_pages 数据，并且更新全局变量 totalpages 和
realtotalpages 的数量。最后将所有 zone 的 spanned 物理页帧
数据存储到 pglist 的 node_spanned_pages 里，也将所有 zone
的 real 物理页帧存储到 pglist 的 node_present_pages 里。
最后打印消息。

> - [zone_spanned_pages_in_node](#A0320)
>
> - [zone_absent_pages_in_node](#A0321)

------------------------------------

#### <span id="A0323">pgdat_end_pfn</span>

{% highlight c %}
static inline unsigned long pgdat_end_pfn(pg_data_t *pgdat)
{
        return pgdat->node_start_pfn + pgdat->node_spanned_pages;
}
{% endhighlight %}

pgdat_end_pfn() 函数用于获得 pglist 指向的结束页帧号。函数通过从
pglist 结构中获得起始页帧加上横跨页帧的数量，以此获得结束页帧号。

------------------------------------

#### <span id="A0324">memblock_alloc_internal</span>

{% highlight c %}
/**
 * memblock_alloc_internal - allocate boot memory block
 * @size: size of memory block to be allocated in bytes
 * @align: alignment of the region and block's size
 * @min_addr: the lower bound of the memory region to allocate (phys address)
 * @max_addr: the upper bound of the memory region to allocate (phys address)
 * @nid: nid of the free area to find, %NUMA_NO_NODE for any node
 *
 * The @min_addr limit is dropped if it can not be satisfied and the allocation
 * will fall back to memory below @min_addr. Also, allocation may fall back
 * to any node in the system if the specified node can not
 * hold the requested memory.
 *
 * The allocation is performed from memory region limited by
 * memblock.current_limit if @max_addr == %MEMBLOCK_ALLOC_ACCESSIBLE.
 *
 * The phys address of allocated boot memory block is converted to virtual and
 * allocated memory is reset to 0.
 *
 * In addition, function sets the min_count to 0 using kmemleak_alloc for
 * allocated boot memory block, so that it is never reported as leaks.
 *
 * Return:
 * Virtual address of allocated memory block on success, NULL on failure.
 */
static void * __init memblock_alloc_internal(
                                phys_addr_t size, phys_addr_t align,
                                phys_addr_t min_addr, phys_addr_t max_addr,
                                int nid)
{
        phys_addr_t alloc;
        void *ptr;
        enum memblock_flags flags = choose_memblock_flags();

        if (WARN_ONCE(nid == MAX_NUMNODES, "Usage of MAX_NUMNODES is deprecated. Use NUMA_NO_NODE instead\n"))
                nid = NUMA_NO_NODE;

        /*
         * Detect any accidental use of these APIs after slab is ready, as at
         * this moment memblock may be deinitialized already and its
         * internal data may be destroyed (after execution of memblock_free_all)
         */
        if (WARN_ON_ONCE(slab_is_available()))
                return kzalloc_node(size, GFP_NOWAIT, nid);

        if (!align) {
                dump_stack();
                align = SMP_CACHE_BYTES;
        }

        if (max_addr > memblock.current_limit)
                max_addr = memblock.current_limit;
again:
        alloc = memblock_find_in_range_node(size, align, min_addr, max_addr,
                                            nid, flags);
        if (alloc && !memblock_reserve(alloc, size))
                goto done;

        if (nid != NUMA_NO_NODE) {
                alloc = memblock_find_in_range_node(size, align, min_addr,
                                                    max_addr, NUMA_NO_NODE,
                                                    flags);
                if (alloc && !memblock_reserve(alloc, size))
                        goto done;
        }

        if (min_addr) {
                min_addr = 0;
                goto again;
        }

        if (flags & MEMBLOCK_MIRROR) {
                flags &= ~MEMBLOCK_MIRROR;
                pr_warn("Could not allocate %pap bytes of mirrored memory\n",
                        &size);
                goto again;
        }

        return NULL;
done:
        ptr = phys_to_virt(alloc);

        /* Skip kmemleak for kasan_init() due to high volume. */
        if (max_addr != MEMBLOCK_ALLOC_KASAN)
                /*
                 * The min_count is set to 0 so that bootmem allocated
                 * blocks are never reported as leaks. This is because many
                 * of these blocks are only referred via the physical
                 * address which is not looked up by kmemleak.
                 */
                kmemleak_alloc(ptr, size, 0, 0);

        return ptr;
}
{% endhighlight %}

memblock_alloc_internal() 用于从 MEMBLOCK 内存分配器中获得指定
大小的物理内存。参数 size 指向需要分配物理内存的带下，参数 align
指向对齐方式，参数 min_addr 指向分配的最小物理地址，参数 max_addr
指向分配的最大物理地址，参数 nid 指向 NUMA 信息。由于代码较长，分段
解析：

{% highlight c %}
        if (WARN_ONCE(nid == MAX_NUMNODES, "Usage of MAX_NUMNODES is deprecated. Use NUMA_NO_NODE instead\n"))
                nid = NUMA_NO_NODE;

        /*
         * Detect any accidental use of these APIs after slab is ready, as at
         * this moment memblock may be deinitialized already and its
         * internal data may be destroyed (after execution of memblock_free_all)
         */
        if (WARN_ON_ONCE(slab_is_available()))
                return kzalloc_node(size, GFP_NOWAIT, nid);
{% endhighlight %}

函数首先检查 nid 的有效性，然后判断当前 slab 内存分配器是否启用。
如果启用，直接低啊用 kzalloc_node() 函数分配内存，并返回；反之继续
指向下面的代码：

{% highlight c %}
        if (!align) {
                dump_stack();
                align = SMP_CACHE_BYTES;
        }

        if (max_addr > memblock.current_limit)
                max_addr = memblock.current_limit;
{% endhighlight %}

如果 align 参数为零，那么函数调用 dump_stack() 函数打印堆栈
信息，并且将 align 设置为 SMP_CACHE_BYTES. 如果 max_addr
参数大于 MEMBLOCK 内存分配器最大设置 current_limit，那么
函数将 max_addr 设置为 MEMBLOCK 最大限制。

{% highlight c %}
again:
        alloc = memblock_find_in_range_node(size, align, min_addr, max_addr,
                                            nid, flags);
        if (alloc && !memblock_reserve(alloc, size))
                goto done;

        if (nid != NUMA_NO_NODE) {
                alloc = memblock_find_in_range_node(size, align, min_addr,
                                                    max_addr, NUMA_NO_NODE,
                                                    flags);
                if (alloc && !memblock_reserve(alloc, size))
                        goto done;
        }

        if (min_addr) {
                min_addr = 0;
                goto again;
        }

        if (flags & MEMBLOCK_MIRROR) {
                flags &= ~MEMBLOCK_MIRROR;
                pr_warn("Could not allocate %pap bytes of mirrored memory\n",
                        &size);
                goto again;
        }
{% endhighlight %}

检查通过之后，函数调用 memblock_find_in_range_node() 函数就从
MEMBLOCK 分配器中获得指定物理内存，如果分配成功，函数此时调用
memblock_reserve() 函数将分配的物理内存加入到预留区。添加成功
直接跳转到 done。如果 nid 不是 NUMA_NO_NODE, 那么此时重新调用
memblock_find_in_range_node() 函数从 NUMA_NO_NODE 中分配内存，
同样的操作，将该区域加入到系统预留区内。如果分配的物理内存含有
MEMBLOCK_MINOR 标志，那么函数去掉 MEMBLOCK_MINOR 标志并答应
信息后跳转到 again 处。

{% highlight c %}
done:
        ptr = phys_to_virt(alloc);

        /* Skip kmemleak for kasan_init() due to high volume. */
        if (max_addr != MEMBLOCK_ALLOC_KASAN)
                /*
                 * The min_count is set to 0 so that bootmem allocated
                 * blocks are never reported as leaks. This is because many
                 * of these blocks are only referred via the physical
                 * address which is not looked up by kmemleak.
                 */
                kmemleak_alloc(ptr, size, 0, 0);

        return ptr;
}
{% endhighlight %}

如果物理内存分配成功，那么函数调用 phys_to_virt() 函数
获得物理地址对应的虚拟地址。

> - [memblock_find_in_range_node](#A0155)
>
> - [memblock_reserve](#A0203)
>
> - [phys_to_virt](#A0325)
>
> - [choose_memblock_flags](#A0157)

------------------------------------

#### <span id="A0326">phys_to_virt</span>

{% highlight c %}
static inline void *phys_to_virt(phys_addr_t x)
{
        return (void *)__phys_to_virt(x);
}
{% endhighlight %}

phys_to_virt() 用于将物理地址转换成虚拟地址。函数通过调用
__phys_to_virt() 函数实现。

> - [\_\_phys_to_virt](#A0289)

------------------------------------

#### <span id="A0327">memblock_alloc_try_nid_nopanic</span>

{% highlight c %}
/**
 * memblock_alloc_try_nid_nopanic - allocate boot memory block
 * @size: size of memory block to be allocated in bytes
 * @align: alignment of the region and block's size
 * @min_addr: the lower bound of the memory region from where the allocation
 *        is preferred (phys address)
 * @max_addr: the upper bound of the memory region from where the allocation
 *            is preferred (phys address), or %MEMBLOCK_ALLOC_ACCESSIBLE to
 *            allocate only from memory limited by memblock.current_limit value
 * @nid: nid of the free area to find, %NUMA_NO_NODE for any node
 *
 * Public function, provides additional debug information (including caller
 * info), if enabled. This function zeroes the allocated memory.
 *
 * Return:
 * Virtual address of allocated memory block on success, NULL on failure.
 */
void * __init memblock_alloc_try_nid_nopanic(
                                phys_addr_t size, phys_addr_t align,
                                phys_addr_t min_addr, phys_addr_t max_addr,
                                int nid)
{
        void *ptr;

        memblock_dbg("%s: %llu bytes align=0x%llx nid=%d from=%pa max_addr=%pa %pF\n",
                     __func__, (u64)size, (u64)align, nid, &min_addr,
                     &max_addr, (void *)_RET_IP_);

        ptr = memblock_alloc_internal(size, align,
                                           min_addr, max_addr, nid);
        if (ptr)
                memset(ptr, 0, size);
        return ptr;
}
{% endhighlight %}

memblock_alloc_try_nid_nopanic() 函数用于从指定的 NUMA 中分配
物理内存，不能失败。参数 size 指向分配物理内存的大小，参数 align
代表对齐方式，参数 min_addr 指向分配的最小物理地址，参数 max_addr
指向分配最大物理地址。函数通过 memblock_alloc_internal() 函数
不失败的分配内存，然后将分配出来的物理内存进行初始化操作。

> - [memblock_alloc_internal](#A0325)

------------------------------------

#### <span id="A0328">memblock_alloc_node_nopanic</span>

{% highlight c %}
static inline void * __init memblock_alloc_node_nopanic(phys_addr_t size,
                                                        int nid)
{
        return memblock_alloc_try_nid_nopanic(size, SMP_CACHE_BYTES,
                                              MEMBLOCK_LOW_LIMIT,
                                              MEMBLOCK_ALLOC_ACCESSIBLE, nid);
}
{% endhighlight %}

memblock_alloc_node_nopanic() 函数的作用是从 MEMBLOCK 内存
分配器中不失败的分配指定大小的物理内存。参数 size 指向需要分配
物理内存的大小，参数 nid 指向 NUMA。函数通过调用
memblock_alloc_try_nid_nopanic() 函数实现。

> - [memblock_alloc_try_nid_nopanic](#A0327)

------------------------------------

#### <span id="A0329">alloc_node_mem_map</span>

{% highlight c %}
static void __ref alloc_node_mem_map(struct pglist_data *pgdat)
{
        unsigned long __maybe_unused start = 0;
        unsigned long __maybe_unused offset = 0;

        /* Skip empty nodes */
        if (!pgdat->node_spanned_pages)                 
                return;                                 

        start = pgdat->node_start_pfn & ~(MAX_ORDER_NR_PAGES - 1);
        offset = pgdat->node_start_pfn - start;
        /* ia64 gets its own node_mem_map, before this, without bootmem */
        if (!pgdat->node_mem_map) {
                unsigned long size, end;
                struct page *map;

                /*
                 * The zone's endpoints aren't required to be MAX_ORDER
                 * aligned but the node_mem_map endpoints must be in order
                 * for the buddy allocator to function correctly.
                 */
                end = pgdat_end_pfn(pgdat);
                end = ALIGN(end, MAX_ORDER_NR_PAGES);   
                size =  (end - start) * sizeof(struct page);
                map = memblock_alloc_node_nopanic(size, pgdat->node_id);
                pgdat->node_mem_map = map + offset;
        }
        pr_debug("%s: node %d, pgdat %08lx, node_mem_map %08lx\n",
                                __func__, pgdat->node_id, (unsigned long)pgdat,
                                (unsigned long)pgdat->node_mem_map);
#ifndef CONFIG_NEED_MULTIPLE_NODES            
        /*
         * With no DISCONTIG, the global mem_map is just set as node 0's
         */
        if (pgdat == NODE_DATA(0)) {
                mem_map = NODE_DATA(0)->node_mem_map;
#if defined(CONFIG_HAVE_MEMBLOCK_NODE_MAP) || defined(CONFIG_FLATMEM)
                if (page_to_pfn(mem_map) != pgdat->node_start_pfn)
                        mem_map -= offset;
#endif /* CONFIG_HAVE_MEMBLOCK_NODE_MAP */
        }
#endif
}
{% endhighlight %}

alloc_node_mem_map() 函数用于创建全局 struct page 数组 mem_map，并将
mem_map 与节点 0 的物理页帧进行映射。参数 pglist 指向当前节点的 pglist
结构。分段解析函数：

{% highlight c %}
        unsigned long __maybe_unused start = 0;
        unsigned long __maybe_unused offset = 0;

        /* Skip empty nodes */
        if (!pgdat->node_spanned_pages)                 
                return;                                 

        start = pgdat->node_start_pfn & ~(MAX_ORDER_NR_PAGES - 1);
        offset = pgdat->node_start_pfn - start;
{% endhighlight %}

函数首先判断 pgdat 对应的节点是否包含物理页帧，其通过
node_spanned_pages 可以知道，如果该值为 0，那么函数直接返回；
如果不为零，那么函数首先获得当前节点的起始页帧，然后按
MAX_ORDER_NR_PAGES 对齐之后，然后将该值与当前节点的起始
页帧相减以此作为偏移。

{% highlight c %}
        /* ia64 gets its own node_mem_map, before this, without bootmem */
        if (!pgdat->node_mem_map) {
                unsigned long size, end;
                struct page *map;

                /*
                 * The zone's endpoints aren't required to be MAX_ORDER
                 * aligned but the node_mem_map endpoints must be in order
                 * for the buddy allocator to function correctly.
                 */
                end = pgdat_end_pfn(pgdat);
                end = ALIGN(end, MAX_ORDER_NR_PAGES);   
                size =  (end - start) * sizeof(struct page);
                map = memblock_alloc_node_nopanic(size, pgdat->node_id);
                pgdat->node_mem_map = map + offset;
        }
{% endhighlight %}

如果当前节点的 node_mem_map 为零，那么函数通过计算当前节点
包含的页帧数量，并乘与 "sizeof(struct page)", 以此计算当前
节点维护节点所有页帧所需要的内存大小，然后调用 memblock_alloc_node_nopanic()
函数从 MEMBLOCK 内存分配器中分配指定大小的物理内存用于存储
页帧信息。最后函数将分配的内存使用 node_mem_map 指定，并添加
偏移。

{% highlight c %}
#ifndef CONFIG_NEED_MULTIPLE_NODES            
        /*
         * With no DISCONTIG, the global mem_map is just set as node 0's
         */
        if (pgdat == NODE_DATA(0)) {
                mem_map = NODE_DATA(0)->node_mem_map;
#if defined(CONFIG_HAVE_MEMBLOCK_NODE_MAP) || defined(CONFIG_FLATMEM)
                if (page_to_pfn(mem_map) != pgdat->node_start_pfn)
                        mem_map -= offset;
#endif /* CONFIG_HAVE_MEMBLOCK_NODE_MAP */
        }
#endif
}
{% endhighlight %}

如果宏 CONFIG_NEED_MULTIPLE_NODES 没有定义，那么函数首先
判断当前节点是否为节点 0，如果是，那么将全局变量 mem_map
指向当前节点的 node_mem_map 区域，以此维护系统所有 struct page。
如果此时定义了宏 CONFIG_HAVE_MEMBLOCK_NODE_MAP 或者宏 CONFIG_FLATMEM,
那么 mem_map 对应的页帧不等于当前节点的起始页帧，那么将
mem_map 减去偏移。这里实际做了一个 struct page 到物理页帧的
映射，确保 mem_map 的第一个 struct page 指向节点 0 的起始物理页帧。

> - [memblock_alloc_node_nopanic](#A0328)
>
> - [pgdat_end_pfn](#A0323)
>
> - [page_to_pfn](#A0331)

------------------------------------

#### <span id="A0330">__page_to_pfn</span>

{% highlight c %}
#define __page_to_pfn(page)     ((unsigned long)((page) - mem_map) + \
                                 ARCH_PFN_OFFSET)
{% endhighlight %}

__page_to_pfn() 函数用于将 struct page 转换成物理页帧号。参数 page
指向 struct page 结构，函数通过参数 page 与 mem_map 之间的差值，然后
在加上 ARCH_PFN_OFFSET，以此获得物理页帧号。

------------------------------------

#### <span id="A0331">page_to_pfn</span>

{% highlight c %}
#define page_to_pfn __page_to_pfn
{% endhighlight %}

page_to_pfn() 函数用于获得 struct page 对应的物理页帧号。
函数通过 __page_to_pfn() 函数实现。

> - [\_\_page_to_pfn](#A0330)

------------------------------------

#### <span id="A0332">calc_memmap_size</span>

{% highlight c %}
static unsigned long __init calc_memmap_size(unsigned long spanned_pages,
                                                unsigned long present_pages)
{
        unsigned long pages = spanned_pages;

        /*
         * Provide a more accurate estimation if there are holes within
         * the zone and SPARSEMEM is in use. If there are holes within the
         * zone, each populated memory region may cost us one or two extra
         * memmap pages due to alignment because memmap pages for each
         * populated regions may not be naturally aligned on page boundary.
         * So the (present_pages >> 4) heuristic is a tradeoff for that.
         */
        if (spanned_pages > present_pages + (present_pages >> 4) &&
            IS_ENABLED(CONFIG_SPARSEMEM))
                pages = present_pages;

        return PAGE_ALIGN(pages * sizeof(struct page)) >> PAGE_SHIFT;
}
{% endhighlight %}

calc_memmap_size() 函数用于计算当前节点用于维护 spanned
物理页帧的 struct page 占用的页的数量。函数首先判断是否
启用 CONFIG_SPARSEMEM 宏，如果没有启用，那么函数直接将当前
节点的 spanned_pages 与 sizeof(struct page) 相乘以此得到
所需要的内存，最后在向右偏移 PAGE_SHIFT, 以此计算需要的
页数量。

------------------------------------

#### <span id="A0333">zone_movable_is_highmem</span>

{% highlight c %}
static inline int zone_movable_is_highmem(void)
{
#ifdef CONFIG_HAVE_MEMBLOCK_NODE_MAP
        return movable_zone == ZONE_HIGHMEM;
#else
        return (ZONE_MOVABLE - 1) == ZONE_HIGHMEM;
#endif
}
{% endhighlight %}

zone_movable_is_highmem() 函数用于判断 ZONE_MOVABLE
是否是 ZONE_HIGHMEM. 如果系统定义了 CONFIG_HAVE_MEMBLOCK_NODE_MAP，
那么当 movable_zone 是 ZONE_HIGHMEM 是，那么函数返回 1；
反之如果没有定义该宏，只有当 ZONE_MOVABLE - 1 等于
ZONE_HIGHMEM 的时候，函数返回 1.

------------------------------------

#### <span id="A0334">is_highmem_idx</span>

{% highlight c %}
static inline int is_highmem_idx(enum zone_type idx)
{
#ifdef CONFIG_HIGHMEM
        return (idx == ZONE_HIGHMEM ||
                (idx == ZONE_MOVABLE && zone_movable_is_highmem()));
#else
        return 0;
#endif
}
{% endhighlight %}

is_highmem_idx() 函数用于判断参数 idx 是否指向高端内存。当
系统启用了 CONFIG_HIGHMEM 宏，则当 idx 是 ZONE_HIGHMEM 或者
idx 是 ZONE_MOVABLE 且 zone_movable_is_highmem() 函数返回 1，
那么此时 idx 对应的区域就是高端内存。

> - [zone_movable_is_highmem](#A0333)

------------------------------------

#### <span id="A0335">populated_zone</span>

{% highlight c %}
/* Returns true if a zone has memory */
static inline bool populated_zone(struct zone *zone)
{
        return zone->present_pages;
}
{% endhighlight %}

populated_zone() 函数用于判断 ZONE 是否已经包含物理内存。
参数 zone 指向特定 ZONE。当参数 zone 的 present_pages 不为
零时，代表该 ZONE 已经包含了物理内存，即 ZONE 已经 populated。

------------------------------------

#### <span id="A0336">zone_pcp_init</span>

{% highlight c %}
static __meminit void zone_pcp_init(struct zone *zone)
{
        /*
         * per cpu subsystem is not up at this point. The following code
         * relies on the ability of the linker to provide the
         * offset of a (static) per cpu variable into the per cpu area.
         */
        zone->pageset = &boot_pageset;

        if (populated_zone(zone))
                printk(KERN_DEBUG "  %s zone: %lu pages, LIFO batch:%u\n",
                        zone->name, zone->present_pages,
                                         zone_batchsize(zone));
}
{% endhighlight %}

zone_pcp_init() 函数用于初始化 ZONE 的 PCP 分配器。参数 zone
指向特定的 ZONE。函数首先将 zone 的 pageset 指向 boot_pageset。
接着调用函数 populated_zone() 函数判断该 zone 是否已经维护内存，
如果 ZONE 已经包含内存，那么打印相关信息。

> - [populated_zone](#A0335)

------------------------------------

#### <span id="A0337">zone_init_internals</span>

{% highlight c %}
static void __meminit zone_init_internals(struct zone *zone, enum zone_type idx, int nid,
                                                        unsigned long remaining_pages)
{
        atomic_long_set(&zone->managed_pages, remaining_pages);
        zone_set_nid(zone, nid);
        zone->name = zone_names[idx];
        zone->zone_pgdat = NODE_DATA(nid);
        spin_lock_init(&zone->lock);
        zone_seqlock_init(zone);
        zone_pcp_init(zone);
}
{% endhighlight %}

zone_init_internals() 函数用于初始化 zone。参数 zone 指向
特定的 ZONE，参数 idx 指向 ZONE 索引，参数 remaining_pages
参数指向 ZONE 维护的 page 数量。函数首先调用 atomic_long_set()
实现原子操作，将 zone 的 managed_pages 设置为 remaining_pages，
以此管理该 ZONE 所维护的 page 数量。函数调用 zone_set_nid()
函数设置 ZONE 的 NUMA 信息，函数继续填充 ZONE 的结构，包括
name，zone_pgdat，函数调用 zone_seqlock_init() 初始化 zone
的锁，最后调用 zoen_pcp_init() 函数初始化当前 ZONE 的 PCP
分配器。

> - [zone_pcp_init](#A0336)

------------------------------------

#### <span id="A03"></span>

{% highlight c %}

{% endhighlight %}

> - [](#A0)
>

------------------------------------

#### <span id="A03"></span>

{% highlight c %}

{% endhighlight %}

> - [](#A0)
>

------------------------------------

#### <span id="A03"></span>

{% highlight c %}

{% endhighlight %}

> - [](#A0)
>

------------------------------------

#### <span id="A03"></span>

{% highlight c %}

{% endhighlight %}

> - [](#A0)
>

------------------------------------

#### <span id="A03"></span>

{% highlight c %}

{% endhighlight %}

> - [](#A0)
>

------------------------------------

#### <span id="A03"></span>

{% highlight c %}

{% endhighlight %}

> - [](#A0)
>

------------------------------------

#### <span id="A03"></span>

{% highlight c %}

{% endhighlight %}

> - [](#A0)
>

------------------------------------

#### <span id="A03"></span>

{% highlight c %}

{% endhighlight %}

> - [](#A0)
>

------------------------------------

#### <span id="A03"></span>

{% highlight c %}

{% endhighlight %}

> - [](#A0)
>

------------------------------------

#### <span id="A03"></span>

{% highlight c %}

{% endhighlight %}

> - [](#A0)
>

------------------------------------

#### <span id="A03"></span>

{% highlight c %}

{% endhighlight %}

> - [](#A0)
>



## 赞赏一下吧 🙂

![MMU](/assets/PDB/BiscuitOS/kernel/HAB000036.jpg)

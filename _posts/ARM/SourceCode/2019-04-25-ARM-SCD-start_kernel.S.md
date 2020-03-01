---
layout:             post
title:              "start_kernel"
date:               2019-04-25 08:58:30 +0800
categories:         [MMU]
excerpt:            start_kernel.
tags:
  - MMU
---

> Email: BuddyZhang1 <buddy.zhang@aliyun.com>
>
> Architecture: ARMv7 Cortex-A9MP (arm32) Linux 5.0

# 目录

> - [实践](#实践)
>
> - [附录](#附录)

--------------------------------------------------------------
<span id="实践"></span>

![MMU](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND00000A.jpg)

# 实践

> - [set_task_stack_end_magic](#set_task_stack_end_magic)
>
> - [smp_setup_processor_id](#smp_setup_processor_id)
>
> - [debug_objects_early_init](#debug_objects_early_init)
>
> - [cgroup_init_early](#cgroup_init_early)

<span id="set_task_stack_end_magic"></span>

内核初始化到 start_kernel() 函数是，第一个执行的函数是 set_task_stack_end_magic(),
这个函数的功能是标记内核，其定义如下：

{% highlight haskell %}
void set_task_stack_end_magic(struct task_struct *tsk)
{
        unsigned long *stackend;

        stackend = end_of_stack(tsk);
        *stackend = STACK_END_MAGIC;    /* for overflow detection */
}
{% endhighlight %}

set_task_stack_end_magic 函数通过调用 end_of_stack() 函数获得 task_struct 对应
的堆栈结束位置，然后在这个位置上写入 STACK_END_MAGIC，那 task_struct 的堆栈如何
获得，可以看一下 end_of_stack() 函数的定义，如下：

{% highlight haskell %}
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
        return (unsigned long *)(task_thread_info(p) + 1);
}

#define task_thread_info(task) ((struct thread_info *)(task)->stack)
{% endhighlight %}

通过上面的源码可以知道，init_task 包含了内核堆栈 init_stack, init_stack 与
thread_info 是公用一个 THREAD_SIZE 空间，end_of_stack() 函数通过
task_thread_info() 函数将共用 THREAD_SIZE 空间的下一个 struct thread_info
地址作为堆栈的结束地址，因此 init_task 的 thread_info 和 init_stack
关系如下图：

![MMU](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/boot/BOOT000054.png)

因此堆栈结束地址就如上图，更多关于内核堆栈的信息，请看：

> [Linux init_stack 堆栈](https://biscuitos.github.io/blog/ARM-init_stack/)

调用完 end_of_stack() 获得了 task_struct 对应的堆栈，然后将 STACK_END_MAGIC 写入
该地址。开发者可以在 set_task_stack_end_magic() 处加入断点，然后使用 GDB 进行调试，
调试的情况如下：

{% highlight haskell %}
(gdb) b set_task_stack_end_magic
Breakpoint 1 at 0x8011da84: set_task_stack_end_magic. (2 locations)
(gdb) c
Continuing.

Breakpoint 1, set_task_stack_end_magic (tsk=0x80b0ba40 <init_task>)
    at kernel/fork.c:829
829	{
(gdb) print &__start_init_task
$1 = (<data variable, no debug info> *) 0x80b00000 <init_thread_info>
(gdb) print &init_thread_union
$2 = (union thread_union *) 0x80b00000 <init_thread_info>
(gdb) print &init_stack
$3 = (unsigned long (*)[2048]) 0x80b00000 <init_thread_info>
(gdb) print &__end_init_task
$4 = (<data variable, no debug info> *) 0x80b02000 <vdso_data_store>
(gdb) x/16x 0x80b0020c
0x80b0020c <init_thread_info+524>:	0x00000000	0x00000000	0x00000000	0x00000000
0x80b0021c:	0x00000000	0x00000000	0x00000000	0x00000000
0x80b0022c:	0x00000000	0x00000000	0x00000000	0x00000000
0x80b0023c:	0x00000000	0x00000000	0x00000000	0x00000000
(gdb) n
832		stackend = end_of_stack(tsk);
(gdb) n
833		pr_info("DD: %#lx\n", stackend);
(gdb) n
834		*stackend = STACK_END_MAGIC;	/* for overflow detection */
(gdb) n
835	}
(gdb) x/16x 0x80b0020c
0x80b0020c <init_thread_info+524>:	0x00000000	0x57ac6e9d	0x00000000	0x00000000
0x80b0021c:	0x00000000	0x00000000	0x00000000	0x00000000
0x80b0022c:	0x00000000	0x00000000	0x00000000	0x00000000
0x80b0023c:	0x00000000	0x00000000	0x00000000	0x00000000
(gdb)
{% endhighlight %}

由于 struct thread_info 的大小为 0x210, 从调试的情况看出，init_stack 的地址是
0x80b00000, 那么 end_of_stack(&init_task) 返回的地址就是 0x80b00210, 所有
开发者就观察 0x80b00210 地址对应内容的变化，最后调试结果显示，STACK_END_MAGIC
被写入到这个地址。接下来运行的函数如下：

<span id="smp_setup_processor_id"></span>
{% highlight haskell %}
void __init smp_setup_processor_id(void)
{
        int i;
        u32 mpidr = is_smp() ? read_cpuid_mpidr() & MPIDR_HWID_BITMASK : 0;
        u32 cpu = MPIDR_AFFINITY_LEVEL(mpidr, 0);

        cpu_logical_map(0) = cpu;
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

smp_setup_processor_id() 函数的主要任务就是设置 boot CPU 信息；函数首先调用 is_smp() 函数判断
当前系统是否支持 SMP，其定义如下：

{% highlight haskell %}
arch/arm/include/asm/smp_plat.h

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

arch/arm/kernel/head.S

        .pushsection .data
        .align  2
        .globl  smp_on_up
smp_on_up:
        ALT_SMP(.long   1)
        ALT_UP(.long    0)
        .popsection
{% endhighlight %}

本实践中，宏 CONFIG_SMP_ON_UP 和 CONFIG_SMP 都启用，因此，smp 的支持情况存储
在 smp_on_up 里，由于 CONFIG_SMP_ON_UP 的定义，因此 smp_on_up 的设置为
"ALT_UP(.long 0)", 因此 is_smp() 将会返回 0。回到 smp_setup_processor_id()
函数，在通过 is_smp() 函数返回 0 之后，mpidr 局部变量的值为 0。然后通过
MPIDR_AFFINITY_LEVEL() 与 mpidr 获得 CPU 号，MPIDR_AFFINITY_LEVEL 的定义如下：

{% highlight haskell %}
arch/arm/include/asm/cputype.h

#define MPIDR_AFFINITY_LEVEL(mpidr, level) \
        ((mpidr >> (MPIDR_LEVEL_BITS * level)) & MPIDR_LEVEL_MASK)

#define MPIDR_LEVEL_BITS 8
#define MPIDR_LEVEL_MASK ((1 << MPIDR_LEVEL_BITS) - 1)
{% endhighlight %}

在执行这段代码之前，先看一下 MPIDR 寄存器，通过 ARMv7 手册可以找到其内存布局如下：

![MMU](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/boot/BOOT000055.png)

MPIDR 寄存器的主要作用是在 SMP 系统中，为调度提供了一个附加的处理器识别机制。其中
每个 Affn 占用 8 bit， Aff0 bits[7:0] 代表 Affinity level 0，Aff1 bits[15:8]
代表 Affinity level 1，Aff2 bits[23:6] 代表 Affinity level 2，因此
MPIDR_AFFINITY_LEVEL() 要将对应 level 的 Affinity 读取出来就需要将 MPIDR
特定 level 向右移动 "MPIDR_LEVEL_BITS * level" 获得，然后与 MPIDR_LEVEL_MASK
相与就可以读出 MPIDR 中特定的 Affn。回到 smp_setup_processor_id() 中，由于
获得的 mpidr 为 0，那么通过 MPIDR_AFFINITY_LEVEL() 处理之后的值为 0。内核创建了
一个 u32 的数组 __cpu_logical_map[] 用于管理逻辑 CPU 的映射，其定义如下：

{% highlight haskell %}
/*
 * Logical CPU mapping.
 */     
extern u32 __cpu_logical_map[];
#define cpu_logical_map(cpu)    __cpu_logical_map[cpu]

arch/arm/kernel/setup.c

u32 __cpu_logical_map[NR_CPUS] = { [0 ... NR_CPUS-1] = MPIDR_INVALID };
#define MPIDR_INVALID (~MPIDR_HWID_BITMASK)
{% endhighlight %}

内核定义了 u32 数组 __cpu_logical_map， 其长度与 NR_CPUS 有关，每个成员初始化为
MPIDR_INVALID。smp_setup_processor_id() 函数中，将获得的 cpu 号写入数组的第一个
成员里，接着在 for 循环中，先判断 i 的值是否与 cpu 的值相等，如果相等，则将
__cpu_logical_map[i] 对应的值设置为 0,；反之如果不等，则将  __cpu_logical_map[i]
设置为 i。接着调用函数 set_my_cpu_offset()，其定义如下：

{% highlight haskell %}
static inline void set_my_cpu_offset(unsigned long off)
{             
        /* Set TPIDRPRW */
        asm volatile("mcr p15, 0, %0, c13, c0, 4" : : "r" (off) : "memory");
}
{% endhighlight %}

这里使用内嵌汇编将汇编，会对 ARMv7 的 CP15 寄存器操作，此时的 CP15 c13 布局如下：

![MMU](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/boot/BOOTCP15C13.png)

选中的 TPIDRPRW 寄存器，该寄存器的作用是当系统运行在 PL1 或者更高的级别，提供一个
位置以此听 thread identifying information. smp_setup_processor_id() 中将 0 写入
该寄存器，以此清除 __my_cpu_offset 在 boot CPU 上避免由 percpu 变量引起的挂起。
最后 smp_setup_processor_id() 调用 pr_info() 打印字符串 "Booting Linux on physical CPU".
开发者可以在 smp_setup_processor_id() 处添加断点，然后使用 GDB 进行调试，调试
情况如下：

{% highlight haskell %}
(gdb) b smp_setup_processor_id
Breakpoint 1 at 0x80a009d0: smp_setup_processor_id. (2 locations)
(gdb) c
Continuing.

Breakpoint 1, smp_setup_processor_id () at arch/arm/kernel/setup.c:591
591		u32 mpidr = is_smp() ? read_cpuid_mpidr() & MPIDR_HWID_BITMASK : 0;
(gdb) n
589	{
(gdb)
591		u32 mpidr = is_smp() ? read_cpuid_mpidr() & MPIDR_HWID_BITMASK : 0;
(gdb) n
595		for (i = 1; i < nr_cpu_ids; ++i)
(gdb) n
594		cpu_logical_map(0) = cpu;
(gdb) print __cpu_logical_map[0]
$3 = 4278190080
(gdb) n
595		for (i = 1; i < nr_cpu_ids; ++i)
(gdb) print __cpu_logical_map[0]
$4 = 0
(gdb) print __cpu_logical_map[1]
$5 = 4278190080
(gdb) n
596			cpu_logical_map(i) = i == cpu ? 0 : i;
(gdb) n 10
595		for (i = 1; i < nr_cpu_ids; ++i)
(gdb) print __cpu_logical_map[1]
$6 = 1
(gdb) print __cpu_logical_map[2]
$7 = 2
(gdb) print __cpu_logical_map[3]
$8 = 3
(gdb) n
605		pr_info("Booting Linux on physical CPU 0x%x\n", mpidr);
(gdb) print __cpu_logical_map[3]
$9 = 3
(gdb) info reg TPIDRPRW
TPIDRPRW       0x0                 0
(gdb)
{% endhighlight %}

通过实践，可以看到运行的结果和预期的一致。接下来运行的函数是：
debug_objects_early_init() 函数，其定义如下：

<span id="debug_objects_early_init"></span>
{% highlight haskell %}
#ifndef CONFIG_DEBUG_OBJECTS
static inline void debug_objects_early_init(void) { }
#endif
{% endhighlight %}

由于本系统不支持 CONFIG_DEBUG_OBJECTS 宏，因此 debug_objects_early_init() 定义
为空。接下里执行的函数是：

<span id="cgroup_init_early"></span>
{% highlight haskell %}
/**
 * cgroup_init_early - cgroup initialization at system boot
 *      
 * Initialize cgroups at system boot, and initialize any
 * subsystems that request early init.
 */
int __init cgroup_init_early(void)
{
        static struct cgroup_sb_opts __initdata opts;
        struct cgroup_subsys *ss;
        int i;

        init_cgroup_root(&cgrp_dfl_root, &opts);
        cgrp_dfl_root.cgrp.self.flags |= CSS_NO_REF;

        RCU_INIT_POINTER(init_task.cgroups, &init_css_set);

        for_each_subsys(ss, i) {
                WARN(!ss->css_alloc || !ss->css_free || ss->name || ss->id,
                     "invalid cgroup_subsys %d:%s css_alloc=%p css_free=%p id:name=%d:%s\n",
                     i, cgroup_subsys_name[i], ss->css_alloc, ss->css_free,
                     ss->id, ss->name);
                WARN(strlen(cgroup_subsys_name[i]) > MAX_CGROUP_TYPE_NAMELEN,
                     "cgroup_subsys_name %s too long\n", cgroup_subsys_name[i]);

                ss->id = i;
                ss->name = cgroup_subsys_name[i];
                if (!ss->legacy_name)
                        ss->legacy_name = cgroup_subsys_name[i];

                if (ss->early_init)
                        cgroup_init_subsys(ss, true);
        }
        return 0;
}
{% endhighlight %}

cgroup_init_early() 函数的主要作用是在内核启动的早期，初始化 cgroup。

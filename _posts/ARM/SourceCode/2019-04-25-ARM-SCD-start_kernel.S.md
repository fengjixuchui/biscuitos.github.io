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

![MMU](https://raw.githubusercontent.com/EmulateSpace/PictureSet/master/BiscuitOS/kernel/IND00000A.jpg)

# 实践

> - [set_task_stack_end_magic](#set_task_stack_end_magic)
>

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

![MMU](https://raw.githubusercontent.com/EmulateSpace/PictureSet/master/BiscuitOS/boot/BOOT000054.png)

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
被写入到这个地址。

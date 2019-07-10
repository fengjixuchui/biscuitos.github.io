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

### <span id="A0000">start_kernel</span>

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
> - [smp_setup_processor_id]()
>
> - [debug_objects_early_init]()

------------------------------------

### <span id="A0001">set_task_stack_end_magic</span>

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

### <span id="A0002">end_of_stack</span>

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

### <span id="A0003">task_thread_info</span>

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


## 赞赏一下吧 🙂

![MMU](https://raw.githubusercontent.com/EmulateSpace/PictureSet/master/BiscuitOS/kernel/HAB000036.jpg)

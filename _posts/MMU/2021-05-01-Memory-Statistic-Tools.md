---
layout: post
title:  "Memory Statistic Tools"
date:   2021-05-01 06:00:00 +0800
categories: [HW]
excerpt: MMU Tools.
tags:
  - Tools
---

![](/assets/PDB/BiscuitOS/kernel/IND00000L0.PNG)

![](/assets/PDB/RPI/RPI100100.png)

#### 目录

> - [Memory Statistic 简介](#A)
>
> - [cgroup memory ctrol](#T0)
>
> - [/proc/meminfo](#T1)
>
> - [/proc/buddyinfo](#T2)
>
> - [/proc/pagetypeinfo](#T3)
>
> - [/proc/vmstat](#T4)
>
> - [/proc/zoneinfo](#T5)
>
> - [/proc/pid/maps](#T6)
>
> - [/proc/oid/smaps](#T7)
>
> - [/proc/pid/limit](#T8)
>
> - [free](#T9)
>
> - [dmidecode](#TA)
>
> - [vmstat](#TB)
>
> - [top](#TC)
>
> - [htop](#TD)
>
> - [pmap](#TE)
>
> - [memstat](#TF)
>
> - [nmon](#TG)
>
> - [smem](#TH)
>
> - [ksysguard](#TJ)
>
> - [df](#TK)
>
> - [ipcs](#TL)
>
> - [numactl](#TM)
>
> - [lscpu](#TN)
>
> - [crash](#TP)
>
> - [留言与问题反馈](#ANS)
>
> - [附录/捐赠](#Z0)

![](/assets/PDB/BiscuitOS/kernel/IND000100.png)

----------------------------------

<span id="T0"></span>

![](/assets/PDB/BiscuitOS/kernel/IND00000H.jpg)

> - [cgroup 简介](#T00)
>
> - [cgroup memory subsystem](#T01)
>
> - [cgroup memory 内核配置](#T02)
>
> - [cgroup memory 基础使用](#T03)

#### <span id="T00">cgroup 简介</span>

cgroup 与 namespace 类似，也是对进程进行分组控制，但与 namespace 不一样，namespace 是为了隔离进程之间的资源，而 cgroup 是为了对一组进程统一的资源进行控制和限制。在 Linux 世界里，一致以来就有对进程进行分组的概念和需求，比如 session group, process group 等，后来随着这方面的需求越来越多，比如需要跟踪一组进程的内存和 IO 使用情况等，于是出现了 cgroup, 用来统一将进程进行分组，并在分组的基础上对进程进程监控和资源控制管理等。cgroup 是 Linux 下的一种将进程按组进行管理的机制，在用户层看来，cgroup 技术就是把系统中的所有进程组织成一颗一颗独立的树，每棵树都包含系统的所有进程，树的每个节点是一个进程组，而每颗树又和一个或者多个 subsystem 关联，树的作用是将进程分组，而 subsystem 的作用就是对这些组进行操作。cgroup 主要包括下面两部分:

###### subsystem

一个 subsystem 就是一个内核模块，他被关联到一颗 cgroup 树之后，就会在树的每个节点(进程组)上做具体的操作。subsystem 经常被称作 "resource controller"，因为它主要被用来调度或者限制每个进程组的资源，但是这个说法不完全准确，因为有时我们将进程分组只是为了做一些监控，观察一下他们的状态，比如 perf_event subsystem。到目前为止，Linux 支持 12 种 subsystem，比如限制 CPU 的使用时间，限制使用的内存，统计 CPU 的使用情况，冻结和恢复一组进程等.

###### hierarchy

一个 hierarchy 可以理解为一棵 cgroup 树，树的每个节点就是一个进程组，每棵树都会与零到多个 subsystem 关联。在一颗树里面，会包含 Linux 系统中的所有进程，但每个进程只能属于一个节点(进程组)。系统中可以有很多颗 cgroup 树，每棵树都和不同的 subsystem 关联，一个进程可以属于多颗树，即一个进程可以属于多个进程组，只是这些进程组和不同的 subsystem 关联。目前 Linux 支持 12 种 subsystem，如果不考虑不与任何 subsystem 关联的情况 (systemd 就属于这种情况），Linux 里面最多可以建 12 颗 cgroup 树，每棵树关联一个 subsystem，当然也可以只建一棵树，然后让这棵树关联所有的 subsystem。当一颗 cgroup 树不和任何 subsystem 关联的时候，意味着这棵树只是将进程进行分组，至于要在分组的基础上做些什么，将由应用程序自己决定.

![](/assets/PDB/BiscuitOS/kernel/IND000100.png)

----------------------------------

#### <span id="T01">cgroup memory subsystem</span>

代码总会有 bug，有时会有内存泄漏，或者有意想不到的内存分配情况，或者这是个恶意程序，运行起来就是为了榨干系统内存，让其它进程无法分配到足够的内存而出现异常，如果系统配置了交换分区，会导致系统大量使用交换分区，从而系统运行很慢。cgroup 对内存控制能做的如下:

* 限制 cgroup 中所有进程所能使用的物理内存总量
* 限制 cgroup 中所有进程所能使用的物理内存与交换空间总量(CONFIG_MEMCG_SWAP)
* 限制 cgroup 中所有进程所能使用的内核内存总量及其它一些内核资源.

![](/assets/PDB/BiscuitOS/kernel/IND000100.png)

---------------------------------------------

#### <span id="T02">cgroup 内核配置</span>

cgroup 使用 memory subsystem 对进程组的内存资源进行控制，具体的资源与内核配置有关，因此在使用 cgroup 控制内存之前，请确保内核相应的宏已经打开，其在 BiscuitOS 的配置如下(以 BiscuitOS Linux 5.0 X86_64 为例)

{% highlight bash %}
cd BiscuitOS/output/linux-5.0-x86_64/linux/linux
make menuconfig ARCH=x86_64

      General setup  --->
      [*] Control Group support  --->

{% endhighlight %}

![](/assets/PDB/HK/TH000301.png)

重新编译内核之后，可以在系统 "/proc/cgroup" 中查看 Cgroup subsystem 的支持信息:

![](/assets/PDB/HK/TH000304.png)

由于当前系统只启用了 memory subsystem, 因此只看到 memory susbsystem.

--------------------------------------

###### CMDLINE

内核 CMDLINE 提供了多个字段用于控制 Cgroup memory subsystem, 具体如下:

* cgroup_disable=memory 用于禁用 memory subsystem
* swapaccount=0 禁用 memory subsystem 的 Swap Extension
* swapaccount=1 启用 memory subsystem 的 Swap Extension

--------------------------------------

###### CONFIG_CGROUPS

{% highlight bash %}
This option adds support for grouping sets of processes together, for  
use with process control subsystems such as Cpusets, CFS, memory       
controls or device isolation.                                          
See                                                                    
      - Documentation/scheduler/sched-design-CFS.txt   (CFS)           
      - Documentation/cgroup-v1/ (features for grouping, isolation     
                                and resource control) 
{% endhighlight %}

CONFIG_CGROUPS 宏用于控制内核的 CGROUP 子系统，在使用 cgroup 的系统上必须打开该宏.

---------------------------------------------

###### CONFIG_MEMCG

{% highlight bash %}
Provides control over the memory footprint of tasks in a cgroup.    
                                                                    
Symbol: MEMCG [=y]                                                  
Type  : bool                                                        
Prompt: Memory controller                                           
  Location:                                                         
    -> General setup                                                
      -> Control Group support (CGROUPS [=y])                       
  Defined at init/Kconfig:722                                       
  Depends on: CGROUPS [=y]                                          
  Selects: PAGE_COUNTER [=n] && EVENTFD [=y]
{% endhighlight %}

CONFIG_MEMCG 宏用于打开 Cgroupp 的 memory subsystem. 如果要使用 Cgroup 控制进程组的内存资源，那么该宏一定要启用. 当启用该宏之后创建一颗 Cgroup 树的时候，其挂载点下的布局如下:

![](/assets/PDB/HK/TH000302.png)

从上图可以看出，当系统启用 CONFIG_MEMCG 宏之后，挂载一颗 cgroup 树之后，在树的目录下会自动生成多个与内存相关的文件节点，系统可以通过提供的文件节点对进程组的内存资源进行控制。

-------------------------------------

###### CONFIG_MEMCG_KMEM

{% highlight bash %}
Symbol: MEMCG_KMEM [=y]                                                   
Type  : bool                                                              
  Defined at init/Kconfig:749                                             
  Depends on: CGROUPS [=y] && MEMCG [=y] && !SLOB [=n]
{% endhighlight %}

CONFIG_MEMCG_KMEM 宏用于 Cgroup memory subsystem 对 KMEM 内存资源的控制，该宏由 CONFIG_MEMCG 自动选择，因此 KMEM 内存资源称为 Cgroup memory 默认管理的资源，当挂载一颗 Cgroup 树的时候，其挂载点布局如下:

![](/assets/PDB/HK/TH000302.png)

从上图可以看出，当系统挂载一颗 Cgroup 树之后，在树的目录下就存在多个与 KMEM 内存相关的文件节点，系统可以通过控制 KMEM 文件节点来控制 KMEM 内存资源.

------------------------------------------

###### CONFIG_MEMCG_SWAP

{% highlight bash %}      
Provides control over the swap space consumed by tasks in a cgroup.  
                                                                     
Symbol: MEMCG_SWAP [=y]                                              
Type  : bool                                                         
Prompt: Swap controller                                              
  Location:                                                          
    -> General setup                                                 
      -> Control Group support (CGROUPS [=y])                        
        -> Memory controller (MEMCG [=y])                            
  Defined at init/Kconfig:729                                        
  Depends on: CGROUPS [=y] && MEMCG [=y] && SWAP [=y]
{% endhighlight %}

-----------------------------------------

###### CONFIG_MEMCG_SWAP_ENABLED

{% highlight bash %}
Memory Resource Controller Swap Extension comes with its price in  
a bigger memory consumption. General purpose distribution kernels  
which want to enable the feature but keep it disabled by default   
and let the user enable it by swapaccount=1 boot command line      
parameter should have this option unselected.                      
For those who want to have the feature enabled by default should   
select this option (if, for some reason, they need to disable it   
then swapaccount=0 does the trick).                                
                                                                   
Symbol: MEMCG_SWAP_ENABLED [=y]                                    
Type  : bool                                                       
Prompt: Swap controller enabled by default                         
  Location:                                                        
    -> General setup                                               
      -> Control Group support (CGROUPS [=y])                      
        -> Memory controller (MEMCG [=y])                          
          -> Swap controller (MEMCG_SWAP [=y])                     
  Defined at init/Kconfig:735                                      
  Depends on: CGROUPS [=y] && MEMCG_SWAP [=y]
{% endhighlight %}

CONFIG_MEMCG_SWAP_ENABLED (3.6 以后的内核新加的参数) 控制默认情况下是否使用 Swap Extension，由于 Swap Extension 比较耗资源，所以很多发行版 (比如 Ubuntu) 默认情况下会禁用该功能，当然用户也可以根据实际情况，通过设置内核参数 "swapaccount=0" 或者 1 来手动禁用和启用 Swap Extension. 当挂载一颗 Cgroup 树的时候，其挂载点布局如下:

![](/assets/PDB/HK/TH000303.png)

正如上面的布局，当挂载一颗 Cgroup 树之后，在树目录下多了 "memory.memsw" 相关文件节点，系统可以通过控制这些文件节点来控制 SWAP 相关的内存资源.

![](/assets/PDB/BiscuitOS/kernel/IND000100.png)

----------------------------------

<span id="T3"></span>

![](/assets/PDB/BiscuitOS/kernel/IND00000T.jpg)

#### cgroup memory 基础使用

Cgroup 相关的所有操作都是基于内核中的 cgroup virtual filesystem，因此通过 Cgroup 控制进程的内存可以按照下面的步骤进行操作:

###### [步骤 1] 挂载一颗 Cgroup 树

首先需要挂载一颗与 memory subsystem 关联的 Cgroup 树到指定的目录:

{% highlight bash %}
# 创建目录
mkdir -p /Cgroup_memory
# 挂载 Cgroup 树
# 命令格式:
#   mount -t cgroup -o subsystem Name Mount_dir
mount -t cgroup -o memory BiscuitOS /Cgroup_memory
{% endhighlight %}

在挂载之前确认已经创建好挂载点，挂载点为一个目录，可以使用 mkdir 命令进行创建。使用 mount 命令进行挂载，将挂载的文件系统类型设置为 cgroup, 然后通过 "-o" 指定 subsystem 的类型，这里选择 subsystem, NAME 字段则是私有名字，可以任意，Mount_dir 字段则是挂载点的路径。在挂载完毕之后，可以查看 memory cgroup 提供的文件节点:

![](/assets/PDB/HK/TH000335.png)


----------------------------------

<span id="T1"></span>

![](/assets/PDB/BiscuitOS/kernel/IND00000M.jpg)

#### /proc/meminfo

![](/assets/PDB/HK/TH000882.png)

"/proc/meminfo" 是了解 Linux 系统内存使用状态的主要接口，与常用的 "free"、"vmstat" 等命令通过它获得数据。"/proc/meminfo" 的源码来自 "fs/proc/meminfo.c" 的 "meminfo_proc_show()" 函数，那么各字段的含义如下:

> - [MemTotal](#T101)
>
> - [MemFree](#T102)
>
> - [MemAvailable](#T103)
>
> - [Buffers](#T104)
>
> - [Cached](#T105)
>
> - [SwapCached](#T106)
>
> - [Active](#T107)
>
> - [Inactive](#T108)
>
> - [Active(anon)](#T10)
>
> - [Inactive(anon)](#T10)
>
> - [Active(file)](#T10)
>
> - [Inactive(file)](#T10)
>
> - [Unevictable](#T10)
>
> - [Mlocked](#T10)
>
> - [SwapTotal](#T10)
>
> - [SwapFree](#T10)
>
> - [Dirty](#T10)
>
> - [Writeback](#T10)
>
> - [AnonPages](#T10)
>
> - [Mapped](#T10)
>
> - [Shmem](#T10)
>
> - [KReclaimable](#T10)
>
> - [Slab](#T10)
>
> - [SReclaimable](#T10)
>
> - [SUnreclaim](#T10)
>
> - [KernelStack](#T10)
>
> - [PageTables](#T10)
>
> - [NFS_Unstable](#T10)
>
> - [Bounce](#T10)
>
> - [WritebackTmp](#T10)
>
> - [CommitLimit](#T10)
>
> - [Committed_AS](#T10)
>
> - [VmallocTotal](#T10)
>
> - [VmallocUsed](#T10)
>
> - [VmallocChunk](#T10)
>
> - [Percpu](#T10)
>
> - [HugePages_Total](#T10)
>
> - [HugePages_Free](#T10)
>
> - [HugePages_Rsvd](#T10)
>
> - [HugePages_Surp](#T10)
>
> - [Hugepagesize](#T10)
>
> - [Hugetlb](#T10)
>
> - [DirectMap4k](#T10)
>
> - [DirectMap2M](#T10)
>
> - [DirectMap1G](#T10)

![](/assets/PDB/BiscuitOS/kernel/IND000100.png)


###### <span id="T101">MemTotal</span>

系统从加电开始到引导完成，firmware/BIOS 要保留一些内存，kernel 本身要占用一些内存，最后剩下可供 kernel 支配的内存就是 MemTotal。这个值在系统运行期间一般是固定不变的。可参阅解读 DMESG 中的内存初始化信息.

###### <span id="T102">MemFree</span>

表示系统尚未使用的内存。"MemTotal - MemFree" 就是已被用掉的内存.

###### <span id="T103">MemAvailable</span>

有些应用程序会根据系统的可用内存大小自动调整内存申请的多少，所以需要一个记录当前可用内存数量的统计值，MemFree 并不适用，因为 MemFree 不能代表全部可用的内存，系统中有些内存虽然已被使用但是可以回收的，比如 cache/buffer、slab 都有一部分可以回收，所以这部分可回收的内存加上 MemFree 才是系统可用的内存，即 MemAvailable。/proc/meminfo 中的 MemAvailable 是内核使用特定的算法估算出来的，要注意这是一个估计值，并不精确.






-----------------------------------------------

#### <span id="Z0">附录</span>

> [BiscuitOS Home](https://biscuitos.github.io/)
>
> [BiscuitOS Blog 2.0](/blog/BiscuitOS_Catalogue/)
>
> [Linux Kernel](https://www.kernel.org/)
>
> [Bootlin: Elixir Cross Referencer](https://elixir.bootlin.com/linux/latest/source)
>

#### 捐赠一下吧 🙂

![MMU](/assets/PDB/BiscuitOS/kernel/HAB000036.jpg)

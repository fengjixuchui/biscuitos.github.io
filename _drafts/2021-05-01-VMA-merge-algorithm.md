---
layout: post
title:  "VMA Merge Algorithm"
date:   2021-05-01 06:00:00 +0800
categories: [HW]
excerpt: VMA Merge Algorithm.
tags:
  - MMU
---

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND00000L0.PNG)

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI100100.png)

#### 目录

> - [VMA Merge 原理](#A)
>
> - [Merge Layout 0: VMA Disconnect with PREV Algorithm](#B)
>   
>   - [VMA Disconnect with NEXT Algorithem](#B0)
>
>   - [VMA Adjoin with NEXT Algorithm](#B1)
> 
>   - [VMA Cross with NEXT Algorithm](#B2)
> 
>   - [VMA Cover with NEXT Algorithm](#B3)
> 
> - [Merge Layout 1: VMA Adjoin with PREV Algorithm](#C)
> 
>   - [VMA Disconnect with NEXT Algorithem](#C0)
>
>   - [VMA Adjoin with NEXT Algorithm](#C1)
> 
>   - [VMA Cross with NEXT Algorithm](#C2)
> 
>   - [VMA Cover with NEXT Algorithm](#C3)
> 
> - [Merge Layout 2: VMA Cross with PREV Algorithm](#D)
> 
>   - [VMA Disconnect with NEXT Algorithem](#D0)
>
>   - [VMA Adjoin with NEXT Algorithm](#D1)
> 
>   - [VMA Cross with NEXT Algorithm](#D2)
> 
>   - [VMA Cover with NEXT Algorithm](#D3)
>
> - [问题反馈与留言](https://github.com/BiscuitOS/BiscuitOS/issues)
>
> - [附录/捐赠](#Z0)

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND000100.png)

----------------------------------

<span id="B"></span>

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND00000H.jpg)

#### Merge Layout 0: VMA Disconnect with PREV Algorithm

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH000329.png)

当一个新建的虚拟区域 NEW VMA 准备加入进程时，其区域正好前一个虚拟区域 PREV 相离，那么此时将这种虚拟区域布局定义为 "Merge Layout 0". 在这种情况下，PREV 的 NEXT 区域可能存在，也可能不存在。如果 NEXT 存在，那么新建区域可能和 NEXT 存在多种位置关系，假设新建区域与其他 VMA 区域符合合并的条件。那么接下来分别讨论不同位置关系下的合并算法:

> - [VMA disconnect with NEXT region (or NEXT doesn't exist) Algorithem](#B0)
>
> - [VMA adjoin with NEXT region Algorithm](#B1)
>
> - [VMA intersect with NEXT region Algorithm](#B2)
>
> - [VMA cover with NEXT region Algorithm](#B3)

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND000100.png)

----------------------------------

#### <span id="B0">VMA Disconnect with NEXT Algorithem</span>

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH000330.png)

基于 "Merge Layout 0", PREV 与新建虚拟区域相离，因此 NEW VMA 不会与 PREV 合并; 另外 NEW VMA 与 NEXT 相离，因此 NEW VMA 与 NEXT 也不会合并, 当 NEW VMA 插入进程之后，进程会独立维护这三个虚拟区域。接下来通过一个实例进行讲解验证, 该实例可以在 BiscuitOS 进行实践部署，部署请参考:

{% highlight bash %}
cd BiscuitOS
make menuconfig

  [*] Package  --->
      [*] Memory Mapping Mechanism  --->
          [*] VMA Merge Region Algorithm  --->
              [*] Layout0: disconnect with NEXT region  --->

BiscuitOS/output/linux-XXXX/package/BiscuitOS-vma-merge-layout0-disconnect-default
{% endhighlight %}

> [VMA Merge Layout0: Disconnect with NEXT Source Code](https://gitee.com/BiscuitOS_team/HardStack/tree/Gitee/Memory-Allocator/MMAP/BiscuitOS-vma-merge/BiscuitOS-vma-merge-layout0-disconnect)
>
> [BiscuitOS 独立程序实践教程](https://biscuitos.github.io/blog/Human-Knowledge-Common/#C)

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH000331.png)

在本例中，程序通过匿名私有映射的方式，并指明映射的虚拟地址，建立了三个虚拟区域。程序在 59 行通过调用函数建立 PREV 虚拟区域，其区域范围是 "0x600000000000 - 0x600000001000"; 程序接着在 60 行调用函数建立 NEXT 虚拟区域，其区域范围是 "0x600000008000 - 0x600000009000"; 程序最后在 63 行调用函数建立 NEW VMA 虚拟区域，其区域范围是 "0x600000004000 - 0x600000005000". 三个虚拟区域各自相离，正好符合 "Merge Layout 0" 的空间布局. 69 到 70 行代码仅仅调试使用，接下分析内核对这种情况的处理逻辑.

###### 内核逻辑分析

采用本文提供的实践方法，可以通过跟踪 SYS_mmap 系统调用一起分析整个处理逻辑。如下图，在应用程序例子的 63 和 65 行分别加上 BiscuitOS 的调试开关，以此跟踪 64 行 SYS_mmap 系统调用.

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH000332.png)

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH000282.png)

当用户空间调用 SYS_mmap 系统调用进行映射，内核会按上图函数调用逻辑，最终调用掉 vma_merge() 函数，该函数就是本节重点讨论的地方。接下来分析 "Merge Layout 0" 情况下 vma_merge() 函数的处理逻辑.

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH000283.png)

当内核调用 vma_merge() 函数，参数 mm 指向进程地址空间，参数 prev 则指向 PREV, 参数 addr 和 end 指明了新建虚拟区域的范围，参数 vm_flags 指明了虚拟区域的标志集合,参数 anon_vma 指向了匿名映射相关的数据结构，参数 file 则指明了文件映射所打开的文件，参数与 pgoff 指明了新建虚拟区域对应的 pgoff, 参数 policy 则指明了内存分配策略，参数 vm_userfaultfd_ctx 则指向 userfaultfd 接口。

vma_merge() 函数首先在 1139 行计算新建虚拟区域需要占用物理页的数量，接着定义了两个局部变量 area 和 next。函数如果在 1147 行检测到新建虚拟区域的标志集合中包含了 VM_SPECIAL 标志，那么函数禁止合并操作直接退出。函数接着在 1150 行检测到 PREV 存在，那么将 next 指向 PREV 的下一个虚拟区域; 反之 PREV 不存在的话，那么函数直接将 next 指向了进程地址空间的第一个虚拟区域，即 mm->mmap 指向的虚拟区域。函数接着将 area 指向了 next 维护的 VMA，接着函数在 1155 行检测 area 如果存在，且 area 指向的 VMA 的结束地址是否与新建区域的结束地址重叠，那么函数将 next 指向下一个 VMA。通过测试用例可知，PREV 和 NEXT 皆存在，且 1153、1156 行代码均不会执行。函数继续在 1159 行检测 PREV 存在的情况下，新建虚拟区域的起始地址是否小于或等于 PREV 的起始地址，这种情况是不允许存在的，如果存在直接报错; 函数接着在 1160 行检测 NEXT 存在的情况下，新建虚拟区域的结束地址是否大于 NEXT 的结束地址，这种情况也是不允许的，如果存在则直接报错，最后函数在 1161 行检测到 addr 大于等于 end，也就是越界，那么函数页同样会报错。结合测试用例，上面三种情况都不会发生，接下来可以打印 PREV、NEXT 和新建区域的信息:

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH000284.png)

在 BiscuitOS 运行可以看出，PREV、NEXT 和新建区域在进行合并操作之前各自相离，符合 "Merge Layout 0" 的布局。

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH000333.png)

vma_merge() 函数接下来对处理进行分流，首先函数在 4 行判断 PREV 是否与新建区域相邻，根据测试实例可知，PREV 与新建区域相离，那么 5 到 33 行的代码分支直接跳过; 函数接下来在 38 行判断 NEXT 是否与新建区域相邻，根据测试实例可知，NEXT 与新建区域相离，那么 39 行到 60 行的代码分支将直接跳过，最后 vma_merge() 函数返回 NULL。通过上面的分析可以知道新建区域与 PREV 和 NEXT 相离的情况下，内核不进行任何合并操作，最后在 BiscuitOS 验证测试结果:

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH000334.png)

在 BiscuitOS 上将测试用例压后台运行，并通过测试用例的 PID 查看进程的地址空间，可以看到进程地址空间独立维护了 PREV、NEXT 和新建的虚拟区域。分析验证符合预期.

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND000100.png)

----------------------------------

#### <span id="B1">VMA adjoin with NEXT region Algorithem</span>

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH000336.png)

基于 "Merge Layout 0", PREV 与新建虚拟区域相离，因此 NEW VMA 不会与 PREV 合并; 另外 NEW VMA 与 NEXT 相邻，因此 NEW VMA 与 NEXT 会合并, 因此当 NEW VMA 插入进程之后，进程还是维护这两个虚拟区域。接下来通过一个实例进行讲解验证, 该实例可以在 BiscuitOS 进行实践部署，部署请参考:

{% highlight bash %}
cd BiscuitOS
make menuconfig

  [*] Package  --->
      [*] Memory Mapping Mechanism  --->
          [*] VMA Merge Region Algorithm  --->
              [*] Layout0: adjoin with NEXT region  --->

BiscuitOS/output/linux-XXXX/package/BiscuitOS-vma-merge-layout0-adjoin-default
{% endhighlight %}

> [VMA Merge Layout0: adjoin with NEXT region Source Code](https://gitee.com/BiscuitOS_team/HardStack/tree/Gitee/Memory-Allocator/MMAP/BiscuitOS-vma-merge/BiscuitOS-vma-merge-layout0-adjoin)
>
> [BiscuitOS 独立程序实践教程](https://biscuitos.github.io/blog/Human-Knowledge-Common/#C)

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH000337.png)

在本例中，程序通过匿名私有映射的方式，并指明映射的虚拟地址，建立了三个虚拟区域。程序在 59 行通过调用函数建立 PREV 虚拟区域，其区域范围是 "0x600000000000 - 0x600000001000"; 程序接着在 60 行调用函数建立 NEXT 虚拟区域，其区域范围是 "0x600000008000 - 0x600000009000"; 程序最后在 63 行调用函数建立 NEW VMA 虚拟区域，其区域范围是 "0x600000007000 - 0x600000008000". 新建区域与 PREV 相离，正好符合 "Merge Layout 0" 的空间布局，此时新建区域与 NEXT 区域相邻 (adjoin), 符合本节设定的内存布局. 69 到 70 行代码仅仅调试使用，接下分析内核对这种情况的处理逻辑.

###### 内核逻辑分析

采用本文提供的实践方法，可以通过跟踪 SYS_mmap 系统调用一起分析整个处理逻辑。如下图，在应用程序例子的 63 和 65 行分别加上 BiscuitOS 的调试开关，以此跟踪 64 行 SYS_mmap 系统调用.

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH000332.png)

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH000282.png)

当用户空间调用 SYS_mmap 系统调用进行映射，内核会按上图函数调用逻辑，最终调用掉 vma_merge() 函数，该函数就是本节重点讨论的地方。接下来分析 "Merge Layout 0" 情况下 vma_merge() 函数的处理逻辑.

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH000283.png)

当内核调用 vma_merge() 函数，参数 mm 指向进程地址空间，参数 prev 则指向 PREV, 参数 addr 和 end 指明了新建虚拟区域的范围，参数 vm_flags 指明了虚拟区域的标志集合,参数 anon_vma 指向了匿名映射相关的数据结构，参数 file 则指明了文件映射所打开的文件，参数与 pgoff 指明了新建虚拟区域对应的 pgoff, 参数 policy 则指明了内存分配策略，参数 vm_userfaultfd_ctx 则指向 userfaultfd 接口。

vma_merge() 函数首先在 1139 行计算新建虚拟区域需要占用物理页的数量，接着定义了两个局部变量 area 和 next。函数如果在 1147 行检测到新建虚拟区域的标志集合中包含了 VM_SPECIAL 标志，那么函数禁止合并操作直接退出。函数接着在 1150 行检测到 PREV 存在，那么将 next 指向 PREV 的下一个虚拟区域; 反之 PREV 不存在的话，那么函数直接将 next 指向了进程地址空间的第一个虚拟区域，即 mm->mmap 指向的虚拟区域。函数接着将 area 指向了 next 维护的 VMA，接着函数在 1155 行检测 area 如果存在，且 area 指向的 VMA 的结束地址是否与新建区域的结束地址重叠，那么函数将 next 指向下一个 VMA。通过测试用例可知，PREV 和 NEXT 皆存在，且 1153、1156 行代码均不会执行。函数继续在 1159 行检测 PREV 存在的情况下，新建虚拟区域的起始地址是否小于或等于 PREV 的起始地址，这种情况是不允许存在的，如果存在直接报错; 函数接着在 1160 行检测 NEXT 存在的情况下，新建虚拟区域的结束地址是否大于 NEXT 的结束地址，这种情况也是不允许的，如果存在则直接报错，最后函数在 1161 行检测到 addr 大于等于 end，也就是越界，那么函数页同样会报错。结合测试用例，上面三种情况都不会发生，接下来可以打印 PREV、NEXT 和新建区域的信息:

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH000338.png)

在 BiscuitOS 运行可以看出，PREV 和新建区域在进行合并操作之前各自相离，符合 "Merge Layout 0" 的布局, 此时新建区域与 NEXT 相邻，符合预期。

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH000333.png)

vma_merge() 函数接下来对处理进行分流，首先函数在 4 行判断 PREV 是否与新建区域相邻，根据测试实例可知，PREV 与新建区域相离，那么 5 到 33 行的代码分支直接跳过; 函数接下来在 38 行判断 NEXT 是否与新建区域相邻，根据测试实例可知，NEXT 与新建区域相邻，那么函数继续在 43 行检测 PREV 存在的情况下，新建区域的起始地址是否小于 PREV 的结束地址，由测试实例可以知道 PREV 存在且新建区域的起始地址大于 PREV 的终止地址，那么函数进入 46 行代码分支执行。函数在 47 行调用 \_\_vma_adjust() 函数进行实际的合并操作, 当合并操作完成之后，函数在 59 行返回了 NEXT。接下来分析 \_\_vma_adjust() 内部处理。

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH000339.png)

在本测试实例下调用 \_\_vma_adjust() 函数，参数 vma 指向 NEXT，参数 start 指向新建区域的起始地址，参数 end 指向 NEXT 的结束地址，参数 pgoff 指向 NEXT 的 pgoff 减去新建区域占用的页面数，参数 insert 为 NULL，参数 expand 指向 NEXT。函数在 719 行创建两个变量，其中 next 指向 NEXT 的下一个 VMA，参数 orig_vma 指向了 VMA.

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH000340.png) 

在本测试实例中，由于 next 指向了 NEXT 的下一个 VMA，而 end 指向了 NEXT 的结束地址，那么函数 4、50 以及 59 行的判断条件均不满足，并且 exporter 为 NULL，因此函数不会进入上图中任一分支执行。

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH000341.png)

在本测试实例中，由于采用匿名映射的方式申请内存，那么函数不会进入 816 行代码分支执行。函数接着在 835 行获得 NEXT 对应的 anon_vma 数据，但是由于测试程序位于 NEXT 虚拟区域进行访问，那么 NEXT 的 anon_vma 为空。函数在 836 行判断，由于 adjust_next 为 false，因此 837 行代码以及 839 行代码分支不会被执行。最后由于 root 此时也为空，那么 846 行代码也不会被执行。

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH000342.png)

在本测试实例中，start 指向了新建区域的起始地址，而此时 vma 指向 NEXT，因此 854 行条件成立，函数执行 855 行分支，函数在 855 行调整了 NEXT 的起始地址为新建区域的起始地址，然后又将 start_changed 设置为 true, 这里其实已经完成了核心的合并操作。函数接着在 858 行进行判断，有上面分析可知 end 与 vma 的 end 是同一个值，因此不会进行 859 行分支执行。函数在 862 行更新了 NEXT 的 pgoff，此时 pgoff 是在 NEXT pgoff 的基础上减去了新建区域占用的页面数，也就是将 NEXT 的 pgoff 设置为新建区域的 pgoff。由于 adjust_next 和 root 都为空，那么函数不会进入 864 和 869 分支进行执行。

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH000343.png)

在本测试实例中，由于 remove_next 和 insert 均为空，那么函数直接进入 903 行分支进行执行。由上面的分析可知 start_changed 为 true，而 end_changed 为 false，因此函数只进入 904 行分支执行，此时函数调用 vma_gap_update() 函数更新了 NEXT 的 GAP 信息。

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH000345.png)

在本测试实例中，由于为对 NEXT 虚拟区域进行访问，那么 anon_vma 和 mapping, 以及 root 均为空，那么函数均不会进入 914、920 以及 923 行代码分支执行.

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH000344.png)

在本测试实例中，通过上面的代码逻辑分析可以知道 remove_next 为 false，那么 3 行代码分支不会被执行。

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH000346.png)

在本测试实例中，insert 和 file 都为空，因此函数不会进入 996 行分支进行执行，函数最后在 998 行调用 validate_mm() 函数使进程地址空间有效. 至此 \_\_vma_adjust() 函数的代码逻辑已经处理完毕，VMA 合并逻辑也处理完成。。通过上面的分析可以知道新建区域与 PREV 相离，并与 NEXT 相邻的情况下，内核会将新建区域与 NEXT 合并成新的 NEXT ，而 PREV 保持不变，最后在 BiscuitOS 验证测试结果:

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH000347.png)

在 BiscuitOS 上将测试用例压后台运行，并通过测试用例的 PID 查看进程的地址空间，可以看到进程地址空间独立维护了 PREV, NEXT 的范围则包含了新建的虚拟区域。分析验证符合预期.

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND000100.png)

----------------------------------

#### <span id="B2">VMA intersect with NEXT region Algorithem</span>

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH000378.png)

基于 "Merge Layout 0", PREV 与新建虚拟区域相离，因此 NEW VMA 不会与 PREV 合并; 另外 NEW VMA 与 NEXT 相交，因此 NEW VMA 与 NEXT 会合并, 当 NEW VMA 插入进程之后，进程只会维护两个虚拟区域。接下来通过一个实例进行讲解验证, 该实例可以在 BiscuitOS 进行实践部署，部署请参考:

{% highlight bash %}
cd BiscuitOS
make menuconfig

  [*] Package  --->
      [*] Memory Mapping Mechanism  --->
          [*] VMA Merge Region Algorithm  --->
              [*] Layout0: intersect with NEXT region  --->

BiscuitOS/output/linux-XXXX/package/BiscuitOS-vma-merge-layout0-intersect-default
{% endhighlight %}

> [VMA Merge Layout0: intersect with NEXT region Source Code](https://gitee.com/BiscuitOS_team/HardStack/tree/Gitee/Memory-Allocator/MMAP/BiscuitOS-vma-merge/BiscuitOS-vma-merge-layout0-intersect)
>
> [BiscuitOS 独立程序实践教程](https://biscuitos.github.io/blog/Human-Knowledge-Common/#C)

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH000379.png)

在本例中，程序通过匿名私有映射的方式，并指明映射的虚拟地址，建立了三个虚拟区域。程序在 59 行通过调用函数建立 PREV 虚拟区域，其区域范围是 "0x600000000000 - 0x600000001000"; 程序接着在 60 行调用函数建立 NEXT 虚拟区域，其区域范围是 "0x600000008000 - 0x60000000c000"; 程序最后在 63 行调用函数建立 NEW VMA 虚拟区域，其区域范围是 "0x600000007000 - 0x600000009000". 新建区域与 PREV 相离，正好符合 "Merge Layout 0" 的空间布局，此时新建区域与 NEXT 区域相交 (intersect), 符合本节设定的内存布局. 69 到 70 行代码仅仅调试使用，接下分析内核对这种情况的处理逻辑.

###### 内核逻辑分析

采用本文提供的实践方法，可以通过跟踪 SYS_mmap 系统调用一起分析整个处理逻辑。如下图，在应用程序例子的 63 和 65 行分别加上 BiscuitOS 的调试开关，以此跟踪 64 行 SYS_mmap 系统调用.

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH000332.png)

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH000282.png)

当用户空间调用 SYS_mmap 系统调用进行映射，内核会按上图函数调用逻辑，最终调用掉 vma_merge() 函数，该函数就是本节重点讨论的地方。接下来分析 "Merge Layout 0" 情况下 vma_merge() 函数的处理逻辑.

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH000283.png)

当内核调用 vma_merge() 函数，参数 mm 指向进程地址空间，参数 prev 则指向 PREV, 参数 addr 和 end 指明了新建虚拟区域的范围，参数 vm_flags 指明了虚拟区域的标志集合,参数 anon_vma 指向了匿名映射相关的数据结构，参数 file 则指明了文件映射所打开的文件，参数与 pgoff 指明了新建虚拟区域对应的 pgoff, 参数 policy 则指明了内存分配策略，参数 vm_userfaultfd_ctx 则指向 userfaultfd 接口。

vma_merge() 函数首先在 1139 行计算新建虚拟区域需要占用物理页的数量，接着定义了两个局部变量 area 和 next。函数如果在 1147 行检测到新建虚拟区域的标志集合中包含了 VM_SPECIAL 标志，那么函数禁止合并操作直接退出。函数接着在 1150 行检测到 PREV 存在，那么将 next 指向 PREV 的下一个虚拟区域; 反之 PREV 不存在的话，那么函数直接将 next 指向了进程地址空间的第一个虚拟区域，即 mm->mmap 指向的虚拟区域。函数接着将 area 指向了 next 维护的 VMA，接着函数在 1155 行检测 area 如果存在，且 area 指向的 VMA 的结束地址是否与新建区域的结束地址重叠，那么函数将 next 指向下一个 VMA。通过测试用例可知，PREV 和 NEXT 皆存在，且 1153、1156 行代码均不会执行。函数继续在 1159 行检测 PREV 存在的情况下，新建虚拟区域的起始地址是否小于或等于 PREV 的起始地址，这种情况是不允许存在的，如果存在直接报错; 函数接着在 1160 行检测 NEXT 存在的情况下，新建虚拟区域的结束地址是否大于 NEXT 的结束地址，这种情况也是不允许的，如果存在则直接报错，最后函数在 1161 行检测到 addr 大于等于 end，也就是越界，那么函数页同样会报错。结合测试用例，上面三种情况都不会发生，接下来可以打印 PREV、NEXT 和新建区域的信息:

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH000380.png)

在 BiscuitOS 运行可以看出，PREV 和新建区域在进行合并操作之前各自相离，符合 "Merge Layout 0" 的布局, 此时新建区域与 NEXT 相交，符合预期。

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH000333.png)

vma_merge() 函数接下来对处理进行分流，首先函数在 4 行判断 PREV 是否与新建区域相邻，根据测试实例可知，PREV 与新建区域相离，那么 5 到 33 行的代码分支直接跳过; 函数接下来在 38 行判断 NEXT 是否与新建区域相邻，根据测试实例可知，NEXT 与新建区域相邻，那么函数继续在 43 行检测 PREV 存在的情况下，新建区域的起始地址是否小于 PREV 的结束地址，由测试实例可以知道 PREV 存在且新建区域的起始地址大于 PREV 的终止地址，那么函数进入 46 行代码分支执行。函数在 47 行调用 \_\_vma_adjust() 函数进行实际的合并操作, 当合并操作完成之后，函数在 59 行返回了 NEXT。接下来分析 \_\_vma_adjust() 内部处理。

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH000339.png)

在本测试实例下调用 \_\_vma_adjust() 函数，参数 vma 指向 NEXT，参数 start 指向新建区域的起始地址，参数 end 指向 NEXT 的结束地址，参数 pgoff 指向 NEXT 的 pgoff 减去新建区域占用的页面数，参数 insert 为 NULL，参数 expand 指向 NEXT。函数在 719 行创建两个变量，其中 next 指向 NEXT 的下一个 VMA，参数 orig_vma 指向了 VMA.

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH000340.png) 

在本测试实例中，由于 next 指向了 NEXT 的下一个 VMA，而 end 指向了 NEXT 的结束地址，那么函数 4、50 以及 59 行的判断条件均不满足，并且 exporter 为 NULL，因此函数不会进入上图中任一分支执行。

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH000341.png)

在本测试实例中，由于采用匿名映射的方式申请内存，那么函数不会进入 816 行代码分支执行。函数接着在 835 行获得 NEXT 对应的 anon_vma 数据，但是由于测试程序位于 NEXT 虚拟区域进行访问，那么 NEXT 的 anon_vma 为空。函数在 836 行判断，由于 adjust_next 为 false，因此 837 行代码以及 839 行代码分支不会被执行。最后由于 root 此时也为空，那么 846 行代码也不会被执行。

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH000342.png)

在本测试实例中，start 指向了新建区域的起始地址，而此时 vma 指向 NEXT，因此 854 行条件成立，函数执行 855 行分支，函数在 855 行调整了 NEXT 的起始地址为新建区域的起始地址，然后又将 start_changed 设置为 true, 这里其实已经完成了核心的合并操作。函数接着在 858 行进行判断，有上面分析可知 end 与 vma 的 end 是同一个值，因此不会进行 859 行分支执行。函数在 862 行更新了 NEXT 的 pgoff，此时 pgoff 是在 NEXT pgoff 的基础上减去了新建区域占用的页面数，也就是将 NEXT 的 pgoff 设置为新建区域的 pgoff。由于 adjust_next 和 root 都为空，那么函数不会进入 864 和 869 分支进行执行。

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH000343.png)

在本测试实例中，由于 remove_next 和 insert 均为空，那么函数直接进入 903 行分支进行执行。由上面的分析可知 start_changed 为 true，而 end_changed 为 false，因此函数只进入 904 行分支执行，此时函数调用 vma_gap_update() 函数更新了 NEXT 的 GAP 信息。

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH000345.png)

在本测试实例中，由于为对 NEXT 虚拟区域进行访问，那么 anon_vma 和 mapping, 以及 root 均为空，那么函数均不会进入 914、920 以及 923 行代码分支执行.

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH000344.png)

在本测试实例中，通过上面的代码逻辑分析可以知道 remove_next 为 false，那么 3 行代码分支不会被执行。

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH000346.png)

在本测试实例中，insert 和 file 都为空，因此函数不会进入 996 行分支进行执行，函数最后在 998 行调用 validate_mm() 函数使进程地址空间有效. 至此 \_\_vma_adjust() 函数的代码逻辑已经处理完毕，VMA 合并逻辑也处理完成。。通过上面的分析可以知道新建区域与 PREV 相离，并与 NEXT 相邻的情况下，内核会将新建区域与 NEXT 合并成新的 NEXT ，而 PREV 保持不变，最后在 BiscuitOS 验证测试结果:

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH000347.png)

在 BiscuitOS 上将测试用例压后台运行，并通过测试用例的 PID 查看进程的地址空间，可以看到进程地址空间独立维护了 PREV, NEXT 的范围则包含了新建的虚拟区域。分析验证符合预期.

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND000100.png)

----------------------------------

<span id="C"></span>

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND00000T.jpg)

#### Merge Layout 1: VMA adjoin with PREV region Algorithm

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH000349.png)

当一个新建的虚拟区域 NEW VMA 准备加入进程时，其区域正好前一个虚拟区域 PREV 相邻，那么此时将这种虚拟区域布局定义为 "Merge Layout 1". 在这种情况下，PREV 的 NEXT 区域可能存在，也可能不存在。如果 NEXT 存在，那么新建区域可能和 NEXT 存在多种位置关系，假设新建区域与其他 VMA 区域符合合并的条件。那么接下来分别讨论不同位置关系>下的合并算法:

> - [VMA disconnect with NEXT region (or NEXT doesn't exist) Algorithem](#C0)
>
> - [VMA adjoin with NEXT region Algorithm](#C1)
>
> - [VMA intersect with NEXT region Algorithm](#C2)
>
> - [VMA cover with NEXT region Algorithm](#C3)

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND000100.png)

----------------------------------

#### <span id="C0">VMA disconnect with NEXT region Algorithem</span>

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH000350.png)

基于 "Merge Layout 1", PREV 与新建虚拟区域相邻，因此 NEW VMA 会与 PREV 合并; 另外 NEW VMA 与 NEXT 相离，因此 NEW VMA 与 NEXT 也不会合并, 当 NEW VMA 插入进程之后，进程保持维护这两个虚拟区域。接下来通过一个实例进行讲解验证, 该实例可以在 BiscuitOS 进行实践部署，部署请参考:

{% highlight bash %}
cd BiscuitOS
make menuconfig

  [*] Package  --->
      [*] Memory Mapping Mechanism  --->
          [*] VMA Merge Region Algorithm  --->
              [*] Layout1: disconnect with NEXT region  --->

BiscuitOS/output/linux-XXXX/package/BiscuitOS-vma-merge-layout1-disconnect-default
{% endhighlight %}

> [VMA Merge Layout1: disconnect with NEXT region Source Code](https://gitee.com/BiscuitOS_team/HardStack/tree/Gitee/Memory-Allocator/MMAP/BiscuitOS-vma-merge/BiscuitOS-vma-merge-layout1-disconnect)
>
> [BiscuitOS 独立程序实践教程](https://biscuitos.github.io/blog/Human-Knowledge-Common/#C)

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH000351.png)

在本例中，程序通过匿名私有映射的方式，并指明映射的虚拟地址，建立了三个虚拟区域。程序在 59 行通过调用函数建立 PREV 虚拟区域，其区域范围是 "0x600000000000 - 0x600000001000"; 程序接着在 60 行调用函数建立 NEXT 虚拟区域，其区域范围是 "0x600000008000 - 0x600000009000"; 程序最后在 63 行调用函数建立 NEW VMA 虚拟区域，其区域范围是 "0x600000001000 - 0x600000002000". 新建区域与 PREV 相邻，正好符合 "Merge Layout 1" 的空间布局，此时新建区域与 NEXT 区域相离 (disconnect), 符合本节设定的内存布局. 69 到 70 行代码仅仅调试使用，接下分析内核对这种情况的处理逻辑.

###### 内核逻辑分析

采用本文提供的实践方法，可以通过跟踪 SYS_mmap 系统调用一起分析整个处理逻辑。如下图，在应用程序例子的 63 和 65 行分别加上 BiscuitOS 的调试开关，以此跟踪 64 行 SYS_mmap 系统调用.

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH000332.png)

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH000282.png)

当用户空间调用 SYS_mmap 系统调用进行映射，内核会按上图函数调用逻辑，最终调用掉 vma_merge() 函数，该函数就是本节重点讨论的地方。接下来分析 "Merge Layout 1" 情况下 vma_merge() 函数的处理逻辑.

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH000283.png)

当内核调用 vma_merge() 函数，参数 mm 指向进程地址空间，参数 prev 则指向 PREV, 参数 addr 和 end 指明了新建虚拟区域的范围，参数 vm_flags 指明了虚拟区域的标志集合,参数 anon_vma 指向了匿名映射相关的数据结构，参数 file 则指明了文件映射所打开的文件，参数与 pgoff 指明了新建虚拟区域对应的 pgoff, 参数 policy 则指明了内存分配策略，参数 vm_userfaultfd_ctx 则指向 userfaultfd 接口。

vma_merge() 函数首先在 1139 行计算新建虚拟区域需要占用物理页的数量，接着定义了两个局部变量 area 和 next。函数如果在 1147 行检测到新建虚拟区域的标志集合中包含了 VM_SPECIAL 标志，那么函数禁止合并操作直接退出。函数接着在 1150 行检测到 PREV 存在，那么将 next 指向 PREV 的下一个虚拟区域; 反之 PREV 不存在的话，那么函数直接将 next 指向了进程地址空间的第一个虚拟区域，即 mm->mmap 指向的虚拟区域。函数接着将 area 指向了 next 维护的 VMA，接着函数在 1155 行检测 area 如果存在，且 area 指向的 VMA 的结束地址是否与新建区域的结束地址重叠，那么函数将 next 指向下一个 VMA。通过测试用例可知，PREV 和 NEXT 皆存在，且 1153、1156 行代码均不会执行。函数继续在 1159 行检测 PREV 存在的情况下，新建虚拟区域的起始地址是否小于或等于 PREV 的起始地址，这种情况是不允许存在的，如果存在直接报错; 函数接着在 1160 行检测 NEXT 存在的情况下，新建虚拟区域的结束地址是否大于 NEXT 的结束地址，这种情况也是不允许的，如果存在则直接报错，最后函数在 1161 行检测到 addr 大于等于 end，也就是越界，那么函数页同样会报错。结合测试用例，上面三种情况都不会发生，接下来可以打印 PREV、NEXT 和新建区域的信息:

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH000352.png)

在 BiscuitOS 运行可以看出，PREV 和新建区域在进行合并操作之前相邻，符合 "Merge Layout 1" 的布局, 此时新建区域与 NEXT 相离，符合预期。

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH000333.png)

vma_merge() 函数接下来对处理进行分流，首先函数在 4 行判断 PREV 是否与新建区域相邻，根据测试实例可知，PREV 与新建区域相邻，那么函数继续在 12 行进行判断，如果 NEXT 存在的情况下新建区域的结束地址是否等于 NEXT 的起始地址。根据本测试实例可以知道新建区域与 NEXT 相离，因此函数跳转到 25 行分支执行。函数在 25 行调用 \_\_vma_adjust() 函数进行实际的合并操作, 当合并操作完成之后，函数在 32 行返回 PREV。接下来分析 \_\_vma_adjust() 内部处理。

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH000339.png)

在本测试实例下调用 \_\_vma_adjust() 函数，参数 vma 指向 PREV，参数 start 指向 PREV 的起始地址，参数 end 指向新建区域的结束地址，参数 pgoff 指向 PREV 的 pgoff，参数 insert 为 NULL，参数 expand 指向 PREV。函数在 719 行创建两个变量，其中 next 指向 NEXT，参数 orig_vma 指向了 PREV.

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH000340.png) 

在本测试实例中，由于 next 指向了 NEXT，而 end 指向了新建区域的结束地址，那么函数 4、50 以及 59 行的判断条件均不满足，并且 exporter 为 NULL，因此函数不会进入上图中任一分支执行。

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH000341.png)

在本测试实例中，由于采用匿名映射的方式申请内存，那么函数不会进入 816 行代码分支执行。函数接着在 835 行获得 PREV 对应的 anon_vma 数据，但是由于测试程序位于 PREV 虚拟区域进行访问，那么 PREV 的 anon_vma 为空。函数在 836 行判断，由于 adjust_next 为 false，因此 837 行代码以及 839 行代码分支不会被执行。最后由于 root 此时也为空，那么 846 行代码也不会被执行。

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH000342.png)

在本测试实例中，start 指向了 PREV 的起始地址，而此时 vma 指向 PREV，因此 854 行条件不成立，函数不执行 855 行分支。由于 end 指向新建区域的结束地址，此时 858 条件成立，那么函数进入 859 行分支执行，函数将 PREV 的结束地址设置为新建区域的结束地址，并将 end_changed 设置为 true，这里其实已经完成了核心的合并操作。函数在 862 行更新了 NEXT 的 pgoff，此时 pgoff 是在 PREV pgoff。由于 adjust_next 和 root 都为空，那么函数不会进入 864 和 869 分支进行执行。

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH000343.png)

在本测试实例中，由于 remove_next 和 insert 均为空，那么函数直接进入 903 行分支进行执行。由上面的分析可知 start_changed 为 false，而 end_changed 为 true，并且 next 不为空，因此函数只进入 909 行分支执行，此时函数调用 vma_gap_update() 函数更新了 NEXT 的 GAP 信息。

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH000345.png)

在本测试实例中，由于为对 NEXT 虚拟区域进行访问，那么 anon_vma 和 mapping, 以及 root 均为空，那么函数均不会进入 914、920 以及 923 行代码分支执行.

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH000344.png)

在本测试实例中，通过上面的代码逻辑分析可以知道 remove_next 为 false，那么 3 行代码分支不会被执行。

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH000346.png)

在本测试实例中，insert 和 file 都为空，因此函数不会进入 996 行分支进行执行，函数最后在 998 行调用 validate_mm() 函数使进程地址空间有效. 至此 \_\_vma_adjust() 函数的代码逻辑已经处理完毕，VMA 合并逻辑也处理完成。通过上面的分析可以知道新建区域与 PREV 相邻，并与 NEXT 相离的情况下，内核会将新建区域与 PREV 合并成新的 PREV，而 NEXT 保持不变，最后在 BiscuitOS 验证测试结果:

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH000353.png)

在 BiscuitOS 上将测试用例压后台运行，并通过测试用例的 PID 查看进程的地址空间，可以看到进程地址空间独立维护了 NEXT, PREV 的范围则包含了新建的虚拟区域。分析验证符合预期.

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND000100.png)


----------------------------------

#### <span id="C1">VMA adjoin with NEXT region Algorithem</span>

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH000354.png)

基于 "Merge Layout 1", PREV 与新建虚拟区域相邻，因此 NEW VMA 不会与 PREV 合并; 另外 NEW VMA 与 NEXT 相邻，因此 NEW VMA 与 NEXT 也会合并, 当 NEW VMA 插入进程之后，进程只需维护一个虚拟区域。接下来通过一个实例进行讲解验证, 该实例可以在 BiscuitOS 进行实践部署，部署请参考:

{% highlight bash %}
cd BiscuitOS
make menuconfig

  [*] Package  --->
      [*] Memory Mapping Mechanism  --->
          [*] VMA Merge Region Algorithm  --->
              [*] Layout1: adjoin with NEXT region  --->

BiscuitOS/output/linux-XXXX/package/BiscuitOS-vma-merge-layout1-adjoin-default
{% endhighlight %}

> [VMA Merge Layout1: adjoin with NEXT region Source Code](https://gitee.com/BiscuitOS_team/HardStack/tree/Gitee/Memory-Allocator/MMAP/BiscuitOS-vma-merge/BiscuitOS-vma-merge-layout1-adjoin)
>
> [BiscuitOS 独立程序实践教程](https://biscuitos.github.io/blog/Human-Knowledge-Common/#C)

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH000355.png)

在本例中，程序通过匿名私有映射的方式，并指明映射的虚拟地址，建立了三个虚拟区域。程序在 59 行通过调用函数建立 PREV 虚拟区域，其区域范围是 "0x600000000000 - 0x600000001000"; 程序接着在 60 行调用函数建立 NEXT 虚拟区域，其区域范围是 "0x600000002000 - 0x600000003000"; 程序最后在 63 行调用函数建立 NEW VMA 虚拟区域，其区域范围是 "0x600000001000 - 0x600000002000". 新建区域与 PREV 相邻，正好符合 "Merge Layout 1" 的空间布局，此时新建区域与 NEXT 区域相邻 (adjoin), 符合本节设定的内存布局. 69 到 70 行代码仅仅调试使用，接下分析内核对这种情况的处理逻辑.

###### 内核逻辑分析

采用本文提供的实践方法，可以通过跟踪 SYS_mmap 系统调用一起分析整个处理逻辑。如下图，在应用程序例子的 63 和 65 行分别加上 BiscuitOS 的调试开关，以此跟踪 64 行 SYS_mmap 系统调用.

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH000332.png)

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH000282.png)

当用户空间调用 SYS_mmap 系统调用进行映射，内核会按上图函数调用逻辑，最终调用掉 vma_merge() 函数，该函数就是本节重点讨论的地方。接下来分析 "Merge Case1" 情况下 vma_merge() 函数的处理逻辑.

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH000283.png)

当内核调用 vma_merge() 函数，参数 mm 指向进程地址空间，参数 prev 则指向 PREV, 参数 addr 和 end 指明了新建虚拟区域的范围，参数 vm_flags 指明了虚拟区域的标志集合,参数 anon_vma 指向了匿名映射相关的数据结构，参数 file 则指明了文件映射所打开的文件，参数与 pgoff 指明了新建虚拟区域对应的 pgoff, 参数 policy 则指明了内存分配策略，参数 vm_userfaultfd_ctx 则指向 userfaultfd 接口。

vma_merge() 函数首先在 1139 行计算新建虚拟区域需要占用物理页的数量，接着定义了两个局部变量 area 和 next。函数如果在 1147 行检测到新建虚拟区域的标志集合中包含了 VM_SPECIAL 标志，那么函数禁止合并操作直接退出。函数接着在 1150 行检测到 PREV 存在，那么将 next 指向 PREV 的下一个虚拟区域; 反之 PREV 不存在的话，那么函数直接将 next 指向了进程地址空间的第一个虚拟区域，即 mm->mmap 指向的虚拟区域。函数接着将 area 指向了 next 维护的 VMA，接着函数在 1155 行检测 area 如果存在，且 area 指向的 VMA 的结束地址是否与新建区域的结束地址重叠，那么函数将 next 指向下一个 VMA。通过测试用例可知，PREV 和 NEXT 皆存在，且 1153、1156 行代码均不会执行。函数继续在 1159 行检测 PREV 存在的情况下，新建虚拟区域的起始地址是否小于或等于 PREV 的起始地址，这种情况是不允许存在的，如果存在直接报错; 函数接着在 1160 行检测 NEXT 存在的情况下，新建虚拟区域的结束地址是否大于 NEXT 的结束地址，这种情况也是不允许的，如果存在则直接报错，最后函数在 1161 行检测到 addr 大于等于 end，也就是越界，那么函数页同样会报错。结合测试用例，上面三种情况都不会发生，接下来可以打印 PREV、NEXT 和新建区域的信息:

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH000356.png)

在 BiscuitOS 运行可以看出，PREV 和新建区域在进行合并操作之前相邻，符合 "Merge Layout 1" 的布局, 此时新建区域与 NEXT 相邻，符合预期.

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH000333.png)

vma_merge() 函数接下来对处理进行分流，首先函数在 4 行判断 PREV 是否与新建区域相邻，根据测试实例可知，PREV 与新建区域相邻，因此函数继续在 12 行进行判断，如果在 NEXT 存在的情况下，新建区域的结束地址是否等于 NEXT 的起始地址，有本测试实例可以知道该条件符合，那么函数进入 21 行分支进行执行。函数在 21 行通过调用 \_\_vma_adjust() 函数进行实际的合并操作，合并完成之后函数在 32 行返回 PREV. 接下来分析 \_\_vma_adjust() 内部处理。

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH000339.png)

在本测试实例下调用 \_\_vma_adjust() 函数，参数 vma 指向 PREV，参数 start 指向 PREV 的起始地址，参数 end 指向 NEXT 的结束地址，参数 pgoff 指向 PREV 的 pgoff，参数 insert 为 NULL，参数 expand 指向 PREV。函数在 719 行创建两个变量，其中 next 指向 NEXT，参数 orig_vma 指向了 PREV.

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH000340.png) 

在本测试实例中，由于 next 指向了 NEXT，而 end 指向了 NEXT 的结束地址，那么函数满足 4 行的条件，函数进入 4 行分支，函数继续在 11 行进行检测，由于 expand 指向 PREV，因此函数直接进入 26 行分支进行执行。由于 expand 等于 PREV，vma 也指向 PREV，因此 26 行不会产生报警。函数在 31 行用于计算 remove_next 的数量，当 end 大于 NEXT 的结束地址，那么 remove_next 的数量为 2，否则为 1，通过本实例可以知道此时 end 等于 next->vm_end, 因此 remove_next 的数量为 1. 函数接着对 remove_next 的有效性进行检测，如果 remove_next 的数量为 2，但 end 不等于 NEXT 的下一个 VMA 的结束地址，那么这种条件不符合要求，如果符合，那么这是新建区域 "Cover Next Regions". 函数接着在 34 行检测，如果 remove_next 为 1 时，如果 end 不等于 NEXT 的结束地址，由本实例可以知道 end 等于 NEXT 的结束地址，因此不会触发报警，最后函数将 end 设置为 "next->vm_end". 函数接着将 exporter 指向 NEXT，而将 import 指向了 PREV。由于此时 remove_next 为 1，因此函数不会进入 48 行分支。最后由于 PREV 和 NEXT 区域均为访问，因此不存在 anon_vma, 那么函数不会进入 77 行分支进行执行。

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH000341.png)

在本测试实例中，由于采用匿名映射的方式申请内存，那么函数不会进入 816 行代码分支执行。函数接着在 835 行获得 PREV 对应的 anon_vma 数据，但是由于测试程序未对 PREV 虚拟区域进行访问，那么 PREV 的 anon_vma 为空。函数在 836 行判断，由于 adjust_next 为 false，因此 837 行代码以及 839 行代码分支不会被执行。最后由于 root 此时也为空，那么 846 行代码也不会被执行。

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH000342.png)

在本测试实例中，start 指向了 PREV 的起始地址，而此时 vma 指向 PREV，因此 854 行条件不成立，函数不执行 855 行分支。函数接着在 858 行进行判断，由上面分析可知 end 与 vma 的 vm_end 不是同一个值，因此函数进入 859 行分支。函数在 859 行将 PREV 的结束地址设置为 end 即 NEXT 的结束地址，并将 end_changed 设置为 true, 这里其实已经完成了核心的合并操作。函数在 862 行更新了 PREV 的 pgoff，此时 pgoff 是 PREV pgoff。由于 adjust_next 和 root 都为空，那么函数不会进入 864 和 869 分支进行执行。

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH000343.png)

在本测试实例中，由于 remove_next 的值为 1，那么函数进入 876 行分支执行。函数在 880 行判断 remove_next 的值是否为 3，由上面的分析可知，函数此时进入 881 行分支，调用 \_\_vma_unlink_prev() 函数，该函数用于将 NEXT 从进程地址空间维护的红黑树和链表中将 NEXT 节点移除。由于本实例采用匿名映射，因此函数不会执行 894 行代码分支。

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH000345.png)

在本测试实例中，由于为对 NEXT 虚拟区域进行访问，那么 anon_vma 和 mapping, 以及 root 均为空，那么函数均不会进入 914、920 以及 923 行代码分支执行.

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH000344.png)

在本测试实例中，通过上面的代码逻辑分析可以知道 remove_next 为 1，那么函数进入 3 行分支进行执行。由于本示例采用匿名映射，并未对虚拟区域进行访问，因此函数不会进入 4 和 8 行的代码分支。函数在 9 行更新了进程地址空间映射区域数量，将其减一。函数接着在 10 行调用 mpol_put() 释放 NEXT 占用的内存分配策略，最后函数在 11 行调用 vm_area_free() 函数彻底释放 NEXT。函数接下来在 17 再次对 remove_next 的数量是否为 3 进行判断，此时 remove_next 为 1，那么函数进入 24 行分支，此时将 next 指向 PREV 新的下一个 VMA。由于 remove_next 为 1 的缘故，如果此时 next 不为空，那么函数进入 44 行分支执行，此时函数滴啊用 vma_gap_update() 函数更新了 next 的 GAP 信息。

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH000346.png)

在本测试实例中，insert 和 file 都为空，因此函数不会进入 996 行分支进行执行，函数最后在 998 行调用 validate_mm() 函数使进程地址空间有效. 至此 \_\_vma_adjust() 函数的代码逻辑已经处理完毕，VMA 合并逻辑也处理完成。通过上面的分析可以知道新建区域与 PREV 相邻，并与 NEXT 相邻的情况下，内核会将新建区域和 NEXT 合并到 PREV，最后在 BiscuitOS 验证测试结果:

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH000357.png)

在 BiscuitOS 上将测试用例压后台运行，并通过测试用例的 PID 查看进程的地址空间，可以看到进程地址空间最后只维护了 PREV。分析验证符合预期.

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND000100.png)

-----------------------------------------------

#### <span id="Z0">附录</span>

> [BiscuitOS Home](https://biscuitos.github.io/)
>
> [BiscuitOS Blog 2.0](https://biscuitos.github.io/blog/BiscuitOS_Catalogue/)
>
> [Linux Kernel](https://www.kernel.org/)
>
> [Bootlin: Elixir Cross Referencer](https://elixir.bootlin.com/linux/latest/source)
>

#### 捐赠一下吧 🙂

![MMU](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/HAB000036.jpg)

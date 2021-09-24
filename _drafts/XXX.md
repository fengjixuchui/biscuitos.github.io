---
layout: post
title:  "VMA merge Algorithm"
date:   2020-10-01 06:00:00 +0800
categories: [HW]
excerpt: MMAP.
tags:
  - VMA merge
---

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND00000L0.PNG)

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/RPI/RPI100100.png)

#### 目录

> - [HKC 计划原理](#A)
>
> - [附录/捐赠](#Z0)

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND000100.png)

----------------------------------

<span id="A0"></span>

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND00000H.jpg)

#### Merge Case1

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH000279.png)

当一个新建的虚拟区域 VMA 加入进程时，如果此时新建的虚拟区域的范围正好位于 PREV 和 NEXT 区域中间，正好填补了 "空洞"，那么此时 PREV 虚拟区域的结束地址等于新建虚拟区域的起始地址，且 NEXT 虚拟区域的起始地址等于新建虚拟区域的终止地址，如上图所示, 此时我们称这种情况为 "Merge Case1". 

新建虚拟区域在 "Merge Case1" 的情况下进行合并，可能出现多种情况，例如三个虚拟区域合并为一个虚拟区域，或者合并为两个虚拟区域，或者三个区域都不能合并。本节继续研究 "Merge Case1" 情况下的合并算法，具体如下:

> - [三个虚拟区域合并为一个](#A001)
>
> - [三个虚拟区域合并为两个区域](#A002)
>
>   - [Merge Triple Areas to Double Areas (Merge PREV)](#A0020)
>
>   - [Merge Triple Areas to Double Areas (Merge NEXT)](#A0021)
>
> - [三个虚拟区域不能合并](#A003)

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND000100.png)

----------------------------------

#### <span id="A001">三个虚拟区域合并为一个</span>

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH000278.png)

虚拟区域要合并的前提是采用一致的映射方式、采用一致的内存策略、以及虚拟区域支持合并功能. 在 "Merge Case1" 情况下三个区域需要合并为一个虚拟区域，那么三个区域必须满足上面的条件。首先通过一个实例进行讲解, 该实例可以在 BiscuitOS 进行实践部署，部署请参考:

{% highlight bash %}
cd BiscuitOS
make menuconfig

  [*] Package  --->
      [*] Memory Mapping Mechanism  --->
          [*] VMA Merge Region Algorithm  --->
              [*] Case1: Merge Triple Areas to Single Area  --->

BiscuitOS/output/linux-XXXX/package/BiscuitOS-vma-merge-case1-single-default
{% endhighlight %}

> [VMA Merge Case1 Single Source Code](https://gitee.com/BiscuitOS_team/HardStack/tree/Gitee/Memory-Allocator/MMAP/BiscuitOS-vma-merge/BiscuitOS-vma-merge-case1-single)
>
> [BiscuitOS 独立程序实践教程](https://biscuitos.github.io/blog/Human-Knowledge-Common/#C)

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH000280.png)

在本例中，程序通过匿名私有映射的方式，并指定了映射的虚拟地址，这三次映射的虚拟区域的关系正好符合 "Merge Case1" 的情况，程序先在 59 和 60 行构造了两个虚拟区域，这两个虚拟区域分别形成了 PREV 和 NEXT 区域，接着在 63 行再次进行映射，此时新建的区域正好填补了 PREV 和 NEXT 之间的空洞，此时内核会将三个区域进行合并。例子到这里已经符合调试分析的要求。

在本例例子中，PREV 区域的虚拟范围是 "0x600000000000 - 0x600000001000", NEXT 区域的范围是 "0x600000002000 - 0x600000003000", 新建区域的范围是 "0x600000001000 - 0x600000002000". 三个区域首先在范围上符合了 "Merge Case1" 的布局，其次三个区域都采用同样的映射方式和映射标志，均为 PROT_WRITE 和 PROT_READ. 最后就是对于同一个进程，其内存分配策略是一致的，因此三个虚拟区域可以合并为一个内存区域.

###### 内核逻辑分析

采用本文提供的实践方法，可以通过跟踪 SYS_mmap 系统调用一起分析整个处理逻辑。如下图，在应用程序例子的 63 和 65 行分别加上 BiscuitOS 的调试开关，以此跟踪 64 行 SYS_mmap 系统调用.

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH000281.png)

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH000282.png)

当用户空间调用 SYS_mmap 系统调用进行映射，内核会按上图函数调用逻辑，最终调用掉 vma_merge() 函数，该函数就是本节重点讨论的地方。接下来分析 "Merge Case1" 情况下 vma_merge() 函数的处理逻辑.

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH000283.png)

vma_merge() 函数通过 1150 到 1161 行获得了 PREV 和 NEXT 虚拟区域，并且与新建虚拟区域地址进行检测，以此确认满足 "Merge Case1" 的条件，此时开发者可以按 1163 到 1165 行添加调试信息，以此在 BiscuitOS 运行查看运行结果:

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH000284.png)

测试用例在 BsicuitOS 运行之后，内核打印相关的日志，从日志可以看出此时 PREV 和 NEXT 虚拟区域，以及新建区域符合 "Merge Case1" 情况。

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH000285.png)

在当前情况下，vma_merge() 函数在 1166 行进行判断，此时 PREV 存在，且 PREV 的结束地址等于新建虚拟区的起始地址，由于 PREV 和新建虚拟区的内存分配策略相同，最后调用了 can_vma_merge_after() 函数检测了 PREV 和新建虚拟区的映射方式和标志等是否相同，此时是相同的，因此函数进入到 1171 行继续执行，此时函数在 1174 行同样检测 NEXT 和新建虚拟区是否可以重合，此处多了一个检测，函数调用 is_mergable_anon_vma() 函数对匿名映射进行了更多的判断，如果此时 PREV 和 NEXT 匿名映射了不同的物理页，那么不能进行合并。由于本案例中 PREV 和 NEXT 并未发生缺页，因此 PREV 和 NEXT 可以进行合并，因此函数在 1183 行调用 \_\_vma_adjust() 函数将 PREV、NEXT 和新建的虚拟区域进行合并。

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH000286.png)

函数在调用 \_\_vma_adjust() 函数的时候，vma 参数指向了 PREV, start 参数指向了 PREV 的起始地址, end 指向了 NEXT 的终止地址, 由于是匿名映射，因此 pgoff 指向了 PREV 的 pgoff，insert 参数为空，而 expand 参数指向 PREV。函数在 719 行定义了局部变量 next 指向了 NEXT, orig_vma 变量则指向 PREV.

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH000287.png)

在当前情况下, end 参数等于 next 指向虚拟区域的结束地址，因此函数进入 731 行逻辑。此时 PREV 和 NEXT 将会合并，那么系统继续保留 PREV, 而将 NEXT 进行移除，因此函数会跳入 752 至 764 行执行，以此计算移除 NEXT 区域的数量，以及合并之后虚拟区域的结束地址。函数在 767 行将 exporter 变量指向了 NEXT, 并将 importer 变量指向了 PREV. 此时由于 remove_next 为 1，因此 774 行判断无效。

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH000288.png)

在本案例中，由于 PREV 和 NEXT 通过匿名映射获得的虚拟区域，当未访问过，因此其对应的 anon_vma 则不存在，那么 836 到 845 行的逻辑不执行。函数在 854 行对 start 与 PREV 的起始地址进行比较，此时两者相同。函数接着在 858 行进行检测，此时 end 不等于 NEXT 的结束地址，因此函数会进入 859 行执行，此时函数将 PREV 的结束地址设置为 NEXT 的结束地址，然后将 end_changed 设置为 true. 函数接着在 862 行将 PREV 的 vm_pgoff 设置为 pgoff. 由于 adjust_next 不为真，因此 864 和 865 行代码不执行.

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH000289.png)

函数接着在 875 行对 remove_next 变量进行判断，该变量用于描述需要移除 next 虚拟区域的数量，由之前的逻辑可以知道，此时 remove_next 为 1，因此函数在 880 行的判断导致 881 行的逻辑被执行，此时函数通过调用 \_\_vma_unlink_prev() 函数进行移除。

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH000290.png)

\_\_vma_unlink_prev() 函数核心通过函数 \_\_vma_unlink_common() 函数实现，此时 mm 指向当前进程，参数 vma 指向 NEXT，参数 prev 指向 VMA，参数 has_prev 为 ture，参数 ignore 指向 PREV. 函数首先在 682 行调用 vma_rb_erase_ignore() 函数将 NEXT 从进程 mm 维护的区间树 (红黑树) 中移除, 然后在 683 行获得 NEXT 在进程 mm 维护的 VMA 链表的下一个成员。由于 has_prev 为真，那么函数将 PREV 的 vm_next 指向了 NEXT 的 vm_next 指向的 VMA。函数接着在 693 行判断 NEXT 的下一个 VMA 是否存在，如果存在，那么函数在 694 行将 NEXT 的下一个 VMA 的 vm_prev 指向了 PREV. 通过上面的操作，NEXT 从进程 mm 维护的区间树 (红黑树) 和 VMA 链表中进行移除，并重更新了因 NEXT 移除而修改的关系。函数最后在 697 行调用 vmacache_invalidate() 函数是进程的 VMA 缓存无效。

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH000291.png)

在回到 \_\_vma_adjust() 函数，函数执行到 929 行，此时 remove_next 为 1，因此函数进入分支执行。在该例子中，由于采用匿名映射，并且为发生缺页，因此 930 到 935 行代码逻辑均不执行。函数在 936 行将进程 mm 的 map_count 减一，因此更新程序映射的数量。函数在 937 行将 NEXT 对应的分配策略进行释放，最后在 938 行调用 vm_area_free() 函数将 NEXT 进行彻底的释放. 由于 remove_next 为 1，因此函数在 951 行将 next 变量指向了 PREV 的 vm_next VMA. 如果此时 next 存在，那么函数将在 971 行调用 vma_gap_update() 函数更新 next VMA 的 gap。

在该种情况下，\_\_vma_adjust() 函数运行到这里进行返回，回到 vma_merge() 函数，函数将 PREV 直接进行返回。此时在 vma_merge() 函数里添加如下打印，因此验证上面的分析:

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH000299.png)

在 BiscuitOS 上运行结果如下, PREV 虚拟区域占用 0x600000000000 到 0x600000001000, NEXT 占用 0x600000002000 到 0x600000003000 的区域，最后新建区域占用 0x600000001000 到 0x600000002000, 经过 \_\_vma_adjust() 函数合并之后，全部合入 PREV 虚拟区域，该区域占用 0x600000000000 到 0x600000003000，符合合并预期。

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH000300.png)

上面分析的情况在 vma_merge() 函数中属于 **Case1** 的情况，即三个 VMA 相邻，然后符合合并条件，并全部合入 PREV 虚拟区域。

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND000100.png)

-----------------------------------------------

#### <span id="A002">三个虚拟区域合并为两个</span>

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH000293.png)

虚拟区域要合并的前提是采用一致的映射方式、采用一致的内存策略、以及虚拟区域支持合并功能. 在 "Merge Case1" 情况下三个区域合并为两个虚拟区域，存在上面两种情况，第一种就是新建虚拟区和 PREV 进行合并; 第二种则是新建虚拟区和 NEXT 进行合并。因此需要对两种情况分别进行说明。

> [Merge Triple Areas to Double Areas (Merge PREV)](#A0020)
>
> [Merge Triple Areas to Double Areas (Merge NEXT)](#A0021)

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND000100.png)

------------------------------------------

###### <span id="A0020">Merge Triple Areas to Double Areas (Merge PREV)</span>

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH000294.png)

这种情况要发生，首先 PREV 和 NEXT 是不符合合并的要求，其次 PREV 和新建虚拟区域符合合并的条件。首先通过一个实例进行讲解, 该实例可以在 BiscuitOS 进行实践部署，部署请参考:

{% highlight bash %}
cd BiscuitOS
make menuconfig

  [*] Package  --->
      [*] Memory Mapping Mechanism  --->
          [*] VMA Merge Region Algorithm  --->
              [*] Case1: Merge Triple to double Areas (Merge PREV)  --->

BiscuitOS/output/linux-XXXX/package/BiscuitOS-vma-merge-case1-double-prev-default
{% endhighlight %}

> [VMA Merge Case1 Double (PREV) Source Code](https://gitee.com/BiscuitOS_team/HardStack/tree/Gitee/Memory-Allocator/MMAP/BiscuitOS-vma-merge/BiscuitOS-vma-merge-case1-double-prev)
>
> [BiscuitOS 独立程序实践教程](https://biscuitos.github.io/blog/Human-Knowledge-Common/#C)

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH000295.png)

在本例中，程序通过匿名私有映射的方式，并指定了映射的虚拟地址，这三次映射的虚拟区域的关系正好符合 "Merge Case1" 的情况，程序先在 59 和 60 行构造了两个虚拟区域，这两个虚拟区域分别形成了 PREV 和 NEXT 区域，但两个虚拟区域的映射属性不同，PREV 包含了 PROT_EXEC, 而 NEXT 不包含 PROT_EXEC, 因此 PREV 的虚拟区域可执行，但 NEXT 虚拟区域不可执行，因此 PREV 和 NEXT 不能进行合并. 接着在 63 行再次进行映射，此时新建的区域正好填补了 PREV 和 NEXT 之间的空洞，新建的虚拟区域同样包含了 PROT_EXEC 属性，因此新建区域只能与 PREV 进行合并，此时内核会将三个区域合并为两个。例子到这里已经符合调试分析的要求。

在本例例子中，PREV 区域的虚拟范围是 "0x600000000000 - 0x600000001000", NEXT 区域的范围是 "0x600000002000 - 0x600000003000", 新建区域的范围是 "0x600000001000 - 0x600000002000". 三个区域首先在范围上符合了 "Merge Case1" 的布局，其次 PREV 与新建虚拟区域都采用同样的映射方式和映射标志，均为 PROT_WRITE、PROT_EXEC 和 PROT_READ, 而 NEXT 只包含了 PROT_READ 和 PROT_WRITE, 因此 NEXT 不能与其余两个虚拟区域进行合并。最后就是对于同一个进程，其内存分配策略是一致的，因此三个虚拟最终可以合并为两个，并且 PREV 与新建虚拟区域将合并在一起.

###### 内核逻辑分析

采用本文提供的实践方法，可以通过跟踪 SYS_mmap 系统调用一起分析整个处理逻辑。如下图，在应用程序例子的 63 和 65 行分别加上 BiscuitOS 的调试开关，以此跟踪 64 行 SYS_mmap 系统调用.

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH000296.png)

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH000282.png)

当用户空间调用 SYS_mmap 系统调用进行映射，内核会按上图函数调用逻辑，最终调用掉 vma_merge() 函数，该函数就是本节重点讨论的地方。接下来分析 "Merge Case1" 情况下 vma_merge() 函数的处理逻辑.

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH000283.png)

vma_merge() 函数通过 1150 到 1161 行获得了 PREV 和 NEXT 虚拟区域，并且与新建虚拟区域地址进行检测，以此确认满足 "Merge Case1" 的条件，此时开发者可以按 1163 到 1165 行添加调试信息，以此在 BiscuitOS 运行查看运行结果:

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH000284.png)

测试用例在 BsicuitOS 运行之后，内核打印相关的日志，从日志可以看出此时 PREV 和 NEXT 虚拟区域，以及新建区域符合 "Merge Case1" 情况。

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH000285.png)

在当前情况下，vma_merge() 函数在 1166 行进行判断，此时 PREV 存在，且 PREV 的结束地址等于新建虚拟区的起始地址，由于 PREV 和新建虚拟区的内存分配策略相同，最后调用了 can_vma_merge_after() 函数检测了 PREV 和新建虚拟区的映射方式和标志等是否相同，此时是相同的，因此函数进入到 1171 行继续执行，此时函数在 1174 行同样检测 NEXT 和新建虚拟区域，由于新将虚拟区域和 NEXT 的映射属性不同，那么函数 can_vma_merge_before() 函数将返回 false，导致函数直接跳转到 1187 行执行，函数调用 \_\_vma_adjust() 函数将 PREV 和新建区域进行合并。

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH000286.png)

函数在调用 \_\_vma_adjust() 函数的时候，vma 参数指向了 PREV, start 参数指向了 PREV 的起始地址, end 指向了新建虚拟区的结束地址, 由于是匿名映射，因此 pgoff 指向了 PREV 的 pgoff，insert 参数为空，而 expand 参数指向 PREV。函数在 719 行定义了局部变量 next 指向了 NEXT, orig_vma 变量则指向 PREV.

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH000287.png)

在当前情况下, end 参数小于 next 指向虚拟区域的结束地址，因此函数不进入 731 行分支辑。

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH000288.png)

在本案例中，由于 PREV 和 NEXT 通过匿名映射获得的虚拟区域，当未访问过，因此其对应的 anon_vma 则不存在，那么 836 到 845 行的逻辑不执行。函数在 854 行对 start 与 PREV 的起始地址进行比较，此时两者相同。函数接着在 858 行进行检测，此时 end 不等于 PREV 的结束地址，因此函数会进入 859 行执行，此时函数将 PREV 的结束地址设置为新建虚拟区域的结束地址，然后将 end_changed 设置为 true. 函数接着在 862 行将 PREV 的 vm_pgoff 设置为 pgoff. 由于 adjust_next 不为真，因此 864 和 865 行代码不执行.

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH000289.png)

函数接着在 875 行对 remove_next 变量进行判断，该变量用于描述需要移除 next 虚拟区域的数量，由之前的逻辑可以知道，此时 remove_next 为 0，因此函数不进入分支。在本例中，函数会进入 902 行对应的分支. 函数在在 903 行判断 start_changed 变量是否为 true，由之前的运行逻辑可知，start_chagned 为 false，但 end_changed 变量为真，因此进入 906 分支，但此时 NEXT 存在，以及 adjuest_next 为 false，因此函数在 909 行调用 vma_gap_update() 函数更新了 NEXT 的 GAP 信息. 至此 \_\_vma_adjust() 函数的逻辑已经执行完成，在回到 vma_merge() 函数，此时新建区域已经和 VMA 进行合并，形成新的 PREV, 此时可以加入调试信息，在 BiscuitOS 上查看执行之后的结果，添加代码如下:

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH000297.png)

从 BiscuitOS 运行的情况可以看出，PREV 已经和新建区域进行合并, 且 NEXT 保持不变。反观整个例子，测试代码的结果符合预期.

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH000298.png)

上面分析的情况在 vma_merge() 函数中属于 **Case9** 的情况，即三个 VMA 相邻，然后i 新建区域只和 PREV 符合合并条件，并合入 PREV 虚拟区域，NEXT 区域则保留。

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND000100.png)

------------------------------------------

###### <span id="A0021">Merge Triple Areas to Double Areas (Merge NEXT)</span>

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH000305.png)

这种情况要发生，首先 PREV 和 NEXT 是不符合合并的要求，其次 NEXT 和新建虚拟区域符合合并的条件。首先通过一个实例进行讲解, 该实例可以在 BiscuitOS 进行实践部署，部署请参考:

{% highlight bash %}
cd BiscuitOS
make menuconfig

  [*] Package  --->
      [*] Memory Mapping Mechanism  --->
          [*] VMA Merge Region Algorithm  --->
              [*] Case1: Merge Triple to double Areas (Merge NEXT)  --->

BiscuitOS/output/linux-XXXX/package/BiscuitOS-vma-merge-case1-double-next-default
{% endhighlight %}

> [VMA Merge Case1 Double (NEXT) Source Code](https://gitee.com/BiscuitOS_team/HardStack/tree/Gitee/Memory-Allocator/MMAP/BiscuitOS-vma-merge/BiscuitOS-vma-merge-case1-double-next)
>
> [BiscuitOS 独立程序实践教程](https://biscuitos.github.io/blog/Human-Knowledge-Common/#C)

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH000306.png)

在本例中，程序通过匿名私有映射的方式，并指定了映射的虚拟地址，这三次映射的虚拟区域的关系正好符合 "Merge Case1" 的情况，程序先在 59 和 60 行构造了两个虚拟区域，这两个虚拟区域分别形成了 PREV 和 NEXT 区域，但两个虚拟区域的映射属性不同，NEXT 包含了 PROT_EXEC, 而 PREV 不包含 PROT_EXEC, 因此 NEXT 的虚拟区域可执行，但 PREV 虚拟区域不可执行，因此 PREV 和 NEXT 不能进行合并. 接着在 63 行再次进行映射，此时新建的区域正好填补了 PREV 和 NEXT 之间的空洞，新建的虚拟区域同样包含了 PROT_EXEC 属性，因此新建区域只能与 NEXT 进行合并，此时内核会将三个区域合并为两个。例子到这里已经符合调试分析的要求。在本例例子中，PREV 区域的虚拟范围是 "0x600000000000 - 0x600000001000", NEXT 区域的范围是 "0x600000002000 - 0x600000003000", 新建区域的范围是 "0x600000001000 - 0x600000002000". 三个区域首先在范围上符合了 "Merge Case1" 的布局，其次 NEXT 与新建虚拟区域都采用同样的映射方式和映射标志，均为 PROT_WRITE、PROT_EXEC 和 PROT_READ, 而 PREV 只包含了 PROT_READ 和 PROT_WRITE, 因此 PREV 不能与其余两个虚拟区域进行合并。最后就是对于同一个进程，其内存分配策略是一致的，因此三个虚拟最终可以合并为两个，并且 NEXT 与新建虚拟区域将合并在一起.

###### 内核逻辑分析

采用本文提供的实践方法，可以通过跟踪 SYS_mmap 系统调用一起分析整个处理逻辑。如下图，在应用程序例子的 63 和 65 行分别加上 BiscuitOS 的调试开关，以此跟踪 64 行 SYS_mmap 系统调用.

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH000307.png)

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH000282.png)

当用户空间调用 SYS_mmap 系统调用进行映射，内核会按上图函数调用逻辑，最终调用掉 vma_merge() 函数，该函数就是本节重点讨论的地方。接下来分析 "Merge Case1" 情况下 vma_merge() 函数的处理逻辑.

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH000283.png)

vma_merge() 函数通过 1150 到 1161 行获得了 PREV 和 NEXT 虚拟区域，并且与新建虚拟区域地址进行检测，以此确认满足 "Merge Case1" 的条件，此时开发者可以按 1163 到 1165 行添加调试信息，以此在 BiscuitOS 运行查看运行结果:

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH000284.png)

测试用例在 BsicuitOS 运行之后，内核打印相关的日志，从日志可以看出此时 PREV 和 NEXT 虚拟区域，以及新建区域符合 "Merge Case1" 情况。

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH000308.png)

在当前情况下，vma_merge() 函数由于无法与 PREV 进行合并，那么函数直接在 1200 行进行判断，此时 NEXT 存在，且 NEXT 的起始地址等于新建虚拟区的结束地址，由于 NEXT 和新建虚拟区的内存分配策略相同，最后调用了 can_vma_merge_before() 函数检测了 NEXT 和新建虚拟区的映射方式和标志等是否相同，此时是相同的。由于新建虚拟区的起始地址等于 PREV 区域的结束地址，因此函数直接跳转到 1209 行进行执行，函数调用 \_\_vma_adjust() 函数将 NEXT 和新建区域进行合并。

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH000286.png)

函数在调用 \_\_vma_adjust() 函数的时候，vma 参数指向了 NEXT, start 参数指向了新建区域的起始地址, end 指向了 NEXT 虚拟区的结束地址, 由于是匿名映射，因此 pgoff 指向了 NEXT 的 pgoff 减去新建区域占用的 pgoff 长度，insert 参数为空，而 expand 参数指向 NEXT。函数在 719 行定义了局部变量 next 指向了 NEXT, orig_vma 变量则指向 NEXT.

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH000287.png)

在当前情况下, end 参数小于 next 指向虚拟区域的结束地址，因此函数不进入 731 行分支辑。

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH000288.png)

在本案例中，由于 PREV 和 NEXT 通过匿名映射获得的虚拟区域，当未访问过，因此其对应的 anon_vma 则不存在，那么 836 到 845 行的逻辑不执行。函数在 854 行对 start 与 NEXT 的起始地址进行比较，此时两者不相同，因此函数进入分支，函数在在 855 行将 NEXT 的起始地址重设置为新建虚拟区域的起始地址，并将 start_changed 设置为 true。函数接着在 858 行进行检测，此时 end 等于 NEXT 的起始地址，因此函数不执行 859 行分支。 函数接着在 862 行将 NEXT 的 vm_pgoff 设置为 pgoff. 由于 adjust_next 不为真，因此 864 和 865 行代码分支不执行.

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH000289.png)

函数接着在 875 行对 remove_next 变量进行判断，该变量用于描述需要移除 next 虚拟区域的数量，由之前的逻辑可以知道，此时 remove_next 为 0，因此函数不进入分支。在本例中，函数会进入 902 行对应的分支. 函数在 903 行判断 start_changed 变量是否为 true，由之前的运行逻辑可知，start_chagned 为 true，但 end_changed 变量为 false，因此进入 904 分支，此时更新 NEXT 的 gap 信息. 至此 \_\_vma_adjust() 函数的逻辑已经执行完成，在回到 vma_merge() 函数，此时新建区域已经和 VMA 进行合并，形成新的 NEXT, 此时可以加入调试信息，在 BiscuitOS 上查看执行之后的结果，添加代码如下:

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH000309.png)

从 BiscuitOS 运行的情况可以看出，NEXT 已经和新建区域进行合并, 且 PREV 保持不变。反观整个例子，测试代码的结果符合预期.

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH000310.png)

上面分析的情况在 vma_merge() 函数中属于 **Case3** 的情况，即三个 VMA 相邻，然后新建区域只和 NEXT 符合合并条件，并合入 PREV 虚拟区域，PREV 区域则保留。

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND000100.png)

------------------------------------------

###### <span id="A003">三个虚拟区域不能合并</span>

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH000311.png)

这种情况要发生，首先 PREV 和 NEXT 是不符合合并的要求，其次新建虚拟区域也不符合合并的条件。首先通过一个实例进行讲解, 该实例可以在 BiscuitOS 进行实践部署，部署请参考:

{% highlight bash %}
cd BiscuitOS
make menuconfig

  [*] Package  --->
      [*] Memory Mapping Mechanism  --->
          [*] VMA Merge Region Algorithm  --->
              [*] Case1: Merge Triple to Triple Areas  --->

BiscuitOS/output/linux-XXXX/package/BiscuitOS-vma-merge-case1-triple-default
{% endhighlight %}

> [VMA Merge Case1 Triple Source Code](https://gitee.com/BiscuitOS_team/HardStack/tree/Gitee/Memory-Allocator/MMAP/BiscuitOS-vma-merge/BiscuitOS-vma-merge-case1-triple)
>
> [BiscuitOS 独立程序实践教程](https://biscuitos.github.io/blog/Human-Knowledge-Common/#C)

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH000312.png)

在本例中，程序通过匿名私有映射的方式，并指定了映射的虚拟地址，这三个映射的虚拟区域的关系正好符合 "Merge Case1" 的情况，程序先在 59 和 60 行构造了两个虚拟区域，这两个虚拟区域分别形成了 PREV 和 NEXT 区域，两个虚拟区域的映射属性相同，NEXT 包含了 PROT_EXEC, 而 PREV 也包含 PROT_EXEC, 因此 NEXT 和 PREV 的虚拟区域可执行，也能进行合并. 接着在 63 行再次进行映射，此时新建的区域正好填补了 PREV 和 NEXT 之间的空洞，新建的虚拟区域不包含了 PROT_EXEC 属性，因此新建区域不能与 NEXT 和 PREV进行合并，此时内核会将新建虚拟区域作为独立区域加入到系统。例子到这里已经符合调试分析的要求。在本例例子中，PREV 区域的虚拟范围是 "0x600000000000 - 0x600000001000", NEXT 区域的范围是 "0x600000002000 - 0x600000003000", 新建区域的范围是 "0x600000001000 - 0x600000002000". 三个区域首先在范围上符合了 "Merge Case1" 的布局，其次 NEXT 与新建虚拟区域采用不同的映射标志，同理新建虚拟区域与 PREV 的映射标志不同，因此不能进行合并。最后就是对于同一个进程，其内存分配策略是一致的，因此系统最终包含三个虚拟区域.

###### 内核逻辑分析

采用本文提供的实践方法，可以通过跟踪 SYS_mmap 系统调用一起分析整个处理逻辑。如下图，在应用程序例子的 63 和 65 行分别加上 BiscuitOS 的调试开关，以此跟踪 64 行 SYS_mmap 系统调用.

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH000313.png)

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH000282.png)

当用户空间调用 SYS_mmap 系统调用进行映射，内核会按上图函数调用逻辑，最终调用掉 vma_merge() 函数，该函数就是本节重点讨论的地方。接下来分析 "Merge Case1" 情况下 vma_merge() 函数的处理逻辑.

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH000283.png)

vma_merge() 函数通过 1150 到 1161 行获得了 PREV 和 NEXT 虚拟区域，并且与新建虚拟区域地址进行检测，以此确认满足 "Merge Case1" 的条件，此时开发者可以按 1163 到 1165 行添加调试信息，以此在 BiscuitOS 运行查看运行结果:

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH000284.png)

测试用例在 BsicuitOS 运行之后，内核打印相关的日志，从日志可以看出此时 PREV 和 NEXT 虚拟区域，以及新建区域符合 "Merge Case1" 情况。

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH000314.png)

由于新建虚拟区域和 PREV 相邻但映射条件不同，导致 can_vma_merge_after() 返回 false，因此新建区域与 PREV 不能合并，那么 1170 行的分支将不会被执行。

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH000315.png)

同理，由于新建虚拟区域和 NEXT 相邻但映射条件不同，导致 can_vma_merge_before() 函数返回 false，因此新建区域与 NEXT 不能合并，那么 1204 行的分支将不会被执行。至此 \_\_vma_adjust() 函数没有被调用，PREV 和 NEXT 将保持原样，新建虚拟区域将作为独立的虚拟区插入到进程中。此时可以在 vma_merge() 函数最后加入调试信息对上诉分析进行验证:

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH000316.png)

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH000317.png)

从实际运行的情况来看，新建虚拟区域确实作为独立的虚拟区域加入到进程，PREV 和 NEXT 保持原始。符合预期分析.

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND000100.png)

----------------------------------

<span id="B0"></span>

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND00000Q.jpg)

#### Merge Case2

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH000318.png)

当一个新建的虚拟区域 VMA 加入进程时，如果此时新建的虚拟区域的范围正好与 PREV 相邻，但与 NEXT 可能存在相离、相邻、相交以及覆盖的情况，此时我们称这种情况为 "Merge Case2". 接下来对这几种情况进行详细分析:

> - [NEW VMA 与 NEXT 相离](#B001)
>
> - [NEW VMA 与 NEXT 相邻](#B002)
>
> - [NEW VMA 与 NEXT 相交](#B003)
>
> - [NEW VMA 与 NEXT 覆盖](#B004)

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND000100.png)

------------------------------------------

###### <span id="B001">NEW VMA 与 NEXT 相离</span>

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH000319.png)

这种情况要发生，首先 PREV 和 NEXT 不相邻且 PREV 与新建虚拟区域相邻，NEXT 与新建虚拟区域相离。其次 PREV 和新建虚拟区域符合合并的条件。首先通过一个实例进行讲解, 该实例可以在 BiscuitOS 进行实践部署，部署请参考:

{% highlight bash %}
cd BiscuitOS
make menuconfig

  [*] Package  --->
      [*] Memory Mapping Mechanism  --->
          [*] VMA Merge Region Algorithm  --->
              [*] Case2: Merge PREV With Distant NEXT  --->

BiscuitOS/output/linux-XXXX/package/BiscuitOS-vma-merge-case2-distant-default
{% endhighlight %}

> [VMA Merge PREV with Distant NEXT Source Code](https://gitee.com/BiscuitOS_team/HardStack/tree/Gitee/Memory-Allocator/MMAP/BiscuitOS-vma-merge/BiscuitOS-vma-merge-case2-distant)
>
> [BiscuitOS 独立程序实践教程](https://biscuitos.github.io/blog/Human-Knowledge-Common/#C)

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH000320.png)

在本例中，程序通过匿名私有映射的方式，并指定了映射的虚拟地址，这三次映射的虚拟区域的关系正好符合 "Merge Case2" 的情况，程序先在 59 和 60 行构造了两个虚拟区域，这两个虚拟区域分别形成了 PREV 和 NEXT 区域，但两个虚拟区域相离. 接着在 63 行再次进行映射，新建的虚拟区域与 PREV 相邻，与 NEXT 相离，且 PREV 和 NEXT 中间没有其他 VMA，由于 PREV 和新建虚拟区域符合合并条件，因此 PREV 可以与新建虚拟区域合并。例子到这里已经符合调试分析的要求。在本例例子中，PREV 区域的虚拟范围是 "0x600000000000 - 0x600000001000", NEXT 区域的范围是 "0x600000008000 - 0x600000009000", 新建区域的范围是 "0x600000001000 - 0x600000002000". 三个区域首先在范围上符合了 "Merge Case2" 的布局，其次 PREV 与新建虚拟区域都采用同样的映射方式和映射标志，均为 PROT_WRITE 和 PROT_READ。最后就是对于同一个进程，其内存分配策略是一致的，最后就是 NEXT 和 PREV 之间不存在其他 VMA，因此测试例子符合 "Merge Case2" 的条件.

###### 内核逻辑分析

采用本文提供的实践方法，可以通过跟踪 SYS_mmap 系统调用一起分析整个处理逻辑。如下图，在应用程序例子的 63 和 65 行分别加上 BiscuitOS 的调试开关，以此跟踪 64 行 SYS_mmap 系统调用.

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH000321.png)

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH000282.png)

当用户空间调用 SYS_mmap 系统调用进行映射，内核会按上图函数调用逻辑，最终调用掉 vma_merge() 函数，该函数就是本节重点讨论的地方。接下来分析 "Merge Case2" 情况下 vma_merge() 函数的处理逻辑.

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH000283.png)

vma_merge() 函数通过 1150 到 1161 行获得了 PREV 和 NEXT 虚拟区域，并且与新建虚拟区域地址进行检测，以此确认满足 "Merge Case2" 的条件，由于 PREV 和 NEXT 之间不存在其他 VMA，因此 1155 行的条件不成立，因此 1156 行代码不会被执行，此时开发者可以按 1163 到 1165 行添加调试信息，以此在 BiscuitOS 运行查看运行结果:

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH000322.png)

测试用例在 BsicuitOS 运行之后，内核打印相关的日志，从日志可以看出此时 PREV 和 NEXT 虚拟区域，以及新建区域符合 "Merge Case2" 情况。

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH000323.png)

在当前情况下，vma_merge() 函数先将 PREV 与新建区域进行合并，那么函数符合 1166 到 1170 行检测条件，函数接着在 1174 行再次进行检测，由于新建虚拟区域与 NEXT 相离，且 PREV 和 NEXT 中间不存在其他 VMA，那么函数直接跳转到 1187 行执行，函数调用 \_\_vma_adjust() 函数处理实际的合并。

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH000286.png)

函数在调用 \_\_vma_adjust() 函数的时候，vma 参数指向了 PREV, start 参数指向 PREV 的起始地址, end 指向新建虚拟区的结束地址, 由于是匿名映射，因此 pgoff 指向了 PREV 的 pgoff，insert 参数为空，而 expand 参数指向 PREV。函数在 719 行定义了局部变量 next 指向了 NEXT, orig_vma 变量则指向 PREV.

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH000287.png)

在当前情况下, end 参数小于 next 指向虚拟区域的起始地址，因此函数不进入 731 行分支辑。

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH000288.png)

在本案例中，由于 PREV 和 NEXT 通过匿名映射获得的虚拟区域，但未访问过，因此其对应的 anon_vma 则不存在，那么 836 到 845 行的逻辑不执行。函数在 854 行对 start 与 PREV 的起始地址进行比较，此时两者相同，因此函数不进入分支。函数接着在 858 行进行检测，此时 end 等于 PREV 的结束地址，因此函数执行 859 行分支, 函数将 PREV 的结束地址设置为新建虚拟区域的结束地址，然后将 end_changed 设置为 true. 函数接着在 862 行将 PREV 的 vm_pgoff 设置为 pgoff. 由于 adjust_next 不为真，因此 864 和 865 行代码分支不执行.

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH000289.png)

函数接着在 875 行对 remove_next 变量进行判断，该变量用于描述需要移除 next 虚拟区域的数量，由之前的逻辑可以知道，此时 remove_next 为 0，因此函数不进入分支。在本例中，函数会进入 905 行对应的分支. 函数在 905 行判断 end_changed 变量是否为 true，由之前的运行逻辑可知，end_chagned 为 true，但 start_changed 变量为 false，因此进入 906 分支，由于此时 NEXT 存在且 adjust_next 为假，因此函数执行 909 行代码逻辑，此时函数调用 vma_gap_update() 函数更新 NEXT 的 gap 信息. 至此 \_\_vma_adjust() 函数的逻辑已经执行完成，在回到 vma_merge() 函数，此时新建区域已经和 PREV 进行合并，形成新的 PREV, 此时可以加入调试信息，在 BiscuitOS 上查看执行之后的结果，添加代码如下:

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH000324.png)

从 BiscuitOS 运行的情况可以看出，PREV 已经和新建区域进行合并, 且 NEXT 保持不变。反观整个例子，测试代码的结果符合预期.

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH000325.png)

上面分析的情况在 vma_merge() 函数中属于 **Case2** 的情况，即三个 VMA 相邻，然后新建区域只和 NEXT 符合合并条件，并合入 PREV 虚拟区域，PREV 区域则保留。

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/BiscuitOS/kernel/IND000100.png)

------------------------------------------

###### <span id="B004">NEW VMA 与 NEXT 覆盖</span>

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH000326.png)

这种情况要发生，首先 PREV 和 NEXT 不相邻且中间存在其他 VMA，其次 NEXT 与新建虚拟区域相离，另外新建虚拟区包含一个或多个 VMA，且新建虚拟区域的结束地址与所包含 VMA 中的最后一个 VMA 的结束地址重合，新建虚拟区域与其他虚拟区域符合合并的条件。首先通过一个实例进行讲解, 该实例可以在 BiscuitOS 进行实践部署，部署请参考:

{% highlight bash %}
cd BiscuitOS
make menuconfig

  [*] Package  --->
      [*] Memory Mapping Mechanism  --->
          [*] VMA Merge Region Algorithm  --->
              [*] Case2: Merge PREV With Cover NEXT  --->

BiscuitOS/output/linux-XXXX/package/BiscuitOS-vma-merge-case2-cover-default
{% endhighlight %}

> [VMA Merge PREV with Distant Multi VMA Source Code](https://gitee.com/BiscuitOS_team/HardStack/tree/Gitee/Memory-Allocator/MMAP/BiscuitOS-vma-merge/BiscuitOS-vma-merge-case2-distant-multi)
>
> [BiscuitOS 独立程序实践教程](https://biscuitos.github.io/blog/Human-Knowledge-Common/#C)

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH000327.png)

在本例中，程序通过匿名私有映射的方式，并指定了映射的虚拟地址，这四次映射的虚拟区域的关系正好符合 "Merge Case2" 的情况，程序先在 60 到 62 行构造了三个虚拟区域，这三个虚拟区域分别形成了 PREV 和 NEXT 区域，以及两者中间的其他 VMA，三个虚拟区域相离. 接着在 65 行再次进行映射，新建的虚拟区域与 PREV 相邻，与 NEXT 相离，且 PREV 和 NEXT 中间有其他 VMA，新建的虚拟区域正好覆盖了中间的 VMA，并且与中间的 VMA 符合合并的条件，另外 PREV 和新建虚拟区域符合合并条件，因此这些虚拟区域可以合并。例子到这里已经符合调试分析的要求。在本例例子中，PREV 区域的虚拟范围是 "0x600000000000 - 0x600000001000", NEXT 区域的范围是 "0x600000008000 - 0x600000009000", 新建区域的范围是 "0x600000001000 - 0x600000002000"，中间的 VMA 范围是 "0x600000002000 - 0x600000003000". 几个区域首先在范围上符合了 "Merge Case2" 的布局。

###### 内核逻辑分析

采用本文提供的实践方法，可以通过跟踪 SYS_mmap 系统调用一起分析整个处理逻辑。如下图，在应用程序例子的 65 和 67 行分别加上 BiscuitOS 的调试开关，以此跟踪 66 行 SYS_mmap 系统调用.

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH000328.png)

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH000282.png)

当用户空间调用 SYS_mmap 系统调用进行映射，内核会按上图函数调用逻辑，最终调用掉 vma_merge() 函数，该函数就是本节重点讨论的地方。接下来分析 "Merge Case2" 情况下 vma_merge() 函数的处理逻辑.

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH000283.png)

vma_merge() 函数通过 1150 到 1161 行获得了 PREV 和 NEXT 虚拟区域，并且与新建虚拟区域地址进行检测，以此确认满足 "Merge Case2" 的条件，由于 PREV 和 NEXT 之间不存在其他 VMA，因此 1155 行的条件不成立，因此 1156 行代码不会被执行，此时开发者可以按 1163 到 1165 行添加调试信息，以此在 BiscuitOS 运行查看运行结果:

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH000322.png)

测试用例在 BsicuitOS 运行之后，内核打印相关的日志，从日志可以看出此时 PREV 和 NEXT 虚拟区域，以及新建区域符合 "Merge Case2" 情况。

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH000323.png)

在当前情况下，vma_merge() 函数先将 PREV 与新建区域进行合并，那么函数符合 1166 到 1170 行检测条件，函数接着在 1174 行再次进行检测，由于新建虚拟区域与 NEXT 相离，且 PREV 和 NEXT 中间不存在其他 VMA，那么函数直接跳转到 1187 行执行，函数调用 \_\_vma_adjust() 函数处理实际的合并。

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH000286.png)

函数在调用 \_\_vma_adjust() 函数的时候，vma 参数指向了 PREV, start 参数指向 PREV 的起始地址, end 指向新建虚拟区的结束地址, 由于是匿名映射，因此 pgoff 指向了 PREV 的 pgoff，insert 参数为空，而 expand 参数指向 PREV。函数在 719 行定义了局部变量 next 指向了 NEXT, orig_vma 变量则指向 PREV.

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH000287.png)

在当前情况下, end 参数小于 next 指向虚拟区域的起始地址，因此函数不进入 731 行分支辑。

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH000288.png)

在本案例中，由于 PREV 和 NEXT 通过匿名映射获得的虚拟区域，但未访问过，因此其对应的 anon_vma 则不存在，那么 836 到 845 行的逻辑不执行。函数在 854 行对 start 与 PREV 的起始地址进行比较，此时两者相同，因此函数不进入分支。函数接着在 858 行进行检测，此时 end 等于 PREV 的结束地址，因此函数执行 859 行分支, 函数将 PREV 的结束地址设置为新建虚拟区域的结束地址，然后将 end_changed 设置为 true. 函数接着在 862 行将 PREV 的 vm_pgoff 设置为 pgoff. 由于 adjust_next 不为真，因此 864 和 865 行代码分支不执行.

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH000289.png)

函数接着在 875 行对 remove_next 变量进行判断，该变量用于描述需要移除 next 虚拟区域的数量，由之前的逻辑可以知道，此时 remove_next 为 0，因此函数不进入分支。在本例中，函数会进入 905 行对应的分支. 函数在 905 行判断 end_changed 变量是否为 true，由之前的运行逻辑可知，end_chagned 为 true，但 start_changed 变量为 false，因此进入 906 分支，由于此时 NEXT 存在且 adjust_next 为假，因此函数执行 909 行代码逻辑，此时函数调用 vma_gap_update() 函数更新 NEXT 的 gap 信息. 至此 \_\_vma_adjust() 函数的逻辑已经执行完成，在回到 vma_merge() 函数，此时新建区域已经和 PREV 进行合并，形成新的 PREV, 此时可以加入调试信息，在 BiscuitOS 上查看执行之后的结果，添加代码如下:

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH000324.png)

从 BiscuitOS 运行的情况可以看出，PREV 已经和新建区域进行合并, 且 NEXT 保持不变。反观整个例子，测试代码的结果符合预期.

![](https://gitee.com/BiscuitOS_team/PictureSet/raw/Gitee/HK/TH000325.png)

上面分析的情况在 vma_merge() 函数中属于 **Case2** 的情况，即三个 VMA 相邻，然后新建区域只和 NEXT 符合合并条件，并合入 PREV 虚拟区域，PREV 区域则保留。

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








MMU
=========
 |
 |
 |    +--------------+
 +--->| Linux 2.6.12 |
 |    +--------------+
 |         |
 |         +--- [*] 提供了一个完整的 MMU 子系统，包含多个内存分配器，并提供
 |         |        内存分配管理任务.
 |         +--- SLAB: [*] 提供了一个完整的 SLAB 内存分配器，用于分配小粒度内存
 |                    [*] 增加 kmem_cache_alloc_node() 和 kmalloc_node() 函数，
 |                        使其在 NUMA 架构中从指定 NODE 中分配内存.
 |                    [*] 增加 kmem_find_general_cachep() 函数以便获得通用高速
 |                        缓存
 |
 |    +----------------+
 +--->| Linux 2.6.12.1 |
 |    +----------------+
 |
 |    +----------------+
 +--->| Linux 2.6.12.2 |
 |    +----------------+
 |
 |    +----------------+
 +--->| Linux 2.6.12.3 |
 |    +----------------+
 |
 |    +----------------+
 +--->| Linux 2.6.12.4 |
 |    +----------------+
 |
 |    +----------------+
 +--->| Linux 2.6.12.5 |
 |    +----------------+
 |
 |    +----------------+
 +--->| Linux 2.6.12.6 |
 |    +----------------+
 |
 |    +--------------+
 +--->| Linux 2.6.13 |
 |    +--------------+
 |         |
 |         +--- bootmem: [*] 提供了在未启用 CONFIG_HAVE_ARCH_ALLOC_REMAP 宏的
 |         |                 情况下 "alloc_remap()" 函数的实现。
 |         |             [*] 针对 i386 架构，新增了 saved_max_pfn 变量，用于
 |         |                 维护当前系统最大可用物理帧。
 |         |             [*] 使用 ALIGN() 替换了 bootmem 分配器的对齐方式.
 |         |             [*] bootmem 支持早期的 PFN.
 |         |
 |         +--- PERCPU:  [*] 将 __per_cpu_start 和 __per_cpu_end 放到 sections.h
 |         |
 |         +--- buddy:   [*] 添加 __GFP_NOMEMPOOL 标志，允许 MEMPOOL 直接从
 |         |                 Buddy 分配器中获得物理内存.
 |         |             [*] 在系统中添加 "/proc/zoneinfo" 节点，以便获得 ZONE
 |         |                 的使用情况.
 |         |             [*] 向 Buddy 分配器添加页回收机制，以及添加了一个新的
 |         |                 系统调用 sys_set_zone_reclaim 用于控制 Buddy 分配
 |         |                 器是否支持页回收机制. 添加 __GFP_NORECLAIM 标志可
 |         |                 以让 Buddy 分配器不进行页回收机制.
 |         |             [*] 重新规划了 ZONETABLE_MASK 和 NODES_MASK, 让 page
 |         |                 初始化时关联到正确的 NODE 和 ZONE 上。
 |         |
 |         |
 |         +--- PCP:     [*] struct zone 支持 NUMA/UMA pageset 成员.
 |         |             [*] start_kernel() 中新增 setup_per_cpu_pageset() 调用.
 |         |             [*] NUMA 体系中添加 pageset_table[] 数组.
 |         |             [*] 新增 PCP Hot/Cold 链表 batchsize 计算方法.
 |         |             [*] 增加 /proc/zoneinfo 对 PCP count 数量的打印.
 |         |             [*] PCP 初始化代码转移到 setup_pageset() 函数.
 |         |
 |         +--- VMALLOC: [*] 新增页表权限 PAGE_KERNEL_EXEC，使分配的虚拟内存
 |         |                 拥有执行和读写权限.
 |         |             [*] 新增 __remove_vm_area() 函数，更新 remove_vm_area
 |         |                 函数的内部实现.
 |         |
 |         +--- FIXMAP:  [*] 导出 kmap_atomic/kunmap_atomic 符号.
 |         |             [*] 新增 kmap_atomic_pfn() 函数实现.
 |         |
 |         +--- KMAP:    [*] 新增 kmap_atomic_pfn() 函数.
 |         |
 |         +--- SLAB:    [*] 增加 kmem_cache_name() 函数获得高速缓存名字.
 |                       [*] 增加 kstrdup() 函数用于为字符串分配空间并拷贝内容到
 |                           新分配的内存上.
 |                       [*] 修复 kmem_cache_alloc_node() 从当前阶段分配缓存
 |                           对象.
 |
 |    +----------------+
 +--->| Linux 2.6.13.1 |
 |    +----------------+
 |
 |    +--------------+
 +--->| Linux 2.6.14 |
 |    +--------------+
 |         |
 |         +--- bootmem: [*] 提供带 limit 限制的接口函数实现.
 |         |
 |         +--- PERCPU:  [*] 将 PERCPU 分配器从 kmem_cache_alloc_node() 替换
 |         |                 成了 kmalloc_node().
 |         |
 |         +--- buddy:   [*] 添加了 __ClearPageDirty() 函数，修改了 Zone 的
 |         |                 reclaim_in_progress 引用规则.
 |         |             [*] 将 Buddy 管理器使用的核心数据设置为 __read_mostly.
 |         |             [*] 添加了 __GFP_HARDWALL 标志.
 |         |             [*] 将 Buddy 内存分配器标志类型该为 gfp_t.
 |         |
 |         +--- PCP:     [*] 提供对 cpuset_zone_allowed() 的支持.
 |         |
 |         +--- VMALLOC: [*] 将 IOREMAP_MAX_ORDER 宏定义移到 vmalloc.h
 |         |             [*] 修改 VMALLOC 分配器分配标志为 gfp_t.
 |         |
 |         +--- SLAB:    [*] 将 kmem_bufctl_t 的定义迁移到 mm/slab.c.
 |                       [*] 从高速缓存中分配缓存时，使用 prefetchw() 预加载
 |                           缓存对象.
 |                       [*] 将 kcalloc() 函数替换成 kzalloc() 函数.
 |                       [*] 支持本地高速缓存的缓存栈.
 |                       [*] 支持共享高速缓存.
 |                       [*] 支持 NUMA 架构多节点 slab 链表.
 |
 |    +--------------+
 +--->| Linux 2.6.15 |
      +--------------+
           |
           +--- bootmem: [*] bootmem 将可用物理页释放给 buddy 内存分配器时，
           |                 将可用页的引用计数设置为 0.
           |             [*] 当 bootmem 在当前节点中找不到可用物理内存，那么
           |                 可以去其他节点中查找.
           |
           +--- PERCPU:  [*] 对于 UP 架构的 PERCPU 变量引用进行修改，添加了对
           |                 CPU 的引用.
           |
           +--- buddy:   [*] 对 Buddy 分配器标志添加 __force 强制类型检测.
           |             [*] 为每个 ZONE 建立后备分配 ZONE 添加 highest_zone().
           |             [*] 将 struct page 的 private 成员与 ptl 成员组成新的
           |                 联合体 u. 对 private 成员的引用和设置提供统一接口
           |                 "page_private()" 和 "set_page_private()".
           |             [*] Buddy 内存分配器初始化支持并兼容 Empty Zone.
           |             [*] 向 struct zone 中添加 span_seqlock 读写锁，当 Buddy
           |                 分配器在某些情况下读取特定 Zone 的 spanned_pages
           |                 成员时使用该读写锁.
           |             [*] Buddy 内存分配器 __alloc_pages() 核心函数新增加了
           |                 get_page_from_freelist() 调用，移除其替代的代码,
           |                 并增加了 ALLOC_ 标志控制分配过程.
           |             [*] Buddy 内存分配器移除 __GFP_NORECLAIM 标志.
           |             [*] 为 ZONE_HIGHMEM 添加了水位线.
           |             [*] Buddy 内存分配器新增 ZONE_DMA32 分区.
           |             [*] 修改 struct page 的 flags 成员类型为 unsigned long
           |
           +--- PCP:     [*] 修改了 PCP batchsize 的算法.
           |             [*] PCP 分配器初始时，将 Hot 页链表的 low 参数设置
           |                 为 0，将 Cold 页链表的 batch 设置为 
           |                 "max(1UL, batch/2)".
           |
           +--- VMALLOC: [*] 新增 vmalloc_node() 函数.
           |             [*] 新增 __vmalloc_node() 函数.
           |             [*] VMALLOC 建立 PTE 页表时，移除了 init_mm 的
           |                 page_table_lock 锁.
           |
           +--- SLAB:    [*] 将 struct kmem_cache_s 修改为 struct kmem_cache.
                         [*] 提供 page、slab 与高速缓存之间的绑定函数.
                         [*] 提供 SLAB 从 Buddy 分配器获得内存的统一接口.














buddy
=========
 |
 |
 |    +--------------+
 +--->| Linux 2.6.12 |
 |    +--------------+
 |         |
 |         +--- buddy: 1. 提供了一个完整的 buddy 内存分配器，用于管理
 |                        Linux 的物理内存分配和释放。
 |                     2. Linus 将所有的 buddy 代码添加到 git 里。
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
 |         +--- buddy: 1. 添加 __GFP_NOMEMPOOL 标志，允许 MEMPOOL 直接从
 |                        Buddy 分配器中获得物理内存.
 |                     2. 在系统中添加 "/proc/zoneinfo" 节点，以便获得 ZONE
 |                        的使用情况.
 |                     3. 向 Buddy 分配器添加页回收机制，以及添加了一个新的
 |                        系统调用 sys_set_zone_reclaim 用于控制 Buddy 分配
 |                        器是否支持页回收机制. 添加 __GFP_NORECLAIM 标志可
 |                        以让 Buddy 分配器不进行页回收机制.
 |                     4. 重新规划了 ZONETABLE_MASK 和 NODES_MASK, 让 page
 |                        初始化时关联到正确的 NODE 和 ZONE 上。
 | 
 |    +----------------+
 +--->| Linux 2.6.13.1 |
 |    +----------------+
 |
 |    +--------------+
 +--->| Linux 2.6.14 |
 |    +--------------+
 |         |
 |         +--- buddy: 1. 添加了 __ClearPageDirty() 函数，修改了 Zone 的
 |                        reclaim_in_progress 引用规则.
 |                     2. 将 Buddy 管理器使用的核心数据设置为 __read_mostly.
 |                     3. 添加了 __GFP_HARDWALL 标志.
 |                     4. 将 Buddy 内存分配器标志类型该为 gfp_t.
 |
 |    +--------------+
 o--->| Linux 2.6.15 |
      +--------------+
           |
           +--- buddy: 1. 对 Buddy 分配器标志添加 __force 强制类型检测.
                       2. 为每个 ZONE 建立后备分配 ZONE 添加 highest_zone().
                       3. 将 struct page 的 private 成员与 ptl 成员组成新的
                          联合体 u. 对 private 成员的引用和设置提供统一接口
                          "page_private()" 和 "set_page_private()".
                       4. Buddy 内存分配器初始化支持并兼容 Empty Zone.
                       5. 向 struct zone 中添加 span_seqlock 读写锁，当 Buddy
                          分配器在某些情况下读取特定 Zone 的 spanned_pages 
                          成员时使用该读写锁.
                       6. Buddy 内存分配器 __alloc_pages() 核心函数新增加了
                          get_page_from_freelist() 调用，移除其替代的代码, 
                          并增加了 ALLOC_ 标志控制分配过程.
                       7. Buddy 内存分配器移除 __GFP_NORECLAIM 标志.
                       8. 为 ZONE_HIGHMEM 添加了水位线.
                       9. Buddy 内存分配器新增 ZONE_DMA32 分区.
                       10. 修改 struct page 的 flags 成员类型为 unsigned long
































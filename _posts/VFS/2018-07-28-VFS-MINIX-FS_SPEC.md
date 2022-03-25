---
layout: post
title:  "实践 minix-fs 文件系统"
date:   2018-07-28 00:00:00 +0800
categories: [VFS]
excerpt: 实践 minix-fs 文件系统.
tags:
  - VFS
  - FS
  - MINIX-FS
  - Linux-1.0
---

![MINIX_MAIN](/assets/PDB/BiscuitOS/buildroot/VFS000028.JPG)

Kernel: **Linux 1.0.1.1**

MINIX-fs Version: **2.0**

Author: **BuddyZhang1**

Email: **buddy.zhang@aliyun.com**

----------------------------------------------------------------

## 简介

MINIX-FS 文件系统是一款为 MINIX 操作系统设计的文件系统，其包含了一个类 
Unix 文件系统必须的组件，也避开类 Unix 文件系统的复杂性，与其简单易用
的特点，在学术界获得普遍好评。MINIX 操作系统是由 Andrew S. Tanenbaum 
与 1980 编写的一款用于教学目的的操作系统，该操作系统简易而又保存了操作
系统的基本属性，在学术界获得较大范围的普及，用于介绍操作系统的基本实现。

Linus 在 Linux 1.0 中，支持了 MINIX-FS 1.0 到 3.0 的版本，并将 MINIX-FS
完整的适配到 Linux 1.0 的 VFS 子系统中。本文首先介绍 MINIX-FS 的基础属
性，然后介绍如何在 BiscuitOS 中实践 MINIX-FS。

----------------------------------------------------------------

## 目录

> MINIX-FS 架构

> MINIX-FS 数据结构

> BiscuitOS 中实践 MINIX-FS

> 制作一个 MINIX-FS 文件系统

> BiscuitOS MINIX-FS 内核配置

> MINIX-FS 调试准备

> MINIX-FS Boot 块实践分析

> MINIX-FS SuperBlock 块实践分析

> MINIX-FS Inode-BitMap 实践分析

> MINIX-FS Zone-BitMap 实践分析

> MINIX-FS Inode-Table 实践分析

> MINIX-FS 文件/目录管理实践分析

----------------------------------------------------------------

## MINIX-FS 构架

![MINIX_Layout](/assets/PDB/BiscuitOS/buildroot/MINIX-Layout.png)

MINIX-FS 是一款可以存储于磁盘或软盘上的文件系统，由于其简单易用的特点，
成为了初识文件系统的不二之选。MINIX-FS 的基本逻辑架构如上图。MINIX-FS 
将文件系统分成了 BLOCK_SIZE 大小的块，以此为 MINIX-FS 的最小原数据。 
BLOCK_SIZE 的大小为 1K Byte，也可修改为其他值。MINIX-FS 由 6 大部分组
成，缺一不可。每个部分负责管理和维护文件系统的基础数据和功能功能，使
操作系统其能够实现一个文件系统的读，写，同步等操作，也为实现数据的管理
和存储提供了可能。下面分别介绍每个部分的功能和意义。

##### Boot 

  Boot 块主要存储 Boot Loader 程序，如果 Boot Loader 不存在，那么 Boot 分区
  基本不使用。如果 MINIX-FS 正好是磁盘的第一个分区，那么 Boot 块里会包含整个
  磁盘的分区信息。具体操作请看 Boot 块详解。

##### Superblock 

Superblock 块为 MINIX-FS 的第二个块，其包含了 MINIX-FS 的 Super block 信
息，该信息用于描述 MINIX-FS 文件系统，其中包含了 MINIX-FS 的 MAGIC 信息，
文件系统大小和各部分大小信息。具体操作请看 superblock 块详解。

##### Inode-BitMap

Inode-BitMap 块是一个 BitMap 的位图，位图中每个位用于跟踪一个 inode 的使
用情况。操作系统可以通过 Inode-BitMap 块管理 MINIX-FS 的 inode。
Inode-BitMap 起始于 MINIX-FS 的第三个块，但其长度有文件系统的大小而定。
具体操作请看 Inode-BitMap 块详解。

##### Zone-BitMap

Zone-BitMap 块也是一个 BitMap 的位图，位图中的每个位用于跟踪一个 zone 
的使用情况。MINIX-FS 将文件系统分作 BLOCK_SIZE 大小的块，每个块称作 Zone。
Zone 用于存储数据，其大小由文件系统的大小而定。具体操作请看 Zone-BitMap 
块详解。

##### Inode-Table

Inode-Table 是一个典型的数组，数组由 minix_inode 构成，每个 minix_inode 
用于管理文件系统中的一个目录或文件。数组的索引有 inode 的 ino 指定。具体
操作请看 Inode-Table 块详解。

##### Data Zone

Data Zone 块是用于存储数据的块。在 MINIX-FS 中，文件系统被分作 
BLOCK_SIZE 大小的数据块，称为 Zone。Zone 分作两种，一种是 Data Zone，
用于存储数据，即存储文件和目录。另一种是 Reserved Zone，这些 Zone 用于存
储 MINIX-FS 的基础块，也就是 Boot，Superblock，Inode-BitMap，Zone-BitMap 
和 Inode-Table 块。本质上两种快没有任何区别，只是存储的数据不同而已。具体
操作请看 Data Zone 块详解。

------------------------------------------------------------------

## MINIX-FS 数据结构

为了在实践中能够让 MINIX-FS 的构架运行起来，基础数据是不可或缺的，每种基
础数据在 MINIX-FS 中担当着重要作用，也为操作系统对 MINIX-FS 的管理提供了
可能。本段重点解析每种数据结构。

#### minix_super_block

minix_super_block 用于描述 MINIX-FS 超级快信息， 其存储在磁盘或软盘中。
linux 1.0 版本中定义如下：

{% highlight ruby %}
/*
 * minix super-block data on disk
 */
struct minix_super_block {
        unsigned short s_ninodes;
        unsigned short s_nzones;
        unsigned short s_imap_blocks;
        unsigned short s_zmap_blocks;
        unsigned short s_firstdatazone;
        unsigned short s_log_zone_size;
        unsigned long s_max_size;
        unsigned short s_magic;
        unsigned short s_state;
};
{% endhighlight %}

###### s_ninodes

该成员用于描述 MINIX-FS 文件系统含有的 inode 数量。该值用于计算 
Inode-Bitmap 和 Inode-Table 的大小。

###### s_nzones

该成员用于描述 MINIX-FS 文件系统中含有的 Zone 的数量。该值用于计算 
Zone-Bitmap。

###### s_imap_blocks

该成员用于描述 Inode-BitMap 占用 BLOCK_SIZE 的数量，Inode-BitMap 中的每
个位表示一个 inode 的使用情况，如果某一个置位，那么对应的 minix_inode 
就被使用，反之表示对应的 minix_inode 没有被使用。

###### s_zmap_blocks

该成员用于描述 Zone-BitMap 占用 BLOCK_SIZE 的数量。Zone-BitMap 中的每
个位表示一个 Zone 的使用情况，如果某一位置位，那么对应的 zone 就被使
用，反之表示对应的 zone 没有被使用。

###### s_firstdatazone

该成员用于描述第一个 data zone 所在的 block 号。 Zone 分为两类，一类是 
Reserved 的，用于存储 MINIX-FS 的系统信息，另外一类是 Data zone，用于
存储文件和目录的，而 s_firstdatazone 用于指向第一个 data zone 的 block 号。

###### s_log_zone_size

MINIX-FS 将文件系统分成 BLOCK_SIZE 大小的数据块，BLOCK_SIZE 的大小由 
s_log_zone_size 决定，其计算方法如下：

{% highlight ruby %}
    BLOCK_SIZE = 1024 << s_log_zone_size
{% endhighlight %}

###### s_max_size

s_max_size 用于描述 MINIX-FS 文件或目录大小的最大值。

###### s_magic

s_magic 用于标注 MINIX-FS 的内部版本号，V1 版本的值是 0x137f， V2 版本
的值是 0x138f.

###### s_state 

s_state 用于描述 MINIX-FS 的 mount 状态。

#### minix_sb_info

minix_sb_info  用于描述 MINIX-FS 超级快信息， 其存储在内存中。linux 1.0 
版本中定义如下：

{% highlight ruby %}
/*
 * minix super-block data in memory
 */
struct minix_sb_info {
    unsigned long s_ninodes;
    unsigned long s_nzones;
    unsigned long s_imap_blocks;
    unsigned long s_zmap_blocks;
    unsigned long s_firstdatazone;
    unsigned long s_log_zone_size;
    unsigned long s_max_size;
    struct buffer_head * s_imap[8];
    struct buffer_head * s_zmap[8];
    unsigned long s_dirsize;
    unsigned long s_namelen;
    struct buffer_head * s_sbh;
    struct minix_super_block * s_ms;
    unsigned short s_mount_state;
};
{% endhighlight %}

###### s_ninodes

该成员用于描述 MINIX-FS 文件系统含有的 inode 数量。该值用于计算 
Inode-Bitmap 和 Inode-Table 的大小。

###### s_nzones

该成员用于描述 MINIX-FS 文件系统中含有的 Zone 的数量。该值用于计算 
Zone-Bitmap。

###### s_imap_blocks

该成员用于描述 Inode-BitMap 占用 BLOCK_SIZE 的数量，Inode-BitMap 
中的每个位表示一个 inode 的使用情况，如果某一个置位，那么对应的 
minix_inode 就被使用，反之表示对应的 minix_inode 没有被使用。

###### s_zmap_blocks

该成员用于描述 Zone-BitMap 占用 BLOCK_SIZE 的数量。Zone-BitMap 中的
每个位表示一个 Zone 的使用情况，如果某一位置位，那么对应的 zone 就被
使用，反之表示对应的 zone 没有被使用。

###### s_firstdatazone

该成员用于描述第一个 data zone 所在的 block 号。 Zone 分为两类，一类是 
Reserved 的，用于存储 MINIX-FS 的系统信息，另外一类是 Data zone，用于
存储文件和目录的，而 s_firstdatazone 用于指向第一个 data zone 的 block 号。

###### s_log_zone_size

MINIX-FS 将文件系统分成 BLOCK_SIZE 大小的数据块，BLOCK_SIZE 的大小由 
s_log_zone_size 决定，其计算方法如下：

{% highlight ruby %}
  BLOCK_SIZE = 1024 << s_log_zone_size
{% endhighlight %}

###### s_imap[8]

s_imap[8] 是一个文件缓存数组。数组用于缓存 MINIX-FS 的所有 Inode-BitMap 内容。

###### s_zmap[8]

s_zmap[8] 是一个文件缓存数组，数组用于缓存 MINIX-FS 的所有 Zone-BitMap 内容。

###### s_dirsize 

s_dirsize 用于描述 minix_dir_entry 结构的大小，minix_dir_entry 用于描述
一个 minix 目录。

###### s_namelen

s_namelen 用于描述文件或目录名字的大小。

###### s_sbh

s_sbh 用于缓存磁盘中 MINIX-FS 的超级快信息。

###### s_ms

s_ms 用于指向 minix_super_block 在内存中的位置。

###### s_mount_state

s_mount_state 用于描述 MINIX-FS 的 mount 状态。

#### minix_inode 

minix_inode 用于描述一个 MINIX-FS 中的一个文件或目录，其存储在磁盘中。

{% highlight ruby %}
struct minix_inode {
        unsigned short i_mode;
        unsigned short i_uid;
        unsigned long i_size;
        unsigned long i_time;
        unsigned char i_gid;
        unsigned char i_nlinks;
        unsigned short i_zone[9];
};
{% endhighlight %}

###### i_mode

i_mode 用于描述一个 inode 的类型和访问属性。

###### i_uid

i_uid 用于描述一个 inode 需求的 uid 权限。

###### i_size

i_size 用于描述一个 inode 的大小。

###### i_time

i_time 用于描述修改 inode 的最后时间

###### i_gid

用于描述一个 inode 需求的 gid 权限

###### i_nlinks

i_nlinks 用于描述一个 inode 被 link 的数量

###### i_zone[9]

i_zone[9] 用于描述一个 inode 所有内容的 block 号。

#### minix_inode_info 

minix_inode_info 用于描述 MINIX-FS 中 inode 所有内容的 block 号，其存储在内存中

{% highlight ruby %}
/*
 * minix fs inode data in memory
 */
struct minix_inode_info {
        unsigned short i_data[16];
};
{% endhighlight %}

###### i_data[16]

i_data[16] 用于存储 minix inode 所有的数据 block 号

#### minix_dir_entry

minix_dir_entry 用于描述一个目录或文件的入口，其存储在磁盘中

{% highlight ruby %}
struct minix_dir_entry {
    unsigned short inode;
    char name[0];
};
{% endhighlight %}

###### inode

minix_inode 的 ino 号，用于唯一指定一个 inode

###### name[0]

name[0] 是一个空字符数组，用于存放文件或目录的名字

------------------------------------------

## BiscuitOS 中实践 MINIX-FS

BiscuitOS 中实践 MINIX-FS 文件系统包括以下几个步骤：

  > 制作一个 minix-fs 文件系统
    
  > 在 BiscuitOS 中运行 minix-fs 文件系统
    
  > 分析并调试 minix-fs 的元数据，包括 super block，inode，zone
    
  > 分歧并调试 minix-fs 的数据管理里机制
    
  > 基于 minix-fs 的原理开发自己的内核代码


BiscuitOS 目前完整支持 MINIX-FS 的所有版本，开发小组也提供了 MINIX-FS 的
开发 demo code，开发者可以根据 demo code 的介绍和实践完成 MINIX-FS 相关
的开发。本段将介绍调试前的准备工作。首先是制作一个 BiscuitOS 发行版，
以此建立调试基础，请参考上面 Blog 进行创建：

{% highlight ruby %}
BiscuitOS Linux 1.0.1.1 minix-fs 镜像制作方法： https://biscuitos.github.io/blog/Linux1.0.1.1_minixfs_Usermanual/
文件系统基础：
{% endhighlight %}

通过上面两篇文章的介绍，我们准备好源码和一个 MINIX-FS 的镜像，就可以在 
BiscuitOS 上一步步测试 MINIX-FS 功能。本文提供了以下实验进行实践和原理
解析，请自行参考：

> 制作一个 MINIX-FS 文件系统

> BiscuitOS MINIX-FS 内核配置

> MINIX-FS 调试准备

> MINIX-FS Boot 块分析

> MINIX-FS Superblock 分析

> MINIX-FS Inode-BitMap 分析

> MINIX-FS Zone-BitMap 分析

> MINIX-FS Inode-Table 分析

> MINIX-FS Data zone 分析

> MINIX-FS 文件和目录分析

--------------------------------------------------

## 制作一个 MINIX-FS 文件系统

Ubuntu 等发行版提供了 mkfs.minix 工具去创建一个 mini-fs 文件系统，具体代
码过程请参考脚本：

{% highlight ruby %}
BiscuitOS/scripts/fs/mkimage.sh
{% endhighlight %}

制作一个 mini-fs 文件系统的步骤如下：

首先使用 dd 工具制作一个空的镜像文件，使用如下命令：

{% highlight ruby %}
dd if=/dev/zero bs=512 count=${ROOTFS_sect} \
                   of=${IMAGE_DIR}/rootfs.img > /dev/null 2>&1
{% endhighlight %}

ROOTFS_Sect 为 rootfs 的扇区数，这里用户可以自行设置，设置方法如下：

{% highlight ruby %}
cd BiscuitOS
make clean
make linux_1_0_1_1_minix_defconfig
make menuconfig
{% endhighlight %}

输入上面的命令之后，开发者可以获得 Kbuild 提供的可视化配置界面，如下图：

![ROOTFS_MENU0](/assets/PDB/BiscuitOS/buildroot/VFS000006.png)

此时，选择 **Rootfs: file system --->** 选项 ,如下图：

![ROOTFS_MENU1](/assets/PDB/BiscuitOS/buildroot/VFS000007.png)

此时，选择 **Main Partition Size (MB)** 选项，如下图

![ROOTFS_MENU2](/assets/PDB/BiscuitOS/buildroot/VFS000008.png)

此时，开发者根据自己的需求输入 MINIX-FS 的大小，设置完之后，按下回车，
最后连续按下 ESC 按键保存配置文件，如下图

![ROOTFS_MENU3](/assets/PDB/BiscuitOS/buildroot/VFS000009.png)

至此，开发者已经设置好了 rootfs 也就是 minix-fs 的大小

接下来，将镜像文件 rootfs.img 格式化为 minix-fs ，使用如下命令：

{% highlight ruby %}
mkfs.minix -1 ${IMAGE_DIR}/rootfs.img
{% endhighlight %}

至此，一个 minix-fs 已经制作完成了。为了使 BiscuitOS 能正常启动，开发者需
要制作一个虚拟磁盘，这个磁盘包括三部分，分别为：

> MBR ： 这部分包含了磁盘的分区表信息

> rootfs： 这部分包含了文件系统，这里也就是 minix-fs 文件系统
    
> swap：这部分为交换分区

只有包含了上面三个部分，一个完整的 BiscuitOS 磁盘才算完成，接下来将继续介
绍如何制作一个完整的 BiscuitOS 磁盘。
首先制作 MBR，MBR 成为主引导部分，位于整个磁盘的最前面，使用如下命令制作

{% highlight ruby %}
dd if=/dev/zero bs=512 count=${MBR_sect} of=${IMAGE_DIR}/mbr.img
{% endhighlight %}

**MBR_sect** 用于指定 MBR 的扇区数，现代的 MBR 大小为 1MB，古老 MBR 占
一个扇区，也就是 512 个字节。这里开发者设置 MBR_sect 的值为 2048， 通过
上面的命令，开发者获得一个大小为 1MB 的镜像文件。此时，可是用 hexdump 
工具查看 MBR 的内容，如下：

![ROOTFS_MENU4](/assets/PDB/BiscuitOS/buildroot/VFS000010.png)

接下来是制作一个 Swap 交换分区，使用 mkswap 制作，使用如下命令

{% highlight ruby %}
dd if=/dev/zero bs=512 count=${SWAP_sect} of=${IMAGE_DIR}/swap.img
mkswap ${IMAGE_DIR}/swap.img
{% endhighlight %}

**SWAP_sect** 用于指定交换分区的大小，开发者可以自行设置，设置方法如下：

{% highlight ruby %}
cd BiscuitOS
make clean
make linux_1_0_1_1_minix_defconfig
make menuconfig
{% endhighlight %}

输入上面的命令之后，开发者可以获得 Kbuild 提供的可视化配置界面，如下图：

![ROOTFS_MENU5](/assets/PDB/BiscuitOS/buildroot/VFS000006.png)

此时，选择 **Rootfs: file system --->** 选项 ,如下图：

![ROOTFS_MENU6](/assets/PDB/BiscuitOS/buildroot/VFS000007.png)

此时，选择 **Swap Partition Size (MB)** 选项，如下图

![ROOTFS_MENU7](/assets/PDB/BiscuitOS/buildroot/VFS000011.png)

此时，开发者根据自己的需求输入 Swap 的大小，设置完之后，按下回车，
最后连续按下 ESC 按键保存配置文件，如下图

![ROOTFS_MENU8](/assets/PDB/BiscuitOS/buildroot/VFS000009.png)

至此，开发者已经设置好了 swap 也就是交换分区的大小。制作完 swap 镜像文件
之后，使用 mkswap 工具进行格式化

准备好三个部分的镜像文件之后，按下图方式进行磁盘文件拼接：

![ROOTFS_MENU8](/assets/PDB/BiscuitOS/buildroot/VFS000012.png)

开发者可以使用 dd 命令进行镜像文件的拼接，可以使用如下命令：

{% highlight ruby %}
# Append rootfs
dd if=${IMAGE_DIR}/rootfs.img conv=notrunc oflag=append bs=512  seek=${MBR_sect} of=${IMAGE_DIR}/mbr.img

# Append SWAP
dd if=${IMAGE_DIR}/swap.img conv=notrunc oflag=append bs=512  seek=${SWAP_SEEK} of=${IMAGE_DIR}/mbr.img
{% endhighlight %}

通过上面的命令，已经物理上将三个镜像文件拼接成一个镜像文件。制作完裸磁盘
镜像之后，开发者需要对磁盘文件进行分区表创建，可以使用 fdisk 工具进行磁盘
分区表的创建，使用如下命令：

{% highlight ruby %}
fdisk ${IMAGE_DIR}/mbr.img
{% endhighlight %}

以本例进行 fdisk 参数讲解，其中 MBR 的大小为 1MB，rootfs 也就是 minix-fs 
的大小为 40 MB，Swap 分区的大小为 80MB。使用上面命令之后，fdisk 运行如下图

![ROOTFS_MENU9](/assets/PDB/BiscuitOS/buildroot/VFS000015.JPG)

首先，创建一个新分区，输入 **n**

![ROOTFS_MENU10](/assets/PDB/BiscuitOS/buildroot/VFS000016.png)

设置第一个分区为主分区，并且分区号设置为 1，输入 **p**，然后再输入 **1**

![ROOTFS_MENU11](/assets/PDB/BiscuitOS/buildroot/VFS000017.png)

然后设置第一个分区的起始扇区为 2048，由于 MBR 的大小为 1MB，所以 MBR 占用
了从 0 到 2047 的扇区，第一个分区紧紧跟着 MBR，所以第一个分区的起始扇区数
为 2048. 在本例中 minix-fs 的体积为 40MB，所以 MINIX-fs 占用的扇区数计算如下：

{% highlight ruby %}
MINIX 扇区数 = (40 * 1024 * 1024 ) / 512 = 81920
{% endhighlight %}

所以 MINIX-FS 占用了 81920 个扇区，由于 MINIX-FS 在磁盘中的起始扇区数为 2048，
所以结束扇区数的计算如下：

{% highlight ruby %}
第一分区结束扇区数 = 第一分区起始扇区数 + 第一个分区占用的扇区数 - 1
MINIX 结束扇区数 = 81920 + 2048 - 1 = 83967
{% endhighlight %}

因此 fdisk 中 Firs sector 设置为 2048， Last sector 设置为 83967，如下图

![ROOTFS_MENU12](/assets/PDB/BiscuitOS/buildroot/VFS000018.png)

接着创建第二个分区，继续输入 n. 设置第二个分区也为主分区，并且分区号为 2， 
输入 **p**，然后输入 1.如图

![ROOTFS_MENU13](/assets/PDB/BiscuitOS/buildroot/VFS000019.png)

然后设置第二个分区的起始扇区，由于第二个分区为 Swap，其紧跟 rootfs 也就是 
minix-fs 分区，所以第二分区的起始扇区就是第一分区结束扇区增加 1. 所以计算
方法如下：

{% highlight ruby %}
Swap 起始扇区 = 第一分区结束扇区 + 1 = 83967 + 1 = 83968
{% endhighlight %}

第二分区结束扇区数的计算同第一分区计算方法一样，如下：

{% highlight ruby %}
第二分区扇区数 = Swap 分区体积(Byte) / 512 = (59 X 1024 X 1024) / 512 = 120832
第二分区结束扇区数 = 第二分区起始扇区数 + 第二个分区占用的扇区数 - 1
Swap 结束扇区 = 第二分区起始扇区 - 1 + 第二分区占有的扇区数 =  83968 + 120832 - 1 = 204799
{% endhighlight %}

根据上面的计算之后，设置 fdisk 中 Partition2 的 First sector 为 83968，
Last Sector 设置为 204799. 如下图：

![ROOTFS_MENU14](/assets/PDB/BiscuitOS/buildroot/VFS000020.png)

接下来设置分区 1 和分区 2 的分区类型，在 fdisk 中输入 **t**， 然后先设置分区 1，
在 fdisk 中输入 **1**，如下图：

![ROOTFS_MENU15](/assets/PDB/BiscuitOS/buildroot/VFS000021.png)

开发者如果不知道分区类型性，测试可以输入 L，该输入可以查看系统支持的分区类
型，如下图：

![ROOTFS_MENU16](/assets/PDB/BiscuitOS/buildroot/VFS000022.png)

根据上表可知，第一分区是一个 minix-fs 文件系统，而且是使用 mkfs.minix 
制作的 V1 版本的 minix-fs，所以分区类型为 **81 Minix / old Lin**. 因此 
fdisk 输入 **81**，如下图：

![ROOTFS_MENU17](/assets/PDB/BiscuitOS/buildroot/VFS000023.png)

接下来设置第二个分区的分区类型，第二个分区是 Swap，所以分区类型为 
**82 Linux swap / So**， 如下图

![ROOTFS_MENU18](/assets/PDB/BiscuitOS/buildroot/VFS000024.png)

至此，磁盘分区表已经设置好，最后在 fdisk 中输入 **w**，以此向磁盘中写入分
区表，写入成功后，一个 BiscuitOS 磁盘就制作完成。

![ROOTFS_MENU19](/assets/PDB/BiscuitOS/buildroot/VFS000025.png)

此时可以使用 hexdump 查看磁盘镜像的第一个扇区内容，如下图：

![ROOTFS_MENU20](/assets/PDB/BiscuitOS/buildroot/VFS000026.png)

以上制作 BiscuitOS 磁盘的过程也可以制作成自动化脚本，如下：

{% highlight ruby %}
## Add Partition Table
cat <<EOF | fdisk "${IMAGE}"
n
p
1
${ROOTFS_START}
${ROOTFS_END}
n
p
2
${SWAP_START}

t
1
${FS_TYPE}
t
2
82
w
EOF
{% endhighlight %}

在制作一个 linux 的 rootfs 最后一步骤就是将所有的配置文件和用户空间工具拷
贝到 rootfs 所在的磁盘上。因此对制作 MINIX-fs 的最后一步就是挂载 MINIX-fs，
只有将其挂载，配置文件和各种用户空间程序才能拷贝到 MINIX-fs 文件系统里。
开发者可以借助 losetup 和 mount 工具将上面制作好的磁盘镜像挂载到系统上。

首先，开发者需要将制作好的磁盘文件虚拟为一个块设备，使用 losetup 工具，
将磁盘镜像虚拟成块设备 /dev/loopx。开发者使用如下命令：

{% highlight ruby %}
LOOPDEV=`sudo losetup -f`
sudo losetup -d ${LOOPDEV}

# Mount 1st partition
sudo losetup -o 1048576 ${LOOPDEV} ${IMAGE}
{% endhighlight %}

LOOPDEV 指向一个空闲的 loop 设备，IMAGE 指向磁盘镜像。最关键就是 losetup 
的 -o 参数，该参数用于指定从磁盘文件开始挂载的位置。从上面磁盘镜像的制作
可知，开发者需要挂载 minix-fs 文件系统，其位于 MBR 之后，所以挂载位置的计
算如下：

{% highlight ruby %}
offset = First sector * 512 = 2048 * 512 = 1048576
{% endhighlight %}

接着使用 mount 工具将其挂载到指定目录下，使用如下命令：

{% highlight ruby %}
mkdir rootfs
sudo mount ${LOOPDEV} rootfs
{% endhighlight %}

执行完上面的命令之后，minix-fs 已经挂载到 rootfs 目录下面，开发者可以进入
到这个目录下，并将准备好的配置文件和用户空间的工具都拷贝到这个目录下，使用
如下命令：

{% highlight ruby %}
sudo cp -rf BiscuitOS/output/rootfs/linux_1.0.1.1/ rootfs
{% endhighlight %}

![ROOTFS_MENU21](/assets/PDB/BiscuitOS/buildroot/VFS000027.png)

最后，拷贝完 minix-fs 里面的数据之后，开发者将卸载 minix-fs，可以使用如
下命令：

{% highlight ruby %}
sudo umount rootfs
sudo losetup -d ${LOOPDEV}
{% endhighlight %}

至此，一个完整的 minix-fs 已经制作完成。开发者可以使用这个 BiscuitOS 磁盘
镜像。

--------------------------------------------------------

## BiscuitOS MINIX-FS 内核配置

BiscuitOS 采用 Linux 1.0.1.1 内核，内核已经提供了关于 MINIX-FS 的配置，
开发者可以在源码目录下输入以下命令进行内核配置

{% highlight ruby %}
cd BiscuitOS/kernel/linux_1.0.1.1/
make clean
make menuconfig
{% endhighlight %}


###### 启用 kernel hacking 选项

该选项用于启用内核调试机制，如下图，选择 **kernel hacking**

![ROOTFS_MENU22](/assets/PDB/BiscuitOS/buildroot/MINIXFS_00.png)

###### 启用 demo code 机制

BiscuitOS 提供了 MINIX-FS 相关的 demo code，选择 
**Demo Code for variable subsystem mechanism** 来加入 MINIX-FS demo code 功能

![ROOTFS_MENU23](/assets/PDB/BiscuitOS/buildroot/MINIXFS_01.png)

###### 启用 VFS 在线调试功能

MINIX-FS 启动之前需要启用 VFS 调试选项，选择 
**VFS Mechanism on X86 Architecture**.

![ROOTFS_MENU24](/assets/PDB/BiscuitOS/buildroot/MINIXFS_02.png)


###### 启用 MINIX-FS 文件系统级调试

BiscuitOS 支持多种文件系统调试，此处选择 **MINIX Filesystem**

![ROOTFS_MENU25](/assets/PDB/BiscuitOS/buildroot/MINIXFS_03.png)


###### MINIX-FS 调试选择

BiscuitOS 支持 MINIX-FS 的多种调试项目，开发者可以根据选项进行调试。
选择 **MINIX Filesystem: A Unix Filesystem**

> "Layout: boot block(boot code, partittion table)": 用于调试 MINIX-FS 的 boot 块
    
> "Layout: super block (inode,zone,BitMap)": 用于调试 MINIX-FS 的 superblock。
   
> "Layout: Inode-BitMap (manage unused/used Inode)": 用于调试 MINIX-FS 的 Inode BitMap
   
> "Layout: Zone-BitMap (manage unused/used Zone)": 用于调试 MINIX-FS 的 Zone BitMap
    
> "Layout: Inode-Table (manage minix Inode)": 用于调试 MINIX-FS 的 Inode Table
    
> "Layout: Data Zone (Store file/directory)": 用于调试 MINIX-FS 的 Data Zone
    
> "minix_inode: a inode on minix-fs": 用于调试 MINIX-FS 的 minix_node
    
> "minix_dir_entry: a directory on minix-fs": 用于调试 MINIX-FS 的 minix_dir_entry

--------------------------------------------------------------------

## MINIX-FS 调试准备

在配置完成调试的项目之后，开发者可以参照下面的步骤进行在线调试，使用两个
终端，第一个终端负责编写代码，另外一个终端负责运行代码。在其中一个终端中
输入如下命令，然后查看源码。

{% highlight ruby %}
cd BiscuitOS/kernel/linux_1.0.1.1/tools/demo/vfs/minix/
vi minixfs.c
{% endhighlight %}

另外一个终端中输入如下命令：

{% highlight ruby %}
cd BiscuitOS/kernel/linux_1.0.1.1/
make clean
make
make start
{% endhighlight %}

![ROOTFS_MENU26](/assets/PDB/BiscuitOS/buildroot/MINIX-FS_97.png)

源码如下图：

![ROOTFS_MENU27](/assets/PDB/BiscuitOS/buildroot/MINIX_FS-06.jpg)

从上图可以获得 MINIX-FS demo code 和运行之后的结果，开发者可以根据这个方
法，开始一步步调试 MINIX-FS。
对文件系统的访问一般发生在用户空间，用户程序通过系统调用 open，read 和 
write 等访问到文件系统，所以开发者应该先建立一个系统调用的环境来模拟这个
过程。BiscuitOS 系统调用实现方法请参考下面文章：

{% highlight ruby %}
BiscuitOS 系统调用实现方法：
BiscuitOS 如何添加源代码：
{% endhighlight %}

###### 构建模拟调试环境

为了构建模拟调试环境，开发者需要从用户空间打开指定的文件，通过在用户空间传
入文件名给 open() 系统调用。open() 通过 VFS 机制获得文件名对应的 inode 和 fd，
fd 为当前文件的句柄，然后将 fd 传给 demo_minixfs() 系统调用，demo_minixfs() 
通过系统调用，调用到内核的 sys_demo_minixfs() 接口，此时系统已经进入内核态，
开发者可以根据 fd 句柄获得 inode，从而通过分析 inode 的行为来了解 MINIX-FS 
的运作机制。

![ROOTFS_MENU28](/assets/PDB/BiscuitOS/buildroot/demo_minixfs.png)

###### 用户空间入口

用户空间程序主要实现两个目的，第一个目的是调用 open 函数获得特定文件的句柄，
第二个目的是将 fd 句柄传给 demo_minixfs() ，其代码如下：

{% highlight ruby %}
#include <linux/unistd.h>
#include <demo/debug.h>

/* System call entry */
static inline _syscall3(int, open, const char *, file, int, flag, int, mode);
static inline _syscall1(int, close, int, fd)；
inline _syscall1(int, demo_minixfs, int, fd);

static int debug_minixfs(void)
{
    int fd;

    fd = open("/etc/rc", O_RDONLY, 0);
    if (fd < 0) {
        printf("Unable to read /etc/rc\n");
        return -1;
    }

    demo_minixfs(fd);

    close(fd);

    return 0;
}
user1_debugcall_sync(debug_minixfs);
{% endhighlight %}


用户空间通过调用 user1_debugcall_sync() 接口，将函数 debug_minixfs 传入
到系统中，这样就会生成一个调用入口，每当系统启动到用户空间阶段，
debug_minixfs() 函数就会被调用。所以我们使用这种机制作为用户空间的调试入口。

debug_minix() 函数实现的逻辑很简单，首先调用 open 函数打开指定的文件，此
处将 ‘/etc/rc’ 文件做为调试对象。在获得打开文件的句柄 fd 之后，开发者
可以将 fd 句柄传入到 demo_minixfs() 函数中，这样就可以通过系统调用方式，
进入到我们设定的内核空间了。用户空间的最后一个动作就是关闭 fd 句柄，
开发者可以使用 close() 函数完成这个目的。为了在用户空间能够调用非 C 库的
系统调用，开发者需要在代码里显性的声明系统调用，代码如下：

{% highlight ruby %}
/* System call entry */
static inline _syscall3(int, open, const char *, file, int, flag, int, mode);
static inline _syscall1(int, close, int, fd)；
inline _syscall1(int, demo_minixfs, int, fd);
{% endhighlight %}

**_syscall3()** 和 **_syscall1()** 函数用于声明一个系统调用，具体过程可
以参考文章：

{% highlight ruby %}
Linux 系统调用机制：
{% endhighlight %}

debug_minixfs() 函数内部即用于模拟用户空间环境，开发者可以在其中使用 
printf 等函数，以此来添加打印信息，如下：

{% highlight ruby %}
#include <linux/unistd.h>
#include <demo/debug.h>

/* System call entry */
static inline _syscall3(int, open, const char *, file, int, flag, int, mode);
static inline _syscall1(int, close, int, fd)；
inline _syscall1(int, demo_minixfs, int, fd);

static int debug_minixfs(void)
{
    int fd;

    fd = open("/etc/rc", O_RDONLY, 0);
    if (fd < 0) {
        printf("Unable to read /etc/rc\n");
        return -1;
    }
    printf("Userland minixfs fd: %d\n", fd);

    demo_minixfs(fd);

    close(fd);

    return 0;
}
user1_debugcall_sync(debug_minixfs);
{% endhighlight %}

添加完之后，在终端中输入命令运行系统，如下：

![ROOTFS_MENU29](/assets/PDB/BiscuitOS/buildroot/MINIX-FS_09.png)

###### 内核空间程序入口

用户空间通过 demo_minixfs() 系统调用，将执行权转移到 sys_demo_minixfs() 
函数中，sys_demo_minixfs() 函数的主要目的是获得传入句柄 fd 对应的 inode，
也就是文件名对应的 inode。具体源代码如下：

{% highlight ruby %}
asmlinkage int sys_demo_minixfs(int fd)
{
    struct inode *inode;
    struct file *filp;
    struct super_block *sb;

    /* get file descriptor from current task */
    filp = current->filp[fd];
    if (!filp) {
        printk("Unable to open minixfs inode.\n");
        return -EINVAL;
    }

    /* get special inode */
    inode = filp->f_inode;
    if (!inode) {
        printk(KERN_ERR "Invalid inode.\n");
        return -EINVAL;
    }

    /* get special super block */
    sb = inode->i_sb;
    if (!sb) {
        printk(KERN_ERR "Invalid super block.\n");
        return -EINVAL;
    }

    /* Parse MINIX Filsystem layout */
    minix_layout(inode, sb);
    /* MINIX-FS inode */
    minix_inode(inode, sb);

    return 0;
}
{% endhighlight %}

当前进程维护一个该进程已经打开的文件描述表，每个打开的文件都有一个文件
句柄，开发者可以使用文件句柄来获得文件描述符，代码如下：

{% highlight ruby %}
    struct file *filp;

    /* get file descriptor from current task */
    filp = current->filp[fd];
    if (!filp) {
        printk("Unable to open minixfs inode.\n");
        return -EINVAL;
    }
{% endhighlight %}

![ROOTFS_MENU30](/assets/PDB/BiscuitOS/buildroot/filestruct01.png)

每个文件描述符用户描述一个打开的文件。文件描述符使用 struct file 数据结
构进行维护，在文件系统中，每个文件或目录都使用唯一的 inode 进行管理，每
一个打开的文件使用 struct file 进行管理，而且一个文件能被打开多次，所以 
struct file 和 struct file 之间的关系如下：

![ROOTFS_MENU31](/assets/PDB/BiscuitOS/buildroot/struct_file.png)


通过上图的关系，可以知道 struct file 中包含了 inode 相关的信息，struct file 
的成员 f_inode 指针用于指向文件对应的 struct inode，所以可以使用下面的代码
获得 inode 相关信息，如下：

{% highlight ruby %}
    /* get special inode */
    inode = filp->f_inode;
    if (!inode) {
        printk(KERN_ERR "Invalid inode.\n");
        return -EINVAL;
    }
{% endhighlight %}

获得 inode 之后，开发者就可以通过 inode 获得很多有用的信息，其中一个就是 
inode 所在的文件系统的 superblock。代码如下：

{% highlight ruby %}
    /* get special super block */
    sb = inode->i_sb;
    if (!sb) {
        printk(KERN_ERR "Invalid super block.\n");
        return -EINVAL;
    }
{% endhighlight %}

至此，MINIX-FS 的代码调试环境已经搭建完成，下面可以对 MINIX-FS 进行不同的调试。
以下小结用于介绍 MINIX-FS 的 superblock 入口设置和 minix_inode 和 
minix_dir_entry 入口设置。以下实验章节都是基于这节展开的。

###### MINIX-FS Super Block 入口

通过上面的步骤，开发者已经获得打开文件对应的 inode 和 super block 信息，
为了调试 MINIX-FS 文件系统相关的信息，开发者在 sys_demo_minixfs() 中调用
函数 minix_layout() 函数，该函数是 MINIX-FS 各块调试入口，调用如下：

{% highlight ruby %}
asmlinkage int sys_demo_minixfs(int fd)
{

   ......

    /* Parse MINIX Filsystem layout */
    minix_layout(inode, sb);

   .....
    return 0;
}
{% endhighlight %}

此时，使用一个终端对内核进行配置，一起启用 MINIX-FS 各块区的调试功能，
输入如下命令

{% highlight ruby %}
make clean
make menuconfig
{% endhighlight %}

配置参照 **BiscuitOS MINIX-FS 内核配置**，最后选择 MINIX-FS 配置如下：

> "Layout: boot block(boot code, partittion table)": 用于调试 MINIX-FS 的 boot 块

> "Layout: super block (inode,zone,BitMap)": 用于调试 MINIX-FS 的 superblock。

> "Layout: Inode-BitMap (manage unused/used Inode)": 用于调试 MINIX-FS 的 Inode BitMap

> "Layout: Zone-BitMap (manage unused/used Zone)": 用于调试 MINIX-FS 的 Zone BitMap

> "Layout: Inode-Table (manage minix Inode)": 用于调试 MINIX-FS 的 Inode Table

> "Layout: Data Zone (Store file/directory)": 用于调试 MINIX-FS 的 Data Zone

> "minix_inode: a inode on minix-fs": 用于调试 MINIX-FS 的 minix_node

> "minix_dir_entry: a directory on minix-fs": 用于调试 MINIX-FS 的 minix_dir_entry

![ROOTFS_MENU32](/assets/PDB/BiscuitOS/buildroot/MINIXFS_04.png)

选择其中一个配置，minix_layout() 函数将对其中一个功能进行调试，其源码如下：

![ROOTFS_MENU33](/assets/PDB/BiscuitOS/buildroot/minix_layout.jpg)

当调试选项打开后，sys_demo_minixfs()  函数会调用 minix_layout() 函数，
该函数首先做的就是判断 inode 对应的 super block 是否存在，如果不存在，
系统就从磁盘中读取 super block 块到内存中。由 MINIX-FS 的构架可知，
Super Block 块位于 MINIX-FS 文件系统的第二块，所以调用下面的代码进行读写：

{% highlight ruby %}
#define SUPER_BLOCK    0x1    

struct buffer_head *bh = NULL;
    struct minix_super_block *ms;
    if (!sb) {
        /* Read super block from Disk */
        if (!(bh = bread(inode->i_dev, SUPER_BLOCK, BLOCK_SIZE))) {
            printk(KERN_ERR "Unable obtain minix fs super block.\n");
            return -EINVAL;
        }
        ms = (struct minix_super_block *)bh->b_data;
        sb->u.minix_sb.s_ms = ms;
        sb->s_dev = inode->i_dev;
    }
{% endhighlight %}

运行上面的代码后，系统就可以从磁盘中读取 MINIX-FS 的 Super block. 在获得
超级块之后，开发者可以根据不同的内核配置来调试不同的功能，配置对应的函数如下：

> Layout: boot block(boot code, partittion table)

  >> 启用该功能之后，编译系统 Kbuild 会生成 CONFIG_DEBUG_BOOT_BLOCK, 
  >> 该宏用于调试 MINIX-FS 的 Boot Block 块，minix_layout() 函数就会调用 
  >> minixfs_boot_block() 函数。

> Layout: super block (inode,zone,BitMap)

  >> 启用该功能之后，编译系统 Kbuild 会生成 CONFIG_DEBUG_SUPER_BLOCK，该宏
  >> 用于调试 MINIX-FS 的 superblock。minix_layout() 函数就会调用 
  >> minixfs_super_block() 函数。

> Layout: Inode-BitMap (manage unused/used Inode)

  >> 启用该功能之后，编译系统 Kbuild 会生成 CONFIG_DEBUG_INODE_BITMAP，该
  >> 宏用于调试 MINIX-FS 的 Inode BitMap。minix_layout() 函数就会调用 
  >> minixfs_inode_bitmap() 函数。

> Layout: Zone-BitMap (manage unused/used Zone)

  >> 启用该功能之后，编译系统 Kbuild 会生成 CONFIG_DEBUG_ZONE_BITMAP， 该
  >> 宏用于调试 MINIX-FS 的 Zone BitMap。minix_layout() 函数就会调用 
  >> minixfs_zone_bitmap() 函数。

> Layout: Inode-Table (manage minix Inode)

  >> 启用该功能之后，编译系统 Kbuild 会生成 CONFIG_DEBUG_INODE_TABLE，
  >> 该宏用于调试 MINIX-FS 的 Inode Table。minix_layout() 函数就会调用 
  >> minixfs_inode_table() 函数。

> Layout: Data Zone (Store file/directory)

  >> 启用该功能之后，编译系统 Kbuild 会生成 CONFIG_DEBUG_DATA_ZONE，该宏用
  >> 于调试 MINIX-FS 的 Data Zone。minix_layout() 函数就会调用 
  >> minixfs_data_zone() 函数。

---------------------------------------------------

## MINIX-FS Boot 块分析

MINIX-FS 的 Boot 块为 MINIX-FS 的第一块，其大小为 1 KByte。Boot 块有两个作用：

> 存储 bootloader 代码
    
> 存储磁盘的分区表


###### 调试 MINIX-FS Boot 块

调试之前，开发者请参照 **MINIX-FS Super Block 入口** 和 **MINIX-FS 调试准备** 
两节，以此了解 BiscuitOS 如何调试 MINIX-FS。内核进行配置如下：

![ROOTFS_MENU34](/assets/PDB/BiscuitOS/buildroot/MINIXFS_04.png)

选择 **Layout: boot block(boot code, partition table)**,
此时系统会调用 minixfs_boot_block() 函数，该函数包含了对 boot block 
的操作和分析代码。源代码如下：

![ROOTFS_MENU35](/assets/PDB/BiscuitOS/buildroot/MINIX-FS11.jpg)

运行之后的如下图：

![ROOTFS_MENU36](/assets/PDB/BiscuitOS/buildroot/MINIX-FS_BOOT.png)

通过上面的实践，可知，Boot block 可以包含 bootload 代码也可以不包含 
bootloader 代码。minixfs_boot_block() 函数用于加载 MINIX-FS 的第一块到内存，
使用如下代码：

{% highlight ruby %}
    #define BOOT_BLOCK    0x0
    struct buffer_head *bh;

    /* Read Boot block from Disk */
    if (!(bh = bread(sb->s_dev, BOOT_BLOCK, BLOCK_SIZE))) {
        printk(KERN_ERR "Unable to read BOOT block.\n");
        return -EINVAL;
    }
{% endhighlight %}

bread() 函数用于从磁盘第 BOOT_BLOCK 块读取 BLOCK_SIZE 大小的数据到内存
的 buffer 中，bh 是一个 buffer_head 结构，用于维护一个 buffer，其中 bh 
的 b_data 成员用于指向 buffer 在内存中的位置。所以开发者可以使用如下代
码进行 boot block 数据块的使用：

{% highlight ruby %}
    char *buf;
    int i;

    /* Read boot code */
    buf = (char *)bh->b_data;
    printk("Boot Code:\n");
    for (i = 510; i < BLOCK_SIZE; i++)
        printk("%x ", buf[i]);
    printk("\n");
{% endhighlight %}

BiscuitOS 使用的 rootfs 为 BiscuitOS-1.0.1.img, 其分区结构如图，前 1 M 
空间为 MBR，后续空间为 MINIXFS 文件系统的空间，为第一个硬盘分区。

![ROOTFS_MENU37](/assets/PDB/BiscuitOS/buildroot/MINIX-FS_MBR.png)

使用 hexdump 工具查看器内容，使用如下命令：

{% highlight ruby %}
hexdump BiscuitOS-1.0.1.img
{% endhighlight %}

![ROOTFS_MENU38](/assets/PDB/BiscuitOS/buildroot/MINIX-FX_HEXP.png)

由 boot block 块知道，Boot block 块也包含分区的基本信息，其分区图如下描述：

![ROOTFS_MENU39](/assets/PDB/BiscuitOS/buildroot/MINIX-FS_Comment.png)

其数据如下：

{% highlight ruby %}
0000000 0000 0000 0000 0000 0000 0000 0000 0000
*
00001b0 0000 0000 0000 0000 0f12 7e4e 0000 2000
00001c0 0021 3981 0534 0800 0000 4000 0001 3900
00001d0 0535 dc83 0517 4800 0001 2800 0000 0000
00001e0 0000 0000 0000 0000 0000 0000 0000 0000
00001f0 0000 0000 0000 0000 0000 0000 0000 aa55
0000200 0000 0000 0000 0000 0000 0000 0000 0000
{% endhighlight %}


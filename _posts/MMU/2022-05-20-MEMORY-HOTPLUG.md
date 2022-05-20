---
layout: post
title:  "内存热插拔机制(未完成,边写中)"
date:   2022-05-20 12:00:00 +0800
categories: [HW]
excerpt: Memory Hotplug.
tags:
  - Memory Hotplug
---

![](/assets/PDB/BiscuitOS/kernel/IND00000L0.PNG)

![](/assets/PDB/RPI/RPI100100.png)

#### 目录

> - [内存热插拔基础知识](#A)
>
> - [内存热插拔实践攻略](#B)
>
> - [内存热插拔使用手册](#C)
>
> - [内存热插拔源码分析](#D)
>
> - [内存热插拔开源工具](#E)
>
> - 内存热插拔进阶研究

######  🙂🙂🙂🙂🙂🙂🙂🙂🙂🙂🙂🙂🙂🙂🙂🙂 捐赠一下吧 🙂🙂🙂🙂🙂🙂🙂🙂🙂🙂🙂🙂🙂🙂🙂🙂

![BiscuitOS](/assets/PDB/BiscuitOS/kernel/HAB000036.jpg)

-------------------------------------------

<span id="A"></span>

![](/assets/PDB/BiscuitOS/kernel/IND00000I.jpg)

#### 内存热插拔基础知识

内存热插调用栈:

{% highlight bash %}
__add_memory
acpi_memory_device_add
acpi_bus_attach
acpi_bus_scan
acpi_device_hotplug
acpi_hotplug_work_fn
{% endhighlight %}

内存使能调用栈:

{% highlight bash %}
__free_page
init_page_count
ClearPageReserved
generic_online_page
online_pages_range
walk_system_ram_range
online_pages
memory_subsys_online
device_online
state_store
kernfs_fop_write
{% endhighlight %}


-------------------------------------------

<span id="B"></span>

![](/assets/PDB/BiscuitOS/kernel/IND00000Q.jpg)

#### 内存热插拔实践攻略

开发者想热插一块内存首先要准备一块新的内存条，然后在支持内存热插的机器上进行，想必这样大多数开发者对内存热插拔实践已经望而却步了，那开发者难道只能纸上谈兵吗? 幸运的是 BiscuitOS 给开发者提供一个内存热插的环境，BiscuitOS 本身基于 QEMU 构建的虚拟系统，其本身也可以模拟内存热插，那么本节就带各位开发者来一起在 BiscuitOS 上实践内存热插拔. 为了实践内存热插拔，开发者先搭建 Linux 5.0 x86_64 环境，可以参考如下链接，如果已经搭建可以继续本节内容。在实践之前请确保内核已经打开了相应的宏，开发者参考如下 (本文以 X86_64 架构为例进行讲解):

> [BiscuitOS Linux 5.0 X86-64 开发环境部署](/blog/Linux-5.0-x86_64-Usermanual/)

{% highlight bash %}
# KERNEL_VERSION: 内核版本字段 e.g. 5.0
cd BiscuitOS/output/linux-${KERNEL_VERSION}-x86_64/linux/linux/
make menuconfig ARCH=x86_64

    Memory Management options  --->
          Memory model (Sparse Memory)  --->
      [*] Allow for memory hot-add
      [*]   Allow for memory hot remove
    Power management and ACPI options  --->
      [*] ACPI (Advanced Configuration and Power Interface) Support  --->
          [*]   Memory Hotplug 
     
# 重新编译内核
make ARCH=x86_64 bzImage -j4
{% endhighlight %} 

![](/assets/PDB/HK/TH001679.png)
![](/assets/PDB/HK/TH001680.png)
![](/assets/PDB/HK/TH001681.png)
![](/assets/PDB/HK/TH001682.png)
![](/assets/PDB/HK/TH001683.png)

以上操作确保了 CONFIG_SPARSEMEM、CONFIG_MEMORY_HOTPLUG、CONFIG_MEMORY_HOTREMOVE、CONFIG_ACPI_HOTPLUG_MEMORY 以及 CONFIG_MIGRATION 宏启用. 接下来编译内核，内核编译完毕之后，接下来需要配置 BiscuitOS 的 QEMU 启动选项，以此启用更多的内存插槽来进行热插，其修改参考如下命令:

{% highlight bash %}
cd BiscuitOS/output/linux-${KERNEL_VERSION}-${ARCHITECTURE}/
vi RunBiscuitOS.sh

        -enable-kvm \
-       -m ${RAM_SIZE}M \
+       -m ${RAM_SIZE}M,slots=32,maxmem=8G \
        -device BiscuitOS-DMA \
        -kernel ${LINUX_DIR}/x86/boot/bzImage \

# 启动 BiscuitOS
./RunBiscuitOS.sh
{% endhighlight %}

对于 QEMU 的内存配置选项同添加 slots 和 maxmem 字段，其中 slots 为内存插槽的个数，而 maxmem 表示系统最大支持内存。修改完毕之后通过 RunBiscuitOS.sh 脚本启动 BiscuitOS，启动完毕之后，以此按下键盘 Ctrl 和 A 按键之后再按 C 键 (Ctrl-A C)，以此进入 QEMU monitor 模式，此时输入以下命令:

{% highlight bash %}
(qemu) object_add memory-backend-ram,id=BiscuitOS-mem,size=512M
(qemu) device_add pc-dimm,id=BiscuitOS-dimm,memdev=BiscuitOS-mem
{% endhighlight %}

QEMU 通过 object_add 命令新增了一个 memory-backend-ram 对象，其名字为 BisucitOS-mem, 其长度为 512M, 接着通过 device_add 命令新增一个 DIMM 设备，其名字为 BiscuitOS-dimm, 并通过 memdev 字段与 BiscuitOS-mem 进行绑定。设置完毕之后同样使用 "Ctrl-A C" 按键退出 QEMU monitor 模式，通过以上命令可以热插一块内存.

![](/assets/PDB/HK/TH001684.png)

对比热插前后 "free -m" 命令显示的可用内存并没有增加，但 "/sys/devices/system/memory/" 目录下新增了多个 memoryX 目录。支持一块内存已经热插到系统，但由于系统软件还没有使能这块内存，即内存的状态处在 offline 状态，那么接下来使用如下命令使能一块内存:

{% highlight bash %}
echo online > /sys/devices/system/memory/memory33/state
{% endhighlight %}

![](/assets/PDB/HK/TH001685.png)

当使能热插的内存之前先查看 "/sys/devices/system/memory/" 目录下 memory section 的 state，如果为 online 表示该 memory section 已经插入到系统内，反之 offline 表示 memory section 还没有插入到内存，那么接下来向 memory section 的 state 写入 online 字符串，以此使能 memory section，为何更好的验证使能的有效性，在使能前后通过 "free -m" 命令查看系统可用物理内存的信息。通过在 BiscuitOS 上的实践可以看到热插的 memory section 以 128MiB 为一个块，热插之后的内存加入到了系统可用物理内存池子, 支持内存热插完毕。那么接下来讲解如何热拔一块内存，原理就是热插的逆操作，首先让 memory section 下线，参考如下命令:

{% highlight bash %}
echo offline > /sys/devices/system/memory/memory33/state
{% endhighlight %}

![](/assets/PDB/HK/TH001686.png)

热拔一块内存首先要将其对应的 memory section 从系统下线，切记一个 memory section 长度为 128MiB，那么在 "/sys/devices/system/memory/" 目录下找到指定的 memoryX, 然后向其 state 节点写入 offline 字符串，顺利的话那么 memory section 会从系统下线，此时通过 "free -m" 命令查看可用物理内存的变化，通过实践结果可以看出，可用物理内存减少了 128MiB，那么 memory section 下线成功。那么接下来就是让 QEMU 将 DIMM 热拔掉，同理按下键盘 Ctrl 和 A 按键之后再按 C 键 (Ctrl-A C)，以此进入 QEMU monitor 模式, 使用如下命令:

{% highlight bash %}
(qemu) device_del BiscuitOS-dimm
(qemu) object_del BiscuitOS-mem
{% endhighlight %}

![](/assets/PDB/BiscuitOS/kernel/IND000100.png)

---
layout: post
title:  "of_get_flat_dt_prop"
date:   2019-02-22 10:43:30 +0800
categories: [HW]
excerpt: DTS API of_get_flat_dt_prop().
tags:
  - DTS
---

![DTS](/assets/PDB/BiscuitOS/kernel/DEV000106.jpg)

> Github: [of_get_flat_dt_prop](https://github.com/BiscuitOS/HardStack/tree/master/Device-Tree/kernel/API/of_get_flat_dt_prop)
>
> Email: BuddyZhang1 <buddy.zhang@aliyun.com>

# 目录

> - [源码版本信息](#源码版本信息)
>
> - [源码分析](#源码分析)
>
> - [实践](#实践)
>
> - [附录](#附录)

-----------------------------------

# <span id="源码版本信息">源码版本信息</span>

> 核版本： Linux 3.10/4.20.8
>
> Architecture：ARM32
>
> 函数名字：of_get_flat_dt_prop
>
> 函数路径：linux/drivers/of/fdt.c
>
> 函数作用：启动阶段，从 DTB 的 root 节点中获得属性的值。

# <span id="源码分析">源码分析</span>

代码流程图如下：

{% highlight c %}
of_get_flat_dt_prop
|
|---fdt_getprop
    |
    |---fdt_getprop_namelen
        |
        |---fdt_get_property_namelen
            |
            |---fdt_first_property_offset
            |   |
            |   |---_fdt_check_node_offset
            |       |
            |       |---fdt_next_tag
            |           |
            |           |---fdt_offset_ptr
            |               |
            |               |---fdt_version
            |               |   |
            |               |   |---fdt_get_header
            |               |
            |               |---fdt_size_dt_struct
            |               |
            |               |---_fdt_offset_ptr
            |
            |---fdt_next_property_offset
            |   |
            |   |---fdt_next_property_offset
            |
            |---fdt_get_property_by_offset
            |
            |---_fdt_string_eq
                |
                |---_fdt_string
                    |
                    |---fdt_off_dt_string
{% endhighlight %}

##### of_get_flat_dt_prop

{% highlight c %}
/**
* of_get_flat_dt_prop - Given a node in the flat blob, return the property ptr
*
* This function can be used within scan_flattened_dt callback to get
* access to properties
*/
const void *__init of_get_flat_dt_prop(unsigned long node, const char *name,
                       int *size)
{
    return fdt_getprop(initial_boot_params, node, name, size);
}
{% endhighlight %}

函数用于获得一个属性对应的值。

参数 node 指向属性在 DTB device-tree structure 段中的偏移，如果是 0 表示根节
点；参数 name 表示属性的名字；参数 size 表示属性名字的长度。

函数将 initial_boot_params 变量, name , size 直接传给了 fdt_getprop() 函数，
其中 initial_boot_params 指向 DTB 所在的虚拟地址。

##### fdt_getprop

{% highlight c %}
const void *fdt_getprop(const void *fdt, int nodeoffset,
            const char *name, int *lenp)
{
    return fdt_getprop_namelen(fdt, nodeoffset, name, strlen(name), lenp);
}
{% endhighlight %}

函数用于返回属性对应的值。

参数 fdt 指向一个可用的 DTB 所在的虚拟地址；nodeoffset 表示属性在 DTB 
device-tree structure 段中的偏移；name 代表索要找的属性名字，len 表示名字长度。
函数直接调用 fdt_getprop_namelen() 函数来获得属性的值。

##### fdt_getprop_namelen

{% highlight c %}
const void *fdt_getprop_namelen(const void *fdt, int nodeoffset,
                const char *name, int namelen, int *lenp)
{
    const struct fdt_property *prop;

    prop = fdt_get_property_namelen(fdt, nodeoffset, name, namelen, lenp);
    if (! prop)
        return NULL;

    return prop->data;
}
{% endhighlight %}

函数用于获得属性对应的值。

参数 fdt 指向一个可用的 DTB 所在的虚拟地址；nodeoffset 表示属性在 DTB 
device-tree structure 段中的偏移；name 代表索要找的属性名字，len 表示名字长度。
函数直接调用 fdt_get_property_namelen() 之后获得属性结构，然后直接返回属性的值。

##### fdt_get_property_namelen

{% highlight c %}
const struct fdt_property *fdt_get_property_namelen(const void *fdt,
                            int offset,
                            const char *name,
                            int namelen, int *lenp)
{
    for (offset = fdt_first_property_offset(fdt, offset);
         (offset >= 0);
         (offset = fdt_next_property_offset(fdt, offset))) {
        const struct fdt_property *prop;

        if (!(prop = fdt_get_property_by_offset(fdt, offset, lenp))) {
            offset = -FDT_ERR_INTERNAL;
            break;
        }
        if (_fdt_string_eq(fdt, fdt32_to_cpu(prop->nameoff),
                   name, namelen))
            return prop;
    }

    if (lenp)
        *lenp = offset;
    return NULL;
}
{% endhighlight %}

fdt_get_property_namelen() 函数用于获得指定名字的属性 device-tree strucure。

函数首先进行 for循环，遍历的起点是一个节点，如果不是则遍历失败，如果是节点，那
么就遍历这个节点里面的所有属性。fdt_first_property_offset() 函数可以判断当前 
device-tree structure 是不是一个节点，fdt_next_proprety_offset() 函数可以判断
当前 device-tree structure 是不是一个属性，如果是并将 offset 指向下一个 
device-tree structure，并返回下一个属性在 DTB device-tree structure 段中的偏
移，否则遍历停止。每次遍历中，调用 fdt_get_property_by_offset() 函数获得 
offset 对应的属性。然后调用 _fdt_string_eq() 函数对比属性的名字是否是参数 
name 对应的名字，如果是则返回该属性的虚拟地址；否则继续遍历。

##### fdt_get_property_by_offset

{% highlight c %}
const struct fdt_property *fdt_get_property_by_offset(const void *fdt,
                              int offset,
                              int *lenp)
{
    int err;
    const struct fdt_property *prop;

    if ((err = _fdt_check_prop_offset(fdt, offset)) < 0) {
        if (lenp)
            *lenp = err;
        return NULL;
    }

    prop = _fdt_offset_ptr(fdt, offset);

    if (lenp)
        *lenp = fdt32_to_cpu(prop->len);

    return prop;
}
{% endhighlight %}

fdt_get_property_by_offset() 函数用于通过 offset 获得一个属性的虚拟地址。

fdt 参数表示一个可用的 DTB 虚拟地址，offset 参数表示当前 device-tree structure
在 DTB device-tree structure 中的偏移。fdt_get_property_by_offset() 函数首先调
用 _fdt_check_prop_offset() 函数检测当前 device-tree structure 是不是一个属性，
如果是则调用 _fdt_offset_ptr() 函数获得 offset 对应 device-tree structure 的
虚拟地址，并且将 lenp 参数设置为平台大小端适配的数据，最后返回属性的虚拟地址。

##### fdt_first_property_offset

{% highlight c %}
int fdt_first_property_offset(const void *fdt, int nodeoffset)
{
    int offset;

    if ((offset = _fdt_check_node_offset(fdt, nodeoffset)) < 0)
        return offset;

    return _nextprop(fdt, offset);
}
{% endhighlight %}

fdt_first_property_offset() 函数用于检测当前 device-tree structure 是不是一个
节点，如果是，就返回节点中第一个属性在 DTB device-tree structure 段中的偏移。

参数 fdt 指向一个可用的 DTB 首地址，nodeoffset 代表 device-tree structure 在 
DTB device-tree structure 段中的偏移。首先函数调用 _fdt_check_node_offset() 函
数查看 nodeoffset 对应的 device-tree structure 是不是一个节点，如果不是，则返
回一个负值；如果是，则调用 _nextprop() 函数获得当前节点的第一个属性在 DTB 
device-tree structure 中的偏移。

##### fdt_next_property_offset

{% highlight c %}
int fdt_next_property_offset(const void *fdt, int offset)
{
    if ((offset = _fdt_check_prop_offset(fdt, offset)) < 0)
        return offset;

    return _nextprop(fdt, offset);
}
{% endhighlight %}

fdt_next_property_offset() 函数属性的下一个属性。

fdt 参数指向一个可用的 DTB， offset 表示当前 device-tree structure 在 DTB 
device-tree structure 段中的偏移。_fdt_check_prop_offset() 函数用于确定当前 
device-tree structure 是不是一个属性，只有当 device-tree structure 是一个属性
之后，调用 _nextporp() 函数获得 device-tree structure 之后的第一个属性。

##### _fdt_string_eq

{% highlight c %}
static int _fdt_string_eq(const void *fdt, int stroffset,
              const char *s, int len)
{
    const char *p = fdt_string(fdt, stroffset);

    return (strlen(p) == len) && (memcmp(p, s, len) == 0);
}
{% endhighlight %}

_fdt_string_eq() 函数用于对应传入的字符串是否和当前 device-tree structure 在 
DTB device-tree strings 段中的字符是否相等。如果相等则返回 1，否则返回 0.

_fdt_string_eq() 函数首先通过调用 fdt_string() 函数获得当前 device-tree 
structure 在 DTB device-tree strings 段中的字符串的虚拟地址，然后将这个字符串
与参数 s 和 len 进行比较。

##### fdt_string

{% highlight c %}
const char *fdt_string(const void *fdt, int stroffset)
{
    return (const char *)fdt + fdt_off_dt_strings(fdt) + stroffset;
}
{% endhighlight %}

fdt_string() 函数通过 stroffset 参数从 DTB 的 device-tress strings 段中找到 
strings 的虚拟地址。

##### _nextprop

{% highlight c %}
static int _nextprop(const void *fdt, int offset)
{
    uint32_t tag;
    int nextoffset;

    do {
        tag = fdt_next_tag(fdt, offset, &nextoffset);

        switch (tag) {
        case FDT_END:
            if (nextoffset >= 0)
                return -FDT_ERR_BADSTRUCTURE;
            else
                return nextoffset;

        case FDT_PROP:
            return offset;
        }
        offset = nextoffset;
    } while (tag == FDT_NOP);

    return -FDT_ERR_NOTFOUND;
}
{% endhighlight %}

该函数用于查找当前 device-tree structure 之后的第一个属性的偏移值。

参数 fdt 代表一个可用的 DTB，offset 代表一个 device-tree structure 在 DTB 
device-tree structure 段中的偏移。

_nextprop() 函数首先调用 fdt_next_tag() 函数获得了当前 devicet-tree structure 
的虚拟地址和下一个 device-tree structure 在 DTB device-tree structure 段中的偏
移。然后对当前 device-tree structure 的 tag 进行处理。如果 tag 指向 FDT_END，
并且下一个 device-tree structure 的偏移小于零，那么直接返回该偏移值。如果 tag 
指向一个 FDT_PROP，即一个属性的开始，那么直接返回当前 device-tree structure 的
偏移。如果上面两个条件不满足，那么内核就继续遍历剩下的 device-tree structure，
以此找到一个属性。

##### _fdt_check_node_offset

{% highlight c %}
int _fdt_check_node_offset(const void *fdt, int offset)
{
    if ((offset < 0) || (offset % FDT_TAGSIZE)
        || (fdt_next_tag(fdt, offset, &offset) != FDT_BEGIN_NODE))
        return -FDT_ERR_BADOFFSET;

    return offset;
}
{% endhighlight %}

_fdt_check_node_offset() 函数用于判断一个 device-tree structure 是不是一个节点。

参数 fdt 指向 DTB 的虚拟地址，offset 表示 device-tree structure 在 DTB 
device-tree structure 段中的偏移

这个函数对 offset 进行检测，如果 offset 小于 0 或 offset 没有按 FDT_TAGSIZE 对
齐，或者 device-tree structure 不是一个节点，那么判定 offset 是一个 
FDT_ERR_BADOFFSET; 如果 device-tree structure 是一个节点，并将 offset 的值设置
为当前 device-tree structure 的下一个 device-tree structure。

##### _fdt_check_prop_offset

{% highlight c %}
int _fdt_check_prop_offset(const void *fdt, int offset)
{
    if ((offset < 0) || (offset % FDT_TAGSIZE)
        || (fdt_next_tag(fdt, offset, &offset) != FDT_PROP))
        return -FDT_ERR_BADOFFSET;

    return offset;
}
{% endhighlight %}

_fdt_check_prop_offset() 函数用于查看 offset 对应的 device-tree structure 是不
是一个属性，如果是则返回下一个 device-tree structure 在 DTB device-tree 
structure 段中的偏移。

##### fdt_next_tag

{% highlight c %}
uint32_t fdt_next_tag(const void *fdt, int startoffset, int *nextoffset)
{
    const uint32_t *tagp, *lenp;
    uint32_t tag;
    int offset = startoffset;
    const char *p;

    *nextoffset = -FDT_ERR_TRUNCATED;
    tagp = fdt_offset_ptr(fdt, offset, FDT_TAGSIZE);
    if (!tagp)
        return FDT_END; /* premature end */
    tag = fdt32_to_cpu(*tagp);
    offset += FDT_TAGSIZE;

    *nextoffset = -FDT_ERR_BADSTRUCTURE;
    switch (tag) {
    case FDT_BEGIN_NODE:
        /* skip name */
        do {
            p = fdt_offset_ptr(fdt, offset++, 1);
        } while (p && (*p != '\0'));
        if (!p)
            return FDT_END; /* premature end */
        break;

    case FDT_PROP:
        lenp = fdt_offset_ptr(fdt, offset, sizeof(*lenp));
        if (!lenp)
            return FDT_END; /* premature end */
        /* skip-name offset, length and value */
        offset += sizeof(struct fdt_property) - FDT_TAGSIZE
            + fdt32_to_cpu(*lenp);
        break;

    case FDT_END:
    case FDT_END_NODE:
    case FDT_NOP:
        break;

    default:
        return FDT_END;
    }

    if (!fdt_offset_ptr(fdt, startoffset, offset - startoffset))
        return FDT_END; /* premature end */

    *nextoffset = FDT_TAGALIGN(offset);
    return tag;
}
{% endhighlight %}

fdt_next_tag() 函数主要用于获得 offset 指定的 device-tree structure 的虚拟地
址，以及下一个 device-tree structure 在 device-tree structure 段中偏移。
startoff 参数表示当前 device-tree structure 在 device-tree structure 段中的偏
移，fdt 参数表示一个可用的 DTB 虚拟地址。

fdt_next_tag() 函数首先通过传入参数 startoffset 和 fdt 到 fdt_offfset_ptr() 函
数，以此获得一个 device-tree structure 的虚拟地址。如果获得的虚拟地址为 NULL，
那么内核认为未找到期望的 device-tree structure，所以直接返回 FDT_END，代表查找
过早结束，并且将下一个 device-tree structure 在 device-tree structure 段中的
偏移标识为 -FDT_ERR_TRUNCATED, 以此表示 DTB 被截断了。接下来通过 
fdt32_to_cpu() 函数，将  device-tree structure 的虚拟地址转换为符合平台大小端
的数据结构，存储在 tag 变量里，然后将 offset 增加 FDT_TAGSIZE，以此指向 
device-tree structure 的第二个成员。在获得 device-tree structure 首地址之后，
对 device-tree structure 第一个字节也就是 tag 进行判断，看其是否属于节点还是属
性，以下是一个 device-tree structure 的基本结构：

![DTS](/assets/PDB/BiscuitOS/kernel/MMU000547.png)

如果 device-tree structure 的 tag 是 FDT_BEGIN_NODE，那么 device-tree 
structure 就是一个独立的节点或子节点；如果 device-tree structure 的首地址是 
FDT_PROP，那么节点就是一个属性描述；如果 device-tree structure 首地址是 
FDT_END, FDT_END_NODE, FDT_NOP 则忽略。

如果 device-tree structure 是一个节点，又根据节点的定义可知：

{% highlight c %}
struct fdt_node_header {
    uint32_t tag;
    char name[0];
};
{% endhighlight %}

此时，内核通过使用 while 循环将跳过 name 域，代码如下：

{% highlight c %}
    case FDT_BEGIN_NODE:
        /* skip name */
        do {
            p = fdt_offset_ptr(fdt, offset++, 1);
        } while (p && (*p != '\0'));
        if (!p)
            return FDT_END; /* premature end */
        break;
{% endhighlight %}

通过上面的代码，struct fdt_node_header 结构中的 name 成员将被跳过，这样就可
以找到下一个 device-tree structure 的首地址。如果  device-tree structure 是
一个属性，属性的定义如下：

{% highlight c %}
struct fdt_property {
    uint32_t tag;
    uint32_t len;
    uint32_t nameoff;
    char data[0];
};
{% endhighlight %}

对于 device-tree structure 是一个属性，内核的处理如下：

{% highlight c %}
    case FDT_PROP:
        lenp = fdt_offset_ptr(fdt, offset, sizeof(*lenp));
        if (!lenp)
            return FDT_END; /* premature end */
        /* skip-name offset, length and value */
        offset += sizeof(struct fdt_property) - FDT_TAGSIZE
            + fdt32_to_cpu(*lenp);
        break;
{% endhighlight %}

首先通过 fdt_offset_ptr() 函数获得属性的长度值后，跳过了属性的 nameoff，len，
和 data 成员。通过上面的处理之后，内核已经计算出下一个 device-tree structure 
相对于当前 device-tree structure 之间的偏移，然后调用 fdt_offset_ptr() 获得下
一个 device-tree structure 的虚拟地址，以此判断下一个 device-tree structure 是
否存在，如果不存在则返回 FDT_END；如果下一个 device-tree structure 的虚拟地址
存在，那么内核将下一个 device-tree structure 在 DTB device-tree structure 段中
的偏移赋值给 netoffset 指针。代码如下：

{% highlight c %}
    if (!fdt_offset_ptr(fdt, startoffset, offset - startoffset))
        return FDT_END; /* premature end */

    *nextoffset = FDT_TAGALIGN(offset);
    return tag;
{% endhighlight %}

因此，fdt_next_tag() 函数通过 DTB 的虚拟地址，以及 device-tree structure 在 
device-tree structure 段中的偏移可以获得 device-tree structure 的虚拟地址以及
下一个 device-tree structure 在 device-tree structure 段中的偏移。

##### fdt_offset_ptr

{% highlight c %}
const void *fdt_offset_ptr(const void *fdt, int offset, unsigned int len)
{
    const char *p;

    if (fdt_version(fdt) >= 0x11)
        if (((offset + len) < offset)
            || ((offset + len) > fdt_size_dt_struct(fdt)))
            return NULL;

    p = _fdt_offset_ptr(fdt, offset);

    if (p + len < p)
        return NULL;
    return p;
}
{% endhighlight %}

这个函数主要做一些基本检测之后，返回节点的虚拟地址。本函数中，传入参数 fdt 指
向一个可用的 DTB，offset 参数指定了节点在 DTB device-tree structure 段中的偏
移，len 参数指定了节点长度。函数首先检查 DTB 的 version 是否符合特定要求，如果
version 大于 0x10，且节点在 device-tree structure 段中的空间超出了 device-tree
structure 的空间，那么这是一个错误的情况，不能返回指定节点的虚拟地址，直接返回
NULL。如果前面的检测没有问题，那么函数调用 _fdt_offset_ptr() 去获得节点的虚拟
地址。在获得节点虚拟地址之后，如果节点的虚拟加上其长度小于虚拟地址，这是一种错
误的越界问题，所以也返回 NULL。待上面的检测都通过之后，就直返回节点的虚拟地址。

##### _fdt_offset_ptr

{% highlight c %}
static inline const void *_fdt_offset_ptr(const void *fdt, int offset)
{
    return (const char *)fdt + fdt_off_dt_struct(fdt) + offset;
}
{% endhighlight %}

在这个函数中，fdt 指向一个可用的 DTB 的虚拟地址，offset 参数指定了节点在 DTB 
的 device-tree structure 段中的偏移。通过这个函数，可以获得节点的虚拟地址，其
他程序就可以通过这个地址直接访问节点。

##### fdt_version

{% highlight c %}
#define fdt_version(fdt)        (fdt_get_header(fdt, version))
{% endhighlight %}

通过这个函数，从 DTB header 中获得 version 域信息。

##### fdt_size_dt_struct

{% highlight c %}
#define fdt_size_dt_struct(fdt)        (fdt_get_header(fdt, size_dt_struct))
{% endhighlight %}

这个函数通过调用 fdt_get_header() 从 DTB header 中获得 device-tree structure 
的大小。

##### fdt_get_header

{% highlight c %}
#define fdt_get_header(fdt, field) \
    (fdt32_to_cpu(((const struct fdt_header *)(fdt))->field))
{% endhighlight %}

该函数用于从 DTB header 中读取指定的域。并按指定的大小端进行变换。

-------------------------------------------------

# <span id="函数实践">函数实践</span>

实践目的是在 DTS 文件中构建一个私有节点，并在 root 节点下添加私有属性。然后在
驱动中通过 of_get_flat_dt_prop() 函数来读取这些属性的值。函数的定义如下：

{% highlight c %}
const void *__init of_get_flat_dt_prop(unsigned long node, const char *name, int *size);
{% endhighlight %}

这个函数只存在启动阶段，系统启动完毕之后会自动丢弃，所以不能使用模块的方式进行
测试，只能在内核启动阶段进行测试。这个函数只能读取 root 节点的属性，不能读私有
节点的属性，所以使用 of_get_flat_dt_root() 函数获得 root 节点。

本文实践基于 Linux 4.20.8 arm32 平台，开发者可以参考如下文章快速搭建一个
调试环境：

> [Establish Linux 4.20.8 on ARM32](/blog/Linux-4.20.8-arm32-Usermanual/)

#### DTS 文件

由于使用的平台是 ARM32，所以在源码 /arch/arm/boot/dts 目录下创建一个 DTSI 文
件，具体内容如下：

{% highlight c %}
/*
 * DTS Demo Code
 *
 * (C) 2019.01.06 <buddy.zhang@aliyun.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
/ {
      BiscuitOS-int = <0x10203040>;
      BiscuitOS-name = "BiscuitOS";
      DTS_demo {
            compatible = "DTS_demo, BiscuitOS";
            status = "okay";
      };
};
{% endhighlight %}

创建完毕之后，将其保存并命名为 DTS_demo.dtsi。然后开发者在 Linux 4.20.8 的源
码中，找到 arch/arm/boot/dts/vexpress-v2p-ca9.dts 文件，然后在文件开始地方添
加如下内容：

{% highlight c %}
#include "DTS_demo.dtsi"
{% endhighlight %}

#### 编写 DTS 对应驱动

准备好 DTSI 文件之后，开发者编写一个简单的驱动，这个驱动作为 DTS_demo 的设备驱
动，在 DTS 机制遍历时会调用匹配成功的驱动，最终运行驱动里面的代码。驱动编写如下：

{% highlight c %}
/*
 * DTS:
 *    of_get_flat_dt_root()
 *    of_get_flat_dt_prop()
 *
 * (C) 2018.11.14 <buddy.zhang@aliyun.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
/*
 * Private DTS file: DTS_demo.dtsi
 *
 * / {
 *        BiscuitOS-int = <0x10>;
 *        BiscuitOS-name = "BiscuitOS";
 *
 *        DTS_demo {
 *                compatible = "DTS_demo, BiscuitOS";
 *                status = "okay";
 *        };
 * };
 *
 * On Core dtsi:
 *
 * include "DTS_demo.dtsi"
 */

#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/of_platform.h>
#include <linux/of_fdt.h>

/* define name for device and driver */
#define DEV_NAME "DTS_demo"

/* probe platform driver */
static int DTS_demo_probe(struct platform_device *pdev)
{
    struct device_node *np = pdev->dev.of_node;
    unsigned long dt_root;
    unsigned int *dt_int;
    const char *dt_char;
    
    printk("DTS demo probe entence\n");

    /* find the root node in the flat blob */
    dt_root = of_get_flat_dt_root();

    /* Obtain int property */
    dt_int = of_get_flat_dt_prop(dt_root, "BiscuitOS-int", NULL);
    /* Obtain char * property */
    dt_char = of_get_flat_dt_prop(dt_root, "BiscuitOS-name", NULL);

    if (dt_int > 0)
        printk("BiscuitOS-int property: %#x\n", *dt_int);

    if (dt_char)
        printk("BiscuitOS-name property: %s\n", dt_char);

    return 0;
}

/* remove platform driver */
static int DTS_demo_remove(struct platform_device *pdev)
{
    return 0;
}

static const struct of_device_id DTS_demo_of_match[] = {
    { .compatible = "DTS_demo, BiscuitOS", },
    { },
};
MODULE_DEVICE_TABLE(of, DTS_demo_of_match);

/* platform driver information */
static struct platform_driver DTS_demo_driver = {
    .probe  = DTS_demo_probe,
    .remove = DTS_demo_remove,
    .driver = {
        .owner = THIS_MODULE,
        .name = DEV_NAME, /* Same as device name */
        .of_match_table = DTS_demo_of_match,
    },
};
module_platform_driver(DTS_demo_driver);
{% endhighlight %}

编写好驱动之后，将其编译进内核。编译内核和 dts，如下命令：

{% highlight bash %}
make ARCH=arm BiscuitOS/output/linux-4.20.8/arm-linux-gnueabi/arm-linux-gnueabi/bin/arm-linux-gnueabi- j8
make ARCH=arm BiscuitOS/output/linux-4.20.8/arm-linux-gnueabi/arm-linux-gnueabi/bin/arm-linux-gnueabi- dtbs
{% endhighlight %}

启动内核，在启动阶段就会运行驱动的 probe 函数，并打印如下信息：

{% highlight bash %}
[    3.553142] io scheduler cfg registered (default)
[    3.553439] DTS demo probe entence
[    3.553443] BiscuitOS-int property: 0x40302010
[    3.553449] BiscuitOS-name property: BiscuitOS
{% endhighlight %}

从实际运行的情况可以知道，of_get_flat_dt_prop() 函数只能读取 root 节点的属性，
而且对于 int 属性，其按大段读取数据，所以实践应用中应该注意大小端问题。为了解
决大小端问题，可以使用 of_read_number() 函数，使用如下：

{% highlight c %}
/* probe platform driver */
static int DTS_demo_probe(struct platform_device *pdev)
{
    struct device_node *np = pdev->dev.of_node;
    unsigned long dt_root;
    const __be32 *dt_int;
    const char *dt_char;
    u64 value;int len;
    
    printk("DTS demo probe entence\n");

    /* find the root node in the flat blob */
    dt_root = of_get_flat_dt_root();

    /* Obtain int property */
    dt_int = of_get_flat_dt_prop(dt_root, "BiscuitOS-int", &len);
    /* Obtain char * property */
    dt_char = of_get_flat_dt_prop(dt_root, "BiscuitOS-name", NULL);

    if (dt_int > 0) {
        value = of_read_number(dt_int, len / 4);
        printk("BiscuitOS-int property: %#llx\n", (unsigned long long)value);
    }

    if (dt_char)
        printk("BiscuitOS-name property: %s\n", dt_char);

    return 0;
}
{% endhighlight %}

系统运行如下：

{% highlight bash %}
[    3.553142] io scheduler cfg registered (default)
[    3.553439] DTS demo probe entence
[    3.553443] BiscuitOS-int property: 0x10203040
[    3.553449] BiscuitOS-name property: BiscuitOS
{% endhighlight %}

-------------------------------------------------

# <span id="附录">附录</span>

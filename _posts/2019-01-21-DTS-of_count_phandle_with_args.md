---
layout: post
title:  "of_count_phandle_with_args"
date:   2019-01-21 17:26:30 +0800
categories: [HW]
excerpt: DTS API of_count_phandle_with_args().
tags:
  - DTS
---

![DTS](https://raw.githubusercontent.com/EmulateSpace/PictureSet/master/BiscuitOS/kernel/DEV000106.jpg)

> Github: [of_count_phandle_with_args](https://github.com/BiscuitOS/HardStack/tree/master/Device-Tree/kernel/API/of_count_phandle_with_args)
>
> Email: BuddyZhang1 <buddy.zhang@aliyun.com>

# 目录

> - [源码分析](#源码分析)
>
> - [实践](#实践)
>
> - [附录](#附录)

-----------------------------------

# <span id="源码分析">源码分析</span>

##### of_count_phandle_with_args

{% highlight ruby %}
of_count_phandle_with_args
|
|---__of_parse_phandle_with_args
    |
    |---of_get_property
    |
    |---of_find_node_by_phandle
    |
    |---of_property_read_u32
    |
    |---of_node_put
{% endhighlight %}

函数作用：获得节点 phandle 列表中 phandle 的数量

平台： ARM64
linux： 3.10/4.14

{% highlight ruby %}
/**
* of_count_phandle_with_args() - Find the number of phandles references in a property
* @np:        pointer to a device tree node containing a list
* @list_name:    property name that contains a list
* @cells_name:    property name that specifies phandles' arguments count
*
* Returns the number of phandle + argument tuples within a property. It
* is a typical pattern to encode a list of phandle and variable
* arguments into a single property. The number of arguments is encoded
* by a property in the phandle-target node. For example, a gpios
* property would contain a list of GPIO specifies consisting of a
* phandle and 1 or more arguments. The number of arguments are
* determined by the #gpio-cells property in the node pointed to by the
* phandle.
*/
int of_count_phandle_with_args(const struct device_node *np, const char *list_name,
                const char *cells_name)
{
    return __of_parse_phandle_with_args(np, list_name, cells_name, -1, NULL);
}
EXPORT_SYMBOL(of_count_phandle_with_args);
{% endhighlight %}

参数 np 指向当前节点；list_name 指向节点中 phandle 列表的属性名；cells_name 参
数指明 phandle 指向的节点所含的 cells 个数。函数直接调用 
__of_parse_phandle_with_args() 函数，然后返回。

##### __of_parse_phandle_with_args

函数较长，分段分析。

{% highlight ruby %}
/**
* of_parse_phandle_with_args() - Find a node pointed by phandle in a list
* @np:        pointer to a device tree node containing a list
* @list_name:    property name that contains a list
* @cells_name:    property name that specifies phandles' arguments count
* @index:    index of a phandle to parse out
* @out_args:    optional pointer to output arguments structure (will be filled)
*
* This function is useful to parse lists of phandles and their arguments.
* Returns 0 on success and fills out_args, on error returns appropriate
* errno value.
*
* Caller is responsible to call of_node_put() on the returned out_args->node
* pointer.
*
* Example:
*
* phandle1: node1 {
*     #list-cells = <2>;
* }
*
* phandle2: node2 {
*     #list-cells = <1>;
* }
*
* node3 {
*     list = <&phandle1 1 2 &phandle2 3>;
* }
*
* To get a device_node of the `node2' node you may call this:
* of_parse_phandle_with_args(node3, "list", "#list-cells", 1, &args);
*/
static int __of_parse_phandle_with_args(const struct device_node *np,
                    const char *list_name,
                    const char *cells_name, int index,
                    struct of_phandle_args *out_args)
{
    const __be32 *list, *list_end;
    int rc = 0, size, cur_index = 0;
    uint32_t count = 0;
    struct device_node *node = NULL;
    phandle phandle;
{% endhighlight %}

参数 np 指向当前节点；list_name 指向节点中 phandle 列表的属性名；cells_name 参
数指明 phandle 指向的节点所含的 cells 个数；index 表示 phandle 列表的索引，0 
代表第一个 phandle，1 代表第二个 phandle；out_args 参数用于存储 phandle 中的参
数。

{% highlight ruby %}
    /* Retrieve the phandle list property */
    list = of_get_property(np, list_name, &size);
    if (!list)
        return -ENOENT;
    list_end = list + size / sizeof(*list);
{% endhighlight %}

调用函数 of_get_property() 函数获得当前节点的 phandle list 属性的值，存储到 
list 变量，然后计算 phandle list 属性结束的值。

{% highlight ruby %}
    /* Loop over the phandles until all the requested entry is found */
    while (list < list_end) {
        rc = -EINVAL;
        count = 0;

        /*
         * If phandle is 0, then it is an empty entry with no
         * arguments.  Skip forward to the next entry.
         */
        phandle = be32_to_cpup(list++);
        if (phandle) {
            /*
             * Find the provider node and parse the #*-cells
             * property to determine the argument length
             */
            node = of_find_node_by_phandle(phandle);
            if (!node) {
                pr_err("%s: could not find phandle\n",
                     np->full_name);
                goto err;
            }
            if (of_property_read_u32(node, cells_name, &count)) {
                pr_err("%s: could not get %s for %s\n",
                     np->full_name, cells_name,
                     node->full_name);
                goto err;
            }

            /*
             * Make sure that the arguments actually fit in the
             * remaining property data length
             */
            if (list + count > list_end) {
                pr_err("%s: arguments longer than property\n",
                     np->full_name);
                goto err;
            }
        }
{% endhighlight %}

然后遍历节点 phandle list 里面的 cells，每遍历依次，只要 phandle 有效，就调用 
of_find_node_by_phandle() 函数获得 phandle 对应的节点，然后读取该节点中 
cells_name 名字对应的属性值，存储在 count 变量中。如果 list + count 的值越界，
那么判定位越界。

{% highlight ruby %}
        /*
         * All of the error cases above bail out of the loop, so at
         * this point, the parsing is successful. If the requested
         * index matches, then fill the out_args structure and return,
         * or return -ENOENT for an empty entry.
         */
        rc = -ENOENT;
        if (cur_index == index) {
            if (!phandle)
                goto err;

            if (out_args) {
                int i;
                if (WARN_ON(count > MAX_PHANDLE_ARGS))
                    count = MAX_PHANDLE_ARGS;
                out_args->np = node;
                out_args->args_count = count;
                for (i = 0; i < count; i++)
                    out_args->args[i] = be32_to_cpup(list++);
            } else {
                of_node_put(node);
            }

            /* Found it! return success */
            return 0;
        }

        of_node_put(node);
        node = NULL;
        list += count;
        cur_index++;
    }
{% endhighlight %}

cur_index 和 index 的比较确保了正在读取指定的 phandle。如果 out_args 存在，那
么函数将 phandle 对应的参数都存储在 out_args 的 args 数组里，然后返回；否则调
用 of_node_put() 函数，释放节点的使用权；如果不是需要找的 phandle，那么继续遍
历下一个。

{% highlight ruby %}
    /*
     * Unlock node before returning result; will be one of:
     * -ENOENT : index is for empty phandle
     * -EINVAL : parsing error on data
     * [1..n]  : Number of phandle (count mode; when index = -1)
     */
    rc = index < 0 ? cur_index : -ENOENT;
err:
    if (node)
        of_node_put(node);
    return rc;
}
{% endhighlight %}

函数的错误处理。

##### of_find_node_by_phandle

{% highlight ruby %}
/**
* of_find_node_by_phandle - Find a node given a phandle
* @handle:    phandle of the node to find
*
* Returns a node pointer with refcount incremented, use
* of_node_put() on it when done.
*/
struct device_node *of_find_node_by_phandle(phandle handle)
{
    struct device_node *np;
    unsigned long flags;

    raw_spin_lock_irqsave(&devtree_lock, flags);
    for (np = of_allnodes; np; np = np->allnext)
        if (np->phandle == handle)
            break;
    of_node_get(np);
    raw_spin_unlock_irqrestore(&devtree_lock, flags);
    return np;
}
EXPORT_SYMBOL(of_find_node_by_phandle);
{% endhighlight %}

参数 handle 指向节点中 phandle 的属性值。

函数首先调用 raw_spin_lock_irqsave() 函数加锁。由于 DTB 将所有节点都存放在 
of_allnodes 为表头的单链表里，然后调用 for 循环遍历所有节点。每次遍历一个节点，
如果节点 device_node 的 phandle 成员和遍历到的节点一致，那么就找到 phandle 对
应的节点。接着停止 for 循环，调用 of_node_get() 函数添加节点引用数。最后返回 
device_node 之前调用 raw_spin_unlock_irqrestore() 函数释放锁。

##### of_property_read_u32_array

{% highlight ruby %}
/**
* of_property_read_u32_array - Find and read an array of 32 bit integers
* from a property.
*
* @np:        device node from which the property value is to be read.
* @propname:    name of the property to be searched.
* @out_value:    pointer to return value, modified only if return value is 0.
* @sz:        number of array elements to read
*
* Search for a property in a device node and read 32-bit value(s) from
* it. Returns 0 on success, -EINVAL if the property does not exist,
* -ENODATA if property does not have a value, and -EOVERFLOW if the
* property data isn't large enough.
*
* The out_value is modified only if a valid u32 value can be decoded.
*/
int of_property_read_u32_array(const struct device_node *np,
                   const char *propname, u32 *out_values,
                   size_t sz)
{
    const __be32 *val = of_find_property_value_of_size(np, propname,
                        (sz * sizeof(*out_values)));

    if (IS_ERR(val))
        return PTR_ERR(val);

    while (sz--)
        *out_values++ = be32_to_cpup(val++);
    return 0;
}
EXPORT_SYMBOL_GPL(of_property_read_u32_array);
{% endhighlight %}

参数 np 指向当前节点；propbane 参数指向属性的名字；out_values 参数用于存储整形
数组的指针；sz 参数用于执行需要读取 32 位整形的数量。

函数首先调用 of_find_property_value_of_size() 函数获得属性对应的属性值。然后进
行判断之后，将每个属性值存储到 out_values 中。

##### of_find_property_value_of_size

{% highlight ruby %}
/**
* of_find_property_value_of_size
*
* @np:        device node from which the property value is to be read.
* @propname:    name of the property to be searched.
* @len:    requested length of property value
*
* Search for a property in a device node and valid the requested size.
* Returns the property value on success, -EINVAL if the property does not
*  exist, -ENODATA if property does not have a value, and -EOVERFLOW if the
* property data isn't large enough.
*
*/
static void *of_find_property_value_of_size(const struct device_node *np,
            const char *propname, u32 len)
{
    struct property *prop = of_find_property(np, propname, NULL);

    if (!prop)
        return ERR_PTR(-EINVAL);
    if (!prop->value)
        return ERR_PTR(-ENODATA);
    if (len > prop->length)
        return ERR_PTR(-EOVERFLOW);

    return prop->value;
}
{% endhighlight %}

参数 np 指向当前节点；propname 指向属性的名字，len 参数表示属性的长度。

函数首先调用 of_find_property() 函数获得属性，然后分别判断属性，属性的值，以及
属性的长度是否都符合要求。当条件都满足之后，返回属性的值。

##### of_find_property

{% highlight ruby %}
struct property *of_find_property(const struct device_node *np,
                  const char *name,
                  int *lenp)
{
    struct property *pp;
    unsigned long flags;

    raw_spin_lock_irqsave(&devtree_lock, flags);
    pp = __of_find_property(np, name, lenp);
    raw_spin_unlock_irqrestore(&devtree_lock, flags);

    return pp;
}
EXPORT_SYMBOL(of_find_property);
{% endhighlight %}

参数 np 指向一个 device_node；name 参数表示需要查看属性的名字；lenp 用于存储属
性长度。

函数通过调用 raw_spin_lock_irqsave() 函数加锁之后，就直接调用 
__of_find_property() 函数。__of_find_property() 函数用于查找并返回属性名字与 
name 一致的属性。调用完 __of_find_property() 函数之后，调用 
raw_spin_unlock_irqrestore() 函数解锁。最后返回 property 结构。

##### __of_find_property

{% highlight ruby %}
static struct property *__of_find_property(const struct device_node *np,
                       const char *name, int *lenp)
{
    struct property *pp;

    if (!np)
        return NULL;

    for (pp = np->properties; pp; pp = pp->next) {
        if (of_prop_cmp(pp->name, name) == 0) {
            if (lenp)
                *lenp = pp->length;
            break;
        }
    }

    return pp;
}
{% endhighlight %}

由于每个 device_node 包含一个名为 properties 的成员，properties 是一个单链表的
表头，这个单链表包含了该节点的所有属性，函数通过使用 for 循环遍历这个链表，以
此遍历节点所包含的所有属性。每遍历一个属性就会调用 of_prop_cmp() 函数，
of_prop_cmp() 函数用于对比两个字符串是否相等，如果相等返回 0. 因此，当遍历到的
属性的名字与参数 name 一致，那么定义为找到了指定的属性。此时，如果 lenp 不为 
NULL，那么会将属性的长度即 length 存储到 lenp 指向的地址。

----------------------------------------

# <span id="实践">实践</span>

实践目的是在 DTS 文件中构建三个私有节点，第一个私有节点通过 phandle 的方式引用
了第二个和第三个节点，节点二和节点三都存储在第一个节点的 phandle list 属性中，
然后通过 of_count_phandle_with_args() 函数统计 phandle list 中 phandle 的数量，
函数定义如下：

{% highlight ruby %}
int of_count_phandle_with_args(const struct device_node *np, const char *list_name,
                const char *cells_name)
{% endhighlight %}

这个函数经常用用于从节点的 phandle list 中读取 phandle 对应的节点

#### DTS 文件

由于使用的平台是 ARM64，所以在源码 /arch/arm64/boot/dts 目录下创建一个 DTSI 文
件，在 root 节点之下创建一个名为 DTS_demo 的子节点。节点默认打开。再创建两个节
点，节点的 cells 分别是 3 和 2，具体内容如下：

{% highlight ruby %}
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
        DTS_demo {
                compatible = "DTS_demo, BiscuitOS";
                status = "okay";
                phy-handle = <&phy0 1 2 3 &phy1 4 5>;
        };

        phy0: phy@0 {
                #phy-cells = <3>;
                compatible = "PHY0, BiscuitOS";
        };

        phy1: phy@1 {
                #phy-cells = <2>;
                compatible = "PHY1, BiscuitOS";
        };
};
{% endhighlight %}

创建完毕之后，将其保存并命名为 DTS_demo.dtsi。然后开发者找到系统默认的 DTS 文
件，比如当前编译使用的 DTS 文件为 XXX.dtsi，然后在 XXX.dtsi 文件开始地方添加如
下内容：

{% highlight ruby %}
#include "DTS_demo.dtsi"
{% endhighlight %}

#### 编写 DTS 对应驱动

准备好 DTSI 文件之后，开发者编写一个简单的驱动，这个驱动作为 DTS_demo 的设备驱
动，在 DTS 机制遍历时会调用匹配成功的驱动，最终运行驱动里面的代码。在驱动的 
probe 函数中，首先获得驱动所对应的节点，通过 platform_device 的 of_node 成员传
递。获得驱动对应的节点之后，通过调用 of_count_phandle_with_args() 函数统计当前 
phandle list 属性中有几个 phandle。驱动编写如下：

{% highlight ruby %}
/*
 * DTS: of_count_phandle_with_args
 *
 * int of_count_phandle_with_args(const struct device_node *np,
 *                    const char *list_name, const char *cells_name)
 *
 * (C) 2019.01.11 BuddyZhang1 <buddy.zhang@aliyun.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

/*
 * Private DTS file: DTS_demo.dtsi
 *
 * / {
 *        DTS_demo {
 *                compatible = "DTS_demo, BiscuitOS";
 *                status = "okay";
 *                phy-handle = <&phy0 1 2 3 &phy1 4 5>;
 *        };
 *
 *        phy0: phy@0 {
 *                #phy-cells = <3>;
 *                compatible = "PHY0, BiscuitOS";
 *        };
 *
 *        phy1: phy@1 {
 *                #phy-cells = <2>;
 *                compatible = "PHY1, BiscuitOS";
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

/* define name for device and driver */
#define DEV_NAME "DTS_demo"

/* probe platform driver */
static int DTS_demo_probe(struct platform_device *pdev)
{
    struct device_node *np = pdev->dev.of_node;
    struct of_phandle_args args;
    int rc, index = 0;
    const u32 *comp;

    printk("DTS demo probe entence.\n");

    /* Read first phandle argument */
    rc = of_count_phandle_with_args(np, "phy-handle", "#phy-cells");
    if (rc < 0) {
        printk("Unable to parse phandle.\n");
        return -1;
    }
    printk("Number phandle: %#x\n", rc);
    
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

{% highlight ruby %}
make ARCH=arm64
make ARCH=arm64 dtbs
{% endhighlight %}

启动内核，在启动阶段就会运行驱动的 probe 函数，并打印如下信息：

{% highlight ruby %}
[    3.534323] DTS demo probe entence.
[    3.534359] Number phandle: 2
{% endhighlight %}

-----------------------------------------------

#<span id="附录">附录</span>


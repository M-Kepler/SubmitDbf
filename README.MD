[TOC]

## 说明

* makefile

```
GeneSql.cpp 和 HandleData.cpp 代码一样, 只是把与数据库相关的注释掉, 方便在没有oracle的环境下编译
如果在有oracle的环境下, 要编译 HandleData.cpp, 要先把GeneSql.cpp 先备份到其他地方
```


* DbfRead.cpp 提供了三个处理方法, 下面是速度比较:

```
/*
 * 测试数据: 1.7G 2700w条记录的dbf文件测试耗时

 【不使用mmap, 单纯使用fstream读文件】
 * $ date && ./mainnommap.out gene ../../dbf/qtzhzl100077.b09 && date
 * Thu Jan 10 11:15:29 DST 2019
 * Consume: 554s
 * Thu Jan 10 11:24:43 DST 2019

 【一次性把整个文件映射到内存】
 * $ date && ./mainmmaponce.out gene ../../dbf/qtzhzl100077.b09 && date
 * Thu Jan 10 11:06:07 DST 2019
 * Consume: 217s
 * Thu Jan 10 11:09:46 DST 2019

 【分多次映射, 一次映射500M】
 * $ date && ./mainmmapmulti.out gene ../../dbf/qtzhzl100077.b09 && date
 * Thu Jan 10 11:11:09 DST 2019
 * Consume: 184s
 * Thu Jan 10 11:14:13 DST 2019
 */
```


## 使用方法

* 配置 config.ini

* 仅生成sql文件

```
$ ./main.out gene /path/to/dbf
```

* 仅运行sql文件

```
$ ./main.out run
```

* 一步操作

```
$ ./main.out batch /path/to/dbf
```


# 注意

* 找不到libCommondao.so的解决方法

  修改.bash_profile 添加把libCommondao.so路径添加到LD_LIBRARY_PATH




# 任务清单

## DONE

* 从dbf二进制文件中读取文件信息

* 根据dbf文件头信息拆解文件, 根据配置信息组装sql语句

* 插入oracle数据库


## TODO


[]

0. 用char[]去存是个大坑, 应该用上uint8_t、uint_16_t这些类型的
在后面的计算中很痛苦, 要先从字段配置string中解析出字段值如 LEN:20中的20
再把20存放到stFieldHead.szLen中, 所以要把string转int, 再用memcpy把这个值拷贝到地址上
因为string类不能用memcpy、memset等内存操作的函数(因为string是个类, 类里还有别的占用空间的东西)


1. AppendRec的时候要从string中读取到Name的值, 然后写入到dbf文件中
因为定义头文件的时候没有用uint8_t这些类型,所以在后面的计算加减中要频繁地转换,
比如szLen[4], 长度要+1, 要通过memcpy来拷贝到一个整形上进行计算, 计算完在把整形拷会szLen
如果是保存长度的只有一个字节szLen[1], 要把这一个字节的数据进行加减计算, 那么用memcpy的时候就要注意拷贝的长度了

2. 记录的长度recsize+1是因为记录的第一个字节用来存删除标记位

3. 新纪录的数据是memcpy到一个char*上, 根据字段配置里的偏移量来拷贝到对应的地址上

4. 之所以用find("NAME:"), 是因为如果NAME:ID, 那么这个ID和ID属性会冲突, 解析ID:1的时候会有问题, 所以改为去找NAME:


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



[TOC]

## 说明

* makefile
GeneSql.cpp 和 HandleData.cpp 代码一样, 只是把与数据库相关的注释掉, 方便在没有oracle的环境下编译 
`make`的默认命令就是编译该文件


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



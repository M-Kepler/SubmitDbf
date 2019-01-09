[TOC]

## 说明

* makefile
```
makefile_genq 是单纯为编译生成sql的代码而写的, 不涉及数据库, 可直接make
makefile_all  包括生成 sql 文件和插入数据两个步骤, 需要安装了oracle, 且配置了 libCommondao.so 的环境才能编译完成
编译前把文件改名为 makefile
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



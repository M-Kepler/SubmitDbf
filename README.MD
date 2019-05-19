- [说明](#说明)
- [使用方法](#使用方法)
- [其他](#其他)
- [注意](#注意)
- [任务清单](#任务清单)
  - [TODO](#todo)
  - [DONE](#done)

# 说明

# 使用方法

- 在 `.src/` 下进行编译

  ```sh
  make
  ```

- 生成 sql 文件

  ```sh
  ./main.out dbf2sql ./output/data.dbf
  # 在 ./output/ 下面生成 tmp_data_check_rst_0000.sql
  ```

- dbf 转 csv 文件

  ```sh
  ./main.out dbf2csv ./output/data.dbf
  # 在 ./output/ 下面生成 tmp_data_check_rst_0000.csv
  ```

- csv 转 dbf  

  ```sh
  ./main.out csv2dbf ./output/tmp_data_check_rst_0000.csv
  # 在 ./output 下生成 data.dbf
  ```

- 提交数据库

  ```sh
  ./main.out sqlldr /path/to/data
  # 用了 `sqlldr`，程序自动生成`ctrl`文件，并连接 `Oracal`，执行 `sqllr`命令
  ```

# 其他

- `src/mmap_test/` 下提供了提供了三个处理方法, 下面是速度比较:
  ```cpp
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

# 注意

- 找不到 `libCommondao.so` 的解决方法  
  修改 `.bash_profile` 添加把 `libCommondao.so` 路径添加到 `LD_LIBRARY_PATH`

# 任务清单

## TODO

- [ ] 数据库文件导出 csv、dbf

## DONE

- [x] 用 char[]去存是个大坑, 应该用上 uint8_t、uint_16_t 这些类型的
  - 在后面的计算中很痛苦, 要先从字段配置 string 中解析出字段值如 LEN:20 中的 20
  - 再把 20 存放到 stFieldHead.szLen 中, 所以要把 string 转 int, 再用 memcpy 把这个值拷贝到地址上
  - 因为 string 类不能用 memcpy、memset 等内存操作的函数(因为 string 是个类, 类里还有别的占用空间的东西)

- [x] AppendRec 的时候要从 string 中读取到 Name 的值, 然后写入到 dbf 文件中
  - 因为定义头文件的时候没有用 uint8_t 这些类型,所以在后面的计算加减中要频繁地转换,
  - 比如 szLen[4], 长度要+1, 要通过 memcpy 来拷贝到一个整形上进行计算, 计算完在把整形拷会 szLen
  - 如果是保存长度的只有一个字节 szLen[1], 要把这一个字节的数据进行加减计算, 那么用 memcpy 的时候就要注意拷贝的长度了

- [x] 记录的长度 recsize+1 是因为记录的第一个字节用来存删除标记位

- [x] 新纪录的数据是 memcpy 到一个 char\*上, 根据字段配置里的偏移量来拷贝到对应的地址上

- [x] 之所以用 find("NAME:"), 是因为如果 NAME:ID, 那么这个 ID 和 ID 属性会冲突, 解析 ID:1 的时候会有问题, 所以改为去找 NAME:

- [x] 从 csv 转 dbf 时，配置中 IncAndSplit=0, 生成的 csv 中，如果字段值为 NULL，实际生成会变成 0；可以根据配置生成有双引号包围的 csv

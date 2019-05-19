LOAD DATA
INFILE '../file//tmp_data_check_rst_0000.csv'
INFILE '../file//tmp_data_check_rst_0001.csv'
INFILE '../file//tmp_data_check_rst_0002.csv'
INFILE '../file//tmp_data_check_rst_0003.csv'
INFILE '../file//tmp_data_check_rst_0004.csv'
INFILE '../file//tmp_data_check_rst_0005.csv'
INFILE '../file//tmp_data_check_rst_0006.csv'
APPEND INTO TABLE tmp_data_check_rst
FIELDS TERMINATED BY ","
Optionally enclosed by '"'
TRAILING NULLCOLS
(
     KHJGDM,
     KHWDDM,
       ZJZH,
       YMTH,
       ZQZH,
       SCDM,
      BYZLX,
     ZJHMYY,
     ZJHMSM,
     KHQCYY,
     KHQCSM,
       SJRQ
)

/*
 ****************************************************
 * Author       : M_Kepler
 * EMail        : m_kepler@foxmail.com
 * Last modified: 2018-11-18 22:41:27
 * Filename     : dbf_read.h
 * Description  : 以二进制形式读入dbf文件
 * dbf文件的二进制文件格式 https://blog.csdn.net/xwebsite/article/details/6912146
 ****************************************************
*/

#ifndef _DBF_READ_H_
#define _DBF_READ_H_

#include <stdio.h>
#include <string>
#include <vector>

// dbf文件中存储的字段信息
typedef struct stFieldHead
{
    char szName[11];  // 0 - 10  字段名称
    char szType[1];   // 11      字段的数据类型，C字符、N数字、D日期、B二进制、等
    char szResv[4];   // 12 - 15 保留字节, 默认为0
    char szLen[1];    // 16      字段长度
    char szJingDu[1]; // 17      字段精度
    char szResv2[2];  // 18 - 19 保留字节, 默认为0
    char szId[1];     // 20      工作区ID
    char szResv3[11]; // 21 - 31 保留字节, 默认为0
} stFieldHead;

typedef void (*pCallback)(std::vector<stFieldHead> vecFieldHead, char *pcszContent);

class CDbfRead
{
  public:
    CDbfRead(const std::string &strFile);
    ~CDbfRead(void);

  public:
    /*
     * @brief:  读入dbf文件, 并解析dbf文件头信息
     *          并把字段信息保存到 m_vecFieldHead
     */
    int ReadHead();

    /*
     * @brief:  读入dbf文件, 并进行文件内存映射, 读取每行数据记录给pfn处理
     * @param:  pfn     函数指针 
     * @return:
     */
    int Read(pCallback pfn, int nPageNum = 128000);

    int GetRecordNum()
    {
        return m_nRecordNum;
    }

    int GetRecordSize()
    {
        return m_sRecordSize;
    }

    std::vector<stFieldHead> &GetFieldArray()
    {
        return m_vecFieldHead;
    }

  private:
    std::string m_strFile;

    /* dbf二进制文件中的记录条数 */
    int m_nRecordNum;
    /* dbf二进制文件的文件头中字节数 */
    short m_sFileHeadBytesNum;
    /* 一条记录中的字节长度,每行数据所占长度 */
    short m_sRecordSize;
    /* 从dbf文件头中解析出的每个字段信息 */
    std::vector<stFieldHead> m_vecFieldHead;
};

#endif

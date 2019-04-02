/*
 ****************************************************
 * Author       : M_Kepler
 * EMail        : m_kepler@foxmail.com
 * Last modified: 2018-11-18 22:41:27
 * Filename     : dbf_read.h
 * Description  : 以二进制形式读入dbf文件
    * dbf文件的二进制文件格式:  http://www.xumenger.com/dbf-20160703/
    * https://github.com/bramhe/DBFEngine
    * 编辑器中看到的的十六进制是: 9f17 对应的十进制应该是6047(即十六进制179f对应的十进制)
    * 之所以要从右往左读是因为内存中存储数据采用的是"小端模式",数据的低位存在前面，高位存在后面
 ****************************************************
*/

#ifndef _DBF_READ_H_
#define _DBF_READ_H_

#include <stdio.h>
#include <string>
#include <vector>
#include <iostream>
using namespace std;

// dbf文件文件头信息
typedef struct stDBFHead
{
    char szMark[1];          // 版本信息
    char szYear[1];
    char szMonth[1];
    char szDay[1];
    char szRecCount[4];    // 4字节保存记录数
    char szDataOffset[2];  // 2字节保存文件头字节数
    char szRecSize[2];     // 2字节保存每行数据的长度
    char Reserved[20];
} stDbfHead, *LPDBFHEAD;

// dbf文件中存储的字段信息
typedef struct stFieldHead
{
    char szName[11];        // 0  - 10  字段名称
    char szType[1];         // 11       字段的数据类型，C字符、N数字、D日期、B二进制、等
    char szOffset[4];         // 12 - 15  字段偏移量, 默认为0
    char szLen[1];          // 16       字段长度
    char szPrecision[1];    // 17       字段精度
    char szResv2[2];        // 18 - 19  保留字节, 默认为0
    char szId[1];           // 20       工作区ID
    char szResv3[11];       // 21 - 31  保留字节, 默认为0
} stFieldHead;

typedef void (*pCallback)(std::vector<stFieldHead> vecFieldHead, char *pcszContent);

class CDbfRead
{
  public:
    CDbfRead(){};
    CDbfRead(const std::string &strFile);
    ~CDbfRead(void);

  public:
    /*
     * @brief:  并解析dbf文件头信息,
     *          把 dbf 文件属性  如记录数、文件头字节等保存到私有变量
     *          把 dbf 字段属性 保存到 m_vecFieldHead
     */
    int ReadHead();

    /*
     * @brief:  读入dbf文件, 并进行文件内存映射, 读取每行数据记录给pfn处理
     *          耗时: 184s
     * @param:  pfn         函数指针
     * @param:  nPageNum    申请12800个页的内存大小; 12800 * 4k = 500M
     * @return:
     */
    int Read(pCallback pfn, int nPageNum = 128000);

    /*
     * @brief	: 上面的接口以每次500m来映射, 该接口一次全部映射完
     *            耗时: 217s
     * @param	:
     * @return	:
     */
    int ReadMmapOnce(pCallback pfn, int nPageNum = 128000);

    /*
     * @brief	: 不使用映射, 单纯地用fstream去读文件
     *            耗时: 554s
     * @param	:
     * @return	:
     */
    int ReadNoMmap(pCallback pfn);


    /*
     * @brief	: 生成dbf文件
     * @param	: vecFieldHead 字段定义vec
     * @return	: errcode
     */
    int AddHead(std::vector<stFieldHead> vecFieldHead);

    /*
     * @brief	: 插入记录
     * @param	:
     * @return	:
     */
    int AppendRec(std::string *sValues);

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
    FILE *m_newDbf;
    char *m_pRecord;

    int m_nRecordNum;                           // dbf二进制文件中的记录条数
    short m_sFileHeadBytesNum;                  // dbf二进制文件的文件头中字节数
    short m_sRecordSize;                        // 一条记录中的字节长度, 每行数据所占长度
    stDbfHead m_stDbfHead;
    std::vector<stFieldHead> m_vecFieldHead;    // 从dbf文件头中解析出的每个字段信息
};

#endif

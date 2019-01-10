#include "DbfRead.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <vector>
#include <string>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>

#include <fstream>
using namespace std;

CDbfRead::CDbfRead(const std::string &strFile)
    : m_nRecordNum(0),
      m_sFileHeadBytesNum(0),
      m_sRecordSize(0)
{
    m_strFile = strFile;
}

CDbfRead::~CDbfRead(void)
{
}

int CDbfRead::ReadHead()
{
    char szHead[32] = {0};
    int fieldLen;   // 数据记录长度
    int filedCount; // 数据记录数量
    char *pField;   // 数据记录开始的位置

    if (m_strFile.empty())
    {
        return -1;
    }

    FILE *pf = fopen(m_strFile.c_str(), "r");
    if (!pf)
    {
        return -1;
    }

    int readNum = fread(szHead, sizeof(char), 32, pf); // 1. dbf二进制文件中前32位为文件头,保存dbf属性等信息
    if (readNum <= 0)
    {
        return -1;
    }

    memcpy(&m_nRecordNum, &szHead[4], 4);        // 从szHead[4]开始读入4个字符到m_nRecordNum
    memcpy(&m_sFileHeadBytesNum, &szHead[8], 2); // dbf二进制文件中文件头字节数
    memcpy(&m_sRecordSize, &szHead[10], 2);      // 每行记录所占空间 + 末尾一位标识符

    fieldLen = m_sFileHeadBytesNum - 32; // 文件头去除前32个描述文件属性的字节后, 得到字段信息的长度
    pField = (char *)malloc(fieldLen);

    if (!pField)
    {
        return -1;
    }

    memset(pField, 0, fieldLen);

    // 从上次读的位置继续往后读
    readNum = fread(pField, sizeof(char), fieldLen, pf);
    if (readNum <= 0)
    {
        return -1;
    }

    // 文件头中前 32 个描述文件属性字节之后直到, 最后一个标记字节之前的都是字段的属性信息
    // 每个字段的属性各占32字节, 标记字节为0d
    filedCount = (fieldLen - 1) / 32;

    char *pTmp = pField;

    // 把每个字段memcpy到同样大小的结构体里
    for (int i = 0; i < filedCount; i++)
    {
        stFieldHead item;
        // 从 pTmp 中读入 sizeof(stFieldHead) 大小的字节到 item 中
        memcpy(&item, pTmp, sizeof(stFieldHead));
        pTmp = pTmp + sizeof(stFieldHead);

        m_vecFieldHead.push_back(item);
    }

    free(pField);

    fclose(pf);

    return 0;
}

int CDbfRead::Read(pCallback pfn, int nPageNum)
{
    // int fd;
    streampos pPos; // 读取位置记录

    fstream file;
    file.open(m_strFile.c_str(), ios::binary | ios::in);
    if (!file.is_open())
    {
        return -1;
    }
    file.seekg(0, ios::beg);
    streampos pMmapBuf = file.tellg();

    file.clear();
    file.seekg(0, ios::end);

    size_t len = file.tellg();

    pPos = pMmapBuf + m_sFileHeadBytesNum + 1;

    int j = 0;
    while (!file.eof())
    {
        char szBuf[2048] = {0};
        file.seekg(pPos + j * m_sRecordSize);
        file.read(szBuf, m_sRecordSize);
        pfn(m_vecFieldHead, szBuf);
        j++;
    }

    file.close();
    return 0;
}

/*
 * 1.7G 2700w条记录的dbf文件测试耗时
 * $ date && ./mainnommap.out gene ../../dbf/qtzhzl100077.b09 && date
 * Thu Jan 10 11:15:29 DST 2019
 * Consume: 554s
 * Thu Jan 10 11:24:43 DST 2019
 */
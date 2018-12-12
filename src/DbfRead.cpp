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
    int fieldLen;   // 字段长度
    int filedCount; // 字段数量
    char* pField;   // 字段开始的位置


    if (m_strFile.empty())
    {
        return -1;
    }

    FILE *pf = fopen(m_strFile.c_str(), "r");
    if (!pf)
    {
        return -1;
    }

    int readNum = fread(szHead, sizeof(char), 32, pf); // 从文件pf中读入32个字符到缓存
    if (readNum <= 0)
    {
        return -1;
    }

    // 从szHead[4]开始读入4个字符到m_nRecordNum
    memcpy(&m_nRecordNum, &szHead[4], 4);
    // dbf二进制文件中文件头字节数
    memcpy(&m_sFileHeadBytesNum, &szHead[8], 2);
    // 每行记录所占空间 + 末尾一位标识符
    memcpy(&m_sRecordSize, &szHead[10], 2);

    fieldLen = m_sFileHeadBytesNum - 32; // 除去文件头前32个字节是dbf文件描述外的是字段
    pField = (char *)malloc(fieldLen);

    if (!pField)
    {
        return -1;
    }

    memset(pField, 0, fieldLen);

    // 从上次读的位置继续读入
    readNum = fread(pField, sizeof(char), fieldLen, pf);
    if (readNum <= 0)
    {
        return -1;
    }

    // 文件头中前 32 字节之后直到
    // 最后一个标记字节之前的都是数据字段的描述
    filedCount = (fieldLen - 1) / 32;

    char *pTmp = pField;

    // 把每列字段push到vector里
    for (int i = 0; i < filedCount; i++)
    {
        stFieldHead item;
        // 从 pTmp 中读入 sizeof(stFieldHead) 大小的字节到 item中
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
    int fd; // 文件描述符
    int mmapCount = 0;
    int tailLen = 0;
    int remainLen = 0;
    char remainBuf[2048] = {0};   // 一条记录被分在A,B页了, 这个缓存存放A页的记录
    char firstRecord[1024] = {0}; // 一条记录被分在A,B页了, 这个缓存存放被分页的记录
    size_t mmapSize;              // 映射需要的内存的大小?
    size_t remainFileSize;        // 文件大小
    size_t totalSize;             // 记录数据的总共大小
    char *pPos = NULL;            // 读取位置记录
    struct stat stat;


    fd = open(m_strFile.c_str(), O_RDONLY, 0);
    if (fd < 0)
    {
        return -1;
    }

    fstat(fd, &stat); // 文件fd结构到stat结构体

    mmapSize = nPageNum * getpagesize(); // getpagesize()获取内存页大小
    mmapCount = stat.st_size / mmapSize;

    // ??? 为什么多出来的一点也要占用一页内存大小
    if (0 != stat.st_size % mmapSize)
    {
        mmapCount = mmapCount + 1;
    }

    if (mmapSize > stat.st_size)
    {
        mmapSize = stat.st_size;
    }

    remainFileSize = stat.st_size;


    for (int i = 0; i < mmapCount; i++)
    {
        void *pMmapBuf = mmap(NULL, mmapSize, PROT_READ, MAP_SHARED, fd, i * mmapSize);
        if (pMmapBuf == (void *)-1)
        {
            return -1;
        }

        totalSize = mmapSize;
        // 这里应该是因为映射文件的时候
        if (mmapSize > remainFileSize)
        {
            totalSize = remainFileSize;
        }

        if (0 == i)
        {
            /*
             * 第一次读的时候要跳过文件头和标记字节
             * 指针指向数据记录真正开始的地方
             * +1 是因为每条记录的第一个字节属于特殊标记字节
             */
            pPos = (char *)pMmapBuf + m_sFileHeadBytesNum + 1;
            totalSize = totalSize - m_sFileHeadBytesNum;
        }
        else
        {
            pPos = (char *)pMmapBuf + 1;
        }

        // 第一次读取的时候 和 记录没有被分在两页的情况
        if (0 == remainLen)
        {
            int j = 0;

            // 循环读入一行数据送给函数(指针)做处理
            while (totalSize >= m_sRecordSize)
            {
                char szBuf[2048] = {0};
                memcpy(szBuf, pPos + j * m_sRecordSize, m_sRecordSize);
                // pfn(szBuf);
                pfn(m_vecFieldHead, szBuf);
                j++;
                totalSize = totalSize - m_sRecordSize;
            }

            // 一条记录被分在A, B两页了
            if (totalSize > 0)
            {
                memcpy(remainBuf, pPos + j * m_sRecordSize, totalSize);
                remainLen = totalSize;
            }
        }
        else
        {
            // 特殊处理记录被分页截断的情况
            // firstRecord[1024] = {0};
            memset(firstRecord, 0x00, 1024);
            tailLen = m_sRecordSize - remainLen;
            memcpy(firstRecord, remainBuf, remainLen);
            memcpy(firstRecord + remainLen, pPos, tailLen);
            pfn(m_vecFieldHead, firstRecord);
            // pfn(firstRecord);

            pPos = pPos + tailLen;
            totalSize = totalSize - tailLen;

            int j = 0;
            while (totalSize >= m_sRecordSize)
            {
                char szBuf[2048] = {0};
                memcpy(szBuf, pPos + j * m_sRecordSize, m_sRecordSize);
                // pfn(szBuf);
                pfn(m_vecFieldHead, szBuf);
                totalSize = totalSize - m_sRecordSize;
                j++;
            }

            if (totalSize > 0)
            {
                memcpy(remainBuf, pPos + j * m_sRecordSize, totalSize);
                remainLen = totalSize;
            }
        }

        remainFileSize = remainFileSize - mmapSize;

        munmap(pMmapBuf, mmapSize);
    }

    close(fd);

    return 0;
}


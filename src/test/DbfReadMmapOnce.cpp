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
    int fd;
    int mmapCount = 0; // 根据文件大小定映射次数, ??? 为什么不一次映射完
    char *pPos = NULL; // 读取位置记录
    struct stat stat;
    size_t mmapSize;              // 一次映射的长度大小, 必须是页的整数倍;不满一页也要分配一页空间
                                  // 虽然多出来的空间会清零,32位机器上1页是4k大小
    size_t totalSize;             // 保存一次映射中未处理的数据记录的大小
    size_t remainFileSize;        // dbf文件的未映射到内存区域的大小
    int remainLen = 0;            // 一条记录被分在A,B页, 保存被截断的记录在A页页尾的长度
    int tailLen = 0;              // 一条记录被分在A,B页, 保存被截断的记录在B页页头的长度
    char remainBuf[2048] = {0};   // 一条记录被分在A,B页, 保存被截断的那条记录在A页中的数据
    char firstRecord[1024] = {0}; // 一条记录被分在A,B页, 保存被截断的那条记录

    fd = open(m_strFile.c_str(), O_RDONLY, 0);
    if (fd < 0)
    {
        return -1;
    }

    fstat(fd, &stat); // 文件fd结构到stat结构体

    // mmapSize = nPageNum * getpagesize(); // getpagesize()获取内存页大小, 这里一次映射500M
    // mmapCount = stat.st_size / mmapSize; // 已经知道文件大小了, 为什么不直接根据大小进行合理映射呢

    size_t pagesize = getpagesize();
    size_t size;

    if (0 != stat.st_size % pagesize)
    {
        size = ((stat.st_size / pagesize) + 1) * pagesize;
    }

    // 映射到内存的大小和文件的大小
    // if (0 != stat.st_size % mmapSize)
    // {
    //     mmapCount = mmapCount + 1;
    // }

    // if (mmapSize > stat.st_size)
    // {
    //     mmapSize = stat.st_size;
    // }

    // remainFileSize = stat.st_size;
    void *pMmapBuf = mmap(NULL, size, PROT_READ, MAP_SHARED, fd, 0);
    if (pMmapBuf == (void *)-1)
    {
        return -1;
    }

    totalSize = stat.st_size;
    /*
     * 第一次读的时候要跳过文件头和末尾的标记字节, 指针指向数据记录真正开始的地方
     * +1 是因为每条记录的第一个字节属于特殊标记字节
     */
    pPos = (char *)pMmapBuf + m_sFileHeadBytesNum + 1;
    totalSize = totalSize - m_sFileHeadBytesNum; // 总大小减去文件头后就是数据记录的大小

    int j = 0;
    while (totalSize >= m_sRecordSize)
    {
        char szBuf[2048] = {0};
        memcpy(szBuf, pPos + j * m_sRecordSize, m_sRecordSize);
        // pfn(szBuf);
        pfn(m_vecFieldHead, szBuf);
        j++;
        totalSize = totalSize - m_sRecordSize;
    }

    // 退出循环: 读取映射到内存的数据最后面一部分的时候, 发现比一条记录的长度要小
    // 一条记录被分在A, B两页了
    // if (totalSize > 0)
    // {
    //     memcpy(remainBuf, pPos + j * m_sRecordSize, totalSize);
    //     remainLen = totalSize;
    // }
    munmap(pMmapBuf, mmapSize);

    /*
    for (int i = 0; i < mmapCount; i++)
    {
        void *pMmapBuf = mmap(NULL, mmapSize, PROT_READ, MAP_SHARED, fd, i * mmapSize);
        if (pMmapBuf == (void *)-1)
        {
            return -1;
        }

        totalSize = mmapSize;

        if (mmapSize > remainFileSize)
        {
            totalSize = remainFileSize;
        }

        if (0 == i)
        {
            /*
             * 第一次读的时候要跳过文件头和末尾的标记字节, 指针指向数据记录真正开始的地方
             * +1 是因为每条记录的第一个字节属于特殊标记字节
             *
            pPos = (char *)pMmapBuf + m_sFileHeadBytesNum + 1;
            totalSize = totalSize - m_sFileHeadBytesNum; // 总大小减去文件头后就是数据记录的大小
        }
        else
        {
            pPos = (char *)pMmapBuf + 1;
        }

        // 第一次读取的时候 和 记录没有被截断在A,B页的情况
        if (0 == remainLen)
        {
            int j = 0;

            // 如果恰好的话, totalSize 应该是 m_sRecordSize 的整数倍
            while (totalSize >= m_sRecordSize)
            {
                char szBuf[2048] = {0};
                memcpy(szBuf, pPos + j * m_sRecordSize, m_sRecordSize);
                // pfn(szBuf);
                pfn(m_vecFieldHead, szBuf);
                j++;
                totalSize = totalSize - m_sRecordSize;
            }

            // 退出循环: 读取映射到内存的数据最后面一部分的时候, 发现比一条记录的长度要小
            // 一条记录被分在A, B两页了
            if (totalSize > 0)
            {
                memcpy(remainBuf, pPos + j * m_sRecordSize, totalSize);
                remainLen = totalSize;
            }
        }
        else
        {
            // 由于被截断, 把A页页尾和B页页头的数据拷贝到一起
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
    */
    close(fd);

    return 0;
}

/*
 * 1.7G 2700w条记录的dbf文件测试耗时
 * $ date && ./mainmmaponce.out gene ../../dbf/qtzhzl100077.b09 && date
 * Thu Jan 10 11:06:07 DST 2019
 * Consume: 217s
 * Thu Jan 10 11:09:46 DST 2019
*/
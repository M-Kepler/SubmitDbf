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
      m_sRecordSize(0),
      m_newDbf(NULL)
{
    m_strFile = strFile;
}

CDbfRead::~CDbfRead(void)
{
    if (m_newDbf != NULL)
    {
        fclose(m_newDbf); // 有可能要连续append, 所以在析构的时候再close
        m_newDbf = NULL;
    }
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

int CDbfRead::AddHead(std::vector<stFieldHead> vecField)
{
    time_t now;
    stDbfHead dbfHead;
    int iFieldCnt = 0; // 确定文件头大小需要知道字段数
    short offset = 0;
    short recsize = 0;
    char chFieldEndFlag = 0x0d; // 字段定义终止标志
    char chFileEndFlag = 0x1A;  // 文件结束标志

    m_vecFieldHead = vecField;

    time(&now);
    tm *tp = localtime(&now);

    iFieldCnt = vecField.size();
    offset = (short)(sizeof(stDbfHead) + (iFieldCnt * sizeof(stFieldHead)) + 1);

    for (auto x : vecField)
    {
        recsize += x.szLen[0];
    }
    recsize += 1; // 记录长度+1, 保存记录的删除标记

    // 初始化文件头
    memset(&dbfHead, 0x00, sizeof(stDbfHead));
    dbfHead.szMark[0] = 0x30;
    memmove(&dbfHead.szYear, &tp->tm_year, sizeof(dbfHead.szYear));
    memmove(&dbfHead.szMonth, &tp->tm_mon, sizeof(dbfHead.szMonth));
    memmove(&dbfHead.szDay, &tp->tm_mday, sizeof(dbfHead.szDay));
    memmove(&dbfHead.szDataOffset, &offset, sizeof(dbfHead.szDataOffset));
    memmove(&dbfHead.szRecSize, &recsize, sizeof(dbfHead.szRecSize));

    m_newDbf = fopen("./data.dbf", "wb");
    // 写文件头
    if (fwrite(&dbfHead, sizeof(dbfHead), 1, m_newDbf) < 1)
    {
        fclose(m_newDbf);
        m_newDbf = NULL;
        return -2;
    }

    // 写字段
    for (unsigned i = 0; i < iFieldCnt; ++i)
    {
        if (fwrite(&vecField[i], sizeof(stFieldHead), 1, m_newDbf) < 1)
        {
            fclose(m_newDbf);
            m_newDbf = NULL;
            return -2;
        }
    }

    // 加上字段定义结束标记
    if (fwrite(&chFieldEndFlag, 1, 1, m_newDbf) < 1)
    {
        fclose(m_newDbf);
        m_newDbf = NULL;
        return -2;
    }

    // 加上文件结束标记
    if (fwrite(&chFileEndFlag, 1, 1, m_newDbf) < 1)
    {
        fclose(m_newDbf);
        m_newDbf = NULL;
        return -2;
    }
    fflush(m_newDbf);
    // fclose(m_newDbf); // 先别close, 还要addfield呢
    // m_newDbf = NULL;

    m_stDbfHead = dbfHead;

    return 0;
}

int CDbfRead::updateFileHeader()
{
    int iRes = fseek(m_newDbf,0,SEEK_SET);
    if( iRes != 0)
        return 1;

    // 更新时间
    time_t now;
    time(&now);
    tm *tp = localtime(&now);

    // 初始化文件头
    memmove(&m_stDbfHead.szYear, &tp->tm_year, sizeof(m_stDbfHead.szYear));
    memmove(&m_stDbfHead.szMonth, &tp->tm_mon, sizeof(m_stDbfHead.szMonth));
    memmove(&m_stDbfHead.szDay, &tp->tm_mday, sizeof(m_stDbfHead.szDay));

    int iBytesWritten = fwrite(&m_stDbfHead, 1, sizeof(m_stDbfHead), m_newDbf);
    if( iBytesWritten != sizeof(m_stDbfHead))
    {
        std::cerr << __FUNCTION__ << " Failed to update header!" << std::endl;
        return 1;
    }
    return 0;
}


int CDbfRead::AppendRec(std::string *sValues)
{

    int iRecCount;
    int iOffset;
    short iRecSize;
    uint8_t iLen;
    char chFileEndFlag = 0x1A;  // 文件结束标志
    // TODO 处理csv文件不够字段数的情况

    // FIXME char[4] 转 int // 刚开始先用atoi, 但是有问题, 后来改用memcpy, 长度又有问题, 不能把ch[1]拷贝到int, 长度不对，应该只拷贝1

    // 计算插入记录的位置: 文件头定义 + 字段定义 + 记录长度 + 1标记
    memcpy(&iRecSize, m_stDbfHead.szRecSize, sizeof(m_stDbfHead.szRecSize));
    memcpy(&iRecCount, m_stDbfHead.szRecCount, sizeof(m_stDbfHead.szRecCount));
    int iRecPos = sizeof(stDbfHead) + m_vecFieldHead.size() * sizeof(stFieldHead) + iRecSize * iRecCount + 1;
    int iRes = fseek(m_newDbf, iRecPos, SEEK_SET);
    if (iRes != 0 )
    {
        std::cerr << __FUNCTION__ << " Error seeking to new Record position " << std::endl;
        return 1;
    }

    // 初始化指针
    m_pRecord = (char *)malloc(sizeof(char) * iRecSize);
    memset(m_pRecord, 0, sizeof(char) * iRecSize);
    m_pRecord[0] = ' '; // 首字节作为是否删除的标记

    for (int f = 0; f < m_vecFieldHead.size(); f++)
    {
        std::string sFieldValue = sValues[f];
        char cType = m_vecFieldHead[f].szType[0];
        memcpy(&iLen, m_vecFieldHead[f].szLen, 1);
        memcpy(&iOffset, m_vecFieldHead[f].szOffset, sizeof(m_vecFieldHead[f].szOffset));
        if( cType == 'I' )
        {
            auto iTmp = atoi(sFieldValue.c_str());
            memcpy(&m_pRecord[iOffset], &iTmp, sizeof(iTmp));
        }
        else if( cType== 'B' )
        {
            auto iTmp = atof(sFieldValue.c_str());
            memcpy(&m_pRecord[iOffset], &iTmp, sizeof(iTmp));
        }
        else if( cType== 'L' )
        {
            // logical
            if (sFieldValue == "T" || sFieldValue == "TRUE")
                m_pRecord[iOffset] = 'T';
            else if( sFieldValue=="?")
                m_pRecord[iOffset] = '?';
            else
                m_pRecord[iOffset] = 'F';
        }
        else
        {
            // default for character type fields (and all unhandled field types)
            for (unsigned int j = 0; j < iLen; j++)
            {
                int n = iOffset + j;
                if( j < sFieldValue.length() )
                    m_pRecord[n] = sFieldValue[j];
                else
                    m_pRecord[n] = 0;
            }
        }
    }

    // write the record at the end of the file
    // FIXME
    int nBytesWritten = fwrite(m_pRecord, 1, iRecSize, m_newDbf);
    if( nBytesWritten != iRecSize)
    {
        std::cerr << __FUNCTION__ << " Failed to write new record ! wrote " << nBytesWritten << " bytes but wanted to write " << iRecSize << " bytes" << std::endl;
        return 1;
    }

    int iNewRecCount = iRecCount + 1;
    time_t now;
    time(&now);
    tm *tp = localtime(&now);
    memcpy(m_stDbfHead.szRecCount, &iNewRecCount, sizeof(m_stDbfHead.szRecCount));
    memmove(m_stDbfHead.szYear, &tp->tm_year, sizeof(m_stDbfHead.szYear));
    memmove(m_stDbfHead.szMonth, &tp->tm_mon, sizeof(m_stDbfHead.szMonth));
    memmove(m_stDbfHead.szDay, &tp->tm_mday, sizeof(m_stDbfHead.szDay));

    // 加上文件结束标记
    if (fwrite(&chFileEndFlag, 1, 1, m_newDbf) < 1)
    {
        fclose(m_newDbf);
        m_newDbf = NULL;
        return -2;
    }

    if (m_pRecord != NULL)
    {
        delete m_pRecord;
        m_pRecord = NULL;
    }

    fflush(m_newDbf);
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

    mmapSize = nPageNum * getpagesize(); // getpagesize()获取内存页大小, 这里一次映射500M
    mmapCount = stat.st_size / mmapSize; // 已经知道文件大小了, 为什么不直接根据大小进行合理映射呢

    // 映射到内存的大小和文件的大小
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

        if (mmapSize > remainFileSize)
        {
            totalSize = remainFileSize;
        }

        if (0 == i)
        {
            /*
             * 第一次读的时候要跳过文件头和末尾的标记字节, 指针指向数据记录真正开始的地方
             * +1 是因为每条记录的第一个字节属于特殊标记字节
             */
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

    close(fd);

    return 0;
}

/*
 * 1.7G 2700w条记录的dbf文件测试耗时
 * $ date && ./mainmmapmulti.out gene ../../dbf/qtzhzl100077.b09 && date
 * Thu Jan 10 11:11:09 DST 2019
 * Consume: 184s
 * Thu Jan 10 11:14:13 DST 2019
*/
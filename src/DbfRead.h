/*
 ****************************************************
 * Author       : M_Kepler
 * EMail        : m_kepler@foxmail.com
 * Last modified: 2018-11-18 22:41:27
 * Filename     : dbf_read.h
 * Description  : �Զ�������ʽ����dbf�ļ�
 * dbf�ļ��Ķ������ļ���ʽ https://blog.csdn.net/xwebsite/article/details/6912146
 ****************************************************
*/

#ifndef _DBF_READ_H_
#define _DBF_READ_H_

#include <stdio.h>
#include <string>
#include <vector>

// dbf�ļ��д洢���ֶ���Ϣ
typedef struct stFieldHead
{
    char szName[11];  // 0 - 10  �ֶ�����
    char szType[1];   // 11      �ֶε��������ͣ�C�ַ���N���֡�D���ڡ�B�����ơ���
    char szResv[4];   // 12 - 15 �����ֽ�, Ĭ��Ϊ0
    char szLen[1];    // 16      �ֶγ���
    char szJingDu[1]; // 17      �ֶξ���
    char szResv2[2];  // 18 - 19 �����ֽ�, Ĭ��Ϊ0
    char szId[1];     // 20      ������ID
    char szResv3[11]; // 21 - 31 �����ֽ�, Ĭ��Ϊ0
} stFieldHead;

typedef void (*pCallback)(std::vector<stFieldHead> vecFieldHead, char *pcszContent);

class CDbfRead
{
  public:
    CDbfRead(const std::string &strFile);
    ~CDbfRead(void);

  public:
    /*
     * @brief:  ����dbf�ļ�, ������dbf�ļ�ͷ��Ϣ
     *          �����ֶ���Ϣ���浽 m_vecFieldHead
     */
    int ReadHead();

    /*
     * @brief:  ����dbf�ļ�, �������ļ��ڴ�ӳ��, ��ȡÿ�����ݼ�¼��pfn����
     * @param:  pfn     ����ָ�� 
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

    /* dbf�������ļ��еļ�¼���� */
    int m_nRecordNum;
    /* dbf�������ļ����ļ�ͷ���ֽ��� */
    short m_sFileHeadBytesNum;
    /* һ����¼�е��ֽڳ���,ÿ��������ռ���� */
    short m_sRecordSize;
    /* ��dbf�ļ�ͷ�н�������ÿ���ֶ���Ϣ */
    std::vector<stFieldHead> m_vecFieldHead;
};

#endif

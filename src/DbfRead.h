/*
 ****************************************************
 * Author       : M_Kepler
 * EMail        : m_kepler@foxmail.com
 * Last modified: 2018-11-18 22:41:27
 * Filename     : dbf_read.h
 * Description  : �Զ�������ʽ����dbf�ļ�
    * dbf�ļ��Ķ������ļ���ʽ:  http://www.xumenger.com/dbf-20160703/
    * https://github.com/bramhe/DBFEngine
    * �༭���п����ĵ�ʮ��������: 9f17 ��Ӧ��ʮ����Ӧ����6047(��ʮ������179f��Ӧ��ʮ����)
    * ֮����Ҫ�������������Ϊ�ڴ��д洢���ݲ��õ���"С��ģʽ",���ݵĵ�λ����ǰ�棬��λ���ں���
 ****************************************************
*/

#ifndef _DBF_READ_H_
#define _DBF_READ_H_

#include <stdio.h>
#include <string>
#include <vector>
#include <iostream>
using namespace std;

// dbf�ļ��ļ�ͷ��Ϣ
typedef struct stDBFHead
{
    char szMark[1];          // �汾��Ϣ
    char szYear[1];
    char szMonth[1];
    char szDay[1];
    char szRecCount[4];    // 4�ֽڱ����¼��
    char szDataOffset[2];  // 2�ֽڱ����ļ�ͷ�ֽ���
    char szRecSize[2];     // 2�ֽڱ���ÿ�����ݵĳ���
    char Reserved[20];
} stDbfHead, *LPDBFHEAD;

// dbf�ļ��д洢���ֶ���Ϣ
typedef struct stFieldHead
{
    char szName[11];        // 0  - 10  �ֶ�����
    char szType[1];         // 11       �ֶε��������ͣ�C�ַ���N���֡�D���ڡ�B�����ơ���
    char szOffset[4];         // 12 - 15  �ֶ�ƫ����, Ĭ��Ϊ0
    char szLen[1];          // 16       �ֶγ���
    char szPrecision[1];    // 17       �ֶξ���
    char szResv2[2];        // 18 - 19  �����ֽ�, Ĭ��Ϊ0
    char szId[1];           // 20       ������ID
    char szResv3[11];       // 21 - 31  �����ֽ�, Ĭ��Ϊ0
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
     * @brief:  ������dbf�ļ�ͷ��Ϣ,
     *          �� dbf �ļ�����  ���¼�����ļ�ͷ�ֽڵȱ��浽˽�б���
     *          �� dbf �ֶ����� ���浽 m_vecFieldHead
     */
    int ReadHead();

    /*
     * @brief:  ����dbf�ļ�, �������ļ��ڴ�ӳ��, ��ȡÿ�����ݼ�¼��pfn����
     *          ��ʱ: 184s
     * @param:  pfn         ����ָ��
     * @param:  nPageNum    ����12800��ҳ���ڴ��С; 12800 * 4k = 500M
     * @return:
     */
    int Read(pCallback pfn, int nPageNum = 128000);

    /*
     * @brief	: ����Ľӿ���ÿ��500m��ӳ��, �ýӿ�һ��ȫ��ӳ����
     *            ��ʱ: 217s
     * @param	:
     * @return	:
     */
    int ReadMmapOnce(pCallback pfn, int nPageNum = 128000);

    /*
     * @brief	: ��ʹ��ӳ��, ��������fstreamȥ���ļ�
     *            ��ʱ: 554s
     * @param	:
     * @return	:
     */
    int ReadNoMmap(pCallback pfn);


    /*
     * @brief	: ����dbf�ļ�
     * @param	: vecFieldHead �ֶζ���vec
     * @return	: errcode
     */
    int AddHead(std::vector<stFieldHead> vecFieldHead);

    /*
     * @brief	: �����¼
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

    int m_nRecordNum;                           // dbf�������ļ��еļ�¼����
    short m_sFileHeadBytesNum;                  // dbf�������ļ����ļ�ͷ���ֽ���
    short m_sRecordSize;                        // һ����¼�е��ֽڳ���, ÿ��������ռ����
    stDbfHead m_stDbfHead;
    std::vector<stFieldHead> m_vecFieldHead;    // ��dbf�ļ�ͷ�н�������ÿ���ֶ���Ϣ
};

#endif

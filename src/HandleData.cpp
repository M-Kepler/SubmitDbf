#include <string.h>
#include <string>
#include <stdlib.h>
#include <assert.h>
#include <dirent.h>
#include <time.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/time.h>
#include <sys/types.h>
#include <stdio.h>
#include "DbfRead.h"
#include "inifile.h"
#include <fstream>

/* {{{{{{{{不用libcommondao.so}}}}}}}
// 源文件备份在doc/目录下
#include "dao_base.h"
#include "dao_dynamicsql.h"
CDAOBase g_oDaoBaseHandler;
CDAODynamicSql g_oDaoDynamicSqlHandler;
*/

using namespace inifile;

int g_count = 0;
int g_nCommitCount =0;
int fileNo = 0;
FILE *pf;
FILE *fpInsert;
IniFile ini;
string g_strFile;
vector<stFieldHead> vecColumns; // 保存dbf的字段属性
string strSqlFileCnt;
string strTableName;
string strColumns;
string strIncAndSplit;
string strMaxCommitCnt;
string strSqlFileFolder;
char *g_pIncAndSplit = (char*) strIncAndSplit.c_str();

typedef void (*pLineCallback)(int iCnt, const char *pcszContent);
typedef void (*pCallback)(std::vector<stFieldHead> vecFieldHead, char *pcszContent);

void trim(std::string &s, std::string surround = " ")
{
    if (!s.empty())
    {
        s.erase(0, s.find_first_not_of(surround));
        s.erase(s.find_last_not_of(surround) + 1);
    }
}

int GetConfigValue(string &strValue, string strKey, string strSection = "CONFIG")
{
    int iRetCode;
    iRetCode = ini.getValue(strSection, strKey, strValue);
    if (iRetCode != RET_OK)
    {
        return RET_ERR;
    }
    return RET_OK;
}


// 变长结构体
typedef struct stContent
{
    int nSize;
    char buffer[0];
}stContent, *p_stContent;


// 获取要生成的文件的文件名
const char *GetFileName( )
{
    string strValue;
    char szFileName[10] = {0};
    if (GetConfigValue(strValue, "SqlFileFolder") != RET_OK)
    {
        printf("Get Config \"SqlFilePath\" Failed!\n");
        abort();
    }

    memset(szFileName, 0, sizeof(szFileName));
    snprintf(szFileName, sizeof(szFileName), "_%4.4d", fileNo);

    g_strFile = strValue + "/" + strTableName + szFileName;
    return g_strFile.c_str();
}

/*
 * @brief	: 生成sql文件
 * @param	:
 * @return	:
 */
void GenerateSql(std::vector<stFieldHead> vecFieldHead, char *pcszContent)
{
    int iColumnLen = 0;
    char szSql[2048] = {0};
    char chBuff[2] = {0};
    char buffer[1024] = {0};
    char szTempBuff[128] = {0};
    string strTmp;

    /*
    // XXX 严重拖慢性能
    if (GetConfigValue(strSqlFileCnt, "SqlFileCnt") != RET_OK)
    {
        printf("Get Config \"SqlFileCnt\" Failed!");
        abort();
    }
    if (GetConfigValue(strTableName, "TableName") != RET_OK)
    {
        printf("Get Config \"TableName\" Failed!\n");
        abort();
    }

    if (GetConfigValue(strColumns, "Columns") != RET_OK)
    {
        printf("Get Config \"Columns\" Failed!\n");
        abort();
    }
    */

    // 每n条数据输出到一个文件
    if (g_count >= atoi(strSqlFileCnt.c_str()) )
    {
        g_count = 0;
        fclose(pf);
        fileNo++;
        pf = fopen(GetFileName(), "w+");
        assert(pf);
    }
    g_count++;

    // 组装sql语句  --  根据m_vecFieldHead从pcszContent提取记录数据

    snprintf(szSql, sizeof(szSql), "insert into %s values(", strTableName.c_str());

    for (int i = 0; i < vecFieldHead.size(); i++)
    {
        if (vecFieldHead[i].szName[0] != 0x00 && vecFieldHead[i].szName[0] != '\r')
        {
            // sprintf(chBuff, "%d", i + 1); // 溢出
            snprintf(chBuff, sizeof(chBuff), "%d", i + 1);
            memcpy(&iColumnLen, vecFieldHead[i].szLen, 1);
            memset(buffer, 0x00, sizeof(buffer));

            // 按照需要指定需要插入的列
            if ((strstr(strColumns.c_str(), chBuff) != NULL) || strcmp(strColumns.c_str(), "@") == 0)
            {
                memcpy(buffer, pcszContent, iColumnLen); // 按照字段长度拷贝内存
                strTmp = string(buffer);
                trim(strTmp);
            }
            else
            {
                strTmp  = "";
            }
            strncat(szSql, "\'", 1);
            snprintf(szTempBuff, sizeof(szTempBuff), "%s", strTmp.c_str());
            strncat(szSql, szTempBuff, sizeof(szSql) - strlen(szSql) - 1);

            if(i< vecFieldHead.size() - 1)
            {
                strncat(szSql, "\', ", 3);
            }
            else
            {
                strncat(szSql, "\'", 1);
            }
        }
        pcszContent += iColumnLen; // 跳到下一个字段
    }
    snprintf(szTempBuff, sizeof(szTempBuff), "%s", ")");
    strncat(szSql, szTempBuff, sizeof(szSql) - strlen(szSql) - 1);

    fprintf(pf, "%s\n", szSql);
}

/*
 * @brief	: 生成data文件, 以用sqlldr载入数据库
 *            可以自行更改字段用什么包括，字段之间用什么间隔
 * @param	: char* pIncCPlit 表示包裹字符和分割字符
 *            如果是 ", 则表示字段由""包裹, 字段间由,分隔
 *            如果没有包裹，则写0：如：0,
 * @return	:
 */
void GenerateCsv(std::vector<stFieldHead> vecFieldHead, char *pcszContent)
{
    int iColumnLen = 0;
    char szSql[2048] = {0};
    char chBuff[2] = {0};
    char buffer[1024] = {0};
    char szTempBuff[128] = {0};

    char chInclude = g_pIncAndSplit[0];
    char chSplit = g_pIncAndSplit[1];

    string strTmp;

    // 每n条数据输出到一个文件
    if (g_count >= atoi(strSqlFileCnt.c_str()) )
    {
        g_count = 0;
        fclose(pf);
        fileNo++;
        pf = fopen(GetFileName(), "w+");
        assert(pf);
    }
    g_count++;

    for (int i = 0; i < vecFieldHead.size(); i++)
    {
        if (vecFieldHead[i].szName[0] != 0x00 && vecFieldHead[i].szName[0] != '\r')
        {
            // sprintf(chBuff, "%d", i + 1);
            snprintf(chBuff, sizeof(chBuff), "%d", i + 1);
            memcpy(&iColumnLen, vecFieldHead[i].szLen, 1);
            memset(buffer, 0x00, sizeof(buffer));

            // 按照需要指定需要插入的列
            if ((strstr(strColumns.c_str(), chBuff) != NULL) || strcmp(strColumns.c_str(), "@") == 0)
            {
                memcpy(buffer, pcszContent, iColumnLen); // 按照字段长度拷贝内存
                strTmp = string(buffer);
                trim(strTmp);
            }
            else
            {
                strTmp  = "";
            }

            // 包括起字段的左边字符 "a
            if(chInclude != '0')
            {
                // strncat(szSql, "\"", 1);
                strncat(szSql, &chInclude, 1);
            }

            snprintf(szTempBuff, sizeof(szTempBuff), "%s", strTmp.c_str());
            strncat(szSql, szTempBuff, sizeof(szSql) - strlen(szSql) - 1);

            // 处理中间字段和最后一个字段的字段间分隔符
            if (i < vecFieldHead.size() - 1)
            {
                // strncat(szSql, "\",", 2);
                if (chInclude != '0')
                {
                    strncat(szSql, g_pIncAndSplit, 2);
                }
                else
                {
                    strncat(szSql, &chSplit, 1);
                }
            }
            else
            {
                // strncat(szSql, "\"", 1);
                strncat(szSql, &chInclude, 1);
            }
        }
        pcszContent += iColumnLen; // 跳到下一个字段
    }

    fwrite(szSql, strlen(szSql), 1, pf);
    fwrite("\n", strlen("\n"), 1, pf);
    vecColumns = vecFieldHead;
}



// 连接数据库
// 加密 肯定不能明文放在配置文件啊
/* {{{{{{{{不用libcommondao.so}}}}}}}
int ConnectOracle(void)
{
    string strDbName;
    string strDbUserName;
    string strDbPwd;

    if (GetConfigValue(strDbName, "Db_Name") != RET_OK || GetConfigValue(strDbUserName, "Db_UserName") != RET_OK || GetConfigValue(strDbPwd, "Db_Pwd") != RET_OK)
    {
        printf("Get Oracle Connect Config Failed!");
        abort();
    }

    int ret = g_oDaoBaseHandler.Connect(strDbUserName.c_str(), strDbPwd.c_str(), strDbName.c_str());
    return ret;
}
*/


// 执行sql语句
/* {{{{{{{{不用libcommondao.so}}}}}}}
void SqlCommit(int iCommitCnt, const char* pcszSql)
{
    int iRetCode;
    iRetCode = g_oDaoDynamicSqlHandler.StmtExecute(pcszSql);
    if (0 != iRetCode)
    {
        printf("insert failed! iRetCode:%d\n", iRetCode);
        fprintf(fpInsert, "%s\n", pcszSql);
        fflush(fpInsert);
    }

    // 这个全局变量没啥用, 多进程下各自有各自的 g_nCommitCcount
    // TODO 要想缩短时间的话, 不能一条条commit, 现在要打印出来看看commit了多少次
    g_nCommitCount++;
    if (g_nCommitCount % iCommitCnt == 0)
    {
        if ((iRetCode = g_oDaoDynamicSqlHandler.StmtExecute("commit")) != 0)
        {
            printf("Commit to Db Failed!");
            abort();
        }
    }

}
*/

// 读入strFile文件行交给pf函数处理
void ReadFile(const std::string &strFile, int iCommitCnt, pLineCallback pf)
{
    FILE *fp;
    char *line = NULL;
    size_t len = 0;
    size_t read;

    fp = fopen(strFile.c_str(), "r");
    assert(fp);

    while ((read = getline(&line, &len, fp)) != -1)
    {
        pf(iCommitCnt, line);
    }

    if (line)
    {
        free(line);
    }
}

/*
 * @brief	: 删除文件夹下所有后缀为sql的文件
 * @param	:
 * @return	:
 */
void DeleteAllFile(const char* dirPath, const char *extenStr="sql")
{
    DIR *dir  =  opendir(dirPath);
    string strSplit  =  "/";
    dirent *pDirent = NULL;
    while((pDirent = readdir(dir)) != NULL)
    {
        if(strstr(pDirent->d_name, extenStr))
        {
            string FilePath = dirPath + strSplit + string(pDirent->d_name);
            remove(FilePath.c_str());
        }
    }
    closedir(dir);
}


/*
 * @function: 批量给文件加后缀
 * @brief	: 
 * @param	: 
 * @return	: 
 */
int RenameAllFile(const char *dirPath, const char *postfix)
{
    DIR *dir  =  opendir(dirPath);
    dirent *pDirent = NULL;
    string strSplit  =  "/";
    char filenewname[260 + 1] = {0};
    while((pDirent = readdir(dir)) != NULL)
    {
        string fileoldname = dirPath + strSplit + pDirent->d_name;
        snprintf(filenewname, sizeof(filenewname), "%s.%s", fileoldname.c_str(), postfix);
        if (strchr(strrchr(fileoldname.c_str(), '/') + 1, '.') == NULL)
        {
            rename(fileoldname.c_str(), filenewname);
        }
    }
    closedir(dir);
    return EXIT_SUCCESS;
}


/*
 * @brief	: 获取文件夹下所有extenStr后缀的文件名列表
 * @param	:
 * @return	:
 */
void GetAllFile(const char *dirPath, vector<string> &vecFileList, const char *extenStr = "sql")
{
    DIR *dir = opendir(dirPath);
    string strSplit  =  "/";
    dirent *pDirent = NULL;

    while ((pDirent = readdir(dir)) != NULL)
    {
        if (strstr(pDirent->d_name, extenStr))
        {
            string pathFile = dirPath + strSplit  +  string(pDirent->d_name);
            vecFileList.push_back(pathFile);
        }
    }
    closedir(dir);
}

/*
 * @brief	: 从string中根据特定分隔符获取值
 * @param	:
 * @return	:
 */
string GetMsgValue(string strOrig, string strKey, string strSplit = ",")
{
	string strRetValue = "";
	int iStrOrigLen;
	int iStrKeyLen;
	size_t uiPosKeyBegin;
	size_t uiPosKeyEnd;
	size_t uiPosStrSplit;

	iStrOrigLen = strOrig.length();
	iStrKeyLen = strKey.length();
	uiPosKeyBegin = strOrig.find(strKey);

	if (uiPosKeyBegin != string::npos)
	{
		// 从key的位置开始,第一次出现 str_split 的位置
		uiPosStrSplit =  strOrig.substr(uiPosKeyBegin).find(strSplit);
		if (uiPosStrSplit != string::npos)
		{
			uiPosKeyEnd = uiPosKeyBegin + uiPosStrSplit;
		}
		else
		{
			uiPosKeyEnd = iStrOrigLen;
		}
		int pos_begin = uiPosKeyBegin + iStrKeyLen + 1; // +1 跳过'='字符
		int value_len = uiPosKeyEnd - pos_begin;
		strRetValue = strOrig.substr(pos_begin, value_len);
		return strRetValue;
	}
	return strRetValue;
}


/*
 * @function: 获取配置中的dbf文件头配置
 * @brief	: 
 * @param	: 
 * @return	: 
 */
int GetDbfHeadCnf(std::vector<stFieldHead>&  vecFieldHead, vector<string>& vecDbfColumns)
{

    int iRetCode;
    int iTmp;
    int iOffset = 1;
    string strTmp;
    stFieldHead stFieldTmp;
    iRetCode = ini.load("./config.ini");

    iRetCode = ini.getValues("DBF", "FIELD", vecDbfColumns);
    if(iRetCode != 0)
    {
        printf("Get Config \"FIELD\" Failed!\n");
        return iRetCode;
    }

    // 解析配置中的字段属性组装成 stFieldHead 结构体
    for (auto x : vecDbfColumns)
    {
        memset(&stFieldTmp, 0, sizeof(struct stFieldHead));
        if (x.find("NAME:") != string::npos)
        {
            strTmp = GetMsgValue(x, "NAME", ",");
            memcpy(stFieldTmp.szName, strTmp.c_str(), strTmp.length());
        }
        if (x.find("TYPE:") != string::npos)
        {
            strTmp = GetMsgValue(x, "TYPE", ",");
            memcpy(stFieldTmp.szType, strTmp.c_str(), strTmp.length());
        }
        if (x.find("OFFSET:") != string::npos)
        {
            strTmp = GetMsgValue(x, "OFFSET", ",");
            memcpy(stFieldTmp.szOffset, &strTmp, sizeof(stFieldTmp.szOffset));
        }
        if (x.find("LEN:") != string::npos)
        {
            iTmp = atoi(GetMsgValue(x, "LEN", ",").c_str());
            memcpy(stFieldTmp.szLen, &iTmp, sizeof(stFieldTmp.szLen));
            memcpy(stFieldTmp.szOffset, &iOffset, sizeof(stFieldTmp.szOffset));
            iOffset += iTmp;
        }
        if (x.find("PRECISION:") != string::npos)
        {
            iTmp = atoi(GetMsgValue(x, "PRECISION", ",").c_str());
            memcpy(stFieldTmp.szPrecision, &iTmp, strTmp.length());
        }
        if (x.find("RESV2:") != string::npos)
        {
            strTmp =  GetMsgValue(x, "RESV2",",");
            memcpy(stFieldTmp.szResv2, strTmp.c_str(), strTmp.length());
        }
        if (x.find("ID:") != string::npos)
        {
            strTmp =  GetMsgValue(x, "ID",",");
            memcpy(stFieldTmp.szId, strTmp.c_str(), strTmp.length());
        }
        if (x.find("RESV3:") != string::npos)
        {
            strTmp =  GetMsgValue(x, "RESV3",",");
            memcpy(stFieldTmp.szResv3, strTmp.c_str(), strTmp.length());
        }
        vecFieldHead.push_back(stFieldTmp);
    }
    return 0;
}


/*
 * @brief	: 利用 sqlldr 导入文本到 oracle
 * @param   : vecFieldHead  字段vector
 * @param	: strTableName     导入目标表
 * @param	: strFilePath      需要导入的文件
 * @param	: strTok        文件字段间分隔符
 * @param	: strScope      字段包围符, 比如用双引号括住
 * @return	:
 */
int ImportDB(vector<stFieldHead> vecFieldHead, string strTableName, string strFilePath, string strTok=",", string strSplit="\"")
{
    // 产生SQL*Loader控制文件
    FILE *fctl;
    FILE *fp;
    int iStart = 1;
    char execommand[256];
    vector<string> vecFileList;
    string sqlload = "./sqlload.ctl";

    if ((fctl = fopen(sqlload.c_str(), "w")) == NULL)
    {
        return EXIT_FAILURE;
    }
    fprintf(fctl, "LOAD DATA\n");

    GetAllFile(strFilePath.c_str(), vecFileList, "csv");
    for (size_t i = 0; i < vecFileList.size(); i++)
    {
        fprintf(fctl, "INFILE '%s'\n", vecFileList[i].c_str()); // 文件名
    }

    fprintf(fctl, "APPEND INTO TABLE %s\n", strTableName.c_str()); // 表名
    fprintf(fctl, "FIELDS TERMINATED BY \"%s\"\n", strTok.c_str());   // 数据分隔符
    fprintf(fctl, "Optionally enclosed by '%s'\n", strSplit.c_str()); // 字段包围符
    fprintf(fctl, "TRAILING NULLCOLS\n");
    fprintf(fctl, "(\n");

    for (int i = 0; i < vecFieldHead.size(); i++)
    {
        if (vecFieldHead[i].szName[0] != 0x00 && vecFieldHead[i].szName[0] != '\r')
        {
            // fprintf(fctl, "%11s POSITION(%d:%d)", vecFieldHead[i].szName, iStart, *(int *)vecFieldHead[i] + iStart - 1);
            // iStart += *(int *)vecFieldHead[i];
            fprintf(fctl, "%11s", vecFieldHead[i].szName);
            if (i < vecFieldHead.size() - 1)
            {
                fprintf(fctl, ",\n");
            }
        }
    }
    fprintf(fctl, "\n)\n");
    fclose(fctl);

    string strUserName, strPwd, strDb;
    if ((GetConfigValue(strUserName, "Db_UserName") != RET_OK)
        || (GetConfigValue(strPwd, "Db_Pwd") != RET_OK)
        || (GetConfigValue(strDb, "Db_Name") != RET_OK)
    )
    {
        printf("Get db Config Failed!\n");
        return EXIT_FAILURE;
    }

    // 利用sqlldr提交数据
    sprintf(execommand, "sqlldr userid=%s/%s@%s control=%s", strUserName.c_str(), strPwd.c_str(), strDb.c_str(), sqlload.c_str());
    if (system(execommand) == -1)
    {
        perror("sqlldr");
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}



/*
 * @function: main函数命令 dbf2sql dbf2csv, 会根据命令把后缀为sql或csv的文件先删除
 * @brief	: 
 * @param	: 
 * @return	: 
 */
int GeneraCommand(string strFilePath, pCallback handleFunc, string extenStr)
{
    int iRetCode;

    DeleteAllFile(strSqlFileFolder.c_str(), extenStr.c_str());

    pf = fopen(GetFileName(), "w+");

    if (!pf)
    {
        return EXIT_FAILURE;
    }
    assert(pf);

    CDbfRead dbf(strFilePath);

    // 本打算根据dbf信息自动建表, 不太好, 因为多次执行的时候会冲突
    dbf.ReadHead();

    dbf.Read(handleFunc);
    fflush(pf);
    fclose(pf);
    return 0;
}



// main函数命令 run
/* {{{{{{{{不用libcommondao.so}}}}}}}
int RunSqlCommand()
{
    int iRetCode;
    int iCommitCnt;
    string strCommitValue;
    string strSqlFileFolder;
    string strLogFile;
    vector<string> vecFileList;

    if (GetConfigValue(strSqlFileFolder, "SqlFileFolder") != RET_OK)
    {
        printf("Get Config \"SqlFilePath\" Failed!\n");
        abort();
    }

    if (GetConfigValue(strCommitValue, "MaxCommitCnt") != RET_OK)
    {
        printf("Get Config \"MaxCommitCnt\" Failed!\n");
        abort();
    }
    iCommitCnt  =  atoi(strCommitValue.c_str());

    if (GetConfigValue(strLogFile, "LogFile") != RET_OK)
    {
        printf("Get Config \"LogFile\" Failed!\n");
        abort();
    }

    fpInsert = fopen(strLogFile.c_str(), "w+");

    if (!fpInsert)
    {
        return EXIT_FAILURE;
    }

    GetAllFile(strSqlFileFolder.c_str(), vecFileList);

    // 单进程:
    // begin
    /*
    if ((iRetCode = ConnectOracle()) != RET_OK)
    {
        printf("Fail to connect Oracle, ErrCode:%d", iRetCode);
        return iRetCode;
    }
    for(int i = 0; i < vecFileList.size(); i++ )
    {
        ReadFile(vecFileList[i].c_str(), iCommitCnt, SqlCommit);
    }
    // 少于iCommitCnt的和大于iCommitCnt的余数部分在这里提交
    if (g_nCommitCount <  iCommitCnt || g_nCommitCount / iCommitCnt >0)
    {
        if ((iRetCode = g_oDaoDynamicSqlHandler.StmtExecute("commit")) != 0)
        {
            printf("Commit to Db Failed!");
            return iRetCode;
        }
        g_nCommitCount = 0;
    }
    *
    // end

    // TODO 多进程, 每个进程分别调用ReadFile处理一个文件
    // begin
    int status, i;
    for(i = 0; i < vecFileList.size(); i++ )
    {
        status = fork();
        // 确保所有子进程都是由父进程fork出来的
        if(status == 0 || status == -1)
        {
            break;
        }
    }
    if(status == -1)
    {
        printf("error on fork");
    }
    else if(status == 0) // 每个子进程都会运行的代码
    {
        // sub process
        // 多进程如果共用一个连接会导致锁表
        if ((iRetCode = ConnectOracle()) != RET_OK)
        {
            printf("Fail to connect Oracle, ErrCode:%d", iRetCode);
            return iRetCode;
        }

        ReadFile(vecFileList[i].c_str(), iCommitCnt, SqlCommit);

        // 少于iCommitCnt的和大于iCommitCnt的余数部分在这里提交
        if (g_nCommitCount <  iCommitCnt || g_nCommitCount / iCommitCnt >0)
        {
            if ((iRetCode = g_oDaoDynamicSqlHandler.StmtExecute("commit")) != 0)
            {
                printf("Commit to Db Failed!");
                return iRetCode;
            }
            g_nCommitCount = 0;
        }
        exit(0);
    }
    else
    {
        // parent process
        printf("par process:%d\t%d\t\n", getpid(), i);
    }
    // end

    fclose(fpInsert);
    // fflush(fpInsert);
    return 0;
}
*/


// main函数命令 sqlldr
/*
 * @function: 把文件 strFileName 作为sqlldr控制文件中的DATA，导入数据库
 * @brief	: 
 * @param	: 
 * @return	: 
 */
int SqlLoadCommand(string strFileName)
{
    int iRetCode;
    vector<string> vecDbfColumns;
    std::vector<stFieldHead> vecFieldHead;
    if (GetDbfHeadCnf(vecFieldHead, vecDbfColumns)!= 0)
    {
        perror("get dbf config error!\n");
        return EXIT_FAILURE;
    }

    iRetCode = ImportDB(vecFieldHead, strTableName, strFileName, strIncAndSplit.substr(1, 1), strIncAndSplit.substr(0, 1));
    if (iRetCode != 0)
    {
        printf("Load File Faile");
        abort();
    }
    return EXIT_SUCCESS;
}

// main函数命令 2dbf
int Csv2DbfCommand(string strFilePath)
{
    int iRetCode;
    vector<string> vecDbfColumns;
    std::vector<stFieldHead> vecFieldHead;
    if (GetDbfHeadCnf(vecFieldHead, vecDbfColumns)!= 0)
    {
        perror("get dbf config error!\n");
        return EXIT_FAILURE;
    }

    if (GetConfigValue(strIncAndSplit, "IncAndSplit") != RET_OK)
    {
        perror("Get Config Failed!\n");
        abort();
    }

    CDbfRead dbf;
    dbf.AddHead(vecFieldHead);

    // 样例
    /*
    string s1[5] = {"1", "Ric G", "210.123456789123456", "43", "T"};
    string s2[5] = {"1000", "Paul F", "196.2", "33", "T"};
    dbf.AppendRec(s1);
    dbf.AppendRec(s2);
    // 测试案例的配置（放到config.ini 的DBF下）
    FIELD=NAME:ID,TYPE:I,LEN:4
    FIELD=NAME:FirstName,TYPE:C,LEN:20
    FIELD=NAME:Weight,TYPE:F,LEN:12,PRECISION:2
    FIELD=NAME:Age,TYPE:N,LEN:3
    FIELD=NAME:Married,TYPE:L,LEN:1
    */

    fstream csv;
    string buff;
    int iColumns = vecDbfColumns.size();
    int i = 0;
    char* token;
    std::string surr = strIncAndSplit.substr(0, 1);
    const char *delim = strIncAndSplit.substr(1, 1).c_str();

    string szstrTmp [iColumns];

    csv.open(strFilePath.c_str(), ios::binary | ios::in);
    if (!csv)
        perror("open failed\n");

    while(getline(csv, buff))
    {
        char *oristr = (char*)buff.c_str();
        // char *oristr = strdup(buff.c_str()); // XXX coredump 两次free

        // 不能用strtok，不适合字段为空的情况: aaaaa,,bbbb
        // for (token = strtok(const_cast<char*>(buff.c_str()), delim); token != NULL; token = strtok(NULL, delim))
        for (token = strsep(&oristr, delim), i = 0; token != NULL, i < iColumns; token = strsep(&oristr, delim), i++)
        {
            szstrTmp[i] = token;
            if (surr != "0")
            {
                trim(szstrTmp[i], surr); // XXX  调用这个函数后delim值改变了
                delim = strIncAndSplit.substr(1, 1).c_str(); // 暂时先这样
            }
        }
        // free(oristr);
        dbf.AppendRec(szstrTmp);
    }
    return 0;
}

int main(int argc, char *argv[])
{
    int iRetCode;
    long lBeginStampTimes;
    long lEndStampTimes;
    iRetCode = ini.load("./config.ini");

    if (3 != argc)
    {
        printf("param error!\n");
        printf("conver dbf to file:\t[exec] gene [path/to/dbf]\n");
        printf("commit use sqlldr:\t[exec] sqlldr [path/to/datafile(gene first)]\n");
        printf("conver csv to dbf:\t[exec] 2dbf [path/to/csv]\n");
        return EXIT_FAILURE;
    }
    if ((GetConfigValue(strSqlFileFolder, "SqlFileFolder") != RET_OK)
        || (GetConfigValue(strSqlFileCnt, "SqlFileCnt") != RET_OK)
        || (GetConfigValue(strTableName, "TableName") != RET_OK)
        || (GetConfigValue(strColumns, "Columns") != RET_OK)
        || (GetConfigValue(strIncAndSplit, "IncAndSplit") != RET_OK)
    )
    {
        printf("Get Config Failed!\n");
        return EXIT_FAILURE;
    }


    lBeginStampTimes = time(NULL);

    if (strcmp(argv[1], "dbf2sql") == 0 )
    {
        if (GeneraCommand(argv[2], GenerateSql, "sql") != 0)
        {
            printf("Error while generate sql file");
            return EXIT_FAILURE;
        }
        RenameAllFile(strSqlFileFolder.c_str(), "sql");
    }

    if (strcmp(argv[1], "dbf2csv") == 0 )
    {
        if (GeneraCommand(argv[2], GenerateCsv, "csv") != 0)
        {
            printf("Error while generate csv file");
        }
        RenameAllFile(strSqlFileFolder.c_str(), "csv");
    }

    if (strcmp(argv[1], "sqlldr") == 0)
    {
        iRetCode = SqlLoadCommand(argv[2]);
        if (iRetCode != 0)
        {
            printf("\n SqlLoader Error" );
        }
    }

    if (strcmp(argv[1], "csv2dbf") == 0)
    {
        // 兼容csv文件有的不用冒号括起来的
        iRetCode = Csv2DbfCommand(argv[2]);
        if (iRetCode != 0)
        {
            perror("\n csv2dbf Error" );
            return EXIT_FAILURE;
        }
    }

    lEndStampTimes = time(NULL);

    printf("Consume: %lds\n", (lEndStampTimes - lBeginStampTimes));

    /* {{{{{{{{不用libcommondao.so}}}}}}}
    if (strcmp(argv[1], "run") == 0 || strcmp(argv[1], "batch") == 0)
    {
        iRetCode = RunSqlCommand();
        if (iRetCode != 0)
        {
            printf("\nError while run sql, errorcode:%d", iRetCode);
        }
    }
    */
    return 0;
}


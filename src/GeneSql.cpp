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
/* xxx
#include "dao_base.h"
#include "dao_dynamicsql.h"
*/
using namespace inifile;

int g_count = 0;
int g_nCommitCount =0;
int fileNo = 0;
FILE *pf;
FILE *fpInsert;
IniFile ini;
string g_strFile;

string strSqlFileCnt;
string strTableName;
string strColumns;
string strMaxCommitCnt;

typedef void (*pLineCallback)(int iCnt, const char *pcszContent);

/* xxx
CDAOBase g_oDaoBaseHandler;
CDAODynamicSql g_oDaoDynamicSqlHandler;
*/


void trim(std::string &s)
{
    if (!s.empty())
    {
        s.erase(0, s.find_first_not_of(" "));
        s.erase(s.find_last_not_of(" ") + 1);
    }
}

int GetConfigValue(string &strValue, string strKey, string strSection="CONFIG")
{
    int iRetCode;
    iRetCode = ini.load("./config.ini");
    if (iRetCode != RET_OK)
    {
        return RET_ERR;
    }
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
const char *GetFileName(void)
{
    string strValue;
    char szFileName[10] = {0};
    if (GetConfigValue(strValue, "SqlFileFolder") != RET_OK)
    {
        printf("Get Config \"SqlFilePath\" Failed!\n");
        abort();
    }
    
    memset(szFileName, 0, sizeof(szFileName));
    snprintf(szFileName, sizeof(szFileName), "/sql_%4.4d", fileNo);

    // XXX 为什么我返回tmp.cstr() 就不行呢
    // 如果g_strFile不是全局变量, 会报错
    g_strFile = strValue + szFileName + ".sql";
    // return szFileName;
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

    fprintf(pf, "%s;\n", szSql);
}

/*
// 连接数据库
// TODO 加密 肯定不能明文放在配置文件啊
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


// 执行sql语句
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

// 删除文件夹下所有文件
void DeleteAllFile(const char* dirPath, const char *extenStr="sql")
{
    DIR *dir  =  opendir(dirPath);
    string strSplit  =  "/";
    dirent *pDirent = NULL;
    while((pDirent = readdir(dir)) != NULL)
    {
        if(strstr(pDirent->d_name, extenStr))
        {
            string FilePath= dirPath + strSplit + string(pDirent->d_name);
            remove(FilePath.c_str());
        }
    }
    closedir(dir);
}


// 获取文件夹下所有extenStr后缀的文件名列表
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

// main函数命令
int GeneraCommand(string strFilePath)
{
    int iRetCode;
    string strSqlFileFolder;
    if ((GetConfigValue(strSqlFileFolder, "SqlFileFolder") != RET_OK)
        || (GetConfigValue(strSqlFileCnt, "SqlFileCnt") != RET_OK)
        || (GetConfigValue(strTableName, "TableName") != RET_OK)
        || (GetConfigValue(strColumns, "Columns") != RET_OK)
    )
    {
        printf("Get Config  Failed!\n");
        abort();
    }
    DeleteAllFile(strSqlFileFolder.c_str());

    pf = fopen(GetFileName(), "w+");
    
    if (!pf)
    {
        return -1;
    }
    assert(pf);

    CDbfRead dbf(strFilePath);

    // 本打算根据dbf信息自动建表, 不太好, 因为多次执行的时候会冲突
    dbf.ReadHead();

    // TODO 不够快
    dbf.Read(GenerateSql);

    fflush(pf);
    return 0;
}



/*
// main函数命令
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
        return -1;
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

int main(int argc, char *argv[])
{
    int iRetCode;
    long lBeginStampTimes;
    long lEndStampTimes;

    if (2 != argc && 3 != argc)
    {
        printf("param error!\n");
        printf("Generate SqlFile:\t[exec] gene [path/to/dbf]\n");
        printf("Run SqlFile:\t[exec] run\n");
        printf("Generate & Run:\t[exec] batch [path/to/dbf]\n");
        return -1;
    }

    lBeginStampTimes = time(NULL);
    if (strcmp(argv[1], "gene") == 0 || strcmp(argv[1], "batch") == 0)
    {
        if (GeneraCommand(argv[2]) != 0)
        {
            printf("Error while generate sql file");
        }
    }
    /*
    if (strcmp(argv[1], "run") == 0 || strcmp(argv[1], "batch") == 0)
    {
        iRetCode = RunSqlCommand();
        if (iRetCode != 0)
        {
            printf("\nError while run sql, errorcode:%d", iRetCode);
        }
    }
    */
    
    lEndStampTimes = time(NULL);

    printf("Consume: %lds\n", (lEndStampTimes - lBeginStampTimes));

    return 0;
}



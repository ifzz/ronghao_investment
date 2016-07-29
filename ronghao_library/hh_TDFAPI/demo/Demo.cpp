#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <wchar.h>
#include <iconv.h>

#include "TDFAPI.h"
#include "TDFAPIStruct.h"


#define MIN(x, y) ((x)>(y)?(y):(x))

void RecvData(THANDLE hTdf, TDF_MSG* pMsgHead);

void RecvSys(THANDLE hTdf, TDF_MSG* pSysMsg);

void DumpScreenMarket(TDF_MARKET_DATA* pMarket, int nItems);
void DumpScreenFuture(TDF_FUTURE_DATA* pFuture, int nItems);
void DumpScreenIndex(TDF_INDEX_DATA* pIndex, int nItems);
void DumpScreenTransaction(TDF_TRANSACTION* pTransaction, int nItems);
void DumpScreenOrder(TDF_ORDER* pOrder, int nItems);
void DumpScreenOrderQueue(TDF_ORDER_QUEUE* pOrderQueue, int nItems);

#define ELEM_COUNT(arr) (sizeof(arr)/sizeof(arr[0]))
#define SAFE_STR(str) ((str)?(str):"")
#define SAFE_CHAR(ch) ((ch) ? (ch) : ' ')


int code_convert(char *from_charset,char *to_charset,char *inbuf,size_t* inlen,char *outbuf,size_t* outlen)
{
        iconv_t cd;
        int rc;
        char **pin = &inbuf;
        char **pout = &outbuf;

        cd = iconv_open(to_charset,from_charset);
        if (cd==0)
                return -1;
        memset(outbuf,0,*outlen);
        if (iconv(cd,pin,(size_t*)inlen,pout,(size_t*)outlen) == -1)
                return -1;
        iconv_close(cd);
        return 0;
}
char* chararr2str(char* szBuf, int nBufLen, char arr[], int n)
{
    int nOffset = 0;
    for (int i=0; i<n; i++)
    {
        nOffset += snprintf(szBuf+nOffset, nBufLen-nOffset, "%d(%c) ", arr[i], SAFE_CHAR(arr[i]));
    }
    return szBuf;
}

char* intarr2str(char* szBuf, int nBufLen, int arr[], int n)
{
    int nOffset = 0;
    for (int i=0; i<n; i++)
    {
        nOffset += snprintf(szBuf+nOffset, nBufLen-nOffset, "%d ", arr[i]);
    }
    return szBuf;
}


int main(int argc, char*argv[])
{
    //TDF_SetEnv(TDF_ENVIRON_HEART_BEAT_INTERVAL, 10);
    //TDF_SetEnv(TDF_ENVIRON_MISSED_BEART_COUNT, 2);
    TDF_SetEnv(TDF_ENVIRON_OUT_LOG, 1);
    if (argc != 5){
        printf("Usage: program ip port user password\n");
	exit(1);
    }

    TDF_OPEN_SETTING settings = {0};
    strcpy(settings.szIp,   argv[1]);
    strcpy(settings.szPort, argv[2]);
    strcpy(settings.szUser, argv[3]);
    strcpy(settings.szPwd,  argv[4]);
    settings.nReconnectCount = 99999999;
    settings.nReconnectGap = 5;
    settings.pfnMsgHandler = RecvData; //设置数据消息回调函数
    settings.pfnSysMsgNotify = RecvSys;//设置系统消息回调函数
    settings.nProtocol = 0;
    settings.szMarkets = "SZ;SH;";      //需要订阅的市场列表
    settings.szSubScriptions = "000001.sz";    //需要订阅的股票,为空则订阅全市场
    settings.nDate = 0;//请求的日期，格式YYMMDD，为0则请求今墍
    settings.nTime = 0;//请求的时间，格式HHMMSS，为0则请求实时行情，䶿xffffffff从头请求
    settings.nTypeFlags = DATA_TYPE_ALL; //请求的品种。DATA_TYPE_ALL请求所有品祍
        TDF_ERR nErr = TDF_ERR_SUCCESS;
        THANDLE hTDF = NULL;
    hTDF = TDF_Open(&settings, &nErr);
        if (hTDF==NULL)
                printf("TDF_Open return error: %d\n", nErr);


        // Press any key to exit
        getchar();

        TDF_Close(hTDF);
}


#define GETRECORD(pBase, TYPE, nIndex) ((TYPE*)((char*)(pBase) + sizeof(TYPE)*(nIndex)))

void RecvData(THANDLE hTdf, TDF_MSG* pMsgHead)
{
   if (!pMsgHead->pData)
   {
       assert(0);
       return ;
   }

   unsigned int nItemCount = pMsgHead->pAppHead->nItemCount;
   unsigned int nItemSize = pMsgHead->pAppHead->nItemSize;

   if (!nItemCount)
   {
       assert(0);
       return ;
   }

   switch(pMsgHead->nDataType)
   {
   case MSG_DATA_MARKET:
       {
           assert(nItemSize == sizeof(TDF_MARKET_DATA));
           //DumpScreenMarket((TDF_MARKET_DATA*)pMsgHead->pData, nItemCount);

           TDF_MARKET_DATA* pLastMarket = GETRECORD(pMsgHead->pData,TDF_MARKET_DATA, nItemCount-1);
           printf( "接收到行情记录:代码：%s, 业务发生日：%d, 时间:%d, 最新价:%d，成交总量:%I64d \n", pLastMarket->szWindCode, pLastMarket->nActionDay, pLastMarket->nTime, pLastMarket->nMatch, pLastMarket->iVolume);
       }
       break;
   case MSG_DATA_FUTURE:
       {
           assert(nItemSize == sizeof(TDF_FUTURE_DATA));
           DumpScreenFuture((TDF_FUTURE_DATA*)pMsgHead->pData, nItemCount);
           TDF_FUTURE_DATA* pLastFuture = GETRECORD(pMsgHead->pData,TDF_FUTURE_DATA, nItemCount-1);
           printf( "接收到期货行情记录:代码：%s, 业务发生日:%d, 时间:%d, 最新价:%d，持仓总量:%I64d \n", pLastFuture->szWindCode, pLastFuture->nActionDay, pLastFuture->nTime, pLastFuture->nMatch, pLastFuture->iOpenInterest);
       }
       break;

   case MSG_DATA_INDEX:
       {
           DumpScreenIndex((TDF_INDEX_DATA*)pMsgHead->pData, nItemCount);
           TDF_INDEX_DATA* pLastIndex = GETRECORD(pMsgHead->pData,TDF_INDEX_DATA, nItemCount-1);
           printf( "接收到指数记录:代码：%s, 业务发生日:%d, 时间:%d, 最新指数:%d，成交总量:%I64d \n", pLastIndex->szWindCode, pLastIndex->nActionDay, pLastIndex->nTime, pLastIndex->nLastIndex, pLastIndex->iTotalVolume);
       }
       break;
   case MSG_DATA_TRANSACTION:
       {
           DumpScreenTransaction((TDF_TRANSACTION*)pMsgHead->pData, nItemCount);
           TDF_TRANSACTION* pLastTransaction = GETRECORD(pMsgHead->pData,TDF_TRANSACTION, nItemCount-1);
           printf( "接收到逐笔成交记录:代码：%s, 业务发生日:%d, 时间:%d, 成交价格:%d，成交数量:%d \n", pLastTransaction->szWindCode, pLastTransaction->nActionDay, pLastTransaction->nTime, pLastTransaction->nPrice, pLastTransaction->nVolume);
       }
       break;
   case MSG_DATA_ORDERQUEUE:
       {

           DumpScreenOrderQueue((TDF_ORDER_QUEUE*)pMsgHead->pData, nItemCount);
           TDF_ORDER_QUEUE* pLastOrderQueue = GETRECORD(pMsgHead->pData,TDF_ORDER_QUEUE, nItemCount-1);
           printf( "接收到委托队列记录:代码：%s, 业务发生日:%d, 时间:%d, 委托价格:%d，订单数量:%d \n", pLastOrderQueue->szWindCode, pLastOrderQueue->nActionDay, pLastOrderQueue->nTime, pLastOrderQueue->nPrice, pLastOrderQueue->nOrders);
       }
       break;
   case MSG_DATA_ORDER:
       {
           DumpScreenOrder((TDF_ORDER*)pMsgHead->pData, nItemCount);
           TDF_ORDER* pLastOrder = GETRECORD(pMsgHead->pData,TDF_ORDER, nItemCount-1);
           printf("接收到逐笔委托记录:代码：%s, 业务发生日:%d, 时间:%d, 委托价格:%d，委托数量:%d \n", pLastOrder->szWindCode, pLastOrder->nActionDay, pLastOrder->nTime, pLastOrder->nPrice, pLastOrder->nVolume);
       }
       break;
   default:
       {
           assert(0);
       }
       break;
   }
}

void RecvSys(THANDLE hTdf, TDF_MSG* pSysMsg)
{
    if (!pSysMsg ||! hTdf)
    {
        return;
    }

    switch (pSysMsg->nDataType)
    {
    case MSG_SYS_DISCONNECT_NETWORK:
        {
            printf("网络断开\n");
        }
        break;
    case MSG_SYS_CONNECT_RESULT:
        {
            TDF_CONNECT_RESULT* pConnResult = (TDF_CONNECT_RESULT*)pSysMsg->pData;
            if (pConnResult && pConnResult->nConnResult)
            {
                printf("连接 %s:%s user:%s, password:%s 成功!\n", pConnResult->szIp, pConnResult->szPort, pConnResult->szUser, pConnResult->szPwd);
            }
            else
            {
                printf("连接 %s:%s user:%s, password:%s 失败!\n", pConnResult->szIp, pConnResult->szPort, pConnResult->szUser, pConnResult->szPwd);
            }
        }
        break;
    case MSG_SYS_LOGIN_RESULT:
        {
            TDF_LOGIN_RESULT* pLoginResult = (TDF_LOGIN_RESULT*)pSysMsg->pData;
            
                //convert gb2312 to utf-8
                char utf_info[128];
                size_t len2 =128;
                size_t len1 = strlen(pLoginResult->szInfo);
                code_convert("gb2312","utf-8",pLoginResult->szInfo,&len1,utf_info,&len2);
            
            if (pLoginResult && pLoginResult->nLoginResult)
            {
                printf("登陆成功！info:%s, nMarkets:%d\n", utf_info , pLoginResult->nMarkets);
                for (int i=0; i<pLoginResult->nMarkets; i++)
                {
                    printf("market:%s, dyn_date:%d\n", pLoginResult->szMarket[i], pLoginResult->nDynDate[i]);
                }
            }
            else
            {
                printf("登陆失败，原因：%s\n", utf_info);
            }
        }
        break;
    case MSG_SYS_CODETABLE_RESULT:
        {
            TDF_CODE_RESULT* pCodeResult = (TDF_CODE_RESULT*)pSysMsg->pData;
            if (pCodeResult )
            {
                printf("接收到代码表：info:%s, 市场个数:%d\n", pCodeResult->szInfo, pCodeResult->nMarkets);
                for (int i=0; i<pCodeResult->nMarkets; i++)
                {
                    printf("市场:%s, 代码表项数:%d, 代码表日期:%d\n", pCodeResult->szMarket[i], pCodeResult->nCodeCount[i], pCodeResult->nCodeDate[i]);
                    //获取代码表
                    TDF_CODE* pCodeTable;
                    unsigned int nItems;
                    TDF_GetCodeTable(hTdf, pCodeResult->szMarket[i], &pCodeTable, &nItems);
                    for (int i=0; i<nItems; i++)
                    {
                        TDF_CODE& code = pCodeTable[i];
                        //convert gbk to utf-8
                        char utf_name[128];
                        size_t len2 =128;
                        size_t len1 = strlen(code.szCNName);
                        code_convert("gbk","utf-8",code.szCNName,&len1,utf_name,&len2);
                        
                        printf("windcode:%s, code:%s, market:%s, name:%s, nType:0x%x\n",code.szWindCode, code.szCode, code.szMarket, utf_name, code.nType);

                    }
                    TDF_FreeArr(pCodeTable);                   
                }
            }
        }
        break;
    case MSG_SYS_QUOTATIONDATE_CHANGE:
        {
            TDF_QUOTATIONDATE_CHANGE* pChange = (TDF_QUOTATIONDATE_CHANGE*)pSysMsg->pData;
            if (pChange)
            {
                printf("收到行情日期变更通知，即将自动重连！交易所：%s, 原日期:%d, 新日期：%d\n", pChange->szMarket, pChange->nOldDate, pChange->nNewDate);
            }
        }
        break;
    case MSG_SYS_MARKET_CLOSE:
        {
            TDF_MARKET_CLOSE* pCloseInfo = (TDF_MARKET_CLOSE*)pSysMsg->pData;
            if (pCloseInfo)
            {
                printf("闭市消息:market:%s, time:%d, info:%s\n", pCloseInfo->szMarket, pCloseInfo->nTime, pCloseInfo->chInfo);
            }
        }
        break;
    case MSG_SYS_HEART_BEAT:
        {
            printf("收到心跳消息\n");
        }
        break;
    default:
        assert(0);
        break;
    }
}

void DumpScreenMarket(TDF_MARKET_DATA* pMarket, int nItems)
{
#ifdef DUMPALL
    printf("-------- Market, Count:%d --------\n", nItems);
    char szBuf1[512];
    char szBuf2[512];
    char szBuf3[512];
    char szBuf4[512];
    char szBufSmall[64];
    for (int i=0; i<nItems; i++)
    {
        const TDF_MARKET_DATA& marketData = pMarket[i];
        printf("万得代码 szWindCode: %s\n", marketData.szWindCode);
        printf("原始代码 szCode: %s\n", marketData.szCode);
        printf("业务发生日(自然日) nActionDay: %d\n", marketData.nActionDay);
        printf("交易日 nTradingDay: %d\n", marketData.nTradingDay);
        printf("时间(HHMMSSmmm) nTime: %d\n", marketData.nTime);
        printf("状态 nStatus: %d(%c)\n", marketData.nStatus, SAFE_CHAR(marketData.nStatus));
        printf("前收盘价 nPreClose: %d\n", marketData.nPreClose);
        printf("开盘价 nOpen: %d\n", marketData.nOpen);
        printf("最高价 nHigh: %d\n", marketData.nHigh);
        printf("最低价 nLow: %d\n", marketData.nLow);
        printf("最新价 nMatch: %d\n", marketData.nMatch);
        printf("申卖价 nAskPrice: %s \n", intarr2str(szBuf1, sizeof(szBuf1), (int*)marketData.nAskPrice, ELEM_COUNT(marketData.nAskPrice)));

        printf("申卖量 nAskVol: %s \n", intarr2str(szBuf2, sizeof(szBuf2), (int*)marketData.nAskVol, ELEM_COUNT(marketData.nAskVol)));

        printf("申买价 nBidPrice: %s \n", intarr2str(szBuf3, sizeof(szBuf3), (int*)marketData.nBidPrice, ELEM_COUNT(marketData.nBidPrice)));

        printf("申买量 nBidVol: %s \n", intarr2str(szBuf4, sizeof(szBuf4), (int*)marketData.nBidVol, ELEM_COUNT(marketData.nBidVol)));

        printf("成交笔数 nNumTrades: %d\n", marketData.nNumTrades);

        printf("成交总量 iVolume: %I64d\n", marketData.iVolume);
        printf("成交总金额 iTurnover: %I64d\n", marketData.iTurnover);
        printf("委托买入总量 nTotalBidVol: %I64d\n", marketData.nTotalBidVol);
        printf("委托卖出总量 nTotalAskVol: %I64d\n", marketData.nTotalAskVol);

        printf("加权平均委买价格 nWeightedAvgBidPrice: %u\n", marketData.nWeightedAvgBidPrice);
        printf("加权平均委卖价格 nWeightedAvgAskPrice: %u\n", marketData.nWeightedAvgAskPrice);

        printf("IOPV净值估便nIOPV: %d\n",  marketData.nIOPV);
        printf("到期收益率 nYieldToMaturity: %d\n", marketData.nYieldToMaturity);
        printf("涨停价 nHighLimited: %d\n", marketData.nHighLimited);
        printf("跌停价 nLowLimited: %d\n", marketData.nLowLimited);
        printf("证券信息前缀 chPrefix: %s\n", chararr2str(szBufSmall, sizeof(szBufSmall), (char*)marketData.chPrefix, ELEM_COUNT(marketData.chPrefix)));
        printf("市盈率1 nSyl1: %d\n", marketData.nSyl1);
        printf("市盈率1 nSyl2: %d\n", marketData.nSyl2);
        printf("升跌2（对比上一笔） nSD2: %d\n", marketData.nSD2);
        if (nItems>1)
        {
            printf("\n");
        }
    }

    printf("\n");
#endif
}

void DumpScreenFuture(TDF_FUTURE_DATA* pFuture, int nItems)
{
#ifdef DUMPALL
    printf("-------- Future, Count:%d --------\n", nItems);
    char szBuf1[256];
    char szBuf2[256];
    char szBuf3[256];
    char szBuf4[256];

    for (int i=0; i<nItems; i++)
    {
        const TDF_FUTURE_DATA& futureData = pFuture[i];
        printf("万得代码 szWindCode: %s\n", futureData.szWindCode);
        printf("原始代码 szCode: %s\n", futureData.szCode);
        printf("业务发生日(自然日) nActionDay: %d\n", futureData.nActionDay);
        printf("交易日 nTradingDay: %d\n", futureData.nTradingDay);
        printf("时间(HHMMSSmmm) nTime: %d\n", futureData.nTime);
        printf("状态 nStatus: %d(%c)\n", futureData.nStatus, SAFE_CHAR(futureData.nStatus));

        printf("昨持仓 iPreOpenInterest: %I64d\n", futureData.iPreOpenInterest);
        printf("昨收盘价 nPreClose: %d\n", futureData.nPreClose);
        printf("昨结算 nPreSettlePrice: %d\n", futureData.nPreSettlePrice);
        printf("开盘价 nOpen: %d\n", futureData.nOpen);
        printf("最高价 nHigh: %d\n", futureData.nHigh);
        printf("最低价 nLow: %d\n", futureData.nLow);
        printf("最新价 nMatch: %d\n", futureData.nMatch);
        printf("成交总量 iVolume: %I64d\n", futureData.iVolume);
        printf("成交总金额 iTurnover: %I64d\n", futureData.iTurnover);
        printf("持仓总量 iOpenInterest: %I64d\n", futureData.iOpenInterest);
        printf("今收盘 nClose: %u\n", futureData.nClose);
        printf("今结算 nSettlePrice: %u\n", futureData.nSettlePrice);
        printf("涨停价 nHighLimited: %u\n", futureData.nHighLimited);
        printf("跌停价 nLowLimited: %u\n", futureData.nLowLimited);
        printf("昨虚实度 nPreDelta: %d\n", futureData.nPreDelta);
        printf("今虚实度 nCurrDelta: %d\n", futureData.nCurrDelta);

        printf("申卖价 nAskPrice: %s\n", intarr2str(szBuf1, sizeof(szBuf1), (int*)futureData.nAskPrice, ELEM_COUNT(futureData.nAskPrice)));
        printf("申卖量 nAskVol: %s\n", intarr2str(szBuf2, sizeof(szBuf2),(int*)futureData.nAskVol, ELEM_COUNT(futureData.nAskVol)));
        printf("申买价 nBidPrice: %s\n", intarr2str(szBuf3, sizeof(szBuf3),(int*)futureData.nBidPrice, ELEM_COUNT(futureData.nBidPrice)));
        printf("申买量 nBidVol: %s\n", intarr2str(szBuf4, sizeof(szBuf4),(int*)futureData.nBidVol, ELEM_COUNT(futureData.nBidVol)));

        if (nItems>1)
        {
            printf("\n");
        }
    }

    printf("\n");
#endif
}

void DumpScreenIndex(TDF_INDEX_DATA* pIndex, int nItems)
{
#ifdef DUMPALL
    printf("-------- Index, Count:%d --------\n", nItems);

    for (int i=0; i<nItems; i++)
    {
        const TDF_INDEX_DATA& indexData = pIndex[i];
        printf("万得代码 szWindCode: %s\n", indexData.szWindCode);
        printf("原始代码 szCode: %s\n", indexData.szCode);
        printf("业务发生日(自然日) nActionDay: %d\n", indexData.nActionDay);
        printf("交易日 nTradingDay: %d\n", indexData.nTradingDay);
        printf("时间(HHMMSSmmm) nTime: %d\n", indexData.nTime);

        printf("今开盘指数 nOpenIndex: %d\n", indexData.nOpenIndex);
        printf("最高指数 nHighIndex: %d\n", indexData.nHighIndex);
        printf("最低指数 nLowIndex: %d\n", indexData.nLowIndex);
        printf("最新指数 nLastIndex: %d\n", indexData.nLastIndex);
        printf("成交总量 iTotalVolume: %I64d\n", indexData.iTotalVolume);
        printf("成交总金额 iTurnover: %I64d\n", indexData.iTurnover);
        printf("前盘指数 nPreCloseIndex: %d\n", indexData.nPreCloseIndex);

        if (nItems>1)
        {
            printf("\n");
        }
    }

    printf("\n");
#endif
}

void DumpScreenTransaction(TDF_TRANSACTION* pTransaction, int nItems)
{
#ifdef DUMPALL
    printf("-------- Transaction, Count:%d --------\n", nItems);

    for (int i=0; i<nItems; i++)
    {
        const TDF_TRANSACTION& transaction = pTransaction[i];
        printf("万得代码 szWindCode: %s\n", transaction.szWindCode);
        printf("原始代码 szCode: %s\n", transaction.szCode);
        printf("业务发生日(自然日) nActionDay: %d\n", transaction.nActionDay);
        printf("时间(HHMMSSmmm) nTime: %d\n", transaction.nTime);
        printf("成交编号 nIndex: %d\n", transaction.nIndex);
        printf("成交价格 nPrice: %d\n", transaction.nPrice);
        printf("成交数量 nVolume: %d\n", transaction.nVolume);
        printf("成交金额 nTurnover: %d\n", transaction.nTurnover);
        printf("买卖方向 nBSFlag: %d(%c)\n", transaction.nBSFlag, SAFE_CHAR(transaction.nBSFlag));
        printf("成交类别 chOrderKind: %d(%c)\n", transaction.chOrderKind, SAFE_CHAR(transaction.chOrderKind));
        printf("成交代码 chFunctionCode: %d(%c)\n", transaction.chFunctionCode, SAFE_CHAR(transaction.chFunctionCode));
        printf("叫卖方委托序号 nAskOrder: %d\n", transaction.nAskOrder);
        printf("叫买方委托序号 nBidOrder: %d\n", transaction.nBidOrder);

        if (nItems>1)
        {
            printf("\n");
        }
    }

    printf("\n");
#endif
}

void DumpScreenOrder(TDF_ORDER* pOrder, int nItems)
{
#ifdef DUMPALL
    printf("-------- Order, Count:%d --------\n", nItems);

    for (int i=0; i<nItems; i++)
    {
        const TDF_ORDER& order = pOrder[i];
        printf("万得代码 szWindCode: %s\n", order.szWindCode);
        printf("原始代码 szCode: %s\n", order.szCode);
        printf("业务发生日(自然日) nActionDay: %d\n", order.nActionDay);
        printf("时间(HHMMSSmmm) nTime: %d\n", order.nTime);
        printf("委托号 nOrder: %d\n", order.nOrder);
        printf("委托价格 nPrice: %d\n", order.nPrice);
        printf("委托数量 nVolume: %d\n", order.nVolume);
        printf("委托类别 chOrderKind: %d(%c)\n", order.chOrderKind, SAFE_CHAR(order.chOrderKind));
        printf("委托代码 chFunctionCode: %d(%c)\n", order.chFunctionCode, SAFE_CHAR(order.chFunctionCode));

        if (nItems>1)
        {
            printf("\n");
        }
    }

    printf("\n");
#endif
}

void DumpScreenOrderQueue(TDF_ORDER_QUEUE* pOrderQueue, int nItems)
{
#ifdef DUMPALL
    printf("-------- Order, Count:%d --------\n", nItems);

    char szBuf[3200];
    for (int i=0; i<nItems; i++)
    {
        const TDF_ORDER_QUEUE& orderQueue = pOrderQueue[i];
        printf("万得代码 szWindCode: %s\n", orderQueue.szWindCode);
        printf("原始代码 szCode: %s\n", orderQueue.szCode);
        printf("业务发生日(自然日) nActionDay: %d\n", orderQueue.nActionDay);
        printf("时间(HHMMSSmmm) nTime: %d\n", orderQueue.nTime);

        printf("买卖方向 nSide: %d(%c)\n", orderQueue.nSide, SAFE_CHAR(orderQueue.nSide));
        printf("委托价格 nPrice: %d\n", orderQueue.nPrice);
        printf("订单数量 nOrders: %d\n", orderQueue.nOrders);
        printf("明细个数 nOrder: %d\n", orderQueue.nABItems);

        printf("订单明细 nVolume: %s\n", intarr2str(szBuf,sizeof(szBuf), (int*)orderQueue.nABVolume, MIN(ELEM_COUNT(orderQueue.nABVolume),orderQueue.nABItems)));

        if (nItems>1)
        {
            printf("\n");
        }
    }

    printf("\n");
#endif
}


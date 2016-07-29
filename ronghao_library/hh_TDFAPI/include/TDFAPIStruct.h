#ifndef __TDF_API_STRUCT_H__ 
#define __TDF_API_STRUCT_H__
#pragma  pack(push)
#pragma pack(1)


#ifndef THANDLE
typedef void* THANDLE;
#endif

#ifndef __int64
#define __int64 long long
#endif

//ϵͳ��Ϣ��������Ϣ�Ķ���
enum TDF_MSG_ID
{
    MSG_INVALID = -100,

    //ϵͳ��Ϣ
    MSG_SYS_DISCONNECT_NETWORK,     //����Ͽ��¼�
    MSG_SYS_CONNECT_RESULT,         //�����������ӵĽ��
    MSG_SYS_LOGIN_RESULT,           //��½Ӧ��
    MSG_SYS_CODETABLE_RESULT,       //��ȡ�������
    MSG_SYS_QUOTATIONDATE_CHANGE,   //�������ڱ��֪ͨ
    MSG_SYS_MARKET_CLOSE,           //����
    MSG_SYS_HEART_BEAT,             //������������Ϣ

    //������Ϣ
    MSG_DATA_INDEX,                 //ָ������
    MSG_DATA_MARKET,                //��������
    MSG_DATA_FUTURE,                //�ڻ�����
    MSG_DATA_TRANSACTION,           //��ʳɽ�
    MSG_DATA_ORDERQUEUE,            //ί�ж���
    MSG_DATA_ORDER,                 //���ί��
	MSG_DATA_HK_AMOUNT,             //�۹�ͨʵʱ���
	MSG_DATA_HK_STATUS,             //�Ͻ����۹�ͨ�ɽ��ն�����ת���Ĳ�Ʒ״̬����

	MSG_SYS_PACK_OVER = -10,              //��ǰ������������
};
//MSG_SYS_PACK_OVER
struct TDF_PACK_OVER
{
	int nDataNumber;
	int nConID;
};
//�ο�����һ����¼
struct TDF_EXCHRATE_RECORD
{
	__int64    iBuyPrice;       //�ο���������ۣ�С����5λ������100000��
	__int64    iSellPrice;      //�ο����������ۣ�С����5λ������100000��
	__int64    iMedianPrice;    //�ο������м�ۣ�С����5λ������100000��
	char       chCurrencyType[8];   //��������
	//int        nDate;            //��������
};
//������Ϣ��MSG_DATA_HK_AMOUNT���۹�ͨʵʱ������ݸ�ʽ���ͣ�
struct TDF_HK_AMOUNT_DATA
{
	int         nActionDay;             //ҵ������(��Ȼ��)
	int         nTradingDay;            //������
	int			nTime;					//ʱ��(HHMMSSmmm)
	__int64     iThresholdAmount;       //ÿ�ճ�ʼ��ȣ���λ�����Ԫ
	__int64     iPosAmt;	            //����ʣ���ȣ���λ�����Ԫ
	char        chAmountStatus;         //���״̬	1��������ꣻ2����ȿ���
	//�ο�����
	int         nExchRateItems;
	TDF_EXCHRATE_RECORD exchRates[2];
	//    int	nItemCount;	       //��¼����
	//    TDF_EXCHRATE_RECORD* pExchRates;
};

//������Ϣ��MSG_DATA_HK_STATUS���Ͻ����۹�ͨ�ɽ��ն�����ת���Ĳ�Ʒ״̬���ݣ�
struct TDF_HK_STATUS_DATA
{
	char        chCode[32];					    //֤ȯ����
	char        chSecTradingStatus1[9];        //�۹����ֶ���(C8):	���ֶ�Ϊ8λ�ַ���������ÿλ��ʾ�ض��ĺ��壬�޶�������ո񡣵�1λ����0����ʾ�������룬��1����ʾ�����޴����ơ�
											   //��2λ����0����ʾ������������1����ʾ�����޴����ơ�
	char        chSecTradingStatus2[9];        //�۹���ɶ���(C8):	���ֶ�Ϊ8λ�ַ���������ÿλ��ʾ�ض��ĺ��壬�޶�������ո񡣵�1λ����0����ʾ�������룬��1����ʾ�����޴����ơ�
											   //��2λ����0����ʾ������������1����ʾ�����޴����ơ�		
};


//ϵͳ��Ϣ��MSG_SYS_CONNECT_RESULT ��Ӧ�Ľṹ
struct TDF_CONNECT_RESULT
{
    char szIp[32];
    char szPort[8];
    char szUser[32];
    char szPwd[32];

    unsigned int nConnResult; //Ϊ0���ʾ����ʧ�ܣ���0���ʾ���ӳɹ�
    int nConnectionID;        //����ID
};

//ϵͳ��Ϣ��MSG_SYS_LOGIN_RESULT ��Ӧ�Ľṹ
struct TDF_LOGIN_RESULT
{
    unsigned int nLoginResult;//Ϊ0���ʾ��½��֤ʧ�ܣ���0���ʾ��֤�ɹ�

    char szInfo[128];       //��½����ı�
    int nMarkets;           //�г�����
    char szMarket[256][8];  //�г����� SZ, SH, CF, SHF, CZC, DCE
    int nDynDate[256];      //��̬��������
};

//ϵͳ��Ϣ��MSG_SYS_CODETABLE_RESULT ��Ӧ�Ľṹ
struct TDF_CODE_RESULT
{
    char szInfo[128];       //��������ı�
    int nMarkets;           //�г�����
    char szMarket[256][8];  //�г�����
    int nCodeCount[256];    //���������
    int nCodeDate[256];     //���������

};

//ϵͳ��Ϣ��MSG_SYS_QUOTATIONDATE_CHANGE ��Ӧ�Ľṹ
struct TDF_QUOTATIONDATE_CHANGE
{
    char szMarket[8];	    //�г�����
    int nOldDate;	        //ԭ��������
    int nNewDate;	        //����������
};

//ϵͳ��Ϣ��MSG_SYS_MARKET_CLOSE ��Ӧ�Ľṹ
struct TDF_MARKET_CLOSE
{
    char    szMarket[8];        //����������
    int		nTime;				//ʱ��(HHMMSSmmm)
    char	chInfo[64];			//������Ϣ
};


struct TDF_CODE
{
    char szWindCode[32];    //Wind Code: AG1302.SHF
    char szMarket[8];       //market code: SHF
    char szCode[32];        //original code:ag1302
    char szENName[32];
    char szCNName[32];      //chinese name: ����1302
    int nType;                            
};

struct TDF_OPTION_CODE
{
    TDF_CODE basicCode;
    
    char szContractID[32];// ��Ȩ��Լ����
    char szUnderlyingSecurityID[32];//// ���֤ȯ����
    char chCallOrPut;               // �Ϲ��Ϲ�C1        �Ϲ������ֶ�Ϊ��C������Ϊ�Ϲ������ֶ�Ϊ��P��
    int  nExerciseDate;             // ��Ȩ��Ȩ�գ�YYYYMMDD
    
    //�����ֶ�
    char chUnderlyingType;			// ���֤ȯ����C3    0-A�� 1-ETF (EBS �C ETF�� ASH �C A ��)
	char chOptionType;              // ŷʽ��ʽC1        ��Ϊŷʽ��Ȩ�����ֶ�Ϊ��E������Ϊ��ʽ��Ȩ�����ֶ�Ϊ��A��
	
	char chPriceLimitType;          // �ǵ�����������C1 ��N����ʾ���ǵ�����������, ��R����ʾ���ǵ�����������
	int  nContractMultiplierUnit;	// ��Լ��λ,         ������Ȩ��Ϣ������ĺ�Լ��λ, һ��������
	int  nExercisePrice;            // ��Ȩ��Ȩ��,       ������Ȩ��Ϣ���������Ȩ��Ȩ�ۣ��Ҷ��룬��ȷ����
	int  nStartDate;                // ��Ȩ�׸�������,YYYYMMDD
	int  nEndDate;                  // ��Ȩ�������/��Ȩ�գ�YYYYMMDD
	int  nExpireDate;               // ��Ȩ�����գ�YYYYMMDD
};
union TD_EXCODE_INFO
{
	struct TD_OptionCodeInfo            //futures options ר�� (nType >= 0x90 && nType <= 0x95),����Ȩ�����ֶ���Ч
	{
		char chContractID[32];           // ��Ȩ��Լ����C19
		char chUnderlyingSecurityID[32]; // ���֤ȯ����
		char chUnderlyingType;			// ���֤ȯ����C3    0-A�� 1-ETF (EBS �C ETF�� ASH �C A ��)
		char chOptionType;              // ŷʽ��ʽC1        ��Ϊŷʽ��Ȩ�����ֶ�Ϊ��E������Ϊ��ʽ��Ȩ�����ֶ�Ϊ��A��
		char chCallOrPut;               // �Ϲ��Ϲ�C1        �Ϲ������ֶ�Ϊ��C������Ϊ�Ϲ������ֶ�Ϊ��P��
		char chPriceLimitType;          // �ǵ�����������C1 ��N����ʾ���ǵ�����������, ��R����ʾ���ǵ�����������
		int  nContractMultiplierUnit;	// ��Լ��λ,         ������Ȩ��Ϣ������ĺ�Լ��λ, һ��������
		int  nExercisePrice;            // ��Ȩ��Ȩ��,       ������Ȩ��Ϣ���������Ȩ��Ȩ�ۣ��Ҷ��룬��ȷ����
		int  nStartDate;                // ��Ȩ�׸�������,YYYYMMDD
		int  nEndDate;                  // ��Ȩ�������/��Ȩ�գ�YYYYMMDD
		int  nExerciseDate;             // ��Ȩ��Ȩ�գ�YYYYMMDD
		int  nExpireDate;               // ��Ȩ�����գ�YYYYMMDD
	}Option;
};
struct TDF_CODE_INFO
{
	//int  nIdnum;				//���ձ��(���������*100 + ���������)
	int  nType;					//֤ȯ����
	char chCode[32];            //֤ȯ����
	char chName[64];			//����֤ȯ����
	
	TD_EXCODE_INFO exCodeInfo;
};

struct TDF_APP_HEAD
{
    int	nHeadSize;         //����¼�ṹ��С
    int	nItemCount;	       //��¼����
    int	nItemSize;         //��¼��С
};

struct TDF_MSG
{
    unsigned short  	    sFlags;		        //16λ ��ʶ��,������ TDF_VERSION_NX_START_6001 .
    int  	                nDataType;	        //16λ ��������
    int			            nDataLen;	        //32λ ���ݳ��ȣ�������TDF_APP_HEAD�ĳ��ȣ�
    int			            nServerTime;		//32λ������ʱ�������ȷ������HHMMSSmmm��������ϵͳ��ϢΪ0
    int     		        nOrder;		        //32λ ��ˮ��
    int                     nConnectId;         //����ID����TDF_Open����
    TDF_APP_HEAD*           pAppHead;	        //Ӧ��ͷ
    void*                   pData;              //����ָ��
};

typedef void (*TDF_DataMsgHandler)(THANDLE hTdf, TDF_MSG* pMsgHead);   //���ݻص�������֪ͨ�û��յ������顢��ʳɽ������ί�У�ί�ж��е�,pMsgHead->pAppHead->ItemCount�ֶο��Ի�֪�õ��˶�������¼��pMsgHead->pAppHead->pDataָ���һ�����ݼ�¼


typedef void (*TDF_SystemMsgHandler)(THANDLE hTdf, TDF_MSG* pMsgHead); //ϵͳ��Ϣ�ص�������֪ͨ�û��յ�������Ͽ��¼������ӣ��������������������ȡ�����ȡϵͳ��Ϣʱ��pMsgHead->pAppHeadָ��Ϊ��, pMsgHead->pDataָ����Ӧ�Ľṹ��


//������Ϣ��MSG_DATA_MARKET ��Ӧ�Ľṹ
struct TDF_MARKET_DATA
{
    char        szWindCode[32];         //600001.SH 
    char        szCode[32];             //ԭʼCode
    int         nActionDay;             //ҵ������(��Ȼ��)
    int         nTradingDay;            //������
    int			 nTime;					//ʱ��(HHMMSSmmm)
    int			 nStatus;				//״̬
    unsigned int nPreClose;				//ǰ���̼�
    unsigned int nOpen;					//���̼�
    unsigned int nHigh;					//��߼�
    unsigned int nLow;					//��ͼ�
    unsigned int nMatch;				//���¼�
    unsigned int nAskPrice[10];			//������
    unsigned int nAskVol[10];			//������
    unsigned int nBidPrice[10];			//�����
    unsigned int nBidVol[10];			//������
    unsigned int nNumTrades;			//�ɽ�����
    __int64		 iVolume;				//�ɽ�����
    __int64		 iTurnover;				//�ɽ��ܽ��
    __int64		 nTotalBidVol;			//ί����������
    __int64		 nTotalAskVol;			//ί����������
    unsigned int nWeightedAvgBidPrice;	//��Ȩƽ��ί��۸�
    unsigned int nWeightedAvgAskPrice;  //��Ȩƽ��ί���۸�
    int			 nIOPV;					//IOPV��ֵ��ֵ
    int			 nYieldToMaturity;		//����������
    unsigned int nHighLimited;			//��ͣ��
    unsigned int nLowLimited;			//��ͣ��
    char		 chPrefix[4];			//֤ȯ��Ϣǰ׺
    int			 nSyl1;					//��ӯ��1
    int			 nSyl2;					//��ӯ��2
    int			 nSD2;					//����2���Ա���һ�ʣ�
	const TDF_CODE_INFO *  pCodeInfo;     //������Ϣ�� TDF_Close�����������󣬴�ָ����Ч
};

//������Ϣ��MSG_DATA_INDEX ��Ӧ�Ľṹ
struct TDF_INDEX_DATA
{
    char        szWindCode[32];         //600001.SH 
    char        szCode[32];             //ԭʼCode
    int         nActionDay;             //ҵ������(��Ȼ��)
    int         nTradingDay;            //������
    int         nTime;			//ʱ��(HHMMSSmmm)
    int		    nOpenIndex;		//����ָ��
    int 	    nHighIndex;		//���ָ��
    int 	    nLowIndex;		//���ָ��
    int 	    nLastIndex;		//����ָ��
    __int64	    iTotalVolume;	//���������Ӧָ���Ľ�������
    __int64	    iTurnover;		//���������Ӧָ���ĳɽ����
    int		    nPreCloseIndex;	//ǰ��ָ��
	const TDF_CODE_INFO *  pCodeInfo;     //������Ϣ�� TDF_Close�����������󣬴�ָ����Ч
};


//������Ϣ��MSG_DATA_FUTURE ��Ӧ�Ľṹ
struct TDF_FUTURE_DATA
{
    char        szWindCode[32];         //600001.SH 
    char        szCode[32];             //ԭʼCode
    int         nActionDay;             //ҵ������(��Ȼ��)
    int         nTradingDay;            //������
    int			 nTime;					//ʱ��(HHMMSSmmm)	
    int			 nStatus;				//״̬
    __int64		 iPreOpenInterest;		//��ֲ�
    unsigned int nPreClose;				//�����̼�
    unsigned int nPreSettlePrice;		//�����
    unsigned int nOpen;					//���̼�	
    unsigned int nHigh;					//��߼�
    unsigned int nLow;					//��ͼ�
    unsigned int nMatch;				//���¼�
    __int64		 iVolume;				//�ɽ�����
    __int64		 iTurnover;				//�ɽ��ܽ��
    __int64		 iOpenInterest;			//�ֲ�����
    unsigned int nClose;				//������
    unsigned int nSettlePrice;			//�����
    unsigned int nHighLimited;			//��ͣ��
    unsigned int nLowLimited;			//��ͣ��
    int			 nPreDelta;			    //����ʵ��
    int			 nCurrDelta;            //����ʵ��
    unsigned int nAskPrice[5];			//������
    unsigned int nAskVol[5];			//������
    unsigned int nBidPrice[5];			//�����
    unsigned int nBidVol[5];			//������

	//Add 20140605
	int	lAuctionPrice;		//�������жϲο���
	int	lAuctionQty;		//�������жϼ��Ͼ�������ƥ����
	//Add 20141014
	int    lAvgPrice;          //֣�����ڻ�����
	const TDF_CODE_INFO *  pCodeInfo;     //������Ϣ�� TDF_Close�����������󣬴�ָ����Ч
};

//������Ϣ��MSG_DATA_ORDERQUEUE ��Ӧ�Ľṹ
struct TDF_ORDER_QUEUE
{
    char    szWindCode[32]; //600001.SH 
    char    szCode[32];     //ԭʼCode
    int     nActionDay;     //��Ȼ��
    int 	nTime;			//ʱ��(HHMMSSmmm)
    int     nSide;			//��������('B':Bid 'A':Ask)
    int		nPrice;			//ί�м۸�
    int 	nOrders;		//��������
    int 	nABItems;		//��ϸ����
    int 	nABVolume[200];	//������ϸ
	const TDF_CODE_INFO *  pCodeInfo;     //������Ϣ�� TDF_Close�����������󣬴�ָ����Ч
};

//������Ϣ��MSG_DATA_TRANSACTION ��Ӧ�Ľṹ
struct TDF_TRANSACTION
{
    char    szWindCode[32]; //600001.SH 
    char    szCode[32];     //ԭʼCode
    int     nActionDay;     //��Ȼ��
    int 	nTime;		    //�ɽ�ʱ��(HHMMSSmmm)
    int 	nIndex;		    //�ɽ����
    int		nPrice;		    //�ɽ��۸�
    int 	nVolume;	    //�ɽ�����
    int		nTurnover;	    //�ɽ����
    int     nBSFlag;        //��������(��'B', ����'A', ������' ')
    char    chOrderKind;    //�ɽ����
    char    chFunctionCode; //�ɽ�����
    int	    nAskOrder;	    //������ί�����
    int	    nBidOrder;	    //����ί�����
	const TDF_CODE_INFO *  pCodeInfo;     //������Ϣ�� TDF_Close�����������󣬴�ָ����Ч
};

//������Ϣ��MSG_DATA_ORDER ��Ӧ�Ľṹ
struct TDF_ORDER
{
    char    szWindCode[32]; //600001.SH 
    char    szCode[32];     //ԭʼCode
    int 	nActionDay;	    //ί������(YYMMDD)
    int 	nTime;			//ί��ʱ��(HHMMSSmmm)
    int 	nOrder;	        //ί�к�
    int		nPrice;			//ί�м۸�
    int 	nVolume;		//ί������
    char    chOrderKind;	//ί�����
    char    chFunctionCode;	//ί�д���('B','S','C')
	const TDF_CODE_INFO *  pCodeInfo;     //������Ϣ�� TDF_Close�����������󣬴�ָ����Ч
};


enum DATA_TYPE_FLAG
{
    DATA_TYPE_ALL = 0,//����DATA_TYPE_FUTURE_CX �������������
    
    DATA_TYPE_INDEX = 0x1,  //ָ��
    DATA_TYPE_TRANSACTION = 0x2,//��ʳɽ�
    DATA_TYPE_ORDER = 0x4, //���ί��
    DATA_TYPE_ORDERQUEUE=0x8,//ί�ж���
    
    DATA_TYPE_FUTURE_CX = 0x10,//�ڻ�����
};

enum TDF_PROXY_TYPE
{
    TDF_PROXY_SOCK4,
    TDF_PROXY_SOCK4A,
    TDF_PROXY_SOCK5,
    TDF_PROXY_HTTP11,
};

struct TDF_PROXY_SETTING
{
    TDF_PROXY_TYPE nProxyType;
    char szProxyHostIp[32];
    char szProxyPort[8];
    char szProxyUser[32];
    char szProxyPwd[32];
};

struct TDF_OPEN_SETTING
{
    char szIp[32];
    char szPort[8];
    char szUser[32];
    char szPwd[32];

    unsigned int nReconnectCount;   //�����ӶϿ�ʱ��������
    unsigned int nReconnectGap;     //�������
    
    TDF_DataMsgHandler pfnMsgHandler;       //������Ϣ����ص�
    TDF_SystemMsgHandler pfnSysMsgNotify;   //ϵͳ��Ϣ֪ͨ�ص�

    unsigned int nProtocol; //Э��ţ�Ϊ0��ΪĬ�ϣ�����TDF_VERSION_CURRENT
    const char* szMarkets;  //����"SZ;SH;CF;SHF;DCE;SHF"����Ҫ���ĵ��г��б��ԡ�;���ָ�,Ϊ����������֧�ֵ��г�
    const char* szSubScriptions; //����"600000.sh;ag.shf;000001.sz"����Ҫ���ĵĹ�Ʊ���ԡ�;���ָΪ������ȫ�г�
    unsigned int nDate;     //��������ڣ���ʽYYMMDD��Ϊ0���������
    unsigned int nTime;     //�����ʱ�䣬��ʽHHMMSS��Ϊ0������ʵʱ���飬Ϊ0xffffffff��ͷ����
    unsigned int nTypeFlags;    //Ϊ0��������Ʒ�֣�����ȡֵΪDATA_TYPE_FLAG�ж�����𣬱���DATA_TYPE_MARKET | DATA_TYPE_TRANSACTION

    unsigned int nConnectionID; //����ID�����ӻص���Ϣ�ĸ��ӽṹ TDF_CONNECT_RESULT�� ��������ID
	int nInDex; //�ڼ����򿪵��г���
};

// Connect Server Info.
struct TDF_SERVER_INFO
{
	char szIp[32];
	char szPort[8];
	char szUser[32];
	char szPwd[32];
};

// TDF_OPEN_SETTING_EXT 
//   �������ӷ�������ȷ�������ܹ���ȷ��á�
struct TDF_OPEN_SETTING_EXT
{
	TDF_SERVER_INFO	siServer[4];

	unsigned int nReconnectCount;   //�����ӶϿ�ʱ��������
	unsigned int nReconnectGap;     //�������

	TDF_DataMsgHandler pfnMsgHandler;       //������Ϣ����ص�
	TDF_SystemMsgHandler pfnSysMsgNotify;   //ϵͳ��Ϣ֪ͨ�ص�

	unsigned int nProtocol; //Э��ţ�Ϊ0��ΪĬ�ϣ�����0x6001
	const char* szMarkets;  //����"SZ;SH;CF;SHF;DCE;SHF"����Ҫ���ĵ��г��б��ԡ�;���ָ�,Ϊ����������֧�ֵ��г�
	const char* szSubScriptions; //����"600000.sh;ag.shf;000001.sz"����Ҫ���ĵĹ�Ʊ���ԡ�;���ָΪ������ȫ�г�
	unsigned int nDate;     //��������ڣ���ʽYYMMDD��Ϊ0���������
	unsigned int nTime;     //�����ʱ�䣬��ʽHHMMSS��Ϊ0������ʵʱ���飬Ϊ0xffffffff��ͷ����
	unsigned int nTypeFlags;    //Ϊ0��������Ʒ�֣�����ȡֵΪDATA_TYPE_FLAG�ж�����𣬱���DATA_TYPE_MARKET | DATA_TYPE_TRANSACTION

	unsigned int nConnectionID; //����ID�����ӻص���Ϣ�ĸ��ӽṹ TDF_CONNECT_RESULT�� ��������ID
};

//�������ã��ڵ���TDF_Open֮ǰ����
enum TDF_ENVIRON_SETTING
{
    TDF_ENVIRON_HEART_BEAT_INTERVAL,    //Heart Beat�����������, ��ֵΪ0���ʾĬ��ֵ10����
    TDF_ENVIRON_MISSED_BEART_COUNT,     //���û���յ����������������ֵ����û�յ������κ����ݣ����ж�Ϊ���ߣ���ֵ0ΪĬ�ϴ���2��
    TDF_ENVIRON_OPEN_TIME_OUT,          //�ڵ�TDF_Open�ڼ䣬����ÿһ�����ݰ��ĳ�ʱʱ�䣨����������TDF_Open�����ܵ����ȴ�ʱ�䣩����ֵΪ0��Ĭ��30��
	TDF_ENVIRON_OUT_LOG,
	TDF_ENVIRON_NOTTRANS_OLD_DATA, //֪ͨ��������Ҫ�������������
	TDF_ENVIRON_USE_PACK_OVER,
};


enum SUBSCRIPTION_STYLE
{
    SUBSCRIPTION_FULL = 3, //ȫ�г�����
    SUBSCRIPTION_SET=0, //����Ϊ�����б��й�Ʊ��
    SUBSCRIPTION_ADD=1,  //���Ӷ����б��й�Ʊ
    SUBSCRIPTION_DEL=2,   //ɾ���б��еĶ���
};

#pragma pack(pop)
#endif

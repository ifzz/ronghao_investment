#ifndef __WIND_TDF_API_VERSION_H__
#define __WIND_TDF_API_VERSION_H__

#ifdef __cplusplus
extern "C" {
#endif

static const char* TDF_Version()
{
    //TDF API Version: 2383-20140115.1 Comment: �����Ȩ֧��
    //2408-20140116.1: ֧�ֻ�ȡ��Ȩ�����ӿڡ�
    //2448-20140117.1: ���connection id��
    //2501-20140121.1: ���ڻ����߲���������DATA_TYPE_FLAG�����DATA_TYPE_FUTURE_CXö��֧�֡�
    //2504-20140121.1: ��TDF_OPEN_SETTING �е�nConnectionID�ŵ��ṹ��ĺ���ȥ��
    //2565-20140124.1:��1����vld�Ƴ�����2���޸�������ϢnDataLenΪ0��
    //2654-20140211.1:��1��������Ȩdemo
    return "2654-20140211.1";
}

#ifdef __cplusplus
}
#endif

#endif
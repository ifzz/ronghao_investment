#ifndef __Instrument_H
#define __Instrument_H

#include "E15_map.h"
#include "E15_string.h"

#include "stock_data.h"

class InstrumentInfo
{
public:
	E15_String      id;
	E15_String      name;
	ContractInfo		info;		//合约信息
	E15_Intmap 			*m_subscribe_hash; //订阅者列表

	E15_Id				  m_depth;
	E15_Id				 	m_diagram;


public:
	InstrumentInfo();
	virtual ~InstrumentInfo();
};

void del_instrument(void * obj);

#endif

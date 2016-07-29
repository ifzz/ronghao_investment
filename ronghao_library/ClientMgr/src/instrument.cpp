#include "instrument.h"



InstrumentInfo::InstrumentInfo()
{
	m_subscribe_hash = new E15_Intmap;

}

InstrumentInfo::~InstrumentInfo()
{
	delete m_subscribe_hash;
}


void del_instrument(void * obj)
{
	delete (InstrumentInfo*)obj;
}

#include "rhafx.h"

std::deque<raw_dia_group> g_dia_deq;

DataDescription::DataDescription()
{
	class_name[0] = 0;
	name[0] = 0;

	m_dt.class_name = class_name;
	m_dt.name = name;

	m_dt.param = 0;
	m_dt.type_index = 0;
	m_dt.parent_index = 0;

	m_sub = 0;
}

DataDescription::~DataDescription()
{
	delete m_sub;
}



DataDescriptionMgr::DataDescriptionMgr()
{
	m_list = new E15_Queue(0,0);
}

DataDescriptionMgr::~DataDescriptionMgr()
{
	delete m_list;
}

typedef struct OnTagParam
{
	int parent_index;
	E15_Queue * q;
}OnTagParam;

int on_tag_desc(E15_Key * k,E15_Value *v,OnTagParam * p)
{
	unsigned int index = k->GetL();

	DataDescription * desc = (DataDescription *)p->q->At(index,0);
	if( !desc || (desc->m_dt.type_index != index) )
	{
		desc = new DataDescription;
		p->q->InsertAt(desc,index);
	}

	const char * str;

	str = v->BytesS("class",0);
	strncpy(desc->class_name,str,63);
	str = v->BytesS("name",0);
	strncpy(desc->name,str,63);

	desc->m_dt.param = v->BaseS("param");

	desc->m_dt.type_index = index;
	desc->m_dt.parent_index = p->parent_index;

	E15_Debug::Printf(0,"tag[ %d] %s : %s : %ld\n",index,desc->m_dt.class_name,desc->m_dt.name,desc->m_dt.param );

	return 0;
}


int on_data_desc(E15_Key * k,E15_Value *v,E15_Queue *q)
{
	unsigned int index = k->GetL();

	DataDescription * desc = (DataDescription *)q->At(index,0);
	if( !desc || (desc->m_dt.type_index != index) )
	{
		//仅仅做更新处理
		desc = new DataDescription;
		q->InsertAt(desc,index);
	}

	const char * str;

	str = v->BytesS("class",0);
	strncpy(desc->class_name,str,63);
	str = v->BytesS("name",0);
	strncpy(desc->name,str,63);

	desc->m_dt.param = v->BaseS("param");

	desc->m_dt.type_index = index;
	desc->m_dt.parent_index = -1;
	desc->m_dt.store_level = v->BaseS("store");

	E15_Debug::Printf(0,"data[ %d] %s : %s : %ld\n",index,desc->m_dt.class_name,desc->m_dt.name,desc->m_dt.param );

	E15_Value *tags = v->ValueS("tags");
	if( !tags )
		return 0;

	desc->m_sub = new E15_Queue(0,0);

	E15_ValueTable * t = tags->GetValueTable();

	OnTagParam pp;
	pp.parent_index = index;
	pp.q = desc->m_sub;
	t->each( (int (*)(E15_Key * k,E15_Value *v,void *))on_tag_desc,&pp);

	return 0;
}

void DataDescriptionMgr::InitDescription(E15_ValueTable * vt)
{
	vt->each( (int (*)(E15_Key * k,E15_Value *v,void *))on_data_desc,m_list);
}

void DataDescriptionMgr::InitDescription(const char *data, size_t len)
{
	E15_ValueTable vt;
	vt.Import(data, len);

	vt.Print();

	vt.each( (int (*)(E15_Key * k,E15_Value *v,void *))on_data_desc,m_list);
}



DiagramDataFactory::DiagramDataFactory()
{
	m_data = new E15_Queue(0,0);
	m_info = 0;
}

DiagramDataFactory::~DiagramDataFactory()
{
	delete m_data;
}

DiagramDataHandler * DiagramDataFactory::GetDataHandler(int index)
{
	return (DiagramDataHandler *)m_data->At(index,0);
}

inline int is_drop_item(MarketAnalyseDataBase * ref,MarketAnalyseDataBase * data)
{
	if( data->_date > ref->_date)
		return 1;

	if( data->_date < ref->_date)
		return 0;

	if( data->_seq >= ref->_seq )
		return 1;

	return 0;
}

int DiagramDataFactory::LoadCacheData(const char * data,unsigned int len,int index ,int block_size)
{
	int cnt = 0;
	int ret = 0;

	DiagramDataHandler * h = (DiagramDataHandler *)m_data->At(index,0);
	if( !h )
		return 0;
	if( h->m_dt->type_index != (unsigned int)index )
		return 0;

	if( h->ext_len == 0 )
		h->ext_len = block_size - sizeof(MarketAnalyseDataBase);
	if( h->ext_len != (block_size - sizeof(MarketAnalyseDataBase) ) )
			return 0;

	DiagramDataItem * ref_item = (DiagramDataItem *) h->m_data->Tail(0);

	DiagramDataItem * tail = new DiagramDataItem(h->GetTagCount() );
	h->m_data->PutTail(tail);

	//一定是正向排列
	MarketAnalyseDataBase * base = (MarketAnalyseDataBase*)data;
	if( ref_item)
	{
		while( is_drop_item(base,&ref_item->base) )
		{
			len -= block_size;
			data += block_size;

			if( len <= 0 )
				break;

			base = (MarketAnalyseDataBase*)data;
		}
	}

//	int data_index = 8;
//	std::string ins = "ni1609";

	//需要先抛弃比内存中早的数据 to be .....
	while( len >= (unsigned int)block_size )
	{
		const char * ptr = data+block_size *cnt;
		MarketAnalyseDataBase * base = (MarketAnalyseDataBase*)ptr;
		if( base->_date != 0 )
		{

			DiagramDataItem * item = new DiagramDataItem(h->GetTagCount() );
			item->base = *(MarketAnalyseDataBase*)ptr;
			ret++;

			if( h->ext_len > 0 )
			{
				item->pri = new E15_String ;
				item->pri->Memcpy(ptr +sizeof(MarketAnalyseDataBase) ,h->ext_len );
			}
			tail->AddBefore(item);
//			if (!strcmp(ins.c_str(), m_info->id) && data_index == index)
//				printf("[LoadCacheData] ins = %s, date = %d, seq = %d state = %d\n", ins.c_str(), item->base._date, item->base._seq, item->base._state);
		}
		cnt++;
		if( len < (unsigned int)block_size )
			break;
		len -= block_size;

	}


	delete tail;

	return ret;
}



int DiagramDataFactory::LoadCacheTag(const char * data,unsigned int len,int tag_index,int data_index ,int block_size)
{
	int cnt = 0;

	DiagramDataHandler * h = (DiagramDataHandler *)m_data->At(data_index,0);
	if( !h )
		return 0;
	DiagramDataItem * head = (DiagramDataItem * )h->m_data->Head(0);
	if( !head )
		return 0;

	if( tag_index >= head->tag_cnt )
		return 0;

	DiagramDataHandler * tag_h = (DiagramDataHandler *)h->m_sub->At(tag_index,0);

	if( tag_h->ext_len == 0 )
		tag_h->ext_len = block_size - sizeof(MarketAnalyseTagBase) ;
	if( tag_h->ext_len != (block_size - sizeof(MarketAnalyseTagBase)) )
		return 0;

	//找到初始的数据
	MarketAnalyseTagBase * tag = (MarketAnalyseTagBase*)data;
	while( head )
	{
		if( head->base._date > tag->_date )
			return 0;
		if( (head->base._date == tag->_date) && (head->base._seq >= tag->_seq ) )
			break;
		head = (DiagramDataItem * )head->Next();
	}

	if( !head )
		return 0;

	cnt = 0;

	while( len >= (unsigned int)block_size )
	{
		const char * ptr = data+block_size *cnt;

		MarketAnalyseTagBase * tag = (MarketAnalyseTagBase*)ptr;

		if( tag->_date != 0 )
		{
			if( tag->_date == head->base._date && tag->_seq == head->base._seq)
			{
				if( !head->tags[tag_index] )
				{
					head->tags[tag_index] = new DiagramTag();
					if( tag_h->ext_len > 0 )
					{
						head->tags[tag_index]->pri = new E15_String;
					}
				}
				head->tags[tag_index]->base = *tag;
				if( head->tags[tag_index]->pri )
				{
					head->tags[tag_index]->pri->Memcpy(ptr + sizeof(MarketAnalyseTagBase),tag_h->ext_len );
				}
			}
			else
			{
				//E15_Debug::Printf(0,"*** tag[%u:%u] != data[%u:%u] \n",tag->_date,tag->_seq,head->base._date,head->base._seq);

				if( (head->base._date == tag->_date) )
				{ //日期相同，只比较序号，序号小的需要更新到下一个数据
					if( head->base._seq > tag->_seq )
					{
						cnt++;
						len -= block_size;
						continue;
					}

					head = (DiagramDataItem * )head->Next();
					if( !head )
						return 1;
					continue;
				}

				//日期小的需要更新到下一个数据
				if( head->base._date >tag->_date )
				{
					cnt++;
					len -= block_size;
					continue;
				}
				//
				head = (DiagramDataItem * )head->Next();
				if( !head )
					return 1;
				continue;
			}
		}

		cnt++;
		len -= block_size;

		head = (DiagramDataItem * )head->Next();
		if( !head )
			return 1;
	}

	return 1;
}

int DiagramDataFactory::LoadHistoryData(HistoryRequest * req,const char * data,unsigned int len,int index,int more_data)//加载历史数据
{
	if( req->block_size == 0 )
		return 0;
	if(req->block_size > len )
		return 0;

	int cnt = 0;
	int ret = 0;

	DiagramDataHandler * h = (DiagramDataHandler *)m_data->At(index,0);
	if( !h )
		return 0;
	if( h->m_dt->type_index != (unsigned int)index )
		return 0;

	h->m_history_over = !more_data;
	if( h->ext_len == 0 )
		h->ext_len = req->block_size - sizeof(MarketAnalyseDataBase);
	if( h->ext_len != (req->block_size - sizeof(MarketAnalyseDataBase) ) )
			return 0;


	DiagramDataItem * ref_item = (DiagramDataItem *) h->m_data->Head(0);

	DiagramDataItem * head = new DiagramDataItem(h->GetTagCount() );
	h->m_data->PutHead(head);


	if( req->direct > 0 ) //正向排列
	{
		//需要先抛弃比内存中早的数据 to be .....
		while( len >= req->block_size )
		{
			const char * ptr = data+req->block_size *cnt;
			MarketAnalyseDataBase * base = (MarketAnalyseDataBase*)ptr;
			if( base->_date != 0 )
			{

				DiagramDataItem * item = new DiagramDataItem(h->GetTagCount() );
				item->base = *(MarketAnalyseDataBase*)ptr;
				ret++;

				if( h->ext_len > 0 )
				{
					item->pri = new E15_String ;
					item->pri->Memcpy(ptr +sizeof(MarketAnalyseDataBase) ,h->ext_len );
				}
				head->AddBefore(item);
			}
			len -= req->block_size;
			cnt++;
		}
	}
	else //逆向
	{
		MarketAnalyseDataBase * base = (MarketAnalyseDataBase*)data;
		if( ref_item)
		{
			while( is_drop_item(&ref_item->base,base) )
			{
				len -= req->block_size;
				data += req->block_size;

				if( len <= 0 )
					break;

				base = (MarketAnalyseDataBase*)data;
			}
		}

		while( len >= req->block_size )
		{
			const char * ptr = data + len - req->block_size;
			base = (MarketAnalyseDataBase*)ptr;
			if( base->_date != 0 )
			{
				DiagramDataItem * item = new DiagramDataItem(h->GetTagCount() );
				item->base = *(MarketAnalyseDataBase*)ptr;
				ret++;

				if( h->ext_len > 0 )
				{
					item->pri = new E15_String ;
					item->pri->Memcpy(ptr +sizeof(MarketAnalyseDataBase) ,h->ext_len );
				}
				head->AddBefore(item);
			}
			len -= req->block_size;
			cnt++;
		}
	}

	delete head;

	return ret;
}

int DiagramDataFactory::LoadHistoryTag(HistoryRequest * req,const char * data,unsigned int len,int tag_index,int data_index,int more_data)//加载历史数据
{

	//E15_Debug::Printf(0,"\n\n########## LoadHistoryTag data=%d,tag=%d\n\n",data_index,tag_index);
	if( req->block_size == 0 )
		return 0;
	if(req->block_size > len )
		return 0;

	int cnt = 0;

	DiagramDataHandler * h = (DiagramDataHandler *)m_data->At(data_index,0);
	if( !h )
		return 0;
	DiagramDataItem * head = (DiagramDataItem * )h->m_data->Head(0);
	if( !head )
		return 0;

	if( tag_index >= head->tag_cnt )
		return 0;

	DiagramDataHandler * tag_h = (DiagramDataHandler *)h->m_sub->At(tag_index,0);

	if( tag_h->ext_len == 0 )
		tag_h->ext_len = req->block_size - sizeof(MarketAnalyseTagBase) ;
	if( tag_h->ext_len != (req->block_size - sizeof(MarketAnalyseTagBase)) )
		return 0;

	//找到初始的数据

	while( head )
	{
		if( head->base._date > req->date )
			break;
		if( (head->base._date == req->date) && (head->base._seq >= req->seq ) )
			break;
		head = (DiagramDataItem * )head->Next();
	}

	if( !head )
		return 0;

	cnt = 0;

	if( req->direct > 0 ) //正向排列
	{
		while( len >= req->block_size )
		{
			const char * ptr = data+req->block_size *cnt;

			MarketAnalyseTagBase * tag = (MarketAnalyseTagBase*)ptr;

			E15_Debug::Printf(0,"%s:%s:%d tag[%u:%u] ??? data[%u:%u] \n",
					tag_h->m_dt->class_name,tag_h->m_dt->name,tag_h->m_dt->param,
					tag->_date,tag->_seq,head->base._date,head->base._seq);
			if( tag->_date != 0 )
			{

				if( tag->_date == head->base._date && tag->_seq == head->base._seq)
				{
					if( !head->tags[tag_index] )
					{
						head->tags[tag_index] = new DiagramTag();
						if( tag_h->ext_len > 0 )
						{
							head->tags[tag_index]->pri = new E15_String;
						}
					}
					head->tags[tag_index]->base = *tag;
					if( head->tags[tag_index]->pri )
					{
						head->tags[tag_index]->pri->Memcpy(ptr + sizeof(MarketAnalyseTagBase),tag_h->ext_len );
					}
				}
				else
				{
					//E15_Debug::Printf(0,"*** tag[%u:%u] != data[%u:%u] \n",tag->_date,tag->_seq,head->base._date,head->base._seq);

					if( (head->base._date == tag->_date) )
					{ //日期相同，只比较序号，序号小的需要更新到下一个数据
						if( head->base._seq > tag->_seq )
						{
							cnt++;
							len -= req->block_size;
							continue;
						}

						head = (DiagramDataItem * )head->Next();
						if( !head )
							return 1;
						continue;
					}

					//日期小的需要更新到下一个数据
					if( head->base._date >tag->_date )
					{
						cnt++;
						len -= req->block_size;
						continue;
					}
					//
					head = (DiagramDataItem * )head->Next();
					if( !head )
						return 1;
					continue;
				}
			}

			cnt++;
			len -= req->block_size;

			head = (DiagramDataItem * )head->Next();
			if( !head )
				return 1;
		}
	}
	else //反向排列
	{
		while( len >= req->block_size )
		{
			const char * ptr = data + len - req->block_size;

			MarketAnalyseTagBase * tag = (MarketAnalyseTagBase*)ptr;

			if( tag->_date != 0 )
			{

				if( tag->_date == head->base._date && tag->_seq == head->base._seq)
				{
					if( !head->tags[tag_index] )
					{
						head->tags[tag_index] = new DiagramTag();
						if( tag_h->ext_len > 0 )
						{
							head->tags[tag_index]->pri = new E15_String;
						}
					}
					head->tags[tag_index]->base = *tag;
					if( head->tags[tag_index]->pri )
					{
						head->tags[tag_index]->pri->Memcpy(ptr + sizeof(MarketAnalyseTagBase),tag_h->ext_len );
					}
				}
				else
				{
					//E15_Debug::Printf(0,"*** tag[%u:%u] != data[%u:%u] \n",tag->_date,tag->_seq,head->base._date,head->base._seq);

					if( (head->base._date == tag->_date) )
					{ //日期相同，只比较序号，序号小的需要更新到下一个数据
						if( head->base._seq > tag->_seq )
						{
							cnt++;
							len -= req->block_size;
							continue;
						}

						head = (DiagramDataItem * )head->Next();
						if( !head )
							return 1;
						continue;
					}

					//日期小的需要更新到下一个数据
					if( head->base._date >tag->_date )
					{
						cnt++;
						len -= req->block_size;
						continue;
					}
					//
					head = (DiagramDataItem * )head->Next();
					if( !head )
						return 1;
					continue;
				}
			}

			cnt++;
			len -= req->block_size;

			head = (DiagramDataItem * )head->Next();
			if( !head )
				return 1;
		}
	}
	return 1;
}

void insert_package_queue(DiagramDataItem *data, int data_index, int tag_index, int mode) {
	auto it = g_dia_deq.begin();
	for (; it != g_dia_deq.end(); ++it)
		if (it->data == data)
			break;

	if (it == g_dia_deq.end()) {		//没找到
		g_dia_deq.push_back(raw_dia_group());
		it = g_dia_deq.end()-1;
		it->data_index = data_index;
		it->data = data;
	}

	if (tag_index == -1) { 		//data
		it->mode = mode;
//		printf("[insert_package_queue]当前更新的是data=%p, data_index=%d, mode=%d\n", data, data_index, mode);
	} else {		//tag
		it->tag_mode[tag_index] = mode;
//		printf("[insert_package_queue]当前更新的是data（%p）下的tag，data_index=%d, tag_index=%d, mode=%d\n",
//				data, data_index, tag_index, mode);
	}
}

int DiagramDataFactory::OnData(MarketDepthData * depth,int mode,int index,E15_String * vt) 	//网络实时发送的数据
{
	unsigned int len = vt->Length();
	if( len < sizeof(MarketAnalyseDataBase) )
		return 0;

	const char * ptr = vt->c_str();
	MarketAnalyseDataBase * base = (MarketAnalyseDataBase*)ptr;

	int ext_len = len - sizeof(MarketAnalyseDataBase);
	const char * ext = ptr + sizeof(MarketAnalyseDataBase);

	int ret = OnData(depth,mode,index,base,ext,ext_len);
	return ret;
}

int DiagramDataFactory::OnData(MarketDepthData * depth,int mode,int index,MarketAnalyseDataBase * base,const char * ext_data,int len) 	//网络实时发送的数据
{
	if( index == 0)
		return 0;
	DiagramDataHandler * h = (DiagramDataHandler *)m_data->At(index,0);
	if( !h )
		return 0;
	if( h->m_dt->type_index != (unsigned int)index )
		return 0;

	h->m_write_flag |= 0x1;
	if( mode == 1 ) //0 删除，1新增，2修改
	{
		DiagramDataItem * tail = h->PeekDataItem(-1);
		//E15_Debug::Printf(0,"DiagramDataFactory::OnData(%s:%s%d  @[%u:%u])\n",h->m_dt->class_name,h->m_dt->name,h->m_dt->param,base->_date,base->_seq);
		if( tail )
		{
			if( tail->base._date > base->_date) //新增数据日期太旧，抛弃
				return 0;
			if( (tail->base._date == base->_date ) //同一天的数据，新增数据不能比之前数据的序列号小
					&& ( tail->base._seq >= base->_seq) )
				return 0;
		}

		DiagramDataItem * data = new DiagramDataItem(h->GetTagCount() );
		data->base = *base;
		if( len > 0 )
		{
			h->ext_len = len;
			data->pri = new E15_String;
			data->pri->Strcpy(ext_data, h->ext_len);
		}

		if( !h->m_write_item )
			h->m_write_item = data;
		h->m_data->PutTail(data);

		DiagramDataItem *pre = (DiagramDataItem *)data->Pre();
		if( pre)
		{
			pre->base._state = 2;
			insert_package_queue(pre, index, -1, 2);
		}
		insert_package_queue(data, index, -1, mode);


		while (h->m_data->Count() > 2000)
			h->m_data->RemoveHead(0);

		return h->m_data->Count(); //新增后，可能需要写文件了
	}

	DiagramDataItem * data = h->FindData(base->_date,base->_seq);
	if( !data )
	{
		if( mode == 0 )
			return h->m_data->Count(); //不存在

		if( h->m_data->Count() > 0 )
			return h->m_data->Count();
		//增加,第一个
		data = new DiagramDataItem(h->GetTagCount() );
		data->base = *base;
		if( len > 0 )
		{
			h->ext_len = len ;
			data->pri = new E15_String;
			data->pri->Strcpy(ext_data, h->ext_len);
		}

		insert_package_queue(data, index, -1, mode);
		h->m_data->PutHead(data);
		if( !h->m_write_item )
			h->m_write_item = data;

		return h->m_data->Count();

	}

	if( mode == 0 )
	{
		if( h->m_write_item == data)
			h->m_write_item = (DiagramDataItem *)data->Next();

		delete data;
		return h->m_data->Count();
	}

	data->need_write |= 0x1;
	data->base = *base;
	if( len > 0  )
	{
		if( !data->pri )
			data->pri = new E15_String;
		data->pri->Strcpy(ext_data, len);
	}

	if( !h->m_write_item )
		h->m_write_item = data;
	insert_package_queue(data, index, -1, mode);
	return h->m_data->Count();

}

int DiagramDataFactory::OnTag(MarketDepthData * depth,int mode,int data_index,int tag_index,E15_String * vt)		//网络实时发送的数据
{
	const char * ptr = vt->c_str();
	MarketAnalyseTagBase * base = (MarketAnalyseTagBase*)ptr;
	const char * ext_data = ptr + sizeof(MarketAnalyseTagBase);
	int ext_len = vt->Length() - sizeof(MarketAnalyseTagBase);

	int ret = OnTag(depth,mode,data_index,tag_index,base,ext_data,ext_len);

	return ret;

}


inline int is_old_item(MarketAnalyseTagBase * base,DiagramDataItem * item)
{
	if( base->_date < item->base._date)
		return 1;
	if( base->_date > item->base._date)
		return 0;

	if( base->_seq < item->base._seq)
		return 1;
	return 0;
}

int DiagramDataFactory::OnTag(MarketDepthData * depth,int mode,int data_index,int tag_index,MarketAnalyseTagBase * base,const char * ext_data,int len)		//网络实时发送的数据
{
	DiagramDataHandler * h = (DiagramDataHandler *)m_data->At(data_index,0);
	if( !h )
		return 0;
	if( !h->m_sub )
		return 0;
	if( h->m_dt->type_index != (unsigned int )data_index )
		return 0;

	if( (unsigned int )tag_index >= h->m_sub->Count() )
		return 0;

	h->m_write_flag |= 0x2;

	DiagramDataHandler * tag_h = (DiagramDataHandler *)h->m_sub->At(tag_index,0);

	DiagramDataItem * data = h->FindData(base->_date,base->_seq);
	if( !data )
		return 0;

	if( !tag_h->m_write_item )
		tag_h->m_write_item = data;
	else if( is_old_item(base,tag_h->m_write_item))
	{
		tag_h->m_write_item = data;
	}

	data->need_write |= 0x2;

	if( mode == 0 && data->tags[tag_index])
	{
//		data->tags[tag_index]->base._date = 0; //做一个无效标记即可
		insert_package_queue(data, data_index, tag_index, mode);
		return 1;
	}

	DiagramTag * tag = 0;
	if( mode == 2 && data->tags[tag_index])
	{
		tag  = data->tags[tag_index];

		tag->base = *base;
		if( len > 0 )
		{
			tag_h->ext_len = len ;
			tag->pri->Strcpy(ext_data, tag_h->ext_len);
		}
		insert_package_queue(data, data_index, tag_index, mode);
		return 1;
	}

	//新增

	if( data->tags[tag_index] )
		tag = data->tags[tag_index];
	else
	{
		tag = new DiagramTag;

		if( len > 0 )
		{
			if( !tag->pri)
				tag->pri = new E15_String;
		}
		data->tags[tag_index] = tag;
	}


	tag->base = *base;
	if( len > 0 )
	{
		tag_h->ext_len = len ;
		tag->pri->Strcpy(ext_data, tag_h->ext_len );
	}

	insert_package_queue(data, data_index, tag_index, mode);
	return 1;
}


int factory_on_tag(DataDescription * desc, DiagramDataHandler * h)
{
	E15_Queue * sub = h->PeekSub();

	DiagramDataHandler * tags = (DiagramDataHandler *)sub->At(desc->m_dt.type_index,0);
	if( !tags || (tags->m_dt->type_index != desc->m_dt.type_index) )
	{
		tags = new DiagramDataHandler;
		sub->InsertAt(tags,desc->m_dt.type_index);
	}

	tags->m_dt = &desc->m_dt;

	return 0;
}


int factory_on_data(DataDescription * desc, DiagramDataFactory * factory)
{
	DiagramDataHandler * h = (DiagramDataHandler *)factory->m_data->At(desc->m_dt.type_index,0);
	if( !h || (h->m_dt->type_index != desc->m_dt.type_index) )
	{
		h = new DiagramDataHandler;
		factory->m_data->InsertAt(h,desc->m_dt.type_index);
	}

	h->m_dt = &desc->m_dt;

	if( !desc->m_sub )
		return 0;

	h->m_sub = new E15_Queue(0,0);
	desc->m_sub->each((int (*)(E15_Object * ,void *) ) factory_on_tag , h);

	return 0;
}


void DiagramDataFactory::Init(E15_Queue * q)
{
	q->each( (int (*)(E15_Object * ,void *) ) factory_on_data , this);
}

void DiagramDataFactory::Reset() //清理数据，避免内存占用太多
{

}

E15_Queue *	DiagramDataHandler::PeekSub()
{
	return m_sub;
}


DiagramDataHandler::DiagramDataHandler()
{
	m_data = new E15_Queue(0,0);
	m_sub = 0;
	m_write_item = 0;
	m_write_flag = 0;
	m_write_date = 0;
	m_write_seq = 0;
	m_history_over = 0;
	ext_len = 0;

	m_write_cache = new E15_String;
	m_write_pos = 0;
	m_cache_date = 0;
	m_cache_seq = 0;

	m_hash = rand() % 500;
}

DiagramDataHandler::~DiagramDataHandler()
{
	delete m_data;
	delete m_sub;
	delete m_write_cache;
}

inline int is_need_data(DiagramDataItem * data,unsigned int  date,unsigned int seq)
{
	if( date != data->base._date )
	{
		if( data->base._date < date  )
			return -1;
		return 0;
	}
	if( seq != data->base._seq )
	{
		if(data->base._seq < seq )
			return -1;
		return 0;
	}
	return 1;
}

DiagramDataItem * DiagramDataHandler::FindData(unsigned int  date,unsigned int seq)
{
	DiagramDataItem * data = (DiagramDataItem * )m_data->Tail(0);
	int ret = 0;
	while( data )
	{
		ret = is_need_data(data,date,seq);
		if( ret > 0 )
			return data;
		if( ret < 0 )
			return 0;
		data = (DiagramDataItem * )data->Pre();
	}
	return 0;
}



DiagramDataItem * DiagramDataHandler::PeekDataItem(int offset)
{
	return (DiagramDataItem *)m_data->At(offset,0);
}

int DiagramDataHandler::GetTagCount()
{
	if( !m_sub )
		return 0;
	return m_sub->Count();
}

E15_Queue * DiagramDataHandler::PeekData()
{
	return m_data;
}

MarketDataType * DiagramDataHandler::GetDataType()
{
	return m_dt;
}

MarketDataType * DiagramDataHandler::GetTagType(int index)
{
	if( !m_sub )
		return 0;

	DiagramDataHandler * h = (DiagramDataHandler *)m_sub->At(index,0);
	if( !h )
		return 0;
	if( h->m_dt->type_index != (unsigned int )index )
		return 0;
	return h->m_dt;
}



MarketDataType * DiagramDataHandler::DataType()
{
	return m_dt;
}

void DiagramDataHandler::BuildHistoryReq(E15_String * req) //请求的历史数据格式
{
	E15_ValueTable vt;
	E15_ValueTable * data = &vt;//.InsertTableI(m_dt->type_index);

	data->SetSS("class",m_dt->class_name);
	data->SetSS("name",m_dt->name);
	data->SetSI("param",m_dt->param);
	data->SetSI("di",m_dt->type_index);
	data->SetSI("store",m_dt->store_level);

	E15_Debug::Printf(0,"BuildHistoryReq %s:%s:%d store=%d\n",m_dt->class_name,m_dt->name,m_dt->param,m_dt->store_level);

	if( m_sub && m_sub->Count() > 0)
	{
		E15_ValueTable * tags = data->InsertTableS("tags");
		DiagramDataHandler * h = (DiagramDataHandler *)m_sub->Head(0);
		while(h )
		{
			E15_ValueTable * tag = tags->InsertTableI(h->m_dt->type_index);
			tag->SetSS("class",h->m_dt->class_name);
			tag->SetSS("name",h->m_dt->name);
			tag->SetSI("param",h->m_dt->param);
			tag->SetSI("di",h->m_dt->type_index);
			tag->SetSI("store",h->m_dt->store_level);

			E15_Debug::Printf(0,"BuildHistoryReq %s:%s:%d store=%d\n",h->m_dt->class_name,h->m_dt->name,h->m_dt->param,h->m_dt->store_level);

			h = (DiagramDataHandler *)h->Next();
		}
	}

	vt.Dump(req);

}

int DiagramDataHandler::SaveData(E15_Zip * zip)
{
	DiagramDataItem * di = (DiagramDataItem *)m_data->Head(0);
	if( !di )
		return 0;
	int ret = sizeof(di->base);
	if( di->pri)
		ret += di->pri->Length();

	while(di)
	{
		zip->zip((const char *)&di->base,sizeof(di->base));
		if(di->pri)
			zip->zip(di->pri->c_str(),di->pri->Length());

		di = (DiagramDataItem *)di->Next();
	}
	return ret;
}

int DiagramDataHandler::SaveTag(int index,E15_Zip * zip)
{
	DiagramDataItem * di = (DiagramDataItem *)m_data->Head(0);
	if( !di )
		return 0;
	int ret = 0;
	while(di)
	{
		if( di->tags[index] && (di->tags[index]->base._date != 0 ) )
		{
			zip->zip((const char *)&di->tags[index]->base,sizeof(di->tags[index]->base));
			if(di->tags[index]->pri)
			{
				zip->zip(di->tags[index]->pri->c_str(),di->tags[index]->pri->Length());
			}
			if( ret == 0 )
			{
				ret = sizeof(di->tags[index]->base);
				if(di->tags[index]->pri)
					ret += di->tags[index]->pri->Length();
			}
		}
		di = (DiagramDataItem *)di->Next();
	}
	return ret;
}

DataDescriptionMgr g_data_tag_info;



	E15_ValueTable * vt = m_instrument_list.InsertTableS(id);
	market = MarketCodeById(id);
	vt->SetSI("market", market);
	vt->SetSS("name", id);
	vt->SetSI("tick", 100);
	vt->SetSI("Multiple", 1);
	vt->SetSS("exchange", "sh");
	vt->SetSS("product", ":CNA");

	E15_String s;
	m_instrument_list.Dump(&s);

	m_instrument_buffer.Reset();
	m_zip.zip_start(&m_instrument_buffer);
	m_zip.zip(s.c_str(), s.Length());
	m_zip.zip_end();




	if( m_instrument_buffer.Length() > 0 )
	{
		E15_ServerCmd  cmd;
		cmd.cmd = Stock_Msg_InstrumentList; //合约代码信息
		cmd.status = market;

		Notify(&info->id,0,&cmd,m_instrument_buffer.c_str(),m_instrument_buffer.Length(),1 );
	}

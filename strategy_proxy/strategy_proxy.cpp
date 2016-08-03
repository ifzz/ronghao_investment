#include <stdio.h>
#include <string.h>

#include "strategy_proxy.h"
#include "stock_msg.h"
#include "E15_value.h"

E15_Socket g_socket;

void trans::start() {
	crx::xml_parser xml;
	xml.load("ini/config.xml", "config");
	xml.for_each_child([&](const char *name, const char *value, std::map<std::string, const char*>& attr, void*)->void {
		if (!strncmp("role", name, strlen(name))) {
			m_role.real_md = attr["real_md"];
			m_role.his_md = attr["his_md"];
			m_role.trade = attr["trade"];
			m_role.client_ui = attr["ui"];
			m_role.ope_mgr = attr["ope_mgr"];
		}
	}, nullptr);

	g_socket.Start();
	Start(&g_socket, "ini/proxy.ini");
}

void trans::stop() {
	Stop();
	g_socket.Stop();
}

int trans::OnOpen(E15_ServerInfo * info,E15_String *& json) {
	printf("\n[%x:%x] (N=%d,name=%s:role=%s) 上线\n",info->id.h,info->id.l,info->N,info->name,info->role);
	if (info->N == 0) {		//当前连接的是策略客户端
		m_sub_list.push_back(std::make_shared<node_sub>());
		m_sub_list.back()->id = info->id;
		printf("客户端[%x:%x]完成连接：name=%s，role=%s\n", info->id.h, info->id.l, info->name, info->role);

		E15_ServerCmd cmd;
		cmd.cmd = Stock_Msg_InstrumentList;		//先发送合约列表
		if (m_ins_list)
			Notify(&info->id, 0, &cmd, m_ins_list->c_str(), m_ins_list->Length());

		cmd.cmd = Stock_Msg_DiagramInfo;		//再转发指标描述信息
		if (m_diagram_info)
			Notify(&info->id, 0, &cmd, m_diagram_info->c_str(), m_diagram_info->Length());
	} else {		//当前连接的是域中的其他服务器
		if (!strcmp(info->role, m_role.real_md.c_str()) ||			//实时行情服务器
				!strcmp(info->role, m_role.his_md.c_str()) || 		//历史行情服务器
				!strcmp(info->role, m_role.trade.c_str()) ||			//交易服务器
				!strcmp(info->role, m_role.client_ui.c_str()) ||		//前端界面节点
				!strcmp(info->role, m_role.ope_mgr.c_str())) {	//后台运维节点
			m_role_id[info->role] = info->id;
			printf("服务器端完成连接：name=%s，role=%s\n", info->name, info->role);
		}
	}
	return 1;
}

int trans::OnClose(E15_ServerInfo * info) {
	printf("\n[%x:%x] (N=%d,name=%s:role=%s) 下线\n",info->id.h,info->id.l,info->N,info->name,info->role);
	if (info->N == 0) {
		printf("客户端断开连接：name=%s，role=%s\n\n", info->name, info->role);
		for (auto it = m_sub_list.begin(); it != m_sub_list.end(); ++it) {
			if ((*it)->id == info->id) {
				m_sub_list.erase(it);
				break;
			}
		}
		return -1;
	}
	return 1000;		//域中的其他服务器断开，自动重联
}

void trans::OnRequest(E15_ServerInfo * info,E15_ServerRoute * rt,E15_ServerCmd * cmd,E15_String *& data) {
	E15_ValueTable vt;
	vt.Import(data->c_str(), data->Length());
	E15_Value *v = vt.ValueS("conn_real");

	switch (cmd->cmd) {
	case Stock_Msg_SubscribeById:
	case Stock_Msg_UnSubscribeById:
	case Stock_Msg_SubscribeAll:
	case Stock_Msg_UnSubscribeAll: {		//上述请求转发给行情服务器
		printf("收到客户端的订阅请求 [%x:%x] (N=%d, name=%s, role=%s), cmd=%d, 是否连接历史行情服务器：%s\n",
				info->id.h, info->id.l, info->N, info->name, info->role, cmd->cmd, v->GetInt() ? "否" : "是");

		E15_StringArray *sa = vt.ValueS("id_list")->GetStringArray();
		for (auto& node : m_sub_list) {
			if (node->id != info->id)
				continue;

			for (unsigned long i = 0; i < sa->Size(); ++i)
				node->subs.insert(sa->At(i)->c_str());
		}

		Request(&m_role_id[m_role.real_md], 0, cmd, data->c_str(), data->Length());
		if (!v->GetInt())		//与历史行情节点有连接，那么还要将请求转发给历史行情服务器
			Request(&m_role_id[m_role.his_md], 0, cmd, data->c_str(), data->Length());
		break;
	}

	case TRADE_MSG_INPUT_ORDER: {		//这一请求发送给交易服务器及前端界面
		printf("收到客户端的订阅请求 [%x:%x] (N=%d, name=%s, role=%s), cmd=%d\n",
				info->user_id.h, info->user_id.l, info->N, info->name, info->role, cmd->cmd);
		if (m_role_id.end() != m_role_id.find(m_role.trade))
			Request(&m_role_id[m_role.trade], 0, cmd, data->c_str(), data->Length());
		if (m_role_id.end() != m_role_id.find(m_role.client_ui))
			Notify(&m_role_id[m_role.client_ui], 0, cmd, data->c_str(), data->Length());
		break;
	}
	}
}

void trans::OnNotify(E15_ServerInfo * info,E15_ServerRoute * rt,E15_ServerCmd * cmd,E15_String *& data) {
//	printf("@@@@@@ [%x:%x] cmd = %d, data = %s\n", info->id.h, info->id.l, cmd->cmd, data->c_str());

	if (Stock_Msg_InstrumentList == cmd->cmd && !m_ins_list) {
		printf("trans::OnNotify [%x:%x]cmd=%d 收到合约列表!\n", info->id.h, info->id.l, cmd->cmd);
		m_ins_list = data;
		data = nullptr;

		for (auto& node : m_sub_list)
			Notify(&node->id, 0, cmd, m_ins_list->c_str(), m_ins_list->Length());
		return;
	}

	if (Stock_Msg_DiagramInfo == cmd->cmd && !m_diagram_info) {
		printf("trans::OnNotify [%x:%x]cmd=%d 收到指标描述信息!\n\n", info->id.h, info->id.l, cmd->cmd);
		m_diagram_info = data;
		data = nullptr;

		for (auto& node : m_sub_list)
			Notify(&node->id, 0, cmd, m_diagram_info->c_str(), m_diagram_info->Length());
		return;
	}

	for (auto& node : m_sub_list)
		if (node->subs.end() != node->subs.find(data->c_str()))
			Notify(&node->id, 0, cmd, data->c_str(), data->Length());
}

int main(int argc, char *argv[]) {
	proxy app;
	return app.run(argc, argv);
}

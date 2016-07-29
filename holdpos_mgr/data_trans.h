#pragma once

class data_trans : public E15_Server {
public:
	data_trans();
	virtual ~data_trans() {}

	int start();
	void stop();

public:
	/* server callback */
	virtual int OnOpen(E15_ServerInfo * info,E15_String *& json) { return 1; }
	virtual int OnClose(E15_ServerInfo * info) { return 1000; }
	virtual void OnRequest(E15_ServerInfo * info,E15_ServerRoute * rt,E15_ServerCmd * cmd,E15_String *& data) {}
	virtual void OnResponse(E15_ServerInfo * info,E15_ServerRoute * rt,E15_ServerCmd * cmd,E15_String *& data) {}
	virtual void OnNotify(E15_ServerInfo * info,E15_ServerRoute * rt,E15_ServerCmd * cmd,E15_String *& data) {}

private:

};

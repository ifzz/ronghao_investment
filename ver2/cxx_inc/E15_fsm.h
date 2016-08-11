#ifndef __E15_fsm_H
#define __E15_fsm_H

#include "E15_object.h"

typedef struct E15_fsm_ * E15_fsm;
typedef struct E15_fsm_item_ *  E15_fsm_item;
typedef struct E15_fsm_state_ * E15_fsm_state;


#define E15_fsm_stateInitState 0
#define E15_fsm_stateSelfState ""

//驱动对象状态的事件
typedef struct E15_FsmMsg
{
	const char * sMsg;
	unsigned long iMsg;
	long status;
	void * params;
}E15_FsmMsg;

class E15_FsmState;

class E15_cxx_object_API E15_FsmObject : public E15_Object
{
public:
    E15_FsmObject();
    virtual ~E15_FsmObject();
};

typedef void  ( E15_FsmObject::*E15_FsmHandler) (E15_FsmMsg * msg,E15_FsmState * pre,E15_FsmState *current,E15_FsmState*next);


class E15_cxx_object_API E15_FsmState : public E15_Object
{
public:
    E15_FsmState();
    virtual ~E15_FsmState();

	const char * Name();

	//整形消息处理函数和对应状态逻辑
	void SetIntMsgHandle(unsigned long iMsg,long Status,E15_FsmHandler pHandler ,const char * pNextStateName);
	void SetIntMsgHandleRange(unsigned long iMsg,long StatusLeft,long StatusRight,E15_FsmHandler pHandler ,const char * pNextStateName);
	void SetIntMsgHandleDefault(unsigned long iMsg,E15_FsmHandler pHandler ,const char * pNextStateName);

	//字符串型消息处理函数和对应状态逻辑
	void SetStrMsgHandle(const char * sMsg,long Status,E15_FsmHandler pHandler ,const char * pNextStateName);
	void SetStrMsgHandleRange(const char * sMsg,long StatusLeft,long StatusRight,E15_FsmHandler pHandler ,const char * pNextStateName);
	void SetStrMsgHandleDefault(const char * sMsg,E15_FsmHandler pHandler ,const char * pNextStateName);

	//未知事件的处理
	void SetUnExceptEventHandle(E15_FsmHandler pHandler ,const char * pNextStateName);
	//进入状态的处理，pre and next is null
	void SetStateEnterHandler(E15_FsmHandler pHandler);
	//无条件转移到pNextStateName状态，pre and next is null
	void SetForceState(const char * pNextStateName,E15_FsmHandler pHandler );
	//退出状态的处理函数，pre and next is null
	void SetStateExitHandler(E15_FsmHandler pHandler);
	//进入某个状态后，该对象的状态机完结，需要作资源回收了
	void SetStateCleanHandler(E15_FsmHandler pHandler);
};


//first state = GetState(NULL);
//state_current ---msg-->(1 handler msg,2,state_current state exit) ----> new_state(state enter)
class E15_Log;
class E15_cxx_object_API E15_Fsm : public E15_Object
{
public:
    E15_Fsm();
    virtual ~E15_Fsm();

	void HandleStrMsg(E15_FsmObject * item,const char *  sMsg,long status,void * params);
	void HandleIntMsg(E15_FsmObject * item,unsigned long iMsg,long status,void * params);

	//增加状态的名称不能为空或者NULL
	//初始状态不支持无条件转
	E15_FsmState * AddState(const char * name);
	E15_FsmState * GetState(const char * name);
	unsigned long GetStateCount();

	//任何状态下都可能要做的响应
	void SetIntMsgHandle(unsigned long iMsg,long Status,E15_FsmHandler pHandler,const char * pNextStateName );
	void SetIntMsgHandleRange(unsigned long iMsg,long StatusLeft,long StatusRight,E15_FsmHandler pHandler,const char * pNextStateName );
	void SetIntMsgHandleDefault(unsigned long iMsg,E15_FsmHandler pHandler,const char * pNextStateName );

	void SetStrMsgHandle(const char * sMsg,long Status,E15_FsmHandler pHandler,const char * pNextStateName );
	void SetStrMsgHandleRange(const char * sMsg,long StatusLeft,long StatusRight,E15_FsmHandler pHandler,const char * pNextStateName );
	void SetStrMsgHandleDefault(const char * sMsg,E15_FsmHandler pHandler,const char * pNextStateName );

	//检查是否有孤岛状态以及未定义状态,0表示OK，0x1表示引用了未定义的状态，0x2表示存在未被引用的状态，0x3 = 0x1 && 0x2
	int Inspect();
	void SetLog(E15_Log * log);
};


#endif

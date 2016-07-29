#ifndef __E15_app_H
#define __E15_app_H

#include "E15_object.h"

class E15_cxx_object_API E15_ConsoleApp
{
public:
    E15_ConsoleApp();
    virtual ~E15_ConsoleApp();

    virtual int OnInit(int argc,char * argv[]) = 0;
    virtual void OnDestroy() = 0;

	static void SetInfo(const char * version,const char * author,const char * build_time) ;
	static void AddCmd(const char * cmd,const char * cmd_short ,void (*)(const char * cmdline) ,const char * comment) ;

	static int Run(int argc,char * argv[] ); //default console

	static void Register(const char * name,E15_Object  * obj);
	static E15_Object * Find(const char * name) ;

	static int ArgK(const char ** cmd_line  ,const char ** pname ) ;
	static int ArgV(const char ** cmd_line ,const char ** pvalue );
};

#endif

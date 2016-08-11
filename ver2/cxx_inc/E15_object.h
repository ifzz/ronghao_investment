#ifndef __E15_object_H
#define __E15_object_H

#ifdef E15_cxx_object_export
        #ifndef E15_cxx_object_API
                #ifdef WIN32
                        #define E15_cxx_object_API __declspec(dllexport)
                #else
                        #define E15_cxx_object_API __attribute__((visibility("default")))
                #endif
        #endif
#else
        #ifndef E15_cxx_object_API
                #ifdef WIN32
                        #define E15_cxx_object_API __declspec(dllimport)
                #else
                        #define E15_cxx_object_API
                #endif
        #endif
#endif


#pragma pack(1)
class E15_cxx_object_API E15_Id
{
public:
    unsigned int h;//对象的id(高位)
	unsigned int l;//对象的id(低位)

    E15_Id();
    E15_Id(unsigned int h,unsigned int l);
    E15_Id(const E15_Id & id);

    ~E15_Id();
    void Reset();


    E15_Id & operator = (const E15_Id & data);

    friend E15_cxx_object_API int operator == (const E15_Id &,const E15_Id &);
	friend E15_cxx_object_API int operator != (const E15_Id &,const E15_Id &);
};

#pragma pack()


class E15_cxx_object_API E15_Object
{
public :
    E15_Object();
    virtual ~E15_Object();

	virtual void   Reset();	//对象复位
    E15_Object & operator = (const E15_Object & data);
    friend E15_cxx_object_API int operator == (const E15_Object &,const E15_Object &);//判断对象是否相同
	friend E15_cxx_object_API int operator != (const E15_Object &,const E15_Object &);

public:
    void *          cObj;
};

E15_cxx_object_API void E15_CxxObject_Delete( void * obj);

#endif

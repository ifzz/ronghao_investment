#ifndef __E15_value_H
#define __E15_value_H


#include "E15_object.h"

#include "E15_string.h"
#include "E15_string_array.h"
#include "E15_key.h"


///////////////////////////////////////////////////////////////
// value
///////////////////////////////////////////////////////////////
class E15_ValueTable;

class E15_cxx_object_API E15_Value : public E15_Object
{

public:
    E15_Value();
    virtual ~E15_Value();

    enum ValueType {
        Value_Unknown = 0,
        Value_CHAR,
        Value_UCHAR,
        Value_SHORT,
        Value_USHORT,
        Value_INT,
        Value_UINT,
        Value_LONG,
        Value_ULONG,

        Value_PTR,
        Value_Ref,
        //
        Value_FloatStart,
        Value_Int64,
        Value_UInt64,
        Value_FLOAT,
        Value_DOUBLE,
        Value_LDOUBLE,
        Value_ArrayStart,

        Value_STRING,
        Value_PCHAR,
        Value_PUCHAR,
        Value_PSHORT,
        Value_PUSHORT,
        Value_PINT,
        Value_PUINT,
        Value_PLONG,
        Value_PULONG,

        Value_PFLOAT,
        Value_PDOUBLE,
        Value_PLDOUBLE,
        Value_ArrayOtherObject,

        Value_STRINGARRAY,
        Value_Table,
        Value_End
    };

	unsigned long Dump(E15_String *buf);
	unsigned long Import(const char * bytearray,int maxlen);

	E15_Value::ValueType Type();
	void Init(E15_Value::ValueType type);

	void SetString(const char * data,int len);
	void SetChar(const char data);
	void SetShort(const short data);
	void SetLong(const long data);
	void SetInt(const int data);
	void SetFloat(const float data);
	void SetDouble(const double data);
	void SetInt64(const long long data);

	void SetUChar(const unsigned char data);
	void SetUShort(const unsigned short data);
	void SetULong(const unsigned long data);
	void SetUInt(const unsigned int data);

	void SetShortArray(const short * data,int icount);
	void SetIntArray(const int * data,int icount);
	void SetLongArray(const long * data,int icount);
	void SetFloatArray(const float * data,int icount);
	void SetDoubleArray(const double * data,int icount);
	void SetUShortArray(const unsigned short * data,int icount);
	void SetUIntArray(const unsigned int * data,int icount);
	void SetULongArray(const unsigned long * data,int icount);

	void SetStringArray(const E15_StringArray * data);
	void SetTable(const E15_ValueTable * data);
	void SetPtr(const void * ptr);
	void SetRef(const E15_Value * pRealValue);

	void AppendString(const E15_String * data);
	void AppendStringArray(const E15_StringArray * data);

	void AppendShort(const short data);
	void AppendLong(const long data);
	void AppendInt(const int data);

	void AppendUShort(const unsigned short data);
	void AppendULong(const unsigned long data);
	void AppendUInt(const unsigned int data);

	void AppendFloat(const float data);
	void AppendDouble(const double data);
	void AppendBytes(const char * data,int len);

	//////////////////////////////////////////////////////////////////////////
	char GetChar();
	short GetShort();
	int  GetInt();
	long GetLong();
	float GetFloat();
	double GetDouble();
	long long GetInt64();

	unsigned char GetUChar();
	unsigned short GetUShort();
	unsigned int  GetUInt();
	unsigned long GetULong();
	void * GetPtr();

	char * GetBytes(unsigned long * length);
	short * GetShortArray(unsigned long *itemcount);
	int * GetIntArray(unsigned long * itemcount);
	long * GetLongArray(unsigned long * itemcount);
	float * GetFloatArray(unsigned long * itemcount);
	double * GetDoubleArray(unsigned long * itemcount);


	unsigned short * GetUShortArray(unsigned long *itemcount);
	unsigned int * GetUIntArray(unsigned long * itemcount);
	unsigned long * GetULongArray(unsigned long * itemcount);

	E15_String * GetString();
	E15_StringArray * GetStringArray();
	E15_ValueTable *  GetValueTable();

	E15_Value * GetRef();


	void Echo(void pf(void *obj,const char * ,...),void * p_obj,int dense);
	void Print();

	const char * Json_encode( E15_String * outstr); //如果outstr == NULL,表示输出到stdout
	int Json_decode(const char * data,int len);


	E15_Value * SetSS(const char * skey,const char * value);
	E15_Value * SetIS(unsigned long ukey,const char * value);

	E15_Value * SetSI(const char * skey,long value);
	E15_Value * SetII(unsigned long ukey,long value);

	E15_Value * SetPtrS(const char * skey,void *);
	E15_Value * SetPtrI(unsigned long ukey,void *);

	E15_Value * InsertS(const char * key);
	E15_Value * InsertI(unsigned long ukey);
	E15_Value * InsertK(E15_Key * key);

	E15_Value * InsertAt(int index);
	void RemoveAt(int index);

	void ModifyKeyK(int index,E15_Key * key);
	void ModifyKeyS(int index,const char * key);
	void ModifyKeyI(int index,unsigned long key);

	E15_Value * ValueK(const E15_Key * );
	E15_Value * ValueS(const char *);
	E15_Value * ValueI(const unsigned long ukey);

	void * PtrK(const E15_Key * );
	void * PtrS(const  char *);
	void * PtrI(const unsigned long ukey);

	char * BytesK(const E15_Key * ,unsigned long * length );
	char * BytesS(const  char *,unsigned long * length );
	char * BytesI(const unsigned long ukey,unsigned long * length);

	unsigned long BaseK(const E15_Key * );
	unsigned long BaseS(const  char *);
	unsigned long BaseI(const unsigned long ukey);

	long long Int64K(const E15_Key * );
	long long Int64S(const  char *);
	long long Int64I(const unsigned long ukey);

	E15_Value * First(E15_Key **) ;
	E15_Value * Next(E15_Key **) ;
	E15_Value * At(int index,E15_Key **) ;

	void RemoveK(E15_Key *);
	void RemoveS(const char * key);
	void RemoveI(unsigned long ukey);
	void RemoveAll();

	unsigned long Count();

};

///////////////////////////////////////////////////////////////
// tables
///////////////////////////////////////////////////////////////

class E15_cxx_object_API E15_ValueTable : public E15_Object
{
public:
    E15_ValueTable();
    virtual ~E15_ValueTable();

	unsigned long Dump(E15_String *buf);
	unsigned long Import(const char * bytearray,int maxlen);

	E15_Value * SetSS(const char * skey,const char * value);
	E15_Value * SetIS(unsigned long ukey,const char * value);

	E15_Value * SetSI(const char * skey,long value);
	E15_Value * SetII(unsigned long ukey,long value);

	E15_Value * SetSInt64(const char * skey,long long value);
	E15_Value * SetIInt64(unsigned long ukey,long long value);

	E15_Value * SetPtrS(const char * skey,void *);
	E15_Value * SetPtrI(unsigned long ukey,void *);

	E15_Value * InsertS(const char * key);
	E15_Value * InsertI(unsigned long ukey);
	E15_Value * InsertK(E15_Key * key);

	E15_Value * InsertAt(int index);
	void RemoveAt(int index);

	void ModifyKeyK(int index,E15_Key * key);
	void ModifyKeyS(int index,const char * key);
	void ModifyKeyI(int index,unsigned long key);


	E15_ValueTable * InsertTableI(unsigned long ukey);
	E15_ValueTable * InsertTableS(const char * skey);
	E15_ValueTable * InsertTableK(E15_Key *key);

	E15_Value * ValueK(const E15_Key * );
	E15_Value * ValueS(const char *);
	E15_Value * ValueI(const unsigned long ukey);

	E15_ValueTable * TableK(const E15_Key * );
	E15_ValueTable * TableS(const char *);
	E15_ValueTable * TableI(const unsigned long ukey);

	void * PtrK(const E15_Key * );
	void * PtrS(const char *);
	void * PtrI(const unsigned long ukey);

	char * BytesK(const E15_Key * ,unsigned long * length = 0 );
	char * BytesS(const char *,unsigned long * length = 0);
	char * BytesI(const unsigned long ukey,unsigned long * length = 0);

	unsigned long BaseK(const E15_Key * );
	unsigned long BaseS(const char *);
	unsigned long BaseI(const unsigned long ukey);

	long long Int64K(const E15_Key * );
	long long Int64S(const char *);
	long long Int64I(const unsigned long ukey);

	E15_Value * First(E15_Key **) ;
	E15_Value * Next( E15_Key **) ;
	E15_Value * At( int index,E15_Key **) ;

	void RemoveK(E15_Key *);
	void RemoveS(const char * key);
	void RemoveI(unsigned long ukey);
	void RemoveAll();

	unsigned long Count();

	E15_ValueTable * Append(const E15_ValueTable * data);
	void CaseFlag(int cf);

	void each( int f(E15_Key *,E15_Value *,void *),void * params );
	void Print();
	void Echo( void pf(void *obj,const char * ,...),void * p_obj,int dense);

};


#endif

#ifndef __E15_map_H
#define __E15_map_H

#include "E15_key.h"

class E15_cxx_object_API E15_Strmap : public E15_Object
{
public:
    E15_Strmap();
    virtual ~E15_Strmap();

	unsigned long HashTableSize();
	void Init(unsigned long hashSize);

	int Count();
	int IsEmpty() ;
	void CaseFlag(int cf);

	void * LookupK(E15_Key * key);
	void * LookupI(unsigned long ukey);
	void * LookupS(const char * key);
	void * LookupSI(const char * strkey,unsigned long ukey);

	void SetAtK(void * newValue,E15_Key * key);
	void SetAtI(void * newValue,unsigned long ukey);
	void SetAtS(void * newValue,const char * strkey);
	void SetAtSI(void * newValue,const char * strkey,unsigned long ukey);

	void * RemoveK(E15_Key * key);
	void * RemoveI(unsigned long ukey);
	void * RemoveS(const char * strkey);
	void * RemoveSI(const char * strkey,unsigned long ukey);

	void RemoveAll();

	void * NextK( E15_Key ** key, void ** pos);
	void * NextSI(const char ** strkey,unsigned long *ukey, void ** pos);

	void each( int (*f)(E15_Key * key,void * value,void * param) ,void * param);
	void each( int (*f)(const char * skey,unsigned long ukey,void * value,void * param) ,void * param);
};


//-----------------------------------------------------------------------------------

class E15_cxx_object_API E15_Intmap : public E15_Object
{
public:
    E15_Intmap();
    virtual ~E15_Intmap();


	unsigned long HashTableSize() ;
	void Init(unsigned long hashSize);

	int Count() ;
	int IsEmpty() ;

	void * Lookup1(unsigned long ukey1);
	void * Lookup2(unsigned long ukey1,unsigned long ukey2);
	void * Lookup3(unsigned long ukey1,unsigned long ukey2,unsigned long ukey3);

	// add a new (key, value) pair
	void SetAt1(void * newValue,unsigned long ukey1);
	void SetAt2(void * newValue,unsigned long ukey1,unsigned long ukey2);
	void SetAt3(void * newValue,unsigned long ukey1,unsigned long ukey2,unsigned long ukey3);

	void * Remove1(unsigned long ukey1);
	void * Remove2(unsigned long ukey1,unsigned long ukey2);
	void * Remove3(unsigned long ukey1,unsigned long ukey2,unsigned long ukey3);

	void RemoveAll();
	void * Next(unsigned long *ukey1,unsigned long *ukey2,unsigned long *ukey3, void ** pos);

	void each( int (*f)(unsigned long key1,unsigned long key2, unsigned long key3,void * value,void * param) ,void * param);
};



//-----------------------------------

class E15_cxx_object_API E15_Strmap2 : public E15_Object
{
public:
    E15_Strmap2();
    virtual ~E15_Strmap2();

	unsigned long HashTableSize();
	void Init(unsigned long hashSize);

	int Count();
	int IsEmpty() ;
	void CaseFlag(int cf);


	void * LookupI(unsigned long ukey);
	void * LookupS(const char * key);
	void * LookupSI(const char * strkey,unsigned long ukey);

	void SetAtI(void * newValue,unsigned long ukey);
	void SetAtS(void * newValue,const char * strkey);
	void SetAtSI(void * newValue,const char * strkey,unsigned long ukey);


	void * RemoveI(unsigned long ukey);
	void * RemoveS(const char * strkey);
	void * RemoveSI(const char * strkey,unsigned long ukey);

	void RemoveAll();

	void * NextSI(const char ** strkey,unsigned long *ukey, void ** pos);

	void each( int (*f)(const char * skey,unsigned long key,void * value,void * param) ,void * param);
};




#endif


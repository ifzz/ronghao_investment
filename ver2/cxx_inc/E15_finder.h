#ifndef __E15_finder_H
#define __E15_finder_H

#include "E15_object.h"
#include "E15_file.h"

class E15_cxx_object_API E15_Finder : public E15_Object
{
public :
    E15_Finder();
    virtual ~E15_Finder();

	unsigned long Find(const char * filter);
	void SortSize();
	void SortModifyTime();
	void SortCreateTime();
	void SortName();

	E15_File * First();
	E15_File * Tail();
	E15_File * Next();
	E15_File * Prev();

	unsigned long Count();
private:
    E15_File * m_item;
    void *     m_citem;
};



#endif

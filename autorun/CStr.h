/*
	CStr.h - Header for Generic String Class.
	Copyright 1998 by Seumas McNally.
*/

//This was my first real class, with most of the code taken from
//C++ in Plain English, so it's a little ratty in places as I've
//tried to keep updating it with my growing knowledge.  I've got
//a number of convenience functions that mimic Amos' string
//functionality, and a few new really handy file path/extension
//parsing functions.

#ifndef CSTR_H
#define CSTR_H

#ifdef WIN32
#include <windows.h>
#else
#include "Compatible Types.h"
#endif

inline int Range(int a, int b, int c) {return a < b ? b : ( a > c ? c : a );}
inline int Max(int a, int b) {return a < b ? b : a;}
inline int Min(int a, int b) {return a > b ? b : a;}

class CStr{
	char	*pData;
	int		nLength;
	const static char Dummy;
	static char Playground[];
	void InitialInit();
	//For pointing to if mallocs fail.
	//Leave pData NULL when they fail, but return pointer to Dummy when
	//someone asks for a const char *.
public:
	CStr();
	CStr(const char *s);
	CStr(const CStr &str);
	CStr(const char *s, const char *ins, int pos);
	~CStr();

	const char	*get() const {return pData ? pData : &Dummy;}
	int		len() const {return pData ? nLength : 0;}
	int		alloc(int size);	//Allocates size _uninitialized_ bytes not counting ending null, ready to be filled.  Err, how, there's no non-const way to get at pData...  Sigh.
	void	cpy(const char *s);
	void	cat(const char *s);
	int		cmp(const char *s) const;

	char	chr(unsigned int pos) const {
		if(pos < (unsigned int)nLength) return pData[pos];
		return 0;
	};
	char &operator[](int n){
		if((unsigned int)n < (unsigned int)nLength) return pData[n];
		Playground[0] = '\0';
		return Playground[0];	//This will give any writes a safe place to play.
	};

	CStr& operator=(const CStr &source) {
		if(&source != this) cpy(source.get());	//Handle identity copy.
		return *this;
	}
	CStr& operator=(const char *s) {
		if(s != pData) cpy(s);
		return *this;
	}

	operator const char*() const {return get();}
};

CStr operator+(const CStr &str1, const CStr &str2);
CStr operator+(const CStr &str, const char *s);
CStr operator+(const char *s, const CStr &str);

inline int operator==(const CStr &str1, const CStr &str2){
	return str1.cmp(str2.get());
};
inline int operator==(const CStr &str1, const char *s){
	return str1.cmp(s);
};
inline int operator==(const char *s, const CStr &str2){
	return str2.cmp(s);
};
inline int operator!=(const CStr &str1, const CStr &str2){
	return !str1.cmp(str2.get());
};
inline int operator!=(const CStr &str1, const char *s){
	return !str1.cmp(s);
};
inline int operator!=(const char *s, const CStr &str2){
	return !str2.cmp(s);
};

//Seperate CStr manipulation functions.

CStr Mid(const CStr &str, int pos, int num = -1);
inline CStr Left(const CStr &str, int num) {return Mid(str, 0, num);}
inline CStr Right(const CStr &str, int num) {return Mid(str, Max(str.len() - num, 0));}
int Instr(const CStr &str, const CStr &fnd, int pos = 0);
inline CStr Insert(const CStr &str, const CStr &ins, int pos) {CStr nstr(str, ins, pos); return nstr;}
CStr Lower(const char *str);
CStr Upper(const char *str);
CStr String(const double a);
CStr String(const int a);
CStr String(const char c);

CStr FileExtension(const char *n);	//Returns only aspects after period ".".
CStr FileNoExtension(const char *n);	//Returns file path/name with .foo extension removed.
CStr FilePathOnly(const char *n);	//Returns path (with trailing path char) without name.
CStr FileNameOnly(const char *n);	//Returns name without any path.
int FileInPath(const char *n, const char *path);	//Returns true if string n starts with string path.  Only works with fully qualified drive:\dir pathnames.
CStr FileMinusPath(const char *n, const char *path);	//Removes path string from name, or if no match returns FileNameOnly().
CStr FileNameSafe(const char *n);	//Kills path chars and anything not a letter, number, space, or underscore.

int CmpLower(const char *s1, const char *s2);

CStr PadString(const char *str, int padlen, char padchar = ' ', char clip = TRUE);

class CStrList
{
protected:
	CStrList *Prev;
	CStrList *Next;
public:
	CStr *Data;
	CStrList() {Data = NULL; Prev = NULL; Next = NULL;}
	CStrList(CStr *data) {Data = data; Prev = NULL, Next = NULL;}
	~CStrList() {delete Data;}

	CStrList *AddItem(CStr *data = NULL){	//(Important usage info!)
		//Adds item to the end of the list.  If item allocation fails and passed in data
		//object pointer is non-null, "delete" is called on it, allowing for safe usage
		//such as "List->Add(new FooData(bar));".  Do NOT store your "data" items in
		//an external array, the Linklist class will handle ALL deletion of data items!
		//It is OK to create a list with NULL data pointers, and fill them later.
		if(Next) return Next->AddItem(data);
		Next = new CStrList(data);
		if(Next){
			Next->Prev = this; return Next;
		}else{
			if(data) delete data; return 0;
		}
	};
	CStrList *InsertAfter(CStr *data = NULL){
		//Inserts a list item after the current item.
		CStrList *t = new CStrList(data);
		if(t){
			if(Next){
				Next->Prev = t; t->Next = Next;
			}
			Next = t;
			t->Prev = this;
			return t;
		}else{
			if(data) delete data; return 0;
		}
	};
	CStrList *InsertBefore(CStr *data = NULL){
		//Inserts a list item before the current item.
		CStrList *t = new CStrList(data);
		if(t){
			if(Prev){
				Prev->Next = t; t->Prev = Prev;
			}
			Prev = t;
			t->Next = this;
			return t;
		}else{
			if(data) delete data; return 0;
		}
	};
	void DeleteList(){
		CStrList *Ptr, *Ptr2;
		Ptr = Next;
		while(Ptr){
			Ptr2 = Ptr;	//Copy pointer.
			Ptr = Ptr->Next;	//Next item.
			Ptr2->UnlinkItem();	//Unlink current item.
			delete Ptr2;	//Destroy unlinked item.
		}
		Ptr = Prev;
		while(Ptr){
			Ptr2 = Ptr;	//Copy pointer.
			Ptr = Ptr->Prev;	//Next item.
			Ptr2->UnlinkItem();	//Unlink current item.
			delete Ptr2;	//Destroy unlinked item.
		}
		UnlinkItem();
		delete Data;
		Data = NULL;
	};
	void UnlinkItem(){
		if(Prev) Prev->Next = Next;
		if(Next) Next->Prev = Prev;
		Next = Prev = NULL;
	};
	CStrList *NextLink(){ return Next; };
};

#endif
// This file is part of Tread Marks
// 
// Tread Marks is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
// 
// Tread Marks is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
// 
// You should have received a copy of the GNU General Public License
// along with Tread Marks.  If not, see <http://www.gnu.org/licenses/>.

//Binary Tree and Octal Tree templates.
//Ok, and a doubly linked list too, for good measure.
//By Seumas McNally.

#ifndef TREES_H
#define TREES_H

#include <new>

//Returns bool of whether bit is set.
inline int TestBit(int val, int bit){
	return (val & (1 <<bit)) ? 1 : 0;
};
//Returns val with specified bit either set or cleared.
inline int SetBit(int val, int bit, int set = 1){
	return (set ? val | (1 <<bit) : val & (~(1 <<bit)));
};

//Base class template for making inherently linklistable objects.
//USAGE: class MyClass : public LinklistBase<MyClass> { };
//Making the inherited prev and next pointers point to objects of MyClass.
//For the list head, use a single object of class LinklistBase<MyClass>.
//MyClass *MUST* *ALWAYS* derive from LinklistBase<MyClass>!
//
//Ah crud, there's a problem with making the list head a Linklistbase<MyClass>
//object, and that is that if you take the PrevLink() of the first real link,
//it will ostensibly point to a real object of MyClass, but if you try to access
//any MyClass members it will bomb!  Argh!  So the list can't be walked backwards
//without having a pointer to the head object to test links against...
//
template <class ME> class LinklistBase{
protected:
	ME *Prev;
	ME *Next;
public:
	LinklistBase() : Prev(nullptr), Next(nullptr) {};
	virtual ~LinklistBase(){
		//Deletes this item and the entire list, so Local list heads are OK, the auto-
		//destructor will erase the whole list attached to it.
		if(Next){
			Next->Prev = nullptr;	//Unlink first, so we don't get deleted again.
			delete Next;
		}
		if(Prev){
			Prev->Next = nullptr;
			delete Prev;
		}
	};
	void DeleteList(){
		//Deletes the items attached to this item (and attached to those, etc.) but
		//does NOT delete this item, unlike DeleteItem.  Use to clear the attached list
		//of a global or local list head object without touching the head.
		//Now uses a non-recursive algorithm to avoid problems with huge lists.
		//But be sure to DeleteList the head and not just auto-destruct!
		ME *Ptr, *Ptr2;
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
	//	if(Next){
	//		Next->Prev = nullptr;
	//		delete Next;
	//		Next = nullptr;
	//	}
	//	if(Prev){
	//		Prev->Next = nullptr;
	//		delete Prev;
	//		Prev = nullptr;
	//	}
	};
	void UnlinkItem(){
		//Use this function to link and unlink items in various lists without deleting them.
		//If using a linklist child as a sub-object that will be in and out of lists, be
		//sure to call UnlinkItem on the linklist nodes in your class's destructor, and
		//never call DeleteList or delete on any member of the lists!
		if(Prev) Prev->Next = Next;
		if(Next) Next->Prev = Prev;
		Next = Prev = nullptr;
	};
	void DeleteItem(){
		//Deletes this one item and relinks list around it.  Calls destructor on "this".
		//Do NOT DeleteItem the head item in a list, or the rest of the list will be lost!
		UnlinkItem();
		delete this;
	};
	ME *FindItem(int n){	//Returns pointer to nth item in list, 0 being the item whose member you called.
		ME *Ptr;	// Russ - bug fix, was used out of scope
		if(n < 0) return 0;
	//	if(n == 0) return (ME*)this;
	//	if(Next) return Next->FindItem(n - 1);
		for(Ptr = (ME*)this; Ptr && n; Ptr = Ptr->Next) n--;
		return Ptr;
	//	return 0;
	};
	int CountItems(int n = 0){	//Counts items in list including head, going forwards.  To not count the head, pass in -1.
	//	if(Next) return Next->CountItems(n + 1);
	//	return n + 1;
		for(ME *Ptr = (ME*)this; Ptr; Ptr = Ptr->Next) n++;
		return n;
	};
	ME *Head(){	//Finds and returns the head.
	//	if(Prev) return Prev->Head();
	//	return (ME*)this;
		ME *Ptr = (ME*)this;
		while(Ptr->Prev) Ptr = Ptr->Prev;
		return Ptr;
	};
	ME *Tail(){	//Finds tail (end) of list.
	//	if(Next) return Next->Tail();
	//	return (ME*)this;
		ME *Ptr = (ME*)this;
		while(Ptr->Next) Ptr = Ptr->Next;
		return Ptr;
	};
	ME *AddObject(ME *item){
		//USAGE:  foo->AddItem(new MyClass(whatever));
	//	if(Next) return Next->AddObject(item);
		if(Next) return Tail()->AddObject(item);
		//Head and Tail have been optimized to non-recursive
		Next = item;
		if(Next){
			Next->Prev = (ME*)this;
			Next->Next = nullptr;
		}
		return Next;
	};
	ME *InsertObjectAfter(ME *item){
		//Inserts a list item after the current item.
		if(item){
			if(Next){
				Next->Prev = item; item->Next = Next;
			}else{
				item->Next = nullptr;
			}
			Next = item;
			item->Prev = (ME*)this;
		}
		return item;
	};
	ME *InsertObjectBefore(ME *item){
		//Inserts a list item before the current item.
		if(item){
			if(Prev){
				Prev->Next = item; item->Prev = Prev;
			}else{
				item->Prev = nullptr;
			}
			Prev = item;
			item->Next = (ME*)this;
		}
		return item;
	};
	ME *SwapWith(ME *swap){
		ME *tn, *tp;
		if(swap){
			tn = Next;
			tp = Prev;
			Next = swap->Next;
			Prev = swap->Prev;	//Ok, pointers of the two involved items are now swapped.
			swap->Next = tn;
			swap->Prev = tp;
			//Here we need an identity pointer check, for when swapping with an item right next to us.
			if(Next == (ME*)this) Next = swap;
			if(Prev == (ME*)this) Prev = swap;
			if(swap->Next == swap) swap->Next = (ME*)this;	//Might be able to optimize out this pair of checks.
			if(swap->Prev == swap) swap->Prev = (ME*)this;
			//Now reach out from their known-good pointers and redo back-links.
			if(Next) Next->Prev = (ME*)this;
			if(Prev) Prev->Next = (ME*)this;
			if(swap->Next) swap->Next->Prev = swap;
			if(swap->Prev) swap->Prev->Next = swap;
			return (ME*)this;
		}
		return 0;
	};
	ME *ShiftItemUp(){
		return SwapWith(Prev);
	};
	ME *ShiftItemDown(){
		return SwapWith(Next);
	};
	ME *NextLink(){ return Next; };
	ME *PrevLink(){ return Prev; };
};
#if 0
//Simple doubly linked list class template.
//Data is a pointer, so it's up to you to slap a "new"ed data in each new list entry.
//Data will automatically be deleted on deletion of the list item if non-null.
template <class T> class Linklist : public LinklistBase<Linklist> {
friend class LinklistBase<Linklist>;
private:
	Linklist *AddObject(Linklist*);
	Linklist *InsertObjectAfter(Linklist*);	//Overload and hide these members.
	Linklist *InsertObjectBefore(Linklist*);
public:
	T *Data;
	Linklist() : Data(nullptr) { };
	Linklist(T *data) : Data(data) { };
	virtual ~Linklist(){
		if(Data) delete Data;
	};
	Linklist *AddItem(T *data = nullptr){	//(Important usage info!)
		//Adds item to the end of the list.  If item allocation fails and passed in data
		//object pointer is non-null, "delete" is called on it, allowing for safe usage
		//such as "List->Add(new FooData(bar));".  Do NOT store your "data" items in
		//an external array, the Linklist class will handle ALL deletion of data items!
		//It is OK to create a list with nullptr data pointers, and fill them later.
		if(Next) return Next->AddItem(data);
		Next = new Linklist(data);
		if(Next){
			Next->Prev = this; return Next;
		}else{
			if(data) delete data; return 0;
		}
	};
	Linklist *InsertAfter(T *data = nullptr){
		//Inserts a list item after the current item.
		Linklist *t = new Linklist(data);
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
	Linklist *InsertBefore(T *data = nullptr){
		//Inserts a list item before the current item.
		Linklist *t = new Linklist(data);
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
};

#endif

//NOTE:  The template below has never been tested.  It may not work.
//The ones above have been tested extensively though.
//
//Template for binary search/storage tree.  Your Key class must have "<" and ">" comparison
//operators, which when both fail must equate to "==" being true, and your key and data
//classes must have "=" operators defined.  Or use built in types like int, long, char.
//
//Ack, with exceptions "new" will never return nullptr, just chuck a bloody exception.
//Phooey, this could mean reqriting a fair bit of stuff to ick, use exceptions...
//I wonder if the compiler can be configured to simply return nullptr on failed "new"s.
//
/*
template <class K, class D> class Btree{
private:
	Btree *pLess;
		//Le child node whit less key.
	Btree *pMore;
		//Le child node whit more key.	(Also same-as in an inclusive tree.)
	K Key;
		//Le Key.
public:
	D Data;
		//Le Data.
	Btree() : pLess(nullptr), pMore(nullptr) {
	//	pLess = pMore = nullptr;
	};
	Btree(K &key, D &data) : pLess(nullptr), pMore(nullptr), Key(key), Data(data) {
	//	pLess = pMore = nullptr;
	//	Init(key, data);
	};
	~Btree(){
		Delete();
	};
	void Init(K &key, D &data){
		//Initializes key and data of node.  Use on root node if global/static.
		Key = key;
		Data = data;
	};
	bool Add(K &key, D &data, Btree **node){
		//Adds a new node, returns true if added, false if key already present (or out of mem).
		//node will point to either added Btree node or node that already had key.
		//"node" MUST BE A VALID POINTER-POINTER!  For speed there is no checking!
		if(key < Key){
			if(pLess){
				return pLess->Add(key, data, node);
			}else{
				pLess = new Btree<K, D>(key, data);
				if(pLess){ *node = pLess; return true; }
				else{ *node = this; return false; }
			}
		}
		if(key > Key){
			if(pMore){
				return pMore->Add(key, data, node);
			}else{
				pMore = new Btree<K, D>(key, data);
				if(pMore){ *node = pMore; return true; }
				else{ *node = this; return false; }
			}
		}
		*node = this; return false;	//Keys must be identical.
	};
	bool AddInclusive(K &key, D &data, Btree **node){
		//Adds a new node even if the same key is present in the tree.  node points
		//to node just added, if add was successful with a true return code.
		if(key < Key){
			if(pLess){
				return pLess->Add(key, data, node);
			}else{
				pLess = new Btree<K, D>(key, data);
				if(pLess){ *node = pLess; return true; }
				else{ *node = this; return false; }
			}
		}
		//Failed test, must be same or greater, add to pMore link.
		if(pMore){
			return pMore->Add(key, data, node);
		}else{
			pMore = new Btree<K, D>(key, data);
			if(pMore){ *node = pMore; return true; }
			else{ *node = this; return false; }
		}
	};
	bool Delete(){
		//Empties node and deletes descendants.  Returns true if there were descendants.
		bool Ret = false;
		if(pLess){ delete pLess; Ret = true; }
		pLess = nullptr;
		if(pMore){ delete pMore; Ret = true; }
		pMore = nullptr;
		return Ret;
	};
	bool Find(K &key, Btree **nearest){
		//Returns true if exact match found and nearest is match pointer, false if nearest
		//points to last node before giving up search.
		if(key < Key){
			if(pLess){
				return pLess->Find(key, nearest);
			}else{
				*nearest = this; return false;
			}
		}
		if(key > Key){
			if(pMore){
				return pMore->Find(key, nearest);
			}else{
				*nearest = this; return false;
			}
		}
		//We have a weiner!
		*nearest = this; return true;
	};
};
*/

#endif

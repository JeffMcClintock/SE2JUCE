#if !defined(AFX_ITERATOR_H__CE376192_7B18_11D5_AE69_000103170662__INCLUDED_)
#define AFX_ITERATOR_H__CE376192_7B18_11D5_AE69_000103170662__INCLUDED_

#pragma once

#include <assert.h>

class Iterator

{
public:

	virtual void First() = 0;

	virtual void Next() = 0;

	virtual bool IsDone() = 0;

	virtual ~Iterator() {}
};

// generic iterator

template <class T> class GenericIterator : public Iterator

{
public:

	GenericIterator() {}

	virtual ~GenericIterator() {}

	virtual void First() = 0;

	virtual void Next() = 0;

	virtual bool IsDone() = 0;

	virtual T* CurrentItem() = 0;
};

// this iterator contains just one item

template <class T,class DERIVED_FROM> class SingleIterator : public DERIVED_FROM

{
public:

	SingleIterator(T* p_item)
	{
		m_item=p_item;
	};

	virtual ~SingleIterator() {}

	virtual void First()
	{
		done = false;
	};

	virtual void Next()
	{
		done = true;
	};

	virtual bool IsDone()
	{
		return done;
	};

	virtual T* CurrentItem()
	{
		if(!done)
		{
			return m_item;
		}

		return 0;
	};

private:

	T* m_item;

	bool done;
};

// this iterator wraps two iterators into 1

template <class T> class DualIterator : public GenericIterator<T>

{
public:

	DualIterator( GenericIterator<T> *p_it1, GenericIterator<T> *p_it2 ) : it1(p_it1), it2(p_it2), iterating_first(false) {}

	virtual ~DualIterator()
	{
		delete it1;
		delete it2;
	};

	virtual void First()
	{
		iterating_first = true;
		it1->First();

		if( it1->IsDone() )
		{
			iterating_first = false;
			it2->First();
		}
	};

	virtual void Next()
	{
		if( iterating_first )
		{
			it1->Next();

			if( it1->IsDone() )
			{
				iterating_first = false;
				it2->First();
			}
		}
		else
		{
			it2->Next();
		}
	};

	virtual bool IsDone()
	{
		return !iterating_first && it2->IsDone();
	};

	virtual T* CurrentItem()
	{
		if( iterating_first )
		{
			return it1->CurrentItem();
		}

		return it2->CurrentItem();
	};

private:

	GenericIterator<T> *it1;

	GenericIterator<T> *it2;

	bool iterating_first;
};


#endif // !defined(AFX_ITERATOR_H__CE376192_7B18_11D5_AE69_000103170662__INCLUDED_)
#pragma once



template <class T>

class MySmartPtr

{

public:

	explicit MySmartPtr(T* ptr = 0) : pointee(ptr)

	{
		if( pointee != 0 )
		{
			pointee->IncrementRef();
		}
	};

	MySmartPtr(const MySmartPtr& other) // used by STL list push_back()

	{
		pointee = other.pointee;

		if( pointee != 0 )
		{
			pointee->IncrementRef();
		}
	};

	MySmartPtr& operator=(const MySmartPtr& other)

	{
		if( other.pointee != pointee )
		{
			Release();
			pointee = other.pointee;

			if( pointee != 0 )
			{
				pointee->IncrementRef();
			}
		}

		return *this;
	};

	MySmartPtr& operator=(T* other)

	{
		if( other != pointee )
		{
			Release();
			pointee = other;

			if( pointee != 0 )
			{
				pointee->IncrementRef();
			}
		}

		return *this;
	};

	~MySmartPtr()

	{
		Release();
	};

	T& operator *() const
	{
		return *pointee;
	};

	T* operator->() const
	{
		return pointee;
	};



	// allow comparison for NULL

	inline friend bool operator==(const T* lhs, const MySmartPtr& rhs)

	{
		return lhs == rhs.pointee;
	}

	inline friend bool operator==(const MySmartPtr& lhs, const T* rhs )

	{
		return rhs == lhs.pointee;
	}

	inline friend bool operator!=(const T* lhs, const MySmartPtr& rhs)

	{
		return lhs != rhs.pointee;
	}

	inline friend bool operator!=(const MySmartPtr& lhs, const T* rhs )

	{
		return rhs != lhs.pointee;
	}

private:

	void Release()

	{
		if( pointee && pointee->DecrementRef() == 0 )
		{
			delete pointee;
			pointee = 0;
		}
	};

	T* pointee;

};



class ReferenceCounted

{

public:

	ReferenceCounted() : m_reference_count(0) {}

	void IncrementRef()
	{
		m_reference_count++;
	};

	int DecrementRef()
	{
		return --m_reference_count;
	};

	int getReferenceCount()
	{
		return m_reference_count;
	};

private:

	int m_reference_count;

};


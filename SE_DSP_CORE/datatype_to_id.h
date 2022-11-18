#pragma once
#include <map>

#define SE_DECLARE_SERIAL( p_id, p_cls ) template <>int DatatypeToId< p_cls >::DataTypeId(){return p_id;}; bool res##p_id = PersistFactory()->SERegisterClass(p_id, new PersistanceRegister< p_cls, p_id>);
#define SE_DECLARE_SERIAL2( p_id, p_cls, p_template1, p_template2 ) template<>int DatatypeToId< p_cls< p_template1, p_template2 > >::DataTypeId(){return p_id;}; bool res##p_id = PersistFactory()->SERegisterClass(p_id, new PersistanceRegister< p_cls< p_template1, p_template2 >, p_id>);

template <typename T>
class  DatatypeToId
{
public:
	static int DataTypeId(); // must be specialised for each case (see .cpp file)
};


#define PersistFactory() PersistanceFactory::Instance()

class PersistanceRegister_base
{
public:
	virtual ~PersistanceRegister_base(){}
	virtual void* CreateObject() = 0;
};

class PersistanceFactory
{
public:
	~PersistanceFactory();
	static PersistanceFactory* Instance();
	bool SERegisterClass(int p_class_id, PersistanceRegister_base* p_reg);
	void* CreateObject(int p_class_id);
private:
	std::map<int, PersistanceRegister_base*> class_list;
	// not intel11 compat. std::map<int, std::unique_ptr<PersistanceRegister_base> > class_list;
};

template <typename T, int p_class_id>
class PersistanceRegister : public PersistanceRegister_base
{
public:
	PersistanceRegister()
	{
		PersistFactory()->SERegisterClass(p_class_id,this);
	}
	void* CreateObject()
	{
		return new T;
	}
};


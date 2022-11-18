// SeException.cpp: implementation of the SeException class.
//
//////////////////////////////////////////////////////////////////////

#include "pch.h"
#include "SeException.h"


SeException::SeException( int p_problem_code, void* p_hint1, void* p_hint2, std::wstring errorText /*, std::vector<int>* moduleHandles*/ ) :
	problem_code(p_problem_code)
	,hint1(p_hint1)
	,hint2(p_hint2)
	,errorText_(errorText)
{
	//if( moduleHandles )
	//{
	//	moduleHandles_ = *moduleHandles;
	//}

	//		GetApp()->SeMessageBox( L"SeException" ,MB_OK|MB_ICONSTOP );
}

SeException::~SeException()
{
}



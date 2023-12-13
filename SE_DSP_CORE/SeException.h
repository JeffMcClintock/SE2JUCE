// SeException.h: interface for the SeException class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_SEEXCEPTION_H__0BB41BE4_5538_11D5_B783_00104B15CCF0__INCLUDED_)
#define AFX_SEEXCEPTION_H__0BB41BE4_5538_11D5_B783_00104B15CCF0__INCLUDED_

#pragma once
#include <string>
#include <vector>

enum {SE_FEEDBACK_PATH=1, SE_MULTIPLE_NOTESOURCES, SE_FEEDBACK_TO_NOTESOURCE};

class SeException
{
public:

	SeException(int p_problem_code, void* p_hint1 = NULL, void* p_hint2 = NULL, std::wstring errorText = L"") :
		problem_code(p_problem_code)
		, hint1(p_hint1)
		, hint2(p_hint2)
		, errorText_(errorText)
	{
	}

	virtual ~SeException() {}

	int problem_code;
	void* hint1;
	void* hint2;
	std::wstring errorText_;
};


#endif // !defined(AFX_SEEXCEPTION_H__0BB41BE4_5538_11D5_B783_00104B15CCF0__INCLUDED_)

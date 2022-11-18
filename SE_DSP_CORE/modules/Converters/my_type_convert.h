#ifndef MYTYPECONVERT_H_INCLUDED
#define MYTYPECONVERT_H_INCLUDED

#include <string>
#include "../se_sdk3/mp_sdk_common.h"

// Template type-conversion function.
template<typename T1, typename T2>
T2 myTypeConvert( T1 value )
{
	return (T2) value; // generic converion.
}


// specialisations for types that need more complex converting.
template<>
bool myTypeConvert( int value );

template<>
bool myTypeConvert( float value );

template<>
bool myTypeConvert( std::wstring value );

template<>
bool myTypeConvert( std::string value );

template<>
int myTypeConvert( std::wstring value );

template<>
int myTypeConvert( std::string value );

template<>
float myTypeConvert( std::wstring value );

template<>
float myTypeConvert( std::string value );

template<>
std::wstring myTypeConvert( bool value );

template<>
std::string myTypeConvert( bool value );

template<>
std::wstring myTypeConvert( int value );

template<>
std::string myTypeConvert( int value );

template<>
std::wstring myTypeConvert( float value );

template<>
std::string myTypeConvert( float value );

template<>
std::wstring myTypeConvert( std::string value );

template<>
std::string myTypeConvert( std::wstring value );

template<>
std::wstring myTypeConvert( MpBlob value );

template<>
std::string myTypeConvert( MpBlob value );

template<>
MpBlob myTypeConvert( std::wstring value );

std::string UnicodeToAscii(std::wstring); // Actually to UTF8.


#endif



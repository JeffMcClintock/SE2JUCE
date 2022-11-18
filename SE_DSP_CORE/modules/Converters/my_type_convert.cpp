#include "./my_type_convert.h"

#include <locale>
#include <codecvt>

using namespace std;

// specialisations for types that need more complex converting.
template<>
bool myTypeConvert( int value )
{
	return value != 0;
};

template<>
bool myTypeConvert( float value )
{
	return value > 0.0f;
};

template<>
bool myTypeConvert( wstring value )
{
	return wcscmp( value.c_str(), L"1") == 0;
};

template<>
bool myTypeConvert( string value )
{
	return strcmp( value.c_str(), "1") == 0;
};

template<>
int myTypeConvert( wstring value )
{
	return wcstol( value.c_str(), 0 , 10 );
};

template<>
int myTypeConvert( string value )
{
	return strtol( value.c_str(), 0 , 10 );
};

template<>
float myTypeConvert( wstring value )
{
	return (float) wcstod( value.c_str(), 0 );
};

template<>
float myTypeConvert( string value )
{
	return (float) strtod( value.c_str(), 0 );
};

template<>
wstring myTypeConvert( bool value )
{
	if( value )
		return L"1";
	return L"0";
};

template<>
string myTypeConvert( bool value )
{
	if( value )
		return "1";
	return "0";
};

template<>
wstring myTypeConvert( int value )
{
	const int maxSize = 50;
	wchar_t stringval[maxSize];
#if defined(_MSC_VER)
	swprintf_s( stringval, maxSize, L"%d", value );
#else
	swprintf( stringval, maxSize, L"%d", value );
#endif
	return stringval;
};

template<>
string myTypeConvert( int value )
{
	return std::to_string(value);
};

template<>
wstring myTypeConvert( float value )
{
	const int maxSize = 50;
	wchar_t stringval[maxSize];
#if defined(_MSC_VER)
	swprintf_s( stringval, maxSize, L"%f", value );
#else
	swprintf( stringval, maxSize, L"%f", value );
#endif

	// Eliminate Trailing Zeros
	wchar_t *p = 0;
	for (p = stringval; *p; ++p) {
		if (L'.' == *p) {
			while (*++p);
			while (L'0' == *--p) *p = L'\0';
			if (*p == L'.') *p = L'\0';
			break;
		}
	}

	return stringval;
};

template<>
string myTypeConvert( float value )
{
	return std::to_string(value);
};


template<>
std::wstring myTypeConvert(std::string value)
{
	std::wstring_convert<std::codecvt_utf8<wchar_t>> convert;
	return convert.from_bytes(value);
}

template<>
std::string myTypeConvert(std::wstring value)
{
	std::wstring_convert<std::codecvt_utf8<wchar_t>> convert;
	return convert.to_bytes(value);
}

template<>
wstring myTypeConvert( MpBlob value )
{
	int size = value.getSize();

	wchar_t* outputString = new wchar_t[size * 3 + 1];
	outputString[0] = 0; // null terminate incase blob empty.

	unsigned char* blobData = (unsigned char*) value.getData();
	for(int i = 0 ; i < size ; ++i )
	{
		// Use safe printf if available.
		#if defined(_MSC_VER)
			swprintf_s( outputString + i * 3, 4, L"%02X ", blobData[i] );
		#else
			swprintf( outputString + i * 3, 4, L"%02X ", blobData[i] );
		#endif
	}
	
	wstring outputValue;

	outputValue = outputString;

	delete [] outputString;
	return outputValue;
};

template<>
MpBlob myTypeConvert( wstring value )
{
	auto textlength = value.size();

	// estimate max blob size as helf string length.
	unsigned char* blobData = new unsigned char[1 + textlength / 2]; // round up odd numbers.

	int blobSize = 0;

	bool hiByte = true;

	unsigned char binaryValue = 0;

	for(int i = 0 ; i < textlength ; ++i )
	{
		wchar_t c = value[i];
		int nybbleVal = 0;

		if( c != L' ' )
		{
			if( c >= L'0' && c <= L'9' )
			{
				nybbleVal = c - L'0';
			}
			else
			{
				if( c >= L'A' && c <= L'F' )
				{
					nybbleVal = 10 + c - L'A';
				}
				else
				{
					if( c >= L'a' && c <= L'f' )
					{
						nybbleVal = 10 + c - L'a';
					}
				}
			}

			if( hiByte )
			{
				binaryValue = nybbleVal;
			}
			else
			{
				binaryValue = (binaryValue << 4 ) + nybbleVal;

				blobData[blobSize++] = binaryValue;
				binaryValue = 0;
			}

			hiByte = !hiByte;
		}
	}

	// any odd char on end?, include it.
	if( !hiByte )
	{
		blobData[blobSize++] = binaryValue;
	}
	
	MpBlob temp( blobSize, blobData );

	delete [] blobData;

	return temp;
}

string UnicodeToAscii(wstring s) // Actually to UTF8.
{
	if( s.empty() )
	{
		return {};
	}

	std::wstring_convert<std::codecvt_utf8<wchar_t>> convert;
	return convert.to_bytes(s);
}

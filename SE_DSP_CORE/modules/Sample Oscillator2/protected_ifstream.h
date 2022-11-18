#pragma once

#include  <fstream>
#include  <vector>
#include "mp_sdk_common.h"

template <class charT, class traits = std::char_traits<charT> >
class protected_buffer : public std::basic_streambuf<charT,traits>
{
public:
  typedef charT            char_type;
  typedef typename traits::int_type int_type;
  typedef typename traits::off_type off_type;
  typedef typename traits::pos_type pos_type;
  typedef typename traits           traits_type;
  
  protected_buffer(gmpi::IProtectedFile* file) : ifile(file)
  {
     _PBeg = _PCur = 0, _PLength = 0;
     _GBeg = _GCur = _inputbuffer, _GLength = 0;
     _Init(&_GBeg, &_GCur, &_GLength, &_PBeg, &_PCur, &_PLength);

	 ifile->getSize(remaining_bytes);
  }
  
protected:
//    int_type uflow();
	int_type underflow( );

private:
	gmpi::IProtectedFile* ifile;
	char* _PBeg, *_PCur, *_GBeg, *_GCur;
	int _PLength, _GLength;
	int remaining_bytes;
    char _inputbuffer[512];
};

template <class charT, class traits >
typename protected_buffer<charT, traits>::int_type
protected_buffer< charT, traits >::underflow()
{
	// if characters in buffer, return next.
	if(gptr() && gptr() < egptr())
	{
		int c = *gptr();
	    return std::char_traits<char>::not_eof( c );
	}

	// refill buffer.
	int to_get = sizeof(_inputbuffer);
	if( to_get > remaining_bytes )
	{
		to_get = remaining_bytes;
	}

	// nothing left?
	if( to_get <= 0 )
	{
		return std::char_traits<char>::eof();
	}
	
	remaining_bytes -= to_get;

	ifile->read( _inputbuffer, to_get );

	setg( _inputbuffer, _inputbuffer, _inputbuffer + to_get );

    int c = *gptr();
    return std::char_traits<char>::not_eof( c );
}
#ifndef __DspRingBufferWriter_h__
#define __DspRingBufferWriter_h__

#include <assert.h>
#include "WavesPublicAPI/stdint.h"
#include "./my_input_stream.h"

class DspRingBufferWriter : public my_output_stream
{
public:

    DspRingBufferWriter()
        : readOffset_(0)
        ,writeOffset_(0)
        ,buffer_(0)
    {
    }

    // For convenience store the size of the external buffer used by this ringbuffer.
    void Init( int bufferSizeBytes )
    {
        #if defined( _DEBUG )
            int bitCount = 0;
            int temp = bufferSizeBytes;
            while( temp != 0 )
            {
                bitCount += (temp & 0x01);
                temp = temp >> 1;
            }
            assert( bitCount == 1 && "Buffer size (in bytes) must be EXACT power of 2!" );
        #endif
        bufferSize_ = bufferSizeBytes;
    }

    // For convenience store the address of the external buffer, state, and coefs
    // used by this ringbuffer.
    void SetPointers( char* buffer, int32_t readOffset, int32_t& writeOffset )
    {
        buffer_ = buffer;
        readOffset_ = readOffset;
        writeOffset_ = &writeOffset;
    }

    virtual void Write( const void* data, unsigned int size )
   {
        assert( size < bufferSize_ && "Too much data for this buffer size!" );
        assert( size <= FreeSpace() && "Not enough space in buffer!" );

        char* source = (char*) data;
        char* dest = buffer_ + *writeOffset_;
        int todo = size;

        int contiguous = bufferSize_ - *writeOffset_;

        if( todo > contiguous )
        {
            memcpy( dest, source, contiguous );
            todo -= contiguous;
            dest = & (buffer_[0]);
            *writeOffset_ = 0;
            source += contiguous;

            assert( todo < bufferSize_ );
        }

        memcpy( dest, source, todo );

        *writeOffset_ = (*writeOffset_ + todo) & (bufferSize_ - 1); // mask assumes bufferSize_ exact power-of-2.
    };

    int32_t FreeSpace()
    {
        int32_t freespace = readOffset_ - *writeOffset_;
        if( freespace <= 0 )
        {
            freespace += bufferSize_;
        }
        return freespace - 1;
    }
    int32_t TotalSpace()
    {
        return bufferSize_;
    }

private:
    char* buffer_;
    int bufferSize_;
    int32_t readOffset_;
    int32_t* writeOffset_;
};

#endif
#pragma once

#include "./my_input_stream.h"

class DspRingBufferReader :
    public my_input_stream
{
public:

    DspRingBufferReader(char *pBuffer, int32_t pBufferSize, int32_t& pReadOffset, int32_t pWriteOffset ) :
      readOffset_(&pReadOffset)
      ,writeOffset_(pWriteOffset)
      ,bufferSize_(pBufferSize)
      ,buffer_(pBuffer)
      {
      };

    int ReadyBytes()
    {
        // access volatiles only once.
        int readyCount = writeOffset_ - *readOffset_;

        // decide if wrapped and adjust (using local variable, not volatiles).
        if( readyCount < 0 )
        {
            readyCount += bufferSize_;
        }

        return readyCount;
    };

    void Peek( const void* p_data, unsigned int size )
    {
        char *dest = (char *) p_data;
        char *source = buffer_ + *readOffset_;

        int todo = size;
        int contiguous = bufferSize_ - *readOffset_;

        if( todo > contiguous )
        {
            memcpy( dest, source, contiguous );
            todo -= contiguous;

            source = (char*) buffer_;
            dest += contiguous;
        }

        // memcpy, or just a direct loop, usually only small volumes. !!!!
        memcpy( dest, source, todo );
    };

    virtual void Read( const void* p_data, unsigned int size )
    {
        char *dest = (char *) p_data;
        char *source = buffer_ + *readOffset_;

        int todo = size;
        int contiguous = bufferSize_ - *readOffset_;

        if( todo > contiguous )
        {
            memcpy( dest, source, contiguous );
            todo -= contiguous;

            source = (char*) buffer_;
            *readOffset_ = 0;
            dest += contiguous;
        }

        // memcpy, or just a direct loop, usually only small volumes. !!!!
        memcpy( dest, source, todo );

        // _RPT2(_CRT_WARN, "%5d Read %8x\n", *readOffset_, *(int32_t*) source );

        *readOffset_ = (*readOffset_ + todo) & (bufferSize_-1);
    };

    int readyBytes()
    {
        int readyCount = writeOffset_ - *readOffset_;

        // decide if wrapped and adjust.
        if( readyCount < 0 )
        {
            readyCount += bufferSize_;
        }

        return readyCount;
    };

private:
    int32_t* readOffset_;
    int32_t writeOffset_;
    char* buffer_;
    int32_t bufferSize_;
};


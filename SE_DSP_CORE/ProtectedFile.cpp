#include "pch.h"
#include "ProtectedFile.h"

ProtectedFile2* ProtectedFile2::FromUri(const char* fullUri, const char* mode)
{
    const char* defaultMode = "rb";
    
    if( mode == nullptr )
        mode = defaultMode;
    
	FILE* fileObject = fopen(fullUri, mode);
	if( fileObject )
	{
		return new ProtectedFile2(fullUri, fileObject);
	}
	return 0;
}



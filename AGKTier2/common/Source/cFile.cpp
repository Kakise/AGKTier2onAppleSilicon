#include "agk.h"

namespace AGK
{

cFile::cFile()
{
	pFile = UNDEF;
	pFilePtr = UNDEF;
	mode = 0;
}

cFile::~cFile()
{
	Close();
}

bool cFile::IsOpen()
{
	return ( pFile || pFilePtr );
}

}

#include "buffer.h"
#include "utils.h"
#include <string.h>
#include <stdio.h>
#include <string.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

bool
Buffer::allocate(int iS)
{
	if (!pchData)
	{
		iAlloc = iS;
		pchData = new char[iAlloc];
		if (!pchData)
			showMemoryProblem();
	}
	else
	{
		if ((Size() + iS) >= iAlloc)
		{
			int iNew = iAlloc + iS;
			char *pchNew = new char [iNew];
			if (pchNew)
			{
				memcpy(pchNew, getData(), Size());
				iSize -= iSkip;
				iSkip = 0;
				iAlloc = iNew;
				delete [] pchData;
				pchData = pchNew;
			}
			else
				showMemoryProblem();
		}
	}
	return pchData != 0;
}

void
Buffer::Insert(char *p, int len)
{
	if (len <= 0)
		return;

	if (!pchData)
	{
		iAlloc = len;
		pchData = new char[iAlloc];
		if (pchData)
		{
			memcpy(pchData, p, len);
			iSize = len;
		}
		else
			showMemoryProblem();
	}
	else
	{
		if ((Size() + len) > iAlloc)
		{
			int iNew = iAlloc + (len * 2);
			char *pchNew = new char [iNew];
			if (pchNew)
			{
				memcpy(pchNew+len, getData(), Size());
				iSize -= iSkip;
				iSkip = 0;
				memcpy(pchNew, p, len);
				iSize += len;
				iAlloc = iNew;
				delete [] pchData;
				pchData = pchNew;
			}
			else
				showMemoryProblem();
		}
		else
		{
			memmove(pchData+len, pchData, Size());
			memcpy(getData(), p, len);
			iSize += len;
		}
	}
}

char *
Buffer::AddLine(const char *pch, bool bNullTerminate)
{
	char *p = (char *)pch;
	while (p && (*p == '\r' || *p == '\n'))
		p++;
	while (p && *p)
	{
		if (*p == '\r' || *p == '\n')
			break;
		Add(p, 1);
		p++;
	}
	if (bNullTerminate)
		Add("",1);
	return p;
}

void
Buffer::Add(char *p, int len)
{
	if (len <= 0)
		return;

	if (!pchData)
	{
		iAlloc = len;
		pchData = new char[iAlloc];
		if (pchData)
		{
			memcpy(pchData, p, len);
			iSize = len;
		}
		else
			showMemoryProblem();
	}
	else
	{
		if ((Size() + len) > iAlloc)
		{
			int iNew = iAlloc + (len * 2);
			char *pchNew = new char [iNew];
			if (pchNew)
			{
				memcpy(pchNew, getData(), Size());
				iSize -= iSkip;
				iSkip = 0;
				memcpy(pchNew + iSize, p, len);
				iSize += len;
				iAlloc = iNew;
				delete [] pchData;
				pchData = pchNew;
			}
			else
				showMemoryProblem();
		}
		else
		{
			memcpy(getData() + Size(), p, len);
			iSize += len;
		}
	}
}

void
Buffer::Add(const char *p, int len)
{
	if (!pchData)
	{
		iAlloc = len;
		pchData = new char[iAlloc];
		if (pchData)
		{
			memcpy(pchData, p, len);
			iSize = len;
		}
		else
			showMemoryProblem();
	}
	else
	{
		if ((Size() + len) > iAlloc)
		{
			int iNew = iAlloc + (len * 2);
			char *pchNew = new char [iNew];
			if (pchNew)
			{
				memcpy(pchNew, getData(), Size());
				iSize -= iSkip;
				iSkip = 0;
				memcpy(pchNew + iSize, p, len);
				iSize += len;
				iAlloc = iNew;
				delete [] pchData;
				pchData = pchNew;
			}
			else
				showMemoryProblem();
		}
		else
		{
			memcpy(getData() + Size(), p, len);
			iSize += len;
		}
	}
}

void
Buffer::Add(char c)
{
	char ach[2];
	ach[0] = c;
	ach[1] = 0;
	Add(ach, 1);
}

void
Buffer::Add(std::string &str)
{
	Add(str.c_str(), str.length());
}

void
Buffer::AddAt(char *start, char *p, int len)
{
	if (!pchData)
	{
		iAlloc = len;
		pchData = new char[iAlloc];
		if (pchData)
		{
			memcpy(pchData, p, len);
			iSize = len;
		}
		else
			showMemoryProblem();
	}
	else
	{
		int iOff = start - getData();
		if (iOff >= 0)
		{
			if ((iOff + len) > iAlloc)
			{
				int iNew = iAlloc + (len * 2);
				char *pchNew = new char [iNew];
				if (pchNew)
				{
					memcpy(pchNew, getData(), Size());
					iSize -= iSkip;
					iSkip = 0;
					memcpy(pchNew + iOff, p, len);
					iSize = iOff + len;
					iAlloc = iNew;
					delete [] pchData;
					pchData = pchNew;
				}
				else
					showMemoryProblem();
			}
			else
			{
				memcpy(getData() + iOff, p, len);
				iSize = iOff + len;
			}
		}
	}
}

void
Buffer::consume(int iBytes)
{
	if (iBytes == Size())
	{
		iSize = 0;
		iSkip = 0;
	}
	else
	{
		int iData = Size() - iBytes;
		if (iData < 0)
		{
			iData = 0;
		}
		memmove(pchData, getData() + iBytes, iData);
		iSize = iData;
		iSkip = 0;
	}
}

int
Buffer::locate_mem(const char *pchFind, int iLen, int &iOffset)
{
	int 	i;
	int 	iStop	= Size() - iLen;

	for (i=iOffset; i <= iStop; i++)
	{
		if (!memcmp(pchFind, (char *)(pchData+i), iLen))
		{
			return i;
		}
	}
	iOffset = iStop-1;
	return -1;
}

int
Buffer::locate(const char *pchFind, int &iOffset)
{
	int i, j;
	int iLen	= strlen(pchFind);
	int iStop	= Size() - iLen;
	bool found = false;

	if (iLen == 1)
	{
		for (i=iOffset; i <= iStop; i++)
		{
			if (*(pchFind)   == *(getData()+i))
			{
				return i;
			}
		}
		iOffset = iStop-1;
		return -1;
	}

	for (i=iOffset; i <= iStop; i++)
	{
		if (*(pchFind)   == *(getData()+i)   &&
			*(pchFind+1) == *(getData()+i+1))
		{
			found = true;
			for (j = 2; j < iLen; j++)
			{
				if (pchFind[j] != *(getData()+i+j))
				{
					found = false;
					break;
				}
			}
			if (found)
				return i;
		}
	}
	iOffset = iStop-1;
	return -1;
}

int
Buffer::revlocate(const char *pchFind, int iStop, int iOffset)
{
	int i, j;
	int iLen	= strlen(pchFind);
	bool found = false;

	for (i=iOffset; i >= iStop ; i--)
	{
		if (*(pchFind)   == *(getData()+i)   &&
			*(pchFind+1) == *(getData()+i+1))
		{
			found = true;
			for (j = 2; j < iLen; j++)
			{
				if (pchFind[j] != *(getData()+i+j))
				{
					found = false;
					break;
				}
			}
			if (found)
				return i;
		}
	}
	return -1;
}

int
Buffer::locate(const char *pchFind)
{
	int i = 0;
	return locate(pchFind, i);
}

int
Buffer::nlOffset(int iPos_)
{
	if (iPos_ < iSize)
	{
		int off = 1;
		char *p = getData() + iPos_;
		while (p && *p)
		{
			if (*p == '\n')
				return off;
			off++;
			p++;
		}
	}
	return -1;
}

void
MakeTempFileFromBuffer(std::string &strTemp, Buffer *pBuffer)
{
	if (pBuffer)
	{
		char ach[256];
		MakeFullPath("/tmp/iot");
		sprintf(ach, "/tmp/iot/iot_%d.jpg", getpid());
		strTemp = ach;
		if (access(ach, 0))
		{
			mode_t old_mask = umask(0);
			int hf = open(ach, O_RDWR | O_CREAT, 0666);
			if (hf)
			{
				close(hf);
			}
			umask(old_mask);
		}
		FILE *fp = fopen(ach, "wb");
		if (fp)
		{
			int iWrote = 0;
			int iToWrite = pBuffer->Size();
			while (iToWrite)
			{
				int iW = 10240;
				if (iToWrite < 10240)
					iW = iToWrite;
				fwrite(pBuffer->getData() + iWrote, iToWrite, 1, fp);
				iToWrite -= iW;
				iWrote += iW;
			}
			fclose(fp);
		}
	}
}

int
Buffer::readBytes(FILE *fp, int iToRead)
{
	allocate(iToRead);
	int iR = fread(getData()+iSize, 1, iToRead, fp);
	iSize += iR;
	return iR;
}

bool
Buffer::readFile(const char *pchFile)
{
	bool ok = false;
	if (pchFile)
	{
		FILE *fp = fopen(pchFile, "rb");
		if (fp)
		{
			ok = true;
			int iS = fseek(fp, 0, SEEK_END);
			if (iS == 0)
			{
				iS = ftell(fp);
				fseek(fp, 0, SEEK_SET);
			}
			if (iS)
			{
				if (allocate(iS))
				{
					iSkip = 0;
					iSize = fread(getData(), 1, iS, fp);
				}
			}
			fclose(fp);
		}
	}
	return ok;
}

bool
Buffer::writeFile(const char *pchFile, int iLength, bool bConsume)
{
	bool ok = false;

	if (pchFile)
	{
		if (access(pchFile, 0))
		{
			mode_t old_mask = umask(0);
			int hf = open(pchFile, O_RDWR | O_CREAT, 0666);
			if (hf)
			{
				close(hf);
			}
			umask(old_mask);
		}
		FILE *fp = fopen(pchFile, "wb");
		if (fp)
		{
			if ((int)fwrite(getData(), 1, iLength, fp) == iLength)
				ok = true;
			fclose(fp);
			if (bConsume)
				consume(iLength);
		}
	}
	return ok;
}

bool
Buffer::writeFile(const char *pchFile)
{
	return writeFile(pchFile, iSize, false);
}

bool
Buffer::writeFile(const char *pchFile, std::string &strTS)
{
	bool ok = false;

	if (pchFile)
	{
		if (access(pchFile, 0))
		{
			mode_t old_mask = umask(0);
			int hf = open(pchFile, O_RDWR | O_CREAT, 0666);
			if (hf)
			{
				close(hf);
			}
			umask(old_mask);
		}
		FILE *fp = fopen(pchFile, "wb");
		if (fp)
		{
			fwrite(strTS.c_str(), 1, 23, fp);
			if ((int)fwrite(getData(), 1, iSize, fp) == iSize)
				ok = true;
			fclose(fp);
		}
	}
	return ok;
}


char *
Buffer::toCharPtr()
{
	char *pch = new char[iSize+1];
	if (pch)
	{
		memset(pch, 0, iSize+1);
		memcpy(pch, getData(), iSize);
	}
	else
		showMemoryProblem();
	return pch;
}

void
Buffer::toString(std::string &str)
{
	Add("", 1);
	char *pch = toCharPtr();
	str = pch;
	delete [] pch;
}

bool
Buffer::getNextLine(std::string &strLine)
{
	if (iPos > (iSize-1))
		return false;

	int nl = nlOffset(iPos);
	if (nl > 0)
	{
		int len = nl + 5;
		char *pch = new char [len];
		memset(pch, 0, len);

		char *p = pchData + iPos;
		int iP = iPos;
		while (((*p == '\r') || (*p == '\n')) && (iP < iSize))
		{
			p++;
			iP++;
			len--;
		}
		memcpy(pch, p, len-5);

		if (strlen(pch))
			trim_crlf(pch);
		strLine = pch;
		delete [] pch;
		iPos += nl;
		while ((iPos < iSize) && ((pchData[iPos-1] == '\r') || (pchData[iPos-1] == '\n')))
			iPos++;
	}
	else
	{
		if (iPos < iSize)
		{
			int len = iSize - iPos + 1;
			char *pch = new char [len];
			memset(pch, 0, len);
			memcpy(pch, pchData+iPos, len-1);
			iPos = iSize;
		}
	}

	return true;
}

bool
Buffer::peekNextLine(std::string &strLine)
{
	int iSavedPos = iPos;
	bool bVal = getNextLine(strLine);
	iPos = iSavedPos;
	return bVal;
}

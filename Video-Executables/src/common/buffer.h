#ifndef __OI_BUFFER_H
#define __OI_BUFFER_H
#include <string>
#ifdef WIN32
#include <windows.h>
#endif
class Buffer
{
private:
	int	iAlloc;
	int	iSize;
	int	iSkip;
	char	*pchData;
	int	iPos;

public:
	Buffer() :
		iAlloc(0),
		iSize(0),
		iSkip(0),
		pchData(0),
		iPos(0)
	{
	}

	Buffer(const Buffer &other)
		: iAlloc(0)
		, iSize(0)
		, iSkip(0)
		, pchData(0)
		, iPos(0)
	{
		Add(other.getData(), other.Size());
	}

	Buffer(int iAlloc_) :
		iAlloc(0),
		iSize(0),
		iSkip(0),
		pchData(0),
		iPos(0)
	{
		allocate(iAlloc_);
	}

	~Buffer()
	{
		if (pchData)
			delete [] pchData;
	}

	unsigned char	charAt(int i) { return (i < iSize ? (unsigned char)pchData[i] : 0); }
	void	Insert(char *p, int len);
	void	Add(char *p, int len);
	void	Add(const char *p, int len);
	void	AddAt(char *start, char *p, int len);
	void	Add(char c);
	void	Add(std::string &str);
	char	*AddLine(const char *p, bool bNullTerminate = true);
	bool	ContainsBoundary();
	int	Size() const { return iSize - iSkip; }
	void	setSize(int s) { iSize = s; }
	int	getAlloc() { return iAlloc; }
	char	*getData() const { return (pchData + iSkip); }
	void	consume(int iBytes);
	void	skip(int iBytes) { iSkip += iBytes; }
	int	revlocate(const char *pchFind, int iStop, int iOffset);
	int	locate_mem(const char *pchFind, int iLen, int &iOffset);
	int	locate(const char *pchFind, int &iOffset);
	int	locate(const char *pchFind);
	void	reset() { iSize = 0; iSkip = 0; iPos = 0; }
	void	resetPos() { iPos = 0; }
	int	getPos() { return iPos; }
	void	setPos(int i) { iPos = (i >= 0 && i < iSize) ? i : 0; }
	void	nullTerminate() { Add("", 1); }
	int	readBytes(FILE *fp, int iToRead);
	bool	readFile(const char *pchFile);
	bool	writeFile(const char *pchFile);
	bool	writeFile(const char *pchFile, std::string &strTS);
	bool	writeFile(const char *pchFile, int iLength, bool bConsume = false);
	void	truncate(int iT) { if (iT >= 0 && iT < iSize) { iSize = iT; pchData[iT] = 0; }}
	void	resize_down(int iT) { if (iT >= 0 && iT < iSize) { iSize = iT; }}
	void	truncateString(int iT)
	{
		if (iT >= 0 && iT < iSize)
		{
			iSize = iT; pchData[iSize] = 0;
		}
		else
		{
			Add("", 1);
		}
	}
	bool	allocate(int iS);
	int	nlOffset(int iPos);
	char	*toCharPtr();
	void	toString(std::string &str);
	char	lastChar() { return iSize > 0 ? pchData[iSize-1] : 0; }

	//warning dont use these unless you are sure what you are doing
	void	copy_pointer(char *pch, int iLen) { if (pchData) delete [] pchData; pchData = pch; iAlloc = iLen; iSize = iLen; }
	void	release_data() { pchData = 0; iAlloc = 0; iSize = 0; }
	void	steal_data(Buffer &bSrc) { if (pchData) delete [] pchData; pchData = bSrc.pchData; iSize = bSrc.iSize; iAlloc = bSrc.iAlloc; bSrc.pchData = 0; bSrc.iSize = 0; bSrc.iAlloc = 0; }

	bool	getNextLine(std::string &strLine);
	bool	peekNextLine(std::string &strLine);
	bool	endsWith(char c) { return iSize ? pchData[iSize] == c ? true : false : false; }
};

void	MakeTempFileFromBuffer(std::string &strTemp, Buffer *pBuffer);

#endif //__OI_BUFFER_H

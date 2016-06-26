#ifndef __JPEGLIVE_H
#define __JPEGLIVE_H

void	CloseJPEGMappedFile();
bool	writeLiveFileMM_(Buffer *pb, int iCL, std::string &strTimeStamp, char *achImageOK, std::string &strX, bool bSingle, bool bMulti);
bool	writeLiveFile(Buffer *pb, int iCL, std::string &strTimeStamp, char *achImageOK, std::string &strX, bool bSingle, bool bMulti);
bool	ProcessFrames(void *p, int size, const char *pchTimestamp = 0, const char *pchLocation = 0);

#endif

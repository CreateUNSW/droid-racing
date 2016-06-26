#include <sstream>
#include <string>
#include <iostream>
#include <time.h>
#include <stdio.h>
#include <sys/types.h>
#include <fcntl.h>
#include <string.h>
#include <stdint.h>
#include <list>

#include "lock.h"
#include "utils.h"
#include "buffer.h"
#include "status.h"
#include "../capture/webcam.h"

static char achLiveFile[1024];
Buffer *pbx = 0;

std::vector<std::string> svOptions, svVals;
extern char achLiveFile[1024];

extern uint64_t liveCount;
extern uint64_t capCount;
extern float liveRate;
extern float capRate;

int32_t dwStarted = -1;

extern long long llLiveSeq;
extern bool bRecord;
extern std::list<Buffer *> lFrames;
extern CRITICAL_SECTION csRec;
extern CRITICAL_SECTION csLive;

static Buffer b1;

int is_huffman(unsigned char *buf)
{
    unsigned char *ptbuf;
    int i = 0;
    ptbuf = buf;
    while(((ptbuf[0] << 8) | ptbuf[1]) != 0xffda) {
        if(i++ > 2048)
            return 0;
        if(((ptbuf[0] << 8) | ptbuf[1]) == 0xffc4)
            return 1;
        ptbuf++;
    }
    return 0;
}

int memcpy_picture(Buffer *pb, unsigned char *buf, int size)
{
    /*
     * buf: jpeg frame from v4l2
     * out: destination buffer, completed JPEG
     * size: frame size in buf
     */
    unsigned char *ptdeb, *ptlimit, *ptcur = buf;
    int sizein;//, pos = 0;

    if(!is_huffman(buf)) {
        ptdeb = ptcur = buf;
        ptlimit = buf + size-2;
        while((((ptcur[0] << 8) | ptcur[1]) != 0xffc0) && (ptcur < ptlimit))
            ptcur++;
        if(ptcur >= ptlimit)
            return 0;
        sizein = ptcur - ptdeb;

        pb->Add((char *)buf, sizein);
        pb->Add((char *)dht_data, sizeof(dht_data));
        pb->Add((char *)ptcur, size - sizein);
    } else {
        pb->Add((char *)ptcur, size);
    }
    return pb->Size();
}

bool
writeLiveFile(Buffer *pbuffer_, bool bSingle, bool bMulti)
{
	std::string strTS;
	MakeGMTTimeStamp(strTS);
	if (!*achLiveFile)
	{
		dwStarted = GetTickCount();
		strcpy(achLiveFile, "/dev/shm/live.jpg");
		//Buffer b;
		//b.writeFile(achLiveFile);
	}

	EnterCriticalSection(&csLive);
	LockAndWriteFile(achLiveFile, *pbuffer_, strTS);
	LeaveCriticalSection(&csLive);

	liveCount++;
	EnterCriticalSection(&csRec);
	if (bRecord)
	{
		Buffer *pb = new Buffer(pbuffer_->Size());
		pb->Add(pbuffer_->getData(), pbuffer_->Size());
		lFrames.push_back(pb);
	}
	LeaveCriticalSection(&csRec);
	pbuffer_->reset();
	return true;
}

float
getMinLiveRate()
{
	return -1;
}

float
getMinCapRate()
{
	return -1;
}

extern	bool	updateMetaRate(bool bHasMeta);
extern 	void	UpdateRemoteCameraLocation(const char *pchLoc);
extern	float	fMetaRate;
extern	bool	StoppedForSomeReason();
bool	bMulti	= true;

bool ProcessFrames(void *p, int size, const char *pchTimeStamp, const char *pchLocation)
{
	b1.reset();
	b1.Add((char *)p, size);

	//float fLiveRate = 5;
	//float fCapRate  = 5;

	time_t tnow;
	time(&tnow);
	bool bKeep = true;
	bool bLive = true;
	//bool bValid = false;
	//bValid = true; //ValidJpeg(0, (const unsigned char *)p, size, false, 0, 0, 0, &iZoom);

	StringVector sv;

	bool bSaved = false;
	if (bKeep || bLive)
	{
		if (!pbx)
		{
			pbx = new Buffer();
			pbx->allocate(size + sizeof(dht_data) + 1024);
		}

		pbx->reset();
		int new_size = memcpy_picture(pbx, (unsigned char *)p, size);
		if (new_size == 0)
		{
			//could not find picture
			return false;
		}

		/*
		if (bKeep && !bMonitorMode)
		{
			bSaved = true;
			if (prb)
			{
				if (b1.Size())
				{
					if (!prb->AddJpeg(b1, b1.Size(), strTimeStamp.c_str(), pCamera->getDescription(), achImageOK, strX, true, bAlternateIP, wRecSeq))
					{
						Log4_ERROR("Failed to add image to record bank %s", strTimeStamp.c_str());
						bSaved = false;
						pCamera->bumpDroppedFrames_();
					}
				}
				else
				{
					if (!prb->AddJpeg(*pbx, new_size, strTimeStamp.c_str(), pCamera->getDescription(), achImageOK, strX, true, bAlternateIP, wRecSeq))
					{
						Log4_ERROR("Failed to add image to record bank %s", strTimeStamp.c_str());
						bSaved = false;
						pCamera->bumpDroppedFrames_();
					}
				}
			}
		}
		*/
		if (bLive || bKeep)
		{
			if (b1.Size())
			{
				writeLiveFile(&b1, false, false);
			}
			else
			{
				writeLiveFile(pbx, false, false);
			}
		}
/*
		int32_t dwFrameEnd = GetTickCount();
		int iFrameTime = dwFrameEnd - dwFS;

		int iSleepMills = 1;
		int iFudge_ = 0;
		if (bMonitorMode)
		{
			if (iMonitorRate != -1)
			{
				iSleepMills = 1000 / iMonitorRate;
				iSleepMills -= iFrameTime;
				dwNextTick = GetTickCount() + iSleepMills;
			}
			if (bKeep)
			{
				if (fCapRate > 0.0 && fCapRate < 1.0)
				{
					float iSecs = (float)(1.0 / fCapRate);
					tNextFrameTime = tnow + (int)iSecs;
				}
				else if (fCapRate && fCapRate != -1.0)
				{
					int iMills = (int)(1000 / fCapRate);
					iMills -= iFrameTime;
					iMills -= iFudge_;
					if (iMills < 0)
						iMills = 1;
					dwNextTick = GetTickCount() + (int32_t)iMills;
				}
			}
		}
		else
		{
			if (!bPushMotion)
			{
				if (iLiveCount)
					iFudge_ *= 2;

				if (bKeep || bLive)
				{
					if (fLiveRate && fLiveRate != -1)
					{
						if (fLiveRate < 1)
						{
							iSleepMills = (int)(1/fLiveRate) * 1000;
							dwNextLiveTick = GetTickCount() + (int32_t)iSleepMills;
						}
						else
						{
							iSleepMills = (int)(1000 / fLiveRate);
							iSleepMills -= iFrameTime;
							iSleepMills -= iFudge_;
							if (iSleepMills < 0)
								iSleepMills = 1;
							dwNextLiveTick = GetTickCount() + (int32_t)iSleepMills;
						}
					}
				}

				if (bKeep)
				{
					if (fCapRate > 0.0 && fCapRate < 1.0)
					{
						float iSecs = (float)(1.0 / fCapRate);
						tNextFrameTime = tnow + (int)iSecs;
					}
					else if (fCapRate && fCapRate != -1.0)
					{
						int iMills = (int)(1000 / fCapRate);
						iMills -= iFrameTime;
						iMills -= iFudge_;
						if (iMills < 0)
							iMills = 1;
						dwNextTick = GetTickCount() + (int32_t)iMills;
					}
				}
			}
		}
*/
	}

	return bSaved;
}



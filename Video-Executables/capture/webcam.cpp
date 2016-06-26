#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <fcntl.h>

#include <unistd.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/mman.h>
#include <sys/ioctl.h>

#include <linux/videodev2.h>

#include "buffer.h"
#include "webcam.h"
#include "jpeglive.h"
#include "buffer.h"
#include "utils.h"
#include "args.h"

extern Args args;

static char	achSOI[3] = { (char)0xff, (char)0xd8, (char)0x00 };
static char	achEOI[3] = { (char)0xff, (char)0xd9, (char)0x00 };
static Buffer bFrames;

#define CLEAR(x) memset(&(x), 0, sizeof(x))
#ifndef V4L2_PIX_FMT_MJPG
#define V4L2_PIX_FMT_MJPG     v4l2_fourcc('M', 'J', 'P', 'G') /* MJPG with start codes */
#endif

enum io_method {
    IO_METHOD_READ,
    IO_METHOD_MMAP,
    IO_METHOD_USERPTR,
};

struct buffer {
    void   *start;
    size_t length;
};

static enum io_method io = IO_METHOD_MMAP;
struct buffer *buffers;
static unsigned int n_buffers;

static void errno_exit(const char *s)
{
    //Log4_FATAL("%s error %d, %s\n", s, errno, strerror(errno));
    exit(EXIT_FAILURE);
}

static int xioctl(int fh, int request, void *arg)
{
    int r;
    do {
        r = ioctl(fh, request, arg);
    } while (-1 == r && EINTR == errno);
    return r;
}

int ReadFrame(int fd)
{
	struct v4l2_buffer buf;
	CLEAR(buf);
	buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	buf.memory = V4L2_MEMORY_MMAP;

	if (-1 == xioctl(fd, VIDIOC_DQBUF, &buf)) {
		switch (errno) {
			case EAGAIN:
				return 0;
			case EIO:
				// fall through
			default:
				errno_exit("VIDIOC_DQBUF");
		}
	}
	assert(buf.index < n_buffers);

	std::string strTimestamp;
	MakeGMTTimeStamp(strTimestamp);

	bFrames.reset();
	bFrames.Add((char *)buffers[buf.index].start, buf.bytesused);

	int iOff = 0;
	int iStart = bFrames.locate(achSOI, iOff);
	if (iStart != -1)
	{
		int iEnd = bFrames.locate(achEOI, iOff);
		if (iEnd != -1)
		{
			ProcessFrames((void *)(bFrames.getData()+iStart), (iEnd-iStart + 2), strTimestamp.c_str(), 0);
		}
		else
		{
			printf("SOI found but no EOI\n");
		}
	}
	else
	{
		printf("no SOI found\n");
	}

	if (-1 == xioctl(fd, VIDIOC_QBUF, &buf))
	{
		errno_exit("VIDIOC_QBUF");
	}
	return 1;
}

extern bool bTerminated;
void mainloop(int fd)
{
	while (!bTerminated)
	{
		for (;;)
		{
			fd_set fds;
			struct timeval tv;
			int r;
			FD_ZERO(&fds);
			FD_SET(fd, &fds);

			// timeout
			tv.tv_sec = 2;
			tv.tv_usec = 0;
			r = select(fd + 1, &fds, NULL, NULL, &tv);
			if (-1 == r)
			{
				if (EINTR == errno)
					continue;
				errno_exit("select");
			}

			if (0 == r)
			{
				//Log4_ERROR("select timeout");
				//exit(EXIT_FAILURE);
			}

			if (ReadFrame(fd))
			{
				// end of a frame
				break;
			}
			// EAGAIN - continue select loop.
		}
	}
}

void StopCapturing(int fd, bool bExit)
{
	enum v4l2_buf_type type;
	switch (io) {
	case IO_METHOD_READ:
		// Nothing to do.
		break;
	case IO_METHOD_MMAP:
	case IO_METHOD_USERPTR:
		type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		if (-1 == xioctl(fd, VIDIOC_STREAMOFF, &type))
		{
			if (bExit)
				errno_exit("VIDIOC_STREAMOFF");
		}
		break;
	}
}

void StartCapturing(int fd, bool bExit)
{
	int i;
	enum v4l2_buf_type type;
	for (i = 0; i < (int)n_buffers; ++i)
	{
		struct v4l2_buffer buf;

		CLEAR(buf);
		buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		buf.memory = V4L2_MEMORY_MMAP;
		buf.index = i;

		if (-1 == xioctl(fd, VIDIOC_QBUF, &buf))
		{
			if (bExit)
				errno_exit("VIDIOC_QBUF");
		}
	}
	type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	if (-1 == xioctl(fd, VIDIOC_STREAMON, &type))
		if (bExit)
			errno_exit("VIDIOC_STREAMON");
}

void UninitDevice()
{
	unsigned int i;
	for (i = 0; i < n_buffers; ++i)
		if (-1 == munmap(buffers[i].start, buffers[i].length))
			errno_exit("munmap");
	free(buffers);
}

static bool InitMmap(int fd, const char *pchDev, bool bExit)
{
	struct v4l2_requestbuffers req;
	CLEAR(req);
	req.count = 4;
	req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	req.memory = V4L2_MEMORY_MMAP;

	if (-1 == xioctl(fd, VIDIOC_REQBUFS, &req)) {
		if (EINVAL == errno) {
			//Log4_FATAL("%s does not support memory mapping\n", pchDev);
			if (bExit)
				exit(EXIT_FAILURE);
			else
				return false;
		} else {
			if (bExit)
				errno_exit("VIDIOC_REQBUFS");
			else
				return false;
		}
	}

	if (req.count < 2) {
		//Log4_FATAL("Insufficient buffer memory on %s\n", pchDev);
		if (bExit)
			exit(EXIT_FAILURE);
		else
			return false;
	}

	buffers = (struct buffer*)calloc(req.count, sizeof(struct buffer));

	if (!buffers) {
		//Log4_FATAL("Out of memory\n");
		if (bExit)
			exit(EXIT_FAILURE);
		else
			return false;
	}

	for (n_buffers = 0; n_buffers < req.count; ++n_buffers) {
		struct v4l2_buffer buf;
		CLEAR(buf);
		buf.type        = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		buf.memory      = V4L2_MEMORY_MMAP;
		buf.index       = n_buffers;

		if (-1 == xioctl(fd, VIDIOC_QUERYBUF, &buf))
			errno_exit("VIDIOC_QUERYBUF");

		buffers[n_buffers].length = buf.length;
		buffers[n_buffers].start =
			mmap(NULL,                   // start anywhere
					buf.length,
					PROT_READ | PROT_WRITE, // required
					MAP_SHARED,             // recommended
					fd, buf.m.offset);

		if (MAP_FAILED == buffers[n_buffers].start)
			errno_exit("mmap");
	}
	return true;
}

bool
InitOptions()
{
	char achCommand[256];
	sprintf(achCommand, "v4l2-ctl --set-ctrl=horizontal_flip=%s", args.getOption("hf"));
	system(achCommand);
	sprintf(achCommand, "v4l2-ctl --set-ctrl=vertical_flip=%s", args.getOption("vf"));
	system(achCommand);
	sprintf(achCommand, "v4l2-ctl --set-ctrl=video_bitrate=%s", args.getOption("br"));
	system(achCommand);
	return true;
}

bool InitDevice(int fd, int width, int height, float fps, const char *pchDev, bool bExit)
{
	struct v4l2_capability cap;
	struct v4l2_cropcap cropcap;
	struct v4l2_crop crop;
	struct v4l2_format fmt;
	struct v4l2_streamparm streamparm;
	struct v4l2_fract *tpf;
	unsigned int min;

	if (-1 == xioctl(fd, VIDIOC_QUERYCAP, &cap)) {
		if (EINVAL == errno) {
			//Log4_FATAL("%s is no V4L2 device\n", pchDev);
			exit(EXIT_FAILURE);
		} else {
			errno_exit("VIDIOC_QUERYCAP");
		}
	}

	if (!(cap.capabilities & V4L2_CAP_VIDEO_CAPTURE)) {
		//Log4_FATAL("%s is no video capture device\n", pchDev);
		exit(EXIT_FAILURE);
	}

	if (!(cap.capabilities & V4L2_CAP_STREAMING)) {

		//Log4_FATAL("%s does not support streaming i/o\n", pchDev);
		exit(EXIT_FAILURE);
	}

	// Select video input, video standard and tune here.
	CLEAR(cropcap);
	cropcap.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	if (0 == xioctl(fd, VIDIOC_CROPCAP, &cropcap)) {
		crop.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		crop.c = cropcap.defrect; // reset to default

		if (-1 == xioctl(fd, VIDIOC_S_CROP, &crop)) {
			switch (errno) {
				case EINVAL:
					// Cropping not supported.
					break;
				default:
					// Errors ignored
					break;
			}
		}
	}

	CLEAR(fmt);
	fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	fmt.fmt.pix.width       = width;
	fmt.fmt.pix.height      = height;
	fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_MJPG;
	//
	// For JPEGs
	// Set fmt.fmt.pix.pixelformat to V4L2_PIX_FMT_MJPEG;
	// and replace ProcessFrames() with saving JPEG frames function
	//
	fmt.fmt.pix.field       = V4L2_FIELD_ANY;

	if (-1 == xioctl(fd, VIDIOC_S_FMT, &fmt))
	{
		errno_exit("VIDIOC_S_FMT");
	}

	if (fmt.fmt.pix.pixelformat != V4L2_PIX_FMT_MJPEG)
	{
		//printf("pixelformat not jpg instead : %d\n", fmt.fmt.pix.pixelformat);
	}

	if ((int)fmt.fmt.pix.width  != width ||
			(int)fmt.fmt.pix.height != height ) {
		// Actual resolution
		//printf("Actual width: %d\n", fmt.fmt.pix.width);
		//printf("Actual height: %d\n", fmt.fmt.pix.height);
	}

	if (fps != -1.0)
	{
		CLEAR(streamparm);
		streamparm.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		tpf = &streamparm.parm.capture.timeperframe;
		tpf->numerator = 1;
		tpf->denominator = fps;
		if (ioctl(fd, VIDIOC_S_PARM, &streamparm) < 0) {
			errno_exit("VIDIOC_S_PARM");
		}

		if (tpf->denominator != fps || tpf->numerator != 1) {
			// Actual FPS
			//printf("actual fps: %d/%d\n", tpf->numerator, tpf->denominator);
		}
	}

	// Buggy driver paranoia.
	min = fmt.fmt.pix.width * 2;
	if (fmt.fmt.pix.bytesperline < min)
		fmt.fmt.pix.bytesperline = min;
	min = fmt.fmt.pix.bytesperline * fmt.fmt.pix.height;
	if (fmt.fmt.pix.sizeimage < min)
		fmt.fmt.pix.sizeimage = min;

	InitMmap(fd, pchDev, bExit);
	InitOptions();
	return true;
}

void CloseDevice(int fd, bool bExit)
{
	if (-1 == close(fd))
	{
		if (bExit)
			errno_exit("close");
	}
	fd = -1;
}

void
DelayExit(int e)
{
	//Sleep(1000);
	printf("Exiting due to error\n");
	exit(e);
}

int OpenDevice(const char *pchDev, bool bExit)
{
	struct stat st;

	if (-1 == stat(pchDev, &st)) {
		//Log4_ERROR("Cannot identify '%s': %d, %s\n", pchDev, errno, strerror(errno));
		if (bExit)
			DelayExit(EXIT_FAILURE);
		else
			return -1;
	}

	if (!S_ISCHR(st.st_mode)) {
		//Log4_ERROR("%s is no device\n", pchDev);
		if (bExit)
			DelayExit(EXIT_FAILURE);
		else
			return -1;
	}

	int fd = open(pchDev, O_RDWR /* required */ | O_NONBLOCK, 0);

	if (-1 == fd) {
		//Log4_ERROR("Cannot open '%s': %d, %s\n", pchDev, errno, strerror(errno));
		if (bExit)
			DelayExit(EXIT_FAILURE);
		else
			return -1;
	}
	return fd;
}

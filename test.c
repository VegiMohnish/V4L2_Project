#include <errno.h>
#include <fcntl.h>
#include <linux/videodev2.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <unistd.h>
#include <sys/select.h>

uint8_t *buffer;

// creating this function to set the output image format 
int set_format(int fd) {	// parameter fd is file descriptor of v4l2 device
	// structure to store output image properties
    	struct v4l2_format format = {0};
    	
    	format.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    	format.fmt.pix.width = 720;
    	format.fmt.pix.height = 720;
    	format.fmt.pix.pixelformat = V4L2_PIX_FMT_YUYV;
    	format.fmt.pix.field = V4L2_FIELD_INTERLACED;
    	// this VIDIOC_S_FMT ioctl sets the format parameters 
    	int res = ioctl(fd, VIDIOC_S_FMT, &format);
    	if(res == -1) {
        	perror("Could not set format");
        	return 1;
    	}
    	return res;
}

int init_mmap(int fd) {
	/* allocating memory */
	// structure to store buffer request parameters
	struct v4l2_requestbuffers req = {0};
	req.count = 1;
	req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	req.memory = V4L2_MEMORY_MMAP;
	// VIDIOC_REQBUFS ioctl is used here to allocate memory for the frame buffers
    	if (-1 == ioctl(fd, VIDIOC_REQBUFS, &req))
    	{
        	perror("Requesting Buffer");
        	return 1;
    	}
    	/* getting physical address */
    	// structure to query the physical address of allocated buffer
    	struct v4l2_buffer buf = {0};
    	buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    	buf.memory = V4L2_MEMORY_MMAP;
    	buf.index = 0;		
    	// VIDIOC_QUERYBUF ioctl is used here to query the buffer information (buffer size and buffer physical address)
    	int res = ioctl(fd, VIDIOC_QUERYBUF, &buf);
    	if(res == -1) {
        	perror("Could not query buffer");
        	return 1;
    	}
    	// buf.m.offset will contain the physical address returned from driver
    	
    	/* mapping kernel space address to user space */
    	buffer = mmap (NULL, buf.length, PROT_READ | PROT_WRITE, MAP_SHARED, fd, buf.m.offset);
    	// now buffer holds the user space address
    	printf("Length: %d\nAddress: %p\n", buf.length, buffer);
    	printf("Image Length: %d\n", buf.bytesused);
    	return 0;
}

int capture_image(int fd)
{
	struct v4l2_buffer buf = {0};
    	buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    	buf.memory = V4L2_MEMORY_MMAP;
    	buf.index = 0;
    	// VIDIOC_QBUF ioctl en-queues the buffer in the buffer queue, VIDIOC_QBUF is necessary for streaming IO
    	if(-1 == ioctl(fd, VIDIOC_QBUF, &buf))
    	{
        	perror("Query Buffer");
        	return 1;
    	}
	// VIDIOC_STREAMON ioctl starts capturing images from v4l2 device
    	if(-1 == ioctl(fd, VIDIOC_STREAMON, &buf.type))
    	{
        	perror("Start Capture");
        	return 1;
    	}

	
    	fd_set fds;		// set of file descriptors used for the select function
    	FD_ZERO(&fds);		// here FD_ZERO clears set 
    	FD_SET(fd, &fds);	// adds v4l2 file descriptor to the above set
    	struct timeval tv = {0};
    	tv.tv_sec = 2;	// amount of time the select should block for a fd to become ready
    	// here select system call is used to inform when the v4l2 device becomes ready for a read operation i.e we need to wait till the v4l2 device writes to the image so that we can read from the buffer
    	int r = select(fd+1, &fds, NULL, NULL, &tv);
    	// 1st parameter refers to the range of file descriptors select method() checks
    	
    	if(-1 == r)
    	{
        	perror("Waiting for Frame");
        	return 1;
    	}
	// VIDIOC_DQBUF ioctl de-queues the buffer in the buffer queue, VIDIOC_DQBUF de-queues the captured buffer from the buffer queue of the driver
    	if(-1 == ioctl(fd, VIDIOC_DQBUF, &buf))
    	{
        	perror("Retrieving Frame");
        	return 1;
    	}

    	int outfd = open("out.yuv", O_RDWR);
    	write(outfd, buffer, buf.bytesused);
    	close(outfd);

    return 0;
}

int main()
{
	int fd;

        fd = open("/dev/video2", O_RDWR);
        if (fd == -1)
        {
                perror("Opening video device");
                return 1;
        }

        if(set_format(fd))
            return 1;

        if(init_mmap(fd))
            return 1;

        if(capture_image(fd))
            return 1;

        close(fd);
        return 0;
}

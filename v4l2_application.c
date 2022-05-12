// v4l2 user application
#include <sys/mman.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <string.h>
#include <errno.h>

#define GET_FRAME_BUFFER _IOWR('a', 2, int)

#define MAX_WIDTH		720
#define MAX_HEIGHT		720
#define MAX_FPS		120
#define DEFAULT_FPS		30
#define MIN_FCOUNT		80
#define DEFAULT_FCOUNT		120
#define MAX_FCOUNT		240
#define MAX_SIZE		MAX_WIDTH*MAX_HEIGHT*2

struct yuv_data {
	int framerate;
	int framecount;
	int width;
	int height;
} data;

int str_to_int(char *p)
{
	int i, ret = 0;

	for (i = 0; p[i]; i++) {
		if ((p[i] > '9') || (p[i] < '0')) {
			printf("invalid argument..\n");
			return 0;
		}
		ret = ret * 10 + p[i] - '0';
	}
	return ret;
}

int main(int argc, char *argv[])
{
	int v4l2_fd;
	int i_fd[10];
	int i,j,k,f;
	int err[10];
	int m,l,count=0;
	int frame_index[120];
	int yuv_height[10];
	int yuv_width[10];
	int size, fract;
	unsigned int yuv_width_size, width_size;
	unsigned char *buf1, *buf2, *buf[10];
	
	struct stat statbuf;
	/*
	
	struct image_buffer {
		int *buffer[10];
	};
	*/
	char yuv_files[10][20] = {"YUV/1.yuv","YUV/2.yuv","YUV/3.yuv","YUV/4.yuv","YUV/5.yuv","YUV/6.yuv","YUV/7.yuv","YUV/8.yuv","YUV/9.yuv","YUV/10.yuv"};
	for(i=0;i<10;i++) {
		yuv_height[i] = 480;
		yuv_width[i] = 640;
	}
	v4l2_fd = open("/dev/frame_feed",O_RDWR);
	if (v4l2_fd < 0) {
		fprintf(stdout,"Cannot open device file...\n");
		fflush(stdout);
		return -1;
	}
	fprintf(stdout,"Device file opened sucessfully...\n");
	fflush(stdout);
	
	data.framerate = str_to_int(argv[1]);
	data.framecount = str_to_int(argv[2]);
	data.height = str_to_int(argv[3]);
	data.width = data.height;
	
	yuv_width_size = MAX_WIDTH << 1;
	width_size = data.width << 1;
	buf1 = malloc(yuv_width_size);
	buf2 = malloc(width_size);
	for(i=0;i<10;i++) {
		//ioctl call to send frame details
		ioctl(v4l2_fd, 0, &data);
		
		fract = MAX_WIDTH / data.width;
		size = (data.height*data.width) << 1;
		fprintf(stdout,"size is %d\n",size);
		fflush(stdout);
		fprintf(stdout,"%s: frame rate = %d, frame count = %d\n", __func__, data.framerate, data.framecount);
		fflush(stdout);
		fprintf(stdout,"%s: frame width = %d, frame height = %d\n", __func__, data.width, data.height);
		fflush(stdout);
	
		i_fd[i] = open(yuv_files[i],O_RDONLY);
		if (i_fd[i] < 0) {
			fprintf(stdout,"error opening the file %d...\n",i);
			fflush(stdout);
		}
		fprintf(stdout,"YUV file %d opened sucessfully...\n",i);
		fflush(stdout);
		err[i] = fstat(i_fd[i], &statbuf);
		if (err[i] < 0) {
			fprintf(stdout,"could not open\n");
			fflush(stdout);
		}

		buf[i] = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, v4l2_fd, 0);
		//buf[i] = mmap(NULL, statbuf.st_size, PROT_NONE, MAP_SHARED, v4l2_fd, 0);
		if (buf[i] == MAP_FAILED) {
			fprintf(stdout,"mmap failed \n");
			fflush(stdout);	
		}
		else {
			fprintf(stdout,"mmap success \n");
			fflush(stdout);
		}
		//printf("checkpoint 1\n");
		//buf1 = malloc(yuv_width_size);
		//buf2 = malloc(width_size);
		m=0;
		//printf("checkpoint 2\n");
		for(f=0;f<data.framecount;f++) {
			l = 0;
			for (j = 0; j < MAX_HEIGHT; j++) {
				//printf("checkpoint 3\n");
				unsigned char *p = buf2;
				read(i_fd[i], buf1, yuv_width_size);
				//printf("checkpoint 4\n");
				if (j % fract)
					continue;
				//printf("checkpoint 5\n");
				for (k = 0; k < yuv_width_size; k += (fract << 2)) {
					memcpy(p, buf1 + k, 4);
					p += 4;
					count++;
					//printf("checkpoint 6 and count = %d\n",count);
				}
				//printf("checkpoint 7\n");
				memcpy(buf[i] + (l*width_size), buf2, width_size);
				//printf("checkpoint 8\n");
				if(l == data.height)
					break;
				l++;
				//printf("checkpoint 9\n");
			}
		fprintf(stdout,"Data write completed for frame%d\n",(m+1));
		fflush(stdout);
		frame_index[m] = m+1;
		fprintf(stdout,"frame index is %d\n",frame_index[m]);
		fflush(stdout);
		ioctl(v4l2_fd, GET_FRAME_BUFFER,(int) frame_index[m]);
		usleep(1000000/data.framerate);
		m++;
		/*
		if(m == data.framecount) {
			m = 0;
			printf("for yuv file %d\n",i);
			//lseek(i_fd[i], 0, SEEK_SET);
		}
		*/
		}
		munmap(buf[i],size);
		//free(buf1);
		//free(buf2);
		close(i_fd[i]);
		printf("Data write is completed for YUV file %d\n",i+1);

	}	
	free(buf1);
	free(buf2);
	
	close(v4l2_fd);
	
	
	
	return 0;
}

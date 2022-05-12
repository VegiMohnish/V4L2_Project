

#ifndef V4L2_DRIVER_H
#define V4L2_DRIVER_H

struct yuv_frame {
	int frame_no;
	u8 *data;
	struct yuv_frame *next;
};

struct yuv_data {
	int framerate;
	int framecount;
	int width;
	int height;
}F_DATA;

extern void* get_frame(void);
extern bool get_flag(void);
extern void set_flag(bool);
extern struct yuv_data get_frame_data(void);

#endif

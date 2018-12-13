#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <stdint.h>
#include <stdio.h>
#include <linux/input.h>
#include <linux/uinput.h>
#include <signal.h> // signal functions
#include <linux/fb.h>

#define X_MAX  0x0000f3e
#define X_MIN  0x0000081
#define Y_MAX  0x0000eff
#define Y_MIN  0x00000eb

float a[7] = {0.0,0.0,0.0,0.0,0.0,0.0,0.0};

struct ts_status
{
    unsigned long y:16;
    unsigned long x:15;
    unsigned long down:1;
};

union un_status
{
    unsigned long val;
    struct ts_status ts;
};

volatile sig_atomic_t flag = 0;
static void stop_handler(int sig){ // can be called asynchronously
  flag = 1; // set flag
}

void raw2p(long dx,long dy,long *px,long *py)
{

	*px=(long) ((a[2]+(a[0]*dx)+(a[1]*dy))/a[6]);
    *py=(long) ((a[5]+(a[3]*dx)+(a[4]*dy))/a[6]);
}

void mouse_key(int fd ,long val)
{
	struct input_event ev;
	memset(&ev, 0, sizeof(struct input_event));
	gettimeofday(&ev.time,0);
	ev.type = EV_KEY;
	ev.code = BTN_LEFT;
	ev.value = val;
	if (write(fd, &ev, sizeof(struct input_event)) < 0) {
		printf("key error\n");
	}

	memset(&ev, 0, sizeof(struct input_event));
	gettimeofday(&ev.time,0);
	ev.type = EV_SYN;
	ev.code = SYN_REPORT;
	ev.value = 0;
	if (write(fd, &ev, sizeof(struct input_event)) < 0) {
		printf("move error\n");
	}
}

void mouse_move(int fd, long dx, long dy)
{
	struct input_event ev;

	memset(&ev, 0, sizeof(struct input_event));
	gettimeofday(&ev.time,0);
	ev.type = EV_ABS;
	ev.code = ABS_X;
	ev.value = dx;
	if (write(fd, &ev, sizeof(struct input_event)) < 0) {
		printf("move error\n");
	}
 
	memset(&ev, 0, sizeof(struct input_event));
	gettimeofday(&ev.time,0);
	ev.type = EV_ABS;
	ev.code = ABS_Y;
	ev.value = dy;
	if (write(fd, &ev, sizeof(struct input_event)) < 0) {
		printf("move error\n");
	}
	
	memset(&ev, 0, sizeof(struct input_event));
	gettimeofday(&ev.time,0);
	ev.type = EV_SYN;
	ev.code = SYN_REPORT;
	ev.value = 0;
	if (write(fd, &ev, sizeof(struct input_event)) < 0) {
		printf("move error\n");
	}
}
 
void mouse_report_key(int fd, uint16_t type, uint16_t keycode, int32_t value)
{
	struct input_event ev;
 
	memset(&ev, 0, sizeof(struct input_event));
 
	ev.type = type;
	ev.code = keycode;
	ev.value = value;
 
	if (write(fd, &ev, sizeof(struct input_event)) < 0) {
		printf("key report error\n");
	}
}
 
int main(void)
{
	struct uinput_user_dev mouse;
	int fd,touch,ret,fb;
	long dx, dy,lx,ly;
    unsigned int status;
    union un_status s_stat;
	FILE *fp;

	fp = fopen("/etc/pointercal","r");
	if(fp == NULL)
	{
		printf("open file err!\n");
		return;
	}
	fscanf(fp,"%f%f%f%f%f%f%f",&a[0],&a[1],&a[2],&a[3],&a[4],&a[5],&a[6]);
	printf("%f %f %f %f\n %f %f %f\n",a[0],a[1],a[2],a[3],a[4],a[5],a[6]);

    signal(SIGINT, stop_handler); 

    touch = open("/dev/touchscreen-1wire",O_RDONLY | O_NONBLOCK);
    if(touch < 0){
        printf("open touchscreen err ! \n");
		return fd;
    }

	fd = open("/dev/uinput", O_WRONLY | O_NONBLOCK);
	if (fd < 0) {
        printf("open uinput err ! \n");
		return fd;
	}
 
	ioctl(fd, UI_SET_EVBIT, EV_SYN);
	ioctl(fd, UI_SET_EVBIT, EV_KEY);
	ioctl(fd, UI_SET_KEYBIT, BTN_LEFT);
	ioctl(fd, UI_SET_KEYBIT, BTN_RIGHT);
	ioctl(fd, UI_SET_KEYBIT, BTN_MIDDLE);
 
    /*ioctl(fd, UI_SET_EVBIT, EV_REL);
	ioctl(fd, UI_SET_RELBIT, REL_X);
	ioctl(fd, UI_SET_RELBIT, REL_Y);*/

	ioctl(fd, UI_SET_EVBIT, EV_ABS);
    ioctl(fd, UI_SET_ABSBIT, ABS_X);
	ioctl(fd, UI_SET_ABSBIT, ABS_Y);
    ioctl(fd, UI_SET_ABSBIT, ABS_PRESSURE);
	

	memset(&mouse, 0, sizeof(struct uinput_user_dev));
	snprintf(mouse.name, UINPUT_MAX_NAME_SIZE, "touch_mouse zjj");
	mouse.id.bustype = BUS_USB;
	mouse.id.vendor = 0x1234;
	mouse.id.product = 0xfedc;
	mouse.id.version = 4;
    mouse.absmin[ABS_X] = 0;
    mouse.absmax[ABS_X] = 800-1; //sam 把屏幕设为0-65535
    mouse.absmin[ABS_Y] = 0;
    mouse.absmax[ABS_Y] = 480-1;
	mouse.absfuzz[ABS_X] = 0;
	mouse.absflat[ABS_X] = 0;
	mouse.absfuzz[ABS_Y] = 0;
	mouse.absflat[ABS_Y] = 0;
    mouse.absmin[ABS_PRESSURE] = 0;
    mouse.absmax[ABS_PRESSURE] = 0xfff;
	ret = write(fd, &mouse, sizeof(struct uinput_user_dev));
 
	ret = ioctl(fd, UI_DEV_CREATE);
	if (ret < 0) {
		close(fd);
		return ret;
	}
    dx = dy = 0;
	lx = 400;
	ly = 240;
	s_stat.val = 0;
 	while (1) {
		const int cnt = 4; 
		long tmp_x = 0,tmp_y = 0,i;

		read(touch,&s_stat.val,sizeof(long));
        
        if(s_stat.ts.down == 1){
			for(i=0;i<cnt;i++)
			{
				read(touch,&s_stat.val,sizeof(long));
				tmp_x += s_stat.ts.x;
				tmp_y += s_stat.ts.y;
			}
			tmp_x /= 4;
			tmp_y /= 4;
			raw2p(tmp_x,tmp_y,&dx,&dy);
		    printf("x:%04x y:%04x\n",dx,dy);
			mouse_move(fd,dx,dy);
			mouse_key(fd,1);
			lx = dx;
			ly = dy;
        }
		else
		{
			mouse_key(fd,0);
		}
        if(flag == 1){
            break;
        }
		s_stat.val = 0;
		usleep(10000);
	}
 
	ioctl(fd, UI_DEV_DESTROY);
 
	close(fd);
 
	close(touch);
	return 0;
}
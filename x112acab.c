#include <X11/Xos.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/cursorfont.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <X11/Xos.h>
#include <stdlib.h>
#include <stdint.h>
#include <signal.h>

#include <X11/Xlib.h>
#include <X11/Xutil.h>

#define LOG(fmt, args...) {fprintf(logfp, fmt, ##args); fflush(logfp);}

#define PUERTO_X 24
#define PUERTO_Y 4

#define BYPP     3

uint8_t * frame_out;
uint8_t * frame_out_last;

int daemon_pid = 0;
//#define PID_FILE "/var/run/x112acab.pid"
#define PID_FILE "/home/matze/puerto/x112acab/x112acab.pid"

int logfd = -1;
FILE * logfp;
//#define LOG_FILE "/var/log/x112acab.log"
#define LOG_FILE  "/home/matze/puerto/x112acab/x112acab.log"

int fifofd = -1;
#define FIFO_FILE "/tmp/x112acab.fifo"

#define DATA_DIR "/home/matze/puerto/x112acab"

#define BUFSZ 4096
char buf[BUFSZ];

int is_recording = 0;


Display *dpy = NULL;
int      screen = 0;
static int format = ZPixmap;
static Bool use_installed = False;

typedef unsigned long Pixel;

/* Close_Display: Close display
 */
void Close_Display(void)
{
	if (dpy == NULL)
		return;

	XCloseDisplay(dpy);
	dpy = NULL;
}

/* Standard fatal error routine - call like printf but maximum of 7 arguments.
 * Does not require dpy or screen defined.
 */
void Fatal_Error(char *msg, ...)
{
	va_list args;
	LOG("ERROR: ");
	va_start(args, msg);
	LOG(msg, args);
	va_end(args);
	LOG("\n");

	Close_Display();
	exit(EXIT_FAILURE);
}

/* Setup_Display_And_Screen: This routine opens up display 0
 *                           and then stores a
 *                           pointer to it in dpy.  The default screen
 *                           for this display is then stored in screen.
 *                           Does not require dpy or screen defined.
 */
void Setup_Display_And_Screen(void)
{
	dpy = XOpenDisplay (NULL);
	if (!dpy)
		Fatal_Error("cannot XOpen_Display()\n");
	screen = XDefaultScreen(dpy);
}


/* Determine the pixmap size.
 */

int Image_Size(XImage *image)
{
	if (image->format != ZPixmap)
		return(image->bytes_per_line * image->height * image->depth);

	return(image->bytes_per_line * image->height);
}

#define lowbit(x) ((x) & (~(x) + 1))

static int ReadColors(Visual *vis, Colormap cmap, XColor **colors)
{
	int i,ncolors ;

	ncolors = vis->map_entries;

	if (!(*colors = (XColor *) malloc (sizeof(XColor) * ncolors)))
		Fatal_Error("Out of memory!");

	if (vis->class == DirectColor ||
			vis->class == TrueColor) {
		Pixel red, green, blue, red1, green1, blue1;

		red = green = blue = 0;
		red1 = lowbit(vis->red_mask);
		green1 = lowbit(vis->green_mask);
		blue1 = lowbit(vis->blue_mask);
		for (i=0; i<ncolors; i++) {
			(*colors)[i].pixel = red|green|blue;
			(*colors)[i].pad = 0;
			red += red1;
			if (red > vis->red_mask)
				red = 0;
			green += green1;
			if (green > vis->green_mask)
				green = 0;
			blue += blue1;
			if (blue > vis->blue_mask)
				blue = 0;
		}
	} else {
		for (i=0; i<ncolors; i++) {
			(*colors)[i].pixel = i;
			(*colors)[i].pad = 0;
		}
	}

	XQueryColors(dpy, cmap, *colors, ncolors);

	return(ncolors);
}


/* Get the XColors of all pixels in image - returns # of colors
 */
int Get_XColors(XWindowAttributes *win_info, XColor **colors)
{
	int i, ncolors;
	Colormap cmap = win_info->colormap;

	if (use_installed)
		/* assume the visual will be OK ... */
		cmap = XListInstalledColormaps(dpy, win_info->root, &i)[0];
	if (!cmap)
		return(0);
	ncolors = ReadColors(win_info->visual,cmap,colors) ;
	return ncolors ;
}


void scale_down_image(uint8_t * data, unsigned buffer_size, unsigned width, unsigned height, int * bytes_in_out_image)
{
	int xpix = width  / PUERTO_X;
	int ypix = height / PUERTO_Y;

	int px;
	int py;

	int ix;
	int iy;

	uint64_t * sum_r = malloc(PUERTO_X * PUERTO_Y * 8);
	uint64_t * sum_g = malloc(PUERTO_X * PUERTO_Y * 8);
	uint64_t * sum_b = malloc(PUERTO_X * PUERTO_Y * 8);

	uint64_t pixel_count;

	uint8_t * r = malloc(PUERTO_X * PUERTO_Y);
	uint8_t * g = malloc(PUERTO_X * PUERTO_Y);
	uint8_t * b = malloc(PUERTO_X * PUERTO_Y);

	if (
		!sum_r ||
		!sum_g ||
		!sum_b ||
		!r ||
		!g ||
		!b
	   )
	{
		LOG("ERROR: child out of memory\n");
		exit(1);
	}

	for (py=0; py < PUERTO_Y; py++){
		for (px=0; px < PUERTO_X; px++){

			sum_r[px + PUERTO_X * py] = 0;
			sum_g[px + PUERTO_X * py] = 0;
			sum_b[px + PUERTO_X * py] = 0;

			pixel_count = 0;

			for (iy = py * ypix ; iy < (py+1) * ypix; iy++){
				for (ix = px * xpix ; ix < (px+1) * xpix; ix++){

					sum_r[px + PUERTO_X*py] += data[((ix + iy*width) << 2) +2];
					sum_g[px + PUERTO_X*py] += data[((ix + iy*width) << 2) +1];
					sum_b[px + PUERTO_X*py] += data[((ix + iy*width) << 2) +0];

					pixel_count++;

				}
			}

			r[px + PUERTO_X * py] = sum_r[px + PUERTO_X * py] / pixel_count;
			g[px + PUERTO_X * py] = sum_g[px + PUERTO_X * py] / pixel_count;
			b[px + PUERTO_X * py] = sum_b[px + PUERTO_X * py] / pixel_count;
		}
	}
	memcpy(frame_out_last, frame_out, PUERTO_X*PUERTO_Y*BYPP);
	for (py=0; py < PUERTO_Y; py++){
		for (px=0; px < PUERTO_X; px++){
			frame_out[BYPP*(px + PUERTO_X * py) + 0] = r[px + PUERTO_X * py];
			frame_out[BYPP*(px + PUERTO_X * py) + 1] = g[px + PUERTO_X * py];
			frame_out[BYPP*(px + PUERTO_X * py) + 2] = b[px + PUERTO_X * py];
		}
	}

}

/* Window_Dump: dump a window to a file which must already be open for
 *              writing.
 */
void Window_Dump(Window window, FILE *out)
{
	XColor *colors;
	unsigned buffer_size;
	int ncolors;
	XWindowAttributes win_info;
	XImage *image;
	int absx, absy, x, y;
	unsigned width, height;
	int dwidth, dheight;
	int bw;
	Window dummywin;

	/* Get the parameters of the window being dumped.
	 */
	if(!XGetWindowAttributes(dpy, window, &win_info)) 
		Fatal_Error("Can't get target window attributes.");

	/* handle any frame window */
	if (!XTranslateCoordinates (dpy, window, RootWindow (dpy, screen), 0, 0,
				&absx, &absy, &dummywin)) {
		LOG (
		"ERROR:  unable to translate window coordinates (%d,%d)\n",
		absx, absy);
		exit (1);
	}
	win_info.x = absx;
	win_info.y = absy;
	width = win_info.width;
	height = win_info.height;
	bw = 0;

	dwidth = DisplayWidth (dpy, screen);
	dheight = DisplayHeight (dpy, screen);

//	LOG("0x%x: %dx%d\n", (unsigned int)window,  width,  height);
//	LOG("0x%x: %dx%d\n", (unsigned int)window, dwidth, dheight);

	/* clip to window */
	if (absx < 0) width += absx, absx = 0;
	if (absy < 0) height += absy, absy = 0;
	if (absx + width > dwidth) width = dwidth - absx;
	if (absy + height > dheight) height = dheight - absy;

	/* Snarf the pixmap with XGetImage.
	 */
	x = absx - win_info.x;
	y = absy - win_info.y;

	int bytes_in_out_image = 0;

	while (1) /* don't care about rescales - use them as a feature instead P; */
	{
		image = XGetImage (dpy, window, x, y, width, height, AllPlanes, format);
		if (!image) {
			LOG("ERROR: unable to get image at %dx%d+%d+%d\n",
					width, height, x, y);
			exit (1);
		}

		/* Determine the pixmap size.
		*/
		buffer_size = Image_Size(image);

		ncolors = Get_XColors(&win_info, &colors);

		scale_down_image((uint8_t *)image->data, buffer_size, width, height, &bytes_in_out_image);

		/* write only if we have a new image                */
		/* FIXME timestamping // delay                      */
		/* FIXME delta-encoding                             */
		/*       can all be done once we have a file format */
		if (memcmp(frame_out, frame_out_last, PUERTO_X*PUERTO_Y*BYPP))
		{
			/* Write out the buffer */
			if (fwrite(frame_out, PUERTO_X*PUERTO_Y*BYPP, 1, out) != 1) {
				LOG("ERROR: fwrite(): %s\n", strerror(errno));
				exit(1);
			}
		}

		/* free the color buffer */
		if(ncolors > 0) free(colors);

		/* Free image */
		XDestroyImage(image);
		usleep(25000); /* 40 fps */
	}
}

void daemonize(void)
{
	int ret;

	switch (fork()) {
		case 0:
			break;
		case -1:
			printf("ERROR: fork()\n");
			exit(1);
		default:
			exit(0);
	}
	if (setsid() < 0)
	{
		printf("ERROR: setsid()}\n");
		exit(1);
	}
	if (chdir("/") < 0)
	{
		printf("ERROR: chdir()}\n");
		exit(1);
	}

	logfd = open(LOG_FILE,
	             O_WRONLY | O_CREAT | O_TRUNC,
	             S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
	if (logfd < 0)
	{
		printf("ERROR: open(%s): %s\n", LOG_FILE, strerror(errno));
		exit(1);
	}
	logfp = fdopen(logfd, "a");
	if (logfp == 0)
	{
		printf("ERROR: fdopen(%s): %s\n", LOG_FILE, strerror(errno));
		exit(1);
	}

//	close(0);
//	close(1);
//	close(2);

	daemon_pid = getpid();

	LOG("purtod starting up as pid %d\n", daemon_pid);

	int pidfile = open(PID_FILE, 
	                   O_WRONLY | O_CREAT | O_TRUNC,
	                   S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
	if (pidfile < 0)
		exit(1);
	snprintf(buf, 6, "%d", daemon_pid);
	ret = write(pidfile, buf, strlen(buf));
	if (ret != strlen(buf))
	{
		LOG("ERROR: could not write() pidfile\n");
		exit(1);
	}
	close(pidfile);
}

void init_fifo(void)
{
	int ret;

	ret = mkfifo(FIFO_FILE, S_IRUSR | S_IWUSR);
	if (ret)
	{
		LOG("couldn't mkfifo(%s): %s\n",
		    FIFO_FILE, strerror(errno));
		if (errno == EEXIST)
		{
			LOG("using existing fifo\n");
		}else{
			LOG("ERROR: giving up\n");
			exit(1);
		}
	}

	fifofd = open(FIFO_FILE, O_RDONLY | O_SYNC);
	if (fifofd < 0)
	{
		LOG("ERROR: couldn't open(%s): %s\n",
		    FIFO_FILE, strerror(errno));
		exit(1);
	}
	ret = fcntl(fifofd, F_SETFL, O_SYNC);
	if (ret)
	{
		LOG("ERROR: couldn't fcntl(%s): %s\n",
		    FIFO_FILE, strerror(errno));
		exit(1);
	}
}

void start_recording(long unsigned int win_id)
{
	char cbuf[BUFSZ]; /* child buf */
	switch (fork()) {
		case 0:
			break;
		case -1:
			LOG("ERROR: fork()\n");
			exit(1);
		default:
			return;
	}
	LOG("I am the recorder of window 0x%lx, my pid is %d\n", win_id, getpid());

	frame_out      = malloc(PUERTO_X*PUERTO_Y*BYPP);
	frame_out_last = malloc(PUERTO_X*PUERTO_Y*BYPP);
	if (
		!frame_out ||
		!frame_out_last
	   )
	{
		LOG("ERROR: out of memory\n");
		exit(1);
	}

	Setup_Display_And_Screen();
	snprintf(cbuf, BUFSZ-1, "%s/out_0x%lx_%d.puerto",
	         DATA_DIR, win_id, getpid());
	FILE * out = fopen(cbuf, "wb");
	if (!out)
	{
		LOG("ERROR: child cannot fopen %s: %s\n",
		    cbuf, strerror(errno));
		exit(1);
	}
	LOG("dumping to %s\n", cbuf);
	Window_Dump(win_id, out);

	exit(0);
}

void sig_child_handler(int s)
{
	if (s == SIGCHLD)
		is_recording = 0;
}

int main(int argc, char ** argv)
{
	daemonize();
	init_fifo();

	signal(SIGCHLD, sig_child_handler);

	int ret;
	long unsigned int win_id;

	while(1)
	{
		ret = read(fifofd, buf, BUFSZ-1);
		if (ret < 0)
		{
			LOG("ERROR: cannot read(): %s\n",
			    strerror(errno));
			exit(1);
		}
		if (ret > 0)
		{
			buf[ret] = 0;
			win_id = 0;
			sscanf(buf, "0x%lx", &win_id);
			if (!win_id)
			{
				LOG("WARNING: couldn't parse\"%s\"\n", buf);
			}else{
				/* wait for the old child to die, sothat is_recording gets free
				 * through the sigchld-handler */
				usleep(200000);

				if (is_recording)
				{
					LOG("WARNING: already recording. NOT dumping window 0x%lx :(\n", win_id);
				}else{
					LOG("+++ starting to dump window 0x%lx\n", win_id);
					is_recording = 1;
					start_recording(win_id);
				}
			}
		}
		usleep(100000);
	}

	return 0;
}


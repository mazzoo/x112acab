#include <unistd.h>
#include <stdio.h>
#include <stdint.h>
#include <arpa/inet.h>

#define PUERTO_X 24
#define PUERTO_Y  4

#define VERSION 0x00

#define VERSION_SHIFT      24
#define PKT_MASK_VERSION 0xff000000 /* 0x00 */
#define PKT_MASK_DBL_BUF 0x00008000 /* uses feature double buffering    */
#define PKT_MASK_FADING  0x00004000 /* uses feature fading              */
#define PKT_MASK_RGB16   0x00002000 /* r16g16b16 format                 */
#define PKT_MASK_RGB8    0x00001000 /* r8g8b8 format                    */
#define PKT_MASK_GREY4   0x00000800 /* 4bit grey per pixel              */
#define PKT_MASK_BW      0x00000400 /* bw 1 byte per pixel format       */
#define PKT_MASK_BW_PACK 0x00000200 /* bw pixel 8 pixel per byte format */
#define PKT_MASK_TYPE    0x000000ff

#define PKT_TYPE_SET_SCREEN_BLK     0x00
#define PKT_TYPE_SET_SCREEN_WHT     0x01
#define PKT_TYPE_SET_SCREEN_RND_BW  0x02
#define PKT_TYPE_SET_SCREEN_RND_COL 0x04

#define PKT_TYPE_SET_FRAME_RATE     0x08
#define PKT_TYPE_SET_FADE_RATE      0x09
#define PKT_TYPE_SET_DURATION       0x0a

#define PKT_TYPE_SET_PIXEL          0x10
#define PKT_TYPE_SET_SCREEN         0x11
#define PKT_TYPE_FLIP_DBL_BUF       0x12

#define PKT_TYPE_TEXT               0x20
#define PKT_TYPE_SET_FONT           0x21

#define PKT_TYPE_FLUSH_FIFO         0xfd

#define PKT_TYPE_SHUTDOWN           0xfe


int main(int argc, char ** argv)
{
	int screen_bytes = PUERTO_X*PUERTO_Y*3;
	uint8_t buf[screen_bytes];

	uint32_t hdr;
	uint32_t len;
	uint32_t frame_rate;

	hdr = PKT_TYPE_SET_FRAME_RATE;
	len = 12;
	frame_rate = 20;

	hdr = htonl(hdr);
	len = htonl(len);
	frame_rate = htonl(frame_rate);

	write(1, &hdr, 4);
	write(1, &len, 4);
	write(1, &frame_rate, 4);

	hdr = VERSION << VERSION_SHIFT;
	hdr |= PKT_MASK_RGB8;
	hdr |= PKT_TYPE_SET_SCREEN;
	len = 8 + screen_bytes;

	hdr = htonl(hdr);
	len = htonl(len);

	while (read(0, buf, screen_bytes) == screen_bytes)
	{
		write(1, &hdr, 4);
		write(1, &len, 4);
		write(1, buf, screen_bytes);
	}
	return 0;
}

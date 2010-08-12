#include <unistd.h>
#include <stdio.h>
#include <stdint.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>


#define PUERTO_X 24
#define PUERTO_Y  4

#define XMLHDR (char *) \
"<blm channels=\"3\" bits=\"8\" height=\"4\" width=\"24\">\n" \
"<header><title>%s</title>\n" \
"<description>captured from xscreensaver</description>\n" \
"<creator>xscreensaver/x112acab</creator>\n" \
"<author>mazzoo</author>\n" \
"<email>acab@mazzoo.de</email>\n" \
"<loop>yes</loop>\n" \
"<max_duration>323</max_duration>\n" \
"</header>\n"


int main(int argc, char ** argv)
{
	int screen_bytes = PUERTO_X*PUERTO_Y*3;
	uint8_t   buf[screen_bytes];
	uint8_t * pbuf;
	char      xmlbuf[4096];
	char    * pxmlbuf;

	int i, j;

	sprintf(xmlbuf, XMLHDR, argv[1]);
	printf(xmlbuf);

	int f;
	f = open(argv[1], O_RDONLY);

	while (read(f, buf, screen_bytes) == screen_bytes)
	{
		pbuf    = buf;

		printf("<frame duration=\"50\">\n");
		
		for (j=0; j < PUERTO_Y; j++)
		{
			printf("<row>");
			pxmlbuf = xmlbuf;
			for (i=0; i < PUERTO_X*3; i++)
			{
				sprintf(pxmlbuf, "%2.2x", *pbuf);
				pbuf++;
				pxmlbuf+=2;
			}
			//*pxmlbuf = 0;
			printf("%s", xmlbuf);
			printf("</row>\n");
		}
		printf("</frame>\n");
	}
	printf("</blm>\n");
	return 0;
}

#include <unistd.h>
#include <stdio.h>
#include <stdint.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>


#define PUERTO_X 24
#define PUERTO_Y  4

#define JSONHDR (char *) \
	"{\"max_duration\": 323,\n" \
	"\"description\": \"captured from xscreensaver\",\n" \
	"\"creator\": \"xscreensaver/x112acab\",\n" \
	"\"data\": [\n" \
	"\n"

#define JSONFOOTER (char *) \
	"\n" \
	"],\n" \
	"\"author\": \"mazzoo\",\n" \
	"\"height\": 4,\n" \
	"\"channels\": 3,\n" \
	"\"width\": 24,\n" \
	"\"depth\": 8,\n" \
	"\"title\": \"%s\",\n" \
	"\"type\": \"m\",\n" \
	"\"email\": \"acab@mazzoo.de\"}\n"


int main(int argc, char ** argv)
{
	int screen_bytes = PUERTO_X*PUERTO_Y*3;
	uint8_t   buf[screen_bytes];
	uint8_t * pbuf;
	char   jsonbuf[4096];
	char * pjsonbuf;

	int i, j;

	sprintf(jsonbuf, JSONHDR);
	printf(jsonbuf);

	int f;
	f = open(argv[1], O_RDONLY);

	while (read(f, buf, screen_bytes) == screen_bytes)
	{
		pbuf    = buf;

		printf("\t{\"duration\": 50,\n");
		printf("\t\"rows\": [\n");

		for (j=0; j < PUERTO_Y; j++)
		{
			printf("\t\t\"");
			pjsonbuf = jsonbuf;
			for (i=0; i < PUERTO_X*3; i++)
			{
				sprintf(pjsonbuf, "%2.2x", *pbuf);
				pbuf++;
				pjsonbuf+=2;
			}
			printf("%s", jsonbuf);
			printf("\",\n");
		}
		printf("\t\t]\n\t},\n");
	}

	sprintf(jsonbuf, JSONFOOTER, argv[1]);
	printf(jsonbuf);

	return 0;
}

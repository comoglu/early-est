#include <stdlib.h>
#include <stdio.h>
#include "sachdr.h"

main(int argc, char *argv[])
{
	FILE *fp;
	struct HDR s;
	float buf;
	double time;
	int n=0;

	if(argc<2){ 
		printf("\nUsage: %s SACfile\n\n", argv[0]);
		return 0;
	}

	if((fp=fopen(argv[1], "r"))==0){
		perror(argv[1]);
		return 0;
	}

	fread(&s, sizeof(s), 1, fp);

	while (fread(&buf, sizeof(buf), 1, fp)) {
		time=(n++)*s.DELTA+s.B;
		fprintf (stdout, "%.4f %f\n", time, buf);
	}

	fclose (fp);
	return (0);

}

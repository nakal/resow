#!/bin/sh

export HTTP_HOST=localhost
#export SERVER_PORT=80
export REQUEST_METHOD=POST
export REQUEST_URI="http://localhost/resow/fs?upload="
export SCRIPT_NAME=/resow
export PATH_INFO=/fs
export QUERY_STRING="upload="
export HTTP_ACCEPT=text/html
export HTTP_ACCEPT_LANGUAGE=en
export HTTP_ACCEPT_ENCODING=UTF-8
export CONTENT_LENGTH=637
export CONTENT_TYPE="multipart/form-data; boundary=---------------------------19637658443363693651182958651"

echo "\
-----------------------------19637658443363693651182958651
Content-Disposition: form-data; name="file"; filename="bu-rente-berechnung.c"
Content-Type: application/octet-stream

#include 


int main(void) {

	double sum=0.0;
	int i,j;
	double z = 0.046;

	for (j=1; j<4; j++) {
		for (i=0; i<12; i++) {
			sum=sum+69.0;
		}
		sum += sum*z;
	}

	for (j=4; j<40; j++) {
		for (i=0; i<12; i++) {
			sum=sum+221.0;
		}
		sum += sum*z;
	}

	printf("Euro: %f\n", sum);
	exit(0);
}


-----------------------------19637658443363693651182958651
Content-Disposition: form-data; name="path"

aaa
-----------------------------19637658443363693651182958651--
" | exec /usr/local/www/apache22/resow/resow


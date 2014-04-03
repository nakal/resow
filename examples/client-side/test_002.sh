#!/bin/sh

export HTTP_HOST=localhost
#export SERVER_PORT=80
export REQUEST_METHOD=GET
export REQUEST_URI=http://localhost/resow/fs
export SCRIPT_NAME=/resow
export PATH_INFO=/fs
export QUERY_STRING=
export HTTP_ACCEPT=text/html
export HTTP_ACCEPT_LANGUAGE=en
export HTTP_ACCEPT_ENCODING=UTF-8

exec /usr/local/www/apache22/resow/resow


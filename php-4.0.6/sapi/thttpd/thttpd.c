/*
   +----------------------------------------------------------------------+
   | PHP version 4.0                                                      |
   +----------------------------------------------------------------------+
   | Copyright (c) 1997-2001 The PHP Group                                |
   +----------------------------------------------------------------------+
   | This source file is subject to version 2.02 of the PHP license,      |
   | that is bundled with this package in the file LICENSE, and is        |
   | available at through the world-wide-web at                           |
   | http://www.php.net/license/2_02.txt.                                 |
   | If you did not receive a copy of the PHP license and are unable to   |
   | obtain it through the world-wide-web, please send a note to          |
   | license@php.net so we can mail you a copy immediately.               |
   +----------------------------------------------------------------------+
   | Author: Sascha Schumann <sascha@schumann.cx>                         |
   +----------------------------------------------------------------------+
*/


#include "php.h"
#include "SAPI.h"
#include "php_main.h"
#include "php_thttpd.h"
#include "php_variables.h"
#include "version.h"

#include "ext/standard/php_smart_str.h"

#include <sys/uio.h>

typedef struct {
	httpd_conn *hc;
	int post_off;
	void (*on_close)(int);
} php_thttpd_globals;

static php_thttpd_globals thttpd_globals;

#define TLS_D
#define TLS_DC
#define TLS_C
#define TLS_CC
#define TG(v) (thttpd_globals.v)
#define TLS_FETCH()

static int sapi_thttpd_ub_write(const char *str, uint str_length)
{
	int n;
	uint sent = 0;	
	TLS_FETCH();
	
	while (str_length > 0) {
		n = send(TG(hc)->conn_fd, str, str_length, 0);

		if (n == -1 && errno == EPIPE)
			php_handle_aborted_connection();
		if (n == -1 && errno == EAGAIN)
			continue;
		if (n <= 0) 
			return n;

		TG(hc)->bytes_sent += n;
		str += n;
		sent += n;
		str_length -= n;
	}

	return sent;
}

#define COMBINE_HEADERS 30

static int sapi_thttpd_send_headers(sapi_headers_struct *sapi_headers SLS_DC)
{
	char buf[1024];
	struct iovec vec[COMBINE_HEADERS];
	int n = 0;
	zend_llist_position pos;
	sapi_header_struct *h;
	size_t len;
	
	if (!SG(sapi_headers).http_status_line) {
		snprintf(buf, 1023, "HTTP/1.0 %d Something\r\n", SG(sapi_headers).http_response_code);
		len = strlen(buf);
		vec[n].iov_base = buf;
		vec[n].iov_len = len;
	} else {
		vec[n].iov_base = SG(sapi_headers).http_status_line;
		len = strlen(vec[n].iov_base);
		vec[n].iov_len = len;
		vec[++n].iov_base = "\r\n";
		vec[n].iov_len = 2;
		len += 2;
	}
	TG(hc)->status = SG(sapi_headers).http_response_code;
	TG(hc)->bytes_sent += len;
	n++;

	h = zend_llist_get_first_ex(&sapi_headers->headers, &pos);
	while (h) {
		vec[n].iov_base = h->header;
		vec[n++].iov_len = h->header_len;
		if (n >= COMBINE_HEADERS - 1) {
			if (writev(TG(hc)->conn_fd, vec, n) == -1 && errno == EPIPE)
				php_handle_aborted_connection();
			n = 0;
		}
		vec[n].iov_base = "\r\n";
		vec[n++].iov_len = 2;
		
		h = zend_llist_get_next_ex(&sapi_headers->headers, &pos);
	}

	vec[n].iov_base = "\r\n";
	vec[n++].iov_len = 2;

	if (n) {
		if (writev(TG(hc)->conn_fd, vec, n) == -1 && errno == EPIPE)
			php_handle_aborted_connection();
	}
	
	return SAPI_HEADER_SENT_SUCCESSFULLY;
}

static int sapi_thttpd_read_post(char *buffer, uint count_bytes SLS_DC)
{
	size_t read_bytes = 0, tmp;
	int c;
	TLS_FETCH();

	/* to understand this, read cgi_interpose_input() in libhttpd.c */
	c = TG(hc)->read_idx - TG(hc)->checked_idx;
	if (c > 0) {
		read_bytes = MIN(c, count_bytes);
		memcpy(buffer, TG(hc)->read_buf + TG(hc)->checked_idx, read_bytes);
		TG(hc)->checked_idx += read_bytes;
		count_bytes -= read_bytes;
	}
	
	count_bytes = MIN(count_bytes, 
			SG(request_info).content_length - SG(read_post_bytes) - TG(post_off));

	while (read_bytes < count_bytes) {
		tmp = recv(TG(hc)->conn_fd, buffer + read_bytes, 
				count_bytes - read_bytes, 0);
		if (tmp <= 0) 
			break;
		read_bytes += tmp;
	}
	
	return read_bytes;
}

static char *sapi_thttpd_read_cookies(SLS_D)
{
	TLS_FETCH();
	
	return TG(hc)->cookie;
}

#define BUF_SIZE 512
#define ADD_STRING(name)										\
	php_register_variable(name, buf, track_vars_array ELS_CC PLS_CC)

static void sapi_thttpd_register_variables(zval *track_vars_array ELS_DC SLS_DC PLS_DC)
{
	char buf[BUF_SIZE + 1];
	char *p;
	TLS_FETCH();

	php_register_variable("PHP_SELF", SG(request_info).request_uri, track_vars_array ELS_CC PLS_CC);
	php_register_variable("SERVER_SOFTWARE", SERVER_SOFTWARE, track_vars_array ELS_CC PLS_CC);
	php_register_variable("GATEWAY_INTERFACE", "CGI/1.1", track_vars_array ELS_CC PLS_CC);
	php_register_variable("REQUEST_METHOD", (char *) SG(request_info).request_method, track_vars_array ELS_CC PLS_CC);
	php_register_variable("REQUEST_URI", SG(request_info).request_uri, track_vars_array ELS_CC PLS_CC);
	php_register_variable("PATH_TRANSLATED", SG(request_info).path_translated, track_vars_array ELS_CC PLS_CC);

	p = inet_ntoa(TG(hc)->client_addr.sa_in.sin_addr);
	/* string representation of IPs are never larger than 512 bytes */
	if (p) {
		memcpy(buf, p, strlen(p) + 1);
		ADD_STRING("REMOTE_ADDR");
		ADD_STRING("REMOTE_HOST");
	}

	snprintf(buf, BUF_SIZE, "%d", TG(hc)->hs->port);
	ADD_STRING("SERVER_PORT");

	snprintf(buf, BUF_SIZE, "/%s", TG(hc)->pathinfo);
	ADD_STRING("PATH_INFO");

	snprintf(buf, BUF_SIZE, "/%s", TG(hc)->origfilename);
	ADD_STRING("SCRIPT_NAME");

#define CONDADD(name, field) 							\
	if (TG(hc)->field[0]) {								\
		php_register_variable(#name, TG(hc)->field, track_vars_array ELS_CC PLS_C); \
	}

	CONDADD(HTTP_REFERER, referer);
	CONDADD(HTTP_USER_AGENT, useragent);
	CONDADD(HTTP_ACCEPT, accept);
	CONDADD(HTTP_ACCEPT_ENCODING, accepte);
	CONDADD(HTTP_COOKIE, cookie);
	CONDADD(CONTENT_TYPE, contenttype);
	CONDADD(REMOTE_USER, remoteuser);
	CONDADD(SERVER_PROTOCOL, protocol);

	if (TG(hc)->contentlength != -1) {
		sprintf(buf, "%ld", (long) TG(hc)->contentlength);
		ADD_STRING("CONTENT_LENGTH");
	}

	if (TG(hc)->authorization[0])
		php_register_variable("AUTH_TYPE", "Basic", track_vars_array ELS_CC PLS_C);
}

static sapi_module_struct thttpd_sapi_module = {
	"thttpd",
	"thttpd",
	
	php_module_startup,
	php_module_shutdown_wrapper,
	
	NULL,									/* activate */
	NULL,									/* deactivate */

	sapi_thttpd_ub_write,
	NULL,
	NULL,									/* get uid */
	NULL,									/* getenv */

	php_error,
	
	NULL,
	sapi_thttpd_send_headers,
	NULL,
	sapi_thttpd_read_post,
	sapi_thttpd_read_cookies,

	sapi_thttpd_register_variables,
	NULL,									/* Log message */

	NULL,									/* Block interruptions */
	NULL,									/* Unblock interruptions */

	STANDARD_SAPI_MODULE_PROPERTIES
};

static void thttpd_module_main(TLS_D SLS_DC)
{
	zend_file_handle file_handle;
	CLS_FETCH();
	ELS_FETCH();
	PLS_FETCH();

	file_handle.type = ZEND_HANDLE_FILENAME;
	file_handle.filename = SG(request_info).path_translated;
	file_handle.free_filename = 0;
	file_handle.opened_path = NULL;

	if (php_request_startup(CLS_C ELS_CC PLS_CC SLS_CC) == FAILURE) {
		return;
	}
	
	php_execute_script(&file_handle CLS_CC ELS_CC PLS_CC);
	php_request_shutdown(NULL);
}

static void thttpd_request_ctor(TLS_D SLS_DC)
{
	char buf[1024];
	int offset;
	size_t filename_len;
	size_t cwd_len;
	smart_str s = {0};

	SG(request_info).query_string = TG(hc)->query?strdup(TG(hc)->query):NULL;

	smart_str_appends_ex(&s, TG(hc)->hs->cwd, 1);
	smart_str_appends_ex(&s, TG(hc)->expnfilename, 1);
	smart_str_0(&s);
	SG(request_info).path_translated = s.c;
	
	s.c = NULL;
	smart_str_appendc_ex(&s, '/', 1);
	smart_str_appends_ex(&s, TG(hc)->origfilename, 1);
	smart_str_0(&s);
	SG(request_info).request_uri = s.c;
	SG(request_info).request_method = httpd_method_str(TG(hc)->method);
	SG(sapi_headers).http_response_code = 200;
	SG(request_info).content_type = TG(hc)->contenttype;
	SG(request_info).content_length = TG(hc)->contentlength;
	
	php_handle_auth_data(TG(hc)->authorization SLS_CC);

	TG(post_off) = TG(hc)->read_idx - TG(hc)->checked_idx;

	/* avoid feeding \r\n from POST data to SAPI */
	offset = TG(post_off) - SG(request_info).content_length;

	if (offset > 0) {
		TG(post_off) -= offset;
		TG(hc)->read_idx -= offset;
	}
}

static void thttpd_request_dtor(TLS_D SLS_DC)
{
	if (SG(request_info).query_string)
		free(SG(request_info).query_string);
	free(SG(request_info).request_uri);
	free(SG(request_info).path_translated);
}

off_t thttpd_php_request(httpd_conn *hc)
{
	SLS_FETCH();
	TLS_FETCH();

	TG(hc) = hc;
	hc->bytes_sent = 0;
	
	thttpd_request_ctor(TLS_C SLS_CC);

	thttpd_module_main(TLS_C SLS_CC);

	thttpd_request_dtor(TLS_C SLS_CC);

	return 0;
}

void thttpd_register_on_close(void (*arg)(int)) 
{
	TG(on_close) = arg;
}

void thttpd_closed_conn(int fd)
{
	if (TG(on_close)) TG(on_close)(fd);
}

int thttpd_get_fd(void)
{
	return TG(hc)->conn_fd;
}

void thttpd_set_dont_close(void)
{
	TG(hc)->file_address = (char *) 1;
}

void thttpd_php_init(void)
{
	sapi_startup(&thttpd_sapi_module);
	thttpd_sapi_module.startup(&thttpd_sapi_module);
	SG(server_context) = (void *) 1;
}

void thttpd_php_shutdown(void)
{
	if (SG(server_context) != NULL) {
		thttpd_sapi_module.shutdown(&thttpd_sapi_module);
		sapi_shutdown();
	}
}

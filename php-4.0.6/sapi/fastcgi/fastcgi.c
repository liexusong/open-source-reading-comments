/*
   +----------------------------------------------------------------------+
   | PHP version 4.0							  |
   +----------------------------------------------------------------------+
   | Copyright (c) 1997-2001 The PHP Group				  |
   +----------------------------------------------------------------------+
   | This source file is subject to version 2.02 of the PHP license,	  |
   | that is bundled with this package in the file LICENSE, and is	  |
   | available at through the world-wide-web at				  |
   | http://www.php.net/license/2_02.txt.				  |
   | If you did not receive a copy of the PHP license and are unable to	  |
   | obtain it through the world-wide-web, please send a note to	  |
   | license@php.net so we can mail you a copy immediately.		  |
   +----------------------------------------------------------------------+
   | Author: Ben Mansell <php@slimyhorror.com>				  |
   +----------------------------------------------------------------------+
*/

/* Debugging */
/* #define DEBUG_FASTCGI 1 */

/* Two configurables for the FastCGI runner.
 *
 * PHP_FCGI_CHILDREN - if set, the FastCGI will pre-fork this many processes
 *		       which will accept requests.
 *
 * PHP_FCGI_MAX_REQUESTS - if set, the runner will kill itself after handling
 *			   the given number of requests. This is to curb any
 *			   memory leaks in PHP.
 */


/* The following code is based mainly on the thttpd sapi and the original
 * CGI code, no doubt with many new and interesting bugs created... :)
 */

#include "php.h"
#include "SAPI.h"
#include "php_main.h"
#include "php_fastcgi.h"
#include "php_variables.h"

#include "fcgi_config.h"
#include "fcgiapp.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/stat.h>


#define TLS_D
#define TLS_DC
#define TLS_C
#define TLS_CC
#define TLS_FETCH()


FCGX_Stream *in, *out, *err;
FCGX_ParamArray envp;
char *path_info = NULL;

/* Our original environment from when the FastCGI first started */
char **orig_env;

/* The environment given by the FastCGI */
char **cgi_env;

/* The manufactured environment, from merging the base environ with
 * the parameters set by the per-connection environment
 */
char **merge_env;


static int sapi_fastcgi_ub_write(const char *str, uint str_length)
{
	uint sent = FCGX_PutStr( str, str_length, out );
	return sent;
}


static void sapi_fastcgi_flush( void *server_context )
{
	if( FCGX_FFlush( out ) == -1 ) {
		php_handle_aborted_connection();
	}
}


static void sapi_fastcgi_send_header(sapi_header_struct *sapi_header, void *server_context)
{
	if( sapi_header ) {
#ifdef DEBUG_FASTCGI
		fprintf( stderr, "Header: %s\n", sapi_header->header );
#endif
		FCGX_PutStr( sapi_header->header, sapi_header->header_len, out );
	}
	FCGX_PutStr( "\r\n", 2, out );
}

static int sapi_fastcgi_read_post(char *buffer, uint count_bytes SLS_DC)
{
	size_t read_bytes = 0, tmp;
	int c;
	char *pos = buffer;
	TLS_FETCH();

	while( count_bytes ) {
		c = FCGX_GetStr( pos, count_bytes, in );
		read_bytes += c;
		count_bytes -= c;
		pos += c;
		if( !c ) break;
	}
	return read_bytes;
}

static char *sapi_fastcgi_read_cookies(SLS_D)
{
	return getenv( "HTTP_COOKIE" );
}


static void sapi_fastcgi_register_variables(zval *track_vars_array ELS_DC SLS_DC PLS_DC)
{
	char *self = getenv("REQUEST_URI");
	char *ptr = strchr( self, '?' );

	/*
         * note that the environment will already have been set up
         * via fastcgi_module_main(), below.
         *
         * fastcgi_module_main() -> php_request_startup() ->
         * php_hash_environment() -> php_import_environment_variables()
         */

	/* strip query string off this */
	if ( ptr ) *ptr = 0;
	php_register_variable( "PHP_SELF", getenv("REQUEST_URI"), track_vars_array ELS_CC PLS_CC);
	if ( ptr ) *ptr = '?';
}


static sapi_module_struct fastcgi_sapi_module = {
	"fastcgi",
	"FastCGI",
	
	php_module_startup,
	php_module_shutdown_wrapper,
	
	NULL,									/* activate */
	NULL,									/* deactivate */

	sapi_fastcgi_ub_write,
	sapi_fastcgi_flush,
	NULL,									/* get uid */
	NULL,									/* getenv */

	php_error,
	
	NULL,
	NULL,
	sapi_fastcgi_send_header,
	sapi_fastcgi_read_post,
	sapi_fastcgi_read_cookies,

	sapi_fastcgi_register_variables,
	NULL,									/* Log message */

	NULL,									/* Block interruptions */
	NULL,									/* Unblock interruptions */

	STANDARD_SAPI_MODULE_PROPERTIES
};

static void fastcgi_module_main(TLS_D SLS_DC)
{
	zend_file_handle file_handle;
	CLS_FETCH();
	ELS_FETCH();
	PLS_FETCH();

	file_handle.type = ZEND_HANDLE_FILENAME;
	file_handle.filename = SG(request_info).path_translated;
	file_handle.free_filename = 0;
	file_handle.opened_path = NULL;

	if (php_request_startup(CLS_C ELS_CC PLS_CC SLS_CC) == SUCCESS) {
		php_execute_script(&file_handle CLS_CC ELS_CC PLS_CC);
	}
	php_request_shutdown(NULL);
}


static void init_request_info( SLS_D )
{
	char *content_length = getenv("CONTENT_LENGTH");
	char *content_type = getenv( "CONTENT_TYPE" );
	const char *auth;
	struct stat st;
	char *pi = getenv( "PATH_INFO" );
	char *pt = getenv( "PATH_TRANSLATED" );
	path_info = strdup( pi );

	SG(request_info).request_method = getenv("REQUEST_METHOD");
	SG(request_info).query_string = getenv("QUERY_STRING");
	SG(request_info).request_uri = path_info;
	SG(request_info).content_type = ( content_type ? content_type : "" );
	SG(request_info).content_length = (content_length?atoi(content_length):0);
	SG(sapi_headers).http_response_code = 200;

	SG(request_info).path_translated = pt;
	/*
	 * if the file doesn't exist, try to extract PATH_INFO out
	 * of it by stat'ing back through the '/'
	 */
	if ( stat( pt, &st ) == -1 ) {
	   int len = strlen(pt);
	   char *ptr;
	   while( ptr = strrchr(pt,'/') ) {
	      *ptr = 0;
	      if ( stat(pt,&st) == 0 && S_ISREG(st.st_mode) ) {
		 /*
		  * okay, we found the base script!
		  * work out how many chars we had to strip off;
		  * then we can modify PATH_INFO
		  * accordingly
		  */
		 int slen = len - strlen(pt);
		 if ( pi ) {
		    int pilen = strlen( pi );
		    strcpy( pi, pi + pilen - slen );
		 }
		 break;
	      }
	   }
	   /*
	    * if we stripped out all the '/' and still didn't find
	    * a valid path... we will fail, badly. of course we would
	    * have failed anyway... is there a nice way to error?
	    */
	} else {
	   /* the first stat succeeded... */
	   if ( pi ) *pi = 0;
	}

	/* The CGI RFC allows servers to pass on unvalidated Authorization data */
	auth = getenv("HTTP_AUTHORIZATION");
#ifdef DEBUG_FASTCGI
	fprintf( stderr, "Authorization: %s\n", auth );
#endif
	php_handle_auth_data(auth SLS_CC);


}


void fastcgi_php_init(void)
{
	sapi_startup(&fastcgi_sapi_module);
	fastcgi_sapi_module.startup(&fastcgi_sapi_module);
	SG(server_context) = (void *) 1;
}

void fastcgi_php_shutdown(void)
{
	if (SG(server_context) != NULL) {
		fastcgi_sapi_module.shutdown(&fastcgi_sapi_module);
		sapi_shutdown();
	}
}


int main(int argc, char *argv[])
{
	int exit_status = SUCCESS;
	int c, i, len;
	zend_file_handle file_handle;
	char *s;
	char *argv0=NULL;
	char *script_file=NULL;
	zend_llist global_vars;
	int children = 8;
	int max_requests = 500;
	int requests = 0;
	int status;
	int env_size, cgi_env_size;

#ifdef FASTCGI_DEBUG
	fprintf( stderr, "Initialising now!\n" );
#endif

	/* Calculate environment size */
	env_size = 0;
	while( environ[ env_size ] ) { env_size++; }
	/* Also include the final NULL pointer */
	env_size++;

	/* Allocate for our environment */
	orig_env = malloc( env_size * sizeof( char *));
	if( !orig_env ) {
		perror( "Can't malloc environment" );
		exit( 1 );
	}
	memcpy( orig_env, environ, env_size * sizeof( char *));

#ifdef HAVE_SIGNAL_H
#if defined(SIGPIPE) && defined(SIG_IGN)
	signal(SIGPIPE,SIG_IGN); /* ignore SIGPIPE in standalone mode so
				    that sockets created via fsockopen()
				    don't kill PHP if the remote site
				    closes it.	in apache|apxs mode apache
				    does that for us!  thies@thieso.net
				    20000419 */
#endif
#endif

	sapi_startup(&fastcgi_sapi_module);

	if (php_module_startup(&fastcgi_sapi_module)==FAILURE) {
		return FAILURE;
	}

	/* How many times to run PHP scripts before dying */
	if( getenv( "PHP_FCGI_MAX_REQUESTS" )) {
		max_requests = atoi( getenv( "PHP_FCGI_MAX_REQUESTS" ));
		if( !max_requests ) {
			fprintf( stderr,
				 "PHP_FCGI_MAX_REQUESTS is not valid\n" );
			exit( 1 );
		}
	}

	/* Pre-fork, if required */
	if( getenv( "PHP_FCGI_CHILDREN" )) {
		children = atoi( getenv( "PHP_FCGI_CHILDREN" ));
		if( !children ) {
			fprintf( stderr,
				 "PHP_FCGI_CHILDREN is not valid\n" );
			exit( 1 );
		}
	}

	if( children ) {
		int parent = 1;
		int running = 0;
		while( parent ) {
			do {
#ifdef FASTCGI_DEBUG
				fprintf( stderr, "Forking, %d running\n",
					 running );
#endif
				switch( fork() ) {
				case 0:
					/* One of the children.
					 * Make sure we don't go round the
					 * fork loop any more
					 */
					parent = 0;
					break;
				case -1:
					perror( "php (pre-forking)" );
					exit( 1 );
					break;
				default:
					/* Fine */
					running++;
					break;
				}
			} while( parent && ( running < children ));

			if( parent ) {
				wait( &status );
				running--;
			}
		}
	}

	/* Main FastCGI loop */
#ifdef FASTCGI_DEBUG
	fprintf( stderr, "Going into accept loop\n" );
#endif

	while( FCGX_Accept( &in, &out, &err, &cgi_env ) >= 0 ) {

#ifdef FASTCGI_DEBUG
		fprintf( stderr, "Got accept\n" );
#endif

                cgi_env_size = 0;
                while( cgi_env[ cgi_env_size ] ) { cgi_env_size++; }
                merge_env = malloc( (env_size+cgi_env_size)*sizeof(char*) );
                if( !merge_env ) {
                   perror( "Can't malloc environment" );
                   exit( 1 );
                }
                memcpy( merge_env, orig_env, (env_size-1)*sizeof(char *) );
                memcpy( merge_env + env_size - 1,
                        cgi_env, (cgi_env_size+1)*sizeof(char *) );
                environ = merge_env;

		init_request_info( TLS_C SLS_CC );
		SG(server_context) = (void *) 1; /* avoid server_context==NULL checks */
		CG(extended_info) = 0;		      
		SG(request_info).argv0 = argv0;		       
		zend_llist_init(&global_vars, sizeof(char *), NULL, 0);

		fastcgi_module_main( TLS_C SLS_CC );
		if( path_info ) {
		   free( path_info );
		   path_info = NULL;
		}

		/* TODO: We should free our environment here, but
		 * some platforms are unhappy if they've altered our
		 * existing environment and we then free() the new
		 * environ pointer
		 */

		requests++;
		if( max_requests && ( requests == max_requests )) {
			FCGX_Finish();
			break;
		}
	}

#ifdef FASTCGI_DEBUG
	fprintf( stderr, "Exiting...\n" );
#endif
	return 0;
}

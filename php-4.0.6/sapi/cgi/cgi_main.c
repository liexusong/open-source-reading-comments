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
   | Authors: Rasmus Lerdorf <rasmus@lerdorf.on.ca>                       |
   |          Stig Bakken <ssb@fast.no>                                   |
   |          Zeev Suraski <zeev@zend.com>                                |
   +----------------------------------------------------------------------+
*/


#include "php.h"
#include "php_globals.h"
#include "php_variables.h"
#include "zend_modules.h"

#include "SAPI.h"

#include <stdio.h>
#include "php.h"
#ifdef PHP_WIN32
#include "win32/time.h"
#include "win32/signal.h"
#include <process.h>
#else
#include "build-defs.h"
#endif
#if HAVE_SYS_TIME_H
#include <sys/time.h>
#endif
#if HAVE_UNISTD_H
#include <unistd.h>
#endif
#if HAVE_SIGNAL_H
#include <signal.h>
#endif
#if HAVE_SETLOCALE
#include <locale.h>
#endif
#include "zend.h"
#include "zend_extensions.h"
#include "php_ini.h"
#include "php_globals.h"
#include "php_main.h"
#include "fopen_wrappers.h"
#include "ext/standard/php_standard.h"
#ifdef PHP_WIN32
#include <io.h>
#include <fcntl.h>
#include "win32/php_registry.h"
#endif

#if HAVE_SIGNAL_H
#include <signal.h>
#endif

#include "zend_compile.h"
#include "zend_execute.h"
#include "zend_highlight.h"
#include "zend_indent.h"


#include "php_getopt.h"

#define PHP_MODE_STANDARD	1
#define PHP_MODE_HIGHLIGHT	2
#define PHP_MODE_INDENT		3
#define PHP_MODE_LINT		4

extern char *ap_php_optarg;
extern int ap_php_optind;

#define OPTSTRING "aCc:d:ef:g:hilmnqs?vz:"

static int _print_module_info ( zend_module_entry *module, void *arg ) {
	php_printf("%s\n", module->name);
	return 0;
}

static int sapi_cgibin_ub_write(const char *str, uint str_length)
{
	const char *ptr = str;
	uint remaining = str_length;
	size_t ret;

	while (remaining > 0)
	{
		ret = fwrite(ptr, 1, MIN(remaining, 16384), stdout);
		if (!ret) {
			php_handle_aborted_connection();
		}
		ptr += ret;
		remaining -= ret;
	}

	return str_length;
}


static void sapi_cgibin_flush(void *server_context)
{
	if (fflush(stdout)==EOF) {
		php_handle_aborted_connection();
	}
}


static void sapi_cgi_send_header(sapi_header_struct *sapi_header, void *server_context)
{
	if (sapi_header) {
		PHPWRITE_H(sapi_header->header, sapi_header->header_len);
	}
	PHPWRITE_H("\r\n", 2);
}


static int sapi_cgi_read_post(char *buffer, uint count_bytes SLS_DC)
{
	uint read_bytes=0, tmp_read_bytes;

	count_bytes = MIN(count_bytes, (uint)SG(request_info).content_length-SG(read_post_bytes));
	while (read_bytes < count_bytes) {
		tmp_read_bytes = read(0, buffer+read_bytes, count_bytes-read_bytes);
		if (tmp_read_bytes<=0) {
			break;
		}
		read_bytes += tmp_read_bytes;
	}
	return read_bytes;
}


static char *sapi_cgi_read_cookies(SLS_D)
{
	return getenv("HTTP_COOKIE");
}


static void sapi_cgi_register_variables(zval *track_vars_array ELS_DC SLS_DC PLS_DC)
{
	/* In CGI mode, we consider the environment to be a part of the server
	 * variables
	 */
	php_import_environment_variables(track_vars_array ELS_CC PLS_CC);

	/* Build the special-case PHP_SELF variable for the CGI version */
	php_register_variable("PHP_SELF", (SG(request_info).request_uri ? SG(request_info).request_uri:""), track_vars_array ELS_CC PLS_CC);
}


static void sapi_cgi_log_message(char *message)
{
	if (php_header()) {
		fprintf(stderr, "%s", message);
		fprintf(stderr, "\n");
	}
}

static int sapi_cgi_deactivate(SLS_D)
{
	fflush(stdout);
	if(SG(request_info).argv0) {
		free(SG(request_info).argv0);
		SG(request_info).argv0 = NULL;
	}
	return SUCCESS;
}



static sapi_module_struct cgi_sapi_module = {
	"cgi",							/* name */
	"CGI",							/* pretty name */
									
	php_module_startup,				/* startup */
	php_module_shutdown_wrapper,	/* shutdown */

	NULL,							/* activate */
	sapi_cgi_deactivate,			/* deactivate */

	sapi_cgibin_ub_write,			/* unbuffered write */
	sapi_cgibin_flush,				/* flush */
	NULL,							/* get uid */
	NULL,							/* getenv */

	php_error,						/* error handler */

	NULL,							/* header handler */
	NULL,							/* send headers handler */
	sapi_cgi_send_header,			/* send header handler */

	sapi_cgi_read_post,				/* read POST data */
	sapi_cgi_read_cookies,			/* read Cookies */

	sapi_cgi_register_variables,	/* register server variables */
	sapi_cgi_log_message,			/* Log message */

	NULL,							/* Block interruptions */
	NULL,							/* Unblock interruptions */

	STANDARD_SAPI_MODULE_PROPERTIES
};


static void php_cgi_usage(char *argv0)
{
	char *prog;

	prog = strrchr(argv0, '/');
	if (prog) {
		prog++;
	} else {
		prog = "php";
	}

	php_printf("Usage: %s [-q] [-h] [-s [-v] [-i] [-f <file>] |  {<file> [args...]}\n"
				"  -q             Quiet-mode.  Suppress HTTP Header output.\n"
				"  -s             Display colour syntax highlighted source.\n"
				"  -f <file>      Parse <file>.  Implies `-q'\n"
				"  -v             Version number\n"
                "  -C             Do not chdir to the script's directory\n"
				"  -c <path>      Look for php.ini file in this directory\n"
#if SUPPORT_INTERACTIVE
				"  -a             Run interactively\n"
#endif
				"  -d foo[=bar]   Define INI entry foo with value 'bar'\n"
				"  -e             Generate extended information for debugger/profiler\n"
				"  -z <file>      Load Zend extension <file>.\n"
				"  -l             Syntax check only (lint)\n"
				"  -m             Show compiled in modules\n"
				"  -i             PHP information\n"
				"  -h             This help\n", prog);
}


static void init_request_info(SLS_D)
{
	char *content_length = getenv("CONTENT_LENGTH");
	char *content_type = getenv("CONTENT_TYPE");
	const char *auth;

#if 0
/* SG(request_info).path_translated is always set to NULL at the end of this function
   call so why the hell did this code exist in the first place? Am I missing something? */
	char *script_filename;


	script_filename = getenv("SCRIPT_FILENAME");
	/* Hack for annoying servers that do not set SCRIPT_FILENAME for us */
	if (!script_filename) {
		script_filename = SG(request_info).argv0;
	}
#ifdef PHP_WIN32
	/* FIXME WHEN APACHE NT IS FIXED */
	/* a hack for apache nt because it does not appear to set argv[1] and sets
	   script filename to php.exe thus makes us parse php.exe instead of file.php
	   requires we get the info from path translated.  This can be removed at
	   such a time that apache nt is fixed */
	if (!script_filename) {
		script_filename = getenv("PATH_TRANSLATED");
	}
#endif

	/* doc_root configuration variable is currently ignored,
	   as it is with every other access method currently also. */

	/* We always need to emalloc() filename, since it gets placed into
	   the include file hash table, and gets freed with that table.
	   Notice that this means that we don't need to efree() it in
	   php_destroy_request_info()! */
#if DISCARD_PATH
	if (script_filename) {
		SG(request_info).path_translated = estrdup(script_filename);
	} else {
		SG(request_info).path_translated = NULL;
	}
#endif

#endif /* 0 */

	SG(request_info).request_method = getenv("REQUEST_METHOD");
	SG(request_info).query_string = getenv("QUERY_STRING");
	SG(request_info).request_uri = getenv("PATH_INFO");
	if (!SG(request_info).request_uri) {
		SG(request_info).request_uri = getenv("SCRIPT_NAME");
	}
	SG(request_info).path_translated = NULL; /* we have to update it later, when we have that information */
	SG(request_info).content_type = (content_type ? content_type : "" );
	SG(request_info).content_length = (content_length?atoi(content_length):0);
	SG(sapi_headers).http_response_code = 200;
	
	/* The CGI RFC allows servers to pass on unvalidated Authorization data */
	auth = getenv("HTTP_AUTHORIZATION");
	php_handle_auth_data(auth SLS_CC);
}


static void define_command_line_ini_entry(char *arg)
{
	char *name, *value;

	name = arg;
	value = strchr(arg, '=');
	if (value) {
		*value = 0;
		value++;
	} else {
		value = "1";
	}
	zend_alter_ini_entry(name, strlen(name)+1, value, strlen(value), PHP_INI_SYSTEM, PHP_INI_STAGE_ACTIVATE);
}


static void php_register_command_line_global_vars(char **arg)
{
	char *var, *val;
	ELS_FETCH();
	PLS_FETCH();

	var = *arg;
	val = strchr(var, '=');
	if (!val) {
		printf("No value specified for variable '%s'\n", var);
	} else {
		*val++ = '\0';
		php_register_variable(var, val, NULL ELS_CC PLS_CC);
	}
	efree(*arg);
}
	



int main(int argc, char *argv[])
{
	int exit_status = SUCCESS;
	int cgi = 0, c, i, len;
	zend_file_handle file_handle;
	char *s;
/* temporary locals */
	int behavior=PHP_MODE_STANDARD;
	int no_headers=0;
	int orig_optind=ap_php_optind;
	char *orig_optarg=ap_php_optarg;
	char *argv0=NULL;
	char *script_file=NULL;
	zend_llist global_vars;
#if SUPPORT_INTERACTIVE
	int interactive=0;
#endif
/* end of temporary locals */
#ifdef ZTS
	zend_compiler_globals *compiler_globals;
	zend_executor_globals *executor_globals;
	php_core_globals *core_globals;
	sapi_globals_struct *sapi_globals;
#endif


#ifdef HAVE_SIGNAL_H
#if defined(SIGPIPE) && defined(SIG_IGN)
	signal(SIGPIPE,SIG_IGN); /* ignore SIGPIPE in standalone mode so
								that sockets created via fsockopen()
								don't kill PHP if the remote site
								closes it.  in apache|apxs mode apache
								does that for us!  thies@thieso.net
								20000419 */
#endif
#endif

#ifndef ZTS
	if (setjmp(EG(bailout))!=0) {
		return -1;
	}
#endif

#ifdef ZTS
	tsrm_startup(1,1,0, NULL);
#endif

	sapi_startup(&cgi_sapi_module);

#ifdef PHP_WIN32
	_fmode = _O_BINARY;			/*sets default for file streams to binary */
	setmode(_fileno(stdin), O_BINARY);		/* make the stdio mode be binary */
	setmode(_fileno(stdout), O_BINARY);		/* make the stdio mode be binary */
	setmode(_fileno(stderr), O_BINARY);		/* make the stdio mode be binary */
#endif

	if (php_module_startup(&cgi_sapi_module)==FAILURE) {
		return FAILURE;
	}

	/* Make sure we detect we are a cgi - a bit redundancy here,
	   but the default case is that we have to check only the first one. */
	if (getenv("SERVER_SOFTWARE")
		|| getenv("SERVER_NAME")
		|| getenv("GATEWAY_INTERFACE")
		|| getenv("REQUEST_METHOD")) {
		cgi = 1;
		if (argc > 1) {
			argv0 = strdup(argv[1]);
		} else {
			argv0 = NULL;
		}
#if FORCE_CGI_REDIRECT
		/* Apache will generate REDIRECT_STATUS,
		 * Netscape and redirect.so will generate HTTP_REDIRECT_STATUS.
		 * redirect.so and installation instructions available from
		 * http://www.koehntopp.de/php.
		 *   -- kk@netuse.de
		 */
		if (!getenv("REDIRECT_STATUS") && !getenv ("HTTP_REDIRECT_STATUS")) {
			PUTS("<b>Security Alert!</b>  PHP CGI cannot be accessed directly.\n\
\n\
<P>This PHP CGI binary was compiled with force-cgi-redirect enabled.  This\n\
means that a page will only be served up if the REDIRECT_STATUS CGI variable is\n\
set.  This variable is set, for example, by Apache's Action directive redirect.\n\
<P>You may disable this restriction by recompiling the PHP binary with the\n\
--disable-force-cgi-redirect switch.  If you do this and you have your PHP CGI\n\
binary accessible somewhere in your web tree, people will be able to circumvent\n\
.htaccess security by loading files through the PHP parser.  A good way around\n\
this is to define doc_root in your php.ini file to something other than your\n\
top-level DOCUMENT_ROOT.  This way you can separate the part of your web space\n\n\
which uses PHP from the normal part using .htaccess security.  If you do not have\n\
any .htaccess restrictions anywhere on your site you can leave doc_root undefined.\n\
\n");

			/* remove that detailed explanation some time */

			return FAILURE;
		}
#endif							/* FORCE_CGI_REDIRECT */
	}

	if (!cgi) {
		while ((c=ap_php_getopt(argc, argv, OPTSTRING))!=-1) {
			switch (c) {
				case 'c':
					cgi_sapi_module.php_ini_path_override = strdup(ap_php_optarg);
					break;
			}

		}
		ap_php_optind = orig_optind;
		ap_php_optarg = orig_optarg;
	}

#ifdef ZTS
	compiler_globals = ts_resource(compiler_globals_id);
	executor_globals = ts_resource(executor_globals_id);
	core_globals = ts_resource(core_globals_id);
	sapi_globals = ts_resource(sapi_globals_id);
	if (setjmp(EG(bailout))!=0) {
		return -1;
	}
#endif

	if (!cgi) {
		while ((c=ap_php_getopt(argc, argv, OPTSTRING))!=-1) {
			switch (c) {
				case '?':
					no_headers = 1;
					php_output_startup();
					SG(headers_sent) = 1;
					php_cgi_usage(argv[0]);
					php_end_ob_buffers(1);
					exit(1);
					break;
			}
		}
		ap_php_optind = orig_optind;
		ap_php_optarg = orig_optarg;
	}

	init_request_info(SLS_C);
	SG(server_context) = (void *) 1; /* avoid server_context==NULL checks */
	CG(extended_info) = 0;

	SG(request_info).argv0 = argv0;

	zend_llist_init(&global_vars, sizeof(char *), NULL, 0);

	if (!cgi) {					/* never execute the arguments if you are a CGI */
		if (SG(request_info).argv0) {
			free(SG(request_info).argv0);
			SG(request_info).argv0 = NULL;
		}
		while ((c = ap_php_getopt(argc, argv, OPTSTRING)) != -1) {
			switch (c) {
				
  			case 'a':	/* interactive mode */
#if SUPPORT_INTERACTIVE
					printf("Interactive mode enabled\n\n");
					interactive=1;
#else
					printf("Interactive mode not supported!\n\n");
#endif
					break;
				
			case 'C': /* don't chdir to the script directory */
					SG(options) |= SAPI_OPTION_NO_CHDIR;
					break;
			case 'd': /* define ini entries on command line */
					define_command_line_ini_entry(ap_php_optarg);
					break;
					
  			case 'e': /* enable extended info output */
					CG(extended_info) = 1;
					break;

  			case 'f': /* parse file */
					script_file = estrdup(ap_php_optarg);
					no_headers = 1;
					break;

  			case 'g': /* define global variables on command line */
					{
						char *arg = estrdup(ap_php_optarg);

						zend_llist_add_element(&global_vars, &arg);
					}
					break;

  			case 'h': /* help & quit */
				case '?':
					no_headers = 1;  
					php_output_startup();
					SG(headers_sent) = 1;
					php_cgi_usage(argv[0]);
					php_end_ob_buffers(1);
					exit(1);
					break;

			case 'i': /* php info & quit */
					if (php_request_startup(CLS_C ELS_CC PLS_CC SLS_CC)==FAILURE) {
						php_module_shutdown();
						return FAILURE;
					}
					if (no_headers) {
						SG(headers_sent) = 1;
						SG(request_info).no_headers = 1;
					}
					php_print_info(0xFFFFFFFF);
					exit(1);
					break;

  			case 'l': /* syntax check mode */
					no_headers = 1;
					behavior=PHP_MODE_LINT;
					break;

			case 'm': /* list compiled in modules */
		          	php_output_startup();
                                SG(headers_sent) = 1;
				php_printf("Running PHP %s\n%s\n", PHP_VERSION , get_zend_version());
				php_printf("[PHP Modules]\n");
				zend_hash_apply_with_argument(&module_registry, (int (*)(void *, void *)) _print_module_info, NULL);
				php_printf("\n[Zend Modules]\n");			
				zend_llist_apply_with_argument(&zend_extensions, (void (*)(void *, void *)) _print_module_info, NULL);
				php_printf("\n");
                php_end_ob_buffers(1);
                exit(1);
				break;

#if 0 /* not yet operational, see also below ... */
  			case 'n': /* generate indented source mode*/ 
					behavior=PHP_MODE_INDENT;
					break;
#endif

  			case 'q': /* do not generate HTTP headers */
					no_headers = 1;
					break;

  			case 's': /* generate highlighted HTML from source */
					behavior=PHP_MODE_HIGHLIGHT;
					break;

			case 'v': /* show php version & quit */
					no_headers = 1;
					if (php_request_startup(CLS_C ELS_CC PLS_CC SLS_CC)==FAILURE) {
						php_module_shutdown();
						return FAILURE;
					}
					if (no_headers) {
						SG(headers_sent) = 1;
						SG(request_info).no_headers = 1;
					}
					php_printf("%s\n", PHP_VERSION);
					php_end_ob_buffers(1);
					exit(1);
					break;

			case 'z': /* load extension file */
					zend_load_extension(ap_php_optarg);
					break;

				default:
					break;
			}
		}
	}							/* not cgi */

#if SUPPORT_INTERACTIVE
	EG(interactive) = interactive;
#endif

	if (!cgi) {
		if (!SG(request_info).query_string) {
			len = 0;
			if (script_file) {
				len += strlen(script_file) + 1;
			}
			for (i = ap_php_optind; i < argc; i++) {
				len += strlen(argv[i]) + 1;
			}

			s = malloc(len + 1);	/* leak - but only for command line version, so ok */
			*s = '\0';			/* we are pretending it came from the environment  */
			if (script_file) {
				strcpy(s, script_file);
				if (ap_php_optind<argc) {
					strcat(s, "+");
				}
			}
			for (i = ap_php_optind, len = 0; i < argc; i++) {
				strcat(s, argv[i]);
				if (i < (argc - 1)) {
					strcat(s, "+");
				}
			}
			SG(request_info).query_string = s;
		}
	}

	if (script_file) {
		SG(request_info).path_translated = script_file;
	}

	if (php_request_startup(CLS_C ELS_CC PLS_CC SLS_CC)==FAILURE) {
		php_module_shutdown();
		return FAILURE;
	}
	if (no_headers) {
		SG(headers_sent) = 1;
		SG(request_info).no_headers = 1;
	}
	file_handle.filename = "-";
	file_handle.type = ZEND_HANDLE_FP;
	file_handle.handle.fp = stdin;
	file_handle.opened_path = NULL;

	/* This actually destructs the elements of the list - ugly hack */
	zend_llist_apply(&global_vars, (llist_apply_func_t) php_register_command_line_global_vars);
	zend_llist_destroy(&global_vars);

	if (!cgi) {
		if (!SG(request_info).path_translated && argc > ap_php_optind) {
			SG(request_info).path_translated = estrdup(argv[ap_php_optind]);
		}
	} else {
	/* If for some reason the CGI interface is not setting the
	   PATH_TRANSLATED correctly, SG(request_info).path_translated is NULL.
	   We still call php_fopen_primary_script, because if you set doc_root
	   or user_dir configuration directives, PATH_INFO is used to construct
	   the filename as a side effect of php_fopen_primary_script.
	 */
		char *env_path_translated=NULL;
#if DISCARD_PATH
		env_path_translated = getenv("SCRIPT_FILENAME");
#else
		env_path_translated = getenv("PATH_TRANSLATED");
#endif
		if(env_path_translated) {
			SG(request_info).path_translated = estrdup(env_path_translated);
		}
	}
	if (cgi || SG(request_info).path_translated) {
		file_handle.handle.fp = php_fopen_primary_script();
		file_handle.filename = SG(request_info).path_translated;
	}

	if (cgi && !file_handle.handle.fp) {
		if(!argv0 || !(file_handle.handle.fp = VCWD_FOPEN(argv0, "rb"))) {
			PUTS("No input file specified.\n");
			php_request_shutdown((void *) 0);
			php_module_shutdown();
			return FAILURE;
		}
		file_handle.filename = argv0;
	} else if (file_handle.handle.fp && file_handle.handle.fp!=stdin) {
		/* #!php support */
		c = fgetc(file_handle.handle.fp);
		if (c == '#') {
			while (c != 10 && c != 13) {
				c = fgetc(file_handle.handle.fp);	/* skip to end of line */
			}
			CG(zend_lineno)++;
		} else {
			rewind(file_handle.handle.fp);
		}
	}

	file_handle.free_filename = 0;
	switch (behavior) {
		case PHP_MODE_STANDARD:
			exit_status = php_execute_script(&file_handle CLS_CC ELS_CC PLS_CC);
			break;
		case PHP_MODE_LINT:
			exit_status = php_lint_script(&file_handle CLS_CC ELS_CC PLS_CC);
			break;
		case PHP_MODE_HIGHLIGHT:
			{
				zend_syntax_highlighter_ini syntax_highlighter_ini;

				if (open_file_for_scanning(&file_handle CLS_CC)==SUCCESS) {
					php_get_highlight_struct(&syntax_highlighter_ini);
					zend_highlight(&syntax_highlighter_ini);
					fclose(file_handle.handle.fp);
				}
				return SUCCESS;
			}
			break;
#if 0
		/* Zeev might want to do something with this one day */
		case PHP_MODE_INDENT:
			open_file_for_scanning(&file_handle CLS_CC);
			zend_indent();
			fclose(file_handle.handle.fp);
			return SUCCESS;
			break;
#endif
	}


	if (SG(request_info).path_translated) {
		persist_alloc(SG(request_info).path_translated);
	}

	php_request_shutdown((void *) 0);
	php_module_shutdown();

	STR_FREE(SG(request_info).path_translated);

	if (cgi_sapi_module.php_ini_path_override) {
		free(cgi_sapi_module.php_ini_path_override);
	}
#ifdef ZTS
	tsrm_shutdown();
#endif

	return exit_status;
}

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 */

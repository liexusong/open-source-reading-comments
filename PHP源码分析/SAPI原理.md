
SAPI是PHP提供给其他服务器使用的接口（Server Application Programming Interface），如果一个应用需要与PHP交换数据，就必须通过SAPI来进行。<br />
首先应该必须按照PHP的约定，提供一些接口供PHP调用。PHP通过结构体sapi_module_struct来传递这些接口的，sapi_module_struct结构定义如下：<br />
```C
struct _sapi_module_struct {
	char *name;
	char *pretty_name;

	int (*startup)(struct _sapi_module_struct *sapi_module);
	int (*shutdown)(struct _sapi_module_struct *sapi_module);

	int (*activate)(SLS_D);
	int (*deactivate)(SLS_D);

	int (*ub_write)(const char *str, unsigned int str_length);
	void (*flush)(void *server_context);
	struct stat *(*get_stat)(SLS_D);
	char *(*getenv)(char *name, size_t name_len SLS_DC);

	void (*sapi_error)(int type, const char *error_msg, ...);

	int (*header_handler)(sapi_header_struct *sapi_header, sapi_headers_struct *sapi_headers SLS_DC);
	int (*send_headers)(sapi_headers_struct *sapi_headers SLS_DC);
	void (*send_header)(sapi_header_struct *sapi_header, void *server_context);

	int (*read_post)(char *buffer, uint count_bytes SLS_DC);
	char *(*read_cookies)(SLS_D);

	void (*register_server_variables)(zval *track_vars_array ELS_DC SLS_DC PLS_DC);
	void (*log_message)(char *message);

	char *php_ini_path_override;

	void (*block_interruptions)(void);
	void (*unblock_interruptions)(void);

	void (*default_post_reader)(SLS_D);
};
```
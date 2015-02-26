#ifndef PHP_DOTNET_H
#define PHP_DOTNET_H

#if WIN32|WINNT

extern PHP_MINIT_FUNCTION(DOTNET);
extern PHP_MSHUTDOWN_FUNCTION(DOTNET);
PHP_FUNCTION(DOTNET_load);

extern zend_module_entry DOTNET_module_entry;
#define DOTNET_module_ptr &DOTNET_module_entry

#else

#define DOTNET_module_ptr NULL

#endif  /* Win32|WINNT */

#define phpext_DOTNET_ptr DOTNET_module_ptr

#endif  /* PHP_DOTNET_H */

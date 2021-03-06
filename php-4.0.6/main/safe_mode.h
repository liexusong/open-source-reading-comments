#ifndef SAFE_MODE_H
#define SAFE_MODE_H

/* mode's for php_checkuid() */
#define CHECKUID_DISALLOW_FILE_NOT_EXISTS 0
#define CHECKUID_ALLOW_FILE_NOT_EXISTS 1
#define CHECKUID_CHECK_FILE_AND_DIR 2
#define CHECKUID_ALLOW_ONLY_DIR 3
#define CHECKUID_CHECK_MODE_PARAM 4

extern PHPAPI int php_checkuid(const char *filename, char *fopen_mode, int mode);
extern PHPAPI char *php_get_current_user(void);

#endif

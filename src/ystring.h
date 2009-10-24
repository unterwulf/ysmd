#ifndef _YSTRING_H_
#define _YSTRING_H_

#define STRING_INIT_LEN 4096

typedef struct
{
    char *data;
    char *ptr;
    size_t len;
} string_t;

string_t *initString(void);
char *getString(string_t *buf);
void freeString(string_t *buf);
int concatString(string_t *str, int8_t *str2);
int printfString(string_t *str, const char *fmt, ...);
int vprintfString(string_t *str, const char *fmt, va_list ap);

#endif /* _YSTRING_H_ */

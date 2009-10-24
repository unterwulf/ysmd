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
char *getString(const string_t *str);
size_t getStringLen(const string_t *str);
void clearString(string_t *str);
void freeString(string_t *str);
int concatString(string_t *str, const char *str2);
int printfString(string_t *str, const char *fmt, ...);
int vprintfString(string_t *str, const char *fmt, va_list ap);

#endif /* _YSTRING_H_ */

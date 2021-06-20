#ifndef FILEUTILS_H
#define FILEUTILS_H
#include <time.h>

/******************************************************************************
 * Constants
 *****************************************************************************/
#define MAX_DEEP 32

#define FLAGS_EXIT     0x00000001
#define FLAGS_VERBOSE  0x00000002
#define FLAGS_NOHEADER 0x00000004

/******************************************************************************
 * Macros
 *****************************************************************************/
#define STRINGIFY(x) #x
#define TOSTRING(x) STRINGIFY(x)

#define xcheck(cond, errmsg, ...) \
	do { \
		if (!(cond)) \
		{ \
			fprintf(stderr, __FILE__ ":" TOSTRING(__LINE__) " " errmsg " (%m)\n", ##__VA_ARGS__); \
			goto bye; \
		} \
	} while (0)

#define xcontinue(cond, errmsg, ...) \
{ \
	if (!(cond)) \
	{ \
		fprintf(stderr, __FILE__ ":" TOSTRING(__LINE__) " " errmsg " (%m)\n", ##__VA_ARGS__); \
		continue; /* do not use do { } while(0) with this */\
	} \
}

#define stradd_len(dst, dstoffset, src) \
	 stradd(dst, dstoffset, src, strlen(src))
#define stradd_cst(dst, dstoffset, src) \
	 stradd(dst, dstoffset, src, sizeof(src) - 1)

/******************************************************************************
 * Globals
 *****************************************************************************/
extern int g_flags;

// dst += src; with realloc
void stradd(char **dst, size_t *dstoffset, const char *src, size_t srcsize);

int isDir(const char *path);
int isFile(const char *path);
char* safeBasename(const char *path);
char* safeDirname(const char *path);
time_t lastModificationDateRec(const char *path);
int mkdirp(const char *dir);
int removeRec(const char * path);

char* searchAndReplace(char *str, const char *org, const char *rpl);
char* addQuotes(char *out);
char* sanitizePath(char *out);

int execute(const char *cmd, const char *opt, const char *a, const char *b);
int tar(const char *dst, const char *src);
int zip(const char *dst, const char *src);
int gzip(const char *dst, const char *src);
int bzip2(const char *dst, const char *src);
int cp(const char *dst, const char *src);
int svntargz(const char *dst, const char *src);

void pverbose(const char *msg, ...);

#endif // FILEUTILS_H

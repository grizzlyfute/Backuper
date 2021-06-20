#include "utils.h"
#include <dirent.h>
#include <libgen.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

int g_flags = 0;

void stradd(char **dst, size_t *dstoffset, const char *src, size_t srcsize)
{
	*dst = realloc(*dst, sizeof(char) * (*dstoffset + srcsize + 1));
	memcpy(&((*dst)[*dstoffset]), src, srcsize);
   *dstoffset += srcsize;
	(*dst)[*dstoffset] = '\0';
}

int isDir(const char *path)
{
	struct stat path_stat;
	if (stat(path, &path_stat) < 0) return 0;
	return S_ISDIR(path_stat.st_mode);
}

int isFile(const char *path)
{
	struct stat path_stat;
	if (stat(path, &path_stat) < 0) return 0;
	return S_ISREG(path_stat.st_mode);
}

char * safeDirname(const char * path)
{
	char * ret;
	char * ptr = strdup(path);
	ret = strdup(dirname(ptr));
	free(ptr);
	return ret;
}

char * safeBasename(const char *path)
{
	char * ret;
	char * ptr = strdup(path);
	ret = strdup(basename(ptr));
	free(ptr);
	return ret;
}

time_t _lastModificationDateRec(char **path, int depth, size_t workingOffset)
{
	time_t r = 0, t = 0;
	DIR * dir = NULL;
	struct dirent *ent;
	struct stat path_stat;
	size_t oldOffset;

	xcheck(depth < MAX_DEEP, "Max deep %d reach. Abort", MAX_DEEP);
	// follow symlink only if first call
	if (depth <= 0)
	{
		if (stat(*path, &path_stat) < 0)
		{
			pverbose ("Can not stat '%s' %m. Modification ts would be 0\n", *path);
			goto bye;
		}
	}
	else
	{
		if (lstat(*path, &path_stat) < 0)
		{
			pverbose ("Can not lstat '%s' %m. Modification ts would be 0\n", *path);
			goto bye;
		}
	}

	// Not following sym link has they can point on variable part, like /etc/mtab
	if (
		S_ISCHR(path_stat.st_mode) || // character device
		S_ISBLK(path_stat.st_mode) || // block device
		S_ISFIFO(path_stat.st_mode) || // FIFO device (named pipe)
		(depth > 0 && S_ISLNK(path_stat.st_mode)) || // symbolic link
		S_ISSOCK(path_stat.st_mode) // socket
	   )
	{
		r = 0;
	}
	else if (S_ISREG(path_stat.st_mode)) // Regular file
	{
		r = path_stat.st_mtim.tv_sec;
	}
	else if (S_ISDIR(path_stat.st_mode)) // Directory
	{
		r = path_stat.st_mtim.tv_sec;
		dir = opendir(*path);
		xcheck(dir != NULL, "can not open directory '%s'", *path);
		while ((ent = readdir(dir)) != NULL)
		{
			if(strcmp(ent->d_name, ".") == 0 || strcmp(ent->d_name, "..") == 0) continue;
			oldOffset = workingOffset;
			stradd_cst (path, &workingOffset, "/");
			stradd_len (path, &workingOffset, ent->d_name);
			t = _lastModificationDateRec(path, depth + 1, workingOffset);
			if (t > r) r = t;
			workingOffset = oldOffset;
			(*path)[workingOffset] = '\0';
		}
	}
	else
	{
		r = 0;
		pverbose ("Unsupported file type of '%s'\n", *path);
	}

bye:
	if (dir != NULL) closedir(dir);
	return r;
}
time_t lastModificationDateRec(const char *path)
{
	time_t r = 0;
	char * workingPtr = NULL;
	size_t workinOffset = 0;
	stradd_len (&workingPtr, &workinOffset, path);
	r = _lastModificationDateRec(&workingPtr, 0, workinOffset);
	if (workingPtr) free(workingPtr);
	return r;
}

char * searchAndReplace(char *str, const char *org, const char *rpl)
{
  const size_t rpllen = strlen(rpl);
  const size_t orglen = strlen(org);
  size_t len = strlen(str);
  size_t offset;
  char *ptr = str;
  while (ptr != NULL)
  {
    ptr = strstr(ptr, org);
    if (ptr != NULL)
    {
      offset = ptr - str;
	  len += rpllen - orglen;
	  str = realloc (str, sizeof(char)*(len + 1));
	  ptr = str + offset;
      memmove (ptr + rpllen, ptr + orglen, len - rpllen + orglen - offset);
      memcpy (ptr, rpl, rpllen);
      ptr += rpllen;
    }
  }
  return str;
}

char* sanitizePath(char *out)
{
  size_t len = strlen(out);
  char *zero, *wptr, *rptr;
  // Removing trailling '/'
  while (len > 0 && out[len - 1] == '/')
  {
    out[--len] = '\0';
  }

  // Remove '/./'
  do
  {
    out = searchAndReplace(out, "/./", "/");
  } while (strstr(out, "/./") != NULL);

  // Replace '//' by '/'
  do
  {
    out = searchAndReplace(out, "//", "/");
  } while (strstr(out, "//") != NULL);

  // Keep only one leading './'
  if (strncmp("./", out, 2) == 0)
  {
	  zero = out;
	  do
	  {
		len -= 2;
		zero += 2;
	  }
	  while (strncmp("./", zero, 2) == 0);
	  zero -= 2;
	  len += 2;

	  memmove(out, zero, len + 1);
   }

  // Resolve path '../..'
  if (strncmp("./", out, 2) == 0) rptr = out + 2;
  else rptr = out;
  while (strncmp(rptr, "../", 3) == 0) rptr += 3;
  wptr = rptr;
  zero = rptr;
  while (*rptr)
  {
    if (strncmp(rptr, "/../", 4) == 0 || strncmp(rptr, "/..", 4) == 0)
    {
      --wptr;
      while (wptr > zero && *wptr != '/') --wptr;
      rptr += 3;
    }
    else *wptr++ = *rptr++;
  }

  *wptr = '\0';
  return out;
}

char *addQuotes(char *out)
{
	if (!out) return NULL;
	size_t len = strlen(out);
	size_t i;
	out = realloc(out, len + 2 + 1);
	for (i = len + 1; i > 0; i--)
	{
		out[i] = out[i-1];
	}
	out[0] = '"';
	out[len + 1] = '"';
	out[len + 2] = '\0';
	return out;
}

int mkdirp(const char *dir)
{
	char * ptr = strdup(dir);
	size_t len = 0;
	size_t i;
	int r = 0;

	ptr = sanitizePath(ptr);
	len = strlen(ptr);

	for (i = 0; i < len; i++)
	{
		if (ptr[i] == '/' && i > 0)
		{
			ptr[i] = '\0';
			if (!isDir(ptr))
			{
				pverbose("Creating '%s'\n", ptr);
				xcheck((r = mkdir(ptr, 0755)) >= 0, "Fail to make dir '%s'", ptr);
			}
			ptr[i] = '/';
		}
	}
	xcheck((r = mkdir(ptr, 0755)) >= 0, "Fail to make dir '%s'", ptr);

bye:
	free(ptr);
	return r;
}

int _removeRec(char **path, int depth, size_t workingOffset)
{
	int r = 0,t;
	DIR * dir = NULL;
	struct dirent *ent;
	struct stat path_stat;
	size_t oldOffset;

	xcheck(depth < MAX_DEEP, "Max deep %d reach. Abort", MAX_DEEP);
	if (stat(*path, &path_stat) < 0)
	{
		pverbose ("Can not stat '%s' %m. Modification ts would be 0\n", *path);
		goto bye;
	}

	if (S_ISREG(path_stat.st_mode))
	{
		r = unlink(*path);
		pverbose("Unlink '%s', return %d\n", *path, r);
		xcheck(r >= 0, "Unlink '%s' fail with retcode %d", *path, r);
	}
	else if (S_ISDIR(path_stat.st_mode))
	{
		dir = opendir(*path);
		xcheck(dir != NULL, "Can not open directory '%s'", *path);

		while ((ent = readdir(dir)) != NULL)
		{
			if(strcmp(ent->d_name, ".") == 0 ||
			   strcmp(ent->d_name, "..") == 0)
			{
				continue;
			}
			oldOffset = workingOffset;
			stradd_cst (path, &workingOffset, "/");
			stradd_len (path, &workingOffset, ent->d_name);
			t = _removeRec(path, depth + 1, workingOffset);
			if (r >= 0) r = t;
			(*path)[workingOffset - strlen(ent->d_name) - 1] = '\0';
			workingOffset = oldOffset;
		}

		t = rmdir(*path);
		pverbose("Rmdir '%s', return %d\n", *path, t);
		if (r >= 0) r = t;
		xcheck(t >= 0, "rmdir '%s' fail with retcode %d", *path, t);
	}
	else
	{
		r = -1;
		xcheck(r >= 0, "Unrecognized file type '%s'", *path);
	}
bye:
	if (dir != NULL) closedir(dir);
	return r;
}
int removeRec(const char * path)
{
	time_t r = 0;
	char * workingPtr = NULL;
	size_t workinOffset = 0;
	stradd_len (&workingPtr, &workinOffset, path);
	workingPtr = sanitizePath(workingPtr);
	r = _removeRec(&workingPtr, 0, workinOffset);
	if (workingPtr) free(workingPtr);
	return r;
}

int execute(const char *cmd, const char *opt, const char *foo, const char *bar)
{
	char * command = NULL;
	size_t t = 0;
	int r = 0;

	if (cmd) stradd_len (&command, &t, cmd);
	else return -1;
	command = sanitizePath(command);
	if (opt)
	{
		stradd_cst (&command, &t, " ");
		stradd_len (&command, &t, opt);
	}
	if (foo)
	{
		stradd_cst (&command, &t, " ");
		stradd_len (&command, &t, foo);
	}
	if (bar)
	{
		stradd_cst (&command, &t, " ");
		stradd_len (&command, &t, bar);
	}

	pverbose("Executing '%s'\n", command);
	r = system(command);
	pverbose("Retcode is %d\n", r);
	free (command);
	return r;
}

int tar(const char *dst, const char *src)
{
	char *args = NULL;
	size_t t = 0;
	int r = 0;
	char * ptr;

	stradd_cst(&args, &t, "-C \"");
	ptr = safeDirname(src);
	stradd_len(&args, &t, ptr);
	free (ptr);
	stradd_cst(&args, &t, "\" -cf \"");
	if (*dst != '/')
	{
		ptr = getcwd(NULL, 0);
		stradd_len(&args, &t, ptr);
		free (ptr);
		stradd_cst(&args, &t, "/");
	}
	stradd_len(&args, &t, dst);
	stradd_cst(&args, &t, "\" ");
	ptr = safeBasename(src);
	stradd_len (&args, &t, ptr);
	free (ptr);

	r = execute("tar", args, NULL, NULL);

	free(args);
	return r;
}

int zip(const char *dst, const char *src)
{
	int ret;
	char * qdst = strdup(dst);
	char * qsrc = strdup(src);

	qdst = addQuotes(qdst);
	qsrc = addQuotes(qsrc);
	ret = execute("zip", "-r -9", qdst, qsrc);

	if (qdst) free(qdst);
	if (qsrc) free(qsrc);

	return ret;
}

int gzip(const char *dst, const char *src)
{
	char *args = NULL;
	size_t t = 0;
	int r = 0;

	if (isFile(src))
	{
		stradd_cst(&args, &t, " > \"");
		stradd_len(&args, &t, dst);
		stradd_cst(&args, &t, "\"");
		r = execute ("gzip", "-9 -k -c", src, args);
	}
	else if (strstr (dst, ".tar.gz") != NULL)
	{
		args = strdup(dst);
		*(strstr(args, ".gz")) = '\0';
		r = tar(args, src);
		args = addQuotes (args);
		if (r == 0) r = execute("gzip", "-9", args, NULL);
		else fprintf(stderr, __FILE__ ":" TOSTRING(__LINE__) " tar fail (%d)\n", r);
	}

	return r;
}

int bzip2(const char *dst, const char *src)
{
	char *args = NULL;
	size_t t = 0;
	int r = 0;

	if (isFile(src))
	{
		stradd_cst(&args, &t, " > \"");
		stradd_len(&args, &t, dst);
		stradd_cst(&args, &t, "\"");
		r = execute ("bzip2", "-9 -k -c", src, args);
	}
	else if (strstr (dst, ".tar.bz2") != NULL)
	{
		args = strdup (dst);
		*(strstr (args, ".bz2")) = '\0';
		r = tar(args, src);
		args = addQuotes (args);
		if (r == 0) r = execute("bzip2", "-9", args, NULL);
		else fprintf(stderr, __FILE__ ":" TOSTRING(__LINE__) " tar fail (%d)\n", r);
	}
	if (args) free(args);
	return r;
}

int cp(const char *dst, const char *src)
{
	int ret;
	char * qdst = strdup(dst);
	char * qsrc = strdup(src);

	qdst = addQuotes(qdst);
	qsrc = addQuotes(qsrc);
	ret = execute("cp", "-pr", qsrc, qdst);

	if (qdst) free(qdst);
	if (qsrc) free(qsrc);

	return ret;
}

int svntargz(const char *dst, const char *src)
{
	char *args = NULL;
	size_t t = 0;
	int r = 0;
	char * ptr;

	stradd_cst(&args, &t, "-C \"");
	ptr = safeDirname(src);
	stradd_len(&args, &t, ptr);
	free (ptr);
	stradd_cst(&args, &t, "\" -czf \"");
	if (*dst != '/')
	{
		ptr = getcwd(NULL, 0);
		stradd_len(&args, &t, ptr);
		free (ptr);
		stradd_cst(&args, &t, "/");
	}
	stradd_len(&args, &t, dst);
	stradd_cst(&args, &t, "\" ");
	ptr = safeBasename(src);
	stradd_len (&args, &t, ptr);
	free (ptr);

	r = execute("GZIP=-9 svnadmin freeze", src, "-- tar", args);

	free(args);
	return r;
}

void pverbose(const char *msg, ...)
{
	va_list argp;
	va_start(argp, msg);
	if (g_flags & FLAGS_VERBOSE)
	{
		vprintf(msg, argp);
	}
	va_end(argp);
}

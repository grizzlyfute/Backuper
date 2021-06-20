#include "backuper.h"
#include "utils.h"
#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

#define BUFFER_SIZE 1024

const char * getNextToken (const char * begin, char sep, char **dst)
{
	char * end = NULL;
	if (!begin) return NULL;
	// Remove leading space
	while (*begin != '\0' && *begin <= ' ') begin++;

	end = strchr(begin, sep);
	if (end != NULL)
	{
		*dst = strndup (begin, end - begin);
		end += 1;
	}
	else  *dst = strdup (begin);

	begin = end;

	// Remove trailing space
	end = *dst + strlen(*dst) - 1;
	while (end >= *dst && *end <= ' ') {*end = '\0'; --end;};

	return begin;
}

BackupArr * bkGenerateList(const char * configFile, char separator)
{
	BackupArr * bkl = calloc(1, sizeof(BackupArr));
	BackupItem item;
	char buf[BUFFER_SIZE];
	FILE *file = fopen(configFile, "r");
	const char *ptr = NULL;
	char * intptr = NULL;
	int row = 0;

	xcheck(bkl != NULL, "calloc fail");
	bkl->a = NULL;
	bkl->size = 0;
	xcheck(file, "Error opening file '%s", configFile);

	while (fgets(buf, BUFFER_SIZE, file) != NULL)
	{
		row++;
		if (row <= 1 && (~g_flags & FLAGS_NOHEADER)) continue;
		for (ptr = buf; *ptr != '\0' && (*ptr == '\t' || *ptr == ' '); ptr++) {}
		if (*ptr == '#') continue;
		memset (&item, 0, sizeof(item));

		// strtok can not be used (strok(";;", ";") return NULL instead "\0";
		ptr = buf;
		ptr = getNextToken(ptr, separator, &(item.src));
		item.src = sanitizePath(item.src);
		ptr = getNextToken(ptr, separator, &(item.dst));
		item.dst = sanitizePath(item.dst);
		ptr = getNextToken(ptr, separator, &(item.name));
		ptr = getNextToken(ptr, separator, &(intptr));
		item.nb = atoi (intptr);
		free (intptr);
		ptr = getNextToken(ptr, separator, &(item.method));
		ptr = getNextToken(ptr, separator, &(item.check));

		if (
			item.src != NULL && item.src[0] != '\0' &&
			item.dst != NULL && item.dst[0] != '\0' &&
			item.name != NULL &&
			item.nb > 0 && item.nb < 1000 &&
			item.method != NULL && item.method[0] != '\0' &&
			// item.check != NULL  &&
		1 )
		{
			bkl->a = realloc(bkl->a, (bkl->size + 1)*sizeof(BackupItem));
			memcpy(&bkl->a[bkl->size], &item, sizeof(BackupItem));
			bkl->size++;
		}
		else
		{
			for (intptr = &buf[strlen(buf) - 1];
			   	intptr >= buf && (*intptr == '\n' || *intptr == '\r');
				intptr--) *intptr = '\0';
			if (item.src) free(item.src);
			if (item.dst) free(item.dst);
			if (item.name) free(item.name);
			if (item.method) free(item.method);
			if (item.check) free(item.check);
			xcontinue (0, "Syntax error in file '%s:%d' \"%s\"'. Ignoring", configFile, row, buf);
		}
	}

bye:
	if (file) fclose(file);
	return bkl;
}

int bkDoBackup(const BackupArr * bkl)
{
	char *bkname = NULL;
	size_t bklen = 0;
	size_t i, j;
	int r = 0, err = 0;
	char *ptr = NULL;

	for (i = 0; i < bkl->size; i++)
	{
		if (r == 0 && err != 0) r = err;
		pverbose("Backuping '%s' (%zu %%)\n", bkl->a[i].src, 100 * i / bkl->size);
		// Check read access
		xcontinue((err = access(bkl->a[i].src, R_OK)) >= 0,
				"Can not read file/dir '%s'", bkl->a[i].src);

		// Check if destination directory exists. Else create it
		if (!isDir(bkl->a[i].dst))
		{
			pverbose("Creating dir '%s'\n", bkl->a[i].dst);
			xcontinue((err = mkdirp(bkl->a[i].dst)) >= 0,
					"Can not make destination directory '%s'", bkl->a[i].dst);
		}

		// Check write destination access
		xcontinue((err = access(bkl->a[i].dst, W_OK)) >= 0,
				"Can not write into '%s'. Skip", bkl->a[i].dst);

		// Generate backupname
		bklen = 0;
		stradd_len (&bkname, &bklen, bkl->a[i].dst);
		if (bklen <= 0 || bkname[bklen - 1] != '/') stradd_cst (&bkname, &bklen, "/");
		for (j = 0; bkl->a[i].name[j] != '\0' && bkl->a[i].name[j] <= ' '; j++) { }
		if (bkl->a[i].name[j] == '\0')
		{
			ptr = safeBasename(bkl->a[i].src);
			stradd_len (&bkname, &bklen, ptr);
			free (ptr);
			ptr = NULL;
		}
	   	else
		{
			stradd_len (&bkname, &bklen, &bkl->a[i].name[j]);
		}
		stradd_cst (&bkname, &bklen, ".000");
		if (strcmp(bkl->a[i].method, "copy") == 0)
		{
			stradd_cst (&bkname, &bklen, "");
		}
		else if (strcmp(bkl->a[i].method, "gzip") == 0)
		{
			if (isFile(bkl->a[i].src))
				stradd_cst (&bkname, &bklen, ".gz");
			else
				stradd_cst (&bkname, &bklen, ".tar.gz");
		}
		else if (strcmp(bkl->a[i].method, "bzip2") == 0)
		{
			if (isFile(bkl->a[i].src))
				stradd_cst (&bkname, &bklen, ".bz2");
			else
				stradd_cst (&bkname, &bklen, ".tar.bz2");
		}
		else if (strcmp(bkl->a[i].method, "zip") == 0)
		{
			stradd_cst (&bkname, &bklen, ".zip");
		}
		else if (strcmp(bkl->a[i].method, "tar") == 0)
		{
			stradd_cst (&bkname, &bklen, ".tar");
		}
		else if (strcmp(bkl->a[i].method, "svntargz") == 0)
		{
			stradd_cst (&bkname, &bklen, ".tgz");
		}
		else
		{
			err = -1;
			xcontinue (0, "File '%s': unsupported compression method '%s'",
					bkl->a[i].src, bkl->a[i].method);
		}
		bkname = sanitizePath(bkname);

		// Check if file should be saved
		if (lastModificationDateRec(bkl->a[i].src) <= lastModificationDateRec(bkname))
		{
			pverbose("File '%s' is older than backup '%s'. Skip it !\n",
					bkl->a[i].src, bkname);
			continue;
		}

		// Use check command to check source validity
		if (bkl->a[i].check && bkl->a[i].check[0] != '\0')
		{
			ptr = strdup(bkl->a[i].src);
			ptr = addQuotes(ptr);
			xcontinue((err = execute(bkl->a[i].check, NULL, ptr, NULL)) == 0,
				"Check fail for '%s' (ret=%d)", bkl->a[i].src, err);
			free(ptr);
			ptr = NULL;
		}

		// Rotate backup if any
		xcontinue((err = rotateBackup(bkl->a[i].dst, bkname, bkl->a[i].nb)) >= 0,
				"Fail to rotate backup in '%s' [%s]'", bkl->a[i].dst, bkname);

		// Generate new backup
		pverbose ("Bakuping '%s' to '%s' with method '%s'\n", bkl->a[i].src, bkname, bkl->a[i].method);
		if (strcmp(bkl->a[i].method, "copy") == 0)
		{
			xcontinue((err = cp(bkname, bkl->a[i].src)) >= 0,
					"Fail to cp '%s' to '%s'", bkl->a[i].src, bkname);
		}
		else if (strcmp(bkl->a[i].method, "gzip") == 0)
		{
			xcontinue((err = gzip(bkname, bkl->a[i].src)) >= 0,
					"Fail to gzip '%s' to '%s'", bkl->a[i].src, bkname);
		}
		else if (strcmp(bkl->a[i].method, "bzip2") == 0)
		{
			xcontinue((err = bzip2(bkname, bkl->a[i].src)) >= 0,
					"Fail to bzip2 '%s' to '%s'", bkl->a[i].src, bkname);
		}
		else if (strcmp(bkl->a[i].method, "zip") == 0)
		{
			xcontinue((err = zip(bkname, bkl->a[i].src)) >= 0,
					"Fail to zip '%s' to '%s'", bkl->a[i].src, bkname);
		}
		else if (strcmp(bkl->a[i].method, "tar") == 0)
		{
			xcontinue((err = tar(bkname, bkl->a[i].src)) >= 0,
					"Fail to tar '%s' to '%s'", bkl->a[i].src, bkname);
		}
		else if (strcmp(bkl->a[i].method, "svntargz") == 0)
		{
			xcontinue((err = svntargz(bkname, bkl->a[i].src)) >= 0,
					"Fail to svn targz '%s' to '%s'", bkl->a[i].src, bkname);
		}
		else
		{
			err = -1;
			xcontinue(0, "File '%s': unsupported compression method '%s'",
					bkl->a[i].src, bkl->a[i].method);
		}

		pverbose("Create backup from '%s' to '%s'\n", bkl->a[i].src, bkname);
	}
	if (bkname != NULL) free(bkname);
	return r;
}

typedef struct FilelistItem FilelistItem;
struct FilelistItem { char *old; char *new; };
int cmpFileList(const void *aa, const void *bb)
{
	const FilelistItem *a = (FilelistItem*)aa;
	const FilelistItem *b = (FilelistItem*)bb;
	int r = 0;

	// old
	if (r == 0)
	{
		if (a->old == NULL && b->old == NULL) r = 0;
		else if (a->old != NULL && b->old == NULL) r = 1;
		else if (a->old == NULL && b->old != NULL) r = -1;
		else r = strcmp(a->old, b->old);
	}

	// new
	if (r == 0)
	{
		if (a->new == NULL && b->new == NULL) r = 0;
		else if (a->new != NULL && b->new == NULL) r = 1;
		else if (a->new == NULL && b->new != NULL) r = -1;
		else r = strcmp(a->new, b->new);
	}

	return r;
}

int cmpFileListReverse(const void *aa, const void *bb)
{
	return cmpFileList(bb, aa);
}

// path ref = "name_of_file."
int rotateBackup(const char* dirname, const char *pathrefArg, unsigned int max)
{
	int r = -1;
	DIR *d = NULL;
	struct dirent *dir = NULL;
	FilelistItem * filelist = NULL;
	size_t i = 0;
	size_t listlen = 0;
	size_t dirlen = 0;
	FilelistItem item = { .old = NULL, .new = NULL};
	char nbuf[4];
	size_t pathlen;
	char *pathref = safeBasename(pathrefArg); // should be freed

	item.old = pathref - 1;
	do
	{
		item.new = strstr(item.old + 1, ".000");
		if (item.new != NULL) item.old = item.new;
	} while (item.new);
	if (item.old == NULL || item.old < pathref) goto bye; //nothing to rotate
	pathlen = item.old - pathref;

	// Stage1: analyse file to rotate
	d = opendir(dirname);
	xcheck(d, "Fail to open dir '%s'", dirname);

	pverbose("Rotating file in dir '%s' for pathref '%s'...\n", dirname, pathref);
	while ((dir = readdir(d)) != NULL)
	{
		if (strncmp (dir->d_name, pathref, pathlen) == 0 &&
			dir->d_name[pathlen + 0] == '.' &&
			dir->d_name[pathlen + 1] >= '0' && dir->d_name[pathlen + 1] <= '9' &&
			dir->d_name[pathlen + 2] >= '0' && dir->d_name[pathlen + 2] <= '9' &&
			dir->d_name[pathlen + 3] >= '0' && dir->d_name[pathlen + 3] <= '9')
		{
			item.old = NULL;
			item.new = NULL;
			dirlen = 0;

			for (i = pathlen + 1; dir->d_name[i] == '0'; i++) {}
			if (dir->d_name[i] < '0' || dir->d_name[i] > '9') i = 0;
			else i = atoi(&dir->d_name[i]);

			stradd_len(&item.old, &dirlen, dirname);
			stradd_cst(&item.old, &dirlen, "/");
			stradd_len(&item.old, &dirlen, dir->d_name);

			item.new = NULL;
			if (i < max)
			{
				dirlen = 0;
				snprintf(nbuf, 4, "%03zu", i + 1);
				stradd_len(&item.new, &dirlen, dirname);
				stradd_cst(&item.new, &dirlen, "/");
				stradd(&item.new, &dirlen, dir->d_name, pathlen + 1);
				stradd(&item.new, &dirlen, nbuf, 3);
				stradd_len(&item.new, &dirlen, dir->d_name + pathlen + 4);
			}
			filelist = realloc(filelist, (listlen + 1) * sizeof(FilelistItem));
			memcpy(&filelist[listlen], &item, sizeof(FilelistItem));
			listlen++;
		}
	}
	pverbose("%d item candidates found\n", listlen);

	// If the rotating file exist
	if (filelist != NULL)
	{
		// stage 2 : orderer list to avoid name conflict
		qsort(filelist, listlen, sizeof (FilelistItem), cmpFileListReverse);
	}

	// stage 3 : doing action
	for (i = 0; i < listlen; i++)
	{
		xcheck(filelist[i].old != NULL, "Bug");
		if (filelist[i].new == NULL)
		{
			pverbose("Remove '%s'\n", filelist[i].old);
			xcheck((r = removeRec(filelist[i].old)) >= 0, "Fail to remove '%s'", filelist[i].old);
		}
		else
		{
			pverbose("Rename '%s' to '%s'\n", filelist[i].old, filelist[i].new);
			xcheck((r = rename(filelist[i].old, filelist[i].new)) >= 0,
					"Fail to rename '%s' to '%s'", filelist[i].old, filelist[i].new);
		}
	}

	// exit
	r = 0;
	bye:
	if (d) closedir(d);
	for (i = 0; i < listlen; i++)
	{
		if (filelist[i].old) free (filelist[i].old);
		if (filelist[i].new) free (filelist[i].new);
	}
	if (filelist) free (filelist);
	if (pathref) free(pathref);
	return r;
}

void bkFree(BackupArr * bkl)
{
	if (!bkl) return;

	while (bkl->size > 0)
	{
		--bkl->size;
		if (bkl->a[bkl->size].src) free(bkl->a[bkl->size].src);
		if (bkl->a[bkl->size].dst) free(bkl->a[bkl->size].dst);
		if (bkl->a[bkl->size].name) free(bkl->a[bkl->size].name);
		if (bkl->a[bkl->size].method) free(bkl->a[bkl->size].method);
		if (bkl->a[bkl->size].check) free(bkl->a[bkl->size].check);
	}
	free (bkl->a);
	free (bkl);
}

#ifndef BAKUPER_H
#define BAKUPER_H

typedef struct BackupItem BackupItem;
struct BackupItem
{
	char * src;
	char * dst;
	char * name;
	unsigned int nb;
	char * method;
	char * check;
};

typedef struct BackupArr BackupArr;
struct BackupArr
{
	BackupItem * a;
	unsigned int size;
};

BackupArr * bkGenerateList(const char * configFile, char separator);
int bkDoBackup(const BackupArr *);
int rotateBackup (const char* dirname, const char *pathref, unsigned int max);
void bkFree(BackupArr * bkList);

#endif // BAKUPER_H

#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include "backuper.h"
#include "utils.h"

int main(int argc, char *argv[])
{
	BackupArr * bkList = NULL;
	int r;
	const char *configFile = NULL;
	char sep = ';';
	static struct option long_options[] =
	{
		{"config",    required_argument, NULL, 'c'},
		{"help",      no_argument,       NULL, 'h'},
		{"no-header", no_argument,       NULL, 'n'},
		{"separator", required_argument, NULL, 's'},
		{"verbose",   no_argument,       NULL, 'v'},
		{"version",   no_argument,       NULL, 'V'},
		{NULL,        0,                 NULL, '\0'}
	};

	while ((r = getopt_long(argc, argv, "hc:s:vV", long_options, NULL)) > 0)
	{
		switch (r)
		{
			case 'c':
				configFile = optarg;
				break;
			case 'h':
				printf("usage %s: -c <configFile> ...\n", argv[0]);
				printf("Will backup repository with the csv config file, according to modification date\n");
				printf("Attempt configFile in csv format with\n");
				printf("  source (dir/file);destination (dir);backup_name;nb_backup;compression (copy,tar,gzip,bzip2,zip,svntargz);check_command;\n");
				printf("Options:\n");
				printf("  -c, --config : the configuration file\n");
				printf("  -h, --help : this help\n");
				printf("  -n, --no-header : if configuration file have not header row\n");
				printf("  -s, --separator : csv separator (default ';')\n");
				printf("  -v, --verbose : verbose mode\n");
				printf("  -V, --version : show version and exit\n");
				g_flags |= FLAGS_EXIT;
				break;
			case 'n':
				g_flags |= FLAGS_NOHEADER;
				break;
			case 's':
				if (optarg) sep = *optarg;
				break;
			case 'v':
				g_flags |= FLAGS_VERBOSE;
				break;
			case 'V':
				printf("%s Version 0.1\n", argv[0]);
				g_flags |= FLAGS_EXIT;
				break;
			default: /* '?' */
				fprintf(stderr, "Warning : unknow option '%s'\n", argv[(optind-1) ? optind-1: optind]);
				break;
		}
	}

	r = EXIT_FAILURE;
	if (g_flags & FLAGS_EXIT)
	{
		// do nothing
	}
	else if (!configFile)
	{
		fprintf(stderr, "No configuration file. try --help\n");
	}
	else if ((bkList = bkGenerateList(configFile, sep)) == NULL)
	{
		fprintf(stderr, "Error while parsing config file '%s'\n", configFile);
	}
	else if (bkDoBackup(bkList) < 0)
	{
		fprintf(stderr, "Error when doing backup\n");
	}
	else
	{
		r = EXIT_SUCCESS;
	}

	if (bkList)
	{
		bkFree(bkList);
		bkList = NULL;
	}
	return r;
}


# Backuper
This software will backup your file.
It manage compression and rotating backup

# Requirement
Unix like system with build essential

# Compilation
Simply do 'make'

And clean with 'make clean' or 'make mrproper'

# Use
You need a configuration file in csv format to launch the program.
Do bin/backuper -c config.csv and enjoy

The backup are done only if source is more recent than destination.
If number of backup is greater or equal to 1, the program will rotate backups,
allowing you to keep an history

If any unxepected event occurs, the stacktrace will be printed in stderr and
program will return non zero value.

# Configuration file
You can generate the configuration file with the following format
    Source (dir/file);Destination (dir);Backup_name;Nb_backup;Compression (copy,tar,gzip,bzip2,zip,svntargz);checkcmd;'
With
Source: the source file or directory to backup
Destination: the destination directory
Backup name: An optional name to give to the backup
Nb_backup: The number of rotated backup to keep
Compression: The compression method to use. It use external command.
CheckCmd: use to check repository validity before backuping it up.

# Example of configuration file
    Source (dir/file);Destination (dir);Backup_name;Nb_backup;Compression (copy,tar,gzip,bzip2,zip);checkcmd;
    /srv/nfs/Photos/;/srv/backup/Photos/;;2;copy;true;
    /srv/nfs/Codes/;/srv/backup/Codes/;mysrc;2;gzip;true;

# Example of script generating configuration file
    #!/bin/bash
    SRV=/srv
    DST=/srv/backup
    for i in $SRV/nfs/*
    do
    	if test -d "$i"
    	then
    		echo "$j;$DST/nfs/`basename "$i"`/$(basename "$j");;2;copy;true;"
    	fi
    done
    # Do hot copy of svn and targz it
    for i in $SRV/subversion/*
    do
    	echo "$i;$DST/subversion/$i;`basename $i`;3;svntargz;svnadmin verify;"
    done

# Usage
usage bin/backuper: -c <configFile> ...
Will backup repository with the csv config file, according to modification date
Attempt configFile in csv format with
  source (dir/file);destination (dir);backup_name;nb_backup;compression (copy,tar,gzip,bzip2,zip,svntargz);check_command;
Options:
  -c, --config : the configuration file
  -h, --help : this help
  -n, --no-header : if configuration file have not header row
  -s, --separator : csv separator (default ';')
  -v, --verbose : verbose mode
  -V, --version : show version and exit

#!/bin/sh
# generate test in this directory
cd $(dirname $0)

SRC=src
DST=dst
BIN=../bin/backuper

rm -Rf "$SRC"
rm -Rf "$DST"
rm configuration

#a : new file
#b : new file, sub tree
#c : file exists in backup

# type
# File (cp | bzip2 | gzip | zip)
# Directory (cp | bzip2 | gzip | zip)
# Usecase
# No backup file
# backup file older than file
# backup file older + rotate (subdir)
# backup file newer than file
# backup file ro
# no input file

# DESTINATION
mkdir -p $DST
mkdir -p $DST/nowrite
mkdir -p $DST/shouldnotsave.000/subE
echo $DST > $DST/rotateandlose.000.gz
echo $DST > $DST/rotateandlose.001.gz
echo $DST > $DST/rotateandlose.002.gz
echo $DST > $DST/nowrite/dstnotwritable.000.zip; chmod ugo-w $DST/nowrite
echo $DST > $DST/isfile
echo $DST > $DST/shouldnotsave.000/subE/b

# SOURCE
mkdir -p $SRC
mkdir -p $SRC/shouldbzip2dir/subA
mkdir -p $SRC/shouldgzipdir/subB
mkdir -p $SRC/shouldzipdir/subC
mkdir -p $SRC/shouldcopydir/subD
mkdir -p $SRC/shouldnotsave/subE
echo $SRC > $SRC/cponly
echo $SRC > $SRC/shouldbzip2dir/subA/a
echo $SRC > $SRC/shouldgzipdir/subB/a
echo $SRC > $SRC/shouldzipdir/subC/a
echo $SRC > $SRC/shouldcopydir/subD/a
echo $SRC > $SRC/shouldnotsave/subE/b
echo $SRC > $SRC/rotateandlose
echo $SRC > $SRC/notreadable; chmod ugo-r $SRC/notreadable
echo $SRC > $SRC/dstnotwritable
echo $SRC > $SRC/checkfail

sleep 1; touch $DST/shouldnotsave.000/subE/b

# CONFIGURATION
cat - > configuration << EOF
Source;Destination;Name;Nb backup;Compression;Check command
$SRC/shouldbzip2dir/;$DST/createme/;;2;bzip2;;
$SRC/shouldgzipdir/;$DST/;thegzipfile;1;gzip;;
$SRC/shouldgzipdir/;$DST/isfile;thesecondgzipfile;1;gzip;;
$SRC/shouldzipdir/;$DST/;;1;zip;;
$SRC/shouldcopydir;$DST;;1;copy;;
$SRC/shouldnotsave/;$DST/;;1;copy;true;
$SRC/rotateandlose;$DST/;;2;gzip;;
$SRC/notreadable;$DST/;;1;bzip2;;
$SRC/dstnotwritable;$DST/nowrite;;2;bzip2;;
$SRC/checkfail;$DST/;;1;zip;false;
X;syntax error;X;
EOF

# RUN
valgrind --leak-check=full $BIN -c configuration -s\; -v

# ASSERTS
test -f $DST/createme/shouldbzip2dir.000.tar.bz2   || echo FAIL00: "$SRC/shouldbzip2dir/;$DST/createme/;;1;bzip2;"
test -f $DST/thegzipfile.000.tar.gz                || echo FAIL01: "$SRC/shouldgzipdir/;$DST/;thegzipfile;2;gzip;"
test "$(cat $DST/isfile)" = "$DST"                 || echo FAIL02: "$SRC/shouldgzipdir/;$DST/isfile;thesecondgzipfile;1;gzip;"
test ! -f $DST/thesecondgzipfile.000.tar.gz        || echo FAIL03: "$SRC/shouldgzipdir/;$DST/isfile;thesecondgzipfile;1;gzip;"
test -f $DST/shouldzipdir.000.zip                  || echo FAIL04: "$SRC/shouldzipdir/;$DST/;;1;zip;"
test "$(cat $DST/shouldcopydir.000/subD/a)" = $SRC || echo FAIL05: "$SRC/shouldcopydir;$DST;;1;cp;"
test "$(cat $DST/shouldnotsave.000/subE/b)" = $DST || echo FAIL06: "$SRC/shouldnotsave/;$DST/;;1;cp;"
test "$(ls $DST/rotateandlose* | wc -l)" -eq 3     || echo FAIL07: "$SRC/rotateandlose;$DST/;;2;gzip;"
test "$(ls $DST/notreadable* | wc -l)" -eq 0       || echo FAIL08: "$SRC/notreadable;$DST/;;1;bzip2;"
test "$(ls $DST/nowrite/* | wc -l)" -eq 1          || echo FAIL09: "$SRC/dstnotwritable;$DST/nowrite;;1;bzip2;"
test ! -f $DST/checkfail.000.zip                   || echo FAIL10: "$SRC/checkfail;$DST/;;1;zip;false;"

# CLEAN
chmod -R u+w $DST/nowrite
rm -Rf "$SRC"
rm -Rf "$DST"
rm configuration

exit 0

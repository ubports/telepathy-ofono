#!/bin/sh

if [ $# -lt 3 ]; then
    echo "Usage: $0 <source directory> <target file> <version info file>"
fi

SOURCE_DIR=$1
TARGET_FILE=$2
VERSION_FILE=$3

VERSION="1"
LATEST_VERSION="1"

TMPFILE=`tempfile`

SCHEMA_FILE="$SOURCE_DIR/v${VERSION}.sql"
while [ -e $SCHEMA_FILE ]; do
    cat $SCHEMA_FILE >> $TMPFILE
    LATEST_VERSION=$VERSION
    VERSION=$(($VERSION+1))
    SCHEMA_FILE="$SOURCE_DIR/v${VERSION}.sql"
done

sqlite3 -init $TMPFILE :memory: .schema > $TARGET_FILE
echo $LATEST_VERSION > $VERSION_FILE

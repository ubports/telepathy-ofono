#!/bin/sh
QRC_FILE=$1
FILES=`ls schema/*.sql schema/*.info`

# clear the file
if [ -e $QRC_FILE ]; then
    rm -f $QRC_FILE
fi

# and print the contents
echo '<RCC>' >> $QRC_FILE
echo '    <qresource prefix="/database">' >> $QRC_FILE

for file in $FILES; do
    echo "        <file>$file</file>" >> $QRC_FILE
done

echo '    </qresource>' >> $QRC_FILE
echo '</RCC>' >> $QRC_FILE

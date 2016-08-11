fname=$(basename $6)
basename=${fname%.*}

echo "$7/$basename.d : $6" >> $7/makefile.d
echo "	sh ./depend.generic.sh \"$1\" \"$2\" \"$3\" \"$4\" \"$5\" \"$6\" \"$7\" \"$8\"" >> $7/makefile.d
echo "" >> $7/makefile.d

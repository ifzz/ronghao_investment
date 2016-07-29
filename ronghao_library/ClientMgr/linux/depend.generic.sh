fname=$(basename $6)
basename=${fname%.*}
extention=${fname##*.}


debug=
debug_def=

if [ $8 = "debug" ];then
	debug="-g"
	debug_def="\$(PROJECT_DEF_DEBUG)"
else
	debug="-O3"
	debug_def="\$(PROJECT_DEF_RELEASE)"
fi

cc="PROJECT_CXX"
cc_flag="PROJECT_CXX_FLAGS"
data=

if [ $extention = "c" ];then
	cc_flag="PROJECT_CC_FLAGS"
	cc="PROJECT_CC"
	debug_def="\$(PROJECT_DEF_DEBUG)"
	data=`$1 $3 -MM $6`
else

	data=`$2 $4 -MM $6`
fi


if [ -z "$data" ];then
	echo "依赖文件检查错误 for $6"
else
	echo "$7/$data"   > $7/$basename.d
	echo "	\$($cc) \$($cc_flag) $debug_def $debug -c \$< -o \$@" >> $7/$basename.d
fi

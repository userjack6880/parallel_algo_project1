#!/bin/bash

for RUN in {1..5}
do
	FILES=$(ls data/${1}/run${RUN}/result*)
	for FILE in ${FILES[@]}
	do 
		LINENUM=1
		while IFS= read -r LINE
		do
			if [[ "$LINENUM" == 1 ]]; then
				PROC=$(cat $FILE | sed -n -e 's/^running on \([[:digit:]]\+\) processors/\1/p')
			fi
			if [[ "$LINENUM" == 2 ]]; then
				INIT_PKT=$(cat $FILE | sed -n -e 's/^packet size \([[:digit:]]\+\)/\1/p')
			fi
			if [[ "$LINENUM" == 3 ]]; then
				INCR=$(cat $FILE | sed -n -e 's/^increase packet: \([[:digit:]]\)/\1/p')
			fi
			if [[ "$LINENUM" == 5 ]]; then
				MAX_PKT=$(cat $FILE | sed -n -e 's/^.* maximum packet size of \([[:digit:]]\+\).*/\1/p')
			fi
			if [[ "$LINENUM" == 7 ]]; then
				EXEC_TIME=$(cat $FILE | sed -n -e 's/^execution time = \([[:digit:]]\+\.[[:digit:]]\+\).*/\1/p')
			fi
			((LINENUM++))
		done < $FILE
		echo "$RUN,$PROC,$INIT_PKT,$MAX_PKT,$INCR,$EXEC_TIME"
	done
done

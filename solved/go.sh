for t in {4..5}
do
	for i in {0..1}
	do
		for p in {1..5}
		do
			python solve.py $t $p ./p$p/tc$t\_$i
		done
	done
done
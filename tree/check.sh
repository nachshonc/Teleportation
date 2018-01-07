PRM=$1
for t in 1 2 4 8 12 16 20 24; do 
	for pgm in tree-tsx tree-hoh; do 
		for i in {1..3}; do 
			./${pgm} -t $t -n 1000000 $PRM -m 20 -i5000 | grep time | cut -d':' -f2; 
		done | sort | awk 'NR==2'; 
	done | xargs | awk -v t=$t '{printf "%d\t%.1f\t%.1f\n", t,  $1, $2}'; 
done


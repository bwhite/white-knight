mkdir ./results
rm -r ./output
for i in `ls dats/*.conf | cut -d / -f 2`; do
	echo $i
	#mkdir ./output/
	./pso dats/$i
	mkdir ./results/$i
	mv *.log ./results/$i/
done

OUTPUT=/mnt/newback
rm -r ./output
mkdir $OUTPUT
#NOTE That we are only using the prefix VACE!! < ------
for i in `ls dats/Movie*.dat | cut -d / -f 2`; do
	echo $i
	#sh cleandirs.sh
	mkdir output
	mkdir -p output/box output/forcebgmask output/join output/fgedge output/blur output/finaldiff output/current output/diff output/chips
	#./bgSub dats/$i
	python pyknight.py dats/$i
	#python pyknight.py dats/$i &> output/out.txt
	#mencoder "mf://output/box/*.jpg" -mf fps=60 -o output/$i.avi -ovc lavc -lavcopts vcodec=mpeg4
	rm -r $OUTPUT/$i
	mkdir $OUTPUT/$i
	mv ./output $OUTPUT/$i/
done

cha=$1
chb=$2

convert $cha -resize 909x -colorspace gray -depth 8 -compress none cha.pgm
convert $chb -resize 909x -colorspace gray -depth 8 -compress none chb.pgm

./apt-encode cha.pgm chab.pgm | sox -t raw -b 8 -e unsigned -c 1 -r 8320 - -r 8320 apt.wav

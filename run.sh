ch1=$1
ch2=$2
ch3=$3

convert $ch1 -resize 909x -colorspace gray -depth 8 -compress none ch1.pgm
convert $ch2 -resize 909x -colorspace gray -depth 8 -compress none ch2.pgm

./apt-encode ch1.pgm ch2.pgm | sox -t raw -b 8 -e unsigned -c 1 -r 12480 - -b 16 -e signed -r 48000 apt.wav

noaa-apt apt.wav

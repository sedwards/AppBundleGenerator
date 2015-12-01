#!/bin/bash

# Copy this script in to the directory that contains your SVG files
# to generate ICNS icons 

for i in $(ls *.svg | awk -F "." '{print $1}' )
do
   echo $i
   mkdir $i.iconset

   qlmanage -t -s 1024 -o $i.iconset $i.svg

    sips -z 16 16 $i.iconset/$i.svg.png --out "$i.iconset/icon_16x16.png"
    sips -z 32 32 $i.iconset/$i.svg.png --out "$i.iconset/icon_16x16@2x.png"
    cp "$i.iconset/icon_16x16@2x.png" "$i.iconset/icon_32x32.png"

    sips -z 64 64 $i.iconset/$i.svg.png --out "$i.iconset/icon_32x32@2x.png"
    sips -z 128 128 $i.iconset/$i.svg.png --out "$i.iconset/icon_128x128.png"
    sips -z 256 256 $i.iconset/$i.svg.png --out "$i.iconset/icon_128x128@2x.png"
    cp "$i.iconset/icon_128x128@2x.png" "$i.iconset/icon_256x256.png"

    sips -z 512 512 $i.iconset/$i.svg.png --out "$i.iconset/icon_256x256@2x.png"
    cp "$i.iconset/icon_256x256@2x.png" "$i.iconset/icon_512x512.png"

    sips -z 1024 1024 $i.iconset/$i.svg.png --out "$i.iconset/icon_512x512@2x.png"

    iconutil -c icns $i.iconset
    rm -fr $i.iconset
done



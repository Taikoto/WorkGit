
#!/bin/bash
LOC_DIR=$(cd `dirname $0` ; pwd)

if [ ! -n "$1" ] ;then
    echo "you have not input a word!"
	exit;
else
    echo "the word you input is $1"
fi

echo $LOC_DIR/$1
TARGET_DIR=$1
DIR=$LOC_DIR/$TARGET_DIR

# change dir and file name
find $LOC_DIR/$TARGET_DIR -name 'max9296_mipi_yuv' | xargs -i echo mv \"{}\" \"{}\" | sed 's/max9296_mipi_yuv/tw9992_mipi_yuv/2g' | sh
find $LOC_DIR/$TARGET_DIR -name 'max9296_mipi_yuv*' | xargs -i echo mv \"{}\" \"{}\" | sed 's/max9296_mipi_yuv/tw9992_mipi_yuv/2g' | sh
find $LOC_DIR/$TARGET_DIR -name 'max9296mipiyuv' | xargs -i echo mv \"{}\" \"{}\" | sed 's/max9296mipiyuv/tw9992mipiyuv/2g' | sh
find $LOC_DIR/$TARGET_DIR -type f -name "max9296mipiyuv*" | xargs -i echo mv \"{}\" \"{}\" | sed 's/max9296mipiyuv/tw9992mipiyuv/2g' | sh
find $LOC_DIR/$TARGET_DIR -type f -name "*max9296mipiyuv.h" | xargs -i echo mv \"{}\" \"{}\" | sed 's/max9296mipiyuv/tw9992mipiyuv/2g' | sh

find $LOC_DIR/$TARGET_DIR -type f -name "*max9296mipiyuv.cpp" | xargs -i echo mv \"{}\" \"{}\" | sed 's/max9296mipiyuv/tw9992mipiyuv/2g' | sh
find $LOC_DIR/$TARGET_DIR -type f -name "*max9296mipiyuv.xlsx" | xargs -i echo mv \"{}\" \"{}\" | sed 's/max9296mipiyuv/tw9992mipiyuv/2g' | sh

# replace str
sed -i "s/max9296/tw9992/g" `grep max9296 -rl $DIR`
sed -i "s/MAX9296/TW9992/g" `grep MAX9296 -rl $DIR`


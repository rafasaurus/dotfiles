#!/bin/sh
if [[ $1 == "" ]] #Where "$1" is the positional argument you want to validate 

then
    echo "no arguments, path to the picture expected"
    exit 0

fi

extension="${1##*.}"
if [ "$extension" != "jpeg" ] && [ "$extension" != "jpg" ]; then
    echo "[Warning] sxiv previews not working for non jpeg/jpg formats"
fi

PACK=$(echo -e "$(echo "new old" | tr ' ' '\n')" |  dmenu -i -p "Choose lut pack" -l 15)
if [ "$PACK" == "new" ]; then
    LPATH="/home/rafael/.local/bin/luts/"
else
    LPATH="/home/rafael/.local/bin/512x512/"
fi

ALL_LUTS="$(ls $LPATH)"
# choose dialog
LUTS="`echo $ALL_LUTS | tr ' ' '\n'`"
LUTS=$(echo -e "$LUTS" |  dmenu -i -p "Choose screen option" -l 15)

CONVERT=/usr/bin/convert
FL="${1%.*}"
echo $LUTS
if [ "$LUTS" == "all" ]; then
    # create new folder, and put all edited images into newly created folder
    foldername=${1%.*}
    mkdir $foldername && echo "creating directory $imagename"
    LUTS=$ALL_LUTS
    echo $LUTS
    for LUT in $LUTS
    do
        FLLUT="${LUT%.*}"
        $CONVERT $1 ${LPATH}/${LUT} -hald-clut $foldername/${FL}-${FLLUT}.jpg && echo "done $LUT"
    done
    exit 0
fi

for LUT in $LUTS
do
    FLLUT="${LUT%.*}"
    orig=${FL}_orig.${extension}
    [ -f $orig ] || cp "$1" $orig
    $CONVERT $orig ${LPATH}/${LUT} -hald-clut $1
    exiftool -delete_original -Exif:ImageDescription="${FLLUT}" -Description="${FLLUT}" "$1" 
    # ffmpeg -i $1 -i "${LPATH}/${LUT}" -filter_complex "haldclut" "result.mp4"
    echo "done $LUT"
done

exit 0

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
    ALL_LUTS="dehancer-adox-impolsion-kodak-paper.png dehancer-adox-impolsion.png dehancer-agfa-100.png dehancer-ektar-25-exp-1996+k2383.png dehancer-fujichrome-velvia-100.png dehancer-fujichrome-velvia-50+1ev.png dehancer-fujichrome-velvia-50-1ev.png dehancer-fujichrome-velvia-50-k2383.png dehancer-fuji-eterna-vivid-500-exp-2013-f3513.png dehancer-fuji-eterna-vivid-500-expired-2013-k2383.png dehancer-fuji-eterna-vivid-500.png dehancer-fuji-fp100c-digital-intermediate.png dehancer-fuji-fp100c.png dehancer-fuji-instax-k2383.png dehancer-fuji-reala-500d-exp-2013-k2383.png dehancer-kodachrome-64+3ev.png dehancer-kodachrome-64-3ev.png dehancer-kodachrome-64.png dehancer-kodak-aerocolor-IV-125.png dehancer-kodak-eastman-double-X-5222.png dehancer-kodak-ektar-25-exp-1996+endura.png dehancer-kodak-gold-200.png dehancer-kodak-portra-400+endura.png dehancer-kodak-supra-100+endura.png dehancer-kodak-supra-100+k2383.png dehancer-kodak-vision-3-500t+k2383.png dehancer-konica-minolta-vx400-super.png dehancer-lomochrome-metropolis-xr-100-400.png dehancer-lomochrome-purple.png dehancer-orwo-chrom-ut21-daylight-exp-1992.png dehancer-polachrome-35mm-exp-1986.png dehancer-prokudin-gorskiy-1906.png dehancer-rollei-ortho-25.png dehancer-sverna-type-42-exp-1991.png dehancer-xp2-super400.png"
else
    LPATH="/home/rafael/.local/bin/512x512/"
    ALL_LUTS="dehancer-fuji-astia-100f.png dehancer-fuji-astia-100.png dehancer-fuji-c200.png dehancer-fuji-eterna-vivid-500.png dehancer-fuji-fp100c.png dehancer-fuji-industrial-100.png dehancer-fuji-industrial-400.png dehancer-fuji-natura-1600.png dehancer-fuji-pro-400h.png dehancer-fuji-provia-100f.png dehancer-fuji-provia-400x.png dehancer-fuji-sensia-400.png dehancer-fuji-superia-1600.png dehancer-fuji-superia-200.png dehancer-fuji-velvia-100.png dehancer-fuji-velvia-50-new-new.png dehancer-fuji-velvia-50-new.png dehancer-fuji-velvia-50.png dehancer-ilford-hp5plus-400.png dehancer-ilford-xp2super-400.png dehancer-kodak-ektar-100.png dehancer-orwo-ut18-exp91.png dehancer-orwo-ut21-exp92.png vsco-fuji-pro-160c.png vsco-fuji-pro-160s.png vsco-fuji-pro-400h.png vsco-fuji-pro-800z.png vsco-fuji-pro-800z-new.png vsco-fuji-provia-400x.png vsco-fuji-superia-100.png vsco-fuji-t64.png vsco-fuji-velvia-50.png vsco-kodachrome25.png vsco-kodachrome25-new.png vsco-kodak-portra-160.png vsco-kodak-portra-400.png dehancer-adox-color-implosion.png dehancer-adox-color-implosion-grain.png"
fi

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

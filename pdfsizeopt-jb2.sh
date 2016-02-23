#!/bin/bash

SOURCE="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"

# DPI for text
DT=300
# DPI for pictures
DI=150

extract() {
  FILE="$1"

  DIR=`mktemp -d "./${FILE%.*}.XXXX"`

  PNG="$DIR/${FILE%.*}-%04d.png"
  convert -units PixelsPerInch -density $DT "$FILE" "$PNG" || exit 1
  echo "$DIR"
}

force() {
  DIR="$1"

  cd "$DIR" || exit 1

  declare -a files
  files=(*.png)

  last=${#files[@]}
  let last=last-1

  for (( i=0; i<=${last}; i++ )); do
    if [ "$KEEPFIRST" == "YES" ] && [ $i -eq 0 ]; then
      continue
    fi
    if [ "$KEEPLAST" == "YES" ] && [ $i -eq $last ]; then
      continue
    fi
    f=${files[$i]}
    [ "$VERBOSE" == "YES" ] && echo "monochrome: $f"
    convert "$f" -monochrome "$f" || exit 1
  done
}

process() {
  DIR="$1"

  cd "$DIR" || exit 1

  COUNTER=0
  for f in *.png; do
    [ "$VERBOSE" == "YES" ] && echo "index: $f"
    TYPE=`file "$f"`
    if [[ "$TYPE" =~ "1-bit" ]]; then
      JB2="${f%.*}.jb2"
      mv "$f" "$JB2" || exit 1
      printf "J.%04d\n" $COUNTER >> index
      let COUNTER=COUNTER+1
    else
      JPX="${f%.*}.jpg"
      convert -units PixelsPerInch -density $DT "$f" -resample $DIx$DI -density $DI "$JPX" || exit 1
      rm "$f" || exit 1
      echo ${JPX} >> index
    fi
  done

  [ "$VERBOSE" == "YES" ] && V="-v"

  jbig2 $V -b J -d -p -s *.jb2 || exit 1
  rm *.jb2 || exit 1
}

compile() {
  DIR="$1"
  OUT="$2"

  [ "$VERBOSE" == "YES" ] && echo "compile: $OUT"

  (cd "$DIR" && $SOURCE/pdf.py index) > "$OUT" || exit 1
}

usage() {
  echo "Usage $0: large.pdf"
  echo ""
  echo "Steps:"
  echo -e "\t-e|--extract\tProduce 'extract' pdf images to directory and exit"
  echo -e "\t-p|--process\tProduce 'process' identify images and create jb2 data and exit"
  echo -e "\t-c|--compile\tProduce 'compile' step. Create pdf from directory"
  echo -e "\t-m|--monochrome\tProduce 'monochrome' step. Monochrome all images in target directory"
  echo -e "\t\t-kf|--keepfirst\tDo not monochrome first image in list (usually cover jpg)"
  echo -e "\t\t-kl|--keeplast\tDo not monochrome last image in list (usually cover jpg)"
  echo ""
  echo "Options:"
  echo -e "\t-dt|--density_text\tSet DPI for text contaned images (300)"
  echo -e "\t-di|--density_image\tSet DPI for image contaned images (150)"
  echo -e "\t-f|--force\tForce convert images to monochrome"
  echo -e "\t\t-kf|--keepfirst\tDo not monochrome first image in list (usually cover jpg)"
  echo -e "\t\t-kl|--keeplast\tDo not monochrome last image in list (usually cover jpg)"
}

packages() {
  for p in "$@"; do
    which "$p" > /dev/null 2>&1 || (echo "Package $p not found..." && exit 1)
  done
}

packages convert jbig2 || exit 1

while [[ $# > 0 ]]; do
key="$1"

case $key in
    -e|--extract)
    EXTRACT=YES
    ;;
    -p|--process)
    PROCESS=YES
    ;;
    -c|--compile)
    COMPILE=YES
    ;;
    -f|--force)
    FORCE=YES
    ;;
    -m|--monochrome)
    MONOCHROME=YES
    ;;
    -kf|--keepfirst)
    KEEPFIRST=YES
    ;;
    -kl|--keeplast)
    KEEPLAST=YES
    ;;
    -v|--verbose)
    VERBOSE=YES
    ;;
    -dt|--density_text)
    DT="$2"
    shift # past argument
    ;;
    -di|--density_image)
    DI="$2"
    shift # past argument
    ;;
    -h|--help)
    usage
    exit 0
    ;;
    *)
    break
    ;;
esac
shift # past argument or value
done

if [ "$EXTRACT" == "YES" ];then
  extract "$@" || exit 1
  exit 0
fi

if [ "$PROCESS" == "YES" ];then
  process "$@" || exit 1
  exit 0
fi

if [ "$COMPILE" == "YES" ];then
  compile "$@" || exit 1
  exit 0
fi

if [ "$MONOCHROME" == "YES" ]; then
  (force "$@") || exit 1
  exit 0
fi

if [ $# -eq 0 ]; then
  usage
  exit 0
fi

for f in "$@"; do
  if [ ! -f "$f" ]; then
    echo "file not found: $f"
    exit 1
  fi
  [ "$VERBOSE" == "YES" ] && echo "extract: $f"
  DIR=`extract "$f"` || exit 1
  if [ "$FORCE" == "YES" ]; then
    (force "$DIR") || exit 1 
  fi
  (process "$DIR") || exit 1
  (compile "$DIR" "$f") || exit 1
  rm -rf "$DIR" || exit 1
done

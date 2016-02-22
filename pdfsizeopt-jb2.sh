#!/bin/bash

SOURCE="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"

# DPI for text
DT=300
# DPI for pictures
DI=150

extract() {
  FILE=$1
  
  DIR=`mktemp -d "./${FILE%.*}.XXXX"`

  PNG="$DIR/${FILE%.*}-%04d.png"
  convert -units PixelsPerInch -density $DT "$FILE" "$PNG" || exit 1
  echo $DIR
}

process() {
  DIR="$1"

  cd "$DIR" || exit 1

  COUNTER=0
  for f in *.png; do
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

  jbig2 -v -b J -d -p -s *.jb2 || exit 1
  rm *.jb2 || exit 1
}

compile() {
  DIR="$1"
  OUT="$2"

  (cd "$DIR" && $SOURCE/pdf.py index) > "$OUT" || exit 1
}

usage() {
  echo "Usage $0: large.pdf"
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
    -a|--aaa)
    AAA="$2"
    shift # past argument
    ;;
    --default)
    DEFAULT=YES
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
  extract "$@"
  exit 0
fi

if [ "$PROCESS" == "YES" ];then
  process "$@"
  exit 0
fi

if [ "$COMPILE" == "YES" ];then
  compile "$@"
  exit 0
fi

for f in "$@"; do
  if [ ! -e "$f" ]; then
    echo "file not found: $f"
    exit 1
  fi
  DIR=`extract "$f"` || exit 1
  (process "$DIR") || exit 1
  (compile "$DIR" "$f") || exit 1
  rm -rf "$DIR" || exit 1
done

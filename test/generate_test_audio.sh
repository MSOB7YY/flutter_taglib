#!/bin/bash

# Exit immediately if a command exits with a non-zero status
set -e

# Output directory defaults to the assets directory next to the script
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
OUT_DIR="${1:-$SCRIPT_DIR/assets}"
BASE_NAME="01 TempleOS Hymn Risen (Remix)"
COVER_FILE="$OUT_DIR/cover.jpg"
TEMP_WAV="$OUT_DIR/temp_sine.wav"

echo "Using output directory: $OUT_DIR"
mkdir -p "$OUT_DIR"

# Check if cover.jpg exists, or print a warning
if [ -f "$COVER_FILE" ]; then
  echo "Found cover image: $COVER_FILE"
else
  echo "Warning: cover.jpg not found in $OUT_DIR. Files will be generated without cover art."
fi

echo "Generating 3-second temporary stereo WAV file..."
# Generate a 3-second 44.1kHz stereo sine wave (440Hz)
ffmpeg -y -f lavfi -i "sine=frequency=440:duration=3" -acodec pcm_s16le -ar 44100 -ac 2 "$TEMP_WAV"

# Function to transcode/encode
encode_all() {
  # 1. WAV
  echo "Encoding WAV..."
  ffmpeg -y -i "$TEMP_WAV" -c:a copy \
    -metadata title="TempleOS Hymn Risen (Remix)" \
    -metadata artist="Terry A. Davis" \
    -metadata album="TempleOS Hymns" \
    -metadata track="1" \
    -metadata genre="Electronic" \
    -metadata date="2020" \
    "$OUT_DIR/$BASE_NAME.wav"

  # 2. AAC
  echo "Encoding AAC..."
  ffmpeg -y -i "$TEMP_WAV" -c:a aac -b:a 128k \
    -metadata title="TempleOS Hymn Risen (Remix)" \
    -metadata artist="Terry A. Davis" \
    -metadata album="TempleOS Hymns" \
    -metadata track="1" \
    -metadata genre="Electronic" \
    -metadata date="2020" \
    "$OUT_DIR/$BASE_NAME.aac"

  # 3. AIFF
  echo "Encoding AIFF..."
  ffmpeg -y -i "$TEMP_WAV" -c:a pcm_s16be \
    -metadata title="TempleOS Hymn Risen (Remix)" \
    -metadata artist="Terry A. Davis" \
    -metadata album="TempleOS Hymns" \
    -metadata track="1" \
    -metadata genre="Electronic" \
    -metadata date="2020" \
    "$OUT_DIR/$BASE_NAME.aiff"

  # 4. FLAC
  echo "Encoding FLAC..."
  if [ -f "$COVER_FILE" ]; then
    ffmpeg -y -i "$TEMP_WAV" -i "$COVER_FILE" -map 0:0 -map 1:0 -c:a flac -c:v copy -disposition:v attached_pic \
      -metadata title="TempleOS Hymn Risen (Remix)" \
      -metadata artist="Terry A. Davis" \
      -metadata album="TempleOS Hymns" \
      -metadata track="1" \
      -metadata genre="Electronic" \
      -metadata date="2020" \
      -metadata:s:v title="Album cover" -metadata:s:v comment="Cover (Front)" \
      "$OUT_DIR/$BASE_NAME.flac"
  else
    ffmpeg -y -i "$TEMP_WAV" -c:a flac \
      -metadata title="TempleOS Hymn Risen (Remix)" \
      -metadata artist="Terry A. Davis" \
      -metadata album="TempleOS Hymns" \
      -metadata track="1" \
      -metadata genre="Electronic" \
      -metadata date="2020" \
      "$OUT_DIR/$BASE_NAME.flac"
  fi

  # 5. M4A
  echo "Encoding M4A..."
  if [ -f "$COVER_FILE" ]; then
    ffmpeg -y -i "$TEMP_WAV" -i "$COVER_FILE" -map 0:0 -map 1:0 -c:a aac -b:a 128k -c:v copy -disposition:v attached_pic \
      -metadata title="TempleOS Hymn Risen (Remix)" \
      -metadata artist="Terry A. Davis" \
      -metadata album="TempleOS Hymns" \
      -metadata track="1" \
      -metadata genre="Electronic" \
      -metadata date="2020" \
      "$OUT_DIR/$BASE_NAME.m4a"
  else
    ffmpeg -y -i "$TEMP_WAV" -c:a aac -b:a 128k \
      -metadata title="TempleOS Hymn Risen (Remix)" \
      -metadata artist="Terry A. Davis" \
      -metadata album="TempleOS Hymns" \
      -metadata track="1" \
      -metadata genre="Electronic" \
      -metadata date="2020" \
      "$OUT_DIR/$BASE_NAME.m4a"
  fi

  # 6. MP3
  echo "Encoding MP3..."
  if [ -f "$COVER_FILE" ]; then
    ffmpeg -y -i "$TEMP_WAV" -i "$COVER_FILE" -map 0:0 -map 1:0 -c:a libmp3lame -b:a 128k -c:v copy -disposition:v attached_pic -id3v2_version 3 \
      -metadata title="TempleOS Hymn Risen (Remix)" \
      -metadata artist="Terry A. Davis" \
      -metadata album="TempleOS Hymns" \
      -metadata track="1" \
      -metadata genre="Electronic" \
      -metadata date="2020" \
      -metadata:s:v title="Album cover" -metadata:s:v comment="Cover (Front)" \
      "$OUT_DIR/$BASE_NAME.mp3"
  else
    ffmpeg -y -i "$TEMP_WAV" -c:a libmp3lame -b:a 128k -id3v2_version 3 \
      -metadata title="TempleOS Hymn Risen (Remix)" \
      -metadata artist="Terry A. Davis" \
      -metadata album="TempleOS Hymns" \
      -metadata track="1" \
      -metadata genre="Electronic" \
      -metadata date="2020" \
      "$OUT_DIR/$BASE_NAME.mp3"
  fi

  # 7. OGG
  echo "Encoding OGG..."
  # Ogg/Vorbis does not standardly support video stream copy/disposition for cover art in ffmpeg.
  # We write standard tag metadata.
  ffmpeg -y -i "$TEMP_WAV" -c:a vorbis -strict -2 -q:a 4 \
    -metadata title="TempleOS Hymn Risen (Remix)" \
    -metadata artist="Terry A. Davis" \
    -metadata album="TempleOS Hymns" \
    -metadata track="1" \
    -metadata genre="Electronic" \
    -metadata date="2020" \
    "$OUT_DIR/$BASE_NAME.ogg"

  # 8. OPUS
  echo "Encoding OPUS..."
  # Opus does not standardly support video stream copy/disposition for cover art in ffmpeg.
  # We write standard tag metadata.
  ffmpeg -y -i "$TEMP_WAV" -c:a libopus -b:a 96k \
    -metadata title="TempleOS Hymn Risen (Remix)" \
    -metadata artist="Terry A. Davis" \
    -metadata album="TempleOS Hymns" \
    -metadata track="1" \
    -metadata genre="Electronic" \
    -metadata date="2020" \
    "$OUT_DIR/$BASE_NAME.opus"

  # 9. APE
  echo "Encoding APE..."
  if command -v mac >/dev/null 2>&1; then
    mac "$TEMP_WAV" "$OUT_DIR/$BASE_NAME.ape" -c2000
    mac "$OUT_DIR/$BASE_NAME.ape" -t "Artist=Terry A. Davis|Album=TempleOS Hymns|Title=TempleOS Hymn Risen (Remix)|Track=1|Genre=Electronic|Year=2020"
  else
    echo "Warning: 'mac' tool not found. Skipping APE compression."
  fi

  # 10. MPC
  echo "Encoding MPC..."
  if command -v mpcenc >/dev/null 2>&1; then
    mpcenc --overwrite --artist "Terry A. Davis" --album "TempleOS Hymns" --tag "title=TempleOS Hymn Risen (Remix)" --tag "track=1" --tag "genre=Electronic" --tag "year=2020" "$TEMP_WAV" "$OUT_DIR/$BASE_NAME.mpc"
  else
    echo "Warning: 'mpcenc' tool not found. Skipping MPC compression."
  fi

  # 11. SPX
  echo "Encoding SPX..."
  if command -v speexenc >/dev/null 2>&1; then
    speexenc --title "TempleOS Hymn Risen (Remix)" --author "Terry A. Davis" --comment "album=TempleOS Hymns" --comment "track=1" --comment "genre=Electronic" --comment "year=2020" "$TEMP_WAV" "$OUT_DIR/$BASE_NAME.spx"
  else
    echo "Warning: 'speexenc' tool not found. Skipping Speex compression."
  fi

  # 12. WV
  echo "Encoding WV..."
  if command -v wavpack >/dev/null 2>&1; then
    if [ -f "$COVER_FILE" ]; then
      wavpack -y "$TEMP_WAV" -o "$OUT_DIR/$BASE_NAME.wv" \
        -w "Artist=Terry A. Davis" \
        -w "Album=TempleOS Hymns" \
        -w "Title=TempleOS Hymn Risen (Remix)" \
        -w "Track=1" \
        -w "Genre=Electronic" \
        -w "Year=2020" \
        --write-binary-tag "Cover Art (Front)=@$COVER_FILE"
    else
      wavpack -y "$TEMP_WAV" -o "$OUT_DIR/$BASE_NAME.wv" \
        -w "Artist=Terry A. Davis" \
        -w "Album=TempleOS Hymns" \
        -w "Title=TempleOS Hymn Risen (Remix)" \
        -w "Track=1" \
        -w "Genre=Electronic" \
        -w "Year=2020"
    fi
  else
    echo "Warning: 'wavpack' tool not found. Skipping WavPack compression."
  fi
}

encode_all

# Clean up
if [ -f "$TEMP_WAV" ]; then
  rm "$TEMP_WAV"
fi
echo "All audio assets generated successfully in $OUT_DIR!"

## 1.8.9
* feat: `TagLibFile.readAsfSharedUserRating` and `TagLibFile.setAsfSharedUserRating`, read and write the `WM/SharedUserRating` attribute (the wma/wmv rating windows and most players keep). `readBatchNative` returns it in `TagLibBatchResult.asfSharedUserRating`
* feat: `TagLibFile.getTagType`, the tag a format keeps its properties in (ID3v2, Xiph comment, MP4, APE, ASF, Matroska or tracker modules)

## 1.8.8
* feat: `TagLibFile.readPopularimeters` and `TagLibFile.setPopularimeters`, read and write ID3v2 POPM frames (the rating and play count most players keep in mp3, wav, aiff, dsf, dff and tta files). `readBatchNative` returns them in `TagLibBatchResult.popularimeters`

## 1.8.7
* fix: `readBatchNative` reads `TITLE` from the matroska segment title when there is no `TITLE` tag. It also reads `COMMENT` from the xiph `DESCRIPTION` field when there is no `COMMENT` tag. This matches TagLib's own `Tag::title()` and `Tag::comment()` and covers files tagged by ffmpeg

## 1.8.5
* fix: files whose extension doesn't match their content (m4a saved as `.mp3`, mp3 saved as `.wav`, webm saved as `.m4a`...) are read by their content instead of failing or reporting a zero or made up duration
* fix: files with ID3v2 tags in front (left by MP3-only taggers) keep those tags and take the audio properties from the format behind them

## 1.8.4
* fix(linux): libcurl is loaded at runtime instead of linked, the library loads without it and rpm packages no longer require debian's `CURL_OPENSSL_4` symbol version (uninstallable on fedora)

## 1.8.3
* perf: local files are read through a block cached stream with positional reads instead of stdio, android sdcards read ~20x less per file (bionic sized the stdio buffer from the 128 KB `st_blksize` and refilled it after every seek)

## 1.8.2
* feat: `TagLibFile.pictureOfType`, copies only the embedded picture of the given type

## 1.8.1
* build: utfcpp `4.2.0`

## 1.8.0
* feat: chapters, read with `readBatchNative(readChapters: true)` and written with `TagLibFile.setChapters`, from ID3v2 CHAP frames, MP4 Nero/QuickTime chapters, Matroska chapters and `CHAPTERxxx` comments
* build: TagLib `2.3.1-c3`, mp4 chapters are read from the already parsed atoms

## 1.7.0
* feat: read tags from http(s) urls with range requests (from upstream), also in `readBatchNative` with `headers`
* perf: http reads keep one connection per file and fetch consecutive missing blocks in one request

## 1.6.0
* feat: `TagLibFile.readBatchNative`, reads on a pool of native threads and streams results without spawning isolates

## 1.5.7
* fix(android): `UnsupportedError` on `armeabi-v7a` devices
* fix(android): apps with `minSdk` below 24 now build from source, since the published binaries import libc symbols older devices do not export

## 1.5.5
* core: improve tags extraction for wma/asf formats

## 1.5.4
* feat: add `TagLibFile.bitsPerSample`

## 1.5.3
* core: improve format names for matroska container

## 1.5.2
* fix: cover not extracted from mp4 in rare cases

## 1.5.1
* fix(android): remove dangerous JNI FindClass call in C++ taglib_bridge_open

## 1.5.0
* Supports TagLib AudioPropertiesStyle settings
* Added batch read functionality and multi-isolate support
* Metadata read performance optimization
* Support physical path SAF URI resolution and FD fallback
* Added performance benchmark test to the example

## 1.4.2
* Added `TagLibFile.format` to detect audio file formats (e.g. MP3, FLAC).
* Added `TagLibFile.isLossless` to check if an audio file is lossless.
* Improved `TagLibFile.coverData` for extracting cover art bytes.
* Updated prebuilt native binary download URLs.

## 1.4.1
* Eliminate analyze info prompt

## 1.4.0
* Song duration accurate to milliseconds.
* Deprecated unsupported interfaces.
* Android uses online pre-built artifacts

## 1.3.3
* Lower the Dart SDK requirements to 3.11.0

## 1.3.2
* Remove redundant logs
* Fixes the issue of Windows and Linux dynamic libraries not being downloaded during the build process.

## 1.3.1
* Update readme

## 1.3.0
* Using pre-built artifacts for taglibs in Windows, Linux, and Android
* Fixing Android permission issues

## 1.2.0
* Supports Android content://media format links

## 1.1.1
* Update document

## 1.1.0
* Support PropertyMap

## 1.0.3
* Support SPM

## 1.0.2
* Upgrade dependencies

## 1.0.1
* Adapted to pub.dev

## 1.0.0

* Initial release of flutter_taglib.

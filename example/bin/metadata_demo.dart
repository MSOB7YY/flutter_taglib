// ignore_for_file: avoid_print

import 'dart:io';
import 'package:flutter_taglib/flutter_taglib.dart';

void main() async {
  print('=== Flutter TagLib Metadata Read/Write Example ===\n');

  // Paths to test files relative to the example/ directory
  final originalMp3Path = '../test/assets/01 TempleOS Hymn Risen (Remix).mp3';
  final originalCoverPath = '../test/assets/cover.jpg';

  final originalFile = File(originalMp3Path);
  if (!originalFile.existsSync()) {
    print('Error: Test audio file not found at: $originalMp3Path');
    print('Please run this script from the "example" directory.');
    exit(1);
  }

  // Create a temporary file to perform read/write operations without modifying original test assets
  final tempDir = Directory.systemTemp.createTempSync('taglib_demo');
  final tempMp3Path = '${tempDir.path}/demo_song.mp3';
  originalFile.copySync(tempMp3Path);
  print('Temporary file created for demonstration: $tempMp3Path\n');

  try {
    // ----------------------------------------------------
    // 1. Reading Metadata
    // ----------------------------------------------------
    print('--- 1. Reading Original Metadata ---');
    final file = TagLibFile.open(tempMp3Path);
    if (file == null) {
      print('Failed to open audio file.');
      return;
    }

    print('Title:       ${file.title}');
    print('Artist:      ${file.artist}');
    print('Album:       ${file.album}');
    print('Genre:       ${file.genre}');
    print('Comment:     ${file.comment}');
    print('Year:        ${file.year}');
    print('Track:       ${file.track}');
    print('Duration:    ${file.duration.inSeconds} seconds (${file.duration})');
    print('Bitrate:     ${file.bitrate} kbps');
    print('Sample Rate: ${file.sampleRate} Hz');
    print('Channels:    ${file.channels}');
    print('Has Cover:   ${file.hasCover}');
    if (file.hasCover) {
      print('Cover Mime:  ${file.coverMimeType}');
      print('Cover Size:  ${file.coverData?.length} bytes');
    }
    print('');

    // ----------------------------------------------------
    // 2. Modifying and Saving Metadata
    // ----------------------------------------------------
    print('--- 2. Modifying Metadata Fields ---');
    file.title = 'A Brand New Title';
    file.artist = 'Flutter TagLib Developer';
    file.album = 'Dart Native Demonstration';
    file.genre = 'Programming Audio';
    file.comment = 'Written successfully using TagLib FFI!';
    file.year = 2026;
    file.track = 7;

    print('Saving metadata changes...');
    bool isSaved = file.save();
    print('Save status: ${isSaved ? "SUCCESS" : "FAILED"}');

    // Close the file to release the file handle
    file.close();
    print('Closed file handle.\n');

    // ----------------------------------------------------
    // 3. Verifying Saved Metadata
    // ----------------------------------------------------
    print('--- 3. Verifying Saved Metadata (Re-opening File) ---');
    final fileVerify = TagLibFile.open(tempMp3Path);
    if (fileVerify == null) {
      print('Failed to re-open audio file.');
      return;
    }

    print('New Title:   ${fileVerify.title}');
    print('New Artist:  ${fileVerify.artist}');
    print('New Album:   ${fileVerify.album}');
    print('New Genre:   ${fileVerify.genre}');
    print('New Comment: ${fileVerify.comment}');
    print('New Year:    ${fileVerify.year}');
    print('New Track:   ${fileVerify.track}');
    print('');

    // ----------------------------------------------------
    // 4. Writing & Reading Cover Art
    // ----------------------------------------------------
    print('--- 4. Setting Cover Art ---');
    final coverFile = File(originalCoverPath);
    if (coverFile.existsSync()) {
      final coverBytes = coverFile.readAsBytesSync();
      print('Setting cover art (${coverBytes.length} bytes, image/jpeg)...');
      
      bool isCoverSet = fileVerify.setCover(data: coverBytes, mimeType: 'image/jpeg');
      print('Set cover status: ${isCoverSet ? "SUCCESS" : "FAILED"}');
      
      print('Saving cover art changes...');
      bool isCoverSaved = fileVerify.save();
      print('Save status: ${isCoverSaved ? "SUCCESS" : "FAILED"}');
    } else {
      print('Warning: Cover file not found at $originalCoverPath, skipping cover art write.');
    }
    fileVerify.close();
    print('Closed file handle.\n');

    // ----------------------------------------------------
    // 5. Verifying Cover Art
    // ----------------------------------------------------
    print('--- 5. Verifying Cover Art (Re-opening File) ---');
    final fileVerifyCover = TagLibFile.open(tempMp3Path);
    if (fileVerifyCover == null) {
      print('Failed to re-open audio file.');
      return;
    }

    print('Has Cover:   ${fileVerifyCover.hasCover}');
    if (fileVerifyCover.hasCover) {
      print('Cover Mime:  ${fileVerifyCover.coverMimeType}');
      final readCoverData = fileVerifyCover.coverData;
      print('Cover Size:  ${readCoverData?.length} bytes');
      
      // Let's remove the cover to show how to delete it
      print('\nRemoving cover art...');
      bool isCoverRemoved = fileVerifyCover.setCover(data: null);
      print('Remove cover status: ${isCoverRemoved ? "SUCCESS" : "FAILED"}');
      fileVerifyCover.save();
      print('Saved cover removal.');
    }
    fileVerifyCover.close();
    print('Closed file handle.\n');

    // Final verification that cover is removed
    final fileFinal = TagLibFile.open(tempMp3Path);
    if (fileFinal != null) {
      print('--- 6. Final Verification (After Cover Removal) ---');
      print('Has Cover:   ${fileFinal.hasCover}');
      fileFinal.close();
    }

  } finally {
    // Clean up temporary files
    try {
      tempDir.deleteSync(recursive: true);
      print('\nTemporary directory cleaned up.');
    } catch (e) {
      print('Failed to clean up temporary directory: $e');
    }
  }
}

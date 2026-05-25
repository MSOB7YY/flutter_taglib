import 'dart:io';
import 'package:code_assets/code_assets.dart';
import 'package:native_toolchain_c/native_toolchain_c.dart';
import 'package:logging/logging.dart';
import 'package:hooks/hooks.dart';

void main(List<String> args) async {
  await build(args, (input, output) async {
    if (!input.config.buildCodeAssets) {
      return;
    }

    final targetOSStr = input.config.code.targetOS.toString().split('.').last.toLowerCase();
    if (!_isPlatformEnabled(targetOSStr)) {
      print('flutter_taglib: Building for $targetOSStr is disabled via flutter_taglib.yaml. Skipping compilation.');
      return;
    }

    final packageName = input.packageName;

    final sources = <String>[
      'src/flutter_taglib.cpp',
    ];

    final includes = <String>[
      'src',
      'taglib',
      'taglib/taglib',
      'taglib/3rdparty/utfcpp/source',
    ];

    // Find all .cpp files in taglib/taglib recursively and add to sources
    // Also find all subdirectories in taglib/taglib and add to includes
    final taglibDir = Directory('taglib/taglib');
    if (taglibDir.existsSync()) {
      for (final entity in taglibDir.listSync(recursive: true)) {
        if (entity is File && entity.path.endsWith('.cpp')) {
          sources.add(entity.path);
        } else if (entity is Directory) {
          includes.add(entity.path);
        }
      }
    }

    final cbuilder = CBuilder.library(
      name: packageName,
      assetName: '${packageName}_bindings_generated.dart',
      sources: sources,
      includes: includes,
      defines: {
        'HAVE_CONFIG_H': '1',
        'TAGLIB_STATIC': '1',
      },
      std: 'c++17',
      language: Language.cpp,
      cppLinkStdLib: input.config.code.targetOS.toString().contains('android') ? 'c++_static' : null,
      flags: [
        if (!input.config.code.targetOS.toString().contains('windows'))
          '-fvisibility=hidden',
      ],
      libraries: [
        if (input.config.code.targetOS.toString().contains('android') ||
            input.config.code.targetOS.toString().contains('linux'))
          'm',
        if (input.config.code.targetOS.toString().contains('android'))
          'log',
      ],
    );

    await cbuilder.run(
      input: input,
      output: output,
      logger: Logger('')
        ..level = Level.ALL
        ..onRecord.listen((record) => print(record.message)),
    );
  });
}

bool _isPlatformEnabled(String targetOS) {
  // Check for configuration file in current directory or parent directory
  File? configFile;
  final pathsToCheck = [
    Directory.current.uri.resolve('flutter_taglib.yaml').toFilePath(),
    if (Directory.current.parent.existsSync())
      Directory.current.parent.uri.resolve('flutter_taglib.yaml').toFilePath(),
  ];

  for (final path in pathsToCheck) {
    final file = File(path);
    if (file.existsSync()) {
      configFile = file;
      break;
    }
  }

  if (configFile == null) {
    return true; // Default to enabled if no config file is found
  }

  try {
    final lines = configFile.readAsLinesSync();
    bool inPlatformsBlock = false;
    for (var line in lines) {
      line = line.trim();
      if (line.isEmpty || line.startsWith('#')) continue;

      if (line.startsWith('platforms:')) {
        inPlatformsBlock = true;
        continue;
      }

      // If we hit another top-level key, exit the platforms block
      if (inPlatformsBlock && line.endsWith(':') && !line.startsWith(' ')) {
        inPlatformsBlock = false;
      }

      if (inPlatformsBlock) {
        final parts = line.split(':');
        if (parts.length == 2) {
          final key = parts[0].trim().toLowerCase();
          final val = parts[1].trim().toLowerCase();
          if (key == targetOS.toLowerCase()) {
            return val == 'true';
          }
        }
      }
    }
  } catch (e) {
    print('flutter_taglib hook/build.dart: error reading/parsing flutter_taglib.yaml: $e');
  }

  return true; // Default to enabled on error or if not found in config
}

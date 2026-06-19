library;

// ignore_for_file: non_constant_identifier_names

import 'dart:ffi' as ffi;
import 'dart:io';

import '../flutter_taglib_bindings_generated.dart' as generated;

typedef TagLibBridgeFile = generated.TagLibBridgeFile;
typedef TagLibBridgePictures = generated.TagLibBridgePictures;
typedef TagLibBridgeProperties = generated.TagLibBridgeProperties;

const String _desktopBinaryVersion = '1.2.0';
const String _configuredDesktopBinaryBaseUrl = String.fromEnvironment(
  'FLUTTER_TAGLIB_DESKTOP_BASE_URL',
  defaultValue: '',
);

final _DesktopBindingsLoader _loader = _DesktopBindingsLoader();

bool get usesDownloadedDesktopBinary => _loader.requiresDesktopRuntimeLoad;

String? get loadedDesktopBinaryPath => _loader.loadedLibraryPath;

String? get desktopBinaryError => _loader.lastError?.toString();

void configureDesktopBinarySource({String? baseUrl, String? version}) {
  _loader.configure(baseUrl: baseUrl, version: version);
}

Future<void> ensureDesktopLibraryReady() => _loader.ensureLoaded();

ffi.Pointer<TagLibBridgeFile> taglib_bridge_open(
  ffi.Pointer<ffi.Char> filepath,
) {
  return _loader.bindings.taglibBridgeOpen(filepath);
}

ffi.Pointer<TagLibBridgeFile> taglib_bridge_open_fd(int fd) {
  return _loader.bindings.taglibBridgeOpenFd(fd);
}

int taglib_bridge_save(ffi.Pointer<TagLibBridgeFile> file) {
  return _loader.bindings.taglibBridgeSave(file);
}

void taglib_bridge_close(ffi.Pointer<TagLibBridgeFile> file) {
  _loader.bindings.taglibBridgeClose(file);
}

ffi.Pointer<ffi.Char> taglib_bridge_get_title(
  ffi.Pointer<TagLibBridgeFile> file,
) {
  return _loader.bindings.taglibBridgeGetTitle(file);
}

ffi.Pointer<ffi.Char> taglib_bridge_get_artist(
  ffi.Pointer<TagLibBridgeFile> file,
) {
  return _loader.bindings.taglibBridgeGetArtist(file);
}

ffi.Pointer<ffi.Char> taglib_bridge_get_album(
  ffi.Pointer<TagLibBridgeFile> file,
) {
  return _loader.bindings.taglibBridgeGetAlbum(file);
}

ffi.Pointer<ffi.Char> taglib_bridge_get_genre(
  ffi.Pointer<TagLibBridgeFile> file,
) {
  return _loader.bindings.taglibBridgeGetGenre(file);
}

ffi.Pointer<ffi.Char> taglib_bridge_get_comment(
  ffi.Pointer<TagLibBridgeFile> file,
) {
  return _loader.bindings.taglibBridgeGetComment(file);
}

int taglib_bridge_get_year(ffi.Pointer<TagLibBridgeFile> file) {
  return _loader.bindings.taglibBridgeGetYear(file);
}

int taglib_bridge_get_track(ffi.Pointer<TagLibBridgeFile> file) {
  return _loader.bindings.taglibBridgeGetTrack(file);
}

void taglib_bridge_set_title(
  ffi.Pointer<TagLibBridgeFile> file,
  ffi.Pointer<ffi.Char> title,
) {
  _loader.bindings.taglibBridgeSetTitle(file, title);
}

void taglib_bridge_set_artist(
  ffi.Pointer<TagLibBridgeFile> file,
  ffi.Pointer<ffi.Char> artist,
) {
  _loader.bindings.taglibBridgeSetArtist(file, artist);
}

void taglib_bridge_set_album(
  ffi.Pointer<TagLibBridgeFile> file,
  ffi.Pointer<ffi.Char> album,
) {
  _loader.bindings.taglibBridgeSetAlbum(file, album);
}

void taglib_bridge_set_genre(
  ffi.Pointer<TagLibBridgeFile> file,
  ffi.Pointer<ffi.Char> genre,
) {
  _loader.bindings.taglibBridgeSetGenre(file, genre);
}

void taglib_bridge_set_comment(
  ffi.Pointer<TagLibBridgeFile> file,
  ffi.Pointer<ffi.Char> comment,
) {
  _loader.bindings.taglibBridgeSetComment(file, comment);
}

void taglib_bridge_set_year(ffi.Pointer<TagLibBridgeFile> file, int year) {
  _loader.bindings.taglibBridgeSetYear(file, year);
}

void taglib_bridge_set_track(ffi.Pointer<TagLibBridgeFile> file, int track) {
  _loader.bindings.taglibBridgeSetTrack(file, track);
}

int taglib_bridge_get_duration(ffi.Pointer<TagLibBridgeFile> file) {
  return _loader.bindings.taglibBridgeGetDuration(file);
}

int taglib_bridge_get_bitrate(ffi.Pointer<TagLibBridgeFile> file) {
  return _loader.bindings.taglibBridgeGetBitrate(file);
}

int taglib_bridge_get_samplerate(ffi.Pointer<TagLibBridgeFile> file) {
  return _loader.bindings.taglibBridgeGetSampleRate(file);
}

int taglib_bridge_get_channels(ffi.Pointer<TagLibBridgeFile> file) {
  return _loader.bindings.taglibBridgeGetChannels(file);
}

ffi.Pointer<ffi.Char> taglib_bridge_get_bitrate_mode(
  ffi.Pointer<TagLibBridgeFile> file,
) {
  return _loader.bindings.taglibBridgeGetBitrateMode(file);
}

int taglib_bridge_has_cover(ffi.Pointer<TagLibBridgeFile> file) {
  return _loader.bindings.taglibBridgeHasCover(file);
}

ffi.Pointer<TagLibBridgePictures> taglib_bridge_pictures_create() {
  return _loader.bindings.taglibBridgePicturesCreate();
}

void taglib_bridge_pictures_free(ffi.Pointer<TagLibBridgePictures> pictures) {
  _loader.bindings.taglibBridgePicturesFree(pictures);
}

ffi.Pointer<TagLibBridgePictures> taglib_bridge_pictures_get(
  ffi.Pointer<TagLibBridgeFile> file,
) {
  return _loader.bindings.taglibBridgePicturesGet(file);
}

int taglib_bridge_pictures_set(
  ffi.Pointer<TagLibBridgeFile> file,
  ffi.Pointer<TagLibBridgePictures> pictures,
) {
  return _loader.bindings.taglibBridgePicturesSet(file, pictures);
}

int taglib_bridge_pictures_size(ffi.Pointer<TagLibBridgePictures> pictures) {
  return _loader.bindings.taglibBridgePicturesSize(pictures);
}

int taglib_bridge_pictures_data_size(
  ffi.Pointer<TagLibBridgePictures> pictures,
  int index,
) {
  return _loader.bindings.taglibBridgePicturesDataSize(pictures, index);
}

int taglib_bridge_pictures_data(
  ffi.Pointer<TagLibBridgePictures> pictures,
  int index,
  ffi.Pointer<ffi.Uint8> buffer,
  int bufferSize,
) {
  return _loader.bindings.taglibBridgePicturesData(
    pictures,
    index,
    buffer,
    bufferSize,
  );
}

ffi.Pointer<ffi.Char> taglib_bridge_pictures_mime_type(
  ffi.Pointer<TagLibBridgePictures> pictures,
  int index,
) {
  return _loader.bindings.taglibBridgePicturesMimeType(pictures, index);
}

ffi.Pointer<ffi.Char> taglib_bridge_pictures_description(
  ffi.Pointer<TagLibBridgePictures> pictures,
  int index,
) {
  return _loader.bindings.taglibBridgePicturesDescription(pictures, index);
}

ffi.Pointer<ffi.Char> taglib_bridge_pictures_picture_type(
  ffi.Pointer<TagLibBridgePictures> pictures,
  int index,
) {
  return _loader.bindings.taglibBridgePicturesPictureType(pictures, index);
}

void taglib_bridge_pictures_add(
  ffi.Pointer<TagLibBridgePictures> pictures,
  ffi.Pointer<ffi.Uint8> data,
  int size,
  ffi.Pointer<ffi.Char> mimeType,
  ffi.Pointer<ffi.Char> pictureType,
  ffi.Pointer<ffi.Char> description,
) {
  _loader.bindings.taglibBridgePicturesAdd(
    pictures,
    data,
    size,
    mimeType,
    pictureType,
    description,
  );
}

ffi.Pointer<TagLibBridgeProperties> taglib_bridge_properties_create() {
  return _loader.bindings.taglibBridgePropertiesCreate();
}

void taglib_bridge_properties_free(
  ffi.Pointer<TagLibBridgeProperties> properties,
) {
  _loader.bindings.taglibBridgePropertiesFree(properties);
}

ffi.Pointer<TagLibBridgeProperties> taglib_bridge_properties_get(
  ffi.Pointer<TagLibBridgeFile> file,
) {
  return _loader.bindings.taglibBridgePropertiesGet(file);
}

ffi.Pointer<TagLibBridgeProperties> taglib_bridge_properties_set(
  ffi.Pointer<TagLibBridgeFile> file,
  ffi.Pointer<TagLibBridgeProperties> properties,
) {
  return _loader.bindings.taglibBridgePropertiesSet(file, properties);
}

int taglib_bridge_properties_size(
  ffi.Pointer<TagLibBridgeProperties> properties,
) {
  return _loader.bindings.taglibBridgePropertiesSize(properties);
}

ffi.Pointer<ffi.Char> taglib_bridge_properties_key(
  ffi.Pointer<TagLibBridgeProperties> properties,
  int index,
) {
  return _loader.bindings.taglibBridgePropertiesKey(properties, index);
}

int taglib_bridge_properties_value_count(
  ffi.Pointer<TagLibBridgeProperties> properties,
  ffi.Pointer<ffi.Char> key,
) {
  return _loader.bindings.taglibBridgePropertiesValueCount(properties, key);
}

ffi.Pointer<ffi.Char> taglib_bridge_properties_value(
  ffi.Pointer<TagLibBridgeProperties> properties,
  ffi.Pointer<ffi.Char> key,
  int valueIndex,
) {
  return _loader.bindings.taglibBridgePropertiesValue(
    properties,
    key,
    valueIndex,
  );
}

void taglib_bridge_properties_add(
  ffi.Pointer<TagLibBridgeProperties> properties,
  ffi.Pointer<ffi.Char> key,
  ffi.Pointer<ffi.Char> value,
) {
  _loader.bindings.taglibBridgePropertiesAdd(properties, key, value);
}

class _DesktopBindingsLoader {
  _TagLibBindings? _bindings;
  Future<void>? _loading;
  Object? lastError;
  String? loadedLibraryPath;
  String? _baseUrl;
  String _version = _desktopBinaryVersion;

  bool get requiresDesktopRuntimeLoad => false;

  _TagLibBindings get bindings {
    if (_bindings != null) {
      return _bindings!;
    }
    if (!requiresDesktopRuntimeLoad) {
      _bindings = _TagLibBindings.generated();
      return _bindings!;
    }
    _tryLoadCachedSync();
    if (_bindings != null) {
      return _bindings!;
    }
    if (lastError != null) {
      Error.throwWithStackTrace(lastError!, StackTrace.current);
    }
    throw StateError(
      'Desktop TagLib binary is not ready yet. Call TagLibFile.prepareDesktopLibrary() or use openAsync() first.',
    );
  }

  void configure({String? baseUrl, String? version}) {
    if (_bindings != null) {
      throw StateError(
        'Desktop TagLib binary source cannot be reconfigured after loading.',
      );
    }
    if (baseUrl != null && baseUrl.isNotEmpty) {
      _baseUrl = baseUrl.endsWith('/')
          ? baseUrl.substring(0, baseUrl.length - 1)
          : baseUrl;
    }
    if (version != null && version.isNotEmpty) {
      _version = version;
    }
  }

  Future<void> ensureLoaded() async {
    if (!requiresDesktopRuntimeLoad || _bindings != null) {
      return;
    }
    final current = _loading;
    if (current != null) {
      return current;
    }
    final future = _ensureLoadedImpl();
    _loading = future;
    try {
      await future;
    } finally {
      _loading = null;
    }
  }

  void _tryLoadCachedSync() {
    if (!requiresDesktopRuntimeLoad || _bindings != null) {
      return;
    }
    final descriptor = _currentDescriptor;
    if (descriptor == null) {
      lastError = UnsupportedError(
        'flutter_taglib desktop binaries currently support only x64 Windows/Linux.',
      );
      return;
    }
    final file = File(_localLibraryPath(descriptor));
    if (!file.existsSync()) {
      return;
    }
    _loadLibrary(file);
  }

  Future<void> _ensureLoadedImpl() async {
    final descriptor = _currentDescriptor;
    if (descriptor == null) {
      throw UnsupportedError(
        'flutter_taglib desktop binaries currently support only x64 Windows/Linux.',
      );
    }

    final file = File(_localLibraryPath(descriptor));
    if (!file.existsSync()) {
      await file.parent.create(recursive: true);
      final tempFile = File('${file.path}.download');
      if (tempFile.existsSync()) {
        tempFile.deleteSync();
      }

      final client = HttpClient();
      try {
        final uri = Uri.parse('$_resolvedBaseUrl/${descriptor.remoteFileName}');
        final request = await client.getUrl(uri);
        final response = await request.close();
        if (response.statusCode != 200) {
          throw HttpException(
            'Failed to download ${descriptor.remoteFileName}: HTTP ${response.statusCode}',
            uri: uri,
          );
        }
        final sink = tempFile.openWrite();
        await response.pipe(sink);
        await sink.close();
        if (file.existsSync()) {
          file.deleteSync();
        }
        tempFile.renameSync(file.path);
      } finally {
        client.close();
        if (tempFile.existsSync()) {
          tempFile.deleteSync();
        }
      }
    }

    _loadLibrary(file);
  }

  void _loadLibrary(File file) {
    try {
      final library = ffi.DynamicLibrary.open(file.path);
      _bindings = _TagLibBindings.dynamic(library);
      loadedLibraryPath = file.path;
      lastError = null;
    } catch (error) {
      lastError = error;
      rethrow;
    }
  }

  _DesktopBinaryDescriptor? get _currentDescriptor {
    if (Platform.isWindows && ffi.Abi.current() == ffi.Abi.windowsX64) {
      return const _DesktopBinaryDescriptor(
        remoteFileName: 'flutter_taglib_windows_x64.dll',
        localFileName: 'flutter_taglib_native.dll',
      );
    }
    if (Platform.isLinux && ffi.Abi.current() == ffi.Abi.linuxX64) {
      return const _DesktopBinaryDescriptor(
        remoteFileName: 'libflutter_taglib_linux_x64.so',
        localFileName: 'libflutter_taglib_native.so',
      );
    }
    return null;
  }

  String _localLibraryPath(_DesktopBinaryDescriptor descriptor) {
    final root = _desktopCacheRoot();
    return '$root${Platform.pathSeparator}$_version${Platform.pathSeparator}${descriptor.localFileName}';
  }

  String _desktopCacheRoot() {
    if (Platform.isWindows) {
      final localAppData = Platform.environment['LOCALAPPDATA'];
      if (localAppData != null && localAppData.isNotEmpty) {
        return '$localAppData${Platform.pathSeparator}flutter_taglib${Platform.pathSeparator}native';
      }
    }

    if (Platform.isLinux) {
      final xdgCache = Platform.environment['XDG_CACHE_HOME'];
      if (xdgCache != null && xdgCache.isNotEmpty) {
        return '$xdgCache${Platform.pathSeparator}flutter_taglib${Platform.pathSeparator}native';
      }
      final home = Platform.environment['HOME'];
      if (home != null && home.isNotEmpty) {
        return '$home${Platform.pathSeparator}.cache${Platform.pathSeparator}flutter_taglib${Platform.pathSeparator}native';
      }
    }

    return '${Directory.systemTemp.path}${Platform.pathSeparator}flutter_taglib${Platform.pathSeparator}native';
  }

  String get _resolvedBaseUrl {
    final configured =
        _baseUrl ??
        (_configuredDesktopBinaryBaseUrl.isEmpty
            ? null
            : _configuredDesktopBinaryBaseUrl);
    return configured ??
        'https://github.com/axel10/flutter_taglib/releases/download/desktop-binaries-v$_version';
  }
}

class _DesktopBinaryDescriptor {
  const _DesktopBinaryDescriptor({
    required this.remoteFileName,
    required this.localFileName,
  });

  final String remoteFileName;
  final String localFileName;
}

class _TagLibBindings {
  _TagLibBindings.generated()
    : taglibBridgeOpen = generated.taglib_bridge_open,
      taglibBridgeOpenFd = generated.taglib_bridge_open_fd,
      taglibBridgeSave = generated.taglib_bridge_save,
      taglibBridgeClose = generated.taglib_bridge_close,
      taglibBridgeGetTitle = generated.taglib_bridge_get_title,
      taglibBridgeGetArtist = generated.taglib_bridge_get_artist,
      taglibBridgeGetAlbum = generated.taglib_bridge_get_album,
      taglibBridgeGetGenre = generated.taglib_bridge_get_genre,
      taglibBridgeGetComment = generated.taglib_bridge_get_comment,
      taglibBridgeGetYear = generated.taglib_bridge_get_year,
      taglibBridgeGetTrack = generated.taglib_bridge_get_track,
      taglibBridgeSetTitle = generated.taglib_bridge_set_title,
      taglibBridgeSetArtist = generated.taglib_bridge_set_artist,
      taglibBridgeSetAlbum = generated.taglib_bridge_set_album,
      taglibBridgeSetGenre = generated.taglib_bridge_set_genre,
      taglibBridgeSetComment = generated.taglib_bridge_set_comment,
      taglibBridgeSetYear = generated.taglib_bridge_set_year,
      taglibBridgeSetTrack = generated.taglib_bridge_set_track,
      taglibBridgeGetDuration = generated.taglib_bridge_get_duration,
      taglibBridgeGetBitrate = generated.taglib_bridge_get_bitrate,
      taglibBridgeGetSampleRate = generated.taglib_bridge_get_samplerate,
      taglibBridgeGetChannels = generated.taglib_bridge_get_channels,
      taglibBridgeGetBitrateMode = generated.taglib_bridge_get_bitrate_mode,
      taglibBridgeHasCover = generated.taglib_bridge_has_cover,
      taglibBridgePicturesCreate = generated.taglib_bridge_pictures_create,
      taglibBridgePicturesFree = generated.taglib_bridge_pictures_free,
      taglibBridgePicturesGet = generated.taglib_bridge_pictures_get,
      taglibBridgePicturesSet = generated.taglib_bridge_pictures_set,
      taglibBridgePicturesSize = generated.taglib_bridge_pictures_size,
      taglibBridgePicturesDataSize = generated.taglib_bridge_pictures_data_size,
      taglibBridgePicturesData = generated.taglib_bridge_pictures_data,
      taglibBridgePicturesMimeType = generated.taglib_bridge_pictures_mime_type,
      taglibBridgePicturesDescription =
          generated.taglib_bridge_pictures_description,
      taglibBridgePicturesPictureType =
          generated.taglib_bridge_pictures_picture_type,
      taglibBridgePicturesAdd = generated.taglib_bridge_pictures_add,
      taglibBridgePropertiesCreate = generated.taglib_bridge_properties_create,
      taglibBridgePropertiesFree = generated.taglib_bridge_properties_free,
      taglibBridgePropertiesGet = generated.taglib_bridge_properties_get,
      taglibBridgePropertiesSet = generated.taglib_bridge_properties_set,
      taglibBridgePropertiesSize = generated.taglib_bridge_properties_size,
      taglibBridgePropertiesKey = generated.taglib_bridge_properties_key,
      taglibBridgePropertiesValueCount =
          generated.taglib_bridge_properties_value_count,
      taglibBridgePropertiesValue = generated.taglib_bridge_properties_value,
      taglibBridgePropertiesAdd = generated.taglib_bridge_properties_add;

  _TagLibBindings.dynamic(ffi.DynamicLibrary library)
    : taglibBridgeOpen = library
          .lookupFunction<
            ffi.Pointer<TagLibBridgeFile> Function(ffi.Pointer<ffi.Char>),
            ffi.Pointer<TagLibBridgeFile> Function(ffi.Pointer<ffi.Char>)
          >('taglib_bridge_open'),
      taglibBridgeOpenFd = library
          .lookupFunction<
            ffi.Pointer<TagLibBridgeFile> Function(ffi.Int),
            ffi.Pointer<TagLibBridgeFile> Function(int)
          >('taglib_bridge_open_fd'),
      taglibBridgeSave = library
          .lookupFunction<
            ffi.Int Function(ffi.Pointer<TagLibBridgeFile>),
            int Function(ffi.Pointer<TagLibBridgeFile>)
          >('taglib_bridge_save'),
      taglibBridgeClose = library
          .lookupFunction<
            ffi.Void Function(ffi.Pointer<TagLibBridgeFile>),
            void Function(ffi.Pointer<TagLibBridgeFile>)
          >('taglib_bridge_close'),
      taglibBridgeGetTitle = library
          .lookupFunction<
            ffi.Pointer<ffi.Char> Function(ffi.Pointer<TagLibBridgeFile>),
            ffi.Pointer<ffi.Char> Function(ffi.Pointer<TagLibBridgeFile>)
          >('taglib_bridge_get_title'),
      taglibBridgeGetArtist = library
          .lookupFunction<
            ffi.Pointer<ffi.Char> Function(ffi.Pointer<TagLibBridgeFile>),
            ffi.Pointer<ffi.Char> Function(ffi.Pointer<TagLibBridgeFile>)
          >('taglib_bridge_get_artist'),
      taglibBridgeGetAlbum = library
          .lookupFunction<
            ffi.Pointer<ffi.Char> Function(ffi.Pointer<TagLibBridgeFile>),
            ffi.Pointer<ffi.Char> Function(ffi.Pointer<TagLibBridgeFile>)
          >('taglib_bridge_get_album'),
      taglibBridgeGetGenre = library
          .lookupFunction<
            ffi.Pointer<ffi.Char> Function(ffi.Pointer<TagLibBridgeFile>),
            ffi.Pointer<ffi.Char> Function(ffi.Pointer<TagLibBridgeFile>)
          >('taglib_bridge_get_genre'),
      taglibBridgeGetComment = library
          .lookupFunction<
            ffi.Pointer<ffi.Char> Function(ffi.Pointer<TagLibBridgeFile>),
            ffi.Pointer<ffi.Char> Function(ffi.Pointer<TagLibBridgeFile>)
          >('taglib_bridge_get_comment'),
      taglibBridgeGetYear = library
          .lookupFunction<
            ffi.Uint32 Function(ffi.Pointer<TagLibBridgeFile>),
            int Function(ffi.Pointer<TagLibBridgeFile>)
          >('taglib_bridge_get_year'),
      taglibBridgeGetTrack = library
          .lookupFunction<
            ffi.Uint32 Function(ffi.Pointer<TagLibBridgeFile>),
            int Function(ffi.Pointer<TagLibBridgeFile>)
          >('taglib_bridge_get_track'),
      taglibBridgeSetTitle = library
          .lookupFunction<
            ffi.Void Function(
              ffi.Pointer<TagLibBridgeFile>,
              ffi.Pointer<ffi.Char>,
            ),
            void Function(ffi.Pointer<TagLibBridgeFile>, ffi.Pointer<ffi.Char>)
          >('taglib_bridge_set_title'),
      taglibBridgeSetArtist = library
          .lookupFunction<
            ffi.Void Function(
              ffi.Pointer<TagLibBridgeFile>,
              ffi.Pointer<ffi.Char>,
            ),
            void Function(ffi.Pointer<TagLibBridgeFile>, ffi.Pointer<ffi.Char>)
          >('taglib_bridge_set_artist'),
      taglibBridgeSetAlbum = library
          .lookupFunction<
            ffi.Void Function(
              ffi.Pointer<TagLibBridgeFile>,
              ffi.Pointer<ffi.Char>,
            ),
            void Function(ffi.Pointer<TagLibBridgeFile>, ffi.Pointer<ffi.Char>)
          >('taglib_bridge_set_album'),
      taglibBridgeSetGenre = library
          .lookupFunction<
            ffi.Void Function(
              ffi.Pointer<TagLibBridgeFile>,
              ffi.Pointer<ffi.Char>,
            ),
            void Function(ffi.Pointer<TagLibBridgeFile>, ffi.Pointer<ffi.Char>)
          >('taglib_bridge_set_genre'),
      taglibBridgeSetComment = library
          .lookupFunction<
            ffi.Void Function(
              ffi.Pointer<TagLibBridgeFile>,
              ffi.Pointer<ffi.Char>,
            ),
            void Function(ffi.Pointer<TagLibBridgeFile>, ffi.Pointer<ffi.Char>)
          >('taglib_bridge_set_comment'),
      taglibBridgeSetYear = library
          .lookupFunction<
            ffi.Void Function(ffi.Pointer<TagLibBridgeFile>, ffi.Uint32),
            void Function(ffi.Pointer<TagLibBridgeFile>, int)
          >('taglib_bridge_set_year'),
      taglibBridgeSetTrack = library
          .lookupFunction<
            ffi.Void Function(ffi.Pointer<TagLibBridgeFile>, ffi.Uint32),
            void Function(ffi.Pointer<TagLibBridgeFile>, int)
          >('taglib_bridge_set_track'),
      taglibBridgeGetDuration = library
          .lookupFunction<
            ffi.Int Function(ffi.Pointer<TagLibBridgeFile>),
            int Function(ffi.Pointer<TagLibBridgeFile>)
          >('taglib_bridge_get_duration'),
      taglibBridgeGetBitrate = library
          .lookupFunction<
            ffi.Int Function(ffi.Pointer<TagLibBridgeFile>),
            int Function(ffi.Pointer<TagLibBridgeFile>)
          >('taglib_bridge_get_bitrate'),
      taglibBridgeGetSampleRate = library
          .lookupFunction<
            ffi.Int Function(ffi.Pointer<TagLibBridgeFile>),
            int Function(ffi.Pointer<TagLibBridgeFile>)
          >('taglib_bridge_get_samplerate'),
      taglibBridgeGetChannels = library
          .lookupFunction<
            ffi.Int Function(ffi.Pointer<TagLibBridgeFile>),
            int Function(ffi.Pointer<TagLibBridgeFile>)
          >('taglib_bridge_get_channels'),
      taglibBridgeGetBitrateMode = library
          .lookupFunction<
            ffi.Pointer<ffi.Char> Function(ffi.Pointer<TagLibBridgeFile>),
            ffi.Pointer<ffi.Char> Function(ffi.Pointer<TagLibBridgeFile>)
          >('taglib_bridge_get_bitrate_mode'),
      taglibBridgeHasCover = library
          .lookupFunction<
            ffi.Int Function(ffi.Pointer<TagLibBridgeFile>),
            int Function(ffi.Pointer<TagLibBridgeFile>)
          >('taglib_bridge_has_cover'),
      taglibBridgePicturesCreate = library
          .lookupFunction<
            ffi.Pointer<TagLibBridgePictures> Function(),
            ffi.Pointer<TagLibBridgePictures> Function()
          >('taglib_bridge_pictures_create'),
      taglibBridgePicturesFree = library
          .lookupFunction<
            ffi.Void Function(ffi.Pointer<TagLibBridgePictures>),
            void Function(ffi.Pointer<TagLibBridgePictures>)
          >('taglib_bridge_pictures_free'),
      taglibBridgePicturesGet = library
          .lookupFunction<
            ffi.Pointer<TagLibBridgePictures> Function(
              ffi.Pointer<TagLibBridgeFile>,
            ),
            ffi.Pointer<TagLibBridgePictures> Function(
              ffi.Pointer<TagLibBridgeFile>,
            )
          >('taglib_bridge_pictures_get'),
      taglibBridgePicturesSet = library
          .lookupFunction<
            ffi.Int Function(
              ffi.Pointer<TagLibBridgeFile>,
              ffi.Pointer<TagLibBridgePictures>,
            ),
            int Function(
              ffi.Pointer<TagLibBridgeFile>,
              ffi.Pointer<TagLibBridgePictures>,
            )
          >('taglib_bridge_pictures_set'),
      taglibBridgePicturesSize = library
          .lookupFunction<
            ffi.Int Function(ffi.Pointer<TagLibBridgePictures>),
            int Function(ffi.Pointer<TagLibBridgePictures>)
          >('taglib_bridge_pictures_size'),
      taglibBridgePicturesDataSize = library
          .lookupFunction<
            ffi.Uint32 Function(ffi.Pointer<TagLibBridgePictures>, ffi.Int),
            int Function(ffi.Pointer<TagLibBridgePictures>, int)
          >('taglib_bridge_pictures_data_size'),
      taglibBridgePicturesData = library
          .lookupFunction<
            ffi.Int Function(
              ffi.Pointer<TagLibBridgePictures>,
              ffi.Int,
              ffi.Pointer<ffi.Uint8>,
              ffi.Uint32,
            ),
            int Function(
              ffi.Pointer<TagLibBridgePictures>,
              int,
              ffi.Pointer<ffi.Uint8>,
              int,
            )
          >('taglib_bridge_pictures_data'),
      taglibBridgePicturesMimeType = library
          .lookupFunction<
            ffi.Pointer<ffi.Char> Function(
              ffi.Pointer<TagLibBridgePictures>,
              ffi.Int,
            ),
            ffi.Pointer<ffi.Char> Function(
              ffi.Pointer<TagLibBridgePictures>,
              int,
            )
          >('taglib_bridge_pictures_mime_type'),
      taglibBridgePicturesDescription = library
          .lookupFunction<
            ffi.Pointer<ffi.Char> Function(
              ffi.Pointer<TagLibBridgePictures>,
              ffi.Int,
            ),
            ffi.Pointer<ffi.Char> Function(
              ffi.Pointer<TagLibBridgePictures>,
              int,
            )
          >('taglib_bridge_pictures_description'),
      taglibBridgePicturesPictureType = library
          .lookupFunction<
            ffi.Pointer<ffi.Char> Function(
              ffi.Pointer<TagLibBridgePictures>,
              ffi.Int,
            ),
            ffi.Pointer<ffi.Char> Function(
              ffi.Pointer<TagLibBridgePictures>,
              int,
            )
          >('taglib_bridge_pictures_picture_type'),
      taglibBridgePicturesAdd = library
          .lookupFunction<
            ffi.Void Function(
              ffi.Pointer<TagLibBridgePictures>,
              ffi.Pointer<ffi.Uint8>,
              ffi.Uint32,
              ffi.Pointer<ffi.Char>,
              ffi.Pointer<ffi.Char>,
              ffi.Pointer<ffi.Char>,
            ),
            void Function(
              ffi.Pointer<TagLibBridgePictures>,
              ffi.Pointer<ffi.Uint8>,
              int,
              ffi.Pointer<ffi.Char>,
              ffi.Pointer<ffi.Char>,
              ffi.Pointer<ffi.Char>,
            )
          >('taglib_bridge_pictures_add'),
      taglibBridgePropertiesCreate = library
          .lookupFunction<
            ffi.Pointer<TagLibBridgeProperties> Function(),
            ffi.Pointer<TagLibBridgeProperties> Function()
          >('taglib_bridge_properties_create'),
      taglibBridgePropertiesFree = library
          .lookupFunction<
            ffi.Void Function(ffi.Pointer<TagLibBridgeProperties>),
            void Function(ffi.Pointer<TagLibBridgeProperties>)
          >('taglib_bridge_properties_free'),
      taglibBridgePropertiesGet = library
          .lookupFunction<
            ffi.Pointer<TagLibBridgeProperties> Function(
              ffi.Pointer<TagLibBridgeFile>,
            ),
            ffi.Pointer<TagLibBridgeProperties> Function(
              ffi.Pointer<TagLibBridgeFile>,
            )
          >('taglib_bridge_properties_get'),
      taglibBridgePropertiesSet = library
          .lookupFunction<
            ffi.Pointer<TagLibBridgeProperties> Function(
              ffi.Pointer<TagLibBridgeFile>,
              ffi.Pointer<TagLibBridgeProperties>,
            ),
            ffi.Pointer<TagLibBridgeProperties> Function(
              ffi.Pointer<TagLibBridgeFile>,
              ffi.Pointer<TagLibBridgeProperties>,
            )
          >('taglib_bridge_properties_set'),
      taglibBridgePropertiesSize = library
          .lookupFunction<
            ffi.Int Function(ffi.Pointer<TagLibBridgeProperties>),
            int Function(ffi.Pointer<TagLibBridgeProperties>)
          >('taglib_bridge_properties_size'),
      taglibBridgePropertiesKey = library
          .lookupFunction<
            ffi.Pointer<ffi.Char> Function(
              ffi.Pointer<TagLibBridgeProperties>,
              ffi.Int,
            ),
            ffi.Pointer<ffi.Char> Function(
              ffi.Pointer<TagLibBridgeProperties>,
              int,
            )
          >('taglib_bridge_properties_key'),
      taglibBridgePropertiesValueCount = library
          .lookupFunction<
            ffi.Int Function(
              ffi.Pointer<TagLibBridgeProperties>,
              ffi.Pointer<ffi.Char>,
            ),
            int Function(
              ffi.Pointer<TagLibBridgeProperties>,
              ffi.Pointer<ffi.Char>,
            )
          >('taglib_bridge_properties_value_count'),
      taglibBridgePropertiesValue = library
          .lookupFunction<
            ffi.Pointer<ffi.Char> Function(
              ffi.Pointer<TagLibBridgeProperties>,
              ffi.Pointer<ffi.Char>,
              ffi.Int,
            ),
            ffi.Pointer<ffi.Char> Function(
              ffi.Pointer<TagLibBridgeProperties>,
              ffi.Pointer<ffi.Char>,
              int,
            )
          >('taglib_bridge_properties_value'),
      taglibBridgePropertiesAdd = library
          .lookupFunction<
            ffi.Void Function(
              ffi.Pointer<TagLibBridgeProperties>,
              ffi.Pointer<ffi.Char>,
              ffi.Pointer<ffi.Char>,
            ),
            void Function(
              ffi.Pointer<TagLibBridgeProperties>,
              ffi.Pointer<ffi.Char>,
              ffi.Pointer<ffi.Char>,
            )
          >('taglib_bridge_properties_add');

  final ffi.Pointer<TagLibBridgeFile> Function(ffi.Pointer<ffi.Char>)
  taglibBridgeOpen;
  final ffi.Pointer<TagLibBridgeFile> Function(int) taglibBridgeOpenFd;
  final int Function(ffi.Pointer<TagLibBridgeFile>) taglibBridgeSave;
  final void Function(ffi.Pointer<TagLibBridgeFile>) taglibBridgeClose;
  final ffi.Pointer<ffi.Char> Function(ffi.Pointer<TagLibBridgeFile>)
  taglibBridgeGetTitle;
  final ffi.Pointer<ffi.Char> Function(ffi.Pointer<TagLibBridgeFile>)
  taglibBridgeGetArtist;
  final ffi.Pointer<ffi.Char> Function(ffi.Pointer<TagLibBridgeFile>)
  taglibBridgeGetAlbum;
  final ffi.Pointer<ffi.Char> Function(ffi.Pointer<TagLibBridgeFile>)
  taglibBridgeGetGenre;
  final ffi.Pointer<ffi.Char> Function(ffi.Pointer<TagLibBridgeFile>)
  taglibBridgeGetComment;
  final int Function(ffi.Pointer<TagLibBridgeFile>) taglibBridgeGetYear;
  final int Function(ffi.Pointer<TagLibBridgeFile>) taglibBridgeGetTrack;
  final void Function(ffi.Pointer<TagLibBridgeFile>, ffi.Pointer<ffi.Char>)
  taglibBridgeSetTitle;
  final void Function(ffi.Pointer<TagLibBridgeFile>, ffi.Pointer<ffi.Char>)
  taglibBridgeSetArtist;
  final void Function(ffi.Pointer<TagLibBridgeFile>, ffi.Pointer<ffi.Char>)
  taglibBridgeSetAlbum;
  final void Function(ffi.Pointer<TagLibBridgeFile>, ffi.Pointer<ffi.Char>)
  taglibBridgeSetGenre;
  final void Function(ffi.Pointer<TagLibBridgeFile>, ffi.Pointer<ffi.Char>)
  taglibBridgeSetComment;
  final void Function(ffi.Pointer<TagLibBridgeFile>, int) taglibBridgeSetYear;
  final void Function(ffi.Pointer<TagLibBridgeFile>, int) taglibBridgeSetTrack;
  final int Function(ffi.Pointer<TagLibBridgeFile>) taglibBridgeGetDuration;
  final int Function(ffi.Pointer<TagLibBridgeFile>) taglibBridgeGetBitrate;
  final int Function(ffi.Pointer<TagLibBridgeFile>) taglibBridgeGetSampleRate;
  final int Function(ffi.Pointer<TagLibBridgeFile>) taglibBridgeGetChannels;
  final ffi.Pointer<ffi.Char> Function(ffi.Pointer<TagLibBridgeFile>)
  taglibBridgeGetBitrateMode;
  final int Function(ffi.Pointer<TagLibBridgeFile>) taglibBridgeHasCover;
  final ffi.Pointer<TagLibBridgePictures> Function() taglibBridgePicturesCreate;
  final void Function(ffi.Pointer<TagLibBridgePictures>)
  taglibBridgePicturesFree;
  final ffi.Pointer<TagLibBridgePictures> Function(
    ffi.Pointer<TagLibBridgeFile>,
  )
  taglibBridgePicturesGet;
  final int Function(
    ffi.Pointer<TagLibBridgeFile>,
    ffi.Pointer<TagLibBridgePictures>,
  )
  taglibBridgePicturesSet;
  final int Function(ffi.Pointer<TagLibBridgePictures>)
  taglibBridgePicturesSize;
  final int Function(ffi.Pointer<TagLibBridgePictures>, int)
  taglibBridgePicturesDataSize;
  final int Function(
    ffi.Pointer<TagLibBridgePictures>,
    int,
    ffi.Pointer<ffi.Uint8>,
    int,
  )
  taglibBridgePicturesData;
  final ffi.Pointer<ffi.Char> Function(ffi.Pointer<TagLibBridgePictures>, int)
  taglibBridgePicturesMimeType;
  final ffi.Pointer<ffi.Char> Function(ffi.Pointer<TagLibBridgePictures>, int)
  taglibBridgePicturesDescription;
  final ffi.Pointer<ffi.Char> Function(ffi.Pointer<TagLibBridgePictures>, int)
  taglibBridgePicturesPictureType;
  final void Function(
    ffi.Pointer<TagLibBridgePictures>,
    ffi.Pointer<ffi.Uint8>,
    int,
    ffi.Pointer<ffi.Char>,
    ffi.Pointer<ffi.Char>,
    ffi.Pointer<ffi.Char>,
  )
  taglibBridgePicturesAdd;
  final ffi.Pointer<TagLibBridgeProperties> Function()
  taglibBridgePropertiesCreate;
  final void Function(ffi.Pointer<TagLibBridgeProperties>)
  taglibBridgePropertiesFree;
  final ffi.Pointer<TagLibBridgeProperties> Function(
    ffi.Pointer<TagLibBridgeFile>,
  )
  taglibBridgePropertiesGet;
  final ffi.Pointer<TagLibBridgeProperties> Function(
    ffi.Pointer<TagLibBridgeFile>,
    ffi.Pointer<TagLibBridgeProperties>,
  )
  taglibBridgePropertiesSet;
  final int Function(ffi.Pointer<TagLibBridgeProperties>)
  taglibBridgePropertiesSize;
  final ffi.Pointer<ffi.Char> Function(ffi.Pointer<TagLibBridgeProperties>, int)
  taglibBridgePropertiesKey;
  final int Function(ffi.Pointer<TagLibBridgeProperties>, ffi.Pointer<ffi.Char>)
  taglibBridgePropertiesValueCount;
  final ffi.Pointer<ffi.Char> Function(
    ffi.Pointer<TagLibBridgeProperties>,
    ffi.Pointer<ffi.Char>,
    int,
  )
  taglibBridgePropertiesValue;
  final void Function(
    ffi.Pointer<TagLibBridgeProperties>,
    ffi.Pointer<ffi.Char>,
    ffi.Pointer<ffi.Char>,
  )
  taglibBridgePropertiesAdd;
}

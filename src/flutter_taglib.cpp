#include "flutter_taglib.h"
#include <fileref.h>
#include <tfilestream.h>
#include <tag.h>
#include <audioproperties.h>
#include <tvariant.h>
#include <tbytevector.h>
#include <tstring.h>
#include <mpegfile.h>
#include <mpegproperties.h>
#include <xingheader.h>
#include <flacfile.h>
#include <ogg/vorbis/vorbisfile.h>
#include <ogg/opus/opusfile.h>
#include <ogg/speex/speexfile.h>
#include <ogg/flac/oggflacfile.h>
#include <mp4/mp4file.h>
#include <mp4/mp4properties.h>
#include <riff/wav/wavfile.h>
#include <riff/aiff/aifffile.h>
#include <ape/apefile.h>
#include <wavpack/wavpackfile.h>
#include <mpc/mpcfile.h>
#include <trueaudio/trueaudiofile.h>
#include <asf/asffile.h>
#include <matroska/matroskafile.h>
#include <matroska/matroskaproperties.h>
#include <dsf/dsffile.h>
#include <dsdiff/dsdifffile.h>
#include <mod/modfile.h>
#include <s3m/s3mfile.h>
#include <it/itfile.h>
#include <xm/xmfile.h>
#include <tpropertymap.h>
#include <mpeg/id3v2/id3v2tag.h>
#include <mpeg/id3v2/frames/chapterframe.h>
#include <mpeg/id3v2/frames/tableofcontentsframe.h>
#include <mpeg/id3v2/frames/textidentificationframe.h>
#include <matroska/matroskachapters.h>
#include <matroska/matroskachapteredition.h>
#include <matroska/matroskachapter.h>
#include <ogg/xiphcomment.h>

#include <string>
#include <vector>
#include <map>
#include <unordered_map>
#include <algorithm>
#include <cstring>
#include <cstdlib>
#include <cctype>
#include <typeinfo>
#include <typeindex>
#include <atomic>
#include <thread>
#include <memory>
#include <mutex>

#if defined(_WIN32)
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <winhttp.h>
#pragma comment(lib, "winhttp.lib")
#elif defined(__APPLE__)
#import <Foundation/Foundation.h>
#include <pthread/qos.h>
#else
#include <sys/resource.h>
#ifndef __ANDROID__
#include <curl/curl.h>
#endif
#endif

// Itanium ABI toolchains (Android, Apple, Linux) mangle typeid names and need
// cxxabi.h to demangle them. MSVC-targeting compilers, including clang on
// Windows, already report a readable name and ship no cxxabi.h.
#if !defined(_MSC_VER) && defined(__has_include)
#if __has_include(<cxxabi.h>)
#include <cxxabi.h>
#include <cstdlib>
#define FLUTTER_TAGLIB_HAS_CXA_DEMANGLE 1
#endif
#endif

#include <cstdio>
#include <iostream>

#ifdef __ANDROID__
#include <jni.h>
#include <unistd.h>
#include <android/log.h>

#undef JNIEXPORT
#define JNIEXPORT __attribute__((visibility("default")))

#define LOG_TAG "FlutterTaglib"
#define LOGI(...) do {} while(0) // Disable info logs to optimize performance
#define LOGW(...) __android_log_print(ANDROID_LOG_WARN, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

static JavaVM* g_vm = nullptr;
static jobject g_context = nullptr;

#ifdef __ANDROID__
extern "C" JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM* vm, void* reserved) {
    g_vm = vm;
    LOGW("FlutterTaglib JNI_OnLoad: g_vm initialized successfully");
    return JNI_VERSION_1_6;
}

extern "C" JNIEXPORT void JNICALL
Java_com_axel10_flutter_1taglib_FlutterTaglibPlugin_nativeInitContext(JNIEnv* env, jclass clazz, jobject context) {
    if (env && !g_vm) {
        env->GetJavaVM(&g_vm);
        LOGW("FlutterTaglib nativeInitContext: g_vm initialized via GetJavaVM");
    }
    if (g_context != nullptr) {
        env->DeleteGlobalRef(g_context);
        g_context = nullptr;
    }
    if (context != nullptr) {
        g_context = env->NewGlobalRef(context);
        LOGW("FlutterTaglib nativeInitContext: g_context initialized successfully");
    }
}

extern "C" JNIEXPORT void JNICALL
Java_com_axel10_flutter_1taglib_FlutterTaglibPlugin_00024Companion_nativeInitContext(JNIEnv* env, jclass clazz, jobject context) {
    Java_com_axel10_flutter_1taglib_FlutterTaglibPlugin_nativeInitContext(env, clazz, context);
}
#endif

static JNIEnv* get_jni_env() {
    if (!g_vm) {
        LOGE("get_jni_env: g_vm is null");
        return nullptr;
    }
    JNIEnv* env = nullptr;
    jint res = g_vm->GetEnv((void**)&env, JNI_VERSION_1_6);
    if (res == JNI_EDETACHED) {
        #ifdef __ANDROID__
        res = g_vm->AttachCurrentThread(&env, nullptr);
        #else
        res = g_vm->AttachCurrentThread((void**)&env, nullptr);
        #endif
        if (res != JNI_OK) {
            LOGE("get_jni_env: AttachCurrentThread failed with error: %d", res);
            return nullptr;
        }
    } else if (res != JNI_OK) {
        LOGE("get_jni_env: GetEnv failed with error: %d", res);
        return nullptr;
    }
    return env;
}

static void check_and_clear_jni_exception(JNIEnv* env, const char* context) {
    if (env->ExceptionCheck()) {
        jthrowable exc = env->ExceptionOccurred();
        if (exc) {
            env->ExceptionClear();
            jclass excClass = env->GetObjectClass(exc);
            jmethodID toStringMethod = env->GetMethodID(excClass, "toString", "()Ljava/lang/String;");
            if (toStringMethod) {
                jstring jstr = (jstring)env->CallObjectMethod(exc, toStringMethod);
                if (jstr) {
                    const char* str = env->GetStringUTFChars(jstr, nullptr);
                    LOGE("[%s] JNI Exception: %s", context, str);
                    env->ReleaseStringUTFChars(jstr, str);
                    env->DeleteLocalRef(jstr);
                } else {
                    LOGE("[%s] JNI Exception occurred, but toString failed", context);
                }
            } else {
                LOGE("[%s] JNI Exception occurred, but toString method not found", context);
            }
            env->DeleteLocalRef(excClass);
            env->DeleteLocalRef(exc);
        } else {
            env->ExceptionClear();
            LOGE("[%s] ExceptionCheck was true, but ExceptionOccurred returned null", context);
        }
    }
}

static int open_content_uri_fd(const char* uri_str, const char* mode_str) {
    JNIEnv* env = get_jni_env();
    if (!env) {
        LOGE("open_content_uri_fd: JNI env is null");
        return -1;
    }
    if (!g_context) {
        LOGE("open_content_uri_fd: g_context is null");
        return -1;
    }

    // Get ContentResolver
    jclass contextClass = env->GetObjectClass(g_context);
    jmethodID getContentResolverMethod = env->GetMethodID(contextClass, "getContentResolver", "()Landroid/content/ContentResolver;");
    if (!getContentResolverMethod) {
        LOGE("open_content_uri_fd: getContentResolver method not found");
        return -1;
    }
    jobject resolver = env->CallObjectMethod(g_context, getContentResolverMethod);
    if (env->ExceptionCheck()) {
        check_and_clear_jni_exception(env, "getContentResolver");
        return -1;
    }
    if (!resolver) {
        LOGE("open_content_uri_fd: ContentResolver is null");
        return -1;
    }

    // Parse Uri
    jclass uriClass = env->FindClass("android/net/Uri");
    if (!uriClass) {
        LOGE("open_content_uri_fd: Uri class not found");
        return -1;
    }
    jmethodID parseMethod = env->GetStaticMethodID(uriClass, "parse", "(Ljava/lang/String;)Landroid/net/Uri;");
    if (!parseMethod) {
        LOGE("open_content_uri_fd: Uri.parse method not found");
        return -1;
    }
    jstring juri_str = env->NewStringUTF(uri_str);
    jobject uri = env->CallStaticObjectMethod(uriClass, parseMethod, juri_str);
    env->DeleteLocalRef(juri_str);
    if (env->ExceptionCheck()) {
        check_and_clear_jni_exception(env, "Uri.parse");
        return -1;
    }
    if (!uri) {
        LOGE("open_content_uri_fd: Uri parsing returned null");
        return -1;
    }

    // Call resolver.openFileDescriptor(uri, mode)
    jclass resolverClass = env->GetObjectClass(resolver);
    jmethodID openFileDescriptorMethod = env->GetMethodID(resolverClass, "openFileDescriptor", "(Landroid/net/Uri;Ljava/lang/String;)Landroid/os/ParcelFileDescriptor;");
    if (!openFileDescriptorMethod) {
        LOGE("open_content_uri_fd: openFileDescriptor method not found");
        return -1;
    }
    jstring jmode_str = env->NewStringUTF(mode_str);
    jobject pfd = env->CallObjectMethod(resolver, openFileDescriptorMethod, uri, jmode_str);
    env->DeleteLocalRef(jmode_str);
    if (env->ExceptionCheck()) {
        check_and_clear_jni_exception(env, "openFileDescriptor");
        return -1;
    }
    if (!pfd) {
        LOGE("open_content_uri_fd: openFileDescriptor returned null");
        return -1;
    }

    // Get raw fd and detach it
    jclass pfdClass = env->GetObjectClass(pfd);
    jmethodID detachFdMethod = env->GetMethodID(pfdClass, "detachFd", "()I");
    if (!detachFdMethod) {
        LOGE("open_content_uri_fd: detachFd method not found");
        env->DeleteLocalRef(pfd);
        return -1;
    }
    int fd = env->CallIntMethod(pfd, detachFdMethod);
    if (env->ExceptionCheck()) {
        check_and_clear_jni_exception(env, "detachFd");
        env->DeleteLocalRef(pfd);
        return -1;
    }
    env->DeleteLocalRef(pfd);

    return fd;
}
#else
#define LOGI(...) do {} while(0) // Disable info logs on desktop to optimize performance
#define LOGW(...) do { fprintf(stderr, "[FlutterTaglib WARN] "); fprintf(stderr, __VA_ARGS__); fprintf(stderr, "\n"); } while(0)
#define LOGE(...) do { fprintf(stderr, "[FlutterTaglib ERROR] "); fprintf(stderr, __VA_ARGS__); fprintf(stderr, "\n"); } while(0)
#endif

struct TagLibBridgeFile {
    TagLib::IOStream* stream;
    TagLib::FileRef* fileRef;

    // String cache for FFI lifetime safety
    std::string cachedTitle;
    std::string cachedArtist;
    std::string cachedAlbum;
    std::string cachedGenre;
    std::string cachedComment;
    std::string cachedCoverMime;
    std::string cachedBitrateMode;
    std::string cachedFormat;
    bool formatResolved = false;

    int cachedLossless = -1;
    bool losslessResolved = false;

    int cachedHasCover = -1;
    bool hasCoverResolved = false;

    TagLib::ByteVector cachedFrontCover;

    void invalidateCaches() {
        formatResolved = false;
        losslessResolved = false;
        hasCoverResolved = false;
        cachedFrontCover = TagLib::ByteVector();
    }
};

struct TagLibBridgePictures {
    TagLib::List<TagLib::VariantMap> pictures;
    std::vector<TagLib::VariantMap> cachedPictures;
    std::vector<std::string> cachedMimeTypes;
    std::vector<std::string> cachedDescriptions;
    std::vector<std::string> cachedPictureTypes;

    void refreshCache() {
        cachedPictures.clear();
        cachedMimeTypes.clear();
        cachedDescriptions.clear();
        cachedPictureTypes.clear();

        for (const auto& picture : pictures) {
            cachedPictures.push_back(picture);

            auto mimeVar = picture["mimeType"];
            cachedMimeTypes.push_back(
                mimeVar.isEmpty() ? std::string() : mimeVar.toString().to8Bit(true)
            );

            auto descVar = picture["description"];
            cachedDescriptions.push_back(
                descVar.isEmpty() ? std::string() : descVar.toString().to8Bit(true)
            );

            auto typeVar = picture["pictureType"];
            cachedPictureTypes.push_back(
                typeVar.isEmpty() ? std::string() : typeVar.toString().to8Bit(true)
            );
        }
    }
};

static TagLib::List<TagLib::VariantMap> read_picture_list(TagLibBridgeFile* file) {
    if (!file || !file->fileRef || file->fileRef->isNull()) {
        return TagLib::List<TagLib::VariantMap>();
    }
    return file->fileRef->complexProperties("PICTURE");
}

static const TagLib::VariantMap* picture_at(const TagLibBridgePictures* pictures, int index) {
    if (!pictures || index < 0 || index >= static_cast<int>(pictures->cachedPictures.size())) {
        return nullptr;
    }
    return &pictures->cachedPictures[static_cast<size_t>(index)];
}

// Returns the runtime class name of a TagLib::File subclass, e.g.
// "TagLib::FLAC::File". MSVC already reports a readable name; the Itanium ABI
// (Clang/GCC on Android, Apple and Linux) reports a mangled name that needs
// demangling. Returns an empty string when the name is unavailable.
static std::string runtime_class_name(const TagLib::File* filePtr) {
    if (!filePtr) return std::string();
    const char* rawName = typeid(*filePtr).name();
    if (!rawName) return std::string();

#ifdef FLUTTER_TAGLIB_HAS_CXA_DEMANGLE
    int status = 0;
    char* demangled = abi::__cxa_demangle(rawName, nullptr, nullptr, &status);
    if (status == 0 && demangled) {
        std::string result(demangled);
        std::free(demangled);
        return result;
    }
    if (demangled) std::free(demangled);
    return std::string();
#else
    return std::string(rawName);
#endif
}

// Derives a format token from a TagLib class name for formats the explicit
// dispatch below does not name, so newly supported TagLib formats still report
// something useful instead of nothing. "TagLib::Shorten::File" yields "SHORTEN".
static std::string format_token_from_class_name(const std::string& className) {
    static const std::string fileSuffix = "::File";
    if (className.size() <= fileSuffix.size()) return std::string();
    if (className.compare(className.size() - fileSuffix.size(), fileSuffix.size(), fileSuffix) != 0) {
        return std::string();
    }

    // "class TagLib::Ogg::Speex::File" -> "class TagLib::Ogg::Speex" -> "Speex"
    std::string head = className.substr(0, className.size() - fileSuffix.size());
    size_t separator = head.rfind("::");
    std::string token = (separator == std::string::npos) ? head : head.substr(separator + 2);
    if (token.empty() || token == "TagLib") return std::string();

    for (auto& character : token) {
        character = static_cast<char>(std::toupper(static_cast<unsigned char>(character)));
    }
    return token;
}

// Maps a Matroska CodecID to the format token the rest of this bridge uses.
// Prefix matching absorbs the codec-ID variants ("A_AAC/MPEG4/LC",
// "A_AC3/BSID9", "A_PCM/INT/LIT"). Returns an empty string for codecs this
// table does not name, so the caller can fall back to the container name.
static std::string matroska_codec_token(const TagLib::String& codecId) {
    const std::string codec = codecId.to8Bit(true);
    static const std::pair<const char*, const char*> prefixTokens[] = {
        {"A_OPUS", "OPUS"},
        {"A_VORBIS", "VORBIS"},
        {"A_AAC", "AAC"},
        {"A_MPEG/L3", "MP3"},
        {"A_MPEG/L2", "MP2"},
        {"A_MPEG/L1", "MP1"},
        {"A_FLAC", "FLAC"},
        {"A_ALAC", "ALAC"},
        {"A_PCM", "PCM"},
        {"A_EAC3", "EAC3"},
        {"A_AC3", "AC3"},
        {"A_DTS", "DTS"},
        {"A_TRUEHD", "TRUEHD"},
        {"A_MLP", "MLP"},
        {"A_WAVPACK4", "WAVPACK"},
        {"A_TTA1", "TTA"},
        {"A_MPC", "MPC"},
    };
    for (const auto& entry : prefixTokens) {
        if (codec.compare(0, std::strlen(entry.first), entry.first) == 0) {
            return entry.second;
        }
    }
    return std::string();
}

// Maps each concrete TagLib file class to its format token. Matching the exact
// runtime type turns format detection into a single hash lookup instead of a
// chain of dynamic_casts, and makes the order of entries irrelevant. Formats
// whose token depends on the codec (MPEG, MP4, Matroska) are resolved
// separately below.
static const std::unordered_map<std::type_index, const char*>& format_token_table() {
    static const std::unordered_map<std::type_index, const char*> table = {
        {std::type_index(typeid(TagLib::FLAC::File)), "FLAC"},
        {std::type_index(typeid(TagLib::Ogg::FLAC::File)), "OGGFLAC"},
        {std::type_index(typeid(TagLib::Ogg::Vorbis::File)), "VORBIS"},
        {std::type_index(typeid(TagLib::Ogg::Opus::File)), "OPUS"},
        {std::type_index(typeid(TagLib::Ogg::Speex::File)), "SPEEX"},
        {std::type_index(typeid(TagLib::RIFF::WAV::File)), "WAV"},
        {std::type_index(typeid(TagLib::RIFF::AIFF::File)), "AIFF"},
        {std::type_index(typeid(TagLib::APE::File)), "APE"},
        {std::type_index(typeid(TagLib::WavPack::File)), "WAVPACK"},
        {std::type_index(typeid(TagLib::MPC::File)), "MPC"},
        {std::type_index(typeid(TagLib::TrueAudio::File)), "TTA"},
        {std::type_index(typeid(TagLib::ASF::File)), "WMA"},
        {std::type_index(typeid(TagLib::DSF::File)), "DSF"},
        {std::type_index(typeid(TagLib::DSDIFF::File)), "DFF"},
        {std::type_index(typeid(TagLib::Mod::File)), "MOD"},
        {std::type_index(typeid(TagLib::S3M::File)), "S3M"},
        {std::type_index(typeid(TagLib::IT::File)), "IT"},
        {std::type_index(typeid(TagLib::XM::File)), "XM"},
    };
    return table;
}

// Tri-state verdict for taglib_bridge_is_lossless.
enum LosslessVerdict { kLossy = 0, kLossless = 1, kLosslessUnknown = -1 };

// Formats whose lossless-ness follows from the format alone. The remaining ones
// (MP4, Matroska, WAV, AIFF, WavPack, ASF) can carry either kind of stream and
// are resolved from their audio properties instead.
static const std::unordered_map<std::type_index, int>& lossless_table() {
    static const std::unordered_map<std::type_index, int> table = {
        {std::type_index(typeid(TagLib::MPEG::File)), kLossy},
        {std::type_index(typeid(TagLib::FLAC::File)), kLossless},
        {std::type_index(typeid(TagLib::Ogg::FLAC::File)), kLossless},
        {std::type_index(typeid(TagLib::Ogg::Vorbis::File)), kLossy},
        {std::type_index(typeid(TagLib::Ogg::Opus::File)), kLossy},
        {std::type_index(typeid(TagLib::Ogg::Speex::File)), kLossy},
        {std::type_index(typeid(TagLib::APE::File)), kLossless},
        {std::type_index(typeid(TagLib::MPC::File)), kLossy},
        {std::type_index(typeid(TagLib::TrueAudio::File)), kLossless},
        // DSD stores a raw 1-bit stream; DST compression inside DFF is lossless.
        {std::type_index(typeid(TagLib::DSF::File)), kLossless},
        {std::type_index(typeid(TagLib::DSDIFF::File)), kLossless},
        // Tracker formats sequence sampled instruments, so neither verdict applies.
        {std::type_index(typeid(TagLib::Mod::File)), kLosslessUnknown},
        {std::type_index(typeid(TagLib::S3M::File)), kLosslessUnknown},
        {std::type_index(typeid(TagLib::IT::File)), kLosslessUnknown},
        {std::type_index(typeid(TagLib::XM::File)), kLosslessUnknown},
    };
    return table;
}

// AIFF-C compression identifiers that store PCM verbatim. Every other AIFF-C
// compression in common use (ima4, ulaw, MAC3/MAC6, GSM, QDMC, mp3) is lossy.
static bool is_lossless_aifc_compression(const TagLib::ByteVector& compression) {
    static const char* const losslessTypes[] = {
        "NONE", "sowt", "twos", "raw ", "in24", "in32",
        "fl32", "FL32", "fl64", "FL64",
    };
    for (const char* type : losslessTypes) {
        if (compression == TagLib::ByteVector(type, 4)) return true;
    }
    return false;
}

static TagLib::VariantMap build_picture_map(
    const uint8_t* data,
    uint32_t size,
    const char* mime_type,
    const char* picture_type,
    const char* description
) {
    TagLib::VariantMap picMap;
    picMap["data"] = TagLib::ByteVector(reinterpret_cast<const char*>(data), size);
    picMap["mimeType"] = TagLib::String(mime_type ? mime_type : "image/jpeg", TagLib::String::UTF8);
    picMap["pictureType"] = TagLib::String(picture_type ? picture_type : "Front Cover", TagLib::String::UTF8);
    if (description && *description != '\0') {
        picMap["description"] = TagLib::String(description, TagLib::String::UTF8);
    }
    return picMap;
}

extern "C" {

#ifdef __ANDROID__
JNIEXPORT void JNICALL Java_com_axel10_flutter_1taglib_FlutterTaglibPlugin_setNativeContext(JNIEnv* env, jobject thiz, jobject context) {
    if (g_context) {
        env->DeleteGlobalRef(g_context);
    }
    g_context = env->NewGlobalRef(context);
}

JNIEXPORT void JNICALL Java_com_axel10_flutter_1taglib_FlutterTaglibPlugin_clearNativeContext(JNIEnv* env, jobject thiz) {
    if (g_context) {
        env->DeleteGlobalRef(g_context);
        g_context = nullptr;
    }
}
#endif

static void resolve_read_style(int read_style, bool& readAudioProperties, TagLib::AudioProperties::ReadStyle& style) {
    switch (read_style) {
        case 0: // Fast
            readAudioProperties = true;
            style = TagLib::AudioProperties::Fast;
            break;
        case 1: // Average
            readAudioProperties = true;
            style = TagLib::AudioProperties::Average;
            break;
        case 2: // Accurate
            readAudioProperties = true;
            style = TagLib::AudioProperties::Accurate;
            break;
        case 3: // None
            readAudioProperties = false;
            style = TagLib::AudioProperties::Fast;
            break;
        default:
            readAudioProperties = true;
            style = TagLib::AudioProperties::Average;
            break;
    }
}

TagLibBridgeFile* taglib_bridge_open_with_style(const char* filepath, int read_style) {
    if (!filepath) {
        LOGE("taglib_bridge_open: filepath is null");
        return nullptr;
    }

#ifdef __ANDROID__
    if (std::strncmp(filepath, "content://", 10) == 0) {
        LOGI("taglib_bridge_open: opening content URI: %s", filepath);
        int fd = open_content_uri_fd(filepath, "rw");
        if (fd == -1) {
            LOGW("taglib_bridge_open: failed to open content URI in 'rw' mode, falling back to 'r' (read-only) mode");
            fd = open_content_uri_fd(filepath, "r");
        }
        if (fd != -1) {
            return taglib_bridge_open_fd_with_style(fd, read_style);
        }
        LOGE("taglib_bridge_open: failed to open content URI fd for: %s", filepath);
        return nullptr;
    }
#endif

    LOGI("taglib_bridge_open: opening file path: %s with style: %d", filepath, read_style);
    try {
        bool readAudioProps = true;
        TagLib::AudioProperties::ReadStyle style = TagLib::AudioProperties::Average;
        resolve_read_style(read_style, readAudioProps, style);

#ifdef _WIN32
        TagLib::String pathStr(filepath, TagLib::String::UTF8);
        TagLib::FileName filename(pathStr.toWString().c_str());
#else
        TagLib::FileName filename = filepath;
#endif
        auto fileRef = new TagLib::FileRef(filename, readAudioProps, style);
        if (fileRef->isNull()) {
            delete fileRef;
#ifdef __ANDROID__
            // POSIX open failed for Android filepath.
#endif
            LOGE("taglib_bridge_open: fileRef is null (invalid file or format) for: %s", filepath);
            return nullptr;
        }

        auto bridge = new TagLibBridgeFile();
        bridge->stream = nullptr;
        bridge->fileRef = fileRef;
        LOGI("taglib_bridge_open: successfully opened file: %s", filepath);
        return bridge;
    } catch (const std::exception& e) {
        LOGE("taglib_bridge_open: std::exception caught for %s: %s", filepath, e.what());
        return nullptr;
    } catch (...) {
        LOGE("taglib_bridge_open: unknown exception caught for %s", filepath);
        return nullptr;
    }
}

TagLibBridgeFile* taglib_bridge_open(const char* filepath) {
    return taglib_bridge_open_with_style(filepath, 1);
}

TagLibBridgeFile* taglib_bridge_open_fd_with_style(int fd, int read_style) {
    LOGI("taglib_bridge_open_fd: opening fd: %d with style: %d", fd, read_style);
    try {
        auto stream = new TagLib::FileStream(fd, false);
        if (!stream->isOpen()) {
            LOGW("taglib_bridge_open_fd: fd %d cannot be opened as read-write, trying read-only", fd);
            delete stream;
            stream = new TagLib::FileStream(fd, true);
        }
        if (!stream->isOpen()) {
            LOGE("taglib_bridge_open_fd: fd %d failed to open stream", fd);
            delete stream;
            return nullptr;
        }

        if (stream->readOnly()) {
            LOGW("taglib_bridge_open_fd: fd %d is opened in READ-ONLY mode. Metadata changes will not be saved!", fd);
        } else {
            LOGI("taglib_bridge_open_fd: fd %d opened successfully in read-write mode", fd);
        }

        bool readAudioProps = true;
        TagLib::AudioProperties::ReadStyle style = TagLib::AudioProperties::Average;
        resolve_read_style(read_style, readAudioProps, style);

        auto fileRef = new TagLib::FileRef(stream, readAudioProps, style);
        if (fileRef->isNull()) {
            LOGE("taglib_bridge_open_fd: fileRef is null (invalid file or format) for fd: %d", fd);
            delete fileRef;
            delete stream;
            return nullptr;
        }

        auto bridge = new TagLibBridgeFile();
        bridge->stream = stream;
        bridge->fileRef = fileRef;
        return bridge;
    } catch (const std::exception& e) {
        LOGE("taglib_bridge_open_fd: std::exception caught: %s", e.what());
        return nullptr;
    } catch (...) {
        LOGE("taglib_bridge_open_fd: unknown exception caught");
        return nullptr;
    }
}

TagLibBridgeFile* taglib_bridge_open_fd(int fd) {
    return taglib_bridge_open_fd_with_style(fd, 1);
}

} // extern "C"

// --- HTTP Range Streaming Support ---

static std::map<std::string, std::string> parse_headers_json(const char* json_str) {
    std::map<std::string, std::string> headers;
    if (!json_str || std::strlen(json_str) == 0) return headers;

    const char* p = json_str;
    while (*p && *p != '{') p++;
    if (*p == '{') p++;

    while (*p) {
        while (*p && (*p == ' ' || *p == '\t' || *p == '\r' || *p == '\n' || *p == ',')) p++;
        if (*p == '}' || *p == '\0') break;

        if (*p != '"') break;
        p++; // skip opening quote
        std::string key;
        while (*p && *p != '"') {
            if (*p == '\\' && *(p + 1)) {
                p++;
                if (*p == 'n') key += '\n';
                else if (*p == 'r') key += '\r';
                else if (*p == 't') key += '\t';
                else if (*p == '"') key += '"';
                else if (*p == '\\') key += '\\';
                else key += *p;
            } else {
                key += *p;
            }
            p++;
        }
        if (*p == '"') p++; // skip closing quote

        while (*p && (*p == ' ' || *p == '\t')) p++;
        if (*p != ':') break;
        p++; // skip colon

        while (*p && (*p == ' ' || *p == '\t')) p++;

        if (*p != '"') break;
        p++; // skip opening quote
        std::string val;
        while (*p && *p != '"') {
            if (*p == '\\' && *(p + 1)) {
                p++;
                if (*p == 'n') val += '\n';
                else if (*p == 'r') val += '\r';
                else if (*p == 't') val += '\t';
                else if (*p == '"') val += '"';
                else if (*p == '\\') val += '\\';
                else val += *p;
            } else {
                val += *p;
            }
            p++;
        }
        if (*p == '"') p++; // skip closing quote

        if (!key.empty()) {
            headers[key] = val;
        }
    }
    return headers;
}

// Keeps one stream's connection alive between its range requests, so each
// block doesn't pay a new TCP (and TLS) handshake. Android's HttpURLConnection
// and Apple's shared NSURLSession already pool connections on their own.
struct HttpRangeConnection {
#if defined(_WIN32)
    HINTERNET session = nullptr;
    HINTERNET connect = nullptr;

    ~HttpRangeConnection() {
        if (connect) WinHttpCloseHandle(connect);
        if (session) WinHttpCloseHandle(session);
    }
#elif !defined(__APPLE__) && !defined(__ANDROID__)
    CURL* curl = nullptr;

    ~HttpRangeConnection() {
        if (curl) curl_easy_cleanup(curl);
    }
#endif
};

#if defined(__APPLE__)
static bool apple_fetch_http_range(
    const std::string& url,
    const std::map<std::string, std::string>& headers,
    int64_t offset,
    int64_t length,
    int timeout_ms,
    int64_t& out_total_length,
    std::vector<uint8_t>& out_data,
    std::string& out_error
) {
    @autoreleasepool {
        NSString* urlStr = [NSString stringWithUTF8String:url.c_str()];
        if (!urlStr) {
            out_error = "Invalid UTF-8 URL";
            return false;
        }
        NSURL* nsUrl = [NSURL URLWithString:urlStr];
        if (!nsUrl) {
            out_error = "Invalid URL";
            return false;
        }

        NSMutableURLRequest* request = [NSMutableURLRequest requestWithURL:nsUrl];
        [request setHTTPMethod:@"GET"];
        [request setTimeoutInterval:(timeout_ms > 0 ? (timeout_ms / 1000.0) : 15.0)];

        if (offset >= 0 && length > 0) {
            int64_t end = offset + length - 1;
            NSString* rangeVal = [NSString stringWithFormat:@"bytes=%lld-%lld", (long long)offset, (long long)end];
            [request setValue:rangeVal forHTTPHeaderField:@"Range"];
        }

        for (const auto& kv : headers) {
            NSString* k = [NSString stringWithUTF8String:kv.first.c_str()];
            NSString* v = [NSString stringWithUTF8String:kv.second.c_str()];
            if (k && v) {
                [request setValue:v forHTTPHeaderField:k];
            }
        }

        dispatch_semaphore_t sema = dispatch_semaphore_create(0);
        __block NSData* resData = nil;
        __block NSInteger statusCode = 0;
        __block int64_t parsedTotalLength = -1;
        __block int64_t expectedContentLength = -1;
        __block NSString* errDesc = nil;

        NSURLSession* session = [NSURLSession sharedSession];
        NSURLSessionDataTask* task = [session dataTaskWithRequest:request completionHandler:^(NSData* data, NSURLResponse* response, NSError* error) {
            if (error) {
                errDesc = [error localizedDescription];
            }
            if ([response isKindOfClass:[NSHTTPURLResponse class]]) {
                NSHTTPURLResponse* httpRes = (NSHTTPURLResponse*)response;
                statusCode = [httpRes statusCode];
                expectedContentLength = [httpRes expectedContentLength];

                NSDictionary* allHeaders = [httpRes allHeaderFields];
                NSString* contentRange = [allHeaders objectForKey:@"Content-Range"];
                if (!contentRange) contentRange = [allHeaders objectForKey:@"content-range"];
                if (contentRange) {
                    NSRange slashRange = [contentRange rangeOfString:@"/"];
                    if (slashRange.location != NSNotFound) {
                        NSString* totalStr = [contentRange substringFromIndex:slashRange.location + 1];
                        totalStr = [totalStr stringByTrimmingCharactersInSet:[NSCharacterSet whitespaceCharacterSet]];
                        if (![totalStr isEqualToString:@"*"]) {
                            parsedTotalLength = [totalStr longLongValue];
                        }
                    }
                }
            }
            if (data) {
                resData = [data copy];
            }
            dispatch_semaphore_signal(sema);
        }];
        [task resume];

        long timeoutWait = dispatch_semaphore_wait(sema, dispatch_time(DISPATCH_TIME_NOW, (int64_t)((timeout_ms > 0 ? timeout_ms : 15000) * NSEC_PER_MSEC)));

        if (timeoutWait != 0) {
            [task cancel];
            out_error = "HTTP request timed out";
            return false;
        }
        if (errDesc) {
            out_error = [errDesc UTF8String];
            return false;
        }
        if (statusCode != 200 && statusCode != 206) {
            out_error = "HTTP status " + std::to_string(statusCode);
            return false;
        }

        if (offset > 0 && statusCode == 200) {
            out_error = "Server does not support HTTP Range requests (returned 200 OK for offset > 0)";
            return false;
        }

        if (parsedTotalLength > 0) {
            out_total_length = parsedTotalLength;
        } else if (expectedContentLength > 0 && statusCode == 200) {
            out_total_length = expectedContentLength;
        }

        if (resData && [resData length] > 0) {
            NSUInteger actualLen = [resData length];
            if (statusCode == 200 && length > 0 && actualLen > (NSUInteger)length) {
                actualLen = (NSUInteger)length;
            }
            const uint8_t* bytes = (const uint8_t*)[resData bytes];
            out_data.assign(bytes, bytes + actualLen);
        }
        return true;
    }
}
#endif

#ifdef __ANDROID__
static bool android_fetch_http_range(
    const std::string& url,
    const std::map<std::string, std::string>& headers,
    int64_t offset,
    int64_t length,
    int timeout_ms,
    int64_t& out_total_length,
    std::vector<uint8_t>& out_data,
    std::string& out_error
) {
    JNIEnv* env = get_jni_env();
    if (!env) {
        out_error = "JNI env is null";
        return false;
    }

    jclass urlClass = env->FindClass("java/net/URL");
    if (!urlClass) {
        check_and_clear_jni_exception(env, "android_fetch_http_range: FindClass URL");
        out_error = "FindClass URL failed";
        return false;
    }

    jmethodID urlCtor = env->GetMethodID(urlClass, "<init>", "(Ljava/lang/String;)V");
    jmethodID openConnMethod = env->GetMethodID(urlClass, "openConnection", "()Ljava/net/URLConnection;");

    jstring jUrlStr = env->NewStringUTF(url.c_str());
    jobject jUrl = env->NewObject(urlClass, urlCtor, jUrlStr);
    env->DeleteLocalRef(jUrlStr);

    if (!jUrl) {
        check_and_clear_jni_exception(env, "android_fetch_http_range: URL init");
        env->DeleteLocalRef(urlClass);
        out_error = "Failed to create java.net.URL";
        return false;
    }

    jobject jConn = env->CallObjectMethod(jUrl, openConnMethod);
    env->DeleteLocalRef(jUrl);
    env->DeleteLocalRef(urlClass);

    if (!jConn) {
        check_and_clear_jni_exception(env, "android_fetch_http_range: openConnection");
        out_error = "Failed to open connection";
        return false;
    }

    jclass connClass = env->GetObjectClass(jConn);
    jmethodID setReqPropMethod = env->GetMethodID(connClass, "setRequestProperty", "(Ljava/lang/String;Ljava/lang/String;)V");
    jmethodID setConnTimeoutMethod = env->GetMethodID(connClass, "setConnectTimeout", "(I)V");
    jmethodID setReadTimeoutMethod = env->GetMethodID(connClass, "setReadTimeout", "(I)V");
    jmethodID getRespCodeMethod = env->GetMethodID(connClass, "getResponseCode", "()I");
    jmethodID getHeaderFieldMethod = env->GetMethodID(connClass, "getHeaderField", "(Ljava/lang/String;)Ljava/lang/String;");
    jmethodID getInputStreamMethod = env->GetMethodID(connClass, "getInputStream", "()Ljava/io/InputStream;");
    jmethodID disconnectMethod = env->GetMethodID(connClass, "disconnect", "()V");

    int timeout = timeout_ms > 0 ? timeout_ms : 15000;
    env->CallVoidMethod(jConn, setConnTimeoutMethod, timeout);
    env->CallVoidMethod(jConn, setReadTimeoutMethod, timeout);

    if (offset >= 0 && length > 0) {
        char rangeBuf[64];
        snprintf(rangeBuf, sizeof(rangeBuf), "bytes=%lld-%lld", (long long)offset, (long long)(offset + length - 1));
        jstring jRangeKey = env->NewStringUTF("Range");
        jstring jRangeVal = env->NewStringUTF(rangeBuf);
        env->CallVoidMethod(jConn, setReqPropMethod, jRangeKey, jRangeVal);
        env->DeleteLocalRef(jRangeKey);
        env->DeleteLocalRef(jRangeVal);
    }

    for (const auto& kv : headers) {
        jstring jKey = env->NewStringUTF(kv.first.c_str());
        jstring jVal = env->NewStringUTF(kv.second.c_str());
        env->CallVoidMethod(jConn, setReqPropMethod, jKey, jVal);
        env->DeleteLocalRef(jKey);
        env->DeleteLocalRef(jVal);
    }

    jint respCode = env->CallIntMethod(jConn, getRespCodeMethod);
    if (env->ExceptionCheck()) {
        check_and_clear_jni_exception(env, "android_fetch_http_range: getResponseCode");
        env->DeleteLocalRef(connClass);
        env->DeleteLocalRef(jConn);
        out_error = "Connection exception";
        return false;
    }

    if (respCode != 200 && respCode != 206) {
        if (disconnectMethod) env->CallVoidMethod(jConn, disconnectMethod);
        env->DeleteLocalRef(connClass);
        env->DeleteLocalRef(jConn);
        out_error = "HTTP status " + std::to_string(respCode);
        return false;
    }

    if (offset > 0 && respCode == 200) {
        if (disconnectMethod) env->CallVoidMethod(jConn, disconnectMethod);
        env->DeleteLocalRef(connClass);
        env->DeleteLocalRef(jConn);
        out_error = "Server does not support HTTP Range requests (returned 200 OK for offset > 0)";
        return false;
    }

    jstring jCrKey = env->NewStringUTF("Content-Range");
    jstring jCrVal = (jstring)env->CallObjectMethod(jConn, getHeaderFieldMethod, jCrKey);
    env->DeleteLocalRef(jCrKey);
    if (jCrVal) {
        const char* crStr = env->GetStringUTFChars(jCrVal, nullptr);
        if (crStr) {
            const char* slash = strchr(crStr, '/');
            if (slash && *(slash + 1) != '*' && *(slash + 1) != '\0') {
                out_total_length = strtoll(slash + 1, nullptr, 10);
            }
            env->ReleaseStringUTFChars(jCrVal, crStr);
        }
        env->DeleteLocalRef(jCrVal);
    }

    if (out_total_length <= 0) {
        jstring jClKey = env->NewStringUTF("Content-Length");
        jstring jClVal = (jstring)env->CallObjectMethod(jConn, getHeaderFieldMethod, jClKey);
        env->DeleteLocalRef(jClKey);
        if (jClVal) {
            const char* clStr = env->GetStringUTFChars(jClVal, nullptr);
            if (clStr && respCode == 200) {
                out_total_length = strtoll(clStr, nullptr, 10);
            }
            if (clStr) env->ReleaseStringUTFChars(jClVal, clStr);
            env->DeleteLocalRef(jClVal);
        }
    }

    jobject jInStream = env->CallObjectMethod(jConn, getInputStreamMethod);
    if (!jInStream || env->ExceptionCheck()) {
        check_and_clear_jni_exception(env, "android_fetch_http_range: getInputStream");
        if (disconnectMethod) env->CallVoidMethod(jConn, disconnectMethod);
        env->DeleteLocalRef(connClass);
        env->DeleteLocalRef(jConn);
        out_error = "Failed to get InputStream";
        return false;
    }

    jclass inStreamClass = env->GetObjectClass(jInStream);
    jmethodID readMethod = env->GetMethodID(inStreamClass, "read", "([BII)I");
    jmethodID closeInMethod = env->GetMethodID(inStreamClass, "close", "()V");

    const int chunkBufSize = 16384;
    jbyteArray jChunk = env->NewByteArray(chunkBufSize);

    while (true) {
        if (respCode == 200 && length > 0 && out_data.size() >= (size_t)length) {
            break;
        }
        int toRead = chunkBufSize;
        if (respCode == 200 && length > 0 && out_data.size() + toRead > (size_t)length) {
            toRead = (int)((size_t)length - out_data.size());
        }
        jint bytesRead = env->CallIntMethod(jInStream, readMethod, jChunk, 0, toRead);
        if (bytesRead <= 0) break;

        jbyte* chunkBytes = env->GetByteArrayElements(jChunk, nullptr);
        out_data.insert(out_data.end(), (const uint8_t*)chunkBytes, (const uint8_t*)chunkBytes + bytesRead);
        env->ReleaseByteArrayElements(jChunk, chunkBytes, JNI_ABORT);
    }

    env->DeleteLocalRef(jChunk);
    env->CallVoidMethod(jInStream, closeInMethod);
    env->DeleteLocalRef(jInStream);
    env->DeleteLocalRef(inStreamClass);

    if (disconnectMethod) {
        env->CallVoidMethod(jConn, disconnectMethod);
    }
    env->DeleteLocalRef(connClass);
    env->DeleteLocalRef(jConn);

    return true;
}
#endif

#if defined(_WIN32)
static bool windows_fetch_http_range(
    HttpRangeConnection& connection,
    const std::string& url,
    const std::map<std::string, std::string>& headers,
    int64_t offset,
    int64_t length,
    int timeout_ms,
    int64_t& out_total_length,
    std::vector<uint8_t>& out_data,
    std::string& out_error
) {
    std::wstring wUrl;
    int wlen = MultiByteToWideChar(CP_UTF8, 0, url.c_str(), -1, NULL, 0);
    if (wlen > 0) {
        wUrl.resize(wlen - 1);
        MultiByteToWideChar(CP_UTF8, 0, url.c_str(), -1, &wUrl[0], wlen);
    } else {
        out_error = "URL UTF-8 conversion failed";
        return false;
    }

    URL_COMPONENTS urlComp = {0};
    urlComp.dwStructSize = sizeof(urlComp);
    urlComp.dwHostNameLength = (DWORD)-1;
    urlComp.dwUrlPathLength = (DWORD)-1;
    urlComp.dwExtraInfoLength = (DWORD)-1;

    if (!WinHttpCrackUrl(wUrl.c_str(), (DWORD)wcslen(wUrl.c_str()), 0, &urlComp)) {
        out_error = "WinHttpCrackUrl failed";
        return false;
    }

    std::wstring host(urlComp.lpszHostName, urlComp.dwHostNameLength);
    std::wstring path(urlComp.lpszUrlPath, urlComp.dwUrlPathLength + urlComp.dwExtraInfoLength);

    bool isHttps = (urlComp.nScheme == INTERNET_SCHEME_HTTPS);

    if (!connection.session) {
        connection.session = WinHttpOpen(L"flutter_taglib/1.0",
            WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
            WINHTTP_NO_PROXY_NAME,
            WINHTTP_NO_PROXY_BYPASS, 0);
        if (!connection.session) {
            out_error = "WinHttpOpen failed";
            return false;
        }

        int timeout = timeout_ms > 0 ? timeout_ms : 15000;
        WinHttpSetTimeouts(connection.session, timeout, timeout, timeout, timeout);
    }

    if (!connection.connect) {
        connection.connect = WinHttpConnect(connection.session, host.c_str(), urlComp.nPort, 0);
        if (!connection.connect) {
            out_error = "WinHttpConnect failed";
            return false;
        }
    }

    DWORD flags = isHttps ? WINHTTP_FLAG_SECURE : 0;
    HINTERNET hRequest = WinHttpOpenRequest(connection.connect, L"GET", path.c_str(),
        NULL, WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES, flags);
    if (!hRequest) {
        out_error = "WinHttpOpenRequest failed";
        return false;
    }

    if (offset >= 0 && length > 0) {
        int64_t end = offset + length - 1;
        std::wstring rangeHeader = L"Range: bytes=" + std::to_wstring(offset) + L"-" + std::to_wstring(end) + L"\r\n";
        WinHttpAddRequestHeaders(hRequest, rangeHeader.c_str(), (DWORD)-1, WINHTTP_ADDREQ_FLAG_ADD | WINHTTP_ADDREQ_FLAG_REPLACE);
    }

    for (const auto& kv : headers) {
        std::wstring wk, wv;
        int klen = MultiByteToWideChar(CP_UTF8, 0, kv.first.c_str(), -1, NULL, 0);
        if (klen > 0) { wk.resize(klen - 1); MultiByteToWideChar(CP_UTF8, 0, kv.first.c_str(), -1, &wk[0], klen); }
        int vlen = MultiByteToWideChar(CP_UTF8, 0, kv.second.c_str(), -1, NULL, 0);
        if (vlen > 0) { wv.resize(vlen - 1); MultiByteToWideChar(CP_UTF8, 0, kv.second.c_str(), -1, &wv[0], vlen); }
        std::wstring wHeader = wk.c_str() + std::wstring(L": ") + wv.c_str() + L"\r\n";
        WinHttpAddRequestHeaders(hRequest, wHeader.c_str(), (DWORD)-1, WINHTTP_ADDREQ_FLAG_ADD | WINHTTP_ADDREQ_FLAG_REPLACE);
    }

    if (!WinHttpSendRequest(hRequest, WINHTTP_NO_ADDITIONAL_HEADERS, 0, WINHTTP_NO_REQUEST_DATA, 0, 0, 0) ||
        !WinHttpReceiveResponse(hRequest, NULL)) {
        WinHttpCloseHandle(hRequest);
        out_error = "WinHttpSendRequest or ReceiveResponse failed";
        return false;
    }

    DWORD statusCode = 0;
    DWORD statusSize = sizeof(statusCode);
    WinHttpQueryHeaders(hRequest, WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER,
        WINHTTP_HEADER_NAME_BY_INDEX, &statusCode, &statusSize, WINHTTP_NO_HEADER_INDEX);

    if (statusCode != 200 && statusCode != 206) {
        WinHttpCloseHandle(hRequest);
        out_error = "HTTP status " + std::to_string(statusCode);
        return false;
    }

    if (offset > 0 && statusCode == 200) {
        WinHttpCloseHandle(hRequest);
        out_error = "Server does not support HTTP Range requests (returned 200 OK for offset > 0)";
        return false;
    }

    DWORD crLen = 0;
    WinHttpQueryHeaders(hRequest, WINHTTP_QUERY_CUSTOM, L"Content-Range", NULL, &crLen, WINHTTP_NO_HEADER_INDEX);
    if (GetLastError() == ERROR_INSUFFICIENT_BUFFER && crLen > 0) {
        std::wstring crStr(crLen / sizeof(wchar_t), 0);
        if (WinHttpQueryHeaders(hRequest, WINHTTP_QUERY_CUSTOM, L"Content-Range", &crStr[0], &crLen, WINHTTP_NO_HEADER_INDEX)) {
            size_t slash = crStr.find(L'/');
            if (slash != std::wstring::npos && slash + 1 < crStr.size() && crStr[slash + 1] != L'*') {
                out_total_length = _wcstoi64(crStr.c_str() + slash + 1, NULL, 10);
            }
        }
    }

    if (out_total_length <= 0 && statusCode == 200) {
        DWORD clLen = 0;
        WinHttpQueryHeaders(hRequest, WINHTTP_QUERY_CONTENT_LENGTH | WINHTTP_QUERY_FLAG_NUMBER,
            WINHTTP_HEADER_NAME_BY_INDEX, &clLen, &statusSize, WINHTTP_NO_HEADER_INDEX);
        out_total_length = clLen;
    }

    DWORD bytesAvailable = 0;
    while (WinHttpQueryDataAvailable(hRequest, &bytesAvailable) && bytesAvailable > 0) {
        if (statusCode == 200 && length > 0 && out_data.size() >= (size_t)length) {
            break;
        }
        DWORD toRead = bytesAvailable;
        if (statusCode == 200 && length > 0 && out_data.size() + toRead > (size_t)length) {
            toRead = (DWORD)((size_t)length - out_data.size());
        }
        size_t currentSize = out_data.size();
        out_data.resize(currentSize + toRead);
        DWORD bytesRead = 0;
        if (!WinHttpReadData(hRequest, &out_data[currentSize], toRead, &bytesRead)) {
            out_data.resize(currentSize);
            break;
        }
        out_data.resize(currentSize + bytesRead);
        if (bytesRead < toRead) break;
    }

    WinHttpCloseHandle(hRequest);
    return true;
}
#endif

#if !defined(__APPLE__) && !defined(__ANDROID__) && !defined(_WIN32)
struct CurlWriteContext {
    std::vector<uint8_t>* vec;
    int64_t maxLen;
    bool is200;
};

static size_t taglib_curl_write_callback(void* contents, size_t size, size_t nmemb, void* userp) {
    size_t total = size * nmemb;
    auto ctx = static_cast<CurlWriteContext*>(userp);
    if (ctx->is200 && ctx->maxLen > 0 && ctx->vec->size() >= (size_t)ctx->maxLen) {
        return total; // skip further writing
    }
    size_t toWrite = total;
    if (ctx->is200 && ctx->maxLen > 0 && ctx->vec->size() + toWrite > (size_t)ctx->maxLen) {
        toWrite = (size_t)ctx->maxLen - ctx->vec->size();
    }
    ctx->vec->insert(ctx->vec->end(), static_cast<const uint8_t*>(contents), static_cast<const uint8_t*>(contents) + toWrite);
    return total;
}

static size_t taglib_curl_header_callback(char* buffer, size_t size, size_t nitems, void* userdata) {
    size_t total = size * nitems;
    int64_t* totalLength = static_cast<int64_t*>(userdata);
    std::string header(buffer, total);
    size_t pos = header.find("Content-Range:");
    if (pos == std::string::npos) pos = header.find("content-range:");
    if (pos != std::string::npos) {
        size_t slash = header.find('/', pos);
        if (slash != std::string::npos && slash + 1 < header.size()) {
            std::string num = header.substr(slash + 1);
            while (!num.empty() && (num.back() == '\r' || num.back() == '\n' || num.back() == ' ')) num.pop_back();
            if (!num.empty() && num != "*") {
                *totalLength = std::strtoll(num.c_str(), nullptr, 10);
            }
        }
    }
    return total;
}

static bool curl_fetch_http_range(
    HttpRangeConnection& connection,
    const std::string& url,
    const std::map<std::string, std::string>& headers,
    int64_t offset,
    int64_t length,
    int timeout_ms,
    int64_t& out_total_length,
    std::vector<uint8_t>& out_data,
    std::string& out_error
) {
    if (!connection.curl) {
        // curl_easy_init would otherwise run the global init lazily, which is not thread safe.
        static std::once_flag curlGlobalInitOnce;
        std::call_once(curlGlobalInitOnce, [] { curl_global_init(CURL_GLOBAL_DEFAULT); });
        connection.curl = curl_easy_init();
        if (!connection.curl) {
            out_error = "curl_easy_init failed";
            return false;
        }
    }
    CURL* curl = connection.curl;
    curl_easy_reset(curl); // keeps the open connection

    struct curl_slist* chunk = nullptr;
    for (const auto& kv : headers) {
        std::string h = kv.first + ": " + kv.second;
        chunk = curl_slist_append(chunk, h.c_str());
    }

    CurlWriteContext writeCtx = { &out_data, length, false };

    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT_MS, (long)(timeout_ms > 0 ? timeout_ms : 15000));
    curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT_MS, (long)(timeout_ms > 0 ? timeout_ms : 15000));
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, taglib_curl_write_callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &writeCtx);
    curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION, taglib_curl_header_callback);
    curl_easy_setopt(curl, CURLOPT_HEADERDATA, &out_total_length);

    if (chunk) {
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, chunk);
    }

    if (offset >= 0 && length > 0) {
        char rangeBuf[64];
        snprintf(rangeBuf, sizeof(rangeBuf), "%lld-%lld", (long long)offset, (long long)(offset + length - 1));
        curl_easy_setopt(curl, CURLOPT_RANGE, rangeBuf);
    }

    CURLcode res = curl_easy_perform(curl);
    long http_code = 0;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);
    writeCtx.is200 = (http_code == 200);

    if (chunk) {
        curl_slist_free_all(chunk);
    }

    if (res != CURLE_OK) {
        out_error = curl_easy_strerror(res);
        return false;
    }
    if (http_code != 200 && http_code != 206) {
        out_error = "HTTP status " + std::to_string(http_code);
        return false;
    }

    if (offset > 0 && http_code == 200) {
        out_error = "Server does not support HTTP Range requests (returned 200 OK for offset > 0)";
        return false;
    }

    if (out_total_length <= 0 && http_code == 200) {
        out_total_length = out_data.size();
    }

    return true;
}
#endif

static bool fetch_http_range(
    [[maybe_unused]] HttpRangeConnection& connection,
    const std::string& url,
    const std::map<std::string, std::string>& headers,
    int64_t offset,
    int64_t length,
    int timeout_ms,
    int64_t& out_total_length,
    std::vector<uint8_t>& out_data,
    std::string& out_error
) {
#if defined(__APPLE__)
    return apple_fetch_http_range(url, headers, offset, length, timeout_ms, out_total_length, out_data, out_error);
#elif defined(__ANDROID__)
    return android_fetch_http_range(url, headers, offset, length, timeout_ms, out_total_length, out_data, out_error);
#elif defined(_WIN32)
    return windows_fetch_http_range(connection, url, headers, offset, length, timeout_ms, out_total_length, out_data, out_error);
#else
    return curl_fetch_http_range(connection, url, headers, offset, length, timeout_ms, out_total_length, out_data, out_error);
#endif
}

class HttpRangeIOStream : public TagLib::IOStream {
public:
    static constexpr size_t kBlockSize = 65536; // 64 KB per block
    static constexpr size_t kMaxCachedBlocks = 24; // ~1.5 MB cache

    HttpRangeIOStream(std::string url, std::map<std::string, std::string> headers, int timeout_ms)
        : m_url(std::move(url)), m_headers(std::move(headers)), m_timeout_ms(timeout_ms), m_pos(0), m_length(0), m_isOpen(false)
    {
        std::vector<uint8_t> block0;
        std::string err;
        int64_t totalLen = -1;
        bool ok = fetch_http_range(m_connection, m_url, m_headers, 0, kBlockSize, m_timeout_ms, totalLen, block0, err);
        if (ok && !block0.empty()) {
            m_isOpen = true;
            if (totalLen > 0) {
                m_length = totalLen;
            } else {
                m_length = static_cast<int64_t>(block0.size());
            }
            put_block(0, std::move(block0));
        } else {
            LOGE("HttpRangeIOStream init failed for %s: %s", m_url.c_str(), err.c_str());
        }
    }

    ~HttpRangeIOStream() override = default;

    TagLib::FileName name() const override {
        return m_url.c_str();
    }

    TagLib::ByteVector readBlock(size_t length) override {
        if (!m_isOpen || m_pos >= m_length || length == 0) {
            return TagLib::ByteVector();
        }

        size_t toRead = length;
        if (m_pos + static_cast<int64_t>(toRead) > m_length) {
            toRead = static_cast<size_t>(m_length - m_pos);
        }

        TagLib::ByteVector result;
        result.resize(static_cast<unsigned int>(toRead));
        uint8_t* dest = reinterpret_cast<uint8_t*>(result.data());

        const int64_t lastBlockIndex = (m_pos + static_cast<int64_t>(toRead) - 1) / kBlockSize;
        size_t bytesRead = 0;
        while (bytesRead < toRead) {
            int64_t currentOffset = m_pos + bytesRead;
            int64_t blockIndex = currentOffset / kBlockSize;
            size_t blockOffset = static_cast<size_t>(currentOffset % kBlockSize);
            size_t bytesWanted = toRead - bytesRead;

            const std::vector<uint8_t>* source;
            std::vector<uint8_t> runData;
            int64_t sourceEndIndex = blockIndex;
            auto it = m_cache.find(blockIndex);
            if (it != m_cache.end()) {
                touch_lru(blockIndex);
                source = &it->second;
            } else {
                // one request for every missing block up to the end of this read, a big tag or cover
                // would otherwise cost a round trip per block.
                while (sourceEndIndex < lastBlockIndex && m_cache.find(sourceEndIndex + 1) == m_cache.end()) {
                    sourceEndIndex++;
                }
                if (!fetch_run(blockIndex, sourceEndIndex, runData)) break;
                source = &runData;
            }

            if (blockOffset >= source->size()) break;
            size_t available = source->size() - blockOffset;
            size_t bytesCopied = std::min(bytesWanted, available);
            std::memcpy(dest + bytesRead, source->data() + blockOffset, bytesCopied);
            bytesRead += bytesCopied;

            int64_t sourceStart = blockIndex * kBlockSize;
            int64_t sourceEnd = std::min((sourceEndIndex + 1) * static_cast<int64_t>(kBlockSize), m_length);
            bool isSourceShort = static_cast<int64_t>(source->size()) < sourceEnd - sourceStart;
            if (isSourceShort && bytesCopied == available) break;
        }

        m_pos += bytesRead;
        if (bytesRead < toRead) {
            result.resize(static_cast<unsigned int>(bytesRead));
        }
        return result;
    }

    void writeBlock(const TagLib::ByteVector&) override {}
    void insert(const TagLib::ByteVector&, TagLib::offset_t = 0, size_t = 0) override {}
    void removeBlock(TagLib::offset_t = 0, size_t = 0) override {}

    bool readOnly() const override {
        return true;
    }

    bool isOpen() const override {
        return m_isOpen;
    }

    void seek(TagLib::offset_t offset, TagLib::IOStream::Position p = TagLib::IOStream::Beginning) override {
        if (!m_isOpen) return;
        switch (p) {
            case TagLib::IOStream::Beginning:
                m_pos = offset;
                break;
            case TagLib::IOStream::Current:
                m_pos += offset;
                break;
            case TagLib::IOStream::End:
                m_pos = m_length + offset;
                break;
        }
        if (m_pos < 0) m_pos = 0;
        if (m_pos > m_length) m_pos = m_length;
    }

    void clear() override {}

    TagLib::offset_t tell() const override {
        return m_pos;
    }

    TagLib::offset_t length() override {
        return m_length;
    }

    void truncate(TagLib::offset_t) override {}

private:
    std::string m_url;
    std::map<std::string, std::string> m_headers;
    int m_timeout_ms;
    int64_t m_pos;
    int64_t m_length;
    bool m_isOpen;
    HttpRangeConnection m_connection;

    std::unordered_map<int64_t, std::vector<uint8_t>> m_cache;
    std::vector<int64_t> m_lruOrder;

    void put_block(int64_t blockIndex, std::vector<uint8_t> data) {
        if (m_cache.find(blockIndex) != m_cache.end()) {
            m_cache[blockIndex] = std::move(data);
            touch_lru(blockIndex);
            return;
        }

        if (m_cache.size() >= kMaxCachedBlocks) {
            int64_t oldest = m_lruOrder.front();
            m_lruOrder.erase(m_lruOrder.begin());
            m_cache.erase(oldest);
        }

        m_cache[blockIndex] = std::move(data);
        m_lruOrder.push_back(blockIndex);
    }

    void touch_lru(int64_t blockIndex) {
        auto it = std::find(m_lruOrder.begin(), m_lruOrder.end(), blockIndex);
        if (it != m_lruOrder.end()) {
            m_lruOrder.erase(it);
        }
        m_lruOrder.push_back(blockIndex);
    }

    // Fetches blocks [firstIndex, lastIndex] in one request, caching only the last one since reads
    // usually continue right after a run.
    bool fetch_run(int64_t firstIndex, int64_t lastIndex, std::vector<uint8_t>& out) {
        int64_t startOffset = firstIndex * kBlockSize;
        if (startOffset >= m_length) return false;

        int64_t endOffset = std::min((lastIndex + 1) * static_cast<int64_t>(kBlockSize), m_length);
        int64_t totalLen = -1;
        std::string err;
        bool ok = fetch_http_range(m_connection, m_url, m_headers, startOffset, endOffset - startOffset, m_timeout_ms, totalLen, out, err);
        if (!ok || out.empty()) {
            LOGE("HttpRangeIOStream fetch blocks %lld-%lld failed: %s", (long long)firstIndex, (long long)lastIndex, err.c_str());
            return false;
        }

        size_t lastBlockStart = static_cast<size_t>((lastIndex - firstIndex) * kBlockSize);
        if (out.size() > lastBlockStart) {
            put_block(lastIndex, std::vector<uint8_t>(out.begin() + lastBlockStart, out.end()));
        }
        return true;
    }
};

static TagLibBridgeFile* http_open_file(const char* url, std::map<std::string, std::string> headers, int read_style, int timeout_ms) {
    try {
        auto stream = std::make_unique<HttpRangeIOStream>(url, std::move(headers), timeout_ms > 0 ? timeout_ms : 15000);
        if (!stream->isOpen()) {
            LOGE("taglib_bridge_open_http: stream failed to open for url: %s", url);
            return nullptr;
        }

        bool readAudioProps = true;
        TagLib::AudioProperties::ReadStyle style = TagLib::AudioProperties::Average;
        resolve_read_style(read_style, readAudioProps, style);

        auto fileRef = std::make_unique<TagLib::FileRef>(stream.get(), readAudioProps, style);
        if (fileRef->isNull()) {
            LOGE("taglib_bridge_open_http: fileRef is null (invalid format or unreadable stream) for: %s", url);
            return nullptr;
        }

        auto bridge = new TagLibBridgeFile();
        bridge->fileRef = fileRef.release();
        bridge->stream = stream.release();
        return bridge;
    } catch (const std::exception& e) {
        LOGE("taglib_bridge_open_http: std::exception caught for %s: %s", url, e.what());
        return nullptr;
    } catch (...) {
        LOGE("taglib_bridge_open_http: unknown exception caught for %s", url);
        return nullptr;
    }
}

extern "C" {

TagLibBridgeFile* taglib_bridge_open_http(const char* url, const char* headers_json, int read_style, int timeout_ms) {
    if (!url || std::strlen(url) == 0) {
        LOGE("taglib_bridge_open_http: url is null or empty");
        return nullptr;
    }
    return http_open_file(url, parse_headers_json(headers_json), read_style, timeout_ms);
}

int taglib_bridge_save(TagLibBridgeFile* file) {
    if (!file) {
        LOGE("taglib_bridge_save: file pointer is NULL");
        return 0;
    }
    if (!file->fileRef) {
        LOGE("taglib_bridge_save: fileRef is NULL");
        return 0;
    }
    if (file->fileRef->isNull()) {
        LOGE("taglib_bridge_save: fileRef is null (invalid file)");
        return 0;
    }
    if (file->fileRef->file()) {
        if (file->fileRef->file()->readOnly()) {
            LOGE("taglib_bridge_save: TagLib::File is read-only. Cannot save metadata updates!");
            return 0;
        }
    } else {
        LOGW("taglib_bridge_save: TagLib::File is NULL");
    }

    try {
        bool success = file->fileRef->save();
        if (success) {
            LOGI("taglib_bridge_save: metadata saved successfully");
            return 1;
        } else {
            LOGE("taglib_bridge_save: fileRef->save() returned false");
            return 0;
        }
    } catch (const std::exception& e) {
        LOGE("taglib_bridge_save: std::exception caught while saving: %s", e.what());
        return 0;
    } catch (...) {
        LOGE("taglib_bridge_save: unknown exception caught while saving");
        return 0;
    }
}

void taglib_bridge_close(TagLibBridgeFile* file) {
    if (!file) return;
    try {
        if (file->fileRef) {
            delete file->fileRef;
        }
        if (file->stream) {
            // Delete stream after fileRef, as required by TagLib API
            delete file->stream;
        }
        delete file;
    } catch (...) {
    }
}

const char* taglib_bridge_get_title(TagLibBridgeFile* file) {
    if (!file || !file->fileRef || file->fileRef->isNull() || !file->fileRef->tag()) return "";
    try {
        file->cachedTitle = file->fileRef->tag()->title().to8Bit(true);
        return file->cachedTitle.c_str();
    } catch (...) {
        return "";
    }
}

const char* taglib_bridge_get_artist(TagLibBridgeFile* file) {
    if (!file || !file->fileRef || file->fileRef->isNull() || !file->fileRef->tag()) return "";
    try {
        file->cachedArtist = file->fileRef->tag()->artist().to8Bit(true);
        return file->cachedArtist.c_str();
    } catch (...) {
        return "";
    }
}

const char* taglib_bridge_get_album(TagLibBridgeFile* file) {
    if (!file || !file->fileRef || file->fileRef->isNull() || !file->fileRef->tag()) return "";
    try {
        file->cachedAlbum = file->fileRef->tag()->album().to8Bit(true);
        return file->cachedAlbum.c_str();
    } catch (...) {
        return "";
    }
}

const char* taglib_bridge_get_genre(TagLibBridgeFile* file) {
    if (!file || !file->fileRef || file->fileRef->isNull() || !file->fileRef->tag()) return "";
    try {
        file->cachedGenre = file->fileRef->tag()->genre().to8Bit(true);
        return file->cachedGenre.c_str();
    } catch (...) {
        return "";
    }
}

const char* taglib_bridge_get_comment(TagLibBridgeFile* file) {
    if (!file || !file->fileRef || file->fileRef->isNull() || !file->fileRef->tag()) return "";
    try {
        file->cachedComment = file->fileRef->tag()->comment().to8Bit(true);
        return file->cachedComment.c_str();
    } catch (...) {
        return "";
    }
}

uint32_t taglib_bridge_get_year(TagLibBridgeFile* file) {
    if (!file || !file->fileRef || file->fileRef->isNull() || !file->fileRef->tag()) return 0;
    try {
        return file->fileRef->tag()->year();
    } catch (...) {
        return 0;
    }
}

uint32_t taglib_bridge_get_track(TagLibBridgeFile* file) {
    if (!file || !file->fileRef || file->fileRef->isNull() || !file->fileRef->tag()) return 0;
    try {
        return file->fileRef->tag()->track();
    } catch (...) {
        return 0;
    }
}

void taglib_bridge_set_title(TagLibBridgeFile* file, const char* title) {
    if (!file || !file->fileRef || file->fileRef->isNull() || !file->fileRef->tag()) return;
    try {
        file->fileRef->tag()->setTitle(TagLib::String(title ? title : "", TagLib::String::UTF8));
    } catch (...) {}
}

void taglib_bridge_set_artist(TagLibBridgeFile* file, const char* artist) {
    if (!file || !file->fileRef || file->fileRef->isNull() || !file->fileRef->tag()) return;
    try {
        file->fileRef->tag()->setArtist(TagLib::String(artist ? artist : "", TagLib::String::UTF8));
    } catch (...) {}
}

void taglib_bridge_set_album(TagLibBridgeFile* file, const char* album) {
    if (!file || !file->fileRef || file->fileRef->isNull() || !file->fileRef->tag()) return;
    try {
        file->fileRef->tag()->setAlbum(TagLib::String(album ? album : "", TagLib::String::UTF8));
    } catch (...) {}
}

void taglib_bridge_set_genre(TagLibBridgeFile* file, const char* genre) {
    if (!file || !file->fileRef || file->fileRef->isNull() || !file->fileRef->tag()) return;
    try {
        file->fileRef->tag()->setGenre(TagLib::String(genre ? genre : "", TagLib::String::UTF8));
    } catch (...) {}
}

void taglib_bridge_set_comment(TagLibBridgeFile* file, const char* comment) {
    if (!file || !file->fileRef || file->fileRef->isNull() || !file->fileRef->tag()) return;
    try {
        file->fileRef->tag()->setComment(TagLib::String(comment ? comment : "", TagLib::String::UTF8));
    } catch (...) {}
}

void taglib_bridge_set_year(TagLibBridgeFile* file, uint32_t year) {
    if (!file || !file->fileRef || file->fileRef->isNull() || !file->fileRef->tag()) return;
    try {
        file->fileRef->tag()->setYear(year);
    } catch (...) {}
}

void taglib_bridge_set_track(TagLibBridgeFile* file, uint32_t track) {
    if (!file || !file->fileRef || file->fileRef->isNull() || !file->fileRef->tag()) return;
    try {
        file->fileRef->tag()->setTrack(track);
    } catch (...) {}
}

int taglib_bridge_get_duration(TagLibBridgeFile* file) {
    if (!file || !file->fileRef || file->fileRef->isNull() || !file->fileRef->audioProperties()) return 0;
    try {
        return file->fileRef->audioProperties()->lengthInMilliseconds();
    } catch (...) {
        return 0;
    }
}

int taglib_bridge_get_bitrate(TagLibBridgeFile* file) {
    if (!file || !file->fileRef || file->fileRef->isNull() || !file->fileRef->audioProperties()) return 0;
    try {
        return file->fileRef->audioProperties()->bitrate();
    } catch (...) {
        return 0;
    }
}

int taglib_bridge_get_samplerate(TagLibBridgeFile* file) {
    if (!file || !file->fileRef || file->fileRef->isNull() || !file->fileRef->audioProperties()) return 0;
    try {
        return file->fileRef->audioProperties()->sampleRate();
    } catch (...) {
        return 0;
    }
}

int taglib_bridge_get_channels(TagLibBridgeFile* file) {
    if (!file || !file->fileRef || file->fileRef->isNull() || !file->fileRef->audioProperties()) return 0;
    try {
        return file->fileRef->audioProperties()->channels();
    } catch (...) {
        return 0;
    }
}

int taglib_bridge_get_bits_per_sample(TagLibBridgeFile* file) {
    if (!file || !file->fileRef || file->fileRef->isNull()) return 0;
    try {
        auto props = file->fileRef->audioProperties();
        if (!props) return 0;

        // TagLib's AudioProperties base class does not expose bit depth, so it
        // is read from each concrete Properties subclass that has one. Lossy
        // codecs have no fixed sample size, so MP4 and ASF — whose containers
        // declare a nominal value even for AAC/WMA — only report their
        // lossless codecs. Matroska needs no gate: its BitDepth element is
        // simply absent for lossy tracks.
        if (auto p = dynamic_cast<TagLib::FLAC::Properties*>(props)) return p->bitsPerSample();
        if (auto p = dynamic_cast<TagLib::MP4::Properties*>(props)) {
            return p->codec() == TagLib::MP4::Properties::ALAC ? p->bitsPerSample() : 0;
        }
        if (auto p = dynamic_cast<TagLib::ASF::Properties*>(props)) {
            return p->codec() == TagLib::ASF::Properties::WMA9Lossless ? p->bitsPerSample() : 0;
        }
        if (auto p = dynamic_cast<TagLib::RIFF::WAV::Properties*>(props)) return p->bitsPerSample();
        if (auto p = dynamic_cast<TagLib::RIFF::AIFF::Properties*>(props)) return p->bitsPerSample();
        if (auto p = dynamic_cast<TagLib::APE::Properties*>(props)) return p->bitsPerSample();
        if (auto p = dynamic_cast<TagLib::WavPack::Properties*>(props)) return p->bitsPerSample();
        if (auto p = dynamic_cast<TagLib::TrueAudio::Properties*>(props)) return p->bitsPerSample();
        if (auto p = dynamic_cast<TagLib::DSF::Properties*>(props)) return p->bitsPerSample();
        if (auto p = dynamic_cast<TagLib::DSDIFF::Properties*>(props)) return p->bitsPerSample();
        if (auto p = dynamic_cast<TagLib::Matroska::Properties*>(props)) return p->bitsPerSample();
        return 0;
    } catch (...) {
        return 0;
    }
}

const char* taglib_bridge_get_bitrate_mode(TagLibBridgeFile* file) {
    if (!file || !file->fileRef || file->fileRef->isNull() || !file->fileRef->audioProperties()) return "";
    try {
        auto audioProps = file->fileRef->audioProperties();
        auto filePtr = file->fileRef->file();
        
        if (auto mpegFile = dynamic_cast<TagLib::MPEG::File*>(filePtr)) {
            auto mpegProps = dynamic_cast<TagLib::MPEG::Properties*>(audioProps);
            if (mpegProps) {
                auto xing = mpegProps->xingHeader();
                if (xing && xing->isValid()) {
                    if (xing->type() == TagLib::MPEG::XingHeader::Xing || xing->type() == TagLib::MPEG::XingHeader::VBRI) {
                        file->cachedBitrateMode = "VBR";
                    } else {
                        file->cachedBitrateMode = "CBR";
                    }
                } else {
                    file->cachedBitrateMode = "CBR";
                }
            } else {
                file->cachedBitrateMode = "Unknown";
            }
        } else if (auto flacFile = dynamic_cast<TagLib::FLAC::File*>(filePtr)) {
            file->cachedBitrateMode = "VBR";
        } else if (auto vorbisFile = dynamic_cast<TagLib::Ogg::Vorbis::File*>(filePtr)) {
            file->cachedBitrateMode = "VBR";
        } else if (auto opusFile = dynamic_cast<TagLib::Ogg::Opus::File*>(filePtr)) {
            file->cachedBitrateMode = "VBR";
        } else if (auto wavFile = dynamic_cast<TagLib::RIFF::WAV::File*>(filePtr)) {
            file->cachedBitrateMode = "CBR";
        } else {
            file->cachedBitrateMode = "Unknown";
        }
        return file->cachedBitrateMode.c_str();
    } catch (...) {
        return "";
    }
}

const char* taglib_bridge_get_format(TagLibBridgeFile* file) {
    if (!file || !file->fileRef || file->fileRef->isNull()) return nullptr;

    // The format of an open file never changes, so resolve it only once.
    if (file->formatResolved) {
        return file->cachedFormat.empty() ? nullptr : file->cachedFormat.c_str();
    }

    try {
        auto filePtr = file->fileRef->file();
        if (!filePtr) return nullptr;

        // The concrete TagLib::File subclass is resolved from the file contents,
        // so this stays correct even for files with a wrong or missing extension.
        const std::type_index fileType(typeid(*filePtr));

        if (fileType == std::type_index(typeid(TagLib::MPEG::File))) {
            // MPEG covers layers I/II/III, so report the actual layer.
            auto mpegProps = dynamic_cast<TagLib::MPEG::Properties*>(file->fileRef->audioProperties());
            switch (mpegProps ? mpegProps->layer() : 3) {
                case 1: file->cachedFormat = "MP1"; break;
                case 2: file->cachedFormat = "MP2"; break;
                default: file->cachedFormat = "MP3"; break;
            }
        } else if (fileType == std::type_index(typeid(TagLib::MP4::File))) {
            // Distinguish lossy AAC from lossless ALAC inside the MP4 container.
            auto mp4Props = dynamic_cast<TagLib::MP4::Properties*>(file->fileRef->audioProperties());
            if (mp4Props && mp4Props->codec() == TagLib::MP4::Properties::AAC) {
                file->cachedFormat = "AAC";
            } else if (mp4Props && mp4Props->codec() == TagLib::MP4::Properties::ALAC) {
                file->cachedFormat = "ALAC";
            } else {
                file->cachedFormat = "MP4";
            }
        } else if (fileType == std::type_index(typeid(TagLib::Matroska::File))) {
            // Matroska is a container, so report the audio codec inside it and
            // only fall back to the container name (split by EBML doc type)
            // when the codec is not recognized.
            auto mkProps = dynamic_cast<TagLib::Matroska::Properties*>(file->fileRef->audioProperties());
            std::string token = mkProps ? matroska_codec_token(mkProps->codecName()) : std::string();
            if (!token.empty()) {
                file->cachedFormat = token;
            } else if (mkProps && mkProps->docType() == "webm") {
                file->cachedFormat = "WEBM";
            } else if (mkProps && mkProps->docType() == "weba") {
                file->cachedFormat = "WEBA";
            } else {
                file->cachedFormat = "MATROSKA";
            }
        } else {
            const auto& table = format_token_table();
            const auto match = table.find(fileType);
            if (match != table.end()) {
                file->cachedFormat = match->second;
            } else {
                // A TagLib format this bridge does not name: derive a token from
                // the runtime class name so it still reports something useful.
                file->cachedFormat = format_token_from_class_name(runtime_class_name(filePtr));
            }
        }

        file->formatResolved = true;
        return file->cachedFormat.empty() ? nullptr : file->cachedFormat.c_str();
    } catch (...) {
        return nullptr;
    }
}

int taglib_bridge_is_lossless(TagLibBridgeFile* file) {
    if (!file || !file->fileRef || file->fileRef->isNull()) return kLosslessUnknown;
    if (file->losslessResolved) return file->cachedLossless;
    try {
        auto filePtr = file->fileRef->file();
        if (!filePtr) {
            file->cachedLossless = kLosslessUnknown;
            file->losslessResolved = true;
            return kLosslessUnknown;
        }

        const std::type_index fileType(typeid(*filePtr));
        auto audioProps = file->fileRef->audioProperties();

        int verdict = kLosslessUnknown;

        // Containers that can hold either a lossy or a lossless stream must be
        // resolved from the stream itself rather than from the format.
        if (fileType == std::type_index(typeid(TagLib::MP4::File))) {
            auto props = dynamic_cast<TagLib::MP4::Properties*>(audioProps);
            if (props) {
                if (props->codec() == TagLib::MP4::Properties::ALAC) verdict = kLossless;
                else if (props->codec() == TagLib::MP4::Properties::AAC) verdict = kLossy;
            }
        } else if (fileType == std::type_index(typeid(TagLib::ASF::File))) {
            auto props = dynamic_cast<TagLib::ASF::Properties*>(audioProps);
            if (props) {
                switch (props->codec()) {
                    case TagLib::ASF::Properties::WMA9Lossless: verdict = kLossless; break;
                    case TagLib::ASF::Properties::WMA1:
                    case TagLib::ASF::Properties::WMA2:
                    case TagLib::ASF::Properties::WMA9Pro: verdict = kLossy; break;
                    default: verdict = kLosslessUnknown; break;
                }
            }
        } else if (fileType == std::type_index(typeid(TagLib::Matroska::File))) {
            auto props = dynamic_cast<TagLib::Matroska::Properties*>(audioProps);
            if (props) {
                const std::string token = matroska_codec_token(props->codecName());
                static const std::unordered_map<std::string, int> codecVerdicts = {
                    {"FLAC", kLossless}, {"ALAC", kLossless}, {"PCM", kLossless},
                    {"WAVPACK", kLossless}, {"TTA", kLossless}, {"MLP", kLossless},
                    {"TRUEHD", kLossless},
                    {"OPUS", kLossy}, {"VORBIS", kLossy}, {"AAC", kLossy},
                    {"MP1", kLossy}, {"MP2", kLossy}, {"MP3", kLossy},
                    {"AC3", kLossy}, {"EAC3", kLossy}, {"MPC", kLossy},
                };
                if (token == "DTS") {
                    // Only the DTS-HD Master Audio variant is lossless.
                    verdict = props->codecName() == "A_DTS/LOSSLESS" ? kLossless : kLossy;
                } else {
                    const auto match = codecVerdicts.find(token);
                    if (match != codecVerdicts.end()) verdict = match->second;
                }
            }
        } else if (fileType == std::type_index(typeid(TagLib::WavPack::File))) {
            auto props = dynamic_cast<TagLib::WavPack::Properties*>(audioProps);
            verdict = props ? (props->isLossless() ? kLossless : kLossy) : kLosslessUnknown;
        } else if (fileType == std::type_index(typeid(TagLib::RIFF::WAV::File))) {
            auto props = dynamic_cast<TagLib::RIFF::WAV::Properties*>(audioProps);
            if (props) {
                switch (props->format()) {
                    case 0x0001: // WAVE_FORMAT_PCM
                    case 0x0003: // WAVE_FORMAT_IEEE_FLOAT
                    case 0xFFFE: // WAVE_FORMAT_EXTENSIBLE
                        verdict = kLossless; break;
                    case 0x0000: // unknown / unset
                        verdict = kLosslessUnknown; break;
                    default:
                        verdict = kLossy; break;
                }
            }
        } else if (fileType == std::type_index(typeid(TagLib::RIFF::AIFF::File))) {
            auto props = dynamic_cast<TagLib::RIFF::AIFF::Properties*>(audioProps);
            if (props) {
                if (!props->isAiffC()) verdict = kLossless;
                else verdict = is_lossless_aifc_compression(props->compressionType()) ? kLossless : kLossy;
            }
        } else {
            const auto& table = lossless_table();
            const auto match = table.find(fileType);
            verdict = (match != table.end()) ? match->second : kLosslessUnknown;
        }

        file->cachedLossless = verdict;
        file->losslessResolved = true;
        return file->cachedLossless;
    } catch (...) {
        return kLosslessUnknown;
    }
}

int taglib_bridge_has_cover(TagLibBridgeFile* file) {
    if (!file || !file->fileRef || file->fileRef->isNull()) return 0;
    if (file->hasCoverResolved) return file->cachedHasCover;
    try {
        auto pictures = read_picture_list(file);
        file->cachedHasCover = !pictures.isEmpty() ? 1 : 0;
        file->hasCoverResolved = true;
        return file->cachedHasCover;
    } catch (...) {
        return 0;
    }
}

uint32_t taglib_bridge_get_cover_data_size(TagLibBridgeFile* file) {
    if (!file || !file->fileRef || file->fileRef->isNull()) return 0;
    try {
        auto pictures = read_picture_list(file);
        if (pictures.isEmpty()) return 0;
        auto dataVar = pictures.front()["data"];
        if (dataVar.isEmpty()) return 0;
        return dataVar.toByteVector().size();
    } catch (...) {
        return 0;
    }
}

int taglib_bridge_get_cover_data(TagLibBridgeFile* file, uint8_t* buffer, uint32_t buffer_size) {
    if (!file || !file->fileRef || file->fileRef->isNull() || !buffer || buffer_size == 0) return 0;
    try {
        auto pictures = read_picture_list(file);
        if (pictures.isEmpty()) return 0;
        auto dataVar = pictures.front()["data"];
        if (dataVar.isEmpty()) return 0;
        auto byteVector = dataVar.toByteVector();
        uint32_t toCopy = byteVector.size() < buffer_size ? byteVector.size() : buffer_size;
        std::memcpy(buffer, byteVector.data(), toCopy);
        return 1;
    } catch (...) {
        return 0;
    }
}

const char* taglib_bridge_get_cover_mime_type(TagLibBridgeFile* file) {
    if (!file || !file->fileRef || file->fileRef->isNull()) return "";
    try {
        auto pictures = read_picture_list(file);
        if (pictures.isEmpty()) return "";
        auto mimeVar = pictures.front()["mimeType"];
        if (mimeVar.isEmpty()) return "";
        file->cachedCoverMime = mimeVar.toString().to8Bit(true);
        return file->cachedCoverMime.c_str();
    } catch (...) {
        return "";
    }
}

uint32_t taglib_bridge_front_cover_size(TagLibBridgeFile* file) {
    if (!file || !file->fileRef || file->fileRef->isNull()) return 0;
    try {
        file->cachedFrontCover = TagLib::ByteVector();

        auto pictures = read_picture_list(file);
        if (pictures.isEmpty()) return 0;

        // Prefer an explicitly typed front cover; otherwise take the first picture,
        // matching how embedded art is conventionally ordered.
        const TagLib::VariantMap* selected = nullptr;
        for (const auto& picture : pictures) {
            auto typeVar = picture["pictureType"];
            if (!typeVar.isEmpty() && typeVar.toString() == "Front Cover") {
                selected = &picture;
                break;
            }
        }
        if (!selected) selected = &pictures.front();

        auto dataVar = (*selected)["data"];
        if (dataVar.isEmpty()) return 0;

        file->cachedFrontCover = dataVar.toByteVector();
        return static_cast<uint32_t>(file->cachedFrontCover.size());
    } catch (...) {
        file->cachedFrontCover = TagLib::ByteVector();
        return 0;
    }
}

int taglib_bridge_front_cover_data(TagLibBridgeFile* file, uint8_t* buffer, uint32_t buffer_size) {
    if (!file || !buffer || buffer_size == 0) return 0;
    try {
        if (file->cachedFrontCover.isEmpty()) return 0;
        uint32_t size = static_cast<uint32_t>(file->cachedFrontCover.size());
        uint32_t toCopy = size < buffer_size ? size : buffer_size;
        std::memcpy(buffer, file->cachedFrontCover.data(), toCopy);
        // The bytes are handed off to the caller, so drop our copy right away.
        file->cachedFrontCover = TagLib::ByteVector();
        return 1;
    } catch (...) {
        return 0;
    }
}

int taglib_bridge_set_cover(TagLibBridgeFile* file, const char* mime_type, const uint8_t* data, uint32_t size) {
    if (!file || !file->fileRef || file->fileRef->isNull()) return 0;
    try {
        file->invalidateCaches();
        TagLib::List<TagLib::VariantMap> pictures;
        if (size > 0 && data != nullptr) {
            pictures.append(build_picture_map(data, size, mime_type, "Front Cover", nullptr));
        }
        return file->fileRef->setComplexProperties("PICTURE", pictures) ? 1 : 0;
    } catch (...) {
        return 0;
    }
}

TagLibBridgePictures* taglib_bridge_pictures_create() {
    return new TagLibBridgePictures();
}

void taglib_bridge_pictures_free(TagLibBridgePictures* pictures) {
    if (pictures) delete pictures;
}

TagLibBridgePictures* taglib_bridge_pictures_get(TagLibBridgeFile* file) {
    if (!file || !file->fileRef || file->fileRef->isNull()) return nullptr;
    try {
        auto* bridgePictures = new TagLibBridgePictures();
        bridgePictures->pictures = read_picture_list(file);
        bridgePictures->refreshCache();
        return bridgePictures;
    } catch (...) {
        return nullptr;
    }
}

int taglib_bridge_pictures_set(TagLibBridgeFile* file, TagLibBridgePictures* pictures) {
    if (!file || !file->fileRef || file->fileRef->isNull() || !pictures) return 0;
    try {
        file->invalidateCaches();
        return file->fileRef->setComplexProperties("PICTURE", pictures->pictures) ? 1 : 0;
    } catch (...) {
        return 0;
    }
}

int taglib_bridge_pictures_size(TagLibBridgePictures* pictures) {
    if (!pictures) return 0;
    return static_cast<int>(pictures->cachedPictures.size());
}

uint32_t taglib_bridge_pictures_data_size(TagLibBridgePictures* pictures, int index) {
    const auto* picture = picture_at(pictures, index);
    if (!picture) return 0;
    auto dataVar = (*picture)["data"];
    if (dataVar.isEmpty()) return 0;
    return static_cast<uint32_t>(dataVar.toByteVector().size());
}

int taglib_bridge_pictures_data(TagLibBridgePictures* pictures, int index, uint8_t* buffer, uint32_t buffer_size) {
    if (!pictures || !buffer || buffer_size == 0) return 0;
    const auto* picture = picture_at(pictures, index);
    if (!picture) return 0;
    auto dataVar = (*picture)["data"];
    if (dataVar.isEmpty()) return 0;
    auto byteVector = dataVar.toByteVector();
    uint32_t toCopy = byteVector.size() < buffer_size ? byteVector.size() : buffer_size;
    std::memcpy(buffer, byteVector.data(), toCopy);
    return 1;
}

const char* taglib_bridge_pictures_mime_type(TagLibBridgePictures* pictures, int index) {
    if (!pictures || index < 0 || index >= static_cast<int>(pictures->cachedMimeTypes.size())) return "";
    return pictures->cachedMimeTypes[static_cast<size_t>(index)].c_str();
}

const char* taglib_bridge_pictures_description(TagLibBridgePictures* pictures, int index) {
    if (!pictures || index < 0 || index >= static_cast<int>(pictures->cachedDescriptions.size())) return "";
    return pictures->cachedDescriptions[static_cast<size_t>(index)].c_str();
}

const char* taglib_bridge_pictures_picture_type(TagLibBridgePictures* pictures, int index) {
    if (!pictures || index < 0 || index >= static_cast<int>(pictures->cachedPictureTypes.size())) return "";
    return pictures->cachedPictureTypes[static_cast<size_t>(index)].c_str();
}

void taglib_bridge_pictures_add(
    TagLibBridgePictures* pictures,
    const uint8_t* data,
    uint32_t size,
    const char* mime_type,
    const char* picture_type,
    const char* description
) {
    if (!pictures || !data || size == 0) return;
    try {
        pictures->pictures.append(
            build_picture_map(data, size, mime_type, picture_type, description)
        );
        pictures->refreshCache();
    } catch (...) {
    }
}

struct TagLibBridgeProperties {
    TagLib::PropertyMap properties;
    std::vector<std::string> keys;
    std::map<std::string, std::vector<std::string>> values;

    void refreshCache() {
        keys.clear();
        values.clear();
        for (auto it = properties.begin(); it != properties.end(); ++it) {
            std::string keyStr = it->first.to8Bit(true);
            keys.push_back(keyStr);
            
            std::vector<std::string> valStrs;
            for (auto const& val : it->second) {
                valStrs.push_back(val.to8Bit(true));
            }
            values[keyStr] = valStrs;
        }
    }
};

TagLibBridgeProperties* taglib_bridge_properties_create() {
    return new TagLibBridgeProperties();
}

void taglib_bridge_properties_free(TagLibBridgeProperties* props) {
    if (props) delete props;
}

TagLibBridgeProperties* taglib_bridge_properties_get(TagLibBridgeFile* file) {
    if (!file || !file->fileRef || file->fileRef->isNull()) return nullptr;
    try {
        auto* bridgeProps = new TagLibBridgeProperties();
        bridgeProps->properties = file->fileRef->properties();
        bridgeProps->refreshCache();
        return bridgeProps;
    } catch (...) {
        return nullptr;
    }
}

TagLibBridgeProperties* taglib_bridge_properties_set(TagLibBridgeFile* file, TagLibBridgeProperties* props) {
    if (!file || !file->fileRef || file->fileRef->isNull() || !props) return nullptr;
    try {
        TagLib::PropertyMap unsupported = file->fileRef->setProperties(props->properties);
        
        auto* bridgeUnsupported = new TagLibBridgeProperties();
        bridgeUnsupported->properties = unsupported;
        bridgeUnsupported->refreshCache();
        return bridgeUnsupported;
    } catch (...) {
        return nullptr;
    }
}

int taglib_bridge_properties_size(TagLibBridgeProperties* props) {
    if (!props) return 0;
    return props->keys.size();
}

const char* taglib_bridge_properties_key(TagLibBridgeProperties* props, int index) {
    if (!props || index < 0 || index >= (int)props->keys.size()) return "";
    return props->keys[index].c_str();
}

int taglib_bridge_properties_value_count(TagLibBridgeProperties* props, const char* key) {
    if (!props || !key) return 0;
    auto it = props->values.find(key);
    if (it == props->values.end()) return 0;
    return it->second.size();
}

const char* taglib_bridge_properties_value(TagLibBridgeProperties* props, const char* key, int value_index) {
    if (!props || !key || value_index < 0) return "";
    auto it = props->values.find(key);
    if (it == props->values.end() || value_index >= (int)it->second.size()) return "";
    return it->second[value_index].c_str();
}

void taglib_bridge_properties_add(TagLibBridgeProperties* props, const char* key, const char* value) {
    if (!props || !key || !value) return;
    TagLib::String tKey(key, TagLib::String::UTF8);
    TagLib::String tVal(value, TagLib::String::UTF8);
    props->properties[tKey].append(tVal);
    props->refreshCache();
}

void taglib_bridge_free(void* data) {
    std::free(data);
}

} // extern "C"

// by claude
// Chapters: one sorted list whatever the container stores them as, ID3v2
// CHAP frames, MP4 Nero/QuickTime chapters, Matroska editions or the
// CHAPTERxxx/CHAPTERxxxNAME comments ffmpeg writes to Ogg & FLAC.

static constexpr uint32_t kChapterEndUnknown = 0xFFFFFFFF;

struct BridgeChapter {
    uint32_t startMs;
    uint32_t endMs;
    TagLib::String title;
};

static uint32_t clamp_chapter_ms(long long ms) {
    if (ms <= 0) return 0;
    if (ms >= kChapterEndUnknown) return kChapterEndUnknown - 1;
    return static_cast<uint32_t>(ms);
}

static TagLib::ID3v2::Tag* id3v2_tag_of(TagLib::File* f, bool create) {
    if (auto* mpeg = dynamic_cast<TagLib::MPEG::File*>(f)) return mpeg->ID3v2Tag(create);
    if (auto* wav = dynamic_cast<TagLib::RIFF::WAV::File*>(f)) return wav->ID3v2Tag();
    if (auto* aiff = dynamic_cast<TagLib::RIFF::AIFF::File*>(f)) return aiff->tag();
    if (auto* dsf = dynamic_cast<TagLib::DSF::File*>(f)) return dsf->tag();
    if (auto* dsdiff = dynamic_cast<TagLib::DSDIFF::File*>(f)) return dsdiff->ID3v2Tag(create);
    if (auto* trueAudio = dynamic_cast<TagLib::TrueAudio::File*>(f)) return trueAudio->ID3v2Tag(create);
    return nullptr;
}

static TagLib::Ogg::XiphComment* xiph_comment_of(TagLib::File* f) {
    if (auto* vorbis = dynamic_cast<TagLib::Ogg::Vorbis::File*>(f)) return vorbis->tag();
    if (auto* opus = dynamic_cast<TagLib::Ogg::Opus::File*>(f)) return opus->tag();
    if (auto* speex = dynamic_cast<TagLib::Ogg::Speex::File*>(f)) return speex->tag();
    if (auto* oggFlac = dynamic_cast<TagLib::Ogg::FLAC::File*>(f)) return oggFlac->tag();
    if (auto* flac = dynamic_cast<TagLib::FLAC::File*>(f)) return flac->xiphComment(true);
    return nullptr;
}

static void read_id3v2_chapters(TagLib::ID3v2::Tag* tag, std::vector<BridgeChapter>& out) {
    for (auto* frame : tag->frameList("CHAP")) {
        auto* chapter = dynamic_cast<TagLib::ID3v2::ChapterFrame*>(frame);
        if (!chapter) continue;
        TagLib::String title;
        const auto& titleFrames = chapter->embeddedFrameList("TIT2");
        if (!titleFrames.isEmpty()) title = titleFrames.front()->toString();
        const unsigned int startMs = chapter->startTime();
        const unsigned int endMs = chapter->endTime();
        out.push_back({clamp_chapter_ms(startMs), endMs > startMs ? clamp_chapter_ms(endMs) : kChapterEndUnknown, title});
    }
}

// MP4::Chapter times are in milliseconds, the 100ns atom units are converted by TagLib.
static void read_mp4_chapters(TagLib::MP4::File* mp4, std::vector<BridgeChapter>& out) {
    auto chapters = mp4->neroChapters();
    if (chapters.isEmpty()) chapters = mp4->qtChapters();
    for (const auto& chapter : chapters) {
        out.push_back({clamp_chapter_ms(chapter.startTime()), kChapterEndUnknown, chapter.title()});
    }
}

// Matroska chapter times are in nanoseconds.
static void read_matroska_chapters(TagLib::Matroska::File* matroska, std::vector<BridgeChapter>& out) {
    const auto* chapters = matroska->chapters();
    if (!chapters) return;
    const auto& editions = chapters->chapterEditionList();
    if (editions.isEmpty()) return;
    const TagLib::Matroska::ChapterEdition* edition = &editions.front();
    for (const auto& candidate : editions) {
        if (candidate.isDefault()) {
            edition = &candidate;
            break;
        }
    }
    for (const auto& chapter : edition->chapterList()) {
        if (chapter.isHidden()) continue;
        const auto& displays = chapter.displayList();
        const TagLib::String title = displays.isEmpty() ? TagLib::String() : displays.front().string();
        const auto startNs = chapter.timeStart();
        const auto endNs = chapter.timeEnd();
        const uint32_t endMs = endNs > startNs ? clamp_chapter_ms(static_cast<long long>(endNs / 1000000)) : kChapterEndUnknown;
        out.push_back({clamp_chapter_ms(static_cast<long long>(startNs / 1000000)), endMs, title});
    }
}

// [[h:]m:]s[.fff], as in CHAPTER001=00:01:02.500
static bool parse_chapter_timestamp(const std::string& text, uint32_t& outMs) {
    const size_t length = text.size();
    size_t i = 0;
    long long seconds = 0;
    int fieldsCount = 0;
    while (true) {
        if (i >= length || !std::isdigit(static_cast<unsigned char>(text[i]))) return false;
        long long field = 0;
        while (i < length && std::isdigit(static_cast<unsigned char>(text[i]))) {
            field = field * 10 + (text[i] - '0');
            if (field > 100000000) return false;
            i++;
        }
        seconds = seconds * 60 + field;
        if (++fieldsCount > 3) return false;
        if (i < length && text[i] == ':') {
            i++;
            continue;
        }
        break;
    }
    long long fractionMs = 0;
    if (i < length && text[i] == '.') {
        i++;
        long long scale = 100;
        while (i < length && std::isdigit(static_cast<unsigned char>(text[i]))) {
            fractionMs += (text[i] - '0') * scale;
            scale /= 10;
            i++;
        }
    }
    if (i != length) return false;
    outMs = clamp_chapter_ms(seconds * 1000 + fractionMs);
    return true;
}

// checked in place, so the other keys of every file aren't converted.
static bool has_chapter_key_prefix(const TagLib::String& key) {
    static constexpr char kPrefix[] = "CHAPTER";
    if (key.size() < sizeof(kPrefix) - 1) return false;
    for (unsigned int i = 0; i < sizeof(kPrefix) - 1; i++) {
        if (key[i] != static_cast<wchar_t>(kPrefix[i])) return false;
    }
    return true;
}

static void read_comment_chapters(const TagLib::PropertyMap& properties, std::vector<BridgeChapter>& out) {
    std::map<unsigned int, BridgeChapter> byNumber;
    for (auto it = properties.begin(); it != properties.end(); ++it) {
        if (it->second.isEmpty() || !has_chapter_key_prefix(it->first)) continue;
        const std::string key = it->first.to8Bit(true);
        size_t i = 7;
        unsigned int number = 0;
        while (i < key.size() && i < 12 && std::isdigit(static_cast<unsigned char>(key[i]))) {
            number = number * 10 + (key[i] - '0');
            i++;
        }
        if (i == 7) continue;
        const bool isStart = i == key.size();
        const bool isName = !isStart && key.compare(i, std::string::npos, "NAME") == 0;
        if (!isStart && !isName) continue;
        auto& chapter = byNumber.try_emplace(number, BridgeChapter{kChapterEndUnknown, kChapterEndUnknown, TagLib::String()}).first->second;
        if (isName) {
            chapter.title = it->second.front();
        } else {
            uint32_t startMs = 0;
            if (parse_chapter_timestamp(it->second.front().to8Bit(true), startMs)) chapter.startMs = startMs;
        }
    }
    for (auto& entry : byNumber) {
        if (entry.second.startMs != kChapterEndUnknown) out.push_back(std::move(entry.second));
    }
}

static std::vector<BridgeChapter> read_chapters(TagLib::File* f, const TagLib::PropertyMap& properties) {
    std::vector<BridgeChapter> chapters;
    if (auto* mp4 = dynamic_cast<TagLib::MP4::File*>(f)) {
        read_mp4_chapters(mp4, chapters);
    } else if (auto* matroska = dynamic_cast<TagLib::Matroska::File*>(f)) {
        read_matroska_chapters(matroska, chapters);
    } else if (auto* id3v2 = id3v2_tag_of(f, false)) {
        read_id3v2_chapters(id3v2, chapters);
    }
    if (chapters.empty()) read_comment_chapters(properties, chapters);
    std::stable_sort(chapters.begin(), chapters.end(), [](const BridgeChapter& a, const BridgeChapter& b) { return a.startMs < b.startMs; });
    return chapters;
}

static void write_id3v2_chapters(TagLib::ID3v2::Tag* tag, const std::vector<BridgeChapter>& chapters) {
    tag->removeFrames("CHAP");
    tag->removeFrames("CTOC");
    if (chapters.empty()) return;
    TagLib::ByteVectorList elementIds;
    for (size_t i = 0; i < chapters.size(); i++) {
        const auto& chapter = chapters[i];
        const TagLib::ByteVector elementId = TagLib::ByteVector("chp") + TagLib::String::number(static_cast<int>(i)).data(TagLib::String::Latin1);
        elementIds.append(elementId);
        auto* titleFrame = new TagLib::ID3v2::TextIdentificationFrame("TIT2", TagLib::String::UTF8);
        titleFrame->setText(chapter.title);
        TagLib::ID3v2::FrameList embeddedFrames;
        embeddedFrames.append(titleFrame);
        tag->addFrame(new TagLib::ID3v2::ChapterFrame(elementId, chapter.startMs, chapter.endMs, 0xFFFFFFFF, 0xFFFFFFFF, embeddedFrames));
    }
    auto* toc = new TagLib::ID3v2::TableOfContentsFrame("toc", elementIds);
    toc->setIsTopLevel(true);
    toc->setIsOrdered(true);
    tag->addFrame(toc);
}

static void write_mp4_chapters(TagLib::MP4::File* mp4, const std::vector<BridgeChapter>& chapters) {
    TagLib::MP4::ChapterList list;
    for (const auto& chapter : chapters) {
        list.append(TagLib::MP4::Chapter(chapter.title, chapter.startMs));
    }
    mp4->setNeroChapters(list);
}

static void write_matroska_chapters(TagLib::Matroska::File* matroska, const std::vector<BridgeChapter>& chapters) {
    auto* target = matroska->chapters(!chapters.empty());
    if (!target) return;
    target->clear();
    if (chapters.empty()) return;
    TagLib::List<TagLib::Matroska::Chapter> list;
    for (const auto& chapter : chapters) {
        TagLib::List<TagLib::Matroska::Chapter::Display> displays;
        displays.append(TagLib::Matroska::Chapter::Display(chapter.title, "und"));
        const auto startNs = static_cast<TagLib::Matroska::Chapter::Time>(chapter.startMs) * 1000000;
        const auto endNs = static_cast<TagLib::Matroska::Chapter::Time>(chapter.endMs) * 1000000;
        list.append(TagLib::Matroska::Chapter(startNs, endNs, displays, 0));
    }
    target->addChapterEdition(TagLib::Matroska::ChapterEdition(list, true));
}

static std::string format_chapter_timestamp(uint32_t ms) {
    char buffer[32];
    std::snprintf(buffer, sizeof(buffer), "%02u:%02u:%02u.%03u", ms / 3600000, ms / 60000 % 60, ms / 1000 % 60, ms % 1000);
    return buffer;
}

static void write_comment_chapters(TagLib::Ogg::XiphComment* comment, const std::vector<BridgeChapter>& chapters) {
    TagLib::StringList oldKeys;
    for (const auto& field : comment->fieldListMap()) {
        if (field.first.startsWith("CHAPTER")) oldKeys.append(field.first);
    }
    for (const auto& key : oldKeys) {
        comment->removeFields(key);
    }
    for (size_t i = 0; i < chapters.size(); i++) {
        char key[24];
        std::snprintf(key, sizeof(key), "CHAPTER%03u", static_cast<unsigned int>(i + 1));
        comment->addField(key, format_chapter_timestamp(chapters[i].startMs));
        comment->addField(std::string(key) + "NAME", chapters[i].title);
    }
}

extern "C" {

int taglib_bridge_set_chapters(TagLibBridgeFile* file, const uint32_t* starts_ms, const char* const* titles, int32_t count) {
    if (!file || !file->fileRef || file->fileRef->isNull()) return 0;
    if (count > 0 && (!starts_ms || !titles)) return 0;
    try {
        TagLib::File* f = file->fileRef->file();
        const auto* audioProperties = f->audioProperties();
        const uint32_t durationMs = audioProperties ? clamp_chapter_ms(audioProperties->lengthInMilliseconds()) : 0;

        std::vector<BridgeChapter> chapters;
        chapters.reserve(static_cast<size_t>(count > 0 ? count : 0));
        for (int32_t i = 0; i < count; i++) {
            const uint32_t startMs = starts_ms[i];
            const uint32_t nextMs = i + 1 < count ? starts_ms[i + 1] : durationMs;
            const char* title = titles[i];
            chapters.push_back({startMs, nextMs > startMs ? nextMs : startMs, TagLib::String(title ? title : "", TagLib::String::UTF8)});
        }

        if (auto* mp4 = dynamic_cast<TagLib::MP4::File*>(f)) {
            write_mp4_chapters(mp4, chapters);
        } else if (auto* matroska = dynamic_cast<TagLib::Matroska::File*>(f)) {
            write_matroska_chapters(matroska, chapters);
        } else if (auto* id3v2 = id3v2_tag_of(f, true)) {
            write_id3v2_chapters(id3v2, chapters);
        } else if (auto* comment = xiph_comment_of(f)) {
            write_comment_chapters(comment, chapters);
        } else {
            return 0;
        }
        return 1;
    } catch (...) {
        return 0;
    }
}

} // extern "C"

// by claude
// Batch reading: a pool of native threads opens and parses the files, and hands
// each result to the caller as one packed allocation. Dart isolates are not
// involved, so a slow disk never holds a Dart mutator thread.

struct TagLibBatch {
    std::vector<std::string> paths;
    std::atomic<int32_t> nextIndex{0};
    int readStyle;
    bool readCover;
    bool readChapters;
    std::map<std::string, std::string> httpHeaders;
    int httpTimeoutMs;
    TagLibBatchItemCallback onItem;
    std::vector<std::thread> threads;
};

static void batch_append_u32(std::string& buffer, uint32_t value) {
    buffer.append(reinterpret_cast<const char*>(&value), sizeof(value));
}

static void batch_append_string(std::string& buffer, const TagLib::String& value) {
    const std::string utf8 = value.to8Bit(true);
    batch_append_u32(buffer, static_cast<uint32_t>(utf8.size()));
    buffer.append(utf8);
}

// Reads everything that can throw before allocating, so a failure leaks nothing.
static TagLibBatchItem* batch_read_item(int32_t index, TagLibBridgeFile* file, bool readCover, bool readChapters) {
    std::string region;
    uint32_t propertiesCount = 0;
    const TagLib::PropertyMap properties = file->fileRef->properties();
    for (auto it = properties.begin(); it != properties.end(); ++it) {
        batch_append_string(region, it->first);
        batch_append_u32(region, static_cast<uint32_t>(it->second.size()));
        for (const auto& value : it->second) {
            batch_append_string(region, value);
        }
        propertiesCount++;
    }
    const size_t propertiesSize = region.size();

    uint32_t chaptersCount = 0;
    if (readChapters) {
        for (const auto& chapter : read_chapters(file->fileRef->file(), properties)) {
            batch_append_u32(region, chapter.startMs);
            batch_append_u32(region, chapter.endMs);
            batch_append_string(region, chapter.title);
            chaptersCount++;
        }
    }
    const size_t chaptersSize = region.size() - propertiesSize;

    const char* format = taglib_bridge_get_format(file);
    const size_t formatOffset = region.size();
    if (format) {
        region.append(format);
        region.push_back('\0');
    }
    const size_t bitrateModeOffset = region.size();
    region.append(taglib_bridge_get_bitrate_mode(file));
    region.push_back('\0');

    const int32_t durationMs = taglib_bridge_get_duration(file);
    const int32_t bitrate = taglib_bridge_get_bitrate(file);
    const int32_t sampleRate = taglib_bridge_get_samplerate(file);
    const int32_t channels = taglib_bridge_get_channels(file);
    const int32_t bitsPerSample = taglib_bridge_get_bits_per_sample(file);
    const int32_t lossless = taglib_bridge_is_lossless(file);

    uint8_t* cover = nullptr;
    uint32_t coverSize = readCover ? taglib_bridge_front_cover_size(file) : 0;
    if (coverSize > 0) {
        cover = static_cast<uint8_t*>(std::malloc(coverSize));
        if (!cover || taglib_bridge_front_cover_data(file, cover, coverSize) != 1) {
            std::free(cover);
            cover = nullptr;
            coverSize = 0;
        }
    }

    auto* item = static_cast<TagLibBatchItem*>(std::malloc(sizeof(TagLibBatchItem) + region.size()));
    if (!item) {
        std::free(cover);
        return nullptr;
    }
    auto* regionPtr = reinterpret_cast<uint8_t*>(item + 1);
    std::memcpy(regionPtr, region.data(), region.size());

    item->index = index;
    item->success = 1;
    item->duration_ms = durationMs;
    item->bitrate = bitrate;
    item->sample_rate = sampleRate;
    item->channels = channels;
    item->bits_per_sample = bitsPerSample;
    item->lossless = lossless;
    item->properties_count = propertiesCount;
    item->properties_size = static_cast<uint32_t>(propertiesSize);
    item->properties = regionPtr;
    item->chapters_count = chaptersCount;
    item->chapters_size = static_cast<uint32_t>(chaptersSize);
    item->chapters = regionPtr + propertiesSize;
    item->format = format ? reinterpret_cast<const char*>(regionPtr + formatOffset) : nullptr;
    item->bitrate_mode = reinterpret_cast<const char*>(regionPtr + bitrateModeOffset);
    item->cover = cover;
    item->cover_size = coverSize;
    return item;
}

// Read-only, unlike taglib_bridge_open: no write handle that blocks concurrent
// opens of the same file on Windows or makes Android rescan it once closed.
static TagLibBridgeFile* batch_open_read_only(const TagLibBatch* batch, const char* path) {
    const int readStyle = batch->readStyle;
    if (std::strncmp(path, "http://", 7) == 0 || std::strncmp(path, "https://", 8) == 0) {
        return http_open_file(path, batch->httpHeaders, readStyle, batch->httpTimeoutMs);
    }

#ifdef __ANDROID__
    if (std::strncmp(path, "content://", 10) == 0) {
        const int fd = open_content_uri_fd(path, "r");
        return fd == -1 ? nullptr : taglib_bridge_open_fd_with_style(fd, readStyle);
    }
#endif

    try {
        bool readAudioProps = true;
        TagLib::AudioProperties::ReadStyle style = TagLib::AudioProperties::Average;
        resolve_read_style(readStyle, readAudioProps, style);

#ifdef _WIN32
        TagLib::String pathStr(path, TagLib::String::UTF8);
        TagLib::FileName filename(pathStr.toWString().c_str());
#else
        TagLib::FileName filename = path;
#endif
        auto stream = std::make_unique<TagLib::FileStream>(filename, true);
        if (!stream->isOpen()) return nullptr;
        auto fileRef = std::make_unique<TagLib::FileRef>(stream.get(), readAudioProps, style);
        if (fileRef->isNull()) return nullptr;

        auto bridge = new TagLibBridgeFile();
        bridge->fileRef = fileRef.release();
        bridge->stream = stream.release();
        return bridge;
    } catch (...) {
        return nullptr;
    }
}

static TagLibBatchItem* batch_failed_item(int32_t index) {
    auto* item = static_cast<TagLibBatchItem*>(std::calloc(1, sizeof(TagLibBatchItem)));
    if (item) {
        item->index = index;
        item->lossless = kLosslessUnknown;
    }
    return item;
}

// Workers mostly wait on disk, keep them from competing with the UI threads.
static void batch_lower_thread_priority() {
#if defined(_WIN32)
    SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_BELOW_NORMAL);
#elif defined(__APPLE__)
    pthread_set_qos_class_self_np(QOS_CLASS_UTILITY, 0);
#else
    setpriority(PRIO_PROCESS, 0, 10);
#endif
}

static void batch_worker(TagLibBatch* batch) {
    batch_lower_thread_priority();
    const int32_t count = static_cast<int32_t>(batch->paths.size());
    while (true) {
        const int32_t index = batch->nextIndex.fetch_add(1, std::memory_order_relaxed);
        if (index >= count) break;

        TagLibBatchItem* item = nullptr;
        TagLibBridgeFile* file = batch_open_read_only(batch, batch->paths[index].c_str());
        if (file) {
            try {
                item = batch_read_item(index, file, batch->readCover, batch->readChapters);
            } catch (...) {
            }
            taglib_bridge_close(file);
        }
        if (!item) item = batch_failed_item(index);
        if (item) batch->onItem(item);
    }

#ifdef __ANDROID__
    // content:// paths attach this thread to the JVM, which aborts the process
    // if a thread exits while attached.
    JNIEnv* env = nullptr;
    if (g_vm && g_vm->GetEnv(reinterpret_cast<void**>(&env), JNI_VERSION_1_6) == JNI_OK) {
        g_vm->DetachCurrentThread();
    }
#endif
}

extern "C" {

TagLibBatch* taglib_batch_start(const char* const* paths, int32_t count, int32_t threads, int32_t read_style, int32_t read_cover, int32_t read_chapters, const char* http_headers_json, int32_t http_timeout_ms, TagLibBatchItemCallback on_item) {
    if (!paths || count <= 0 || !on_item) return nullptr;

    auto* batch = new TagLibBatch();
    batch->paths.reserve(static_cast<size_t>(count));
    for (int32_t i = 0; i < count; i++) {
        batch->paths.emplace_back(paths[i] ? paths[i] : "");
    }
    batch->readStyle = read_style;
    batch->readCover = read_cover != 0;
    batch->readChapters = read_chapters != 0;
    batch->httpHeaders = parse_headers_json(http_headers_json);
    batch->httpTimeoutMs = http_timeout_ms;
    batch->onItem = on_item;

    const int32_t threadsLimited = threads < count ? threads : count;
    const int32_t threadCount = threadsLimited > 0 ? threadsLimited : 1;
    batch->threads.reserve(static_cast<size_t>(threadCount));
    try {
        for (int32_t i = 0; i < threadCount; i++) {
            batch->threads.emplace_back(batch_worker, batch);
        }
    } catch (...) {
        LOGE("taglib_batch_start: started %d of %d threads", static_cast<int>(batch->threads.size()), static_cast<int>(threadCount));
    }

    if (batch->threads.empty()) {
        delete batch;
        return nullptr;
    }
    return batch;
}

void taglib_batch_free(TagLibBatch* batch) {
    if (!batch) return;
    for (auto& thread : batch->threads) {
        thread.join();
    }
    delete batch;
}

void taglib_batch_item_free(TagLibBatchItem* item) {
    std::free(item);
}

} // extern "C"

package com.axel10.flutter_taglib

import android.content.Context
import io.flutter.embedding.engine.plugins.FlutterPlugin

class FlutterTaglibPlugin: FlutterPlugin {
    override fun onAttachedToEngine(binding: FlutterPlugin.FlutterPluginBinding) {
        System.loadLibrary("flutter_taglib")
        setNativeContext(binding.applicationContext)
    }

    override fun onDetachedFromEngine(binding: FlutterPlugin.FlutterPluginBinding) {
        clearNativeContext()
    }

    private external fun setNativeContext(context: Context)
    private external fun clearNativeContext()
}

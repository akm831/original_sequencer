package com.originalsequencer.prototype

import android.os.Handler
import android.os.Looper
import android.util.AtomicFile
import io.flutter.embedding.android.FlutterActivity
import io.flutter.embedding.engine.FlutterEngine
import io.flutter.plugin.common.MethodChannel
import java.io.File
import java.util.concurrent.Executors

class MainActivity : FlutterActivity() {
    companion object {
        private val storageExecutor = Executors.newSingleThreadExecutor()
    }
    override fun configureFlutterEngine(flutterEngine: FlutterEngine) {
        super.configureFlutterEngine(flutterEngine)
        val projectFile = AtomicFile(File(filesDir, "groove-project.json"))
        val mainHandler = Handler(Looper.getMainLooper())
        MethodChannel(flutterEngine.dartExecutor.binaryMessenger, "original_sequencer/project")
            .setMethodCallHandler { call, result ->
                if (call.method != "load" && call.method != "save") {
                    result.notImplemented()
                } else {
                    val source = call.arguments as? String
                    storageExecutor.execute {
                        try {
                            if (call.method == "load") {
                                val content = if (projectFile.baseFile.exists() || File(projectFile.baseFile.path + ".bak").exists()) {
                                    projectFile.openRead().use { input ->
                                        val bytes = input.readBytes()
                                        require(bytes.size <= 262144) { "Project is too large" }
                                        String(bytes, Charsets.UTF_8)
                                    }
                                } else null
                                mainHandler.post { result.success(content) }
                            } else {
                                require(source != null && source.toByteArray(Charsets.UTF_8).size <= 262144)
                                val stream = projectFile.startWrite()
                                try {
                                    stream.write(source.toByteArray(Charsets.UTF_8))
                                    projectFile.finishWrite(stream)
                                } catch (error: Exception) {
                                    projectFile.failWrite(stream)
                                    throw error
                                }
                                mainHandler.post { result.success(true) }
                            }
                        } catch (error: Exception) {
                            mainHandler.post { result.error("PROJECT_STORAGE", error.message, null) }
                        }
                    }
                }
            }
    }
}

package com.originalsequencer.prototype

import android.media.AudioAttributes
import android.media.AudioFocusRequest
import android.media.AudioManager
import android.os.Build
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
    private var focusChannel: MethodChannel? = null
    private var focusRequest: AudioFocusRequest? = null
    private val audioManager by lazy { getSystemService(AudioManager::class.java)!! }
    private val focusListener = AudioManager.OnAudioFocusChangeListener { change ->
        if (change < 0) focusChannel?.invokeMethod("lost", null)
    }
    @Suppress("DEPRECATION")
    private fun requestFocus(): Boolean {
        val result = if (Build.VERSION.SDK_INT >= 26) {
            val request = focusRequest ?: AudioFocusRequest.Builder(AudioManager.AUDIOFOCUS_GAIN)
                .setAudioAttributes(AudioAttributes.Builder().setUsage(AudioAttributes.USAGE_MEDIA)
                    .setContentType(AudioAttributes.CONTENT_TYPE_MUSIC).build())
                .setWillPauseWhenDucked(true)
                .setAcceptsDelayedFocusGain(false)
                .setOnAudioFocusChangeListener(focusListener, Handler(Looper.getMainLooper()))
                .build().also { focusRequest = it }
            audioManager.requestAudioFocus(request)
        } else audioManager.requestAudioFocus(focusListener, AudioManager.STREAM_MUSIC, AudioManager.AUDIOFOCUS_GAIN)
        return result == AudioManager.AUDIOFOCUS_REQUEST_GRANTED
    }
    @Suppress("DEPRECATION")
    private fun abandonFocus() {
        if (Build.VERSION.SDK_INT >= 26) focusRequest?.let { audioManager.abandonAudioFocusRequest(it) }
        else audioManager.abandonAudioFocus(focusListener)
    }
    override fun onDestroy() {
        abandonFocus()
        focusChannel?.setMethodCallHandler(null)
        focusChannel = null
        super.onDestroy()
    }
    override fun configureFlutterEngine(flutterEngine: FlutterEngine) {
        super.configureFlutterEngine(flutterEngine)
        focusChannel = MethodChannel(flutterEngine.dartExecutor.binaryMessenger, "original_sequencer/audio_focus")
            .also { channel -> channel.setMethodCallHandler { call, result ->
                when (call.method) {
                    "request" -> result.success(requestFocus())
                    "abandon" -> { abandonFocus(); result.success(null) }
                    else -> result.notImplemented()
                }
            } }
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
                                        val bytes = ByteArray(262145)
                                        var count = 0
                                        while (count < bytes.size) {
                                            val read = input.read(bytes, count, bytes.size - count)
                                            if (read < 0) break
                                            require(read > 0) { "Project read failed" }
                                            count += read
                                        }
                                        require(count <= 262144) { "Project is too large" }
                                        String(bytes, 0, count, Charsets.UTF_8)
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

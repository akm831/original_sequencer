package com.originalsequencer.prototype

import android.app.Activity
import android.content.Intent
import java.io.InputStream
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
    private var documentResult: MethodChannel.Result? = null
    private var documentSource: String? = null
    private val documentRequest = 43102
    private fun readProject(input: InputStream): String {
        val bytes = ByteArray(262145)
        var count = 0
        while (count < bytes.size) {
            val read = input.read(bytes, count, bytes.size - count)
            if (read < 0) break
            require(read > 0) { "Project read failed" }
            count += read
        }
        require(count <= 262144) { "Project is too large" }
        return String(bytes, 0, count, Charsets.UTF_8)
    }
    @Suppress("DEPRECATION")
    private fun selectDocument(export: Boolean, source: String?, result: MethodChannel.Result) {
        if (documentResult != null) { result.error("PROJECT_BUSY", "Document picker is already open", null); return }
        if (export && (source == null || source.toByteArray(Charsets.UTF_8).size > 262144)) {
            result.error("PROJECT_EXPORT", "Invalid project", null); return
        }
        val intent = Intent(if (export) Intent.ACTION_CREATE_DOCUMENT else Intent.ACTION_OPEN_DOCUMENT)
            .addCategory(Intent.CATEGORY_OPENABLE)
            .setType(if (export) "application/json" else "*/*")
        if (export) intent.putExtra(Intent.EXTRA_TITLE, "groove-project.json")
        documentResult = result
        documentSource = if (export) source else null
        try { startActivityForResult(intent, documentRequest) }
        catch (error: Exception) { documentResult = null; documentSource = null; result.error("PROJECT_PICKER", error.message, null) }
    }
    @Deprecated("Android legacy result API used for minimum API 24 compatibility")
    override fun onActivityResult(requestCode: Int, resultCode: Int, data: Intent?) {
        super.onActivityResult(requestCode, resultCode, data)
        if (requestCode != documentRequest) return
        val result = documentResult ?: return
        val source = documentSource
        documentResult = null; documentSource = null
        val uri = data?.data
        if (resultCode != Activity.RESULT_OK || uri == null) { result.success(null); return }
        val handler = Handler(Looper.getMainLooper())
        storageExecutor.execute {
            try {
                if (source != null) {
                    val output = contentResolver.openOutputStream(uri, "wt") ?: error("Cannot open export file")
                    output.use { it.write(source.toByteArray(Charsets.UTF_8)) }
                    handler.post { result.success(true) }
                } else {
                    val input = contentResolver.openInputStream(uri) ?: error("Cannot open import file")
                    val content = input.use { readProject(it) }
                    handler.post { result.success(content) }
                }
            } catch (error: Exception) { handler.post { result.error("PROJECT_DOCUMENT", error.message, null) } }
        }
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
        documentResult?.success(null)
        documentResult = null; documentSource = null
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
                if (call.method == "export" || call.method == "import") {
                    selectDocument(call.method == "export", call.arguments as? String, result)
                } else if (call.method != "load" && call.method != "save" && call.method != "backup_original") {
                    result.notImplemented()
                } else {
                    val source = call.arguments as? String
                    storageExecutor.execute {
                        try {
                            if (call.method == "load") {
                                val content = if (projectFile.baseFile.exists() || File(projectFile.baseFile.path + ".bak").exists()) {
                                    projectFile.openRead().use { readProject(it) }
                                } else null
                                mainHandler.post { result.success(content) }
                            } else if (call.method == "backup_original") {
                                val original = if (projectFile.baseFile.exists()) projectFile.baseFile else File(projectFile.baseFile.path + ".bak")
                                if (original.exists()) original.copyTo(File(filesDir, "groove-recovery-${System.currentTimeMillis()}.json"))
                                mainHandler.post { result.success(true) }
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

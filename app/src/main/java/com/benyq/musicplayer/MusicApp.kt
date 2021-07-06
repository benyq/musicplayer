package com.benyq.musicplayer

import android.app.Application
import android.util.Log
import java.io.File
import java.io.FileOutputStream
import java.lang.Exception

class MusicApp : Application(){

    override fun onCreate() {
        super.onCreate()
        copyMusic()
    }

    private fun copyMusic() {
        val parentPath = getExternalFilesDir("music")!!.absolutePath + File.separator

        assets.list("")?.forEach { file->
            if (file.contains("mp3") || file.contains("flac")) {
                val music = parentPath + file
                if (File(music).exists()) {
                    return@forEach
                }
                try {
                    FileOutputStream(music).use { ous->
                        assets.open(file).use { ins->
                            ins.copyTo(ous)
                        }
                    }
                }catch (e : Exception) {
                    e.printStackTrace()
                }
            }
        }

    }

}
package com.benyq.musicplayer

import android.media.MediaPlayer
import android.os.Bundle
import android.util.Log
import android.widget.SeekBar
import androidx.appcompat.app.AppCompatActivity
import androidx.recyclerview.widget.DividerItemDecoration
import androidx.recyclerview.widget.LinearLayoutManager
import com.benyq.musicplayer.databinding.ActivityMainBinding
import java.io.File

class MainActivity : AppCompatActivity() {


    private val TAG = "MainActivity"

    private val binding: ActivityMainBinding by lazy { ActivityMainBinding.inflate(layoutInflater) }
    private val musicAdapter by lazy { MusicAdapter() }
    private val musicPlayer = MusicPlayer()

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        setContentView(binding.root)

        musicPlayer.setOnProgressListener {
            Log.e(TAG, "onCreate: progress $it")
            runOnUiThread {
                binding.sbProgress.progress = it * 100 / musicPlayer.duration
                binding.tvCurrentDuration.text = durationToStr(it)
            }
        }

        musicPlayer.setOnPreparedListener {
            runOnUiThread {
                binding.tvMaxDuration.text = durationToStr(musicPlayer.duration)
                updatePlayState(true)
            }
            musicPlayer.play()
            Log.e(TAG, "musicPlayer play")
        }
        musicPlayer.setOnErrorListener {
            Log.e(TAG, "musicPlayer error : $it")
        }

        binding.rvMusic.run {
            layoutManager = LinearLayoutManager(this@MainActivity)
            adapter = musicAdapter
            addItemDecoration(
                DividerItemDecoration(
                    this@MainActivity,
                    DividerItemDecoration.VERTICAL
                )
            )
            setData()
            musicPlayer.setDataSource(musicAdapter.data[0].path)
            musicAdapter.updateAdapter(0)
        }
        musicAdapter.setMusicItemAction { path, position ->
            musicPlayer.stop()
            musicPlayer.setDataSource(path)
            musicPlayer.prepare()
            musicAdapter.updateAdapter(position)
        }
        binding.ivMusicPlay.setOnClickListener {
            if (musicPlayer.isPlaying) {
                updatePlayState(false)
                musicPlayer.pause()
            } else if (musicPlayer.isPrepared) {
                updatePlayState(true)
                musicPlayer.play()
            } else {
                updatePlayState(true)
                musicPlayer.prepare()
            }
        }
        binding.ivMusicNext.setOnClickListener {
            var position = musicAdapter.getSelectedPosition()
            if (position == -1) {
                position = 0
            }else {
                position = (position + 1) % musicAdapter.data.size
            }

            musicPlayer.stop()
            musicPlayer.setDataSource(musicAdapter.data[position].path)
            musicPlayer.prepare()
            musicAdapter.updateAdapter(position)
        }
        binding.ivMusicLast.setOnClickListener {
            var position = musicAdapter.getSelectedPosition()
            if (position == -1) {
                position = 0
            }else {
                position = (position + musicAdapter.data.size - 1) % musicAdapter.data.size
            }

            musicPlayer.stop()
            musicPlayer.setDataSource(musicAdapter.data[position].path)
            musicPlayer.prepare()
            musicAdapter.updateAdapter(position)
        }
        binding.sbProgress.setOnSeekBarChangeListener(object : SeekBar.OnSeekBarChangeListener {
            private var seekTime = 0

            override fun onProgressChanged(seekBar: SeekBar?, progress: Int, fromUser: Boolean) {
                seekTime = progress * musicPlayer.duration / 100
            }

            override fun onStartTrackingTouch(seekBar: SeekBar?) {

            }

            override fun onStopTrackingTouch(seekBar: SeekBar?) {
                musicPlayer.seekTo(seekTime)
                Log.d(TAG, "onProgressChanged: $seekTime")
            }
        })
    }

    private fun setData() {

        val parentPath = getExternalFilesDir("music")!!.absolutePath + File.separator
        File(parentPath).run {
            if (exists()) {
                val data = mutableListOf<Music>()
                listFiles()?.forEach {

                    val start = it.absolutePath.lastIndexOf(File.separator) + 1
                    val end = it.absolutePath.lastIndexOf(".")

                    data.add(Music(it.absolutePath.substring(start, end), it.absolutePath))
                }
                musicAdapter.data = data
            }
        }
    }

    override fun onDestroy() {
        super.onDestroy()
        musicPlayer.stop()
        musicPlayer.release()
    }

    private fun updatePlayState(playState: Boolean) {
        binding.ivMusicPlay.setImageResource(if (playState) R.drawable.ic_music_pause else R.drawable.ic_music_play)
    }
    private fun durationToStr(duration: Int): String {
        Log.e(TAG, "durationToStr: $duration")
        val min = duration / 60
        val sec = duration % 60
        return "${if (min < 10) "0$min" else min} : ${if (sec < 10) "0$sec" else sec}"
    }
}
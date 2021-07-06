package com.benyq.musicplayer

import android.content.Intent
import androidx.appcompat.app.AppCompatActivity
import android.os.Bundle
import com.benyq.musicplayer.databinding.ActivityMainBinding
import com.benyq.musicplayer.databinding.ActivitySplashBinding

class SplashActivity : AppCompatActivity() {

    private val binding: ActivitySplashBinding by lazy { ActivitySplashBinding.inflate(layoutInflater) }

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        setContentView(binding.root)

        binding.btnGo.setOnClickListener {
            startActivity(Intent(this, MainActivity::class.java))
        }
    }
}
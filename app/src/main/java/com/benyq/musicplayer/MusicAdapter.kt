package com.benyq.musicplayer

import android.view.LayoutInflater
import android.view.ViewGroup
import androidx.recyclerview.widget.RecyclerView
import com.benyq.musicplayer.databinding.ItemMusicBinding


typealias MusicItemAction = (String, Int)->Unit

class MusicAdapter : RecyclerView.Adapter<MusicViewHolder>(){

    var data = mutableListOf<Music>()
    set(value) {
        field.clear()
        field.addAll(value)
    }

    private var musicItemAction: MusicItemAction? = null

    override fun onCreateViewHolder(parent: ViewGroup, viewType: Int): MusicViewHolder {
        val binding: ItemMusicBinding = ItemMusicBinding.inflate(LayoutInflater.from(parent.context), parent, false)
        return MusicViewHolder(binding)
    }

    override fun onBindViewHolder(holder: MusicViewHolder, position: Int) {
        holder.binding.root.setOnClickListener {
            musicItemAction?.invoke(data[position].path, position)
        }
        holder.binding.tvMusicName.text = data[position].name
        holder.binding.tvMusicName.setTextColor(holder.itemView.context.resources.getColor(if (data[position].selected) R.color.ali_color else R.color.black))
    }

    override fun getItemCount() = data.size

    fun setMusicItemAction(action: MusicItemAction) {
        musicItemAction = action
    }

    fun updateAdapter(newPosition: Int) {
        var oldPosition = -1
        data.forEachIndexed { index, music ->
            if (music.selected) {
                oldPosition = index
            }
            music.selected = index == newPosition
        }
        if (oldPosition != -1) {
            notifyItemChanged(oldPosition)
        }
        notifyItemChanged(newPosition)
    }

    fun getSelectedPosition(): Int {
        data.forEachIndexed { index, music ->
            if (music.selected) {
                return index
            }
        }
        return -1
    }

}


class MusicViewHolder(val binding: ItemMusicBinding) : RecyclerView.ViewHolder(binding.root) {
}

data class Music(val name: String, val path: String, var selected: Boolean = false)


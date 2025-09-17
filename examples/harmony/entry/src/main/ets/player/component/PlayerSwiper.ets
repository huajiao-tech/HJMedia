import { LivePlayerState, LivePlayerStore } from "../store/LivePlayerStore"
import { PlayerXComponent } from "./PlayerXComponent"
import { HJPlayer, HJPlayerSourceType } from 'hjplayer';
import { emitter } from "@kit.BasicServicesKit";

@ComponentV2
export struct PlayerSwiper {
  @Consumer("LivePlayerStore") store?: LivePlayerStore
  livePlayerState?: LivePlayerState = this.store?.getState()
  @Local currentIndex: number = 0

  build() {
    Swiper() {
      Repeat(this.livePlayerState?.PlayMediaInfoList)
        .each(iterator => {
          Stack() {
            if (iterator.item.isPlaying) {
              PlayerXComponent({ url: iterator.item.url })
            }
          }
          .size({ width: "100%", height: "100%" })
        })
    }
    .loop(false)
    .indicator(false)
    .vertical(true)
    .size({ width: "100%", height: "100%" })
    .onChange(index => {
      this.currentIndex = index
      if (this.livePlayerState?.PlayMediaInfoList.length && index + 1 < this.livePlayerState?.PlayMediaInfoList.length) {
        HJPlayer.preloadUrl(this.livePlayerState?.PlayMediaInfoList[index + 1].url ?? "")
      }
      this.livePlayerState!.PlayMediaInfoList.forEach((item, index) => {
        item.isPlaying = false
      })
      this.livePlayerState!.PlayMediaInfoList[index].isPlaying = true
    })
  }
}
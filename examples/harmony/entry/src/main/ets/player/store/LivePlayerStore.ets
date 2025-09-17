import { State, Store, Action } from '../../interface/Store';
import { WindowUtils } from '../../util/WindowUtils';
import { window } from '@kit.ArkUI';
import { NavStackUtil } from '../../util/NavStackUitl';
import { PusherSetting } from '../../pusher/model/PusherPreviewModel';
import { emitter } from '@kit.BasicServicesKit';
import { EMITTER_SOUND } from '../../common/EmitterConstant';

@ObservedV2
class PlayMediaInfo {
  @Trace url: string
  @Trace isPlaying: boolean

  constructor(url: string, isPlaying: boolean) {
    this.url = url
    this.isPlaying = isPlaying
  }
}

@ObservedV2
export class LivePlayerState implements State {
  @Trace PlayMediaInfoList: PlayMediaInfo[] = [
    new PlayMediaInfo("http://vjs.zencdn.net/v/oceans.mp4", true),
    new PlayMediaInfo("http://www.w3school.com.cn/example/html5/mov_bbb.mp4", false),
    new PlayMediaInfo("http://vjs.zencdn.net/v/oceans.mp4", false),
    new PlayMediaInfo("https://www.w3schools.com/html/movie.mp4", false),
    new PlayMediaInfo("https://media.w3.org/2010/05/sintel/trailer.mp4", false),
    new PlayMediaInfo("http://vjs.zencdn.net/v/oceans.mp4", false),
  ]
  @Trace isGiftShow: boolean = false
  isLandscape: boolean = false
  pusherSettings: PusherSetting[] = [
    new PusherSetting("sound", "left", $r("app.media.sound_0_icon"), $r("app.media.sound_1_icon"),  true),
    new PusherSetting("gift", "right", $r("app.media.gift_icon"), $r("app.media.gift_icon"),  false),
  ]
}

export class LivePlayerStore implements Store<LivePlayerState> {
  state = new LivePlayerState()
  getState = () => this.state

  aboutToAppear() {
    this.state.isLandscape = NavStackUtil.getParams<boolean>() ?? false
    emitter.on("closeGift", () => {
      this.state.isGiftShow = false
    })
  }

  aboutToDisappear(): void {
    emitter.off("closeGift")
  }

  onPageShow(): void {
    WindowUtils.setWindowLayoutFullScreen(true)
    if (this.state.isLandscape) {
      WindowUtils.setOrientation(window.Orientation.LANDSCAPE)
    }
  }

  onPageHide(): void {
    WindowUtils.setWindowLayoutFullScreen(false)
    if (this.state.isLandscape) {
      WindowUtils.setOrientation(window.Orientation.UNSPECIFIED)
    }
  }

  dispatch(action: Action<LivePlayerState>): void {
    if (action instanceof GiftAction) {
      this.state.isGiftShow = false
      setTimeout(() => {
        this.state.isGiftShow = true
      }, 10)
    } else if (action instanceof SoundAction) {
      const index = action.payload
      this.state.pusherSettings[index].value = !this.state.pusherSettings[index].value
      emitter.emit<boolean>(EMITTER_SOUND, {data: !this.state.pusherSettings[index].value})
    }
  }
}

export class GiftAction implements Action<LivePlayerState> {
  payload: string

  constructor(payload: string) {
    this.payload = payload
  }
}

export class SoundAction implements Action<LivePlayerState> {
  payload: number

  constructor(payload: number) {
    this.payload = payload
  }
}
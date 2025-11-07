import { State, Store, Action } from '../../interface/Store';
import { WindowUtils } from '../../util/WindowUtils';
import { window } from '@kit.ArkUI';
import { FaceuEventData } from './PlayerXComponentStore';
import { NavStackUtil } from '../../util/NavStackUitl';
import { PusherSetting } from '../../pusher/model/PusherPreviewModel';
import { emitter } from '@kit.BasicServicesKit';
import { EMITTER_SOUND } from '../../common/EmitterConstant';
import { FaceDetectAction, FaceuAction } from './PlayerSettingStore';
import { EMITTER_FACEDETECT, EMITTER_FACEU } from '../../common/EmitterConstant';

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
    new PlayMediaInfo("https://live-pull-3.huajiao.com/main/HJ_0_tc_3_main__h265_273029340_1758867109146_1123_O.flv?txSecret=b0e93402598373fa9874a35f4f4d7959&txTime=68D78083", true),
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
    new PusherSetting("facedetect", "right", $r("app.media.facedetect_close"), $r("app.media.facedetect_open"),  false),
    new PusherSetting("faceu", "right", $r("app.media.faceu_close"), $r("app.media.faceu_open"),  false),
    new PusherSetting("gift", "right", $r("app.media.gift_icon"), $r("app.media.gift_icon"),  false),
  ]
}

export class LivePlayerStore implements Store<LivePlayerState> {
  state = new LivePlayerState()
  getState = () => this.state
  private m_bFaceDetect: boolean = false;
  private m_bFaceu: boolean = false;

  aboutToAppear() {
    const param = NavStackUtil.getParams<[string, boolean]>()
    this.state.PlayMediaInfoList[0].url = param[0]
    this.state.isLandscape = param[1]

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
  switchIcon(index : number)
  {
      this.state.pusherSettings[index].value = !this.state.pusherSettings[index].value
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
    } else if (action instanceof FaceuAction) {
        console.log("faceu player");
        this.m_bFaceu = !this.m_bFaceu;
        emitter.emit<FaceuEventData>(EMITTER_FACEU, { data: { enable: this.m_bFaceu, url: action.url } });
        this.switchIcon(action.payload);
    }
    else if (action instanceof FaceDetectAction){
        this.m_bFaceDetect = !this.m_bFaceDetect;
        emitter.emit(EMITTER_FACEDETECT, { data: this.m_bFaceDetect });
        this.switchIcon(action.payload);
    }
  }
}

export class GiftAction implements Action<LivePlayerState> {
  payload: string

  constructor(payload: string) {
    this.payload = payload
  }
}1

export class SoundAction implements Action<LivePlayerState> {
  payload: number

  constructor(payload: number) {
    this.payload = payload
  }
}
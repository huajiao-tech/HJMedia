import { State, Store, Action } from '../../interface/Store';
import { MediaConfig } from '../../pusher/model/PusherModel';
import { NavStackUtil } from '../../util/NavStackUitl';
import { WindowUtils } from '../../util/WindowUtils';

@ObservedV2
export class PlayerSettingState implements State {
  @Trace playerConfigArray: MediaConfig[] = [
    new MediaConfig("string", 'URL 播放地址', 'https://live-pull-3.huajiao.com/main/HJ_0_tc_3_main__h265_271191270_1780381990844_9551_O.flv?txSecret=ea5d6545efddb72e5d8d287fdd6eb9ce&txTime=6A1FCAB3'),
    // new MediaConfig("string", 'URL 播放地址', '/data/storage/el2/base/haps/entry/files/hjmedias/1525758055919491248.mp4'),
    new MediaConfig("boolean", '是否横屏', false),
    // new MediaConfig("boolean", '是否保持屏幕常亮', false),
    new MediaConfig("boolean", '边下边播', false),
    new MediaConfig("boolean", '点播', false),
  ]
}

export class PlayerSettingStore implements Store<PlayerSettingState> {
  state = new PlayerSettingState()
  getState = () => this.state

  onPageShow(): void {
    WindowUtils.setWindowLayoutFullScreen(true)
  }

  onPageHide(): void {
    WindowUtils.setWindowLayoutFullScreen(false)
  }
  dispatch(action: Action<PlayerSettingState>): void {
    if (action instanceof PlayAction) {
      NavStackUtil.jumpPush({ name: "LivePlayerPage", param: [this.state.playerConfigArray[0].value, this.state.playerConfigArray[1].value, this.state.playerConfigArray[2].value, this.state.playerConfigArray[3].value] })
    }
  }
}

export class PlayAction implements Action<PlayerSettingState> {
  payload: number

  constructor(payload: number) {
    this.payload = payload
  }
}

export class FaceuAction implements Action<PlayerSettingState> {
  payload: number
  url: string
  constructor(payload: number, url: string) {
    this.payload = payload
    this.url = url
  }
}

export class FaceDetectAction implements Action<PlayerSettingState> {
  payload: number

  constructor(payload: number) {
    this.payload = payload
  }
}
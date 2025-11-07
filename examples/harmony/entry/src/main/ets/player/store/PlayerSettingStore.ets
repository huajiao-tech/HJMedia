import { State, Store, Action } from '../../interface/Store';
import { MediaConfig } from '../../pusher/model/PusherModel';
import { NavStackUtil } from '../../util/NavStackUitl';
import { WindowUtils } from '../../util/WindowUtils';

@ObservedV2
export class PlayerSettingState implements State {
  @Trace playerConfigArray: MediaConfig[] = [
    new MediaConfig("string", 'URL 播放地址', 'https://live-pull-2.huajiao.com/main/HJ_0_ali_2_main__h265_263373740_1762484820951_2776_O.flv?auth_key=1762571328-0-0-50c9a99db36020a633251aabd006c130'),
    new MediaConfig("boolean", '是否横屏', false),
    // new MediaConfig("boolean", '是否保持屏幕常亮', false),
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
      NavStackUtil.jumpPush({ name: "LivePlayerPage", param: [this.state.playerConfigArray[0].value, this.state.playerConfigArray[1].value] })
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
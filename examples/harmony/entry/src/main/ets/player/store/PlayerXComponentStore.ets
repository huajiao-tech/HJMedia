import { HJPlayer, HJPlayerNotifyType,
  HJPlayerType,
  HJPlayerVideoCodecType, OpenPlayerInfo,
  SetWindowInfo,
  SetWindowState } from 'hjplayer';
import { State, Store, Action } from '../../interface/Store';
import { emitter } from '@kit.BasicServicesKit';
import { EMITTER_SOUND } from '../../common/EmitterConstant';

@ObservedV2
export class PlayerXComponentState implements State {
  @Trace x: number = 0;
  @Trace y: number = 0;
  @Trace w: number | string = "100%";
  @Trace h: number | string = "100%";
  XW: number = 0
  XH: number = 0
  url: string = "";
  openPlayerInfo: OpenPlayerInfo = {} as OpenPlayerInfo;

  surfaceId: string = "";
  hjPlayer: HJPlayer = new HJPlayer()
}

export class PlayerXComponentStore implements Store<PlayerXComponentState> {
  state = new PlayerXComponentState()
  getState = () => this.state

  soundCall: Callback<emitter.GenericEventData<boolean>> = (eventData) => {
    this.state.hjPlayer.setMute(eventData.data ?? false)
  }

  init(openPlayerInfo: OpenPlayerInfo) {
    this.state.openPlayerInfo = openPlayerInfo
    this.state.hjPlayer.createPlayer()
    emitter.on<boolean>(EMITTER_SOUND, this.soundCall)
  }

  aboutToDisappear() {
    emitter.off<boolean>(EMITTER_SOUND, this.soundCall)
    this.close()
  }

  dispatch(action: Action<PlayerXComponentState>): void {
    if (action instanceof OpenAction) {
      if (action.payload) {
        this.state.openPlayerInfo = action.payload
      }
      this.open()
    } else if (action instanceof WindowAction) {
      this.state.hjPlayer.setWindow(action.payload)
      this.state.XW = action.payload.width
      this.state.XH = action.payload.height
      this.state.surfaceId = action.payload.surfaceId
    }
  }

  private open() {
    this.state.hjPlayer.openPlayer(this.state.openPlayerInfo, (str: string) => {
      const strJson: ESObject = JSON.parse(str)
      console.info("uio123", strJson.type, strJson.msgInfo);
      if (strJson.type == HJPlayerNotifyType.HJ_PLAYER_NOTIFY_EOF) {
        emitter.emit("closeGift")
      } else if (strJson.type == HJPlayerNotifyType.HJ_PLAYER_NOTIFY_CLOSEDONE) { // 可以释放播放器资源
        this.state.hjPlayer.exitPlayer()
        this.state.hjPlayer.destroyPlayer()
      }
    }, { uid: 2342, device: "Harmony", sn: "HJPlayer" }, (str: string) => {
      // console.info("uio123", str)
    })
  }

  private close() {
    this.state.hjPlayer.closePlayer()
    this.state.hjPlayer.setWindow({
      surfaceId: this.state.surfaceId,
      width: 0,
      height: 0,
      state: SetWindowState.TARGET_DESTROY
    })
  }
}

export class OpenAction implements Action<PlayerXComponentState> {
  payload?: OpenPlayerInfo

  constructor(payload?: OpenPlayerInfo) {
    this.payload = payload
  }
}

export class WindowAction implements Action<PlayerXComponentState> {
  payload: SetWindowInfo

  constructor(payload: SetWindowInfo) {
    this.payload = payload
  }
}
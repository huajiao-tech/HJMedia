import { HJPlayer, HJPlayerNotifyType,
  HJPlayerType,
  HJPlayerVideoCodecType, OpenPlayerInfo,
  SetWindowInfo,
  SetWindowState } from 'hjplayer';
import { State, Store, Action } from '../../interface/Store';
import { emitter } from '@kit.BasicServicesKit';
import { EMITTER_FACEDETECT, EMITTER_SOUND, EMITTER_FACEU } from '../../common/EmitterConstant';
//import {HJFaceDetectMgr, HJNativeSourceData, HJFaceDetectMgrRet} from 'hjmediautils'
import {HJFaceDetectMgr, HJNativeSourceData, HJFaceDetectMgrRet} from '../../../../../../hjpusher/src/main/ets/HJFaceDetectMgr'
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

export interface FaceuEventData {
  enable: boolean;
  url: string;
}

export class PlayerXComponentStore implements Store<PlayerXComponentState> {
  state = new PlayerXComponentState()
  private m_faceDetectMgr : HJFaceDetectMgr = new HJFaceDetectMgr();
  getState = () => this.state

  soundCall: Callback<emitter.GenericEventData<boolean>> = (eventData) => {
    this.state.hjPlayer.setMute(eventData.data ?? false)
  }

  async faceDetectCtrl(bFaceDetect: boolean)
  {
      if (bFaceDetect)
      {
        this.m_faceDetectMgr.registerFaceInfoCb((width:number, height:number, faceInfo:string)=>
        {
          this.state.hjPlayer.setFaceInfo(width, height, faceInfo);
        });
        this.m_faceDetectMgr.registerNativeSourceOpenCb(() =>
        {
          this.state.hjPlayer.nativeSourceOpen(true);
          return 0;
        });
        this.m_faceDetectMgr.registerNativeSourceAcquireCb((): HJNativeSourceData | null =>
        {
          return this.state.hjPlayer.nativeSourceAcquire();
        });
        this.m_faceDetectMgr.registerNativeSourceCloseCb(()=>
        {
          this.state.hjPlayer.nativeSourceClose();
        });
        let faceDetectRet: number = await this.m_faceDetectMgr.openFaceDetect();

        if (faceDetectRet != HJFaceDetectMgrRet.ok)
        {
          await this.m_faceDetectMgr.closeFaceDetect();
          console.log("openFaceDetect error");
        }
      }
      else
      {
          await this.m_faceDetectMgr.closeFaceDetect();
      }
  }

  faceutCtrl(bFaceu: boolean, url: string)
  {
      if (bFaceu)
      {
          this.state.hjPlayer.openFaceu(url);
      }
      else
      {
          this.state.hjPlayer.closeFaceu();
      }
  }

  faceDetectCall: Callback<emitter.GenericEventData<boolean>> = (eventData) => {
    //console.log(`faceDetectCall received: ${JSON.stringify(eventData)}`);
    //this.state.hjPlayer.setMute(eventData.data ?? false)
    const data = eventData?.data;
    if (typeof data === "boolean")
    {
        console.log("playerx emitter facedetect: " + data);
        this.faceDetectCtrl(data);
    }
  }
  faceuCall: Callback<emitter.GenericEventData<FaceuEventData>> = (eventData) => {
      const data = eventData?.data;
      if (data && typeof data.enable === 'boolean' && typeof data.url === 'string') {
        const enable = data.enable;
        const url = data.url;
        console.log(`playerx emitter received: enable=${enable}, url=${url}`);
        this.faceutCtrl(enable, url);
      }
  }

  init(openPlayerInfo: OpenPlayerInfo) {
    this.state.openPlayerInfo = openPlayerInfo
    this.state.hjPlayer.createPlayer()
    emitter.on<boolean>(EMITTER_SOUND, this.soundCall)
    emitter.on<boolean>(EMITTER_FACEDETECT, this.faceDetectCall)
    emitter.on<FaceuEventData>(EMITTER_FACEU, this.faceuCall)
  }

  aboutToDisappear() {
    emitter.off<boolean>(EMITTER_SOUND, this.soundCall)
    emitter.off<boolean>(EMITTER_FACEDETECT, this.faceDetectCall)
    emitter.off<FaceuEventData>(EMITTER_FACEU, this.faceuCall)

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
      else if (strJson.type == HJPlayerNotifyType.HJ_RENDER_NOTIFY_FACEU_COMPLETE)
      {
        console.log("faceu complete")
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
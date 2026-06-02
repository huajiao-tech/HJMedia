import { HJPlayer, HJPlayerNotifyType,
  HJRenderNotifyType,
  HJPlayerType,
  HJPlayerSeiData, HJPlayerSeiInfo,
  HJPlayerVideoCodecType, OpenPlayerInfo,
  SetWindowInfo,
  SetWindowState } from 'hjplayer';
import { HJRteGraphConfig } from 'hjplayer/src/main/ets/native/HJRteGraphSetupInfo';
import { State, Store, Action } from '../../interface/Store';
import { emitter } from '@kit.BasicServicesKit';
import { EMITTER_FACEDETECT, EMITTER_SOUND, EMITTER_FACEU } from '../../common/EmitterConstant';
//import {HJFaceDetectMgr, HJNativeSourceData, HJFaceDetectMgrRet} from 'hjmediautils'
import {HJFaceDetectMgr, HJNativeSourceData, HJFaceDetectMgrRet} from '../../../../../../hjmediautils/src/main/ets/HJFaceDetectMgr'

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

interface HJPlayerNotifyInfo {
  type: number;
  msgInfo: string;
}

interface HJDecodedSeiData {
  uuid: string;
  data: string;
  byteLength: number;
  hexData: string;
}

const HJ_HEX_TABLE: string = "0123456789abcdef";

function decodeUtf8Bytes(bytes: Uint8Array): string {
  let result: string = "";
  let index = 0;

  while (index < bytes.length) {
    const first = bytes[index];
    if (first < 0x80) {
      result += String.fromCharCode(first);
      index++;
      continue;
    }

    if ((first & 0xE0) == 0xC0 && index + 1 < bytes.length) {
      const second = bytes[index + 1];
      if ((second & 0xC0) == 0x80) {
        const codePoint = ((first & 0x1F) << 6) | (second & 0x3F);
        result += String.fromCharCode(codePoint);
        index += 2;
        continue;
      }
    }

    if ((first & 0xF0) == 0xE0 && index + 2 < bytes.length) {
      const second = bytes[index + 1];
      const third = bytes[index + 2];
      if ((second & 0xC0) == 0x80 && (third & 0xC0) == 0x80) {
        const codePoint = ((first & 0x0F) << 12) | ((second & 0x3F) << 6) | (third & 0x3F);
        result += String.fromCharCode(codePoint);
        index += 3;
        continue;
      }
    }

    if ((first & 0xF8) == 0xF0 && index + 3 < bytes.length) {
      const second = bytes[index + 1];
      const third = bytes[index + 2];
      const fourth = bytes[index + 3];
      if ((second & 0xC0) == 0x80 && (third & 0xC0) == 0x80 && (fourth & 0xC0) == 0x80) {
        const codePoint = ((first & 0x07) << 18) | ((second & 0x3F) << 12) | ((third & 0x3F) << 6) |
          (fourth & 0x3F);
        const offset = codePoint - 0x10000;
        result += String.fromCharCode(0xD800 + (offset >> 10), 0xDC00 + (offset & 0x3FF));
        index += 4;
        continue;
      }
    }

    result += '\uFFFD';
    index++;
  }

  return result;
}

function encodeBytesToHex(bytes: Uint8Array): string {
  let result: string = "";
  for (let i = 0; i < bytes.length; i++) {
    const value = bytes[i];
    result += HJ_HEX_TABLE.charAt((value >> 4) & 0x0F);
    result += HJ_HEX_TABLE.charAt(value & 0x0F);
    result += ' ';
  }
  return result;
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
          this.state.hjPlayer.setFaceInfo(HJRteGraphConfig.HJNodeClass_SourceBridge, width, height, faceInfo);
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

  private handleSeiInfos(seiInfo: HJPlayerSeiInfo) {
    try {
      const decodedSeiDatas: HJDecodedSeiData[] = [];
      const seiDatas: HJPlayerSeiData[] = Array.isArray(seiInfo.seiDatas) ? seiInfo.seiDatas : [];

      for (let i = 0; i < seiDatas.length; i++) {
        const item = seiDatas[i];
        const bytes = new Uint8Array(item.data);
        decodedSeiDatas.push({
          uuid: item.uuid,
          data: decodeUtf8Bytes(bytes),
          byteLength: bytes.byteLength,
          hexData: encodeBytesToHex(bytes)
        });
      }

      console.info("HJ_PLAYER_NOTIFY_SEI_INFOS 202603101102", JSON.stringify({
        isKeyFrame: !!seiInfo.isKeyFrame,
        seiDatas: decodedSeiDatas
      }));
    } catch (error) {
      console.error(`HJ_PLAYER_NOTIFY_SEI_INFOS decode failed: ${error}`);
    }
  }

  private open() {
    this.state.hjPlayer.openPlayer(this.state.openPlayerInfo, (str: string) => {
      let strJson: HJPlayerNotifyInfo;
      try {
        strJson = JSON.parse(str) as HJPlayerNotifyInfo;
      } catch (error) {
        console.error(`player notify parse failed: ${error}`);
        return;
      }
      console.info("uio123", strJson.type, strJson.msgInfo);
      if (strJson.type == HJPlayerNotifyType.HJ_PLAYER_NOTIFY_EOF) {
        emitter.emit("closeGift")
      }
      else if (strJson.type == HJPlayerNotifyType.HJ_PLAYER_NOTIFY_CLOSEDONE) { // 可以释放播放器资源
        this.state.hjPlayer.exitPlayer()
        this.state.hjPlayer.destroyPlayer()
      }
      else if (strJson.type == HJRenderNotifyType.HJ_RENDER_NOTIFY_FACEU_COMPLETE)
      {
        console.log("faceu complete")
      }
    }, { uid: 2342, device: "Harmony", sn: "HJPlayer" }, (str: string) => {
      // console.info("uio123", str)
    }, (seiInfo: HJPlayerSeiInfo) => {
      this.handleSeiInfos(seiInfo)
    })
  }

  private close() {
    this.state.hjPlayer.setWindow({
      classStyle:HJRteGraphConfig.HJNodeClass_TargetUI_0,
      insName: HJRteGraphConfig.HJNodeClass_TargetUI_0,
      surfaceId: this.state.surfaceId,
      width: 0,
      height: 0,
      state: SetWindowState.TARGET_DESTROY
    })
    this.state.hjPlayer.closePlayer()
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

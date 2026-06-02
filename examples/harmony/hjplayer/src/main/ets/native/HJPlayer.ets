//import { HJNativeSourceData } from 'hjmediautils';
import { HJNativeSourceData } from '../HJFaceDetectMgr'
import nPlayer from 'libHJMediaPlayer.so';
import { MediaStateInfo, SetWindowInfo, HJPlayerContextInitConfig, HJPlayerDownloadInfo } from './HJCommTypes';
import { HJPlayerSeiInfo, OpenPlayerInfo, serializeOpenPlayerInfo } from '../HJPlayerTypes';
import { HJRteAdjacentNodeInfo, HJRteAdjacentNodeInfoWrapper } from './HJRteGraphSetupInfo';

const TAG = "wkshhh"

function priParseAdjacentNodes(json: string): HJRteAdjacentNodeInfo[] {
  if (!json || json.length === 0) {
    return [];
  }
  try {
    const parsedArray = JSON.parse(json) as HJRteAdjacentNodeInfo[];
    if (parsedArray && parsedArray.length !== undefined) {
      return parsedArray;
    }
    const parsedWrapper = JSON.parse(json) as HJRteAdjacentNodeInfoWrapper;
    if (parsedWrapper && parsedWrapper.nodes !== undefined) {
      return parsedWrapper.nodes ?? [];
    }
  } catch (error) {
    console.error(TAG, 'parseAdjacentNodes failed', JSON.stringify(error));
  }
  return [];
}

export class HJPlayer {
  static m_contextInitFlag: boolean = false;
  private m_playerHandler?: bigint
  private isOpen: boolean = false

  public static contextInit(config: HJPlayerContextInitConfig) {
    if (!HJPlayer.m_contextInitFlag) {
      const result: number = nPlayer.n_contextInit(JSON.stringify(config))
      if (result != 0) {
        throw new Error("contextInit failed");
      }
      HJPlayer.m_contextInitFlag = true;
    }
  }

  public static preloadUrl(url: string) {
    nPlayer.n_preloadUrl(url);
  }

  public static download(info: HJPlayerDownloadInfo, stateCall: (str: string) => void) : string {
    const result: string = nPlayer.n_download(JSON.stringify(info), stateCall)
    return result;
  }

  public static cancelDownload(rid: string) {
    nPlayer.n_cancelDownload(rid);
  }

  public createPlayer() {
    this.m_playerHandler = nPlayer.n_createPlayer()
  }

  public destroyPlayer() {
    if (this.m_playerHandler) {
      nPlayer.n_destroyPlayer(this.m_playerHandler)
      this.m_playerHandler = undefined
    }
  }

  public setWindow(setWindowInfo: SetWindowInfo) {
    if (this.m_playerHandler && this.isOpen) {
      nPlayer.n_setWindow(this.m_playerHandler, JSON.stringify(setWindowInfo));
    }
  }

  public openPlayer(openPlayerInfo: OpenPlayerInfo, stateCall: (str: string) => void, stateInfo: MediaStateInfo, statCall: (str: string) => void, seiCall?: (seiInfo: HJPlayerSeiInfo) => void) {
    if (this.m_playerHandler) {
      if (!this.isOpen) {
        const serialized_open_player_info: string = serializeOpenPlayerInfo(openPlayerInfo)
        const result: number = seiCall ?
          nPlayer.n_openPlayer(this.m_playerHandler, serialized_open_player_info, stateCall, JSON.stringify(stateInfo), statCall, seiCall) :
          nPlayer.n_openPlayer(this.m_playerHandler, serialized_open_player_info, stateCall, JSON.stringify(stateInfo), statCall)
        if (result != 0) {
          throw new Error("openPlayer failed");
        }
        this.isOpen = true
      }
    }
  }

  public closePlayer() {
    if (this.m_playerHandler) {
      if (this.isOpen) {
        nPlayer.n_closePlayer(this.m_playerHandler);
        this.isOpen = false
      }
    }
  }

  public exitPlayer() {
    if (this.m_playerHandler) {
      nPlayer.n_exitPlayer(this.m_playerHandler);
    }
  }

  public setMute(mute: boolean) {
    if (this.m_playerHandler && this.isOpen) {
      nPlayer.n_setMute(this.m_playerHandler, mute);
    }
  }

  public setPause(pause: boolean) {
    if (this.m_playerHandler && this.isOpen) {
      nPlayer.n_setPause(this.m_playerHandler, pause);
    }
  }

  public getDuration(): bigint {
    if (this.m_playerHandler && this.isOpen) {
      return nPlayer.n_getDuration(this.m_playerHandler);
    }
    return 0n;
  }

  public getCurrentTimestamp(): bigint {
    if (this.m_playerHandler && this.isOpen) {
      return nPlayer.n_getCurrentTimestamp(this.m_playerHandler);
    }
    return 0n;
  }

  public setEnableSEIUUids(i_uuid: string, bKeyMustCb: boolean): number {
    if (this.m_playerHandler && this.isOpen) {
      return nPlayer.n_setEnableSEIUUids(this.m_playerHandler, i_uuid, bKeyMustCb);
    }
    return -1;
  }

  public setFaceInfo(i_sourceInsName: string, width: number, height: number, faceinfo: string)
  {
    if (this.m_playerHandler) {
      nPlayer.n_setFaceInfo(this.m_playerHandler, i_sourceInsName, width, height, faceinfo);
    }
  }
  public nativeSourceOpen(isPBO: boolean) : number
  {
    if (this.m_playerHandler) {
      return nPlayer.n_nativeSourceOpen(this.m_playerHandler, isPBO);
    }
    return -1;
  }
  public nativeSourceClose()
  {
    if (this.m_playerHandler) {
      nPlayer.n_nativeSourceClose(this.m_playerHandler);
    }
  }
  public nativeSourceAcquire(): HJNativeSourceData | null
  {
    if (this.m_playerHandler) {
      return nPlayer.n_nativeSourceAcquire(this.m_playerHandler);
    }
    return null;
  }

  public openFaceu(url : string) : number
  {
    if (this.m_playerHandler) {
      return nPlayer.n_openFaceu(this.m_playerHandler, url);
    }
    return -1;
  }
  public closeFaceu()
  {
    if (this.m_playerHandler) {
      nPlayer.n_closeFaceu(this.m_playerHandler);
    }
  }

  public nodeEnable(classString:string, insName:string, bEnable:boolean, info:string):number
  {
    if (this.m_playerHandler) {
      return nPlayer.n_nodeEnable(this.m_playerHandler, classString, insName, bEnable, info);
    }
    return -1;
  }

  public nodeSetParam(classString:string, insName:string, paramInfo:string):number
  {
    if (this.m_playerHandler) {
      return nPlayer.n_nodeSetParam(this.m_playerHandler, classString, insName, paramInfo);
    }
    return -1;
  }

  public nodeCreate(nodeInfo:string):number {
    if (this.m_playerHandler) {
      return nPlayer.n_nodeCreate(this.m_playerHandler, nodeInfo);
    }
    return -1;
  }

  public nodeConnect(srcClass:string, srcIns:string, dstClass:string, dstIns:string, shaderType:string, linkInfo:string):number {
    if (this.m_playerHandler) {
      return nPlayer.n_nodeConnect(this.m_playerHandler, srcClass, srcIns, dstClass, dstIns, shaderType, linkInfo);
    }
    return -1;
  }

  public nodeDelete(classStyle:string, insName:string, relink:boolean):number {
    if (this.m_playerHandler) {
      return nPlayer.n_nodeDelete(this.m_playerHandler, classStyle, insName, relink);
    }
    return -1;
  }

  public nodeDisconnect(srcClass:string, srcIns:string, dstClass:string, dstIns:string, linkId:string):number {
    if (this.m_playerHandler) {
      return nPlayer.n_nodeDisconnect(this.m_playerHandler, srcClass, srcIns, dstClass, dstIns, linkId);
    }
    return -1;
  }

  public nodeLinkInfoChange(srcClass:string, srcIns:string, dstClass:string, dstIns:string, linkInfo:string):number {
    if (this.m_playerHandler) {
      return nPlayer.n_nodeLinkInfoChange(this.m_playerHandler, srcClass, srcIns, dstClass, dstIns, linkInfo);
    }
    return -1;
  }

  public nodeGetPre(curClass:string, curIns:string): HJRteAdjacentNodeInfo[] {
    if (this.m_playerHandler) {
      const result: string = nPlayer.n_nodeGetPre(this.m_playerHandler, curClass, curIns);
      return priParseAdjacentNodes(result);
    }
    return [];
  }

  public nodeGetNext(curClass:string, curIns:string): HJRteAdjacentNodeInfo[] {
    if (this.m_playerHandler) {
      const result: string = nPlayer.n_nodeGetNext(this.m_playerHandler, curClass, curIns);
      return priParseAdjacentNodes(result);
    }
    return [];
  }
}

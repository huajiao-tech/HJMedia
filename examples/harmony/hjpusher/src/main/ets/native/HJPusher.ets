import nPusher from 'libHJPusher.so';
import { MediaStateInfo, SetWindowInfo } from './HJCommTypes';
import {
  PreviewInfo, PusherConfig, RecorderInfo,
} from './HJPusherTypes';
import { HJRteAdjacentNodeInfo, HJRteAdjacentNodeInfoWrapper } from './HJRteGraphSetupInfo';

//import {HJNativeSourceData} from 'hjmediautils'
import { HJNativeSourceData } from '../HJFaceDetectMgr'

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

export class HJPusher {
  static m_contextInitFlag: boolean = false;
  private m_pusherHandler?: bigint
  private m_ImageNativeHandler?: bigint
  private isOpen: boolean = false



  public static contextInit(valid: boolean, logDir: string, logLevel: number, logMode: number, maxSize: number,
    maxFiles: number) {
    if (!HJPusher.m_contextInitFlag) {
      const result: number = nPusher.n_contextInit(JSON.stringify({
        valid,
        logDir,
        logLevel,
        logMode,
        maxSize,
        maxFiles
      }))
      if (result != 0) {
        throw new Error("contextInit failed");
      }
      HJPusher.m_contextInitFlag = true;
    }
  }

  public createPusher() {
    this.m_pusherHandler = nPusher.n_createPusher()
    console.info(TAG, "createPusher", this.m_pusherHandler)
  }

  public destroyPusher() {
    if (this.m_pusherHandler) {
      console.info(TAG, "destroyPusher", this.m_pusherHandler)
      nPusher.n_destroyPusher(this.m_pusherHandler);
      this.m_pusherHandler = undefined
    }
  }

  public openPreview(previewInfo: PreviewInfo, pusherStateCall: (str: string) => void): bigint {
    if (this.m_pusherHandler) {
      const previewInfoStr: string = JSON.stringify(previewInfo);
      const result: bigint = nPusher.n_openPreview(this.m_pusherHandler, previewInfoStr, pusherStateCall)
      console.info(TAG, "openPreview", this.m_pusherHandler, previewInfoStr, result)
      return result
    }
    console.warn(TAG, 'openPreview skipped: invalid handler')
    return 0n;
  }

  public closePreview() {
    if (this.m_pusherHandler) {
      console.info(TAG, "closePreview", this.m_pusherHandler)
      nPusher.n_closePreview(this.m_pusherHandler)
    }
  }

  public setWindow(setWindowInfo: SetWindowInfo): number {
    if (this.m_pusherHandler) {
      const result: number = nPusher.n_setWindow(this.m_pusherHandler, JSON.stringify(setWindowInfo));
      console.info(TAG, 'setWindow', this.m_pusherHandler, JSON.stringify(setWindowInfo), result);
      return result;
    }
    console.warn(TAG, 'setWindow skipped: invalid handler', JSON.stringify(setWindowInfo));
    return -1;
  }

  public openPusher(pusherConfig: PusherConfig, stateInfo: MediaStateInfo, stateCall: (str: string) => void) {
    if (this.m_pusherHandler) {
      if (!this.isOpen) {
        console.log(TAG, "openPusher", this.m_pusherHandler)
        const result: number = nPusher.n_openPusher(this.m_pusherHandler, JSON.stringify(pusherConfig), JSON.stringify(stateInfo), stateCall);
        if (result != 0) {
          throw new Error("openPusher failed");
        }
        this.isOpen = true
      }
    }
  }

  public closePusher() {
    if (this.m_pusherHandler) {
      if (this.isOpen) {
        console.log(TAG, "closePusher", this.m_pusherHandler)
        nPusher.n_closePusher(this.m_pusherHandler);
        this.isOpen = false
      }
    }
  }

  public setMute(mute: boolean) {
    if (this.m_pusherHandler) {
      nPusher.n_setMute(this.m_pusherHandler,mute);
    }
  }

  public setPreviewRotation(rotation: number) {
    if (this.m_pusherHandler) {
      console.info(TAG, "setPreviewRotation", this.m_pusherHandler, rotation)
      nPusher.n_setPreviewRotation(this.m_pusherHandler, rotation);
    }
  }

  public openRecorder(recorderInfo: RecorderInfo): number {
    if (this.m_pusherHandler) {
      return nPusher.n_openRecorder(this.m_pusherHandler, recorderInfo);
    } else {
      return -1;
    }
  }

  public closeRecorder() {
    if (this.m_pusherHandler) {
      nPusher.n_closeRecorder(this.m_pusherHandler);
    }
  }

  public openPngSeq(url: string) {
    if (this.m_pusherHandler) {
      nPusher.n_openPngSeq(this.m_pusherHandler, url);
    }
  }

  public setDoubleScreen(flag: boolean) {
    if (this.m_pusherHandler) {
      nPusher.n_setDoubleScreen(this.m_pusherHandler, flag);
    }
  }

  public setGiftPusher(flag: boolean) {
    if (this.m_pusherHandler) {
      nPusher.n_setGiftPusher(this.m_pusherHandler, flag);
    }
  }

  public openSpeechRecognizer(call: (buffer: ArrayBuffer) => void) {
    if (this.m_pusherHandler) {
      nPusher.n_openSpeechRecognizer(this.m_pusherHandler, call);
    }
  }

  public closeSpeechRecognizer() {
    if (this.m_pusherHandler) {
      nPusher.n_closeSpeechRecognizer(this.m_pusherHandler);
    }
  }

  public setFaceInfo(i_sourceInsName: string, width: number, height: number, faceinfo: string)
  {
    if (this.m_pusherHandler) {
      nPusher.n_setFaceInfo(this.m_pusherHandler, i_sourceInsName, width, height, faceinfo);
    }
  }
  public nativeSourceOpen(isPBO: boolean) : number
  {
      if (this.m_pusherHandler) {
        return nPusher.n_nativeSourceOpen(this.m_pusherHandler, isPBO);
      }
      return -1;
  }
  public nativeSourceClose()
  {
    if (this.m_pusherHandler) {
      nPusher.n_nativeSourceClose(this.m_pusherHandler);
    }
  }
  public nativeSourceAcquire(): HJNativeSourceData | null
  {
    if (this.m_pusherHandler) {
      return nPusher.n_nativeSourceAcquire(this.m_pusherHandler);
    }
    return null;
  }

  public openFaceu(url : string) : number
  {
    if (this.m_pusherHandler) {
      return nPusher.n_openFaceu(this.m_pusherHandler, url);
    }
    return -1;
  }
  public closeFaceu()
  {
    if (this.m_pusherHandler) {
      nPusher.n_closeFaceu(this.m_pusherHandler);
    }
  }
  public setVideoEncQuantOffset(i_quantoffset: number)
  {
    if (this.m_pusherHandler) {
      nPusher.n_setVideoEncQuantOffset(this.m_pusherHandler, i_quantoffset);
    }
  }
  public setFaceProtected(i_bFaceProtected: boolean)
  {
    if (this.m_pusherHandler) {
      nPusher.n_setFaceProtected(this.m_pusherHandler, i_bFaceProtected);
    }
  }

  public nodeEnable(classString:string, insName:string, bEnable:boolean, info:string):number
  {
    if (this.m_pusherHandler) {
      return nPusher.n_nodeEnable(this.m_pusherHandler, classString, insName, bEnable, info);
    }
    return -1;
  }

  public nodeCreate(nodeInfo:string):number {
    if (this.m_pusherHandler) {
      return nPusher.n_nodeCreate(this.m_pusherHandler, nodeInfo);
    }
    return -1;
  }

  public nodeConnect(srcClass:string, srcIns:string, dstClass:string, dstIns:string, shaderType:string, linkInfo:string):number {
    if (this.m_pusherHandler) {
      return nPusher.n_nodeConnect(this.m_pusherHandler, srcClass, srcIns, dstClass, dstIns, shaderType, linkInfo);
    }
    return -1;
  }
  public nodeSetParam(classString:string, insName:string, paramInfo:string):number
  {
    if (this.m_pusherHandler) {
      return nPusher.n_nodeSetParam(this.m_pusherHandler, classString, insName, paramInfo);
    }
    return -1;
  }
  public nodeDelete(classStyle:string, insName:string, relink:boolean):number {
    if (this.m_pusherHandler) {
      return nPusher.n_nodeDelete(this.m_pusherHandler, classStyle, insName, relink);
    }
    return -1;
  }

  public nodeDisconnect(srcClass:string, srcIns:string, dstClass:string, dstIns:string, linkId:string):number {
    if (this.m_pusherHandler) {
      return nPusher.n_nodeDisconnect(this.m_pusherHandler, srcClass, srcIns, dstClass, dstIns, linkId);
    }
    return -1;
  }

  public nodeLinkInfoChange(srcClass:string, srcIns:string, dstClass:string, dstIns:string, linkInfo:string):number {
    if (this.m_pusherHandler) {
      return nPusher.n_nodeLinkInfoChange(this.m_pusherHandler, srcClass, srcIns, dstClass, dstIns, linkInfo);
    }
    return -1;
  }

  public nodeGetPre(curClass:string, curIns:string): HJRteAdjacentNodeInfo[] {
    if (this.m_pusherHandler) {
      const result: string = nPusher.n_nodeGetPre(this.m_pusherHandler, curClass, curIns);
      return priParseAdjacentNodes(result);
    }
    return [];
  }

  public nodeGetNext(curClass:string, curIns:string): HJRteAdjacentNodeInfo[] {
    if (this.m_pusherHandler) {
      const result: string = nPusher.n_nodeGetNext(this.m_pusherHandler, curClass, curIns);
      return priParseAdjacentNodes(result);
    }
    return [];
  }

  // public openPixelMapOutput() {
  //   this.m_ImageNativeHandler = nPusher.n_openPixelMapOutput()
  // }
  //
  // public getImageReceiver(): bigint {
  //   if (this.m_ImageNativeHandler) {
  //     return nPusher.n_getImageReceiver(this.m_ImageNativeHandler)
  //   }
  //   return 0n;
  // }
  //
  // public closePixelMapOutput() {
  //   if (this.m_ImageNativeHandler) {
  //     nPusher.n_closePixelMapOutput(this.m_ImageNativeHandler)
  //     this.m_ImageNativeHandler = undefined
  //   }
  // }
}

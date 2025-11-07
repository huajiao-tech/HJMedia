import nPusher from 'libHJPusher.so';
import { MediaStateInfo, SetWindowInfo } from './HJCommTypes';
import {
  PreviewInfo, PusherConfig, RecorderInfo,
} from './HJPusherTypes';

//import {HJNativeSourceData} from 'hjmediautils'
import { HJNativeSourceData } from '../HJFaceDetectMgr'

const TAG = "wkshhh"

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
  }

  public destroyPusher() {
    if (this.m_pusherHandler) {
      nPusher.n_destroyPusher(this.m_pusherHandler);
      this.m_pusherHandler = undefined
    }
  }

  public openPreview(previewInfo: PreviewInfo, pusherStateCall: (str: string) => void): bigint {
    if (this.m_pusherHandler) {
      return nPusher.n_openPreview(this.m_pusherHandler, JSON.stringify(previewInfo), pusherStateCall)
    }
    return 0n;
  }

  public closePreview() {
    if (this.m_pusherHandler) {
      nPusher.n_closePreview(this.m_pusherHandler)
    }
  }

  public setWindow(setWindowInfo: SetWindowInfo) {
    if (this.m_pusherHandler) {
      nPusher.n_setWindow(this.m_pusherHandler, JSON.stringify(setWindowInfo));
    }
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

  public setFaceInfo(width: number, height: number, faceinfo: string)
  {
    if (this.m_pusherHandler) {
      nPusher.n_setFaceInfo(this.m_pusherHandler, width, height, faceinfo);
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
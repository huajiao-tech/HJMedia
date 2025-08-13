import nPusher from 'libHJPusher.so';
import { PreviewInfo, PusherConfig, PusherStateCall,
  RecorderInfo,
  SetWindowInfo, SetWindowState } from './HJPusherTypes';

const TAG = "wkshhh"

export class HJPusher {
  static m_contextInitFlag: boolean = false;
  private m_pusherHandler?: bigint
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

  public openPusher(pusherConfig: PusherConfig) {
    if (this.m_pusherHandler) {
      if (!this.isOpen) {
        console.log(TAG, "openPusher", this.m_pusherHandler)
        const result: number = nPusher.n_openPusher(this.m_pusherHandler, JSON.stringify(pusherConfig));
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
}
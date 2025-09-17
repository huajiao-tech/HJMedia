import nPlayer from 'libHJPlayer.so';
import { MediaStateInfo, SetWindowInfo } from '../HJCommTypes';
import { OpenPlayerInfo } from '../HJPlayerTypes';

const TAG = "wkshhh"

export class HJPlayer {
  static m_contextInitFlag: boolean = false;
  private m_playerHandler?: bigint
  private isOpen: boolean = false

  public static contextInit(valid: boolean, logDir: string, logLevel: number, logMode: number, maxSize: number,
    maxFiles: number) {
    if (!HJPlayer.m_contextInitFlag) {
      const result: number = nPlayer.n_contextInit(JSON.stringify({
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
      HJPlayer.m_contextInitFlag = true;
    }
  }

  public static preloadUrl(url: string) {
    nPlayer.n_preloadUrl(url);
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
    if (this.m_playerHandler) {
      nPlayer.n_setWindow(this.m_playerHandler, JSON.stringify(setWindowInfo));
    }
  }

  public openPlayer(openPlayerInfo: OpenPlayerInfo, stateCall: (str: string) => void, stateInfo: MediaStateInfo, statCall: (str: string) => void) {
    if (this.m_playerHandler) {
      if (!this.isOpen) {
        const result: number = nPlayer.n_openPlayer(this.m_playerHandler, JSON.stringify(openPlayerInfo), stateCall, JSON.stringify(stateInfo), statCall)
        if (result != 0) {
          throw new Error("openPusher failed");
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
    if (this.m_playerHandler) {
      nPlayer.n_setMute(this.m_playerHandler, mute);
    }
  }
}
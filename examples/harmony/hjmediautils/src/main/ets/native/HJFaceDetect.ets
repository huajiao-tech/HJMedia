import { faceDetector } from '@kit.CoreVisionKit';
import { image } from '@kit.ImageKit';

export enum HJFaceDetectState {
  idle,
  ready,
  done,
}

type HJCallbackDetect = (data: faceDetector.Face[], width:number, height:number) => void;

export class HJFaceDetect
{
    private TAG:string = "HJFaceDetect";
    private m_state:number = HJFaceDetectState.idle;
    private m_statIdx:number = 0;
    private m_statTime:number = 0;
    private m_detectCb:HJCallbackDetect | null = null;
    private m_bRunning:boolean = false;

    constructor()
    {
    }

    getState():number
    {
        return this.m_state;
    }

    registerDetectCb(cb:HJCallbackDetect)
    {
      this.m_detectCb = cb;
    }

    private priGetStateInfo():string
    {
      let value:string = "";
      switch (this.m_state)
      {
        case HJFaceDetectState.idle:
          value = "idle";
          break;
        case HJFaceDetectState.ready:
          value = "ready";
          break;
        case HJFaceDetectState.done:
          value = "done";
          break;
        default:
          break;
      }
      return value;
    }

    async open() : Promise<boolean>
    {
        if (this.m_state == HJFaceDetectState.idle)
        {
            const initResult = await faceDetector.init();
            if (initResult)
            {
              this.m_state = HJFaceDetectState.ready;
              return Promise.resolve(true);
            }
            else
            {
              return Promise.reject(false);
            }
            console.log(`${this.TAG} open state is：${this.priGetStateInfo()}`);
        }
        return Promise.resolve(false);
    }

    async detect(chooseImage: image.PixelMap)
    {
      this.m_bRunning = true;
      do
      {
          if (this.m_state == HJFaceDetectState.done)
          {
            break;
          }
          //console.log(`${this.TAG} detect enter`);
          if ((this.m_state == HJFaceDetectState.ready))
          {
              let visionInfo: faceDetector.VisionInfo =
              {
                  pixelMap: chooseImage
              };

              const t0: number = Date.now();
              let data: faceDetector.Face[] = await faceDetector.detect(visionInfo)
              if (data.length > 0)
              {
                  const t1: number = Date.now();
                  this.m_statTime += (t1-t0);
                  this.m_statIdx++;

                  if ((this.m_statIdx - 1) % 60 === 0)
                  {
                      console.log(`${this.TAG} statidx:${this.m_statIdx} avg:${this.m_statTime/this.m_statIdx} cur:${t1-t0} fps:${1000*this.m_statIdx/this.m_statTime}`);
                  }
              }
              if (this.m_detectCb)
              {
                  let info: image.ImageInfo = visionInfo.pixelMap.getImageInfoSync();
                  let width: number = info.size.width;
                  let height: number = info.size.height;
                  this.m_detectCb(data, width, height);
              }
          }
          else
          {
              console.warn(`${this.TAG} detect warning state is：${this.priGetStateInfo()}`);
          }
      } while (false);
      this.m_bRunning = false;
      //console.log(`${this.TAG} detect end====================`);
    }

    async priCheckUntilDone() : Promise<void>
    {
        return new Promise((resolve) =>
        {
            const checkInterval = 10;
            const timeProc = () =>
            {
              if (!this.m_bRunning)
              {
                  resolve();
              }
              else
              {
                  setTimeout(timeProc, checkInterval);
              }
            }
            timeProc();
        });
    }

    async done(): Promise<boolean>
    {
      console.log(`${this.TAG} done enter state is: ${this.priGetStateInfo()}`);

      if (this.m_state === HJFaceDetectState.idle)
      {
          return Promise.reject(false);
      }

      if (!this.m_bRunning)
      {
          console.log(`${this.TAG} done is not running, direct await priDone, ${this.priGetStateInfo()}`);
          await this.priDone();
          return Promise.resolve(true);
      }
      else
      {
          if (this.m_state === HJFaceDetectState.ready)
          {
              this.m_state = HJFaceDetectState.done;
              console.log(`${this.TAG} done is running, so postpone done, ${this.priGetStateInfo()}`);
              await this.priCheckUntilDone();
              await this.priDone();
              return Promise.resolve(true);
          }
          else
          {
              this.m_state = HJFaceDetectState.idle;
              return Promise.reject(false);
          }
      }
    }
    private async priDone()
    {
        await faceDetector.release();
        this.m_state = HJFaceDetectState.idle;
        console.log(`${this.TAG} done really end success------, state is: ${this.priGetStateInfo()}`);
    }
}
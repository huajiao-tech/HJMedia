import { faceDetector } from '@kit.CoreVisionKit';
import { image } from '@kit.ImageKit';
import {HJFaceDetect, HJFaceDetectState} from './HJFaceDetect'
import { BusinessError } from '@kit.BasicServicesKit';

export interface HJNativeSourceData {
  data: ArrayBuffer;
  width: number;
  height: number;
}

export enum HJFaceDetectMgrRet {
  ok                          = 0,

  open_kernal_exception_error = -1,
  open_state_error            = -2,
  open_native_source_error    = -3,

  close_kernal_exception_error = -4,
  close_state_error            = -5,
}


type HJFaceInfoCb = (width:number, height:number, faceInfo:string) => void;
type HJNativeSourceOpenCb = () => number;
type HJNativeSourceAcquireCb = () => HJNativeSourceData | null;
type HJNativeSourceCloseCb = () => void;

export class HJFaceDetectMgr
{
    private TAG:string = "HJFaceDetectMgr";

    private m_bDetectReady:boolean = false;
    private m_detector : HJFaceDetect = new HJFaceDetect();

    private m_faceInfoCb : HJFaceInfoCb | null = null;
    private m_nativeSourceOpenCb : HJNativeSourceOpenCb | null = null;
    private m_nativeSourceAcquireCb : HJNativeSourceAcquireCb | null = null;
    private m_nativeSourceCloseCb : HJNativeSourceCloseCb | null = null;

    constructor()
    {
    }
    registerFaceInfoCb(cb:HJFaceInfoCb)
    {
        this.m_faceInfoCb = cb;
    }
    registerNativeSourceOpenCb(cb:HJNativeSourceOpenCb)
    {
        this.m_nativeSourceOpenCb = cb;
    }
    registerNativeSourceAcquireCb(cb:HJNativeSourceAcquireCb)
    {
        this.m_nativeSourceAcquireCb = cb;
    }
    registerNativeSourceCloseCb(cb:HJNativeSourceCloseCb)
    {
        this.m_nativeSourceCloseCb = cb;
    }

    priPostPoneDetect(delay:number)
    {
        if (this.m_bDetectReady)
        {
          //setTimeout(this.priDetect, delay); //not use this, error
          setTimeout(() =>
          {
              this.priDetect()
          }, delay);
        }
    }

    priDetectStraight()
    {
        this.priPostPoneDetect(0);
    }
    priDetectPostpone()
    {
        this.priPostPoneDetect(10);
    }

    async openFaceDetect() : Promise<number>
    {
        try
        {
          const bOpen:boolean = await this.m_detector.open();
          if (bOpen)
          {
              this.m_bDetectReady = true;
              console.log("HJFaceDetectUI open success");
          }
          else
          {
              const faceState:number = this.m_detector.getState();
              console.log("HJFaceDetectUI open false, state is: " + faceState);
              return Promise.reject(HJFaceDetectMgrRet.open_state_error);
          }
        }
        catch (error)
        {
            console.log("HJFaceDetectUI open catch error" + error);
            return Promise.reject(HJFaceDetectMgrRet.open_kernal_exception_error);
        }

        this.m_detector.registerDetectCb((data: faceDetector.Face[], width:number, height:number)=>
        {
           let faceString = "";
           if (data.length > 0)
           {
              faceString = JSON.stringify(data);
              console.log(`facedetect: ${faceString}`);
            }
            if (this.m_faceInfoCb)
            {
               this.m_faceInfoCb(width, height, faceString);
            }
            this.priDetectStraight();
        });

        this.priDetectStraight();
        if (this.m_nativeSourceOpenCb)
        {
            let ret : number = this.m_nativeSourceOpenCb();
            if (ret < 0)
            {
                return Promise.reject(HJFaceDetectMgrRet.open_native_source_error);
            }
        }
        //HJPusher.tryOpenImgReceiver();
        return Promise.resolve(HJFaceDetectMgrRet.ok);
    }

    priDetect()
    {
      if (this.m_nativeSourceAcquireCb)
      {
          let mediaDataObj : HJNativeSourceData | null = this.m_nativeSourceAcquireCb();
          if (mediaDataObj)
          {
            const rgbaBuffer : ArrayBuffer = mediaDataObj.data;
            const width:number = mediaDataObj.width;
            const height:number = mediaDataObj.height;
            let initializationOptions: image.InitializationOptions = {
              size: {
                height: height,
                width: width
              },
              pixelFormat: 3, //RGBA_8888=3
              alphaType: 0,
              editable: false
            };

            try
            {
              //console.log("HJFaceDetect tryGetMediaData detect");
              let pixelMap: image.PixelMap = image.createPixelMapSync(rgbaBuffer, initializationOptions);
              this.m_detector.detect(pixelMap);
            }
            catch (error)
            {
              console.error('HJFaceDetect catch error ' + (error as BusinessError).message);
            }
          }
          else
          {
              //console.log("HJFaceDetect tryGetMediaData empty, next try again");
              this.priDetectPostpone();
          }
        }
    }
    async closeFaceDetect() : Promise<number>
    {
        this.m_bDetectReady = false;

        if (this.m_nativeSourceCloseCb)
        {
            this.m_nativeSourceCloseCb();
        }

        try
        {
            const bDone:boolean = await this.m_detector.done();
            if (bDone)
            {
                console.log("HJFaceDetectUI done success");
            }
            else
            {
                const faceState:number = this.m_detector.getState();
                console.log("HJFaceDetectUI done false, state is: " + faceState);
                return Promise.reject(HJFaceDetectMgrRet.close_state_error);
            }
        }
        catch (error)
        {
            console.log("HJFaceDetectUI done catch error" + error);
            return Promise.reject(HJFaceDetectMgrRet.close_kernal_exception_error);
        }

        //HJPusher.tryCloseImageReceiver();
        return Promise.resolve(HJFaceDetectMgrRet.ok);
    }
}
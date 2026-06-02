import { faceDetector } from '@kit.CoreVisionKit';
import { image } from '@kit.ImageKit';
import {HJFaceDetect, HJFaceDetectState} from './HJFaceDetect'
import { BusinessError } from '@kit.BasicServicesKit';

export interface HJNativeSourceData {
  data: ArrayBuffer;
  width: number;
  height: number;
  scaleWidth: number;
  scaleHeight: number;
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

    private priRestoreFacePointToSource(point: Object, scaleX: number, scaleY: number): void
    {
        let pointObj = point as Record<string, number>;
        if (typeof pointObj.x === 'number')
        {
            pointObj.x = Math.round(pointObj.x * scaleX);
        }
        if (typeof pointObj.y === 'number')
        {
            pointObj.y = Math.round(pointObj.y * scaleY);
        }
    }

    private priRestoreFaceItemToSource(item: Object, scaleX: number, scaleY: number): void
    {
        let itemObj = item as Record<string, Object>;
        let rect = itemObj.rect;
        if (rect)
        {
            let rectObj = rect as Record<string, number>;
            if (typeof rectObj.left === 'number')
            {
                rectObj.left = Math.round(rectObj.left * scaleX);
            }
            if (typeof rectObj.top === 'number')
            {
                rectObj.top = Math.round(rectObj.top * scaleY);
            }
            if (typeof rectObj.width === 'number')
            {
                rectObj.width = Math.round(rectObj.width * scaleX);
            }
            if (typeof rectObj.height === 'number')
            {
                rectObj.height = Math.round(rectObj.height * scaleY);
            }
        }

        let points = itemObj.points;
        if (Array.isArray(points))
        {
            for (let point of points as Array<Object>)
            {
                this.priRestoreFacePointToSource(point, scaleX, scaleY);
            }
        }
    }

    private priRestoreFaceInfoToSource(faceData: Array<Object>, detectWidth: number, detectHeight: number, sourceWidth: number, sourceHeight: number): void
    {
        if (faceData.length <= 0 || detectWidth <= 0 || detectHeight <= 0 || sourceWidth <= 0 || sourceHeight <= 0)
        {
            return;
        }
        if (detectWidth === sourceWidth && detectHeight === sourceHeight)
        {
            return;
        }

        try
        {
            const scaleX: number = sourceWidth / detectWidth;
            const scaleY: number = sourceHeight / detectHeight;
            for (let item of faceData)
            {
                this.priRestoreFaceItemToSource(item, scaleX, scaleY);
            }
        }
        catch (error)
        {
            console.error(`${this.TAG} restore face info failed ${JSON.stringify(error)}`);
        }
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

        this.m_detector.registerDetectCb((data: faceDetector.Face[], detectWidth:number, detectHeight:number, sourceWidth:number, sourceHeight:number)=>
        {
           let faceString = "";
           if (data.length > 0)
           {
              if (detectWidth !== sourceWidth || detectHeight !== sourceHeight)
              {
                  this.priRestoreFaceInfoToSource(data as Array<Object>, detectWidth, detectHeight, sourceWidth, sourceHeight);
              }
              faceString = JSON.stringify(data);
              console.log(`facedetect: ${faceString}`);
            }
            if (this.m_faceInfoCb)
            {
               this.m_faceInfoCb(sourceWidth, sourceHeight, faceString);
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
            const scaleWidth:number = mediaDataObj.scaleWidth > 0 ? mediaDataObj.scaleWidth : width;
            const scaleHeight:number = mediaDataObj.scaleHeight > 0 ? mediaDataObj.scaleHeight : height;
            let initializationOptions: image.InitializationOptions = {
              size: {
                height: scaleHeight,
                width: scaleWidth
              },
              pixelFormat: 3, //RGBA_8888=3
              alphaType: 0,
              editable: false
            };

            try
            {
              //console.log("HJFaceDetect tryGetMediaData detect");
              let pixelMap: image.PixelMap = image.createPixelMapSync(rgbaBuffer, initializationOptions);
              this.m_detector.detect(pixelMap, width, height);
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

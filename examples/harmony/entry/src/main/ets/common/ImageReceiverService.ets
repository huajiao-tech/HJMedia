import { image } from '@kit.ImageKit';
import { BusinessError } from '@kit.BasicServicesKit';
import { fileIo as fs } from '@kit.CoreFileKit';
import { FaceDetectorService } from './FaceDetectorService';

const TAG = "ImageReceiverService"

export class ImageReceiverService {
  private faceDetectorService: FaceDetectorService = new FaceDetectorService()
  private receiver: image.ImageReceiver = image.createImageReceiver({
    width: 720,
    height: 1280
  }, image.ImageFormat.JPEG, 8) // 这里的设置无效，实际由生产者决定 实际capacity由设备硬件决定。
  private pixelMap?: image.PixelMap
  private context: Context
  private currentIndex: number = 0
  private surfaceId?: string

  public constructor(context: Context) {
    this.context = context
    this.receiver.on('imageArrival', this.imageArrival)
  }

  public release() {
    // this.receiver.off('imageArrival', this.imageArrival)
    this.faceDetectorService.release()
    this.receiver.release()
  }

  public async getReceivingSurfaceId(): Promise<string> {
    if (this.surfaceId) {
      return this.surfaceId
    }
    this.surfaceId = await this.receiver.getReceivingSurfaceId()
    return this.surfaceId
  }

  private imageArrival = async () => {
    const nextImage = await this.receiver.readNextImage()
    // if (this.currentIndex++ > 30) {
    //   nextImage.release()
    //   return
    // }
    const imgComponent = await nextImage.getComponent(image.ComponentType.JPEG); // 实际返回格式由生产者决定
    if (imgComponent.byteBuffer) {
      const width = nextImage.size.width
      const height = nextImage.size.height
      const stride = imgComponent.rowStride
      console.log(TAG, `width:${width} height:${height} stride:${stride} pixelStride:${imgComponent.pixelStride} bufferSize: ${imgComponent.byteBuffer.byteLength}`)
      if (stride == width) {
        this.pixelMap = await image.createPixelMap(imgComponent.byteBuffer, {
          size: { height: height, width: width },
          srcPixelFormat: image.PixelMapFormat.RGBA_8888,
        })
      } else {
        const dstBufferSize = width * height * 4
        const dstArr = new Uint8Array(dstBufferSize)
        for (let j = 0; j < height; j++) {
          const srcBuf = new Uint8Array(imgComponent.byteBuffer, j * stride, width * 4)
          dstArr.set(srcBuf, j * width * 4)
        }
        this.pixelMap = await image.createPixelMap(dstArr.buffer, {
          size: { height: height, width: width },
          srcPixelFormat: image.PixelMapFormat.RGBA_8888,
        })
        // // 创建pixelMap，width宽传行距stride的值。
        // this.pixelMap = await image.createPixelMap(imgComponent.byteBuffer, {
        //   size: { height: height, width: stride }, srcPixelFormat: image.PixelMapFormat.RGBA_8888
        // })
        // // 裁剪多余的像素。
        // this.pixelMap.cropSync({ size: { width: width, height: height }, x: 0, y: 0 });
      }
    }
    // let imagePackerApi = image.createImagePacker();
    // let packOpts: image.PackingOption = { format: "image/jpeg", quality: 90 };
    // const filePath: string = this.context?.cacheDir + `/${this.currentIndex}image.jpg`
    // let file = fs.openSync(filePath, fs.OpenMode.CREATE | fs.OpenMode.READ_WRITE);
    // imagePackerApi.packToFile(this.pixelMap, file.fd, packOpts).then(() => {
    //   console.error('pack success: ' + filePath);
    // }).catch((error: BusinessError) => {
    //   console.error('Failed to pack the image. And the error is: ' + error);
    // })
    this.faceDetectorService.detect(this.pixelMap!)
    nextImage.release();
  }
}
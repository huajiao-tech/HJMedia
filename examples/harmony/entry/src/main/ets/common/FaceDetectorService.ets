import { faceDetector } from "@kit.CoreVisionKit";

const TAG = "FaceDetectorService"

export class FaceDetectorService {
  private promise?: Promise<Array<faceDetector.Face>>
  constructor() {
    faceDetector.init()
  }

  public detect(pixelMap: PixelMap) {
    if (!this.promise) {
      this.doDetect(pixelMap)
    }
  }

  public async doDetect(pixelMap: PixelMap) {
    let visionInfo: faceDetector.VisionInfo = {
      pixelMap: pixelMap
    }
    this.promise = faceDetector.detect(visionInfo)
    try {
      let data:faceDetector.Face[] = await this.promise
      if (data.length === 0) {
        console.log(TAG, "No face was detected in the image.")
      } else {
        let faceString = JSON.stringify(data)
        console.log(TAG, "faceString data is " + faceString)
      }
    } finally {
      this.promise = undefined
    }
  }

  release() {
    faceDetector.release()
  }
}
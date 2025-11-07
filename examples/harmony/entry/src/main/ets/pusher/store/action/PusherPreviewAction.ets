import { Action } from "../../../interface/Store"
import { PusherPreviewState } from "../PusherPreviewStore"

export class MicrophoneAction implements Action<PusherPreviewState> {
  payload: number

  constructor(payload: number) {
    this.payload = payload
  }
}

export class RecorderActionPayLoad {
  index: number
  recordUrl: string

  constructor(index: number, recordUrl: string) {
    this.index = index
    this.recordUrl = recordUrl
  }
}

export class RecorderAction implements Action<PusherPreviewState> {
  payload: RecorderActionPayLoad

  constructor(payload: RecorderActionPayLoad) {
    this.payload = payload
  }
}

export class PusherAction implements Action<PusherPreviewState> {
}

export class CameraAction implements Action<PusherPreviewState> {
  payload: number

  constructor(payload: number) {
    this.payload = payload
  }
}

export class GiftAction implements Action<PusherPreviewState> {
  payload: string

  constructor(payload: string) {
    this.payload = payload
  }
}

export class TimerAction implements Action<PusherPreviewState> {
  payload: number

  constructor(payload: number) {
    this.payload = payload
  }
}

export class SelectorAction implements Action<PusherPreviewState> {
  payload: number

  constructor(payload: number) {
    this.payload = payload
  }
}

export class DoubleScreenAction implements Action<PusherPreviewState> {
}

export class FaceuAction implements Action<PusherPreviewState> {
  payload: number
  url: string
  constructor(payload: number, url: string) {
    this.payload = payload
    this.url = url
  }
}

export class FaceDetectAction implements Action<PusherPreviewState> {
  payload: number

  constructor(payload: number) {
    this.payload = payload
  }
}
export class SpeechRecognizerAction implements Action<PusherPreviewState> {
  payload: number

  constructor(payload: number) {
    this.payload = payload
  }
}

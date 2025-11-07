@ObservedV2
export class MediaConfig {
  type: "string" | "boolean" | "slider"
  title: string
  @Trace value: string | boolean | number
  min?: number
  max?: number

  constructor(type: "string" | "boolean" | "slider", title: string, value: string | boolean | number, min?: number, max?: number) {
    this.type = type
    this.title = title
    this.value = value
    this.min = min
    this.max = max
  }
}
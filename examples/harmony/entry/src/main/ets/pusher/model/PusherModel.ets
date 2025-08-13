@ObservedV2
export class PusherConfig {
  type: "string" | "boolean"
  title: string
  @Trace value: string | boolean

  constructor(type: "string" | "boolean", title: string, value: string | boolean) {
    this.type = type
    this.title = title
    this.value = value
  }
}
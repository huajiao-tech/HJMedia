import { State, Store, Action } from '../../interface/Store';
import { WindowUtils } from '../../util/WindowUtils';

@ObservedV2
export class ShortVideoState implements State {
  @Trace textList: Array<number> = [0, 1]

  @Monitor("textList.length")
  textListChange() {
    console.log("wkshhh length", this.textList.length)
  }
}

export class ShortVideoStore implements Store<ShortVideoState> {
  state = new ShortVideoState()
  getState = () => this.state

  onPageShow(): void {
    WindowUtils.setWindowLayoutFullScreen(true)
  }

  onPageHide(): void {
    WindowUtils.setWindowLayoutFullScreen(false)
  }

  dispatch(action: Action<ShortVideoState>): void {
    if (action instanceof AddAction) {
      console.log("wkshhhh content", this.state.textList.join(","))
      this.state.textList.push(action.payload)
    }
  }
}

export class AddAction implements Action<ShortVideoState> {
  payload: number

  constructor(payload: number) {
    this.payload = payload
  }
}
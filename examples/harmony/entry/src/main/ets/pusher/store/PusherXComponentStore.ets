import { State, Store, Action } from '../../interface/Store';

@ObservedV2
export class PusherXComponentState implements State {
  @Trace x: number = 0;
  @Trace y: number = 0;
  @Trace w: number | string = "100%";
  @Trace h: number | string = "100%";

  public surfaceId: bigint = 0n;
}

export class PusherXComponentStore implements Store<PusherXComponentState> {
  state = new PusherXComponentState()
  getState = () => this.state

  dispatch(action: Action<PusherXComponentState>): void {
    // if (action instanceof AddAction) {
    //
    // }
  }
}

// class AddAction implements Action<PusherState> {
// }
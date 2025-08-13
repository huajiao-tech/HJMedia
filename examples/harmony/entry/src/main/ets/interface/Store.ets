export interface State {
}

export interface Action<T extends State> {
  payload?: ESObject;
}

export interface Store<T extends State> {
  state: T;
  dispatch: (action: Action<T>) => void;
  getState: () => T;

  aboutToAppear?: () => void;

  aboutToDisappear?: () => void;
}

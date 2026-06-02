// export interface HJRteGraphConfigFaceuInfoPlain {
//   faceuUrl: string;
//   bDebugPoints: boolean;
//   bUseFakePoints: boolean;
// }
//
// export class HJRteGraphConfigFaceuInfo {
//   faceuUrl: string = '';
//   bDebugPoints: boolean = false;
//   bUseFakePoints: boolean = false;
//
//   constructor(init?: HJRteGraphConfigFaceuInfoPlain) {
//     if (init !== undefined) {
//       this.faceuUrl = init.faceuUrl;
//       this.bDebugPoints = init.bDebugPoints;
//       this.bUseFakePoints = init.bUseFakePoints;
//     }
//   }
//
//   toPlain(): HJRteGraphConfigFaceuInfoPlain {
//     return {
//       faceuUrl: this.faceuUrl,
//       bDebugPoints: this.bDebugPoints,
//       bUseFakePoints: this.bUseFakePoints
//     };
//   }
//
//   initSerial(): string {
//     return JSON.stringify(this.toPlain());
//   }
//
//   static fromJson(json: string): HJRteGraphConfigFaceuInfo {
//     const parsed: HJRteGraphConfigFaceuInfoPlain = JSON.parse(json) as HJRteGraphConfigFaceuInfoPlain;
//     return new HJRteGraphConfigFaceuInfo(parsed);
//   }
// }

/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License")
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

import { hilog } from '@kit.PerformanceAnalysisKit';

class Logger {
  private domain: number;
  private format: string = '%{public}s';

  constructor() {
    this.domain = 0xFF00;
  }

  debug(...arg: string[]): void {
    hilog.debug(this.domain, arg[0], this.format, arg[1]);
  }

  info(...arg: string[]): void {
    hilog.info(this.domain, arg[0], this.format, arg[1]);
  }

  warn(...arg: string[]): void {
    hilog.warn(this.domain, arg[0], this.format, arg[1]);
  }

  error(...arg: string[]): void {
    hilog.error(this.domain, arg[0], this.format, arg[1]);
  }
}

export default new Logger();
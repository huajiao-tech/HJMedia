import { appTasks } from '@ohos/hvigor-ohos-plugin';
import * as fs from 'fs';
import * as path from 'path';

const copyDir = (src: string, dst: string) => {
  if (!fs.existsSync(src)) {
    return;
  }
  fs.mkdirSync(dst, { recursive: true });
  fs.readdirSync(src, { withFileTypes: true }).forEach((entry) => {
    const from = path.join(src, entry.name);
    const to = path.join(dst, entry.name);
    if (entry.isDirectory()) {
      copyDir(from, to);
    } else {
      fs.copyFileSync(from, to);
    }
  });
};

// Ensure shared ETS/native sources from hjmediautils are available in the HAR modules.
(() => {
  const projectRoot = __dirname;
  const utilsEts = path.resolve(projectRoot, 'hjmediautils', 'src', 'main', 'ets');
  const harTargets = [
    path.resolve(projectRoot, 'hjplayer', 'src', 'main', 'ets'),
    path.resolve(projectRoot, 'hjpusher', 'src', 'main', 'ets'),
  ];

  harTargets.forEach((dstEts) => {
    copyDir(utilsEts, dstEts);
  });
})();

export { appTasks };

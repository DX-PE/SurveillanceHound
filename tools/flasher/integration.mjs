// SPDX-License-Identifier: Apache-2.0
import {createHash} from 'node:crypto';
import {readFile} from 'node:fs/promises';
import {fileURLToPath} from 'node:url';

// Deliberately small modifications to pinned Apache-2.0 ESP Web Tools 10.4.0.
// Fail the build if upstream changes; review the success/error paths again.
export const verificationPlugin = {
  name: 'hound-verified-installation',
  setup(build) {
    build.onLoad({filter: /esp-web-tools\/dist\/flash\.js$/}, async ({path}) => {
      let source = await readFile(path, 'utf8');
      const digest = createHash('sha256').update(source).digest('hex');
      if (digest !== '0c91f8a0c19efe6e9b7c42108cc2024234896458075a69444c4eaf4097427ce1') {
        throw new Error('ESP Web Tools flash integration changed; review verification before updating its pin.');
      }
      source = source.replace(
        'const data = new Uint8Array(buffer, 0, buffer.byteLength);',
        'const data = new Uint8Array(buffer, 0, buffer.byteLength);\n            await verifyFirmware(data, build.parts[part]);'
      ).replace('await esploader.writeFlash({', 'await writeAndVerify(esploader, {');
      const helper = JSON.stringify(fileURLToPath(new URL('./verify.mjs', import.meta.url)));
      return {contents: `import {verifyFirmware, writeAndVerify} from ${helper};\n${source}`, loader: 'js'};
    });
  }
};

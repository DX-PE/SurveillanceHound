// SPDX-License-Identifier: Apache-2.0
import SparkMD5 from 'spark-md5';

// Check the actual download before ESP Web Tools can erase the board.
export async function verifyFirmware(data, part) {
  if (!Number.isSafeInteger(part.size) || part.size <= 0 ||
      !/^[0-9a-f]{64}$/.test(part.sha256) || data.byteLength !== part.size) {
    throw new Error('Firmware size or verification metadata is invalid. Reload the installer.');
  }
  const digest = await crypto.subtle.digest('SHA-256', data);
  const actual = Array.from(new Uint8Array(digest), byte => byte.toString(16).padStart(2, '0')).join('');
  if (actual !== part.sha256) {
    throw new Error('Firmware download verification failed. Reload the installer and try again.');
  }
}

// MD5 is the ESP bootloader's transfer-integrity check; release authenticity uses
// HTTPS and the pinned SHA-256 above. Never signal success from write progress.
export async function writeAndVerify(loader, options) {
  if (options.flashMode !== 'keep' || options.flashFreq !== 'keep' || options.flashSize !== 'keep') {
    throw new Error('Verified factory installation must preserve the release image headers.');
  }
  const expected = options.fileArray.map(({data, address}) => ({
    address,
    size: data.byteLength,
    md5: new SparkMD5.ArrayBuffer().append(data).end()
  }));
  await loader.writeFlash(options);
  for (const part of expected) {
    const actual = await loader.flashMd5sum(part.address, part.size);
    if (typeof actual !== 'string' || actual.toLowerCase() !== part.md5) {
      throw new Error('Firmware verification failed: the board did not receive the complete image. Keep it connected and retry installation.');
    }
  }
}

import { verify } from 'node:crypto';
import { ClientListener, CombinedController, Sp } from './clientListener';
import { SettingsService } from './settingsService';

export class ServerJsVerificationService extends ClientListener {
  constructor(private sp: Sp, private controller: CombinedController) {
    super();
  }

  public verifyServerJs(src: string): string {
    const pubkeys = this.controller.lookupListener(SettingsService).getCachedTargetPeer()?.publicKeys;
    if (!pubkeys) {
      return src;
    }
    const lastLineStart = src.lastIndexOf('\n') + 1;
    const sigPrefix = '// skymp:sig:y:';
    if (lastLineStart === 0 || !src.substring(lastLineStart).startsWith(sigPrefix)) {
      throw new Error('sig not found');
    }
    const [keyId, sig] = src.substring(lastLineStart + sigPrefix.length).split(':');
    if (!this.isAlphaNumeric(keyId)) {
      throw new Error('malformed key id');
    }
    const key = pubkeys[keyId];
    if (!key) {
      throw new Error('unknown key');
    }
    const toVerify = this.toArrayBufferView(src.substring(0, lastLineStart - 1), 'utf8');
    if (!verify(null, toVerify, key, this.toArrayBufferView(sig, 'base64'))) {
      throw new Error('bad signature');
    }
    return src;
  }

  private isAlphaNumeric(str: string) {
    for (let i = 0; i < str.length; ++i) {
      let code = str.charCodeAt(i);
      if (!(
        (97 <= code && code <= 122) ||
        (65 <= code && code <= 90) ||
        (48 <= code && code <= 57)
      )) {
        return false;
      }
    }
    return true;
  }

  private toArrayBufferView(str: string, enc: NodeJS.BufferEncoding): NodeJS.ArrayBufferView {
    const buf = Buffer.from(str, enc);
    return new Uint8Array(buf.buffer, buf.byteOffset, buf.byteLength);
  }
}

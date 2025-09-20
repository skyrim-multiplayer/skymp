import { verify } from 'node:crypto';
import { ClientListener, CombinedController, Sp } from './clientListener';
import { SettingsService } from './settingsService';
import { logTrace } from '../../logging';

export class ServerJsVerificationService extends ClientListener {
  constructor(private sp: Sp, private controller: CombinedController) {
    super();
  }

  public verifyServerJs(src: string): { src: string, error: null } | { src: null, error: string } {
    if (!src) {
      logTrace(this, 'Empty server JS, skipping verification');
      return { src, error: null };
    }

    const settingsService = this.controller.lookupListener(SettingsService);

    const getTargetPeerResult = settingsService.getTargetPeer();
    if (!getTargetPeerResult.targetPeerCached) {
      return { src: null, error: 'target peer not ready' };
    }

    const publicKeys = getTargetPeerResult.targetPeerCached.publicKeys;
    if (!publicKeys) {
      logTrace(this, 'No public keys configured, skipping server JS verification');
      return { src, error: null };
    }

    const lastLineStart = src.lastIndexOf('\n') + 1;
    const sigPrefix = '// skymp:sig:y:';
    if (lastLineStart === 0 || !src.substring(lastLineStart).startsWith(sigPrefix)) {
      return { src: null, error: 'no signature found' };
    }
<<<<<<< Updated upstream
=======

>>>>>>> Stashed changes
    const [keyId, sig] = src.substring(lastLineStart + sigPrefix.length).split(':');
    if (!this.isAlphaNumeric(keyId)) {
      return { src: null, error: 'malformed key id' };
    }

    const key = publicKeys[keyId];
    if (!key) {
      return { src: null, error: 'unknown key' };
    }

    const toVerify = this.toArrayBufferView(src.substring(0, lastLineStart - 1), 'utf8');
    if (!verify(null, toVerify, key, this.toArrayBufferView(sig, 'base64'))) {
      return { src: null, error: 'bad signature' };
    }

    logTrace(this, `Server JS verified with key ${keyId}`);
    return { src, error: null };
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

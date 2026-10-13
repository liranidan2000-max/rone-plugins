// Prints the Ed25519 cross-check vector licence_sim.cpp embeds (kNodeSig):
// Node's own Ed25519 (the same WebCrypto algorithm the site signs with,
// worker/_lib/signing.js) over a fixed TEST seed - never the production key.
//   node Shared/tests/licence_sim/make-vector.mjs
import { createPrivateKey, createPublicKey, sign } from 'node:crypto';

const seed = Buffer.alloc(32);
for (let i = 0; i < 32; i++) seed[i] = (i * 7 + 3) & 0xff;               // kTestSeed in licence_sim.cpp
const key = createPrivateKey({ key: Buffer.concat([Buffer.from('302e020100300506032b657004220420', 'hex'), seed]), format: 'der', type: 'pkcs8' });
const pk  = createPublicKey(key).export({ format: 'der', type: 'spki' }).subarray(-32);
const msg = Buffer.from(JSON.stringify({ v: 1, m: 'abc', pass: true, owned: ['RoneThrow'], iat: 1760000000000 }));
console.log('public key:', pk.toString('hex'));
console.log('message   :', msg.toString());
console.log('signature :', sign(null, msg, key).toString('hex'));

function utf8ToBytes(str) {
  const bytes = []

  for (let i = 0; i < str.length; i += 1) {
    let code = str.charCodeAt(i)

    if (code >= 0xd800 && code <= 0xdbff && i + 1 < str.length) {
      const next = str.charCodeAt(i + 1)
      if (next >= 0xdc00 && next <= 0xdfff) {
        code = 0x10000 + ((code - 0xd800) << 10) + (next - 0xdc00)
        i += 1
      }
    }

    if (code < 0x80) {
      bytes.push(code)
    } else if (code < 0x800) {
      bytes.push(0xc0 | (code >> 6), 0x80 | (code & 0x3f))
    } else if (code < 0x10000) {
      bytes.push(
        0xe0 | (code >> 12),
        0x80 | ((code >> 6) & 0x3f),
        0x80 | (code & 0x3f)
      )
    } else {
      bytes.push(
        0xf0 | (code >> 18),
        0x80 | ((code >> 12) & 0x3f),
        0x80 | ((code >> 6) & 0x3f),
        0x80 | (code & 0x3f)
      )
    }
  }

  return bytes
}

function base64ToBytes(base64) {
  const chars = 'ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/'
  const clean = String(base64).replace(/[\r\n\s]/g, '')
  const bytes = []
  let buffer = 0
  let bits = 0

  for (let i = 0; i < clean.length; i += 1) {
    const char = clean.charAt(i)
    if (char === '=') break

    const value = chars.indexOf(char)
    if (value < 0) {
      throw new Error('Invalid base64 access key')
    }

    buffer = (buffer << 6) | value
    bits += 6

    if (bits >= 8) {
      bits -= 8
      bytes.push((buffer >> bits) & 0xff)
    }
  }

  return bytes
}

function bytesToBase64(bytes) {
  const chars = 'ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/'
  let output = ''

  for (let i = 0; i < bytes.length; i += 3) {
    const b1 = bytes[i]
    const b2 = i + 1 < bytes.length ? bytes[i + 1] : 0
    const b3 = i + 2 < bytes.length ? bytes[i + 2] : 0
    const triplet = (b1 << 16) | (b2 << 8) | b3

    output += chars[(triplet >> 18) & 0x3f]
    output += chars[(triplet >> 12) & 0x3f]
    output += i + 1 < bytes.length ? chars[(triplet >> 6) & 0x3f] : '='
    output += i + 2 < bytes.length ? chars[triplet & 0x3f] : '='
  }

  return output
}

function rotateLeft(value, bits) {
  return ((value << bits) | (value >>> (32 - bits))) >>> 0
}

function sha1(messageBytes) {
  const bytes = messageBytes.slice()
  const originalBitLength = bytes.length * 8
  const words = new Array(80)
  const hash = [
    0x67452301,
    0xefcdab89,
    0x98badcfe,
    0x10325476,
    0xc3d2e1f0
  ]

  bytes.push(0x80)
  while ((bytes.length % 64) !== 56) {
    bytes.push(0)
  }

  const high = Math.floor(originalBitLength / 0x100000000)
  const low = originalBitLength >>> 0

  bytes.push((high >>> 24) & 0xff)
  bytes.push((high >>> 16) & 0xff)
  bytes.push((high >>> 8) & 0xff)
  bytes.push(high & 0xff)
  bytes.push((low >>> 24) & 0xff)
  bytes.push((low >>> 16) & 0xff)
  bytes.push((low >>> 8) & 0xff)
  bytes.push(low & 0xff)

  for (let offset = 0; offset < bytes.length; offset += 64) {
    for (let i = 0; i < 16; i += 1) {
      const j = offset + i * 4
      words[i] = (
        (bytes[j] << 24) |
        (bytes[j + 1] << 16) |
        (bytes[j + 2] << 8) |
        bytes[j + 3]
      ) >>> 0
    }

    for (let i = 16; i < 80; i += 1) {
      words[i] = rotateLeft(words[i - 3] ^ words[i - 8] ^ words[i - 14] ^ words[i - 16], 1)
    }

    let a = hash[0]
    let b = hash[1]
    let c = hash[2]
    let d = hash[3]
    let e = hash[4]

    for (let i = 0; i < 80; i += 1) {
      let f
      let k

      if (i < 20) {
        f = (b & c) | ((~b) & d)
        k = 0x5a827999
      } else if (i < 40) {
        f = b ^ c ^ d
        k = 0x6ed9eba1
      } else if (i < 60) {
        f = (b & c) | (b & d) | (c & d)
        k = 0x8f1bbcdc
      } else {
        f = b ^ c ^ d
        k = 0xca62c1d6
      }

      const temp = (rotateLeft(a, 5) + f + e + k + words[i]) >>> 0
      e = d
      d = c
      c = rotateLeft(b, 30)
      b = a
      a = temp
    }

    hash[0] = (hash[0] + a) >>> 0
    hash[1] = (hash[1] + b) >>> 0
    hash[2] = (hash[2] + c) >>> 0
    hash[3] = (hash[3] + d) >>> 0
    hash[4] = (hash[4] + e) >>> 0
  }

  const digest = []
  for (let i = 0; i < hash.length; i += 1) {
    digest.push((hash[i] >>> 24) & 0xff)
    digest.push((hash[i] >>> 16) & 0xff)
    digest.push((hash[i] >>> 8) & 0xff)
    digest.push(hash[i] & 0xff)
  }

  return digest
}

function hmacSha1(keyBytes, messageBytes) {
  const blockSize = 64
  let key = keyBytes.slice()

  if (key.length > blockSize) {
    key = sha1(key)
  }

  while (key.length < blockSize) {
    key.push(0)
  }

  const inner = []
  const outer = []

  for (let i = 0; i < blockSize; i += 1) {
    inner.push(key[i] ^ 0x36)
    outer.push(key[i] ^ 0x5c)
  }

  return sha1(outer.concat(sha1(inner.concat(messageBytes))))
}

function createCommonToken(params) {
  const accessKey = base64ToBytes(params.author_key)
  const version = params.version
  let res = 'userid' + '/' + params.user_id
  const et = Math.ceil((Date.now() + 365 * 24 * 3600 * 1000) / 1000)
  const method = 'sha1'
  const key = et + '\n' + method + '\n' + res + '\n' + version
  const signBytes = hmacSha1(accessKey, utf8ToBytes(key))
  const sign = encodeURIComponent(bytesToBase64(signBytes))

  res = encodeURIComponent(res)

  return `version=${version}&res=${res}&et=${et}&method=${method}&sign=${sign}`
}

module.exports = {
  createCommonToken
}

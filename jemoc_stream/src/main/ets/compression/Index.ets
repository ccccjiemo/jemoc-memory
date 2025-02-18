export namespace CompressionLevel {
  export const NO_COMPRESSION = 0;

  export const BEST_COMPRESSION = 9;

  export const BEST_SPEED = 1;

  export const DEFAULT_COMPRESSION = -1;
}

export namespace CompressionStrategy {
  export const DEFAULT_STRATEGY = 0;

  export const FILTERED = 1;

  export const HUFFMAN_ONLY = 2;

  export const RLE = 3;

  export const FIXED = 4;
}

export { Inflator } from './Inflator'

export { Deflator } from './Deflator'

export { DeflateStream, DeflateStreamMode } from './DeflateStream'

export { ZipArchive, ZipArchiveEntry, ZipArchiveOption } from './ZipArchive'

export { BrotliStream, BrotliStreamOptions, BrotliStreamMode, BrotliUtils, BrotliConfig } from './BrotliStream'
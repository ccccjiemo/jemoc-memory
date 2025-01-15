export { FileStream } from 'libjemoc_stream.so'

export enum FileMode {
  READ = 0x00,
  WRITE = 0x01,
  APPEND = 0x02,
  TRUNC = 0x04,
  CREATE = 0x08
}

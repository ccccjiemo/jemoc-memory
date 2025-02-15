export enum SeekOrigin { Begin, Current, End }

type BufferLike = ArrayBufferLike | Uint8Array

export interface IStream {
  /**
   * 流是否可读
   * @returns
   */
  get canRead(): boolean;

  /**
   * 流是否可写
   * @returns
   */
  get canWrite(): boolean;

  /**
   * 流是否可随机访问
   * @returns
   */
  get canSeek(): boolean;

  /**
   * 流指针位置
   * @returns
   */
  get position(): number;

  /**
   * 流长度
   * @returns
   */
  get length(): number;

  /**
   * 设置流长度，可能会截断流
   * @param value
   */
  set length(value: number);

  /**
   * 拷贝从指针位置到流末端的数据到指定流（Stream)中.并推动指针到末端
   * @param stream 拷贝接收对象
   * @param bufferSize 拷贝缓冲大小
   */
  copyTo(stream: IStream, bufferSize?: number): void

  /**
   * 拷贝从指针位置到流末端的数据到指定流（Stream)中.并推动指针到末端
   * @param stream 拷贝接收对象
   * @param bufferSize 拷贝缓冲大小
   */
  copyToAsync(stream: IStream, bufferSize?: number): Promise<void>

  /**
   * 随机访问，指定指针位置
   * @param offset 相对偏移
   * @param origin 相对位置
   */
  seek(offset: number, origin: SeekOrigin): void

  /**
   * 从流中读取数据到指定buffer中，并推动指针位置
   * @param buffer 接受buffer
   * @param offset buffer地址偏移。offset不可为负数
   * @param count 读取大小
   * @returns 实际读取大小
   */
  read(buffer: BufferLike, offset?: number, count?: number): number

  /**
   * read的异步方法，从流中读取数据到指定buffer中，并推动指针位置
   * @param buffer 接受buffer
   * @param offset buffer地址偏移。offset不可为负数
   * @param count 读取大小
   * @returns 实际读取大小
   */
  readAsync(buffer: BufferLike, offset?: number, count?: number): Promise<number>

  /**
   * 将buffer数据写入流中，并推动指针位置
   * @param buffer 要写入的数据
   * @param offset 数据buffer的地址偏移。offset不可为负数
   * @param count 写入大小
   * @returns
   */
  write(buffer: BufferLike, offset?: number, count?: number): number

  /**
   * write异步方法
   */
  writeAsync(buffer: BufferLike, offset?: number, count?: number): Promise<number>

  /**
   * 刷新流
   */
  flush(): void

  /**
   * flush异步方法
   */
  flushAsync(): Promise<void>

  /**
   * 关闭流对象，并释放流
   */
  close(): void

  /**
   * 关闭流对象，并释放流
   */
  closeAsync(): Promise<void>
}
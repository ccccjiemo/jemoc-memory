export {XmlReader} from 'libjemoc_stream.so'
export enum XmlNodeType {
  None,
  Element,
  Attribute,
  Text,
  CDATA,
  Comment,
  Document,
  EndElement,
  XmlDeclaration,
  DocumentType,
  ProcessingInstruction,
  Whitespace
}
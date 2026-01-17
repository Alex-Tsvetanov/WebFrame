//
//  V8HeapDumpOutputStream.cpp
//  valdi
//
//  Created by Ramzy Jaber on 8/30/23.
//

#include "valdi/v8/HeapDumpOutputStream.hpp"

namespace Valdi::V8 {

HeapDumpOutputStream::HeapDumpOutputStream() : _byteBuffer(makeShared<ByteBuffer>()) {}

int HeapDumpOutputStream::GetChunkSize() {
    // TODO(rjaber): Figure out the sizing, using default but it can influence performance should be tested
    return 1024;
}

void HeapDumpOutputStream::EndOfStream() {};

HeapDumpOutputStream::WriteResult HeapDumpOutputStream::WriteAsciiChunk(char* data, int size) {
    _byteBuffer->append(data, data + size);
    return kContinue;
}

BytesView HeapDumpOutputStream::getByteView() {
    return _byteBuffer->toBytesView();
}

} // namespace Valdi::V8
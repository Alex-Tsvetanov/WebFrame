//
//  V8HeapDumpOutputStream.hpp
//  valdi
//
//  Created by Ramzy Jaber on 8/30/23.
//

#include "v8/v8-profiler.h"
#include "valdi_core/cpp/Utils/ByteBuffer.hpp"

namespace Valdi::V8 {

class HeapDumpOutputStream : public v8::OutputStream {
public:
    explicit HeapDumpOutputStream();

    int GetChunkSize() override;

    void EndOfStream() override;

    WriteResult WriteAsciiChunk(char* data, int size) override;

    BytesView getByteView();

private:
    Ref<ByteBuffer> _byteBuffer;
};

} // namespace Valdi::V8
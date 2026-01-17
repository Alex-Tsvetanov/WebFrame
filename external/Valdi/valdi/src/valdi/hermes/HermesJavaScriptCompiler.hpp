//
//  HermesJavaScriptCompiler.hpp
//  valdi-hermes
//
//  Created by Simon Corsin on 12/1/23.
//

#pragma once

#include "valdi/hermes/Hermes.hpp"
#include "valdi/hermes/HermesUtils.hpp"

#include "valdi/runtime/JavaScript/JavaScriptTypes.hpp"

#include "valdi_core/cpp/Utils/Bytes.hpp"
#include "valdi_core/cpp/Utils/LRUCache.hpp"
#include "valdi_core/cpp/Utils/Mutex.hpp"
#include "valdi_core/cpp/Utils/Result.hpp"
#include "valdi_core/cpp/Utils/Shared.hpp"

namespace hermes::hbc {
class BCProviderFromSrc;
}

namespace Valdi::Hermes {

class HermesJavaScriptCompiler {
public:
    HermesJavaScriptCompiler();
    ~HermesJavaScriptCompiler();

    Shared<hermes::hbc::BCProvider> compile(const std::string& script,
                                            const std::string_view& sourceFilename,
                                            JSExceptionTracker& exceptionTracker);

    static BytesView compileToSerializedBytecode(const std::string& script,
                                                 const std::string_view& sourceFilename,
                                                 JSExceptionTracker& exceptionTracker);

    static Shared<hermes::hbc::BCProvider> deserializeBytecode(const BytesView& serializedBytecode,
                                                               JSExceptionTracker& exceptionTracker);

    static HermesJavaScriptCompiler* get();

private:
    LRUCache<std::string, Shared<hermes::hbc::BCProvider>> _cache;
    Mutex _mutex;

    static Shared<hermes::hbc::BCProviderFromSrc> compileToBytecode(
        const std::string& script,
        const std::string_view& sourceFilename,
        hermes::OutputFormatKind outputFormat,
        std::function<void(hermes::Module&)> runOptimizations,
        const hermes::BytecodeGenerationOptions& bytecodeGenerationsOptions,
        JSExceptionTracker& exceptionTracker);
};

} // namespace Valdi::Hermes

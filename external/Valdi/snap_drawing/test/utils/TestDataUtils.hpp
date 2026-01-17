#include "valdi_core/cpp/Utils/Bytes.hpp"
#include "valdi_core/cpp/Utils/PathUtils.hpp"
#include "valdi_core/cpp/Utils/Result.hpp"

namespace snap::drawing {

Valdi::Path resolveTestPath(const std::string& path);
Valdi::Result<Valdi::BytesView> getTestData(const std::string& filename);

} // namespace snap::drawing

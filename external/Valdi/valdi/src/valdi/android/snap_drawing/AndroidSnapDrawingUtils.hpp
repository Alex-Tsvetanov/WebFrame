#pragma once

#include "valdi_core/jni/JavaObject.hpp"

namespace snap::drawing {
class Matrix;
class Path;
} // namespace snap::drawing

namespace Valdi {
class CoordinateResolver;
} // namespace Valdi

namespace ValdiAndroid {

JavaObject createTransformJavaArray(JavaEnv env,
                                    const snap::drawing::Matrix& matrix,
                                    const Valdi::CoordinateResolver& coordinateResolver);

JavaObject createPathJavaArray(JavaEnv env,
                               const snap::drawing::Path& path,
                               const Valdi::CoordinateResolver& coordinateResolver);

} // namespace ValdiAndroid
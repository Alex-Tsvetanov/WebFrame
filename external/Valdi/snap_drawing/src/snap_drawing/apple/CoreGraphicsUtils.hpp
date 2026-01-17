//
//  CoreGraphicsUtils.hpp
//  snap_drawing-macos
//
//  Created by Simon Corsin on 2/14/22.
//

#pragma once

#include "snap_drawing/cpp/Utils/Matrix.hpp"
#include "snap_drawing/cpp/Utils/Path.hpp"
#include "valdi_core/cpp/Utils/Result.hpp"

#include <CoreGraphics/CGPath.h>
#include <QuartzCore/CATransform3D.h>

namespace snap::drawing {

CGPathRef cgPathFromPath(const Path& path);
CATransform3D caTransform3DFromMatrix(const Matrix& matrix);
CGAffineTransform cgAffineTransformFromMatrix(const Matrix& matrix);

} // namespace snap::drawing

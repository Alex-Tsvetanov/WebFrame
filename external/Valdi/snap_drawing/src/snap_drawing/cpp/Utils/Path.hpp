//
//  Path.hpp
//  snap_drawing
//
//  Created by Simon Corsin on 1/28/22.
//

#pragma once

#include "snap_drawing/cpp/Utils/Geometry.hpp"
#include "snap_drawing/cpp/Utils/SkiaBridge.hpp"

#include "include/core/SkPath.h"

#include <optional>
#include <ostream>
#include <string>

namespace snap::drawing {

class Matrix;

class PathVisitor;

class Path : public WrappedSkValue<SkPath> {
public:
    Path();
    Path(Path&& other) noexcept;
    Path(const Path& other) noexcept;

    ~Path();

    Path& operator=(Path&& other) noexcept;
    Path& operator=(const Path& other) noexcept;

    void reset();

    void addRoundRect(const Rect& bounds, Scalar radii[8], bool clockwise);

    void addRoundRect(const Rect& bounds, Scalar rx, Scalar ry, bool clockwise);

    /** Set the beginning of the next contour to the point (x,y). */
    void moveTo(Scalar x, Scalar y);

    /**
    Add a line from the last point to the specified point (x,y). If no
    sk_path_move_to() call has been made for this contour, the first
    point is automatically set to (0,0).
    */
    void lineTo(Scalar x, Scalar y);

    /**
    Add a quadratic bezier from the last point, approaching control
    point (x0,y0), and ending at (x1,y1). If no sk_path_move_to() call
    has been made for this contour, the first point is automatically
    set to (0,0).
    */
    void quadTo(Scalar x0, Scalar y0, Scalar x1, Scalar y1);

    /**
    Add a conic curve from the last point, approaching control point
    (x0,y01), and ending at (x1,y1) with weight w.  If no
    sk_path_move_to() call has been made for this contour, the first
    point is automatically set to (0,0).
    */
    void conicTo(Scalar x0, Scalar y0, Scalar x1, Scalar y1, Scalar weight);

    /**
    Add a cubic bezier from the last point, approaching control points
    (x0,y0) and (x1,y1), and ending at (x2,y2). If no
    sk_path_move_to() call has been made for this contour, the first
    point is automatically set to (0,0).
    */
    void cubicTo(Scalar x0, Scalar y0, Scalar x1, Scalar y1, Scalar x2, Scalar y2);

    /** Appends arc to SkPath, as the start of new contour. Arc added is part of ellipse
           bounded by oval, from startAngle through sweepAngle. Both startAngle and
           sweepAngle are measured in degrees, where zero degrees is aligned with the
           positive x-axis, and positive sweeps extends arc clockwise.

           If sweepAngle <= -360, or sweepAngle >= 360; and startAngle modulo 90 is nearly
           zero, append oval instead of arc. Otherwise, sweepAngle values are treated
           modulo 360, and arc may or may not draw depending on numeric rounding.

           @param oval        bounds of ellipse containing arc
           @param startAngle  starting angle of arc in degrees
           @param sweepAngle  sweep, in degrees. Positive is clockwise; treated modulo 360
           @return            reference to SkPath

           example: https://fiddle.skia.org/c/@Path_addArc
       */
    void arcTo(const Rect& oval, Scalar startAngle, Scalar sweepAngle);

    /**
    Close the current contour. If the current point is not equal to the
    first point of the contour, a line segment is automatically added.
    */
    void close();

    /**
    Add a closed rectangle contour to the path.
    */
    void addRect(const Rect& bounds, bool clockwise);

    /**
    Add a closed oval contour to the path
    */
    void addOval(const Rect& bounds, bool clockwise);

    void transform(const Matrix& matrix);

    void addPath(const Path& path);

    /**
     *  If the path is empty, return false and set the rect parameter to [0, 0, 0, 0].
     *  else return true and set the rect parameter to the bounds of the control-points
     *  of the path.
     */
    std::optional<Rect> getBounds() const;

    bool isEmpty() const;

    std::string toString() const;

    Path intersection(const Path& other) const;

    void visit(PathVisitor& visitor) const;

    bool operator==(const Path& other) const;
    bool operator!=(const Path& other) const;
};

std::ostream& operator<<(std::ostream& os, const Path& path);

class PathVisitor {
public:
    virtual ~PathVisitor() = default;

    virtual bool shouldConvertConicToQuad() const = 0;

    virtual void move(const Point& point) = 0;
    virtual void line(const Point& from, const Point& to) = 0;
    virtual void quad(const Point& p1, const Point& p2, const Point& p3) = 0;
    virtual void conic(const Point& p1, const Point& p2, const Point& p3, Scalar weight) = 0;
    virtual void cubic(const Point& p1, const Point& p2, const Point& p3, const Point& p4) = 0;
    virtual void close() = 0;
};

} // namespace snap::drawing

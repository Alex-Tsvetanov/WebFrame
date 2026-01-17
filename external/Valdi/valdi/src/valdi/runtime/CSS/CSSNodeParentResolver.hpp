//
//  CSSNodeParentResolver.hpp
//  valdi-benchmark-utils
//
//  Created by Simon Corsin on 4/22/21.
//

#pragma once

namespace Valdi {

class CSSNode;
class CSSDocument;

class CSSNodeParentResolver {
public:
    virtual ~CSSNodeParentResolver() = default;
    virtual CSSNode* getCSSParentForDocument(const CSSDocument& cssDocument) = 0;
};

} // namespace Valdi

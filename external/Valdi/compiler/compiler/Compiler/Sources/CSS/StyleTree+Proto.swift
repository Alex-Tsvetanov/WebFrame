//
//  StyleTree+Proto.swift
//  Compiler
//
//  Created by Nathaniel Parrott on 5/14/18.
//  Copyright © 2018 Snap Inc. All rights reserved.
//

import Foundation

extension StyleNode {

    private func convert(logger: ILogger, rules: [String: StyleNode]) -> [Valdi_NamedStyleNode] {
        var out: [Valdi_NamedStyleNode] = rules.map {
            var node = Valdi_NamedStyleNode()
            node.name = $0
            node.node = $1.toProto(logger: logger)
            return node
        }

        out.sort(by: { $0.name < $1.name })

        return out
    }

    func toProto(logger: ILogger) -> Valdi_StyleNode {
        var p = Valdi_StyleNode()

        p.ruleIndex.idRules = convert(logger: logger, rules: idRules)
        p.ruleIndex.classRules = convert(logger: logger, rules: classRules)
        p.ruleIndex.tagRules = convert(logger: logger, rules: tagRules)
        for attrRule in attribRules {
            var ruleProto = Valdi_CSSRuleIndex.AttributeRule()
            ruleProto.attribute = Valdi_NodeAttribute(logger: logger, name: attrRule.attrib, value: attrRule.value)
            ruleProto.type = attrRule.type
            ruleProto.node = attrRule.node.toProto(logger: logger)
            p.ruleIndex.attributeRules.append(ruleProto)
        }
        if let firstChild = firstChildRule {
            p.ruleIndex.firstChildRule = firstChild.toProto(logger: logger)
        }
        if let lastChild = lastChildRule {
            p.ruleIndex.lastChildRule = lastChild.toProto(logger: logger)
        }
        for nthChildRule in nthChildRules {
            var ruleProto = Valdi_CSSRuleIndex.NthChildRule()
            if let n = nthChildRule.n {
                ruleProto.n = Int32(n)
            }
            if let offset = nthChildRule.offset {
                ruleProto.offset = Int32(offset)
            }
            ruleProto.node = nthChildRule.node.toProto(logger: logger)
            p.ruleIndex.nthChildRules.append(ruleProto)
        }
        if let parent = directParentRule {
            p.ruleIndex.directParentRules = parent.toProto(logger: logger).ruleIndex
        }
        if let parent = ancestorRule {
            p.ruleIndex.ancestorRules = parent.toProto(logger: logger).ruleIndex
        }
        p.styles = declarations
        return p
    }
}

//
//  Tests.swift
//  Compiler
//
//  Created by Nathaniel Parrott on 5/14/18.
//  Copyright © 2018 Snap Inc. All rights reserved.
//

import Foundation
import SwiftCSSParser
//
// func test() {
//    testCSSParsing()
//    testCSSImportParsing()
//    testCommaSelectorParsing()
//    testHierarchicalSelectorParsing()
//    testCSSErrors()
//    testSelectorTermParsing()
//    testSelectorTermParsing2()
//    testStyleTree()
//    testStyleTreeSpecificity()
//    testUniversalSelector()
//    testMultiSelector()
//    testStyleTreeProto()
//    testAttributeEqualitySelector()
//    testQuoting()
//    testCamelCase()
//    testNumericalValues()
//    testCamelCaseAttributeSelectors()
//    print("🎉 Tests passed!")
// }

// func testCSSParsing() {
//    let basicDeclaration = StyleSheet(string: "#element {color: red;}")!
//    assert(basicDeclaration.rules.count == 1)
//    assert(basicDeclaration.rules[0].declarations.count == 1)
//    assert(basicDeclaration.rules[0].declarations[0].name == "color");
//    assert(basicDeclaration.rules[0].declarations[0].value == "red");
//    assert(basicDeclaration.rules[0].selectors.count == 1)
//    let sel = basicDeclaration.rules[0].selectors[0]
//    assert(sel.match! == .id)
//    assert(sel.value! == "element")
// }
//
// func testCSSImportParsing() {
//    let sheet = StyleSheet(string: "@import 'test.css';")!
//    assert(sheet.importFilenames.count == 1)
//    assert(sheet.importFilenames[0] == "test.css")
// }
//
// func testCommaSelectorParsing() {
//    let sheet = StyleSheet(string: "h1, h2 { x: 1; }")!
//    assert(sheet.rules.count == 1)
//    assert(sheet.rules[0].selectors.count == 2)
// }
//
// func testHierarchicalSelectorParsing() {
//    let sheet = StyleSheet(string: "abc def > ghi:first-child {x: y}")!
//    assert(sheet.rules[0].selectors.count == 1)
//
//    let ghi = sheet.rules[0].selectors[0]
//    assert(ghi.value! == "ghi")
//    assert(ghi.relation! == .subselector)
//
//    let firstChild = ghi.related!
//    assert(firstChild.match! == .pseudoClass)
//    assert(firstChild.psuedoType! == .firstChild)
//    assert(firstChild.relation! == .child)
//
//    let def = firstChild.related!
//    assert(def.relation! == .descendant)
//
//    let abc = def.related!
//    assert(abc.value! == "abc")
//    assert(abc.related!.match == nil)
// }
//
// func testCSSErrors() {
//    let sheet = StyleSheet(string: "{}")!
//    assert(sheet.errors.count == 1)
// }
//
// func testSelectorTermParsing() {
//    let sheet = StyleSheet(string: "*:first-child > myTag.myClass.myClass2[attr=\"xyz\"] {x: y}")!
//    let termNode = SelectorTerm()
//    try! SelectorTerm.parseSelectorToTerms(selector: sheet.rules[0].selectors[0], termNode: termNode)
//    assert(termNode.tagRule == "myTag")
//    assert(termNode.classRules.contains("myClass"))
//    assert(termNode.classRules.contains("myClass2"))
//    assert(termNode.attributeRules[0].attribute == "attr")
//    assert(termNode.attributeRules[0].value == "xyz")
//    assert(termNode.parent!.directChild)
//    let parent = termNode.parent!.term
//    assert(parent.classRules.count == 0)
//    assert(parent.firstChild)
//    assert(!parent.lastChild)
//    assert(parent.parent == nil)
// }
//
// func testSelectorTermParsing2() {
//    let sheet = StyleSheet(string: "#waka:nth-child(3) #flocka.FLAME {x: y}")!
//    let termNode = SelectorTerm()
//    try! SelectorTerm.parseSelectorToTerms(selector: sheet.rules[0].selectors[0], termNode: termNode)
//    assert(termNode.classRules.contains("FLAME"))
//    assert(termNode.idRule! == "flocka")
//    assert(termNode.parent!.directChild == false)
//    assert(termNode.parent!.term.idRule! == "waka")
//    // TODO: (nate) fix nth-child parsing
//    // assert(termNode.parent!.term.nthChildRules.first!.n == 3)
// }
//
// func testStyleTree() {
//    let t = StyleTree()
//    var imports: [String] = []
//    try! t.add(cssString: "@import 'abc.css'; @import 'xyz.css'; #id > .xyz.abc { rule1: val1; rule2: val2; }", filename: "test.css") { (name) in
//        imports.append(name)
//    }
//    assert(imports == ["abc.css", "xyz.css"])
//    let node = t.root.classRules["abc"]!.classRules["xyz"]!.directParentRule!.idRules["id"]!
//    assert(node.declarations.count == 2)
//    assert(node.declarations[0].order == 0)
//    assert(node.declarations[1].order == 1)
//    assert(node.declarations[0].attribute.name == "rule1")
//    assert(node.declarations[0].attribute.strValue == "val1")
// }
//
// func testStyleTreeSpecificity() {
//    let t = StyleTree()
//    try! t.add(cssString: "#xyz #def {rule: abc;} Label:first-child {rule: def;}", filename: "test.css", onImport: nil)
//    let abc = t.root.idRules["def"]!.ancestorRule!.idRules["xyz"]!.declarations.first!
//    let def = t.root.tagRules["Label"]!.firstChildRule!.declarations.first!
//    assert(abc.priority > def.priority)
//
//    try! t.add(cssString: "Label:first-child {rule2: one;} Label.cls {rule2: two;}", filename: "test.css", onImport: nil)
//    let one = t.root.tagRules["Label"]!.firstChildRule!.declarations.first!
//    let two = t.root.classRules["cls"]!.tagRules["Label"]!.declarations.first!
//    assert(one.priority == two.priority)
// }
//
// func testUniversalSelector() {
//    let t = StyleTree()
//    try! t.add(cssString: "* > * {abc: def;}", filename: "test.css", onImport: nil)
//    assert(t.root.tagRules["*"]!.directParentRule!.tagRules["*"]!.declarations.first!.attribute.name == "abc")
// }
//
// func testMultiSelector() {
//    let t = StyleTree()
//    try! t.add(cssString: "h1, h2 { color: red; }", filename: "test.css", onImport: nil)
//    assert(t.root.tagRules["h1"]!.declarations.count == 1)
//    assert(t.root.tagRules["h2"]!.declarations.count == 1)
// }
//
// func testNumericalValues() {
//    let t = StyleTree()
//    try! t.add(cssString: "h1 {width: 100%; height: 50px; left: 20; color: #ffffff; }", filename: "test.css", onImport: nil)
//    let h1 = t.root.tagRules["h1"]!.declarations
//    assert(h1[0].attribute.strValue == "100%")
//    assert(h1[1].attribute.strValue == "50px")
//    assert(h1[2].attribute.strValue == "20")
//    assert(h1[3].attribute.strValue == "#ffffff")
// }
//
// func testStyleTreeProto() {
//    let t = StyleTree()
//    try! t.add(cssString: ".class #id > Tag:first-child {test: value;}", filename: "test.css", onImport: nil)
//    let p = t.root.proto
//    assert(p.ruleIndex.tagRules["Tag"]!.ruleIndex.firstChildRule.ruleIndex.directParentRules.idRules["id"]!.ruleIndex.ancestorRules.classRules["class"]!.styles.first!.attribute.strValue == "value")
//    // ensure that unused fields are unset:
//    assert(!p.ruleIndex.hasDirectParentRules)
// }
//
// func testAttributeEqualitySelector() {
//    let t = StyleTree()
//    try! t.add(cssString: "[def=\"456\"][abc=\"123\"] {test: value;}", filename: "test.css", onImport: nil)
//    let p = t.root.proto
//    // ensure the attributes are sorted properly
//    assert(p.ruleIndex.attributeRules[0].attribute.name == "abc")
//    assert(p.ruleIndex.attributeRules[0].attribute.strValue == "123")
//    assert(p.ruleIndex.attributeRules[0].node.ruleIndex.attributeRules[0].attribute.name == "def")
// }
//
// func testQuoting() {
//    let t = StyleTree()
//    try! t.add(cssString: "Label {x: abc; y: 'def'; z: \"ghi\";}", filename: "test.css", onImport: nil)
//    let p = t.root.proto
//    let styles = p.ruleIndex.tagRules["Label"]!.styles
//    assert(styles[0].attribute.strValue == "abc")
//    assert(styles[1].attribute.strValue == "def")
//    assert(styles[2].attribute.strValue == "ghi")
// }
//
// func testCamelCase() {
//    let t = StyleTree()
//    try! t.add(cssString: "Test { abc-def: abc-def ;}", filename: "test.css", onImport: nil)
//    let p = t.root.proto
//    let rule = p.ruleIndex.tagRules["Test"]!.styles.first!
//    assert(rule.attribute.name == "abcDef")
//    assert(rule.attribute.strValue == "abc-def")
// }
//
// func testCamelCaseAttributeSelectors() {
//    let t = StyleTree()
//    try! t.add(cssString: "[my-attr=\"value\"] { abc: def; }", filename: "test.css", onImport: nil)
//    let p = t.root.proto
//    assert(p.ruleIndex.attributeRules.first!.attribute.name == "myAttr")
// }

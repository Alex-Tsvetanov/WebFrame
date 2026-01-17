#include "ViewNodeTestsUtils.hpp"
#include "gtest/gtest.h"

using namespace Valdi;

namespace ValdiTest {

TEST(ViewNodeAccessibility, isAccessibilityPassthroughByDefaultWhenIsLayout) {
    ViewNodeTestsDependencies utils;

    auto layout = utils.createLayout();

    ASSERT_EQ(layout->isAccessibilityFocusable(), false);
    ASSERT_EQ(layout->isAccessibilityContainer(), true);
    ASSERT_EQ(layout->isAccessibilityPassthrough(), true);

    ASSERT_EQ(layout->getAccessibilityCategory(), AccessibilityCategoryView);
    ASSERT_EQ(layout->getAccessibilityNavigation(), AccessibilityNavigationPassthrough);

    ASSERT_EQ(layout->getAccessibilityLabel(), STRING_LITERAL(""));
    ASSERT_EQ(layout->getAccessibilityHint(), STRING_LITERAL(""));
    ASSERT_EQ(layout->getAccessibilityValue(), STRING_LITERAL(""));

    ASSERT_EQ(layout->getAccessibilityStateDisabled(), false);
    ASSERT_EQ(layout->getAccessibilityStateSelected(), false);
    ASSERT_EQ(layout->getAccessibilityStateLiveRegion(), false);
}

TEST(ViewNodeAccessibility, isAccessibilityCoverByDefaultWhenIsLayoutWithLabel) {
    ViewNodeTestsDependencies utils;

    auto layout = utils.createLayout();
    utils.setViewNodeAttribute(layout, "accessibilityLabel", Value(STRING_LITERAL("accessibility-label")));

    ASSERT_EQ(layout->isAccessibilityFocusable(), true);
    ASSERT_EQ(layout->isAccessibilityContainer(), true);
    ASSERT_EQ(layout->isAccessibilityPassthrough(), true);

    ASSERT_EQ(layout->getAccessibilityCategory(), AccessibilityCategoryView);
    ASSERT_EQ(layout->getAccessibilityNavigation(), AccessibilityNavigationCover);

    ASSERT_EQ(layout->getAccessibilityLabel(), STRING_LITERAL("accessibility-label"));
    ASSERT_EQ(layout->getAccessibilityHint(), STRING_LITERAL(""));
    ASSERT_EQ(layout->getAccessibilityValue(), STRING_LITERAL(""));

    ASSERT_EQ(layout->getAccessibilityStateDisabled(), false);
    ASSERT_EQ(layout->getAccessibilityStateSelected(), false);
    ASSERT_EQ(layout->getAccessibilityStateLiveRegion(), false);
}

TEST(ViewNodeAccessibility, isAccessibilityPassthroughByDefaultWhenHasNothing) {
    ViewNodeTestsDependencies utils;

    auto view = utils.createView();

    ASSERT_EQ(view->isAccessibilityFocusable(), false);
    ASSERT_EQ(view->isAccessibilityContainer(), true);
    ASSERT_EQ(view->isAccessibilityPassthrough(), true);

    ASSERT_EQ(view->getAccessibilityCategory(), AccessibilityCategoryView);
    ASSERT_EQ(view->getAccessibilityNavigation(), AccessibilityNavigationPassthrough);

    ASSERT_EQ(view->getAccessibilityLabel(), STRING_LITERAL(""));
    ASSERT_EQ(view->getAccessibilityHint(), STRING_LITERAL(""));
    ASSERT_EQ(view->getAccessibilityValue(), STRING_LITERAL(""));

    ASSERT_EQ(view->getAccessibilityStateDisabled(), false);
    ASSERT_EQ(view->getAccessibilityStateSelected(), false);
    ASSERT_EQ(view->getAccessibilityStateLiveRegion(), false);
}

TEST(ViewNodeAccessibility, isAccessibilityCoverByDefaultWhenIsViewWithLabel) {
    ViewNodeTestsDependencies utils;

    auto view = utils.createView();
    utils.setViewNodeAttribute(view, "accessibilityLabel", Value(STRING_LITERAL("accessibility-label")));

    ASSERT_EQ(view->isAccessibilityFocusable(), true);
    ASSERT_EQ(view->isAccessibilityContainer(), true);
    ASSERT_EQ(view->isAccessibilityPassthrough(), true);

    ASSERT_EQ(view->getAccessibilityCategory(), AccessibilityCategoryView);
    ASSERT_EQ(view->getAccessibilityNavigation(), AccessibilityNavigationCover);

    ASSERT_EQ(view->getAccessibilityLabel(), STRING_LITERAL("accessibility-label"));
    ASSERT_EQ(view->getAccessibilityHint(), STRING_LITERAL(""));
    ASSERT_EQ(view->getAccessibilityValue(), STRING_LITERAL(""));

    ASSERT_EQ(view->getAccessibilityStateDisabled(), false);
    ASSERT_EQ(view->getAccessibilityStateSelected(), false);
    ASSERT_EQ(view->getAccessibilityStateLiveRegion(), false);
}

TEST(ViewNodeAccessibility, isAccessibilityLeafByDefaultWhenHasValue) {
    ViewNodeTestsDependencies utils;

    auto view = utils.createView();
    utils.setViewNodeAttribute(view, "value", Value(STRING_LITERAL("label-value")));

    ASSERT_EQ(view->isAccessibilityFocusable(), true);
    ASSERT_EQ(view->isAccessibilityContainer(), false);
    ASSERT_EQ(view->isAccessibilityPassthrough(), false);

    ASSERT_EQ(view->getAccessibilityCategory(), AccessibilityCategoryText);
    ASSERT_EQ(view->getAccessibilityNavigation(), AccessibilityNavigationLeaf);

    ASSERT_EQ(view->getAccessibilityLabel(), STRING_LITERAL(""));
    ASSERT_EQ(view->getAccessibilityHint(), STRING_LITERAL(""));
    ASSERT_EQ(view->getAccessibilityValue(), STRING_LITERAL("label-value"));

    ASSERT_EQ(view->getAccessibilityStateDisabled(), false);
    ASSERT_EQ(view->getAccessibilityStateSelected(), false);
    ASSERT_EQ(view->getAccessibilityStateLiveRegion(), false);
}

TEST(ViewNodeAccessibility, isAccessibilityLeafByDefaultWhenHasPlaceholder) {
    ViewNodeTestsDependencies utils;

    auto view = utils.createView();
    utils.setViewNodeAttribute(view, "placeholder", Value(STRING_LITERAL("input-placeholder")));

    ASSERT_EQ(view->isAccessibilityFocusable(), true);
    ASSERT_EQ(view->isAccessibilityContainer(), false);
    ASSERT_EQ(view->isAccessibilityPassthrough(), false);

    ASSERT_EQ(view->getAccessibilityCategory(), AccessibilityCategoryInput);
    ASSERT_EQ(view->getAccessibilityNavigation(), AccessibilityNavigationLeaf);

    ASSERT_EQ(view->getAccessibilityLabel(), STRING_LITERAL(""));
    ASSERT_EQ(view->getAccessibilityHint(), STRING_LITERAL(""));
    ASSERT_EQ(view->getAccessibilityValue(), STRING_LITERAL("input-placeholder"));

    ASSERT_EQ(view->getAccessibilityStateDisabled(), false);
    ASSERT_EQ(view->getAccessibilityStateSelected(), false);
    ASSERT_EQ(view->getAccessibilityStateLiveRegion(), false);
}

TEST(ViewNodeAccessibility, isAccessibilityPassthroughByDefaultWhenHasJustSrc) {
    ViewNodeTestsDependencies utils;

    auto view = utils.createView();
    utils.setViewNodeAttribute(view, "src", Value(STRING_LITERAL("asset-src")));

    ASSERT_EQ(view->isAccessibilityFocusable(), false);
    ASSERT_EQ(view->isAccessibilityContainer(), true);
    ASSERT_EQ(view->isAccessibilityPassthrough(), true);

    ASSERT_EQ(view->getAccessibilityCategory(), AccessibilityCategoryView);
    ASSERT_EQ(view->getAccessibilityNavigation(), AccessibilityNavigationPassthrough);

    ASSERT_EQ(view->getAccessibilityLabel(), STRING_LITERAL(""));
    ASSERT_EQ(view->getAccessibilityHint(), STRING_LITERAL(""));
    ASSERT_EQ(view->getAccessibilityValue(), STRING_LITERAL(""));

    ASSERT_EQ(view->getAccessibilityStateDisabled(), false);
    ASSERT_EQ(view->getAccessibilityStateSelected(), false);
    ASSERT_EQ(view->getAccessibilityStateLiveRegion(), false);
}

TEST(ViewNodeAccessibility, isAccessibilityLeafByDefaultWhenHasSrcAndLabel) {
    ViewNodeTestsDependencies utils;

    auto view = utils.createView();
    utils.setViewNodeAttribute(view, "src", Value(STRING_LITERAL("asset-src")));
    utils.setViewNodeAttribute(view, "accessibilityLabel", Value(STRING_LITERAL("accessibility-label")));

    ASSERT_EQ(view->isAccessibilityFocusable(), true);
    ASSERT_EQ(view->isAccessibilityContainer(), false);
    ASSERT_EQ(view->isAccessibilityPassthrough(), false);

    ASSERT_EQ(view->getAccessibilityCategory(), AccessibilityCategoryImage);
    ASSERT_EQ(view->getAccessibilityNavigation(), AccessibilityNavigationLeaf);

    ASSERT_EQ(view->getAccessibilityLabel(), STRING_LITERAL("accessibility-label"));
    ASSERT_EQ(view->getAccessibilityHint(), STRING_LITERAL(""));
    ASSERT_EQ(view->getAccessibilityValue(), STRING_LITERAL(""));

    ASSERT_EQ(view->getAccessibilityStateDisabled(), false);
    ASSERT_EQ(view->getAccessibilityStateSelected(), false);
    ASSERT_EQ(view->getAccessibilityStateLiveRegion(), false);
}

TEST(ViewNodeAccessibility, hasChildWithAccessibilityId) {
    ViewNodeTestsDependencies utils;

    auto button = utils.createView();
    utils.setViewNodeAttribute(button, "accessibilityCategory", Value(STRING_LITERAL("button")));
    utils.setViewNodeAttribute(button, "accessibilityId", Value(STRING_LITERAL("action-button")));

    auto label = utils.createView();
    utils.setViewNodeAttribute(label, "accessibilityId", Value(STRING_LITERAL("section_header_action_button")));
    utils.setViewNodeMarginAndSize(label, 0, 100, 100);

    auto image = utils.createView();
    utils.setViewNodeMarginAndSize(image, 0, 100, 100);

    button->appendChild(utils.getViewTransactionScope(), label);
    button->appendChild(utils.getViewTransactionScope(), image);

    button->performLayout(utils.getViewTransactionScope(), Size(100, 100), LayoutDirectionLTR);

    ASSERT_EQ(button->hasChildWithAccessibilityId(), true);

    auto children = button->getAccessibilityChildrenRecursive();

    ASSERT_EQ(children.size(), 1ul);
}

TEST(ViewNodeAccessibility, isAccessibilityChildrenShallowSortedByPriority) {
    ViewNodeTestsDependencies utils;

    auto root = utils.createLayout();

    auto layoutP10 = utils.createLayout();
    auto layoutP12 = utils.createLayout();
    auto layoutP90 = utils.createLayout();
    auto layoutP30 = utils.createLayout();

    utils.setViewNodeMarginAndSize(layoutP10, 10, 10, 10);
    utils.setViewNodeMarginAndSize(layoutP12, 10, 10, 10);
    utils.setViewNodeMarginAndSize(layoutP90, 10, 10, 10);
    utils.setViewNodeMarginAndSize(layoutP30, 10, 10, 10);

    root->appendChild(utils.getViewTransactionScope(), layoutP10);
    root->appendChild(utils.getViewTransactionScope(), layoutP12);
    root->appendChild(utils.getViewTransactionScope(), layoutP90);
    root->appendChild(utils.getViewTransactionScope(), layoutP30);

    root->performLayout(utils.getViewTransactionScope(), Size(100, 100), LayoutDirectionLTR);

    layoutP10->setAccessibilityPriority(10);
    layoutP12->setAccessibilityPriority(12);
    layoutP90->setAccessibilityPriority(90);
    layoutP30->setAccessibilityPriority(30);

    auto children = root->getAccessibilityChildrenShallow();

    ASSERT_EQ(children.size(), 4ul);
    ASSERT_EQ(children[0], layoutP90.get());
    ASSERT_EQ(children[1], layoutP30.get());
    ASSERT_EQ(children[2], layoutP12.get());
    ASSERT_EQ(children[3], layoutP10.get());
}

TEST(ViewNodeAccessibility, isAccessibilityChildrenShallowUnsortedWhenNoPriority) {
    ViewNodeTestsDependencies utils;

    auto root = utils.createLayout();

    auto layout1 = utils.createLayout();
    auto layout2 = utils.createLayout();
    auto layout3 = utils.createLayout();
    auto layout4 = utils.createLayout();

    utils.setViewNodeMarginAndSize(layout1, 10, 10, 10);
    utils.setViewNodeMarginAndSize(layout2, 10, 10, 10);
    utils.setViewNodeMarginAndSize(layout3, 10, 10, 10);
    utils.setViewNodeMarginAndSize(layout4, 10, 10, 10);

    root->appendChild(utils.getViewTransactionScope(), layout1);
    root->appendChild(utils.getViewTransactionScope(), layout2);
    root->appendChild(utils.getViewTransactionScope(), layout3);
    root->appendChild(utils.getViewTransactionScope(), layout4);

    root->performLayout(utils.getViewTransactionScope(), Size(100, 100), LayoutDirectionLTR);

    layout1->setAccessibilityPriority(0);
    layout2->setAccessibilityPriority(0);
    layout3->setAccessibilityPriority(0);
    layout4->setAccessibilityPriority(0);

    auto children = root->getAccessibilityChildrenShallow();

    ASSERT_EQ(children.size(), 4ul);
    ASSERT_EQ(children[0], layout1.get());
    ASSERT_EQ(children[1], layout2.get());
    ASSERT_EQ(children[2], layout3.get());
    ASSERT_EQ(children[3], layout4.get());
}

TEST(ViewNodeAccessibility, isAccessibilityChildrenRecursiveWithPassthroughAndPassthroughts) {
    ViewNodeTestsDependencies utils;

    auto root = utils.createLayout();
    auto inner = utils.createLayout();
    auto layout1 = utils.createLayout();
    auto layout2 = utils.createLayout();

    utils.setViewNodeMarginAndSize(inner, 10, 10, 10);
    utils.setViewNodeMarginAndSize(layout1, 10, 10, 10);
    utils.setViewNodeMarginAndSize(layout2, 10, 10, 10);

    root->appendChild(utils.getViewTransactionScope(), inner);
    inner->appendChild(utils.getViewTransactionScope(), layout1);
    inner->appendChild(utils.getViewTransactionScope(), layout2);

    root->performLayout(utils.getViewTransactionScope(), Size(100, 100), LayoutDirectionLTR);

    inner->setAccessibilityNavigation(AccessibilityNavigationPassthrough);
    layout1->setAccessibilityNavigation(AccessibilityNavigationPassthrough);
    layout2->setAccessibilityNavigation(AccessibilityNavigationPassthrough);

    auto childrenRoot = root->getAccessibilityChildrenRecursive();
    ASSERT_EQ(childrenRoot.size(), 0ul);

    auto childrenInner = inner->getAccessibilityChildrenRecursive();
    ASSERT_EQ(childrenInner.size(), 0ul);
}

TEST(ViewNodeAccessibility, isAccessibilityChildrenRecursiveWithPassthroughAndLeafs) {
    ViewNodeTestsDependencies utils;

    auto root = utils.createLayout();
    auto inner = utils.createLayout();
    auto layout1 = utils.createLayout();
    auto layout2 = utils.createLayout();

    utils.setViewNodeMarginAndSize(inner, 10, 10, 10);
    utils.setViewNodeMarginAndSize(layout1, 10, 10, 10);
    utils.setViewNodeMarginAndSize(layout2, 10, 10, 10);

    root->appendChild(utils.getViewTransactionScope(), inner);
    inner->appendChild(utils.getViewTransactionScope(), layout1);
    inner->appendChild(utils.getViewTransactionScope(), layout2);

    root->performLayout(utils.getViewTransactionScope(), Size(100, 100), LayoutDirectionLTR);

    inner->setAccessibilityNavigation(AccessibilityNavigationPassthrough);
    layout1->setAccessibilityNavigation(AccessibilityNavigationLeaf);
    layout2->setAccessibilityNavigation(AccessibilityNavigationLeaf);

    auto childrenRoot = root->getAccessibilityChildrenRecursive();
    ASSERT_EQ(childrenRoot.size(), 2ul);
    ASSERT_EQ(childrenRoot[0], layout1.get());
    ASSERT_EQ(childrenRoot[1], layout2.get());
}

TEST(ViewNodeAccessibility, isAccessibilityChildrenRecursiveWithLeafsAndLeafs) {
    ViewNodeTestsDependencies utils;

    auto root = utils.createLayout();
    auto inner = utils.createLayout();
    auto layout1 = utils.createLayout();
    auto layout2 = utils.createLayout();

    utils.setViewNodeMarginAndSize(inner, 10, 10, 10);
    utils.setViewNodeMarginAndSize(layout1, 10, 10, 10);
    utils.setViewNodeMarginAndSize(layout2, 10, 10, 10);

    root->appendChild(utils.getViewTransactionScope(), inner);
    inner->appendChild(utils.getViewTransactionScope(), layout1);
    inner->appendChild(utils.getViewTransactionScope(), layout2);

    root->performLayout(utils.getViewTransactionScope(), Size(100, 100), LayoutDirectionLTR);

    inner->setAccessibilityNavigation(AccessibilityNavigationLeaf);
    layout1->setAccessibilityNavigation(AccessibilityNavigationLeaf);
    layout2->setAccessibilityNavigation(AccessibilityNavigationLeaf);

    auto childrenRoot = root->getAccessibilityChildrenRecursive();
    ASSERT_EQ(childrenRoot.size(), 1ul);
    ASSERT_EQ(childrenRoot[0], inner.get());

    auto childrenInner = inner->getAccessibilityChildrenRecursive();
    ASSERT_EQ(childrenInner.size(), 0ul);
}

TEST(ViewNodeAccessibility, isAccessibilityChildrenRecursiveWithIgnoredAndLeafs) {
    ViewNodeTestsDependencies utils;

    auto root = utils.createLayout();
    auto inner = utils.createLayout();
    auto layout1 = utils.createLayout();
    auto layout2 = utils.createLayout();

    utils.setViewNodeAttribute(layout1, "accessibilityId", Value(STRING_LITERAL("action-button")));

    utils.setViewNodeMarginAndSize(inner, 10, 10, 10);
    utils.setViewNodeMarginAndSize(layout1, 10, 10, 10);
    utils.setViewNodeMarginAndSize(layout2, 10, 10, 10);

    root->appendChild(utils.getViewTransactionScope(), inner);
    inner->appendChild(utils.getViewTransactionScope(), layout1);
    inner->appendChild(utils.getViewTransactionScope(), layout2);

    root->performLayout(utils.getViewTransactionScope(), Size(100, 100), LayoutDirectionLTR);

    inner->setAccessibilityNavigation(AccessibilityNavigationIgnored);
    layout1->setAccessibilityNavigation(AccessibilityNavigationLeaf);
    layout2->setAccessibilityNavigation(AccessibilityNavigationLeaf);

    auto childrenRoot = root->getAccessibilityChildrenRecursive();
    ASSERT_EQ(childrenRoot.size(), 0ul);

    auto childrenInner = inner->getAccessibilityChildrenRecursive();
    ASSERT_EQ(childrenInner.size(), 0ul);
}

TEST(ViewNodeAccessibility, isAccessibilityChildrenRecursiveWithCoverAndLeafs) {
    ViewNodeTestsDependencies utils;

    auto root = utils.createLayout();
    auto inner = utils.createLayout();
    auto layout1 = utils.createLayout();
    auto layout2 = utils.createLayout();

    utils.setViewNodeMarginAndSize(inner, 10, 10, 10);
    utils.setViewNodeMarginAndSize(layout1, 10, 10, 10);
    utils.setViewNodeMarginAndSize(layout2, 10, 10, 10);

    root->appendChild(utils.getViewTransactionScope(), inner);
    inner->appendChild(utils.getViewTransactionScope(), layout1);
    inner->appendChild(utils.getViewTransactionScope(), layout2);

    root->performLayout(utils.getViewTransactionScope(), Size(100, 100), LayoutDirectionLTR);

    inner->setAccessibilityNavigation(AccessibilityNavigationCover);
    layout1->setAccessibilityNavigation(AccessibilityNavigationLeaf);
    layout2->setAccessibilityNavigation(AccessibilityNavigationLeaf);

    auto childrenRoot = root->getAccessibilityChildrenRecursive();
    ASSERT_EQ(childrenRoot.size(), 3ul);
    ASSERT_EQ(childrenRoot[0], layout1.get());
    ASSERT_EQ(childrenRoot[1], layout2.get());
    ASSERT_EQ(childrenRoot[2], inner.get());
}

TEST(ViewNodeAccessibility, isAccessibilityChildrenRecursiveWithCoverAndCovers) {
    ViewNodeTestsDependencies utils;

    auto root = utils.createLayout();
    auto inner = utils.createLayout();
    auto layout1 = utils.createLayout();
    auto layout2 = utils.createLayout();

    utils.setViewNodeMarginAndSize(inner, 10, 10, 10);
    utils.setViewNodeMarginAndSize(layout1, 10, 10, 10);
    utils.setViewNodeMarginAndSize(layout2, 10, 10, 10);

    root->appendChild(utils.getViewTransactionScope(), inner);
    inner->appendChild(utils.getViewTransactionScope(), layout1);
    inner->appendChild(utils.getViewTransactionScope(), layout2);

    root->performLayout(utils.getViewTransactionScope(), Size(100, 100), LayoutDirectionLTR);

    inner->setAccessibilityNavigation(AccessibilityNavigationCover);
    layout1->setAccessibilityNavigation(AccessibilityNavigationCover);
    layout2->setAccessibilityNavigation(AccessibilityNavigationCover);

    auto childrenRoot = root->getAccessibilityChildrenRecursive();
    ASSERT_EQ(childrenRoot.size(), 3ul);
    ASSERT_EQ(childrenRoot[0], layout1.get());
    ASSERT_EQ(childrenRoot[1], layout2.get());
    ASSERT_EQ(childrenRoot[2], inner.get());
}

TEST(ViewNodeAccessibility, isAccessibilityChildrenRecursiveWithGroupAndLeafs) {
    ViewNodeTestsDependencies utils;

    auto root = utils.createLayout();
    auto inner = utils.createLayout();
    auto layout1 = utils.createLayout();
    auto layout2 = utils.createLayout();

    utils.setViewNodeMarginAndSize(inner, 10, 10, 10);
    utils.setViewNodeMarginAndSize(layout1, 10, 10, 10);
    utils.setViewNodeMarginAndSize(layout2, 10, 10, 10);

    root->appendChild(utils.getViewTransactionScope(), inner);
    inner->appendChild(utils.getViewTransactionScope(), layout1);
    inner->appendChild(utils.getViewTransactionScope(), layout2);

    root->performLayout(utils.getViewTransactionScope(), Size(100, 100), LayoutDirectionLTR);

    inner->setAccessibilityNavigation(AccessibilityNavigationGroup);
    layout1->setAccessibilityNavigation(AccessibilityNavigationLeaf);
    layout2->setAccessibilityNavigation(AccessibilityNavigationLeaf);

    auto childrenRoot = root->getAccessibilityChildrenRecursive();
    ASSERT_EQ(childrenRoot.size(), 1ul);
    ASSERT_EQ(childrenRoot[0], inner.get());

    auto childrenInner = inner->getAccessibilityChildrenRecursive();
    ASSERT_EQ(childrenInner.size(), 2ul);
    ASSERT_EQ(childrenInner[0], layout1.get());
    ASSERT_EQ(childrenInner[1], layout2.get());
}

TEST(ViewNodeAccessibility, isAccessibilityChildrenRecursiveWithGroupAndGroups) {
    ViewNodeTestsDependencies utils;

    auto root = utils.createLayout();
    auto inner = utils.createLayout();
    auto layout1 = utils.createLayout();
    auto layout2 = utils.createLayout();

    utils.setViewNodeMarginAndSize(inner, 10, 10, 10);
    utils.setViewNodeMarginAndSize(layout1, 10, 10, 10);
    utils.setViewNodeMarginAndSize(layout2, 10, 10, 10);

    root->appendChild(utils.getViewTransactionScope(), inner);
    inner->appendChild(utils.getViewTransactionScope(), layout1);
    inner->appendChild(utils.getViewTransactionScope(), layout2);

    root->performLayout(utils.getViewTransactionScope(), Size(100, 100), LayoutDirectionLTR);

    inner->setAccessibilityNavigation(AccessibilityNavigationGroup);
    layout1->setAccessibilityNavigation(AccessibilityNavigationGroup);
    layout2->setAccessibilityNavigation(AccessibilityNavigationGroup);

    auto childrenRoot = root->getAccessibilityChildrenRecursive();
    ASSERT_EQ(childrenRoot.size(), 1ul);
    ASSERT_EQ(childrenRoot[0], inner.get());

    auto childrenInner = inner->getAccessibilityChildrenRecursive();
    ASSERT_EQ(childrenInner.size(), 2ul);
    ASSERT_EQ(childrenInner[0], layout1.get());
    ASSERT_EQ(childrenInner[1], layout2.get());
}

TEST(ViewNodeAccessibility, isAccessibilityChildrenRecursiveWithCoverAndIgnoreds) {
    ViewNodeTestsDependencies utils;

    auto root = utils.createLayout();
    auto inner = utils.createLayout();
    auto layout1 = utils.createLayout();
    auto layout2 = utils.createLayout();

    utils.setViewNodeMarginAndSize(inner, 10, 10, 10);
    utils.setViewNodeMarginAndSize(layout1, 10, 10, 10);
    utils.setViewNodeMarginAndSize(layout2, 10, 10, 10);

    root->appendChild(utils.getViewTransactionScope(), inner);
    inner->appendChild(utils.getViewTransactionScope(), layout1);
    inner->appendChild(utils.getViewTransactionScope(), layout2);

    root->performLayout(utils.getViewTransactionScope(), Size(100, 100), LayoutDirectionLTR);

    inner->setAccessibilityNavigation(AccessibilityNavigationCover);
    layout1->setAccessibilityNavigation(AccessibilityNavigationIgnored);
    layout2->setAccessibilityNavigation(AccessibilityNavigationIgnored);

    auto childrenRoot = root->getAccessibilityChildrenRecursive();
    ASSERT_EQ(childrenRoot.size(), 1ul);
    ASSERT_EQ(childrenRoot[0], inner.get());

    auto childrenInner = inner->getAccessibilityChildrenRecursive();
    ASSERT_EQ(childrenInner.size(), 0ul);
}

TEST(ViewNodeAccessibility, isAccessibilityChildrenRecursiveWithGroupAndIgnoreds) {
    ViewNodeTestsDependencies utils;

    auto root = utils.createLayout();
    auto inner = utils.createLayout();
    auto layout1 = utils.createLayout();
    auto layout2 = utils.createLayout();

    utils.setViewNodeMarginAndSize(inner, 10, 10, 10);
    utils.setViewNodeMarginAndSize(layout1, 10, 10, 10);
    utils.setViewNodeMarginAndSize(layout2, 10, 10, 10);

    root->appendChild(utils.getViewTransactionScope(), inner);
    inner->appendChild(utils.getViewTransactionScope(), layout1);
    inner->appendChild(utils.getViewTransactionScope(), layout2);

    root->performLayout(utils.getViewTransactionScope(), Size(100, 100), LayoutDirectionLTR);

    inner->setAccessibilityNavigation(AccessibilityNavigationGroup);
    layout1->setAccessibilityNavigation(AccessibilityNavigationIgnored);
    layout2->setAccessibilityNavigation(AccessibilityNavigationIgnored);

    auto childrenRoot = root->getAccessibilityChildrenRecursive();
    ASSERT_EQ(childrenRoot.size(), 1ul);
    ASSERT_EQ(childrenRoot[0], inner.get());

    auto childrenInner = inner->getAccessibilityChildrenRecursive();
    ASSERT_EQ(childrenInner.size(), 0ul);
}

TEST(ViewNodeAccessibility, isAccessibilityChildrenRecursiveWithPassthroughAndIgnoreds) {
    ViewNodeTestsDependencies utils;

    auto root = utils.createLayout();
    auto inner = utils.createLayout();
    auto layout1 = utils.createLayout();
    auto layout2 = utils.createLayout();

    utils.setViewNodeMarginAndSize(inner, 10, 10, 10);
    utils.setViewNodeMarginAndSize(layout1, 10, 10, 10);
    utils.setViewNodeMarginAndSize(layout2, 10, 10, 10);

    root->appendChild(utils.getViewTransactionScope(), inner);
    inner->appendChild(utils.getViewTransactionScope(), layout1);
    inner->appendChild(utils.getViewTransactionScope(), layout2);

    root->performLayout(utils.getViewTransactionScope(), Size(100, 100), LayoutDirectionLTR);

    inner->setAccessibilityNavigation(AccessibilityNavigationPassthrough);
    layout1->setAccessibilityNavigation(AccessibilityNavigationIgnored);
    layout2->setAccessibilityNavigation(AccessibilityNavigationIgnored);

    auto childrenRoot = root->getAccessibilityChildrenRecursive();
    ASSERT_EQ(childrenRoot.size(), 0ul);

    auto childrenInner = inner->getAccessibilityChildrenRecursive();
    ASSERT_EQ(childrenInner.size(), 0ul);
}

TEST(ViewNodeAccessibility, hitTestAccessibilityChildrenPassthroughAndLeafsLTR) {
    ViewNodeTestsDependencies utils;

    auto root = utils.createLayout();
    auto inner = utils.createLayout();
    auto layout1 = utils.createLayout();
    auto layout2 = utils.createLayout();

    utils.setViewNodeAttribute(inner, "flexDirection", Value(STRING_LITERAL("row")));

    utils.setViewNodeMarginAndSize(root, 0, 100, 100);
    utils.setViewNodeMarginAndSize(inner, 10, 80, 80);
    utils.setViewNodeMarginAndSize(layout1, 10, 20, 20);
    utils.setViewNodeMarginAndSize(layout2, 10, 20, 20);

    root->appendChild(utils.getViewTransactionScope(), inner);
    inner->appendChild(utils.getViewTransactionScope(), layout1);
    inner->appendChild(utils.getViewTransactionScope(), layout2);

    root->performLayout(utils.getViewTransactionScope(), Size(100, 100), LayoutDirectionLTR);

    inner->setAccessibilityNavigation(AccessibilityNavigationPassthrough);
    layout1->setAccessibilityNavigation(AccessibilityNavigationLeaf);
    layout2->setAccessibilityNavigation(AccessibilityNavigationLeaf);

    ASSERT_EQ(root->hitTestAccessibilityChildren(Point(30, 30)), layout1.get());
}

TEST(ViewNodeAccessibility, hitTestAccessibilityChildrenPassthroughAndLeafsRTL) {
    ViewNodeTestsDependencies utils;

    auto root = utils.createLayout();
    auto inner = utils.createLayout();
    auto layout1 = utils.createLayout();
    auto layout2 = utils.createLayout();

    utils.setViewNodeAttribute(inner, "flexDirection", Value(STRING_LITERAL("row")));

    utils.setViewNodeMarginAndSize(root, 0, 100, 100);
    utils.setViewNodeMarginAndSize(inner, 10, 80, 80);
    utils.setViewNodeMarginAndSize(layout1, 10, 20, 20);
    utils.setViewNodeMarginAndSize(layout2, 10, 20, 20);

    root->appendChild(utils.getViewTransactionScope(), inner);
    inner->appendChild(utils.getViewTransactionScope(), layout1);
    inner->appendChild(utils.getViewTransactionScope(), layout2);

    root->performLayout(utils.getViewTransactionScope(), Size(100, 100), LayoutDirectionRTL);

    inner->setAccessibilityNavigation(AccessibilityNavigationPassthrough);
    layout1->setAccessibilityNavigation(AccessibilityNavigationLeaf);
    layout2->setAccessibilityNavigation(AccessibilityNavigationLeaf);

    ASSERT_EQ(root->hitTestAccessibilityChildren(Point(30, 30)), layout2.get());
}

TEST(ViewNodeAccessibility, hitTestAccessibilityChildrenPassthroughAndPassthroughs) {
    ViewNodeTestsDependencies utils;

    auto root = utils.createLayout();
    auto inner = utils.createLayout();
    auto layout1 = utils.createLayout();
    auto layout2 = utils.createLayout();

    utils.setViewNodeMarginAndSize(root, 0, 100, 100);
    utils.setViewNodeMarginAndSize(inner, 10, 80, 80);
    utils.setViewNodeMarginAndSize(layout1, 10, 20, 20);
    utils.setViewNodeMarginAndSize(layout2, 10, 20, 20);

    root->appendChild(utils.getViewTransactionScope(), inner);
    inner->appendChild(utils.getViewTransactionScope(), layout1);
    inner->appendChild(utils.getViewTransactionScope(), layout2);

    root->performLayout(utils.getViewTransactionScope(), Size(100, 100), LayoutDirectionLTR);

    inner->setAccessibilityNavigation(AccessibilityNavigationPassthrough);
    layout1->setAccessibilityNavigation(AccessibilityNavigationPassthrough);
    layout2->setAccessibilityNavigation(AccessibilityNavigationPassthrough);

    ASSERT_EQ(root->hitTestAccessibilityChildren(Point(30, 30)), nullptr);
}

TEST(ViewNodeAccessibility, hitTestAccessibilityChildrenLeafAndLeafs) {
    ViewNodeTestsDependencies utils;

    auto root = utils.createLayout();
    auto inner = utils.createLayout();
    auto layout1 = utils.createLayout();
    auto layout2 = utils.createLayout();

    utils.setViewNodeMarginAndSize(root, 0, 100, 100);
    utils.setViewNodeMarginAndSize(inner, 10, 80, 80);
    utils.setViewNodeMarginAndSize(layout1, 10, 20, 20);
    utils.setViewNodeMarginAndSize(layout2, 10, 20, 20);

    root->appendChild(utils.getViewTransactionScope(), inner);
    inner->appendChild(utils.getViewTransactionScope(), layout1);
    inner->appendChild(utils.getViewTransactionScope(), layout2);

    root->performLayout(utils.getViewTransactionScope(), Size(100, 100), LayoutDirectionLTR);

    inner->setAccessibilityNavigation(AccessibilityNavigationLeaf);
    layout1->setAccessibilityNavigation(AccessibilityNavigationLeaf);
    layout2->setAccessibilityNavigation(AccessibilityNavigationLeaf);

    ASSERT_EQ(root->hitTestAccessibilityChildren(Point(30, 30)), inner.get());
}

TEST(ViewNodeAccessibility, hitTestAccessibilityChildrenGroupAndGroups) {
    ViewNodeTestsDependencies utils;

    auto root = utils.createLayout();
    auto inner = utils.createLayout();
    auto layout1 = utils.createLayout();
    auto layout2 = utils.createLayout();

    utils.setViewNodeMarginAndSize(root, 0, 100, 100);
    utils.setViewNodeMarginAndSize(inner, 10, 80, 80);
    utils.setViewNodeMarginAndSize(layout1, 10, 20, 20);
    utils.setViewNodeMarginAndSize(layout2, 10, 20, 20);

    root->appendChild(utils.getViewTransactionScope(), inner);
    inner->appendChild(utils.getViewTransactionScope(), layout1);
    inner->appendChild(utils.getViewTransactionScope(), layout2);

    root->performLayout(utils.getViewTransactionScope(), Size(100, 100), LayoutDirectionLTR);

    inner->setAccessibilityNavigation(AccessibilityNavigationGroup);
    layout1->setAccessibilityNavigation(AccessibilityNavigationGroup);
    layout2->setAccessibilityNavigation(AccessibilityNavigationGroup);

    ASSERT_EQ(root->hitTestAccessibilityChildren(Point(30, 30)), layout1.get());
}

TEST(ViewNodeAccessibility, hitTestAccessibilityChildrenPassthroughAndCovers) {
    ViewNodeTestsDependencies utils;

    auto root = utils.createLayout();
    auto inner = utils.createLayout();
    auto layout1 = utils.createLayout();
    auto layout2 = utils.createLayout();

    utils.setViewNodeMarginAndSize(root, 0, 100, 100);
    utils.setViewNodeMarginAndSize(inner, 10, 80, 80);
    utils.setViewNodeMarginAndSize(layout1, 10, 20, 20);
    utils.setViewNodeMarginAndSize(layout2, 10, 20, 20);

    root->appendChild(utils.getViewTransactionScope(), inner);
    inner->appendChild(utils.getViewTransactionScope(), layout1);
    inner->appendChild(utils.getViewTransactionScope(), layout2);

    root->performLayout(utils.getViewTransactionScope(), Size(100, 100), LayoutDirectionLTR);

    inner->setAccessibilityNavigation(AccessibilityNavigationPassthrough);
    layout1->setAccessibilityNavigation(AccessibilityNavigationCover);
    layout2->setAccessibilityNavigation(AccessibilityNavigationCover);

    ASSERT_EQ(root->hitTestAccessibilityChildren(Point(30, 30)), layout1.get());
}

TEST(ViewNodeAccessibility, hitTestAccessibilityChildrenIgnoredAndLeafs) {
    ViewNodeTestsDependencies utils;

    auto root = utils.createLayout();
    auto inner = utils.createLayout();
    auto layout1 = utils.createLayout();
    auto layout2 = utils.createLayout();

    utils.setViewNodeMarginAndSize(root, 0, 100, 100);
    utils.setViewNodeMarginAndSize(inner, 10, 80, 80);
    utils.setViewNodeMarginAndSize(layout1, 10, 20, 20);
    utils.setViewNodeMarginAndSize(layout2, 10, 20, 20);

    root->appendChild(utils.getViewTransactionScope(), inner);
    inner->appendChild(utils.getViewTransactionScope(), layout1);
    inner->appendChild(utils.getViewTransactionScope(), layout2);

    root->performLayout(utils.getViewTransactionScope(), Size(100, 100), LayoutDirectionLTR);

    inner->setAccessibilityNavigation(AccessibilityNavigationIgnored);
    layout1->setAccessibilityNavigation(AccessibilityNavigationLeaf);
    layout2->setAccessibilityNavigation(AccessibilityNavigationLeaf);

    ASSERT_EQ(root->hitTestAccessibilityChildren(Point(30, 30)), nullptr);
}

TEST(ViewNodeAccessibility, hitTestAccessibilityChildrenCoverAndIgnoreds) {
    ViewNodeTestsDependencies utils;

    auto root = utils.createLayout();
    auto inner = utils.createLayout();
    auto layout1 = utils.createLayout();
    auto layout2 = utils.createLayout();

    utils.setViewNodeMarginAndSize(root, 0, 100, 100);
    utils.setViewNodeMarginAndSize(inner, 10, 80, 80);
    utils.setViewNodeMarginAndSize(layout1, 10, 20, 20);
    utils.setViewNodeMarginAndSize(layout2, 10, 20, 20);

    root->appendChild(utils.getViewTransactionScope(), inner);
    inner->appendChild(utils.getViewTransactionScope(), layout1);
    inner->appendChild(utils.getViewTransactionScope(), layout2);

    root->performLayout(utils.getViewTransactionScope(), Size(100, 100), LayoutDirectionLTR);

    inner->setAccessibilityNavigation(AccessibilityNavigationCover);
    layout1->setAccessibilityNavigation(AccessibilityNavigationIgnored);
    layout2->setAccessibilityNavigation(AccessibilityNavigationIgnored);

    ASSERT_EQ(root->hitTestAccessibilityChildren(Point(30, 30)), inner.get());
}

TEST(ViewNodeAccessibility, hitTestAccessibilityChildrenCoverAndLeafs) {
    ViewNodeTestsDependencies utils;

    auto root = utils.createLayout();
    auto inner = utils.createLayout();
    auto layout1 = utils.createLayout();
    auto layout2 = utils.createLayout();

    utils.setViewNodeMarginAndSize(root, 0, 100, 100);
    utils.setViewNodeMarginAndSize(inner, 10, 80, 80);
    utils.setViewNodeMarginAndSize(layout1, 10, 20, 20);
    utils.setViewNodeMarginAndSize(layout2, 10, 20, 20);

    root->appendChild(utils.getViewTransactionScope(), inner);
    inner->appendChild(utils.getViewTransactionScope(), layout1);
    inner->appendChild(utils.getViewTransactionScope(), layout2);

    root->performLayout(utils.getViewTransactionScope(), Size(100, 100), LayoutDirectionLTR);

    inner->setAccessibilityNavigation(AccessibilityNavigationCover);
    layout1->setAccessibilityNavigation(AccessibilityNavigationLeaf);
    layout2->setAccessibilityNavigation(AccessibilityNavigationLeaf);

    ASSERT_EQ(root->hitTestAccessibilityChildren(Point(30, 30)), layout1.get());
}

TEST(ViewNodeAccessibility, computeVisualFrameInRootLTR) {
    ViewNodeTestsDependencies utils;

    auto root = utils.createLayout();
    auto inner = utils.createLayout();
    auto layout1 = utils.createLayout();
    auto layout2 = utils.createLayout();

    utils.setViewNodeAttribute(inner, "flexDirection", Value(STRING_LITERAL("row")));

    utils.setViewNodeMarginAndSize(root, 0, 100, 100);
    utils.setViewNodeMarginAndSize(inner, 10, 80, 80);
    utils.setViewNodeMarginAndSize(layout1, 10, 20, 20);
    utils.setViewNodeMarginAndSize(layout2, 10, 20, 20);

    root->appendChild(utils.getViewTransactionScope(), inner);
    inner->appendChild(utils.getViewTransactionScope(), layout1);
    inner->appendChild(utils.getViewTransactionScope(), layout2);

    root->performLayout(utils.getViewTransactionScope(), Size(100, 100), LayoutDirectionLTR);

    ASSERT_EQ(root->computeVisualFrameInRoot(), Frame(0, 0, 100, 100));
    ASSERT_EQ(inner->computeVisualFrameInRoot(), Frame(10, 10, 80, 80));
    ASSERT_EQ(layout1->computeVisualFrameInRoot(), Frame(20, 20, 20, 20));
    ASSERT_EQ(layout2->computeVisualFrameInRoot(), Frame(60, 20, 20, 20));
}

TEST(ViewNodeAccessibility, computeVisualFrameInRootRTL) {
    ViewNodeTestsDependencies utils;

    auto root = utils.createLayout();
    auto inner = utils.createLayout();
    auto layout1 = utils.createLayout();
    auto layout2 = utils.createLayout();

    utils.setViewNodeAttribute(inner, "flexDirection", Value(STRING_LITERAL("row")));

    utils.setViewNodeMarginAndSize(root, 0, 100, 100);
    utils.setViewNodeMarginAndSize(inner, 10, 80, 80);
    utils.setViewNodeMarginAndSize(layout1, 10, 20, 20);
    utils.setViewNodeMarginAndSize(layout2, 10, 20, 20);

    root->appendChild(utils.getViewTransactionScope(), inner);
    inner->appendChild(utils.getViewTransactionScope(), layout1);
    inner->appendChild(utils.getViewTransactionScope(), layout2);

    root->performLayout(utils.getViewTransactionScope(), Size(100, 100), LayoutDirectionRTL);

    ASSERT_EQ(root->computeVisualFrameInRoot(), Frame(0, 0, 100, 100));
    ASSERT_EQ(inner->computeVisualFrameInRoot(), Frame(10, 10, 80, 80));
    ASSERT_EQ(layout1->computeVisualFrameInRoot(), Frame(60, 20, 20, 20));
    ASSERT_EQ(layout2->computeVisualFrameInRoot(), Frame(20, 20, 20, 20));
}

TEST(ViewNodeAccessibility, ensureFrameIsVisibleWithinParentScrollsSimpleVertical) {
    ViewNodeTestsDependencies utils;

    auto root = utils.createLayout();
    auto scrollVertical = utils.createScroll();
    auto layout1 = utils.createLayout();
    auto layout2 = utils.createLayout();
    auto layout3 = utils.createLayout();

    utils.setViewNodeMarginAndSize(root, 0, 100, 100);
    utils.setViewNodeMarginAndSize(scrollVertical, 10, 80, 80);
    utils.setViewNodeMarginAndSize(layout1, 10, 20, 20);
    utils.setViewNodeMarginAndSize(layout2, 10, 20, 20);
    utils.setViewNodeMarginAndSize(layout3, 10, 20, 20);

    root->appendChild(utils.getViewTransactionScope(), scrollVertical);
    scrollVertical->appendChild(utils.getViewTransactionScope(), layout1);
    scrollVertical->appendChild(utils.getViewTransactionScope(), layout2);
    scrollVertical->appendChild(utils.getViewTransactionScope(), layout3);

    root->performLayout(utils.getViewTransactionScope(), Size(100, 100), LayoutDirectionLTR);

    ASSERT_EQ(scrollVertical->getDirectionAgnosticScrollContentOffset(), Point(0, 0));
    layout3->ensureFrameIsVisibleWithinParentScrolls(utils.getViewTransactionScope(), false);
    ASSERT_EQ(scrollVertical->getDirectionAgnosticScrollContentOffset(), Point(0, 30));
    layout1->ensureFrameIsVisibleWithinParentScrolls(utils.getViewTransactionScope(), false);
    ASSERT_EQ(scrollVertical->getDirectionAgnosticScrollContentOffset(), Point(0, 10));
}

TEST(ViewNodeAccessibility, ensureFrameIsVisibleWithinParentScrollsSimpleHorizontalLTR) {
    ViewNodeTestsDependencies utils;

    auto root = utils.createLayout();
    auto scrollHorizontal = utils.createScroll();
    auto layout1 = utils.createLayout();
    auto layout2 = utils.createLayout();
    auto layout3 = utils.createLayout();

    utils.setViewNodeAttribute(scrollHorizontal, "flexDirection", Value(STRING_LITERAL("row")));

    utils.setViewNodeMarginAndSize(root, 0, 100, 100);
    utils.setViewNodeMarginAndSize(scrollHorizontal, 10, 80, 80);
    utils.setViewNodeMarginAndSize(layout1, 10, 20, 20);
    utils.setViewNodeMarginAndSize(layout2, 10, 20, 20);
    utils.setViewNodeMarginAndSize(layout3, 10, 20, 20);

    root->appendChild(utils.getViewTransactionScope(), scrollHorizontal);
    scrollHorizontal->appendChild(utils.getViewTransactionScope(), layout1);
    scrollHorizontal->appendChild(utils.getViewTransactionScope(), layout2);
    scrollHorizontal->appendChild(utils.getViewTransactionScope(), layout3);

    root->performLayout(utils.getViewTransactionScope(), Size(100, 100), LayoutDirectionLTR);

    ASSERT_EQ(scrollHorizontal->getDirectionAgnosticScrollContentOffset(), Point(0, 0));
    layout3->ensureFrameIsVisibleWithinParentScrolls(utils.getViewTransactionScope(), false);
    ASSERT_EQ(scrollHorizontal->getDirectionAgnosticScrollContentOffset(), Point(30, 0));
    layout1->ensureFrameIsVisibleWithinParentScrolls(utils.getViewTransactionScope(), false);
    ASSERT_EQ(scrollHorizontal->getDirectionAgnosticScrollContentOffset(), Point(10, 0));
}

TEST(ViewNodeAccessibility, ensureFrameIsVisibleWithinParentScrollsSimpleHorizontalRTL) {
    ViewNodeTestsDependencies utils;

    auto root = utils.createLayout();
    auto scrollHorizontal = utils.createScroll();
    auto layout1 = utils.createLayout();
    auto layout2 = utils.createLayout();
    auto layout3 = utils.createLayout();

    utils.setViewNodeAttribute(scrollHorizontal, "flexDirection", Value(STRING_LITERAL("row")));

    utils.setViewNodeMarginAndSize(root, 0, 100, 100);
    utils.setViewNodeMarginAndSize(scrollHorizontal, 10, 80, 80);
    utils.setViewNodeMarginAndSize(layout1, 10, 20, 20);
    utils.setViewNodeMarginAndSize(layout2, 10, 20, 20);
    utils.setViewNodeMarginAndSize(layout3, 10, 20, 20);

    root->appendChild(utils.getViewTransactionScope(), scrollHorizontal);
    scrollHorizontal->appendChild(utils.getViewTransactionScope(), layout1);
    scrollHorizontal->appendChild(utils.getViewTransactionScope(), layout2);
    scrollHorizontal->appendChild(utils.getViewTransactionScope(), layout3);

    root->performLayout(utils.getViewTransactionScope(), Size(100, 100), LayoutDirectionRTL);

    ASSERT_EQ(scrollHorizontal->getDirectionAgnosticScrollContentOffset(), Point(0, 0));
    layout3->ensureFrameIsVisibleWithinParentScrolls(utils.getViewTransactionScope(), false);
    ASSERT_EQ(scrollHorizontal->getDirectionAgnosticScrollContentOffset(), Point(30, 0));
    layout1->ensureFrameIsVisibleWithinParentScrolls(utils.getViewTransactionScope(), false);
    ASSERT_EQ(scrollHorizontal->getDirectionAgnosticScrollContentOffset(), Point(10, 0));
}

TEST(ViewNodeAccessibility, ensureFrameIsVisibleWithinParentScrollsNestedVerticalHorizontalLTR) {
    ViewNodeTestsDependencies utils;

    auto root = utils.createLayout();
    auto scrollVertical = utils.createScroll();
    auto scrollHorizontal = utils.createScroll();
    auto layout1 = utils.createLayout();
    auto layout2 = utils.createLayout();
    auto layout3 = utils.createLayout();

    utils.setViewNodeAttribute(scrollHorizontal, "flexDirection", Value(STRING_LITERAL("row")));

    utils.setViewNodeMarginAndSize(root, 0, 100, 100);
    utils.setViewNodeMarginAndSize(scrollVertical, 10, 80, 80);
    utils.setViewNodeAllMarginsAndSize(scrollHorizontal, 100, 40, 100, 40, 40, 40);
    utils.setViewNodeMarginAndSize(layout1, 10, 20, 20);
    utils.setViewNodeMarginAndSize(layout2, 10, 20, 20);
    utils.setViewNodeMarginAndSize(layout3, 10, 20, 20);

    root->appendChild(utils.getViewTransactionScope(), scrollVertical);
    scrollVertical->appendChild(utils.getViewTransactionScope(), scrollHorizontal);
    scrollHorizontal->appendChild(utils.getViewTransactionScope(), layout1);
    scrollHorizontal->appendChild(utils.getViewTransactionScope(), layout2);
    scrollHorizontal->appendChild(utils.getViewTransactionScope(), layout3);

    root->performLayout(utils.getViewTransactionScope(), Size(100, 100), LayoutDirectionLTR);

    ASSERT_EQ(scrollVertical->getDirectionAgnosticScrollContentOffset(), Point(0, 0));
    ASSERT_EQ(scrollHorizontal->getDirectionAgnosticScrollContentOffset(), Point(0, 0));
    layout3->ensureFrameIsVisibleWithinParentScrolls(utils.getViewTransactionScope(), false);
    ASSERT_EQ(scrollVertical->getDirectionAgnosticScrollContentOffset(), Point(0, 60));
    ASSERT_EQ(scrollHorizontal->getDirectionAgnosticScrollContentOffset(), Point(70, 0));
    layout1->ensureFrameIsVisibleWithinParentScrolls(utils.getViewTransactionScope(), false);
    ASSERT_EQ(scrollVertical->getDirectionAgnosticScrollContentOffset(), Point(0, 60));
    ASSERT_EQ(scrollHorizontal->getDirectionAgnosticScrollContentOffset(), Point(10, 0));
}

TEST(ViewNodeAccessibility, ensureFrameIsVisibleWithinParentScrollsNestedVerticalHorizontalRTL) {
    ViewNodeTestsDependencies utils;

    auto root = utils.createLayout();
    auto scrollVertical = utils.createScroll();
    auto scrollHorizontal = utils.createScroll();
    auto layout1 = utils.createLayout();
    auto layout2 = utils.createLayout();
    auto layout3 = utils.createLayout();

    utils.setViewNodeAttribute(scrollHorizontal, "flexDirection", Value(STRING_LITERAL("row")));

    utils.setViewNodeMarginAndSize(root, 0, 100, 100);
    utils.setViewNodeMarginAndSize(scrollVertical, 10, 80, 80);
    utils.setViewNodeAllMarginsAndSize(scrollHorizontal, 100, 40, 100, 40, 40, 40);
    utils.setViewNodeMarginAndSize(layout1, 10, 20, 20);
    utils.setViewNodeMarginAndSize(layout2, 10, 20, 20);
    utils.setViewNodeMarginAndSize(layout3, 10, 20, 20);

    root->appendChild(utils.getViewTransactionScope(), scrollVertical);
    scrollVertical->appendChild(utils.getViewTransactionScope(), scrollHorizontal);
    scrollHorizontal->appendChild(utils.getViewTransactionScope(), layout1);
    scrollHorizontal->appendChild(utils.getViewTransactionScope(), layout2);
    scrollHorizontal->appendChild(utils.getViewTransactionScope(), layout3);

    root->performLayout(utils.getViewTransactionScope(), Size(100, 100), LayoutDirectionRTL);

    ASSERT_EQ(scrollVertical->getDirectionAgnosticScrollContentOffset(), Point(0, 0));
    ASSERT_EQ(scrollHorizontal->getDirectionAgnosticScrollContentOffset(), Point(0, 0));
    layout3->ensureFrameIsVisibleWithinParentScrolls(utils.getViewTransactionScope(), false);
    ASSERT_EQ(scrollVertical->getDirectionAgnosticScrollContentOffset(), Point(0, 60));
    ASSERT_EQ(scrollHorizontal->getDirectionAgnosticScrollContentOffset(), Point(70, 0));
    layout1->ensureFrameIsVisibleWithinParentScrolls(utils.getViewTransactionScope(), false);
    ASSERT_EQ(scrollVertical->getDirectionAgnosticScrollContentOffset(), Point(0, 60));
    ASSERT_EQ(scrollHorizontal->getDirectionAgnosticScrollContentOffset(), Point(10, 0));
}

TEST(ViewNodeAccessibility, ensureFrameIsVisibleWithinParentScrollsNestedVerticalVertical) {
    ViewNodeTestsDependencies utils;

    auto root = utils.createLayout();
    auto scrollVertical1 = utils.createScroll();
    auto scrollVertical2 = utils.createScroll();
    auto layout1 = utils.createLayout();
    auto layout2 = utils.createLayout();
    auto layout3 = utils.createLayout();

    utils.setViewNodeMarginAndSize(root, 0, 100, 100);
    utils.setViewNodeMarginAndSize(scrollVertical1, 10, 80, 80);
    utils.setViewNodeAllMarginsAndSize(scrollVertical2, 100, 40, 100, 40, 40, 40);
    utils.setViewNodeMarginAndSize(layout1, 10, 20, 20);
    utils.setViewNodeMarginAndSize(layout2, 10, 20, 20);
    utils.setViewNodeMarginAndSize(layout3, 10, 20, 20);

    root->appendChild(utils.getViewTransactionScope(), scrollVertical1);
    scrollVertical1->appendChild(utils.getViewTransactionScope(), scrollVertical2);
    scrollVertical2->appendChild(utils.getViewTransactionScope(), layout1);
    scrollVertical2->appendChild(utils.getViewTransactionScope(), layout2);
    scrollVertical2->appendChild(utils.getViewTransactionScope(), layout3);

    root->performLayout(utils.getViewTransactionScope(), Size(100, 100), LayoutDirectionLTR);

    ASSERT_EQ(scrollVertical1->getDirectionAgnosticScrollContentOffset(), Point(0, 0));
    ASSERT_EQ(scrollVertical2->getDirectionAgnosticScrollContentOffset(), Point(0, 0));
    layout3->ensureFrameIsVisibleWithinParentScrolls(utils.getViewTransactionScope(), false);
    ASSERT_EQ(scrollVertical1->getDirectionAgnosticScrollContentOffset(), Point(0, 60));
    ASSERT_EQ(scrollVertical2->getDirectionAgnosticScrollContentOffset(), Point(0, 70));
    layout1->ensureFrameIsVisibleWithinParentScrolls(utils.getViewTransactionScope(), false);
    ASSERT_EQ(scrollVertical1->getDirectionAgnosticScrollContentOffset(), Point(0, 60));
    ASSERT_EQ(scrollVertical2->getDirectionAgnosticScrollContentOffset(), Point(0, 10));
}

TEST(ViewNodeAccessibility, ensureFrameIsVisibleWithinParentScrollsNestedAnimatedVerticalVertical) {
    ViewNodeTestsDependencies utils;

    auto root = utils.createLayout();
    auto scrollVertical1 = utils.createScroll();
    auto scrollVertical2 = utils.createScroll();
    auto layout1 = utils.createLayout();
    auto layout2 = utils.createLayout();
    auto layout3 = utils.createLayout();

    utils.setViewNodeMarginAndSize(root, 0, 100, 100);
    utils.setViewNodeMarginAndSize(scrollVertical1, 10, 80, 80);
    utils.setViewNodeAllMarginsAndSize(scrollVertical2, 100, 40, 100, 40, 40, 40);
    utils.setViewNodeMarginAndSize(layout1, 10, 20, 20);
    utils.setViewNodeMarginAndSize(layout2, 10, 20, 20);
    utils.setViewNodeMarginAndSize(layout3, 10, 20, 20);

    root->appendChild(utils.getViewTransactionScope(), scrollVertical1);
    scrollVertical1->appendChild(utils.getViewTransactionScope(), scrollVertical2);
    scrollVertical2->appendChild(utils.getViewTransactionScope(), layout1);
    scrollVertical2->appendChild(utils.getViewTransactionScope(), layout2);
    scrollVertical2->appendChild(utils.getViewTransactionScope(), layout3);

    root->performLayout(utils.getViewTransactionScope(), Size(100, 100), LayoutDirectionLTR);

    ASSERT_EQ(scrollVertical1->getDirectionAgnosticScrollContentOffset(), Point(0, 0));
    ASSERT_EQ(scrollVertical2->getDirectionAgnosticScrollContentOffset(), Point(0, 0));
    layout3->ensureFrameIsVisibleWithinParentScrolls(utils.getViewTransactionScope(), true);
    ASSERT_EQ(scrollVertical1->getDirectionAgnosticScrollContentOffset(), Point(0, 60));
    ASSERT_EQ(scrollVertical2->getDirectionAgnosticScrollContentOffset(), Point(0, 70));
    layout1->ensureFrameIsVisibleWithinParentScrolls(utils.getViewTransactionScope(), true);
    ASSERT_EQ(scrollVertical1->getDirectionAgnosticScrollContentOffset(), Point(0, 60));
    ASSERT_EQ(scrollVertical2->getDirectionAgnosticScrollContentOffset(), Point(0, 10));
}

TEST(ViewNodeAccessibility, scrollByOnePageVertical) {
    ViewNodeTestsDependencies utils;

    auto root = utils.createLayout();
    auto scrollVertical = utils.createScroll();
    auto layout1 = utils.createLayout();
    auto layout2 = utils.createLayout();
    auto layout3 = utils.createLayout();
    auto layout4 = utils.createLayout();
    auto layout5 = utils.createLayout();

    utils.setViewNodeMarginAndSize(root, 0, 100, 100);
    utils.setViewNodeMarginAndSize(scrollVertical, 10, 80, 80);
    utils.setViewNodeMarginAndSize(layout1, 10, 20, 20);
    utils.setViewNodeMarginAndSize(layout2, 10, 20, 20);
    utils.setViewNodeMarginAndSize(layout3, 10, 20, 20);
    utils.setViewNodeMarginAndSize(layout4, 10, 20, 20);
    utils.setViewNodeMarginAndSize(layout5, 10, 20, 20);

    root->appendChild(utils.getViewTransactionScope(), scrollVertical);
    scrollVertical->appendChild(utils.getViewTransactionScope(), layout1);
    scrollVertical->appendChild(utils.getViewTransactionScope(), layout2);
    scrollVertical->appendChild(utils.getViewTransactionScope(), layout3);
    scrollVertical->appendChild(utils.getViewTransactionScope(), layout4);
    scrollVertical->appendChild(utils.getViewTransactionScope(), layout5);

    root->performLayout(utils.getViewTransactionScope(), Size(100, 100), LayoutDirectionLTR);

    float scrollPageCurrent;
    float scrollPageMaximum;

    layout4->scrollByOnePage(
        utils.getViewTransactionScope(), ScrollDirectionTopToBottom, false, &scrollPageCurrent, &scrollPageMaximum);
    ASSERT_EQ(scrollVertical->getDirectionAgnosticScrollContentOffset(), Point(0, 80));
    ASSERT_EQ(scrollPageCurrent, 1);
    ASSERT_EQ(scrollPageMaximum, 1.5);

    layout4->scrollByOnePage(
        utils.getViewTransactionScope(), ScrollDirectionTopToBottom, false, &scrollPageCurrent, &scrollPageMaximum);
    ASSERT_EQ(scrollVertical->getDirectionAgnosticScrollContentOffset(), Point(0, 120));
    ASSERT_EQ(scrollPageCurrent, 1.5);
    ASSERT_EQ(scrollPageMaximum, 1.5);

    layout4->scrollByOnePage(
        utils.getViewTransactionScope(), ScrollDirectionBottomToTop, false, &scrollPageCurrent, &scrollPageMaximum);
    ASSERT_EQ(scrollVertical->getDirectionAgnosticScrollContentOffset(), Point(0, 40));
    ASSERT_EQ(scrollPageCurrent, 0.5);
    ASSERT_EQ(scrollPageMaximum, 1.5);

    layout4->scrollByOnePage(
        utils.getViewTransactionScope(), ScrollDirectionBottomToTop, false, &scrollPageCurrent, &scrollPageMaximum);
    ASSERT_EQ(scrollVertical->getDirectionAgnosticScrollContentOffset(), Point(0, 0));
    ASSERT_EQ(scrollPageCurrent, 0);
    ASSERT_EQ(scrollPageMaximum, 1.5);
}

TEST(ViewNodeAccessibility, scrollByOnePageHorizontalLTR) {
    ViewNodeTestsDependencies utils;

    auto root = utils.createLayout();
    auto scrollHorizontal = utils.createScroll();
    auto layout1 = utils.createLayout();
    auto layout2 = utils.createLayout();
    auto layout3 = utils.createLayout();
    auto layout4 = utils.createLayout();
    auto layout5 = utils.createLayout();

    utils.setViewNodeAttribute(scrollHorizontal, "flexDirection", Value(STRING_LITERAL("row")));

    utils.setViewNodeMarginAndSize(root, 0, 100, 100);
    utils.setViewNodeMarginAndSize(scrollHorizontal, 10, 80, 80);
    utils.setViewNodeMarginAndSize(layout1, 10, 20, 20);
    utils.setViewNodeMarginAndSize(layout2, 10, 20, 20);
    utils.setViewNodeMarginAndSize(layout3, 10, 20, 20);
    utils.setViewNodeMarginAndSize(layout4, 10, 20, 20);
    utils.setViewNodeMarginAndSize(layout5, 10, 20, 20);

    root->appendChild(utils.getViewTransactionScope(), scrollHorizontal);
    scrollHorizontal->appendChild(utils.getViewTransactionScope(), layout1);
    scrollHorizontal->appendChild(utils.getViewTransactionScope(), layout2);
    scrollHorizontal->appendChild(utils.getViewTransactionScope(), layout3);
    scrollHorizontal->appendChild(utils.getViewTransactionScope(), layout4);
    scrollHorizontal->appendChild(utils.getViewTransactionScope(), layout5);

    root->performLayout(utils.getViewTransactionScope(), Size(100, 100), LayoutDirectionLTR);

    float scrollPageCurrent;
    float scrollPageMaximum;

    layout4->scrollByOnePage(
        utils.getViewTransactionScope(), ScrollDirectionLeftToRight, false, &scrollPageCurrent, &scrollPageMaximum);
    ASSERT_EQ(scrollHorizontal->getDirectionAgnosticScrollContentOffset(), Point(80, 0));
    ASSERT_EQ(scrollPageCurrent, 1);
    ASSERT_EQ(scrollPageMaximum, 1.5);

    layout4->scrollByOnePage(
        utils.getViewTransactionScope(), ScrollDirectionLeftToRight, false, &scrollPageCurrent, &scrollPageMaximum);
    ASSERT_EQ(scrollHorizontal->getDirectionAgnosticScrollContentOffset(), Point(120, 0));
    ASSERT_EQ(scrollPageCurrent, 1.5);
    ASSERT_EQ(scrollPageMaximum, 1.5);

    layout4->scrollByOnePage(
        utils.getViewTransactionScope(), ScrollDirectionRightToLeft, false, &scrollPageCurrent, &scrollPageMaximum);
    ASSERT_EQ(scrollHorizontal->getDirectionAgnosticScrollContentOffset(), Point(40, 0));
    ASSERT_EQ(scrollPageCurrent, 0.5);
    ASSERT_EQ(scrollPageMaximum, 1.5);

    layout4->scrollByOnePage(
        utils.getViewTransactionScope(), ScrollDirectionRightToLeft, false, &scrollPageCurrent, &scrollPageMaximum);
    ASSERT_EQ(scrollHorizontal->getDirectionAgnosticScrollContentOffset(), Point(0, 0));
    ASSERT_EQ(scrollPageCurrent, 0);
    ASSERT_EQ(scrollPageMaximum, 1.5);
}

TEST(ViewNodeAccessibility, scrollByOnePageHorizontalRTL) {
    ViewNodeTestsDependencies utils;

    auto root = utils.createLayout();
    auto scrollHorizontal = utils.createScroll();
    auto layout1 = utils.createLayout();
    auto layout2 = utils.createLayout();
    auto layout3 = utils.createLayout();
    auto layout4 = utils.createLayout();
    auto layout5 = utils.createLayout();

    utils.setViewNodeAttribute(scrollHorizontal, "flexDirection", Value(STRING_LITERAL("row")));

    utils.setViewNodeMarginAndSize(root, 0, 100, 100);
    utils.setViewNodeMarginAndSize(scrollHorizontal, 10, 80, 80);
    utils.setViewNodeMarginAndSize(layout1, 10, 20, 20);
    utils.setViewNodeMarginAndSize(layout2, 10, 20, 20);
    utils.setViewNodeMarginAndSize(layout3, 10, 20, 20);
    utils.setViewNodeMarginAndSize(layout4, 10, 20, 20);
    utils.setViewNodeMarginAndSize(layout5, 10, 20, 20);

    root->appendChild(utils.getViewTransactionScope(), scrollHorizontal);
    scrollHorizontal->appendChild(utils.getViewTransactionScope(), layout1);
    scrollHorizontal->appendChild(utils.getViewTransactionScope(), layout2);
    scrollHorizontal->appendChild(utils.getViewTransactionScope(), layout3);
    scrollHorizontal->appendChild(utils.getViewTransactionScope(), layout4);
    scrollHorizontal->appendChild(utils.getViewTransactionScope(), layout5);

    root->performLayout(utils.getViewTransactionScope(), Size(100, 100), LayoutDirectionRTL);

    float scrollPageCurrent;
    float scrollPageMaximum;

    layout4->scrollByOnePage(
        utils.getViewTransactionScope(), ScrollDirectionRightToLeft, false, &scrollPageCurrent, &scrollPageMaximum);
    ASSERT_EQ(scrollHorizontal->getDirectionAgnosticScrollContentOffset(), Point(80, 0));
    ASSERT_EQ(scrollPageCurrent, 1);
    ASSERT_EQ(scrollPageMaximum, 1.5);

    layout4->scrollByOnePage(
        utils.getViewTransactionScope(), ScrollDirectionRightToLeft, false, &scrollPageCurrent, &scrollPageMaximum);
    ASSERT_EQ(scrollHorizontal->getDirectionAgnosticScrollContentOffset(), Point(120, 0));
    ASSERT_EQ(scrollPageCurrent, 1.5);
    ASSERT_EQ(scrollPageMaximum, 1.5);

    layout4->scrollByOnePage(
        utils.getViewTransactionScope(), ScrollDirectionLeftToRight, false, &scrollPageCurrent, &scrollPageMaximum);
    ASSERT_EQ(scrollHorizontal->getDirectionAgnosticScrollContentOffset(), Point(40, 0));
    ASSERT_EQ(scrollPageCurrent, 0.5);
    ASSERT_EQ(scrollPageMaximum, 1.5);

    layout4->scrollByOnePage(
        utils.getViewTransactionScope(), ScrollDirectionLeftToRight, false, &scrollPageCurrent, &scrollPageMaximum);
    ASSERT_EQ(scrollHorizontal->getDirectionAgnosticScrollContentOffset(), Point(0, 0));
    ASSERT_EQ(scrollPageCurrent, 0);
    ASSERT_EQ(scrollPageMaximum, 1.5);
}

TEST(ViewNodeAccessibility, getPageNumberForContentOffsetVertical) {
    ViewNodeTestsDependencies utils;

    auto root = utils.createLayout();
    auto scrollVertical = utils.createScroll();
    utils.setViewNodeMarginAndSize(scrollVertical, 10, 80, 80);
    root->appendChild(utils.getViewTransactionScope(), scrollVertical);
    root->performLayout(utils.getViewTransactionScope(), Size(100, 100), LayoutDirectionLTR);

    ASSERT_EQ(scrollVertical->getPageNumberForContentOffset(Point(0, -80)), -1);
    ASSERT_EQ(scrollVertical->getPageNumberForContentOffset(Point(0, 0)), 0);
    ASSERT_EQ(scrollVertical->getPageNumberForContentOffset(Point(0, 80)), 1);
    ASSERT_EQ(scrollVertical->getPageNumberForContentOffset(Point(0, 120)), 1.5);
}

TEST(ViewNodeAccessibility, getPageNumberForContentOffsetHorizontalLTR) {
    ViewNodeTestsDependencies utils;

    auto root = utils.createLayout();
    auto scrollHorizontal = utils.createScroll();
    utils.setViewNodeAttribute(scrollHorizontal, "flexDirection", Value(STRING_LITERAL("row")));
    utils.setViewNodeMarginAndSize(scrollHorizontal, 10, 80, 80);
    root->appendChild(utils.getViewTransactionScope(), scrollHorizontal);
    root->performLayout(utils.getViewTransactionScope(), Size(100, 100), LayoutDirectionLTR);

    ASSERT_EQ(scrollHorizontal->getPageNumberForContentOffset(Point(-80, 0)), -1);
    ASSERT_EQ(scrollHorizontal->getPageNumberForContentOffset(Point(0, 0)), 0);
    ASSERT_EQ(scrollHorizontal->getPageNumberForContentOffset(Point(80, 0)), 1);
    ASSERT_EQ(scrollHorizontal->getPageNumberForContentOffset(Point(120, 0)), 1.5);
}

TEST(ViewNodeAccessibility, getPageNumberForContentOffsetHorizontalRTL) {
    ViewNodeTestsDependencies utils;

    auto root = utils.createLayout();
    auto scrollHorizontal = utils.createScroll();
    utils.setViewNodeAttribute(scrollHorizontal, "flexDirection", Value(STRING_LITERAL("row")));
    utils.setViewNodeMarginAndSize(scrollHorizontal, 10, 80, 80);
    root->appendChild(utils.getViewTransactionScope(), scrollHorizontal);
    root->performLayout(utils.getViewTransactionScope(), Size(100, 100), LayoutDirectionRTL);

    ASSERT_EQ(scrollHorizontal->getPageNumberForContentOffset(Point(-80, 0)), -1);
    ASSERT_EQ(scrollHorizontal->getPageNumberForContentOffset(Point(0, 0)), 0);
    ASSERT_EQ(scrollHorizontal->getPageNumberForContentOffset(Point(80, 0)), 1);
    ASSERT_EQ(scrollHorizontal->getPageNumberForContentOffset(Point(120, 0)), 1.5);
}

} // namespace ValdiTest

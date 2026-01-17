#pragma once

namespace Valdi {

/**
 * Each view node has a "Category"
 * its used to determine its function on the screen,
 * and how it should be announced by the accessibility systems
 * (Checkout NativeTemplateElements.ts for more info)
 */
enum AccessibilityCategory {
    AccessibilityCategoryAuto = 0,
    AccessibilityCategoryView = 1,
    AccessibilityCategoryText = 2,
    AccessibilityCategoryButton = 3,
    AccessibilityCategoryImage = 4,
    AccessibilityCategoryImageButton = 5,
    AccessibilityCategoryInput = 6,
    AccessibilityCategoryHeader = 7,
    AccessibilityCategoryLink = 8,
    AccessibilityCategoryCheckBox = 9,
    AccessibilityCategoryRadio = 10,
    AccessibilityCategoryKeyboardKey = 11,
};

/**
 * Each view node has a "Category"
 * its used to determine how it will be navigated through,
 * when accessibility systems try to walk through the view node hierarchy
 * (Checkout NativeTemplateElements.ts for more info)
 */
enum AccessibilityNavigation {
    AccessibilityNavigationAuto = 0,
    AccessibilityNavigationPassthrough,
    AccessibilityNavigationLeaf,
    AccessibilityNavigationCover,
    AccessibilityNavigationGroup,
    AccessibilityNavigationIgnored,
};

} // namespace Valdi

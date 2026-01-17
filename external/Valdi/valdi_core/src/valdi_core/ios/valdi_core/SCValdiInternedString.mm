//
//  SCValdiInternedString.m
//  valdi-ios
//
//  Created by Simon Corsin on 7/20/19.
//

#import "valdi_core/SCValdiInternedString+CPP.h"
#import "valdi_core/SCValdiValueUtils.h"
#import "valdi_core/cpp/Utils/StringBox.hpp"
#import "valdi_core/cpp/Utils/StringCache.hpp"

static SCValdiInternedStringRef SCValdiInternedStringFromStringBox(const Valdi::StringBox &strBox)
{
    return reinterpret_cast<SCValdiInternedStringRef>(new Valdi::StringBox(strBox));
}

SCValdiInternedStringRef SCValdiInternedStringCreate(const char *cLiteral)
{
    return SCValdiInternedStringFromStringBox(STRING_LITERAL(cLiteral));
}

void SCValdiInternedStringDestroy(SCValdiInternedStringRef internedString)
{
    delete SCValdiInternedStringUnwrap(internedString);
}

SCValdiInternedStringRef SCValdiInternedStringFromNSString(NSString *nsString)
{
    return SCValdiInternedStringFromStringBox(ValdiIOS::InternedStringFromNSString(nsString));
}

Valdi::StringBox *SCValdiInternedStringUnwrap(SCValdiInternedStringRef internedString) {
    return reinterpret_cast<Valdi::StringBox *>(internedString);
}

const char *SCValdiInternedStringGetCStr(SCValdiInternedStringRef internedString)
{
    return SCValdiInternedStringUnwrap(internedString)->getCStr();
}

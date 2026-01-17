//
//  SCValdiBuiltinBlockSupport.h
//  valdi-ios
//
//  Created by Simon Corsin on 6/3/20.
//

#import "valdi_core/SCMacros.h"
#import "valdi_core/SCValdiFieldValue.h"
#import <Foundation/NSObjCRuntime.h>

SC_EXTERN_C_BEGIN

// A list of block support functions that the framework support as builtin.
// This is used to reduce codegen for the common cases of block usages

extern SCValdiFieldValue SCValdiFunctionInvokeO_v(const void* function, SCValdiFieldValue* fields);
extern SCValdiFieldValue SCValdiFunctionInvokeOO_v(const void* function, SCValdiFieldValue* fields);
extern SCValdiFieldValue SCValdiFunctionInvokeOOO_v(const void* function, SCValdiFieldValue* fields);
extern SCValdiFieldValue SCValdiFunctionInvokeOOOO_v(const void* function, SCValdiFieldValue* fields);
extern SCValdiFieldValue SCValdiFunctionInvokeOOOOO_v(const void* function, SCValdiFieldValue* fields);
extern SCValdiFieldValue SCValdiFunctionInvokeOOOOOO_v(const void* function, SCValdiFieldValue* fields);

extern SCValdiFieldValue SCValdiFunctionInvokeO_o(const void* function, SCValdiFieldValue* fields);
extern SCValdiFieldValue SCValdiFunctionInvokeOO_o(const void* function, SCValdiFieldValue* fields);
extern SCValdiFieldValue SCValdiFunctionInvokeOOO_o(const void* function, SCValdiFieldValue* fields);
extern SCValdiFieldValue SCValdiFunctionInvokeOOOO_o(const void* function, SCValdiFieldValue* fields);

extern SCValdiFieldValue SCValdiFunctionInvokeO_d(const void* function, SCValdiFieldValue* fields);
extern SCValdiFieldValue SCValdiFunctionInvokeOO_d(const void* function, SCValdiFieldValue* fields);
extern SCValdiFieldValue SCValdiFunctionInvokeOOO_d(const void* function, SCValdiFieldValue* fields);

extern SCValdiFieldValue SCValdiFunctionInvokeO_b(const void* function, SCValdiFieldValue* fields);
extern SCValdiFieldValue SCValdiFunctionInvokeOO_b(const void* function, SCValdiFieldValue* fields);
extern SCValdiFieldValue SCValdiFunctionInvokeOOO_b(const void* function, SCValdiFieldValue* fields);

extern id SCValdiBlockCreateO_v(SCValdiBlockTrampoline trampoline);
extern id SCValdiBlockCreateOO_v(SCValdiBlockTrampoline trampoline);
extern id SCValdiBlockCreateOOO_v(SCValdiBlockTrampoline trampoline);
extern id SCValdiBlockCreateOOOO_v(SCValdiBlockTrampoline trampoline);
extern id SCValdiBlockCreateOOOOO_v(SCValdiBlockTrampoline trampoline);
extern id SCValdiBlockCreateOOOOOO_v(SCValdiBlockTrampoline trampoline);

extern id SCValdiBlockCreateO_o(SCValdiBlockTrampoline trampoline);
extern id SCValdiBlockCreateOO_o(SCValdiBlockTrampoline trampoline);
extern id SCValdiBlockCreateOOO_o(SCValdiBlockTrampoline trampoline);
extern id SCValdiBlockCreateOOOO_o(SCValdiBlockTrampoline trampoline);

extern id SCValdiBlockCreateO_d(SCValdiBlockTrampoline trampoline);
extern id SCValdiBlockCreateOO_d(SCValdiBlockTrampoline trampoline);
extern id SCValdiBlockCreateOOO_d(SCValdiBlockTrampoline trampoline);

extern id SCValdiBlockCreateO_b(SCValdiBlockTrampoline trampoline);
extern id SCValdiBlockCreateOO_b(SCValdiBlockTrampoline trampoline);
extern id SCValdiBlockCreateOOO_b(SCValdiBlockTrampoline trampoline);

SC_EXTERN_C_END

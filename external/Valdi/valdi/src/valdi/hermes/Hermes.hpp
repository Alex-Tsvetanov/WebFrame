//
//  Hermes.hpp
//  valdi-hermes
//
//  Created by Simon Corsin on 9/27/23.
//

#pragma once

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wshorten-64-to-32"
#pragma clang diagnostic ignored "-Wdeprecated-this-capture"

#ifndef NDEBUG
#define NDEBUG 1
#endif

#include "hermes/ADT/ManagedChunkedList.h"
#include "hermes/BCGen/HBC/BytecodeProviderFromSrc.h"
#include "hermes/Optimizer/PassManager/Pipeline.h"
#include "hermes/Support/MemoryBuffer.h"
#include "hermes/VM/Callable.h"
#include "hermes/VM/HostModel.h"
#include "hermes/VM/JSArray.h"
#include "hermes/VM/JSArrayBuffer.h"
#include "hermes/VM/JSError.h"
#include "hermes/VM/JSTypedArray.h"
#include "hermes/VM/JSWeakRef.h"
#include "hermes/VM/Operations.h"
#include "hermes/VM/Runtime.h"
#include "hermes/VM/StringPrimitive.h"
#include "hermes/VM/StringView.h"

#pragma clang diagnostic pop

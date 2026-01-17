#pragma once

// Define DJINNI_FUNCTION_PROLOGUE to insert logging or tracing to Djinni calls.
// For example: profiling::SCTrace<profiling::OsTraceEmitter> djinniTrace{"Djinni:" method};
// Be careful not to use anything that depends on Djinni calls themselves,
// otherwise it will form an infinite recursion.
#define DJINNI_FUNCTION_PROLOGUE(method)

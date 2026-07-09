// QMake workaround:
// ggml.c and ggml.cpp share the same basename, which makes qmake generate
// the same object file name (ggml.o) for both translation units.
// Include ggml.c through a uniquely named wrapper to avoid object collisions.

#include "whisper.cpp/ggml/src/ggml.c"

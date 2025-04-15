#ifndef CWS_DEBUG_H
#include "chunk.h"
#define CWS_DEBUG_H

void DisassembleChunk(Chunk *chunk, const char *title);
int DisassembleInstruction(Chunk *chunk, int offset);

#endif // !CWS_DEBUG_H

#ifndef CWS_DEBUG_H
#include "chunk.h"
#define CWS_DEBUG_H

void disassemble_chunk(Chunk *chunk, const char *title);
int disassemble_instruction(Chunk *chunk, int offset);

#endif // !CWS_DEBUG_H

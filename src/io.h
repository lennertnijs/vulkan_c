#ifndef IO_H
#define IO_H

#include <stdio.h>
#include "assert.h"
#include <stdlib.h>


extern size_t fetch_file_size(char* file_name);
extern char* read_file(char* file_name, size_t amount);
extern int write_file(char* file_name, void* data, size_t size, size_t amount);

#endif
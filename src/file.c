#include "stdio.h"
#include "assert.h"

size_t fetch_file_size(char* file_name){
	FILE *file = fopen(file_name, "rb");
	fseek(file, 0, SEEK_END);
	size_t length = (size_t)ftell(file);
	fclose(file);
	return length;
}

char* read_file(char* file_name, size_t amount){
	assert(file_name != NULL);
	FILE *file = fopen(file_name, "rb");	
	char* buffer = (char*)malloc(amount);
	fread(buffer, 1, amount, file);
	fclose(file);
	return buffer;
}

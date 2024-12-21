#include <assert.h>

int min(int a, int b){
	if(a <= b){
		return a;
	}
	return b;
}

int max(int a, int b){
	if(a >= b){
		return a;
	}
	return b;
}

int clamp(int value, int min, int max){
	assert(min <= max);
	if(value < min){
		return min;
	}
	if(value > max){
		return max;
	}
	return value;
}

#ifndef PULSE_H
#define PULSE_H
#include <stdbool.h>

typedef enum {
	A, B, C, D, E, F, G, H, I, J, K, L, M, N, O, P, Q, R, S, T, U, V, W, X, Y, Z
} Key;

typedef enum {
	SHIFT, ALT, CAPS
	
} KeyMod;
typedef enum {
	PRESSED, RELEASED
} Action;

typedef struct PulseSession PulseSession;

typedef void (*PulseResizeCallback)(int width, int height);
typedef void (*PulseClickCallback)(int x, int y); 
typedef void (*PulseKeyCallback)(Key key, Action action, KeyMod mod);

extern PulseSession *pulse_create(int width, int height, char* title, bool allow_resize, PulseResizeCallback resize_callback, 
						 PulseClickCallback click_callback, PulseKeyCallback key_callback);
#endif

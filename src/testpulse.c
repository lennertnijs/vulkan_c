#include <stddef.h>

#include "pulse.h"

int main(){
	PulseSession *session = pulse_create(800, 600, "title",true, NULL, NULL, NULL);
	return 0;
}

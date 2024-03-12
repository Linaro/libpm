#include <stdio.h>

#include "power.h"

int main(int argc, char *argv[])
{
	struct power_handler *handler;

	handler = power_create();
	if (!handler)
		return 1;
	
	return 0;
}

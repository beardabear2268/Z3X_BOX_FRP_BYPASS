#include "poc.h"
#include <stdio.h>

int check_poc(uint8_t* response, int response_length) {
				// Real-world validation logic here
				for (int i = 0; i < response_length; ++i) {
								if (response[i] != 0) {
												printf("POC check failed at byte %d\n", i);
												return -1;
								}
				}
				printf("POC check passed.\n");
				return 0;
}
#include <stdbool.h>
#include <stdint.h>

int32_t get_syndrome(int32_t pattern);
int golay23_encode(int data);
int golay23_decode(int received_codeword);
int golay23_count_errors(int recd_codeword, int corrected_codeword);

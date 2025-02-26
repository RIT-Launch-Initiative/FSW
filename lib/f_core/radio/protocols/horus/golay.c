/* Title:   Encoder/decoder for a binary (23,12,7) Golay code
 * Author:  Robert Morelos-Zaragoza (robert@spectra.eng.hawaii.edu)
 * Date:    August 1994
 *
 * The binary (23,12,7) Golay code is an example of a perfect code, that is,
 * the number of syndromes equals the number of correctable error patterns.
 * The minimum distance is 7, so all error patterns of Hamming weight up to
 * 3 can be corrected. The total number of these error patterns is:
 *
 *       Number of errors         Number of patterns
 *       ----------------         ------------------
 *              0                         1
 *              1                        23
 *              2                       253
 *              3                      1771
 *                                     ----
 *    Total number of error patterns = 2048 = 2^{11} = number of syndromes
 *                                               --
 *                number of redundant bits -------^
 *
 * Because of its relatively low length (23), dimension (12) and number of
 * redundant bits (11), the binary (23,12,7) Golay code can be encoded and
 * decoded simply by using look-up tables. The program below uses a 16K
 * encoding table and an 8K decoding table.
 *
 * For more information, suggestions, or other ideas on implementing error
 * correcting codes, please contact me at (I'm temporarily in Japan, but
 * below is my U.S. address):
 *
 *                    Robert Morelos-Zaragoza
 *                    770 S. Post Oak Ln. #200
 *                      Houston, Texas 77056
 *
 *             email: robert@spectra.eng.hawaii.edu
 *
 *       Homework: Add an overall parity-check bit to get the (24,12,8)
 *                 extended Golay code.
 *
 * COPYRIGHT NOTICE: This computer program is free for non-commercial purposes.
 * You may implement this program for any non-commercial application. You may
 * also implement this program for commercial purposes, provided that you
 * obtain my written permission. Any modification of this program is covered
 * by this copyright.
 *
 * ==   Copyright (c) 1994  Robert Morelos-Zaragoza. All rights reserved.   ==
 */
#include "f_core/radio/protocols/horus/golay.h"

#include <assert.h>
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(GOLAY_ENC, CONFIG_HORUS_LOG_LEVEL);

#define X22    0x00400000 /* vector representation of X^{22} */
#define X11    0x00000800 /* vector representation of X^{11} */
#define MASK12 0xfffff800 /* auxiliary vector for testing */
#define GENPOL 0x00000c75 /* generator polinomial, g(x) */

/* Global variables:
 *
 * pattern = error pattern, or information, or received vector
 * encoding_table[] = encoding table
 * decoding_table[] = decoding table
 * data = information bits, i(x)
 * codeword = code bits = x^{11}i(x) + (x^{11}i(x) mod g(x))
 * numerr = number of errors = Hamming weight of error polynomial e(x)
 * position[] = error positions in the vector representation of e(x)
 * recd = representation of corrupted received polynomial r(x) = c(x) + e(x)
 * decerror = number of decoding errors
 * a[] = auxiliary array to generate correctable error patterns
 */

static bool inited = false;

static int encoding_table[4096], decoding_table[2048];

/*
 * Convert a binary vector of Hamming weight r, and nonzero positions in
 * array a[1]...a[r], to a long integer \sum_{i=1}^r 2^{a[i]-1}.
 */
static int arr2int(int a[], int r) {
    int i;
    long mul, result = 0, temp;

    for (i = 1; i <= r; i++) {
        mul = 1;
        temp = a[i] - 1;
        while (temp--) mul = mul << 1;
        result += mul;
    }
    return result;
}
/*
 * Calculate next r-combination of an n-set.
 */
void nextcomb(int n, int r, int a[]) {
    int i, j;

    a[r]++;
    if (a[r] <= n) return;
    j = r - 1;
    while (a[j] == n - r + j) j--;
    for (i = r; i >= j; i--) a[i] = a[j] + i - j + 1;
    return;
}
/*
 * Compute the syndrome corresponding to the given pattern, i.e., the
 * remainder after dividing the pattern (when considering it as the vector
 * representation of a polynomial) by the generator polynomial, GENPOL.
 * In the program this pattern has several meanings: (1) pattern = infomation
 * bits, when constructing the encoding table; (2) pattern = error pattern,
 * when constructing the decoding table; and (3) pattern = received vector, to
 * obtain its syndrome in decoding.
 */
int32_t get_syndrome(int32_t pattern) {
    int32_t aux = X22;

    if (pattern >= X11)
        while (pattern & MASK12) {
            while (!(aux & pattern)) aux = aux >> 1;
            pattern ^= (aux / X11) * GENPOL;
        }
    return pattern;
}

/*---------------------------------------------------------------------------*\

  FUNCTION....: golay23_init()
  AUTHOR......: David Rowe
  DATE CREATED: 3 March 2013

  Call this once when you start your program to init the Golay tables.

\*---------------------------------------------------------------------------*/

int golay23_init(void) {
    int i;
    long temp;
    int a[4];
    int pattern;

    /*
    * ---------------------------------------------------------------------
    *                  Generate ENCODING TABLE
    *
    * An entry to the table is an information vector, a 32-bit integer,
    * whose 12 least significant positions are the information bits. The
    * resulting value is a codeword in the (23,12,7) Golay code: A 32-bit
    * integer whose 23 least significant bits are coded bits: Of these, the
    * 12 most significant bits are information bits and the 11 least
    * significant bits are redundant bits (systematic encoding).
    * ---------------------------------------------------------------------
    */
    for (pattern = 0; pattern < 4096; pattern++) {
        temp = pattern << 11;                                /* multiply information by X^{11} */
        encoding_table[pattern] = temp + get_syndrome(temp); /* add redundancy */
    }

    /*
    * ---------------------------------------------------------------------
    *                  Generate DECODING TABLE
    *
    * An entry to the decoding table is a syndrome and the resulting value
    * is the most likely error pattern. First an error pattern is generated.
    * Then its syndrome is calculated and used as a pointer to the table
    * where the error pattern value is stored.
    * ---------------------------------------------------------------------
    *
    * (1) Error patterns of WEIGHT 1 (SINGLE ERRORS)
    */
    decoding_table[0] = 0;
    decoding_table[1] = 1;
    temp = 1;
    for (i = 2; i <= 23; i++) {
        temp *= 2;
        decoding_table[get_syndrome(temp)] = temp;
    }
    /*
    * (2) Error patterns of WEIGHT 2 (DOUBLE ERRORS)
    */
    a[1] = 1;
    a[2] = 2;
    temp = arr2int(a, 2);
    decoding_table[get_syndrome(temp)] = temp;
    for (i = 1; i < 253; i++) {
        nextcomb(23, 2, a);
        temp = arr2int(a, 2);
        decoding_table[get_syndrome(temp)] = temp;
    }
    /*
    * (3) Error patterns of WEIGHT 3 (TRIPLE ERRORS)
    */
    a[1] = 1;
    a[2] = 2;
    a[3] = 3;
    temp = arr2int(a, 3);
    decoding_table[get_syndrome(temp)] = temp;
    for (i = 1; i < 1771; i++) {
        nextcomb(23, 3, a);
        temp = arr2int(a, 3);
        decoding_table[get_syndrome(temp)] = temp;
    }
    inited = true;
    return 0;
}
// Initialize the golay tables
SYS_INIT(golay23_init, POST_KERNEL, 0);

/*---------------------------------------------------------------------------*\

  FUNCTION....: golay23_encode()
  AUTHOR......: David Rowe
  DATE CREATED: 3 March 2013

  Given 12 bits of data retiurns a 23 bit codeword for transmission
  over the channel.

\*---------------------------------------------------------------------------*/

int golay23_encode(int data) {
    assert(inited);
    assert(data <= 0xfff);

    return encoding_table[data];
}

#ifdef CONFIG_HORUSV2_RX

/*---------------------------------------------------------------------------*\

  FUNCTION....: golay23_decode()
  AUTHOR......: David Rowe
  DATE CREATED: 3 March 2013

  Given a 23 bit received codeword, returns the 12 bit corrected data.

\*---------------------------------------------------------------------------*/

int golay23_decode(int received_codeword) {
    assert(inited);
    assert((received_codeword < (1 << 23)) && (received_codeword >= 0));

    LOG_INF("syndrome: 0x%x", get_syndrome(received_codeword));
    return received_codeword ^= decoding_table[get_syndrome(received_codeword)];
}

int golay23_count_errors(int recd_codeword, int corrected_codeword) {
    int errors = 0;
    int diff, i;

    diff = recd_codeword ^ corrected_codeword;
    for (i = 0; i < 23; i++) {
        if (diff & 0x1) errors++;
        diff >>= 1;
    }

    return errors;
}

#endif

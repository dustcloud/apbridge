/*
 * Copyright (c) 1999-2004 by Eugene B. Surovegin <esl@ebshome.net>
 * Copyright (c) 2007 by Sergey Zolotarev <s.zolotarev@gmail.com>
 *
 * Code ripped from Apache source. Apache extension %pI removed from vformatter.
 * Original esl_vformatter implementation by Eugene B. Surovegin
 * Refactored as C++ base class interface by Sergey Zolotarev
 *
 * $Source:
 * $Id:
 */
#ifdef _WIN32
   #pragma warning( disable : 4996 )
#endif

#include "FmtOutput.h"

#include <ctype.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include <limits.h>
// Platform-specific ntoh* header
#ifdef __linux__
#include <arpa/inet.h> // for ntohl
#elif defined(WIN32)
#include <winsock2.h>  // for ntohl
#endif

/* These macros allow correct support of 8-bit characters on systems which
 * support 8-bit characters.  Pretty dumb how the cast is required, but
 * that's legacy libc for ya.  These new macros do not support EOF like
 * the standard macros do.  Tough.
 */
#define ap_isalnum(c) (isalnum(((unsigned char)(c))))
#define ap_isalpha(c) (isalpha(((unsigned char)(c))))
#define ap_iscntrl(c) (iscntrl(((unsigned char)(c))))
#define ap_isdigit(c) (isdigit(((unsigned char)(c))))
#define ap_isgraph(c) (isgraph(((unsigned char)(c))))
#define ap_islower(c) (islower(((unsigned char)(c))))
#define ap_isprint(c) (isprint(((unsigned char)(c))))
#define ap_ispunct(c) (ispunct(((unsigned char)(c))))
#define ap_isspace(c) (isspace(((unsigned char)(c))))
#define ap_isupper(c) (isupper(((unsigned char)(c))))
#define ap_isxdigit(c) (isxdigit(((unsigned char)(c))))
#define ap_tolower(c) (tolower(((unsigned char)(c))))
#define ap_toupper(c) (toupper(((unsigned char)(c))))

typedef enum {
    NO = 0, YES = 1
} boolean_e;


#define NUL                     '\0'
#define INT_NULL                ((int *)0)
#define WIDE_INT                long

typedef WIDE_INT wide_int;
typedef unsigned WIDE_INT u_wide_int;
typedef int64_t widest_int;
typedef uint64_t u_widest_int;

//#define S_NULL                  "(null)"
#define S_NULL_LEN              6

#define FLOAT_DIGITS            6
#define EXPONENT_LENGTH         10

static const char low_digits[] = "0123456789abcdef";
static const char upper_digits[] = "0123456789ABCDEF";

static char *conv_p2(register u_wide_int num, register int nbits,
                     char format, char *buf_end, register int *len);

/* Extension handlers */
static struct vformatter_extension_t {
  char* (*handler)(const void* arg, char* buf, int* len);
} __extension['Z' - 'A' + 'z' - 'a' + 2];

#define EXTENSION_IS_OK(ch) (((ch) >= 'A' && (ch) <= 'Z') || ((ch) >= 'a' && (ch) <= 'z'))
#define EXTENSION_IDX(ch)   ((ch) <= 'Z' ? ((ch) - 'A') : ((ch) - 'a' + 'Z' - 'A' + 1))

/*
 * NUM_BUF_SIZE is the size of the buffer used for arithmetic conversions
 *
 * XXX: this is a magic number; do not decrease it
 */
#define NUM_BUF_SIZE            512

/*
 * cvt.c - IEEE floating point formatting routines for FreeBSD
 * from GNU libc-4.6.27.  Modified to be thread safe.
 */

/*
 *    ap_ecvt converts to decimal
 *      the number of digits is specified by ndigit
 *      decpt is set to the position of the decimal point
 *      sign is set to 0 for positive, 1 for negative
 */

#define NDIG    80

/* buf must have at least NDIG bytes */
static char *ap_cvt(double arg, int ndigits, int *decpt, bool *sign, int eflag, char *buf)
{
    register int r2;
    double fi, fj;
    register char *p, *p1;

    if (ndigits >= NDIG - 1)
        ndigits = NDIG - 2;
    r2 = 0;
    *sign = false;
    p = &buf[0];
    if (arg < 0) {
        *sign = true;
        arg = -arg;
    }
    arg = modf(arg, &fi);
    p1 = &buf[NDIG];
    /*
     * Do integer part
     */
    if (fi != 0) {
        p1 = &buf[NDIG];
        while (p1 > &buf[0] && fi != 0) {
            fj = modf(fi / 10, &fi);
            *--p1 = (int) ((fj + .03) * 10) + '0';
            r2++;
        }
        if (p1 == &buf[0])
            --r2;
        while (p1 < &buf[NDIG])
            *p++ = *p1++;
    }
    else if (arg > 0) {
        while ((fj = arg * 10) < 1) {
            arg = fj;
            r2--;
        }
    }
    p1 = &buf[ndigits];
    if (eflag == 0)
        p1 += r2;
    if (p1 < &buf[0]) {
        *decpt = -ndigits;
        buf[0] = '\0';
        return (buf);
    }
    *decpt = r2;
    while (p <= p1 && p < &buf[NDIG]) {
        arg *= 10;
        arg = modf(arg, &fj);
        *p++ = (int) fj + '0';
    }
    if (p1 >= &buf[NDIG]) {
        buf[NDIG - 1] = '\0';
        return (buf);
    }
    p = p1;
    *p1 += 5;
    while (*p1 > '9') {
        *p1 = '0';
        if (p1 > buf)
            ++ * --p1;
        else {
            *p1 = '1';
            (*decpt)++;
            if (eflag == 0) {
                if (p > buf)
                    *p = '0';
                p++;
            }
        }
    }
    *p = '\0';
    return (buf);
}

static char *ap_ecvt(double arg, int ndigits, int *decpt, bool *sign, char *buf)
{
    return (ap_cvt(arg, ndigits, decpt, sign, 1, buf));
}

static char *ap_fcvt(double arg, int ndigits, int *decpt, bool *sign, char *buf)
{
    return (ap_cvt(arg, ndigits, decpt, sign, 0, buf));
}

/*
 * ap_gcvt  - Floating output conversion to
 * minimal length string
 */

static char *ap_gcvt(double number, int ndigit, char *buf, boolean_e altform)
{
    bool sign;
    int decpt;
    register char *p1, *p2;
    register int i;
    char buf1[NDIG];

    p1 = ap_ecvt(number, ndigit, &decpt, &sign, buf1);
    p2 = buf;
    if (sign)
        *p2++ = '-';
    for (i = ndigit - 1; i > 0 && p1[i] == '0'; i--)
        ndigit--;
    if ((decpt >= 0 && decpt - ndigit > 4)
        || (decpt < 0 && decpt < -3)) {         /* use E-style */
        decpt--;
        *p2++ = *p1++;
        *p2++ = '.';
        for (i = 1; i < ndigit; i++)
            *p2++ = *p1++;
        *p2++ = 'e';
        if (decpt < 0) {
            decpt = -decpt;
            *p2++ = '-';
        }
        else
            *p2++ = '+';
        if (decpt / 100 > 0)
            *p2++ = decpt / 100 + '0';
        if (decpt / 10 > 0)
            *p2++ = (decpt % 100) / 10 + '0';
        *p2++ = decpt % 10 + '0';
    }
    else {
        if (decpt <= 0) {
            if (*p1 != '0')
                *p2++ = '.';
            while (decpt < 0) {
                decpt++;
                *p2++ = '0';
            }
        }
        for (i = 1; i <= ndigit; i++) {
            *p2++ = *p1++;
            if (i == decpt)
                *p2++ = '.';
        }
        if (ndigit < decpt) {
            while (ndigit++ < decpt)
                *p2++ = '0';
            *p2++ = '.';
        }
    }
    if (p2[-1] == '.' && !altform)
        p2--;
    *p2 = '\0';
    return (buf);
}

/*
 * The INS_CHAR macro outputs a character and increments character count.
 * If putCh() returns error, we return immediately.
 * NOTE: Evaluation of the c argument should not have any side-effects
 */
#define INS_CHAR(c, cc) do { if(!putCh(c)) return cc; cc++; } while(0)

#define NUM( c )                        ( c - '0' )

#define STR_TO_DEC( str, num )          \
    num = NUM( *str++ ) ;               \
    while ( ap_isdigit( *str ) )        \
    {                                   \
        num *= 10 ;                     \
        num += NUM( *str++ ) ;          \
    }

/*
 * This macro does zero padding so that the precision
 * requirement is satisfied. The padding is done by
 * adding '0's to the left of the string that is going
 * to be printed.
 */
#define FIX_PRECISION( adjust, precision, s, s_len )    \
    if ( adjust )                                       \
        while ( s_len < precision )                     \
        {                                               \
            *--s = '0' ;                                \
            s_len++ ;                                   \
        }

/*
 * Macro that does padding. The padding is done by printing
 * the character ch.
 */
#define PAD( width, len, ch )   do              \
        {                                       \
            INS_CHAR( ch, cc ) ;       \
            width-- ;                           \
        }                                       \
        while ( width > len )

/*
 * Prefix the character ch to the string str
 * Increase length
 * Set the has_prefix flag
 */
#define PREFIX( str, length, ch )        *--str = ch ; length++ ; has_prefix = YES


/*
 * Convert num to its decimal format.
 * Return value:
 *   - a pointer to a string containing the number (no sign)
 *   - len contains the length of the string
 *   - is_negative is set to TRUE or FALSE depending on the sign
 *     of the number (always set to FALSE if is_unsigned is TRUE)
 *
 * The caller provides a buffer for the string: that is the buf_end argument
 * which is a pointer to the END of the buffer + 1 (i.e. if the buffer
 * is declared as buf[ 100 ], buf_end should be &buf[ 100 ])
 *
 * Note: we have 2 versions. One is used when we need to use quads
 * (conv_10_quad), the other when we don't (conv_10). We're assuming the
 * latter is faster.
 */
static char *conv_10(register wide_int num, bool is_unsigned,
                     bool *is_negative, char *buf_end,
                     register int *len)
{
    register char *p = buf_end;
    register u_wide_int magnitude;

    if (is_unsigned) {
        magnitude = (u_wide_int) num;
        *is_negative = false;
    }
    else {
        *is_negative = (num < 0);

        /*
         * On a 2's complement machine, negating the most negative integer 
         * results in a number that cannot be represented as a signed integer.
         * Here is what we do to obtain the number's magnitude:
         *      a. add 1 to the number
         *      b. negate it (becomes positive)
         *      c. convert it to unsigned
         *      d. add 1
         */
        if (*is_negative) {
            wide_int t = num + 1;

            magnitude = ((u_wide_int) -t) + 1;
        }
        else
            magnitude = (u_wide_int) num;
    }

    /*
     * We use a do-while loop so that we write at least 1 digit 
     */
    do {
        register u_wide_int new_magnitude = magnitude / 10;

        *--p = (char) (magnitude - new_magnitude * 10 + '0');
        magnitude = new_magnitude;
    }
    while (magnitude);

    *len = buf_end - p;
    return (p);
}

static char *conv_10_quad(widest_int num, bool is_unsigned,
                     bool  *is_negative, char *buf_end,
                     register int *len)
{
    register char *p = buf_end;
    u_widest_int magnitude;

    /*
     * We see if we can use the faster non-quad version by checking the
     * number against the largest long value it can be. If <=, we
     * punt to the quicker version.
     */
    if (((u_widest_int)num <= ULONG_MAX && is_unsigned) || 
        ((u_widest_int)num <= LONG_MAX && !is_unsigned)){
        return(conv_10( (wide_int)num, is_unsigned, is_negative,
               buf_end, len));
    }

    if (is_unsigned) {
        magnitude = (u_widest_int) num;
        *is_negative = false;
    }
    else {
        *is_negative = (num < 0);

        /*
         * On a 2's complement machine, negating the most negative integer 
         * results in a number that cannot be represented as a signed integer.
         * Here is what we do to obtain the number's magnitude:
         *      a. add 1 to the number
         *      b. negate it (becomes positive)
         *      c. convert it to unsigned
         *      d. add 1
         */
        if (*is_negative) {
            widest_int t = num + 1;

            magnitude = ((u_widest_int) -t) + 1;
        }
        else
            magnitude = (u_widest_int) num;
    }

    /*
     * We use a do-while loop so that we write at least 1 digit 
     */
    do {
        u_widest_int new_magnitude = magnitude / 10;

        *--p = (char) (magnitude - new_magnitude * 10 + '0');
        magnitude = new_magnitude;
    }
    while (magnitude);

    *len = buf_end - p;
    return (p);
}

static char *conv_ip_addr(unsigned addr, char *buf_end, int *len)
{
    char *p = buf_end;
    bool is_negative;
    int sub_len;

    p = conv_10((addr & 0x000000FF)      , true, &is_negative, p, &sub_len);
    *--p = '.';
    p = conv_10((addr & 0x0000FF00) >>  8, true, &is_negative, p, &sub_len);
    *--p = '.';
    p = conv_10((addr & 0x00FF0000) >> 16, true, &is_negative, p, &sub_len);
    *--p = '.';
    p = conv_10((addr & 0xFF000000) >> 24, true, &is_negative, p, &sub_len);

    *len = buf_end - p;
    return (p);
}

static inline char *conv_ip_addr_aux(const uint8_t* addr, char *buf_end){
  for (int i = 4; i; --i){
    bool bdummy;
    int  idummy;
    buf_end = conv_10(*--addr, true, &bdummy, buf_end, &idummy);  
    *--buf_end = '.';
  }
  return buf_end + 1;
}

static char* conv_ipv6_addr(const uint8_t* addr, char* buf_end, int* len,
                            bool plain_v4mapped){
  uint16_t words[8];
  char* s = buf_end;
  int i;
  struct longest_zeroes_chunk_t {
    int start;
    int end;
  } cur = {-1, -1}, best = {-1, -1};

  for (i = 0; i < 8; ++i){
    uint16_t w = *addr++ << 8;
    w |= *addr++;

    if (!w){
      if (cur.start < 0)
        cur.start = cur.end = i;
      else
        ++cur.end;
    }
    else if (cur.start >= 0){
      if (best.start < 0 || (best.end - best.start < cur.end - cur.start))
        best = cur;
      cur.start = -1;
    }
    words[i] = w;
  }

  if (cur.start >= 0 && (best.start < 0 || (best.end - best.start < cur.end - cur.start)))
    best = cur;
  
  if (best.start == best.end)
    best.start = best.end = -1;

  /* Special cases for embedded IPv4 addresses */  
  if (!best.start && ((best.end == 4 && words[5] == 0xffff) ||
                      (best.end > 4 && (words[6] || words[7] > 1))
                     ))
  {
    s = conv_ip_addr_aux(addr, s);
    if (words[5]){
      if (plain_v4mapped)
        goto out;
      *--s = ':';
      *--s = 'f'; *--s = 'f';  *--s = 'f'; *--s = 'f'; 
    }
    *--s = ':';
    *--s = ':';
  }
  else {
    for (i = 7; i >= 0; --i){
      if (best.end == i){
        i = best.start - 1;
        *--s = ':';
        if (i < 0){
          *--s = ':';
          break;
        }
      }

      if (i != 7)
        *--s = ':';

      int dummy;
      s = conv_p2(words[i], 4, 'x', s, &dummy);
    }
  }

out:
  *len = buf_end - s;
  return s;
}

/*
 * Convert a floating point number to a string formats 'f', 'e' or 'E'.
 * The result is placed in buf, and len denotes the length of the string
 * The sign is returned in the is_negative argument (and is not placed
 * in buf).
 */
static char *conv_fp(register char format, register double num,
    boolean_e add_dp, int precision, bool *is_negative,
    char *buf, int *len)
{
    register char *s = buf;
    register char *p;
    int decimal_point;
    char buf1[NDIG];

    if (format == 'f')
        p = ap_fcvt(num, precision, &decimal_point, is_negative, buf1);
    else                        /* either e or E format */
        p = ap_ecvt(num, precision + 1, &decimal_point, is_negative, buf1);

    /*
     * Check for Infinity and NaN
     */
    if (ap_isalpha(*p)) {
        strncpy(buf, p, NUM_BUF_SIZE-2); // first array element is the sign, last 0
        buf[NUM_BUF_SIZE-2] = '\0';
        *len = strlen(buf);
        *is_negative = false;
        return (buf);
    }

    if (format == 'f') {
        if (decimal_point <= 0) {
            *s++ = '0';
            if (precision > 0) {
                *s++ = '.';
                while (decimal_point++ < 0)
                    *s++ = '0';
            }
            else if (add_dp)
                *s++ = '.';
        }
        else {
            while (decimal_point-- > 0)
                *s++ = *p++;
            if (precision > 0 || add_dp)
                *s++ = '.';
        }
    }
    else {
        *s++ = *p++;
        if (precision > 0 || add_dp)
            *s++ = '.';
    }

    /*
     * copy the rest of p, the NUL is NOT copied
     */
    while (*p)
        *s++ = *p++;

    if (format != 'f') {
        char temp[EXPONENT_LENGTH];     /* for exponent conversion */
        int t_len;
        bool exponent_is_negative;

        *s++ = format;          /* either e or E */
        decimal_point--;
        if (decimal_point != 0) {
            p = conv_10((wide_int) decimal_point, false, &exponent_is_negative,
                        &temp[EXPONENT_LENGTH], &t_len);
            *s++ = exponent_is_negative ? '-' : '+';

            /*
             * Make sure the exponent has at least 2 digits
             */
            if (t_len == 1)
                *s++ = '0';
            while (t_len--)
                *s++ = *p++;
        }
        else {
            *s++ = '+';
            *s++ = '0';
            *s++ = '0';
        }
    }

    *len = s - buf;
    return (buf);
}


/*
 * Convert num to a base X number where X is a power of 2. nbits determines X.
 * For example, if nbits is 3, we do base 8 conversion
 * Return value:
 *      a pointer to a string containing the number
 *
 * The caller provides a buffer for the string: that is the buf_end argument
 * which is a pointer to the END of the buffer + 1 (i.e. if the buffer
 * is declared as buf[ 100 ], buf_end should be &buf[ 100 ])
 *
 * As with conv_10, we have a faster version which is used when
 * the number isn't quad size.
 */
static char *conv_p2(register u_wide_int num, register int nbits,
                     char format, char *buf_end, register int *len)
{
    register int mask = (1 << nbits) - 1;
    register char *p = buf_end;
    register const char *digits = (format == 'X') ? upper_digits : low_digits;

    do {
        *--p = digits[num & mask];
        num >>= nbits;
    }
    while (num);

    *len = buf_end - p;
    return (p);
}

static char *conv_p2_quad(u_widest_int num, register int nbits,
                     char format, char *buf_end, register int *len)
{
    register int mask = (1 << nbits) - 1;
    register char *p = buf_end;
    register const char *digits = (format == 'X') ? upper_digits : low_digits;

    if (num <= ULONG_MAX)
        return(conv_p2( (u_wide_int)num, nbits, format, buf_end, len));

    do {
        *--p = digits[num & mask];
        num >>= nbits;
    }
    while (num);

    *len = buf_end - p;
    return (p);
}

/*
 * Do format conversion placing the output in buffer
 */
int IFmtOutput::esl_vformatter(const char *fmt, va_list ap)
{
    register int cc = 0;
    register int i;

    register char *s = NULL;
    char *q;
    int s_len;

    register int min_width = 0;
    int precision = 0;
    enum {
        LEFT, RIGHT
    } adjust;
    char pad_char;
    char prefix_char;

    double fp_num;
    widest_int i_quad = (widest_int) 0;
    u_widest_int ui_quad;
    wide_int i_num = (wide_int) 0;
    u_wide_int ui_num;

    char S_NULL[]="(null)";
	char S_BOGUS[] = "bogus %p";
	char S_P[] = "%p";
	char num_buf[NUM_BUF_SIZE];
    char char_buf[2];           /* for printing %% and %<unknown> */

    enum var_type_enum {
        IS_QUAD, IS_LONG, IS_SHORT, IS_INT
    };
    enum var_type_enum var_type = IS_INT;

    /*
     * Flag variables
     */
    boolean_e alternate_form;
    boolean_e print_sign;
    boolean_e print_blank;
    boolean_e adjust_precision;
    boolean_e adjust_width;
    bool is_negative;

    while (*fmt) {
        if (*fmt != '%') {
            INS_CHAR(*fmt, cc);
        }
        else {
            /*
             * Default variable settings
             */
            boolean_e print_something = YES;
            adjust = RIGHT;
            alternate_form = print_sign = print_blank = NO;
            pad_char = ' ';
            prefix_char = NUL;

            fmt++;

            /*
             * Try to avoid checking for flags, width or precision
             */
            if (!ap_islower(*fmt)) {
                /*
                 * Recognize flags: -, #, BLANK, +
                 */
                for (;; fmt++) {
                    if (*fmt == '-')
                        adjust = LEFT;
                    else if (*fmt == '+')
                        print_sign = YES;
                    else if (*fmt == '#')
                        alternate_form = YES;
                    else if (*fmt == ' ')
                        print_blank = YES;
                    else if (*fmt == '0')
                        pad_char = '0';
                    else
                        break;
                }

                /*
                 * Check if a width was specified
                 */
                if (ap_isdigit(*fmt)) {
                    STR_TO_DEC(fmt, min_width);
                    adjust_width = YES;
                }
                else if (*fmt == '*') {
                    min_width = va_arg(ap, int);
                    fmt++;
                    adjust_width = YES;
                    if (min_width < 0) {
                        adjust = LEFT;
                        min_width = -min_width;
                    }
                }
                else
                    adjust_width = NO;

                /*
                 * Check if a precision was specified
                 *
                 * XXX: an unreasonable amount of precision may be specified
                 * resulting in overflow of num_buf. Currently we
                 * ignore this possibility.
                 */
                if (*fmt == '.') {
                    adjust_precision = YES;
                    fmt++;
                    if (ap_isdigit(*fmt)) {
                        STR_TO_DEC(fmt, precision);
                    }
                    else if (*fmt == '*') {
                        precision = va_arg(ap, int);
                        fmt++;
                        if (precision < 0)
                            precision = 0;
                    }
                    else
                        precision = 0;
                }
                else
                    adjust_precision = NO;
            }
            else
                adjust_precision = adjust_width = NO;

            /*
             * Modifier check
             */
            if (*fmt == 'q') {
                var_type = IS_QUAD;
                fmt++;
            }
            else if (*fmt == 'l') {
               var_type = IS_LONG;
               fmt++;
               // check whether the modifier is really long long (%lld)
               if (*fmt == 'l') {
                  var_type = IS_QUAD;
                  fmt++;
               }
            }
            else if (*fmt == 'h') {
                var_type = IS_SHORT;
                fmt++;
            }
            else {
                var_type = IS_INT;
            }

            /*
             * Argument extraction and printing.
             * First we determine the argument type.
             * Then, we convert the argument to a string.
             * On exit from the switch, s points to the string that
             * must be printed, s_len has the length of the string
             * The precision requirements, if any, are reflected in s_len.
             *
             * NOTE: pad_char may be set to '0' because of the 0 flag.
             *   It is reset to ' ' by non-numeric formats
             */
            switch (*fmt) {
            case 'u':
                if (var_type == IS_QUAD) {
                    i_quad = va_arg(ap, u_widest_int);
                    s = conv_10_quad(i_quad, 1, &is_negative,
                            &num_buf[NUM_BUF_SIZE], &s_len);
                }
                else {
                    if (var_type == IS_LONG)
                        i_num = (wide_int) va_arg(ap, u_wide_int);
                    else if (var_type == IS_SHORT)
                        i_num = (wide_int) (unsigned short) va_arg(ap, unsigned int);
                    else
                        i_num = (wide_int) va_arg(ap, unsigned int);
                    s = conv_10(i_num, 1, &is_negative,
                            &num_buf[NUM_BUF_SIZE], &s_len);
                }
                FIX_PRECISION(adjust_precision, precision, s, s_len);
                break;

            case 'd':
            case 'i':
                if (var_type == IS_QUAD) {
                    i_quad = va_arg(ap, widest_int);
                    s = conv_10_quad(i_quad, 0, &is_negative,
                            &num_buf[NUM_BUF_SIZE], &s_len);
                }
                else {
                    if (var_type == IS_LONG)
                        i_num = (wide_int) va_arg(ap, wide_int);
                    else if (var_type == IS_SHORT)
                        i_num = (wide_int) (short) va_arg(ap, int);
                    else
                        i_num = (wide_int) va_arg(ap, int);
                    s = conv_10(i_num, 0, &is_negative,
                            &num_buf[NUM_BUF_SIZE], &s_len);
                }
                FIX_PRECISION(adjust_precision, precision, s, s_len);

                if (is_negative)
                    prefix_char = '-';
                else if (print_sign)
                    prefix_char = '+';
                else if (print_blank)
                    prefix_char = ' ';
                break;


            case 'o':
                if (var_type == IS_QUAD) {
                    ui_quad = va_arg(ap, u_widest_int);
                    s = conv_p2_quad(ui_quad, 3, *fmt,
                            &num_buf[NUM_BUF_SIZE], &s_len);
                }
                else {
                    if (var_type == IS_LONG)
                        ui_num = (u_wide_int) va_arg(ap, u_wide_int);
                    else if (var_type == IS_SHORT)
                        ui_num = (u_wide_int) (unsigned short) va_arg(ap, unsigned int);
                    else
                        ui_num = (u_wide_int) va_arg(ap, unsigned int);
                    s = conv_p2(ui_num, 3, *fmt,
                            &num_buf[NUM_BUF_SIZE], &s_len);
                }
                FIX_PRECISION(adjust_precision, precision, s, s_len);
                if (alternate_form && *s != '0') {
                    *--s = '0';
                    s_len++;
                }
                break;


            case 'x':
            case 'X':
                if (var_type == IS_QUAD) {
                    ui_quad = va_arg(ap, u_widest_int);
                    s = conv_p2_quad(ui_quad, 4, *fmt,
                            &num_buf[NUM_BUF_SIZE], &s_len);
                }
                else {
                    if (var_type == IS_LONG)
                        ui_num = (u_wide_int) va_arg(ap, u_wide_int);
                    else if (var_type == IS_SHORT)
                        ui_num = (u_wide_int) (unsigned short) va_arg(ap, unsigned int);
                    else
                        ui_num = (u_wide_int) va_arg(ap, unsigned int);
                    s = conv_p2(ui_num, 4, *fmt,
                            &num_buf[NUM_BUF_SIZE], &s_len);
                }
                FIX_PRECISION(adjust_precision, precision, s, s_len);
                if (alternate_form && i_num != 0) {
                    *--s = *fmt;        /* 'x' or 'X' */
                    *--s = '0';
                    s_len += 2;
                }
                break;


            case 's':
                s = va_arg(ap, char *);
                if (s != NULL) {
                    s_len = strlen(s);
                    if (adjust_precision && precision < s_len)
                        s_len = precision;
                }
                else {
                    s = S_NULL;
                    s_len = S_NULL_LEN;
                }
                pad_char = ' ';
                break;


            case 'f':
            case 'e':
            case 'E':
                fp_num = va_arg(ap, double);
                /*
                 * * We use &num_buf[ 1 ], so that we have room for the sign
                 */
                s = conv_fp(*fmt, fp_num, alternate_form,
                        (adjust_precision == NO) ? FLOAT_DIGITS : precision,
                            &is_negative, &num_buf[1], &s_len);
                if (is_negative)
                    prefix_char = '-';
                else if (print_sign)
                    prefix_char = '+';
                else if (print_blank)
                    prefix_char = ' ';
                break;


            case 'g':
            case 'G':
                if (adjust_precision == NO)
                    precision = FLOAT_DIGITS;
                else if (precision == 0)
                    precision = 1;
                /*
                 * * We use &num_buf[ 1 ], so that we have room for the sign
                 */
                s = ap_gcvt(va_arg(ap, double), precision, &num_buf[1],
                            alternate_form);
                if (*s == '-')
                    prefix_char = *s++;
                else if (print_sign)
                    prefix_char = '+';
                else if (print_blank)
                    prefix_char = ' ';

                s_len = strlen(s);

                if (alternate_form && (strchr(s,'.') == NULL) ) {
                    s[s_len++] = '.';
                    s[s_len] = '\0'; /* delimit for following strchr() */
                }
                if (*fmt == 'G' && (q = strchr(s, 'e')) != NULL)
                    *q = 'E';
                break;


            case 'c':
                char_buf[0] = (char) (va_arg(ap, int));
                s = &char_buf[0];
                s_len = 1;
                pad_char = ' ';
                break;


            case '%':
                char_buf[0] = '%';
                s = &char_buf[0];
                s_len = 1;
                pad_char = ' ';
                break;


            case 'n':
                if (var_type == IS_QUAD)
                    *(va_arg(ap, widest_int *)) = cc;
                else if (var_type == IS_LONG)
                    *(va_arg(ap, long *)) = cc;
                else if (var_type == IS_SHORT)
                    *(va_arg(ap, short *)) = cc;
                else
                    *(va_arg(ap, int *)) = cc;
                s_len = -1;
                print_something = NO;
                break;

                /*
                 * This is where we extend the printf format, with a second
                 * type specifier
                 */
            case 'p':
                switch(*++fmt) {
                    /*
                     * If the pointer size is equal to or smaller than the size
                     * of the largest unsigned int, we convert the pointer to a
                     * hex number, otherwise we print "%p" to indicate that we
                     * don't handle "%p".
                     */
                case 'p':
#ifdef APR_VOID_P_IS_QUAD
                    if (sizeof(void *) <= sizeof(u_widest_int)) {
                        ui_quad = (u_widest_int) va_arg(ap, void *);
                        s = conv_p2_quad(ui_quad, 4, 'x',
                                &num_buf[NUM_BUF_SIZE], &s_len);
                    }
#else
                    if (sizeof(void *) <= sizeof(u_wide_int)) {
                        ui_num = (u_wide_int) va_arg(ap, void *);
                        s = conv_p2(ui_num, 4, 'x',
                                &num_buf[NUM_BUF_SIZE], &s_len);
                    }
#endif
                    else {
                        s = S_P;
                        s_len = 2;
                        prefix_char = NUL;
                    }
                    pad_char = ' ';
                    break;

                /* print a ulong as a.b.c.d */
                case 'A':
                case 'a':
                    {
                        ui_num = (u_wide_int) va_arg(ap, u_wide_int);
                        if (*fmt == 'A')
                          ui_num = ntohl(ui_num);

                        s = conv_ip_addr(ui_num, &num_buf[NUM_BUF_SIZE], &s_len);
                        if (adjust_precision && precision < s_len)
                          s_len = precision;

                        pad_char = ' ';
                    }
                    break;
                
                case 'B':
                case 'b':
                    {
                        uint8_t* pb = va_arg(ap, uint8_t*);
                        s = conv_ipv6_addr(pb, &num_buf[NUM_BUF_SIZE], &s_len, *fmt == 'b');
                        if (adjust_precision && precision < s_len)
                            s_len = precision;
                        pad_char = ' ';
                    }
                    break;

                case NUL:
                    /* if %p ends the string, oh well ignore it */
                    continue;

                default:
                    { 
                      char ch = *fmt;
                      const void* arg = va_arg(ap, void *);
                      /* Try to find registered extension */
                      if (EXTENSION_IS_OK(ch)){
                        int idx = EXTENSION_IDX(ch);
                        if (__extension[idx].handler){
                          s_len = NUM_BUF_SIZE;
                          s = __extension[idx].handler(arg, num_buf, &s_len);
                          if (adjust_precision && precision < s_len)
                            s_len = precision;
                          pad_char = ' ';
                          break;
                        }
                      }

                      s = S_BOGUS;
                      s_len = 8;
                      prefix_char = NUL;
                      break;
                    }
                }
                break;

            case NUL:
                /*
                 * The last character of the format string was %.
                 * We ignore it.
                 */
                continue;


                /*
                 * The default case is for unrecognized %'s.
                 * We print %<char> to help the user identify what
                 * option is not understood.
                 * This is also useful in case the user wants to pass
                 * the output of format_converter to another function
                 * that understands some other %<char> (like syslog).
                 * Note that we can't point s inside fmt because the
                 * unknown <char> could be preceded by width etc.
                 */
            default:
                char_buf[0] = '%';
                char_buf[1] = *fmt;
                s = char_buf;
                s_len = 2;
                pad_char = ' ';
                break;
            }

            if (prefix_char != NUL && s != S_NULL && s != char_buf) {
                *--s = prefix_char;
                s_len++;
            }

            if (adjust_width && adjust == RIGHT && s_len > 0 && min_width > s_len) {
                if (pad_char == '0' && prefix_char != NUL) {
                    INS_CHAR(*s, cc);
                    s++;
                    s_len--;
                    min_width--;
                }
                PAD(min_width, s_len, pad_char);
            }

            /*
             * Print the string s. 
             */
            if (print_something == YES) {
                for (i = s_len; i != 0; i--) {
                    INS_CHAR(*s, cc);
                    s++;
                }
            }

            if (adjust_width && adjust == LEFT && s_len > 0 && min_width > s_len)
                PAD(min_width, s_len, pad_char);
        }
        fmt++;
    }
    return cc;
}

int IFmtOutput::printf(const char *format,...)
{
    int cc;
    va_list ap;
    va_start(ap, format);
    cc = esl_vformatter(format, ap);
    va_end(ap);
    flush();
    return cc;
}

int IFmtOutput::vprintf(const char *format, va_list ap)
{
    int cc;
    cc = esl_vformatter(format, ap);
    flush();
    return cc;
}

void IFmtOutput::printDump(const unsigned char* array, int length,
                           const char * separator, const char* hdrStr, const char* endStr)
{
   if (hdrStr)
      printfNoFlush_p(hdrStr);
   for (int i=0; i<length; i++) {
      if (i) {
         printfNoFlush_p(separator);
      }
      printfNoFlush_p("%02x", array[i]);
   }
   if (endStr)
      printfNoFlush_p(endStr);
   flush();
}

int IFmtOutput::printfNoFlush_p(const char* format, ...)
{
    int cc;
    va_list ap;
    va_start(ap, format);
    cc = esl_vformatter(format, ap);
    va_end(ap);
    return cc;
}

bool IFmtOutput::registerFormatExt(char ext, FormatExtHandler handler)
{
  if (EXTENSION_IS_OK(ext)){
    int idx = EXTENSION_IDX(ext);
    if (!__extension[idx].handler) {
      __extension[idx].handler = handler;
      return true;
    }
  }
  return false;
}

#ifdef _TEST_MAIN_
char* MacAddrFmtExt(const void* arg, char* buf, int* len)
{
   const uint8_t *mac = (const uint8_t *) arg;
   const char *hexd = "0123456789abcdef";
   *len = 0;
   for(int ix = 0; ix < 8; ix++) {
      if (ix > 0)
         buf[(*len)++] = ':';
      buf[(*len)++] = hexd[(mac[ix] & 0xF0) >> 4];
      buf[(*len)++] = hexd[mac[ix] & 0x0F];
   }
   buf[(*len)++] = '\0';
   return buf;
}

int main(int argc, char **argv)
{
   int ix;
   {
      CFmtBuffer<60> b;
      for(ix = 0; ix < 5; ix++)
         b.printf("Line %d; ", ix);
      printf("This should fit well:\n%s\n", b.str());

      b.clear();
      for(ix = 0; ix < 10; ix++)
         b.printf("Line%05d; ", ix);
      printf("This should not fit:\n%s\n", b.str());

      b.clear();
      for(ix = 0; ix < 2; ix++)
         b.printf("Line %d; ", ix);
      printf("This should fit well:\n%s\n\n", b.str());
   }

   {
      CFmtString s;
      for(ix = 0; ix < 50; ix++)
         s.printf("Str %d; ", ix);
      printf("Everything fits in string:\n%s\n", (const char*)s);

      s.clear();
      for(ix = 0; ix < 3; ix++)
         s.printf("Str%05d; ", ix);
      printf("Everything fits in string:\n%s\n\n", (const char*)s);
   }

   CFmtFile f(stdout);
   f.printf("Testing formats:\n");
   f.printf("'%c' '%-8s' '%s' '%5d' '%05u' '0x%08x' '%6.2f' '%pp'\n", 
                              'c', "string", NULL, 256, 256, 256, 256.0, &f);
   f.printf("'%05d' '%5u' '0x%08x' '%.2e'\n", 
                              -1, -1, -1, -1.0E10);
                              
   uint32_t ip = 192<<24 | 168<<16 | 1<<8 | 100, 
            mask = 255<<24 | 255<<16 | 254<<8 | 0;
   f.printf("IP / Subnet  : %pa / %pa\n", ip, mask);
   
   IFmtOutput::registerFormatExt('M', MacAddrFmtExt);
   uint8_t amac[8] = { 0x01, 0x0A, 0x00, 0x0C, 0x01, 0x02, 0x03, 0x04 };
   uint8_t bcast[8] = { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF };
   f.printf("Some MAC     : %pM\n", amac);
   f.printf("Broadcast MAC: %pM\n", bcast);
}

#endif // _TEST_MAIN_

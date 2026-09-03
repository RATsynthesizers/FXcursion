
/***************************************************************************************************
 * Module includes
 **************************************************************************************************/

/* Native header */
#include "std_lib.h"

/* The C Standard Library <stdarg.h> header */
#include <stdarg.h>

/* The C Standard Library <stdlib.h> header */
#include <stdlib.h>

/* Definitions for memory and string functions */
#include <string.h>



/***************************************************************************************************
 * Verification of the imported configuration parameters
 **************************************************************************************************/

/* None */



/***************************************************************************************************
 * Definitions of global (public) variables
 **************************************************************************************************/

/* None */



/***************************************************************************************************
 * Declarations of local (private) data types
 **************************************************************************************************/

/* Type of printf function pointer */
typedef int (*STD_PUTCHAR_FUNC)(const int ch,
                                uint8_t* const write_data,
                                const uint32_t id);

/*! @brief Specification modifier flags for printf. */
enum std_char_flag
{
    std_char_minus                  = 0x01U,  /*!< Minus FLag. */
    std_char_plus                   = 0x02U,  /*!< Plus Flag. */
    std_char_space                  = 0x04U,  /*!< Space Flag. */
    std_char_zero                   = 0x08U,  /*!< Zero Flag. */
    std_char_pound                  = 0x10U,  /*!< Pound Flag. */
    std_char_length_char            = 0x20U,  /*!< Length: Char Flag. */
    std_char_length_short_int       = 0x40U,  /*!< Length: Short Int Flag. */
    std_char_length_long_int        = 0x80U,  /*!< Length: Long Int Flag. */
    std_char_length_long_long_int   = 0x100U, /*!< Length: Long Long Int Flag. */
};



/***************************************************************************************************
 * Definitions of local (private) constants
 **************************************************************************************************/

/*! @brief Definition the float number */
#ifndef STD_FLOAT_ENABLE
#define STD_FLOAT_ENABLE 0U
#endif /* STD_FLOAT_ENABLE */



/***************************************************************************************************
 * Definitions of static global (private) variables
 **************************************************************************************************/

/* None */



/***************************************************************************************************
 * Declarations of local (private) functions
 **************************************************************************************************/

/* This function create outputs array */
static int std_array_from_formated(STD_PUTCHAR_FUNC func_ptr,
                                   uint8_t* const write_data,
                                   const char *fmt,
                                   va_list ap);

/* Puts a new character into an array to send */
static int std_put_char_in_array(const int ch,
                                 uint8_t* const write_data,
                                 const uint32_t id);

/* This function puts padding character */
static void std_set_charesters_in_array(char c,
                                        int32_t curlen,
                                        int32_t width,
                                        int32_t *count,
                                        STD_PUTCHAR_FUNC func_ptr,
                                        uint8_t* const write_data);

/* Converts a radix number to a string and return its length */
static int32_t std_convert_radix_num_to_string(char *numstr,
                                               void *nump,
                                               int32_t neg,
                                               int32_t radix,
                                               bool use_caps);



/***************************************************************************************************
 * Definitions of global (public) functions
 **************************************************************************************************/

/**
 * @brief The sprintf function formats and stores character sets and values in buffer.
 *          Each argument (if any) preis formed and output according to the corresponding
 *          specification of the form mat in format-string
 *
 * @param p_str Pointer to the string to which the characters will be written 
 * @param p_fmt Format control string
 * @return If successful it returns the total number of characters written excluding
 *          null-character appended in the string, in case of failure a negative number is returned
 */
int std_sprintf(char* const p_str, const char *p_fmt, ...)
{
    va_list ap;
    int result = 0;

    va_start(ap, p_fmt);
    result = std_array_from_formated(std_put_char_in_array,
                                     (uint8_t*)p_str,
                                     p_fmt,
                                     ap);

    va_end(ap);

    return result;
} /* end of std_sprintf */



/**
 * @fn int8_t mem_cmp(void* const, void* const, const uint16_t)
 * 
 * @brief Compares content of the memory blocks.
 *
 * @param  p_mem1 - pointer to memory block1
 * @param  p_mem2 - pointer to memory block2
 * @param  size - memory block's size
 * 
 * @return int8_t - result of the function execution.
 *                  0 - the blocks are equal, !0 - the blocks are not equal
*/
int8_t mem_cmp(void* const p_mem1,
               void* const p_mem2,
               const uint16_t size)
{
    int16_t result = 0;
    uint16_t byte_num;
    uint8_t* p_buf1 = (uint8_t*)p_mem1;
    uint8_t* p_buf2 = (uint8_t*)p_mem2;

    for ( byte_num = 0; byte_num < size; byte_num++)
    {
        if (p_buf1[byte_num] != p_buf2[byte_num])
        {
            if (p_buf1[byte_num] < p_buf2[byte_num])
            {
                result = -((int16_t)(byte_num + 1));
            }
            else
            {
                result = ((int16_t)(byte_num + 1));
            }

            break;
        }
    }
    return result;
}



/**
 * @fn int8_t ascii_to_uint(char const* p_str,
                            const uint16_t str_length,
                            uint32_t* const p_int_value)
 *
 * @brief Compares content of the memory blocks.
 *
 * @param  p_str - pointer to ascii string
 * @param  str_length - string length
 * @param  p_int_value - pointer to store integer value
 *
 * @return int8_t - result of the function execution.
 *                  0 - success, !0 - not success
*/
int8_t ascii_to_uint(char* const p_str,
                     const uint16_t str_length,
                     uint32_t* const p_int_value)
{
    if ((NULL_PTR != p_str) &&
        (NULL_PTR != p_int_value) &&
        ((str_length <= 10U) && (str_length >= 1U)))
    {
        uint8_t numbers[10U];
        uint8_t num_cnt = 0U;
        memset(numbers, 0, SIZE_OF_ARRAY(numbers));
        for (uint8_t i = 0U; i < str_length; i++)
        {
            if ((p_str[i] >= 0x30U) && (p_str[i] <= 0x39U))
            {
                numbers[i] = p_str[i] - 0x30U;
                num_cnt++;
            }
            else
            {
                break;
            }
        }

        if (num_cnt > 0U)
        {
            uint32_t value = 0U;
            uint32_t coef = 1U;
            for (uint8_t i = num_cnt; i > 0U; i--)
            {
                value += numbers[i - 1U] * coef;
                coef = coef * 10U;
            }
            *p_int_value = value;
            return 0;
        }
        else
        {
            return -1;
        }
    }
    else
    {
        return -1;
    }
}



/**
 * @fn    void Byte2AsciiHex(const U8 nByte,
                             U8* const pAsciiBuf)
 *
 * @brief Translate byte to 2 ASCII chars in hex manner.
 *
 * @param[in] nByte - input byte.
 * @param[out] pAsciiBuf - pointer to output buffer.
 *
 * @return    None.
 */
void Byte2AsciiHex(const char nByte,
                   char* const pAsciiBuf)
{
    // Hi nibble
    U8 nChar = (U8)((nByte >> 4U) & 0x0FU);
    U8 nAsciiByte;

    const U8 ASCII_CHAR_0_OFFSET = 0x30U;
    const U8 ASCII_CHAR_A_OFFSET = 0x41U;

    if (nChar > 9U)
    {
        nAsciiByte = nChar + ASCII_CHAR_A_OFFSET - 0x0AU;
    }
    else
    {
        nAsciiByte = nChar + ASCII_CHAR_0_OFFSET;
    }

    pAsciiBuf[0U] = nAsciiByte;

    // Lo nibble
    nChar = (U8)(nByte & 0x0FU);

    if (nChar > 9U)
    {
        nAsciiByte = nChar + ASCII_CHAR_A_OFFSET - 0x0AU;
    }
    else
    {
        nAsciiByte = nChar + ASCII_CHAR_0_OFFSET;
    }

    pAsciiBuf[1U] = nAsciiByte;
}



int ip_to_bytes(char* const ip_str, char* const ip_bytes)
{
    int num, dots = 0;
    const char *ptr = ip_str;

    for (U8 i = 0U; i < 4U; i++)
    {
        num = 0U;

        // Parse each octet
        while (*ptr >= '0' && *ptr <= '9') {
            num = num * 10 + (*ptr - '0');
            if (num > 255) return 0;  // Out of range
            ptr++;
        }

        ip_bytes[i] = (unsigned char)num;

        // Expecting '.' between numbers except after the last one
        if (i < 3) {
            if (*ptr != '.') return 0;
            ptr++;
            dots++;
        }
    }

    // Ensure no extra characters
    return (*ptr == '\0' && dots == 3) ? 1 : 0;
}



/***************************************************************************************************
 * Definitions of local (private) functions
 **************************************************************************************************/

/**
 * @brief This function create outputs array
 * 
 * @param func_ptr Pointer to put data function
 * @param write_data Array for set new data
 * @param fmt_ptr Format string for printf
 * @param args_ptr Arguments to printf
 * @return Number of characters
 */
static int std_array_from_formated(STD_PUTCHAR_FUNC func_ptr,
                                   uint8_t* const write_data,
                                   const char *fmt,
                                   va_list ap)
{

    /* va_list ap; */
    const char *p;
    char c;

    char vstr[33];
    char *vstrp  = NULL;
    int32_t vlen = 0;

    bool done;
    int32_t count = 0;

    uint32_t field_width;
    uint32_t precision_width;
    char *sval;
    int32_t cval;
    bool use_caps;
    uint8_t radix = 0;

    uint32_t flags_used;
    char schar;
    bool dschar;
    int64_t ival;
    uint64_t uval = 0;
    bool valid_precision_width;

#if STD_FLOAT_ENABLE
    double fval;
#endif /* STD_FLOAT_ENABLE */

    /* Start parsing apart the format string and display appropriate formats and data. */
    p = fmt;
    while (true)
    {
        if ('\0' == *p)
        {
            break;
        }
        c = *p;
        /*
         * All formats begin with a '%' marker.  Special chars like
         * '\n' or '\t' are normally converted to the appropriate
         * character by the __compiler__.  Thus, no need for this
         * routine to account for the '\' character.
         */
        if (c != '%')
        {
            (void)func_ptr(c, write_data, count);
            count++;
            p++;
            /* By using 'continue', the next iteration of the loop is used, skipping the code that follows. */
            continue;
        }

        use_caps = true;

        /* First check for specification modifier flags. */
        flags_used = 0;
        done       = false;
        while (!done)
        {
            switch (*++p)
            {
                case '-':
                    flags_used |= (uint32_t)std_char_minus;
                    break;
                case '+':
                    flags_used |= (uint32_t)std_char_plus;
                    break;
                case ' ':
                    flags_used |= (uint32_t)std_char_space;
                    break;
                case '0':
                    flags_used |= (uint32_t)std_char_zero;
                    break;
                case '#':
                    flags_used |= (uint32_t)std_char_pound;
                    break;
                default:
                    /* We've gone one char too far. */
                    --p;
                    done = true;
                    break;
            }
        }

        /* Next check for minimum field width. */
        field_width = 0;
        done        = false;
        while (!done)
        {
            c = *++p;
            if ((c >= '0') && (c <= '9'))
            {
                field_width = (field_width * 10U) + ((uint32_t)c - (uint32_t)'0');
            }
            else if (c == '*')
            {
                field_width = (uint32_t)va_arg(ap, uint32_t);
            }
            else
            {
                /* We've gone one char too far. */
                --p;
                done = true;
            }
        }
        /* Next check for the width and precision field separator. */
        precision_width = 6U; /* MISRA C-2012 Rule 2.2 */
        valid_precision_width = false;
        if (*++p == '.')
        {
            /* Must get precision field width, if present. */
            precision_width = 0U;
            done            = false;
            while (!done)
            {
                c = *++p;
                if ((c >= '0') && (c <= '9'))
                {
                    precision_width = (precision_width * 10U) + ((uint32_t)c - (uint32_t)'0');
                    valid_precision_width = true;
                }
                else if (c == '*')
                {
                    precision_width       = (uint32_t)va_arg(ap, uint32_t);
                    valid_precision_width = true;
                }
                else
                {
                    /* We've gone one char too far. */
                    --p;
                    done = true;
                }
            }
        }
        else
        {
            /* We've gone one char too far. */
            --p;
        }
        /*
         * Check for the length modifier.
         */
        switch (/* c = */ *++p)
        {
            case 'h':
                if (*++p != 'h')
                {
                    flags_used |= (uint32_t)std_char_length_short_int;
                    --p;
                }
                else
                {
                    flags_used |= (uint32_t)std_char_length_char;
                }
                break;
            case 'l':
                if (*++p != 'l')
                {
                    flags_used |= (uint32_t)std_char_length_long_int;
                    --p;
                }
                else
                {
                    flags_used |= (uint32_t)std_char_length_long_long_int;
                }
                break;
            default:
                /* we've gone one char too far */
                --p;
                break;
        }
        /* Now we're ready to examine the format. */
        c = *++p;
        {
            if ((c == 'd') || (c == 'i') || (c == 'f') || (c == 'F') || (c == 'x') || (c == 'X') || (c == 'o') ||
                (c == 'b') || (c == 'p') || (c == 'u'))
            {
                if ((c == 'd') || (c == 'i'))
                {
                    if (0U != (flags_used & (uint32_t)std_char_length_long_long_int))
                    {
                        ival = (int64_t)va_arg(ap, int64_t);
                    }
                    else
                    {
                        ival = (int32_t)va_arg(ap, int32_t);
                    }
                    vlen  = std_convert_radix_num_to_string(vstr, &ival, 1, 10, use_caps);
                    vstrp = &vstr[vlen];
                    if (ival < 0)
                    {
                        schar = '-';
                        ++vlen;
                    }
                    else
                    {
                        if (0U != (flags_used & (uint32_t)std_char_plus))
                        {
                            schar = '+';
                            ++vlen;
                        }
                        else
                        {
                            if (0U != (flags_used & (uint32_t)std_char_space))
                            {
                                schar = ' ';
                                ++vlen;
                            }
                            else
                            {
                                schar = '\0';
                            }
                        }
                    }
                    dschar = false;
                    /* Do the ZERO pad. */
                    if (0U != (flags_used & (uint32_t)std_char_zero))
                    {
                        if ('\0' != schar)
                        {
                            (void)func_ptr(schar, write_data, count);
                            count++;
                        }
                        dschar = true;

                        std_set_charesters_in_array('0', vlen, (int32_t)field_width, &count, func_ptr, write_data);
                        vlen = (int32_t)field_width;
                    }
                    else
                    {
                        if (0U == (flags_used & (uint32_t)std_char_minus))
                        {
                            std_set_charesters_in_array(' ', vlen, (int32_t)field_width, &count, func_ptr, write_data);
                            if ('\0' != schar)
                            {
                                (void)func_ptr(schar, write_data, count);
                                count++;
                            }
                            dschar = true;
                        }
                    }
                    /* The string was built in reverse order, now display in correct order. */
                    if ((!dschar) && ('\0' != schar))
                    {
                        (void)func_ptr(schar, write_data, count);
                        count++;
                    }
                }

#if STD_FLOAT_ENABLE
                if ((c == 'f') || (c == 'F'))
                {
                    fval  = (double)va_arg(ap, double);
                    vlen  = DbgConsole_ConvertFloatRadixNumToString(vstr, &fval, 10, precision_width);
                    vstrp = &vstr[vlen];

                    if (fval < 0.0)
                    {
                        schar = '-';
                        ++vlen;
                    }
                    else
                    {
                        if (0U != (flags_used & (uint32_t)std_char_plus))
                        {
                            schar = '+';
                            ++vlen;
                        }
                        else
                        {
                            if (0U != (flags_used & (uint32_t)std_char_space))
                            {
                                schar = ' ';
                                ++vlen;
                            }
                            else
                            {
                                schar = '\0';
                            }
                        }
                    }
                    dschar = false;
                    if (0U != (flags_used & (uint32_t)std_char_zero))
                    {
                        if ('\0' != schar)
                        {
                            (void)func_ptr(schar, write_data, count);
                            count++;
                        }
                        dschar = true;
                        std_set_charesters_in_array('0', vlen, (int32_t)field_width, &count, func_ptr, write_data);
                        vlen = (int32_t)field_width;
                    }
                    else
                    {
                        if (0U == (flags_used & (uint32_t)std_char_minus))
                        {
                            std_set_charesters_in_array(' ', vlen, (int32_t)field_width, &count, func_ptr, write_data);
                            if (schar)
                            {
                                (void)func_ptr(schar, write_data, count);
                                count++;
                            }
                            dschar = true;
                        }
                    }
                    if ((!dschar) && schar)
                    {
                        (void)func_ptr(schar, write_data, count);
                        count++;
                    }
                }
#endif /* STD_FLOAT_ENABLE */
                if ((c == 'X') || (c == 'x'))
                {
                    if (c == 'x')
                    {
                        use_caps = false;
                    }
                    if (0U != (flags_used & (uint32_t)std_char_length_long_long_int))
                    {
                        uval = (uint64_t)va_arg(ap, uint64_t);
                    }
                    else
                    {
                        uval = (uint32_t)va_arg(ap, uint32_t);
                    }
                    vlen  = std_convert_radix_num_to_string(vstr, &uval, 0, 16, use_caps);
                    vstrp = &vstr[vlen];

                    dschar = false;
                    if (0U != (flags_used & (uint32_t)std_char_zero))
                    {
                        if (0U != (flags_used & (uint32_t)std_char_pound))
                        {
                            (void)func_ptr('0', write_data, count);
                            (void)func_ptr((use_caps ? 'X' : 'x'), write_data, count);
                            count += 2;
                            /*vlen += 2;*/
                            dschar = true;
                        }
                        std_set_charesters_in_array('0', vlen, (int32_t)field_width, &count, func_ptr, write_data);
                        vlen = (int32_t)field_width;
                    }
                    else
                    {
                        if (0U == (flags_used & (uint32_t)std_char_pound))
                        {
                            if (0U != (flags_used & (uint32_t)std_char_pound))
                            {
                                vlen += 2;
                            }
                            std_set_charesters_in_array(' ', vlen, (int32_t)field_width, &count, func_ptr, write_data);
                            if (0U != (flags_used & (uint32_t)std_char_pound))
                            {
                                (void)func_ptr('0', write_data, count);
                                (void)func_ptr((use_caps ? 'X' : 'x'), write_data, count);
                                count += 2;

                                dschar = true;
                            }
                        }
                    }

                    if ((0U != (flags_used & (uint32_t)std_char_pound)) && (!dschar))
                    {
                        (void)func_ptr(('0'), write_data, count);
                        (void)func_ptr((use_caps ? 'X' : 'x'), write_data, count);
                        count += 2;
                        vlen += 2;
                    }
                }
                if ((c == 'o') || (c == 'b') || (c == 'p') || (c == 'u'))
                {
                    if (0U != (flags_used & (uint32_t)std_char_length_long_long_int))
                    {
                        uval = (uint64_t)va_arg(ap, uint64_t);
                    }
                    else
                    {
                        uval = (uint32_t)va_arg(ap, uint32_t);
                    }
                    switch (c)
                    {
                        case 'o':
                            radix = 8;
                            break;
                        case 'b':
                            radix = 2;
                            break;
                        case 'p':
                            radix = 16;
                            break;
                        case 'u':
                            radix = 10;
                            break;
                        default:
                            /* MISRA C-2012 Rule 16.4 */
                            break;
                    }
                    vlen  = std_convert_radix_num_to_string(vstr, &uval, 0, (int32_t)radix, use_caps);
                    vstrp = &vstr[vlen];
                    if (0U != (flags_used & (uint32_t)std_char_zero))
                    {
                        std_set_charesters_in_array('0', vlen, (int32_t)field_width, &count, func_ptr, write_data);
                        vlen = (int32_t)field_width;
                    }
                    else
                    {
                        if (0U == (flags_used & (uint32_t)std_char_minus))
                        {
                            std_set_charesters_in_array(' ', vlen, (int32_t)field_width, &count, func_ptr, write_data);
                        }
                    }
                }
                if (vstrp != NULL)
                {
                    while ('\0' != *vstrp)
                    {
                        (void)func_ptr(*vstrp--, write_data, count);
                        count++;
                    }
                }
                if (0U != (flags_used & (uint32_t)std_char_minus))
                {
                    std_set_charesters_in_array(' ', vlen, (int32_t)field_width, &count, func_ptr, write_data);
                }
            }
            else if (c == 'c')
            {
                cval = (int32_t)va_arg(ap, uint32_t);
                (void)func_ptr(cval, write_data, count);
                count++;
            }
            else if (c == 's')
            {
                sval = (char *)va_arg(ap, char *);
                if (NULL != sval)
                {
                    if (valid_precision_width)
                    {
                        vlen = (int32_t)precision_width;
                    }
                    else
                    {
                        vlen = (int32_t)strlen(sval);
                    }
                    if (0U == (flags_used & (uint32_t)std_char_minus))
                    {
                        std_set_charesters_in_array(' ', vlen, (int32_t)field_width, &count, func_ptr, write_data);
                    }

                    if (valid_precision_width)
                    {
                        while (('\0' != *sval) && (vlen > 0))
                        {
                            (void)func_ptr(*sval++, write_data, count);
                            count++;
                            vlen--;
                        }
                        /* In case that vlen sval is shorter than vlen */
                        vlen = (int32_t)precision_width - vlen;
                    }
                    else
                    {
                        while ('\0' != *sval)
                        {
                            (void)func_ptr(*sval++, write_data, count);
                            count++;
                        }
                    }

                    if (0U != (flags_used & (uint32_t)std_char_minus))
                    {
                        std_set_charesters_in_array(' ', vlen, (int32_t)field_width, &count, func_ptr, write_data);
                    }
                }
            }
            else
            {
                (void)func_ptr(c, write_data, count);
                count++;
            }
        }
        p++;
    }
    return count;
} /* end of std_array_from_formated */



/**
 * @brief Puts a new character into an array to send
 * 
 * @param ch New charester
 * @param write_data Pointer to array
 * @param id Current element number of the array
 * @return int 1
 */
static int std_put_char_in_array(const int ch,
                                 uint8_t* const write_data,
                                 const uint32_t id)
{
    write_data[id] = ch;
    return 1;
} /* end of std_put_char_in_array */



/**
 * @brief This function puts padding character
 *
 * @param c Padding character
 * @param curlen Length of current formatted string
 * @param width Width of expected formatted string
 * @param count Number of characters
 * @param func_ptr Function to put character out
 * @param write_data Pointer to array
 */
static void std_set_charesters_in_array(char c,
                                        int32_t curlen,
                                        int32_t width,
                                        int32_t *count,
                                        STD_PUTCHAR_FUNC func_ptr,
                                        uint8_t* const write_data)
{
    int32_t i;

    for (i = curlen; i < width; i++)
    {
        (void)func_ptr(c, write_data,*count);
        (*count)++;
    }
} /* end of std_set_charesters_in_array */



/**
 * @brief Converts a radix number to a string and return its length
 *
 * @param numstr Converted string of the number
 * @param nump Pointer to the number
 * @param neg Polarity of the number
 * @param radix The radix to be converted to
 * @param use_caps Used to identify %x/X output format
 * @return Length of the converted string
 */
static int32_t std_convert_radix_num_to_string(char *numstr,
                                               void *nump,
                                               int32_t neg,
                                               int32_t radix,
                                               bool use_caps)
{
    int64_t a;
    int64_t b;
    int64_t c;

    uint64_t ua;
    uint64_t ub;
    uint64_t uc;

    int32_t nlen;
    char *nstrp;

    nlen     = 0;
    nstrp    = numstr;
    *nstrp++ = '\0';

    if (0 != neg)
    {
        a = *(int64_t *)nump;
        if (a == 0)
        {
            *nstrp = '0';
            ++nlen;
            return nlen;
        }
        while (a != 0)
        {
            b = (int64_t)a / (int64_t)radix;
            c = (int64_t)a - ((int64_t)b * (int64_t)radix);
            if (c < 0)
            {
                c = (int64_t)'0' - c;
            }
            else
            {
                c = c + '0';
            }
            a        = b;
            *nstrp++ = (char)c;
            ++nlen;
        }
    }
    else
    {
        ua = *(uint64_t *)nump;
        if (ua == 0U)
        {
            *nstrp = '0';
            ++nlen;
            return nlen;
        }
        while (ua != 0U)
        {
            ub = (uint64_t)ua / (uint64_t)radix;
            uc = (uint64_t)ua - ((uint64_t)ub * (uint64_t)radix);

            if (uc < 10U)
            {
                uc = uc + '0';
            }
            else
            {
                uc = uc - 10U + (use_caps ? 'A' : 'a');
            }
            ua       = ub;
            *nstrp++ = (char)uc;
            ++nlen;
        }
    }
    return nlen;
} /* end of std_convert_radix_num_to_string */



/****************************************** end of file *******************************************/

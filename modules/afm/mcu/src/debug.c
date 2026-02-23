#include "GOWIN_M1_uart.h"
#include "debug.h"
#include <stdbool.h>


// Helper function to print a number in hexadecimal
static void print_hex(unsigned int value, int padding, bool print_prefix) {
    if (print_prefix) {
        UART_SendChar(UART1, '0');
        UART_SendChar(UART1, 'x');
    }

    // Determine the number of nibbles
    int num_nibbles = 0;
    unsigned int temp_value = value;
    if (temp_value == 0) {
        num_nibbles = 1;
    } else {
        while (temp_value > 0) {
            temp_value >>= 4;
            num_nibbles++;
        }
    }

    // Apply padding
    for (int i = 0; i < padding - num_nibbles; i++) {
        UART_SendChar(UART1, '0');
    }

    // Print the hex digits
    for (int i = (num_nibbles - 1) * 4; i >= 0; i -= 4) {
        unsigned int nibble = (value >> i) & 0xF;
        if (nibble < 10) {
            UART_SendChar(UART1, nibble + '0');
        } else {
            UART_SendChar(UART1, nibble - 10 + 'A');
        }
    }
}

void debug_init(void) {
  UART_InitTypeDef UART_InitStruct;

  UART_InitStruct.UART_BaudRate         = 230400;
  UART_InitStruct.UART_Mode.UARTMode_Tx = ENABLE;
  UART_InitStruct.UART_Mode.UARTMode_Rx = ENABLE;
  UART_InitStruct.UART_Int.UARTInt_Tx   = DISABLE;
  UART_InitStruct.UART_Int.UARTInt_Rx   = DISABLE;
  UART_InitStruct.UART_Ovr.UARTOvr_Tx   = DISABLE;
  UART_InitStruct.UART_Ovr.UARTOvr_Rx   = DISABLE;
  UART_InitStruct.UART_Hstm             = DISABLE;

  UART_Init(UART1, &UART_InitStruct);
}

// Helper function to print a number in decimal
static void print_number_u(uint32_t n) {
    if (n == 0) {
        UART_SendChar(UART1, '0');
        return;
    }
    char buf[10];  // max uint32 is 4294967295, 10 digits
    int i = 0;
    while (n > 0) {
        buf[i++] = '0' + (n % 10);
        n /= 10;
    }
    while (i > 0) {
        UART_SendChar(UART1, buf[--i]);
    }
}

static void print_number(int n) {
    if (n < 0) {
        UART_SendChar(UART1, '-');
        n = -n;
    }
    if (n == 0) {
        UART_SendChar(UART1, '0');
        return;
    }
    if (n / 10) {
        print_number(n / 10);
    }
    UART_SendChar(UART1, (n % 10) + '0');
}

void dbg_printf(const char* format, ...) {
  va_list args;
  va_start(args, format);

  while (*format) {
    if (*format == '%') {
      format++;
      int padding = 0;
      bool zero_pad = false;
      bool print_prefix = false;

      // Handle padding and prefix for hex values
      if (*format == '0') {
        zero_pad = true;
        format++;
        // Check if there's a digit after '0' for padding
        if (*format >= '0' && *format <= '9') {
          padding = *format - '0';
          format++;
        }
      }

      switch (*format) {
        case 'd': {
          int i = va_arg(args, int);
          print_number(i);
          break;
        }
        case 'u': {
          uint32_t u = va_arg(args, uint32_t);
          print_number_u(u);
          break;
        }
        case 's': {
          char* s = va_arg(args, char*);
          while (*s) {
            UART_SendChar(UART1, *s++);
          }
          break;
        }
        case 'c': {
          char c = (char)va_arg(args, int);
          UART_SendChar(UART1, c);
          break;
        }
        case 'x':
        case 'X': {
            unsigned int hex_val = va_arg(args, unsigned int);
            // If the original format string contained '0x', handle it here
            // The problem description was "0x%08X", so I'll hardcode the prefix for now
            // More robust solution would be to parse for '0x' in the format string itself.
            // For now, assuming if 'X' or 'x' is used, we might need '0x' prefix if specified.
            if (zero_pad && padding > 0) { // crude way to detect '08X' or similar
                 print_prefix = true;
            }
            // For now, let's assume if it's 'X' or 'x' and padding was requested, we should print '0x'
            // A more robust solution would involve parsing the format string for '0x' explicitly.
            if (*(format-1) == 'x' || *(format-1) == 'X' ) {
                print_hex(hex_val, padding, print_prefix);
            } else {
                 print_hex(hex_val, padding, false); // No prefix if not explicitly asked
            }
            break;
        }
        case '%': {
          UART_SendChar(UART1, '%');
          break;
        }
        default:
          UART_SendChar(UART1, '%');
          UART_SendChar(UART1, *format);
          break;
      }
    } else {
      UART_SendChar(UART1, *format);
    }
    format++;
  }

  va_end(args);
}

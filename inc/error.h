#ifndef INC_ERROR_H
#define INC_ERROR_H

enum {
  // Kernel error codes, keep in sync with the list in lib/printfmt.c
  E_UNSPECIFIED = 1, // Unspecified or unknown problem
  E_BAD_ENV, // Environment doesn't exist or otherwise cannot be used
  E_INVAL, // Invalid parameter
  E_NO_MEM, // Request failed due to memory shortage
  E_NO_FREE_ENV, // Attemtp to create a new environment beyond the maximum
  E_FAULT, // Memory fault
  MAXERROR
};

#endif // INC_ERROR_H

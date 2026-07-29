#pragma once

#ifndef MY_STDINT_H
#define MY_STDINT_H

/*
 * Portable stdint.h for a hypothetical standalone OS.
 * This is a simplified example; a real version needs
 * to be more comprehensive and handle different platform variations.
 */

// Basic integer types based on platform assumptions
#if defined(__x86_64__) || defined(_WIN64)
  typedef signed char int8_t;
  typedef unsigned char uint8_t;
  typedef short int16_t;
  typedef unsigned short uint16_t;
  typedef int int32_t;
  typedef unsigned int uint32_t;
  typedef long long int64_t;
  typedef unsigned long long uint64_t;

  // 64-bit pointer and size definitions
  typedef long long intptr_t;
  typedef unsigned long long uintptr_t;
  //typedef unsigned long long size_t;  STDDEF
  typedef long long ssize_t;
#elif defined(__i386__) || defined(_WIN32)
  typedef char int8_t;
  typedef unsigned char uint8_t;
  typedef short int16_t;
  typedef unsigned short uint16_t;
  typedef int int32_t;
  typedef unsigned int uint32_t;
  typedef long long int64_t; // May depend on the platform's long long size
  typedef unsigned long long uint64_t;

   // 32-bit pointer and size definitions
  typedef int intptr_t;
  typedef unsigned int uintptr_t;
  typedef unsigned int size_t;
  typedef int ssize_t;
#else
  #error "Unsupported architecture for custom stdint.h"
#endif

// Add integer types for minimum/maximum sizes and pointer-related types
// ... (e.g., INT8_MIN, INT8_MAX, UINTPTR_MAX, etc.)

#endif // MY_STDINT_H


//typedef signed char int8_t;
//typedef unsigned char uint8_t;
//typedef signed short int16_t;
//typedef unsigned short uint16_t;
//typedef signed long int int32_t;
//typedef unsigned long int uint32_t;
//typedef signed long long int int64_t;
//typedef unsigned long long uint64_t;
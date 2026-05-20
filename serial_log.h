#pragma once

#include <stdio.h>

#ifndef POLSAT_ENABLE_SERIAL
#define POLSAT_ENABLE_SERIAL 0
#endif

#if POLSAT_ENABLE_SERIAL
#define LOG_PRINTF(...) printf(__VA_ARGS__)
#else
#define LOG_PRINTF(...) ((void)0)
#endif

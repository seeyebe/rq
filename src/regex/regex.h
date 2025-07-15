#ifndef REGEX_H
#define REGEX_H

#include <stdbool.h>
#include <stddef.h>
#include "re.h"

re_t regex_compile(const char* pattern);

bool regex_match(const re_t regex, const char* text);

void regex_free(re_t regex);

bool regex_test(const char* pattern, const char* text);

#endif

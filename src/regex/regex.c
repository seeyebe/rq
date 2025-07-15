#include "regex.h"
#include "re.h"


re_t regex_compile(const char* pattern) {
    if (!pattern) return NULL;
    return re_compile(pattern);
}

bool regex_match(const re_t regex, const char* text) {
    if (!regex || !text) return false;

    int matchlength = 0;
    int result = re_matchp(regex, text, &matchlength);
    return result >= 0;
}

void regex_free(re_t regex) {
    (void)regex;
}

bool regex_test(const char* pattern, const char* text) {
    if (!pattern || !text) return false;

    int matchlength = 0;
    int result = re_match(pattern, text, &matchlength);
    return result >= 0;
}

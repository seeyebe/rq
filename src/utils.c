#include "utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

int parse_size_arg(const char *arg, uint64_t *size) {
    if (!arg || !size) return -1;

    char *endptr;
    *size = strtoull(arg, &endptr, 10);

    if (*endptr != '\0') {
        uint64_t multiplier = 1;
        switch (*endptr) {
            case 'K': case 'k': multiplier = 1024; break;
            case 'M': case 'm': multiplier = 1024 * 1024; break;
            case 'G': case 'g': multiplier = 1024 * 1024 * 1024; break;
            case 'T': case 't': multiplier = 1024ULL * 1024 * 1024 * 1024; break;
            default: return -1;
        }
        *size *= multiplier;
    }

    return 0;
}


int parse_date_string(const char *date_str, FILETIME *file_time) {
    if (!date_str || !file_time) {
        return -1;
    }

    int year, month, day;
    if (sscanf(date_str, "%d-%d-%d", &year, &month, &day) != 3) {
        return -1;
    }

    if (year < 1601 || year > 3000 || month < 1 || month > 12 || day < 1 || day > 31) {
        return -1;
    }

    SYSTEMTIME st = {0};
    st.wYear = year;
    st.wMonth = month;
    st.wDay = day;
    st.wHour = 0;
    st.wMinute = 0;
    st.wSecond = 0;
    st.wMilliseconds = 0;

    if (!SystemTimeToFileTime(&st, file_time)) {
        return -1;
    }

    return 0;
}

void format_filetime_iso(const FILETIME *file_time, char *buffer, size_t buffer_size) {
    if (!file_time || !buffer || buffer_size < 20) {
        return;
    }

    SYSTEMTIME st;
    if (!FileTimeToSystemTime(file_time, &st)) {
        strcpy(buffer, "invalid-date");
        return;
    }

    snprintf(buffer, buffer_size, "%04d-%02d-%02dT%02d:%02d:%02d",
             st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond);
}
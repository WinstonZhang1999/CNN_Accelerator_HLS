/* Copyright 2021 Columbia University, SLD Group */

#include "systemc.h"
#include "cynw_flex_channels.h"

#ifndef __UTILS_HPP__
#define __UTILS_HPP__

/* Macros to report information to the users. */
#define REPORT_TIME(time, ...)                          \
    {                                                   \
        std::stringstream _ss; _ss << time;             \
        std::string _s = _ss.str();                     \
        const char * _time = _s.c_str();                \
        printf( "Info: %s: ", sc_object::basename());   \
        printf( "@%s ", _time);                         \
        printf( __VA_ARGS__);                           \
        printf( "\n\n");                                \
    }


#ifndef STRATUS_HLS

#define REPORT_INFO(...)                                        \
    {                                                           \
        fprintf(stderr,  "Info: %s: ", sc_object::basename());  \
        fprintf(stderr,  __VA_ARGS__);                          \
        fprintf(stderr,  "\n\n");                               \
    }

#define REPORT_ERROR(...)                                       \
    {                                                           \
        fprintf(stderr,  "Error: %s: ", sc_object::basename()); \
        fprintf(stderr,  __VA_ARGS__);                          \
        fprintf(stderr,  "\n\n");                               \
    }

#else /* STRATUS_HLS */

#define REPORT_INFO(...)                                \
    {                                                   \
        printf( "Info: %s: ", sc_object::basename());   \
        printf( __VA_ARGS__);                           \
        printf( "\n\n");                                \
    }

#define REPORT_ERROR(...)                               \
    {                                                   \
        printf( "Error: %s: ", sc_object::basename());  \
        printf( __VA_ARGS__);                           \
        printf( "\n\n");                                \
    }

#endif /* STRATUS_HLS */



/* Print the following info only if debug is selected. */
#if defined(DEBUG)

#ifndef STRATUS_HLS

#define REPORT_DEBUG(...)                                       \
    {                                                           \
        fprintf(stderr, "Debug: %s: ", sc_object::basename());  \
        fprintf(stderr, __VA_ARGS__);                           \
        fprintf(stderr, "\n\n");                                \
    }

#else /* STRATUS_HLS */

#define REPORT_DEBUG(...)

#endif /* STRATUS_HLS */

#else /* !DEBUG */

#define REPORT_DEBUG(...)

#endif /* DEBUG */



#endif /* __UTILS_HPP__ */

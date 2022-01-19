#include "datatypes.hpp"

#ifndef __FC_LAYER_DIRECTIVES_H__
#define __FC_LAYER_DIRECTIVES_H__


#ifdef STRATUS_HLS

#ifdef SMALL

#elif MEDIUM

#elif FAST

#else

#error HLS configuration is unknown

#endif // HLS configurations

#else // STRATUS not defined

#endif // STRATUS


#endif // __FC_LAYER_DIRECTIVES_H__

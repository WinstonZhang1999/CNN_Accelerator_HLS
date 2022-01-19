#ifndef _FPDATA_H_
#define _FPDATA_H_

// Select word lenght and integer lenght for fixed-point data types

#ifdef NATIVE //DO NOT MODIFY
// Generic inputs

#define FPDATA_WL 32
#define FPDATA_IL 17


// Input weights

#define W_FPDATA_WL FPDATA_WL
#define W_FPDATA_IL 16

#else //FIXED - YOU CAN MODIFY

// Generic inputs

#define FPDATA_WL 32
#define FPDATA_IL 17


// Input weights

#define W_FPDATA_WL FPDATA_WL
#define W_FPDATA_IL 16

#endif

#endif // _FPDATA_H_


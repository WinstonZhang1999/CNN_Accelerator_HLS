/* Copyright 2021 Columbia University SLD Group */

#include "fpdata.hpp"

#ifndef __DATATYPES_HPP__
#define __DATATYPES_HPP__

#ifdef FIXED

// Stratus fixed-point (HLS-r)
#include <cynw_fixed.h>

// Define conversion functions between bit-vector and floating point types
// Note that the signature of these function is such that a conversion can
// be implemented for both floating point and fixed point data types.

#define FPDATA cynw_fixed<FPDATA_WL, FPDATA_IL>
#define W_FPDATA cynw_fixed<W_FPDATA_WL, W_FPDATA_IL>

typedef sc_dt::sc_uint<FPDATA_WL> FPDATA_WORD;
typedef sc_dt::sc_uint<W_FPDATA_WL> W_FPDATA_WORD;

// cynw_interpret going to a bit vector
inline void cynw_interpret_float(const FPDATA& in, FPDATA_WORD& out)
{ out.range(FPDATA_WL-1,0) = in.range(FPDATA_WL-1,0); }

inline void cynw_interpret_float(const W_FPDATA& in, W_FPDATA_WORD& out)
{ out.range(W_FPDATA_WL-1,0) = in.range(W_FPDATA_WL-1,0); }


// cynw_interpret going from a bit vector
inline void cynw_interpret_float(const FPDATA_WORD& in, FPDATA& out)
{ out.range(FPDATA_WL-1,0) = in.range(FPDATA_WL-1,0); }

inline void cynw_interpret_float(const W_FPDATA_WORD& in, W_FPDATA& out)
{ out.range(W_FPDATA_WL-1,0) = in.range(W_FPDATA_WL-1,0); }


// Conversion from bit-vector to fp
template<typename T, size_t FP_WL, size_t BV_L>
void bv2fp(sc_dt::sc_bv<BV_L> data_in, T &data_out)
{
	// Copy data
	for (unsigned i = 0; i < FP_WL; i++)
	{
		HLS_UNROLL_LOOP(ON, "bv2fp");
		data_out[i] = data_in[i].to_bool();
	}
}

// Conversion from fp to bit-vector
template<typename T, size_t FP_WL, size_t BV_L>
void fp2bv(T data_in, sc_dt::sc_bv<BV_L> &data_out)
{
	// Copy data
	for (unsigned i = 0; i < FP_WL; i++)
	{
		HLS_UNROLL_LOOP(ON, "fp2bv");
		data_out[i] = (bool)data_in[i];
	}
	// Extend sign
	for (unsigned i = FP_WL; i < BV_L; i++)
	{
		HLS_UNROLL_LOOP(ON, "fp2bv_s_ext");
		data_out[i] = (bool)data_in[FP_WL - 1];
	}
}

// Conversion from fixed-point to native floating point
inline void fp2native(const FPDATA& in, float& out)
{ out = in.to_double(); }

inline void fp2native(const W_FPDATA& in, float& out)
{ out = in.to_double(); }

#else // NATIVE

#define FPDATA float
#define W_FPDATA float

typedef sc_dt::sc_uint<32> FPDATA_WORD;
typedef sc_dt::sc_uint<32> W_FPDATA_WORD;

// cynw_interpret going to a bit vector
inline void cynw_interpret_float(const FPDATA& in, FPDATA_WORD& out)
{ uint32_t *ptr = (uint32_t *) &in; out = *ptr; }

// cynw_interpret going from a bit vector
inline void cynw_interpret_float(const FPDATA_WORD& in, FPDATA& out)
{ uint32_t data = in.to_uint(); float *ptr = (float *) &data; out = *ptr; }

template<typename T, size_t FP_WL, size_t BV_L>
void bv2fp(sc_dt::sc_bv<BV_L> data_in, T &data_out)
{ uint32_t data = data_in.to_uint(); T *ptr = (T *) &data; data_out = *ptr; }

template<typename T, size_t FP_WL, size_t BV_L>
void fp2bv(T data_in, sc_dt::sc_bv<BV_L> &data_out)
{ uint32_t *ptr = (uint32_t *) &data_in; data_out = sc_dt::sc_bv<BV_L>(*ptr); }

inline void fp2native(const float& in, float& out)
{ out = in; }

#endif // FIXED

/* Memory alignment adjustment factor */
#define DMA_ADJ (DATA_WIDTH / FPDATA_WL)

#endif /* __DATATYPES_HPP__ */

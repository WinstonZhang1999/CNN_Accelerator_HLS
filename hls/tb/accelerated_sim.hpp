/* Copyright 2021 Columbia University, SLD Group */

#ifndef __ACCELERATED_SIM_H__
#define __ACCELERATED_SIM_H__

#define REDUCED_CHANS(_l)			\
	(					\
		(_l ==  6) ?    10 :		\
		(_l ==  5) ?    64 :		\
		(_l ==  4) ?    32 :		\
		(_l ==  3) ?    16 :		\
		(_l ==  2) ?    4 :		\
		3				\
	)

#define REDUCED_CHANS_NATIVE(_l)		\
	(					\
		(_l ==  6) ?    10 :		\
		(_l ==  5) ?    64 :		\
		(_l ==  4) ?    32 :		\
		(_l ==  3) ?    16 :		\
		(_l ==  2) ?    4 :		\
		3				\
	)

#endif // __ACCELERATED_SIM_H__

#ifndef STDBOOL_H
#define STDBOOL_H

#ifndef __cplusplus

#define bool _Bool
#define true 1
#define false 0

#else

/* Support for C++ just in case */
#define _Bool bool
#define true true
#define false false

#endif

#define __bool_true_false_are_defined 1

#endif /* STDBOOL_H */

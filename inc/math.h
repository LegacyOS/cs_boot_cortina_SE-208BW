#ifndef	_MATH_H
#define _MATH_H	1

extern int MathTest(int key);
extern int DoMathTest(int j, double x, double Result);

extern double sw_copysign(double x, double y);
extern double fabs(double x);
extern double scal_bn(double x, int n);
extern int __ieee754_rem_pio2(double x, double *y);
extern int __kernel_rem_pio2(double *x, double *y, int e0, int nx, int prec, const int *ipio2);
extern double __kernel_cos(double x, double y);
extern double __kernel_sin(double x, double y, int iy);
extern double __kernel_tan(double x, double y, int iy);
extern double sw_sin(double x);
extern double sw_cos(double x);
extern double sw_tan(double x);
#endif 

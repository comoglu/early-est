/*************************************************************************/
/*** Complex Arithmetic ***/
/*************************************************************************/

#include <math.h>
#include "complex.h"

fcomplex Cadd(fcomplex a, fcomplex b)
{
	fcomplex c;
	c.r = a.r + b.r;
	c.i = a.i + b.i;
	return c;
}

fcomplex Csub(fcomplex a, fcomplex b)
{
	fcomplex c;
	c.r = a.r - b.r;
	c.i = a.i - b.i;
	return c;
}

fcomplex Cmul(fcomplex a, fcomplex b)
{
	fcomplex c;
	c.r = a.r * b.r - a.i * b.i;
	c.i = a.i * b.r + a.r * b.i;
	return c;
}

fcomplex Cdiv(fcomplex a, fcomplex b)
{
	fcomplex c;
	REALTYPE r, den;
	if (fabs(b.r) >= fabs(b.i)) {
		r = b.i / b.r;
		den = b.r + r * b.i;
		c.r = (a.r + r * a.i) / den;
		c.i = (a.i - r * a.r) / den;
	} else {
		r = b.r / b.i;
		den = b.i + r * b.r;
		c.r = (a.r * r + a.i) / den;
		c.i = (a.i * r - a.r) / den;
	}
	return c;
}

fcomplex Cmplx(REALTYPE re, REALTYPE im)
{
	fcomplex c;
	c.r = re;
	c.i = im;
	return c;
}

REALTYPE Creal(fcomplex z)
{
	REALTYPE r;
	return (r = z.r);
}

REALTYPE Cimag(fcomplex z)
{
	REALTYPE r;
	return (r = z.i);
}

REALTYPE Cmag(fcomplex z)
{
	return (sqrt(z.r * z.r + z.i * z.i));
}

REALTYPE Cphs(fcomplex z)
{
	if (z.r == 0.0) {
		if (z.i == 0.0)
			return (0.0);
		else
			return ((z.i / fabs(z.i)) * 2.0 * atan(1.0));
	} else {
		return (atan2(z.i, z.r));
	}
}

fcomplex Conjg(fcomplex z)
{
	fcomplex c;
	c.r = z.r;
	c.i = -z.i;
	return c;
}

fcomplex Csqrt(fcomplex z)
{
	fcomplex c;
	REALTYPE x,y,w,r;
	if ((z.r == 0.0) && (z.i == 0.0)) {
		c.r=(c.i=0.0);
		return(c);
	} else {
		x = fabs(z.r);
		y = fabs(z.i);
		if (x >= y) {
			r = y / x;
			w = sqrt(x) * sqrt(0.5 * (1.0 + sqrt(1.0 + r * r)));
		} else {
			r = x / y;
			w = sqrt(y) * sqrt(0.5 * (r + sqrt(1.0 + r * r)));
		}
		if (z.r >= 0.0) {
			c.r = w;
			c.i = z.i / (2.0 * w);
		} else {
			c.i = (z.i >= 0.0) ? w : -w;
			c.r = z.i / (2.0 * c.i);
		}
		return(c);
	}
}


/*** End of Complex Arithmetic ***/
/*************************************************************************/




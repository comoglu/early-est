/*************************************************************************/
/*** Complex Arithmetic Include file ***/
/*************************************************************************/

typedef double REALTYPE;

typedef struct FCOMPLEX {REALTYPE r, i;} fcomplex;

fcomplex Cadd(fcomplex a, fcomplex b);
fcomplex Csub(fcomplex a, fcomplex b);
fcomplex Cmul(fcomplex a, fcomplex b);
fcomplex Cdiv(fcomplex a, fcomplex b);
fcomplex Cmplx(REALTYPE re, REALTYPE im);
REALTYPE Creal(fcomplex z);
REALTYPE Cimag(fcomplex z);
REALTYPE Cmag(fcomplex z);
REALTYPE Cphs(fcomplex z);
fcomplex Conjg(fcomplex z);
fcomplex Csqrt(fcomplex z);

/*** End of Complex Arithmetic ***/
/*************************************************************************/






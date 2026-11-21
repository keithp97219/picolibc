# `<math.h>`, `<complex.h>`, `<fenv.h>`: audit against POSIX.1-2024 (Issue 8), POSIX additions and error reporting

Worktree `/local/home/keithpac/audit-math`, branch `audit/math`, based on 1bac8ef806 (`claude/posix-spec-audit`). Host: AWS Graviton2, aarch64 Linux 5.10. Builds and tests ran in the Debian sid chroot (gcc 16.2, glibc 2.43, clang 21.1.8) as an unprivileged user.

**Scope.** What POSIX adds to ISO C for these headers, and error reporting, not accuracy.
- `<math.h>`: the types and macros (float_t, double_t, the classification macros, HUGE_VAL*, INFINITY, NAN, FP_*, FP_FAST_FMA*, FP_ILOGB0/FP_ILOGBNAN, MATH_ERRNO/MATH_ERREXCEPT, math_errhandling), the 36 XSI M_ constants, MAXFLOAT (removed in Issue 8), signgam, every Issue 8 prototype, and the namespace in strict and XSI mode. For each function, RETURN VALUE and ERRORS: domain, pole, overflow and underflow cases, errno and the exceptions according to math_errhandling, and the special values, for double, float and long double. The XSI Bessel functions and lgamma/signgam.
- `<fenv.h>`: the exception, rounding and environment functions, FE_DFL_ENV, the macros, the namespace, and what the rounding mode does to arithmetic and to the functions that follow it. Checked on aarch64 (128-bit software long double) and armhf (VFP, long double is double), with gcc and with clang linking libgcc or compiler-rt.
- `<complex.h>`, lightly: the macros, CMPLX, the prototypes, and the special values of cabs, carg, cproj, csqrt and clog on their branch cuts.
- Not in Issue 8, so checked only for namespace: the C23 functions, the float Bessel functions, finite, drem, gamma, lgamma_r, significand, infinity, scalb, pow10, sincos, and feenableexcept/fedisableexcept/fegetexcept.

**Implementation.**
- Headers: `libc/include/math.h`, `fenv.h`, `complex.h`, `machine/fenv-softfloat.h`.
- aarch64 and arm machine headers: `libc/machine/aarch64/machine/{fenv.h,fenv-fp.h,math.h}`, `libc/machine/arm/machine/{fenv.h,fenv-fp.h}`.
- libm: `libm/common` (logb, remquo, the `math_err_*.c` helpers, the lrint family), `libm/math` (Bessel, lgamma), `libm/ld`, `libm/complex`, `libm/fenv`, `libm/machine/aarch64`.

**Tests.**
- The existing `test/test-math/test-math-errhandling.c` (with `-tests.c`) already checks errno and the exceptions for most functions and all three types. It is cited below as "errh:LINE", the line in test-math-errhandling-tests.c's table.
- errh checks errno only with `-Dwant-math-errno=true`, which CI uses. The default build here has `__IEEE_LIBM`, so I added `builds/build-aarch64-errno` and `builds/build-arm-errno`. These are targeted builds: the new tests plus test-math-errhandling.
- New tests are in `test/test-posix/test-math/`, `test-fenv/` and `test-complex/`. File names below drop `test-`; line numbers are for the branch tip.
- The new tests check errno only when `math_errhandling & MATH_ERRNO`, and exceptions only when `& MATH_ERREXCEPT`. A "may fail" error passes if it is reported correctly or not at all, but no other error is accepted.
- The picolibc builds of the function tests use `-fno-builtin` and volatile operands, so the library code actually runs.

**Spec source.** pubs.opengroup.org/onlinepubs/9799919799:
- basedefs/math.h, fenv.h, complex.h;
- functions/ for every `<math.h>` function above, plus j0, y0, jn, lgamma, signgam, the eleven fenv functions, CMPLX, cabs, carg, cproj, csqrt, clog and cpow;
- XSH 2.2.2: M_ is reserved only for XSI `<math.h>`; FP_[A-Z] and FE_[A-Z] are header prefixes.

**Status key.** TESTED (file:line), UNTESTED, NOT TESTABLE (reason). Every TESTED row was verified by running on aarch64-linux-gnu and arm-linux-gnueabihf, for picolibc and, unless noted, the `-native` glibc build. The errno parts were also run in both errno builds. "(new)" marks checks this unit added; "(existing)" marks checks that were already there.

## Requirement table

### `<math.h>` header

| # | Requirement | Status | Reference |
|---|---|---|---|
| H1 | float_t, double_t follow FLT_EVAL_METHOD | TESTED (new) | math_h.c:58-69 |
| H2 | Classification and comparison macros are macros returning int | TESTED (new) | math_h.c:72-77, 381-383 |
| H3 | HUGE_VAL/F/L: double/float/long double, +Inf | TESTED (new) | math_h.c:83-85, 365-367 |
| H4 | INFINITY float +Inf; NAN float quiet NaN | TESTED (new) | math_h.c:86-87, 368-369; remquo-type.h:168-181 |
| H5 | FP_INFINITE/NAN/NORMAL/SUBNORMAL/ZERO distinct integer constants, returned by fpclassify | TESTED (new) | math_h.c:95-112, 371-380 |
| H6 | FP_FAST_FMA* expand to the integer constant 1 when defined | TESTED (new) | math_h.c:114-121. **Deviation 2, fixed** |
| H7 | FP_ILOGB0 ∈ {INT_MIN, -INT_MAX}, FP_ILOGBNAN ∈ {INT_MAX, INT_MIN}; ilogb returns them | TESTED (new) | math_h.c:127-128, 385-387 |
| H8 | MATH_ERRNO 1, MATH_ERREXCEPT 2 | TESTED (new) | math_h.c:131-133 |
| H9 | math_errhandling: int, value 1, 2 or 3 | TESTED (new) | math_h.c:134, 389-399 (0 accepted only under `__PICOLIBC__` with a message; see Deviation 11) |
| H10 | FE_DIVBYZERO, FE_INVALID, FE_OVERFLOW defined when MATH_ERREXCEPT can be set | TESTED (new) | math_h.c:137-141, 400-401 |
| H11 | 18 XSI double M_ constants, type double, accurate | TESTED (new; the Issue 8 additions picolibc only) | math_h.c:144-229, 403. **Deviation 1, fixed** |
| H12 | 18 long double M_*l constants, accurate | TESTED (new; picolibc only, glibc has them only with _GNU_SOURCE) | same. **Deviation 1, fixed** |
| H13 | (double)M_PI == M_PI | TESTED (new) | math_h.c:407 |
| H14 | MAXFLOAT removed (Defect 1330) | TESTED (new; picolibc only, glibc defines it) | math_h.c:231-237. **Deviation 1, fixed** |
| H15 | extern int signgam (XSI) | TESTED (new) | math_h.c:248 |
| H16 | Every prototype in the page, XSI ones with _XOPEN_SOURCE 800 | TESTED (new) | math_h.c:251-351 (`_Generic` on each function's address) |
| H17 | XSI mode declares no j0f ... ynf | TESTED (new) | math_h.c:240-245. **Deviation 9, fixed** |
| H18 | Strict POSIX: no M_ constants, MAXFLOAT, Bessel functions, signgam | TESTED (new) | math_h-names.c:48-64 |
| H19 | Strict POSIX: no extensions or removed interfaces (j0f, ynf, finite, drem, gamma, lgamma_r, significand, infinity, scalb, pow10, sincos, isinff/isnanf/isfinitef) | TESTED (new) | math_h-names.c:67-82 |
| H20 | Strict C17 POSIX: no C23 names (exp10, sinpi, iszero, issignaling, FP_INT_*) | TESTED (new) | math_h-names.c:85-91 |
| H21 | "Each function shall execute as if it were a single operation without ... externally visible exceptional conditions" | NOT TESTABLE | not observable beyond the per-function checks |

### Error reporting per function (double, float, long double)

errh's long double exception checks are skipped where fenv.h says `__LONG_DOUBLE_NOEXCEPT` (compiler-rt, or no FPU).

| # | Requirement | Status | Reference |
|---|---|---|---|
| E1 | acos, asin: domain error for \|x\|>1 and ±Inf | TESTED (existing) | errh:4114-4131 |
| E2 | acosh: domain error below 1 and at -Inf | TESTED (existing) | errh:4120-4124 |
| E3 | Subnormal x may be a range error (asin, asinh, atan, atanh, expm1, sinh, tanh) | TESTED (existing; new for atan) | errh:4921-4931; remquo-type.h:207-229 |
| E4 | atan(±0)=±0, atan(±Inf)=±π/2 | TESTED (new) | remquo-type.h:207-229 |
| E5 | atanh: pole at ±1, domain error beyond | TESTED (existing) | errh:4146-4151 |
| E6 | cos/sin/tan: domain error at ±Inf | TESTED (existing) | errh:4165-4168, 4810-4816, 4853-4920 |
| E7 | Overflow is a required range error giving ±HUGE_VAL (cosh, sinh, exp, exp2, expm1, pow, hypot, ldexp, scalbn) | TESTED (existing) | errh:4175-4179, 4201-4213, 4222, 4477, 4517, 4727 |
| E8 | Underflow may be a range error, giving 0 or the correct subnormal | TESTED (existing; new for erfc) | errh:4143, 4213, 4516, 4728; remquo-type.h:231-244 |
| E9 | erfc(±0)=1, erfc(-Inf)=2, erfc(+Inf)=+0, erfc(NaN)=NaN | TESTED (new) | remquo-type.h:231-244 |
| E10 | fdim overflow is a required range error | TESTED (existing) | errh:4287 |
| E11 | fma: Inf*0+z and Inf-Inf are domain errors (EDOM); overflow is a range error (ERANGE) | TESTED (existing; exceptions only) | errh:4302-4325. **Deviation 10, not fixed** |
| E12 | fmax/fmin: one NaN argument returns the other | TESTED (new) | remquo-type.h:279-285 |
| E13 | fmod: domain error for x infinite or y zero | TESTED (existing) | errh:4447-4455 |
| E14 | frexp: ±0 gives ±0 with *exp=0; ±Inf and NaN returned; subnormals | TESTED (new) | remquo-type.h:48-63 |
| E15 | ilogb(0/±Inf/NaN): required domain error (XSI), returning FP_ILOGB0/INT_MAX/FP_ILOGBNAN | TESTED (existing; new in test-posix) | errh:5004-5008; remquo-type.h:148-166 |
| E16 | lgamma: poles at 0 and negative integers return +HUGE_VAL; overflow is a range error; 1 and 2 give +0; ±Inf give +Inf | TESTED (existing; new for the sign of zero and every pole) | errh:4521-4551; signgam.c:192-245 |
| E17 | log, log10, log2: pole at 0, domain error below | TESTED (existing) | errh:4553-4596 |
| E18 | log1p: pole at -1, domain error below -1 and at -Inf, ±0 and +Inf returned, subnormal | TESTED (new) | remquo-type.h:190-205; errh:4569-4574 |
| E19 | logb(±0): required pole error (ERANGE, divide-by-zero), -HUGE_VAL | TESTED (new; errno picolibc only) | errh:4602-4603; remquo-type.h:148-157. **Deviation 4, fixed** |
| E20 | lrint/llrint/lround/llround: NaN, ±Inf and out-of-range are required domain errors (EDOM) | TESTED (existing; exceptions only) | errh:5016-5060. **Deviation 10, not fixed** |
| E21 | modf: ±Inf gives ±0 with *iptr ±Inf; NaN gives NaN in both; signed zero fraction | TESTED (new) | remquo-type.h:65-75 |
| E22 | nan(): quiet NaN for any tagp | TESTED (new) | remquo-type.h:168-181 |
| E23 | nearbyint: keeps the sign, ±0 and ±Inf returned, no inexact | TESTED (new) | remquo-type.h:245-277 |
| E24 | nextafter/nexttoward overflow is a required range error | TESTED (existing) | errh:4732-4785 |
| E25 | pow poles, domain and range errors | TESTED (existing) | errh:4618-4680 |
| E26 | remainder: domain error for x infinite or y zero | TESTED (existing) | errh:4689-4698 |
| E27 | remquo: required domain error (EDOM) for x infinite or y zero; NaN arguments give NaN; *quo's sign and low bits | TESTED (new; errno picolibc only) | remquo-type.h:77-101. **Deviation 5, fixed** |
| E28 | rint/round/ceil/floor/trunc keep the sign, pass ±0/±Inf/NaN, round halves away from zero | TESTED (new; existing for rint, round, floor) | remquo-type.h:245-277 |
| E29 | scalbln/scalbn: overflow is a required range error (±HUGE_VAL); underflow gives 0 or the subnormal; NaN, ±0, ±Inf and n=0 return x | TESTED (new; existing for scalbn overflow) | remquo-type.h:103-146 |
| E30 | sqrt domain error | TESTED (existing) | errh:4840-4849 |
| E31 | tgamma poles, domain errors, overflow | TESTED (existing) | errh:4882-4895 |
| E32 | copysign with zero, Inf and NaN | TESTED (new) | remquo-type.h:182-188 |
| E33 | Functions with "No errors are defined" report none | TESTED (new and existing) | the no_error calls throughout remquo-type.h; errh:4158-4163, 4271-4278 |
| E34 | MX: exact and independent of the rounding mode (frexp, modf, scalbln, copysign, trunc, ceil, fmax, nan, logb, ilogb) | TESTED (new) | remquo.c:220-227 runs every check in all four modes |
| E35 | In the directed rounding modes, overflow rounds as IEEE 754 says | n/a | HUGE_VAL is checked only in round-to-nearest (remquo-type.h:103-146) |

### XSI Bessel functions, lgamma, signgam

| # | Requirement | Status | Reference |
|---|---|---|---|
| J1 | j0/j1/jn(NaN) give NaN, no error | TESTED (new) | j0-errors.c:137-143 |
| J2 | j0/j1/jn(+Inf) give +0 | TESTED (new; jn(-3,Inf) picolibc only, glibc gives -0) | j0-errors.c:145-156 |
| J3 | Huge x may be a range error, which then returns 0.0 | TESTED (new) | j0-errors.c:166-180 |
| J4 | An unrepresentable underflow returns 0.0 | TESTED (new) | j0-errors.c:182-186 |
| J5 | A representable underflow returns the correct value | TESTED (new) | j0-errors.c:188-192 |
| J6 | "No other errors shall occur" | TESTED (new) | every j0-errors.c check |
| J7 | J(-n,x) = (-1)^n J(n,x); jn(0)/jn(1) equal j0/j1 | TESTED (new) | j0-errors.c:194-206 |
| Y1 | y0/y1/yn(NaN) give NaN | TESTED (new) | y0-errors.c:125-131 |
| Y2 | Negative x gives NaN or the x=0 value, may be a domain error | TESTED (new) | y0-errors.c:133-145 |
| Y3 | x=±0 gives -HUGE_VAL, or +HUGE_VAL from yn for negative odd n; may be a pole error | TESTED (new) | y0-errors.c:147-176. **Deviation 3, fixed** |
| Y4 | +Inf gives +0 | TESTED (new) | y0-errors.c:178-186 |
| Y5 | y1/yn overflow gives -HUGE_VAL (+HUGE_VAL for negative odd n) | TESTED (new) | y0-errors.c:188-203 |
| Y6 | Underflow gives 0.0 or the correct value | NOT TESTABLE | no double argument makes Y(n,x) underflow |
| Y7 | y0(x)==yn(0,x), y1(x)==yn(1,x) | TESTED (new) | y0-errors.c:205-216 |
| G1 | lgamma, lgammaf, lgammal set signgam to the sign of Γ(x) | TESTED (new) | signgam.c:162-186 |
| G2 | signgam unspecified for NaN, -Inf, negative integers | n/a | |
| G3 | Concurrent calls leave signgam indeterminate | NOT TESTABLE | indeterminacy |

### `<fenv.h>`

| # | Requirement | Status | Reference |
|---|---|---|---|
| F1 | fenv_t, fexcept_t | TESTED (new) | fenv_h.c:107 |
| F2 | The five exception macros are bitwise distinct; all five with IEC 60559 | TESTED (new) | fenv_h.c:53-82, 126-130 |
| F3 | FE_ALL_EXCEPT is their OR | TESTED (new) | fenv_h.c:53-82 (arm's extra FE_DENORMAL is allowed) |
| F4 | Rounding macros distinct and non-negative | TESTED (new) | fenv_h.c:87-97 |
| F5 | FE_DFL_ENV has type const fenv_t * | TESTED (new) | fenv_h.c:100-104 |
| F6 | The 11 prototypes | TESTED (new) | fenv_h.c:109-121 |
| F7 | Strict POSIX: no GNU extensions; strict C17: no fesetexcept, fetestexceptflag, fegetmode, fesetmode, FE_DFL_MODE | TESTED (new) | fenv_h-names.c:56-68. **Deviation 6, fixed** |
| F8 | Reserved parameter and member names (application macros don't break the header) | TESTED (new) | fenv_h-names.c:37-43. **Deviation 6, fixed** |
| F9 | Default environment at startup | TESTED (new) | feupdateenv.c:158-160 |
| F10 | feraiseexcept raises and returns 0; feraiseexcept(0) | TESTED (new) | feupdateenv.c:162-176 |
| F11 | Inexact raised along with overflow/underflow is implementation-defined | n/a | masked by RAISED(), feupdateenv.c:73 |
| F12 | fetestexcept returns the OR of the requested flags that are set | TESTED (new) | feupdateenv.c:162-176 |
| F13 | feclearexcept clears only the requested flags, returns 0 | TESTED (new) | feupdateenv.c:162-176 |
| F14 | Arithmetic raises the matching flags | TESTED (new and existing) | feupdateenv.c:177-197; test-math/test-fenv.c |
| F15 | fegetexceptflag/fesetexceptflag save and set only the requested flags, return 0 | TESTED (new) | feupdateenv.c:199-217 |
| F16 | fesetexceptflag doesn't trap | NOT TESTABLE | trapped exceptions can't be enabled here (inferred: Neoverse N1 doesn't implement FP traps) |
| F17 | fegetround/fesetround set and return the mode | TESTED (new) | feupdateenv.c:94-95, 219-225 |
| F18 | fesetround of a non-mode returns nonzero and leaves the mode | TESTED (new) | feupdateenv.c:219-225 |
| F19 | The mode governs float, double and long double arithmetic | TESTED (new) | feupdateenv.c:97-116 in all four modes. With compiler-rt, long double division doesn't follow it (**Deviation 12**) |
| F20 | sqrt and fma round in the current mode | TESTED (new) | feupdateenv.c:117-132 |
| F21 | rint, nearbyint, lrint, llrint (f, l) follow the mode; round and lround don't | TESTED (new and existing) | feupdateenv.c:134-147; test-rounding-mode.c |
| F22 | fegetenv/fesetenv save and restore the mode and the flags | TESTED (new and existing) | feupdateenv.c:233-242; test-fenv.c |
| F23 | fesetenv(FE_DFL_ENV) restores the startup environment | TESTED (new) | feupdateenv.c:244-247 |
| F24 | feholdexcept saves, clears the flags, installs non-stop mode, returns 0 | TESTED (new; non-stop checked only as "1/0 doesn't trap") | feupdateenv.c:249-261 |
| F25 | feupdateenv saves the raised flags, installs the environment, re-raises; also with FE_DFL_ENV | TESTED (new) | feupdateenv.c:263-285 |
| F26 | With MATH_ERREXCEPT, functions raise their exceptions | TESTED (errh and all new tests) | with compiler-rt, long double raises none (**Deviation 12**) |

### `<complex.h>` (lightly)

| # | Requirement | Status | Reference |
|---|---|---|---|
| C1 | No `__STDC_NO_COMPLEX__`; complex, _Complex_I, I defined | TESTED (new) | cproj.c:75-87 |
| C2 | imaginary and _Imaginary_I defined only with imaginary types | n/a | the compilers have no imaginary types |
| C3 | CMPLX/F/L: macros, right types, usable in static initializers, keep infinite and NaN parts | TESTED (new) | cproj.c:84-97, 146-151 |
| C4 | The prototypes | TESTED (new) | cproj.c:99-131 |
| C5 | creal, cimag, conj (sign of zero) | TESTED (new) | cproj.c:153-159 |
| C6 | cabs: Inf beats NaN, NaN otherwise, +0, symmetric | TESTED (new) | cproj.c:161-169 |
| C7 | carg special values | TESTED (new) | cproj.c:171-178 |
| C8 | cproj: infinities project to +Inf + i·copysign(0, imaginary part) | TESTED (new) | cproj.c:180-195 |
| C9 | csqrt: branch cut, right half-plane, Inf and NaN cases | TESTED (new) | cproj.c:197-216. **Deviation 8, fixed** |
| C10 | clog: pole at ±0 raises divide-by-zero; branch cut; x + i·Inf | TESTED (new) | cproj.c:218-232 |
| C11 | MXC special values of the other complex functions | UNTESTED | beyond "lightly"; they apply because picolibc claims `__STDC_IEC_559_COMPLEX__` |

### Counts

- 110 requirements: 100 TESTED, 5 NOT TESTABLE, 1 UNTESTED, 4 n/a.
- 38 had a test before this unit; 100 after. Several of the 38 gained new checks (atan, erfc, log1p, logb, ilogb, lgamma's sign of zero and poles, the Bessel functions).

## Deviations found and fixed

1. **Missing Issue 8 M_ constants, wrong M_SQRT3, MAXFLOAT still defined** (`libc/include/math.h`, constants block).
   - M_EGAMMA, M_PHI, M_1_SQRTPI and M_SQRT1_3 were missing.
   - M_SQRT3 was BSD-only, with the value 1.73205080756887719000, wrong in the 17th digit.
   - Of the M_*l forms, only M_El, M_PIl, M_PI_2l and M_PI_4l existed, and only with _GNU_SOURCE.
   - MAXFLOAT was removed by Defect 1330.
   - Fix (2a4169aab7): all 36 constants for BSD or XSI; MAXFLOAT only for BSD or XSI before 800.
   - Without the fix, test-math_h fails to compile: "'M_EGAMMA' undeclared".
2. **FP_FAST_FMA* were empty.** POSIX: "shall expand to the integer constant 1".
   - Fix: 114284a037.
   - Without the fix: "static assertion failed: FP_FAST_FMA is 1" (aarch64).
3. **yn(n, 0) gave -HUGE_VAL for negative odd n** (`libm/math/s_jn.c:244`, `sf_jn.c:180`). POSIX requires +HUGE_VAL.
   - Fix: e349bae16d.
   - Without the fix: "yn(-1, 0) is -inf, not inf".
4. **logb(±0) didn't set errno** (`libm/common/s_logb.c`, `sf_logb.c`, `libm/ld/common/s_logbl.c`).
   - POSIX (MX) requires a pole error.
   - The code divided by zero, which raises the exception but leaves errno at 0.
   - Fix (8f7114b318): use `__math_divzero`. test-math-errhandling now expects ERANGE (0 for glibc).
   - Without the fix, in the errno build: "errno supported but logb_0 returns 0 (Success) instead of 34".
5. **remquo's domain errors didn't set errno** (`libm/common/s_remquo.c:102`, `sf_remquo.c`, `ld/ld128/s_remquol.c`, `ld/ld80/s_remquol.c`).
   - POSIX (MX) requires a domain error.
   - `(x*y)/(x*y)` raised invalid but left errno unset.
   - Fix (f1e8f933e0): NaN arguments return x+y; the rest go through `__math_invalid`, as remainder does.
   - Without the fix, in the errno build: "remquo(1, 0): errno 0, expected 33".
6. **`<fenv.h>` broke under application macros and leaked extensions** (fenv.h, fenv-softfloat.h, arm and aarch64 fenv-fp.h).
   - Parameter names `excepts`, `flagp`, `envp`, `round`, and femode_t members `round`/`except`, collided with application macros.
   - The inline feenableexcept, fedisableexcept and fegetexcept were defined for every compilation, and fesetexcept for every C version.
   - Fix (0674048667): `__` names. The inline extensions are now defined for `_GNU_SOURCE` or C23, or when libm/fenv/fenv.c sets `__PICOLIBC_FENV_ALL`. `_LIBC` couldn't be used because the CMake build passes it to the tests too.
   - Without the fix: "expected ';', ',' or ')' before numeric constant" and "'feenableexcept' redeclared as different kind of symbol".
7. **aarch64 fenv.h said libgcc's long double ignores the rounding mode** (`__LONG_DOUBLE_NOROUND`). libgcc's soft-fp reads the mode from FPCR.
   - Checked with gcc and with clang linking libgcc: long double sums and quotients, test-fma and test-narrow-fenv all pass in every mode.
   - Fix (d41b592a15): keep the macro only for compiler-rt and builds without an FPU. More long double tests now run.
   - Nothing failed before the change, so there is no failing test to show.
8. **csqrt/csqrtf used the wrong side of the branch cut, and mishandled Inf and NaN parts** (`libm/complex/csqrt.c`, `csqrtf.c`).
   - csqrt(-4-0i) returned 0+2i instead of 0-2i.
   - picolibc's stdc-predef.h claims `__STDC_IEC_559_COMPLEX__`, so the Annex G (MXC) cases apply.
   - Fix (782f9bfe0c): handle the special cases first, as csqrtl does, and use `copysign` on the cut.
   - Without the fix: "csqrt(-4 - 0i): same(creal(r), 0.0) && cimag(r) == -2.0".
9. **XSI mode declared j0f ... ynf** (math.h, gated by `__XSI_VISIBLE >= 600`). XSI defines only the double Bessel functions.
   - Fix (eeccd066e7): gate on `__MISC_VISIBLE`.
   - Without the fix: "'j0f' redeclared as different kind of symbol".

## Deviations not fixed

10. **lrint, llrint, lround, llround and fma set no errno** for the errors POSIX (MX) makes mandatory, in `want-math-errno` builds.
    - lrint family: NaN, ±Inf and out-of-range should be EDOM. fma: invalid operations should be EDOM, overflow ERANGE.
    - picolibc raises the exceptions, and errh encodes errno 0 for these. glibc doesn't set errno either.
    - A fix touches every implementation: libm/common, libm/ld, and the aarch64 frintx/fcvtzs, riscv and x86 machine versions. A wrapper that checks the rounded value in WANT_ERRNO builds would cover them all. Left for a decision.
11. **math_errhandling is 0** on thumbv7m soft-float without want-math-errno. Neither method exists there, so the header can't truthfully say otherwise; that configuration doesn't conform. test-math_h prints a message and accepts 0 under `__PICOLIBC__`.
12. **compiler-rt's long double raises no exceptions, and its division ignores the rounding mode** (clang `--rtlib=compiler-rt` on aarch64, `__RUNTIME_COMPILER_RT`).
    - Measured in local cross builds (`/tmp/math-cross-clang-aarch64*.txt`, not part of the patches).
    - Failures: "FE_UPWARD: long double 1/3 rounded wrongly"; "remquo(1, 0): exceptions 0x0, expected 0x1"; "lgammal(0): exceptions 0x0, expected 0x2". math_errhandling still says MATH_ERREXCEPT.
    - fenv.h's NOEXCEPT/NOROUND macros describe this correctly, and errh skips those checks.
    - All of it passes with clang linking libgcc. Raising the flags with double arithmetic in the `__math_*l` error helpers would cover the explicit error paths.

## Other observations

- In the directed rounding modes, an overflow rounds to ±DBL_MAX and erfc(120) to the smallest subnormal, as IEEE 754 says; picolibc and glibc agree. The tests check the HUGE_VAL and 0 values only in round-to-nearest.
- glibc 2.43 gaps the tests guard with `__GLIBC__`:
  - no Issue 8 M_ constants, and the M_*l forms only with _GNU_SOURCE;
  - MAXFLOAT defined for _XOPEN_SOURCE 800;
  - jn(-3, Inf) is -0;
  - no errno from logb(±0), from remquo's domain errors, or from scalbln's overflow in a directed mode.
- femode_t and fesetmode/FE_DFL_MODE (C23 only) lack the `const` that C23 gives them. Not changed.

## Left undone

- The MXC special values of the other complex functions (C11).
- Deviations 10-12.

## Files other units' patches also touch

- `test/test-posix/meson.build` and `CMakeLists.txt`: this unit adds `test-complex`, `test-fenv` and `test-math` to the directory lists; the locale unit adds others. This is a list merge.
- No round-8 patch touches math.h, fenv.h, complex.h, the machine fenv headers or libm.

## Test runs (verified by running)

- **aarch64-linux-gnu** (`-Dwerror=true`, full `meson test -t 4`): Ok 1656, Expected Fail 6, Fail 0, Skipped 41.
- **arm-linux-gnueabihf**: Ok 1654, Expected Fail 6, Fail 0, Skipped 44.
- **Native gcc** (ninja only): builds clean.
- **Clang native** (`CC=clang CXX=clang++`, full test): Ok 1266, Expected Fail 6, Fail 0, Skipped 397. It first caught clang-only float-promotion and int-to-float warnings in the new tests, which are fixed.
- **CMake thumbv7m** (`ninja test`): 657 tests, 100% passed, 365 skipped. It first caught the soft-float target's missing FE_* macros (the tests now fall back to 0), `_LIBC` reaching the tests, and math_errhandling == 0.
- **errno builds** (aarch64 and armhf, `-Dwant-math-errno=true`, the new tests plus test-math-errhandling): Ok 11, Fail 0 each.
- **clang cross builds** (aarch64, errno on):
  - With libgcc: all new tests, test-math-errhandling, test-fenv, test-rounding-mode, test-fma, test-narrow-fenv and test-long-double pass.
  - With compiler-rt: three fail as described in Deviation 12.
- Proofs (library change reversed, test failing) were run on aarch64, or in the aarch64 errno build for Deviations 4 and 5.
- No host security software interference and no timeouts.

## Commits (audit/math, on top of 1bac8ef806)

- 2a4169aab7 math.h: Define the POSIX.1-2024 M_ constants and drop MAXFLOAT
- 114284a037 math.h: Define FP_FAST_FMA, FP_FAST_FMAF and FP_FAST_FMAL as 1
- e349bae16d math: Return +HUGE_VAL from yn(n, 0) for negative odd n
- 8f7114b318 math: Make logb(0) a pole error
- f1e8f933e0 math: Make remquo set errno for its domain errors
- 0674048667 fenv.h: Use reserved names, and hide the extensions in strict mode
- d41b592a15 aarch64: libgcc's long double follows the rounding mode
- 782f9bfe0c complex: Keep csqrt and csqrtf on the right side of the branch cut
- eeccd066e7 math.h: Declare the float Bessel functions only as extensions
- 94acd25770 test-posix: Check <math.h>'s macros, constants, prototypes and namespace
- d7c14c5f56 test-posix: Check the Bessel functions' errors and special values
- b4cfad3c1d test-posix: Check signgam and lgamma's special values and errors
- ed090530a5 test-posix: Check the special values and errors of remquo and others
- 65a28d4df4 test-posix: Check <fenv.h>
- 54ce835353 test-posix: Check <complex.h>'s macros and branch cuts

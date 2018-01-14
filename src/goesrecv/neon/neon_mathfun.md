# neon_mathfun

Originals can be found here: http://gruntthepeon.free.fr/ssemath/neon_mathfun.html.

No modifications have been made.

Output of `neon_mathfun_test.c` on a Raspberry Pi 3:

```
checking sines on [0*Pi, 1*Pi]
max deviation from sinf(x): 1.19209e-07 at 0.263172633899*Pi, max deviation from cephes_sin(x): 0
max deviation from cosf(x): 5.96046e-08 at 0.141602561694*Pi, max deviation from cephes_cos(x): 0
deviation of sin(x)^2+cos(x)^2-1: 1.78814e-07 (ref deviation is 1.78814e-07)
   ->> precision OK for the sin_ps / cos_ps / sincos_ps <<-

checking sines on [-1000*Pi, 1000*Pi]
max deviation from sinf(x): 1.19209e-07 at -998.270005266*Pi, max deviation from cephes_sin(x): 0
max deviation from cosf(x): 1.19209e-07 at -971.759749797*Pi, max deviation from cephes_cos(x): 0
deviation of sin(x)^2+cos(x)^2-1: 1.78814e-07 (ref deviation is 1.78814e-07)
   ->> precision OK for the sin_ps / cos_ps / sincos_ps <<-

checking exp/log [-60, 60]
max (relative) deviation from expf(x): 1.18777e-07 at  3.46936869621, max deviation from cephes_expf(x): 0
max (absolute) deviation from logf(x): 1.19209e-07 at  -1.1134108305, max deviation from cephes_logf(x): 0
deviation of x - log(exp(x)): 1.19209e-07 (ref deviation is 1.19209e-07)
   ->> precision OK for the exp_ps / log_ps <<-

exp([        -1000,          -100,           100,          1000]) = [            0,             0, 2.4061436e+38, 2.4061436e+38]
exp([         -nan,           inf,          -inf,           nan]) = [          nan, 2.4061436e+38,             0,           nan]
log([            0,           -10,         1e+30, 1.0005271e-42]) = [         -nan,          -nan,     69.077553,          -nan]
log([         -nan,           inf,          -inf,           nan]) = [    89.128304,     88.722839,          -nan,     89.128304]
sin([         -nan,           inf,          -inf,           nan]) = [          nan,           nan,          -nan,           nan]
cos([         -nan,           inf,          -inf,           nan]) = [          nan,           nan,           nan,           nan]
sin([       -1e+30,       -100000,         1e+30,        100000]) = [          inf,  -0.035749275,          -inf,   0.035749275]
cos([       -1e+30,       -100000,         1e+30,        100000]) = [          nan,    -0.9993608,           nan,    -0.9993608]
benching                 sinf .. ->    2.6 millions of vector evaluations/second ->  95 cycles/value on a 1000MHz computer
benching                 cosf .. ->    2.3 millions of vector evaluations/second -> 106 cycles/value on a 1000MHz computer
benching                 expf .. ->    2.0 millions of vector evaluations/second -> 121 cycles/value on a 1000MHz computer
benching                 logf .. ->    2.0 millions of vector evaluations/second -> 122 cycles/value on a 1000MHz computer
benching          cephes_sinf .. ->    3.4 millions of vector evaluations/second ->  73 cycles/value on a 1000MHz computer
benching          cephes_cosf .. ->    2.8 millions of vector evaluations/second ->  86 cycles/value on a 1000MHz computer
benching          cephes_expf .. ->    1.8 millions of vector evaluations/second -> 132 cycles/value on a 1000MHz computer
benching          cephes_logf .. ->    1.6 millions of vector evaluations/second -> 150 cycles/value on a 1000MHz computer
benching               sin_ps .. ->   10.0 millions of vector evaluations/second ->  25 cycles/value on a 1000MHz computer
benching               cos_ps .. ->   10.6 millions of vector evaluations/second ->  23 cycles/value on a 1000MHz computer
benching            sincos_ps .. ->   10.0 millions of vector evaluations/second ->  25 cycles/value on a 1000MHz computer
benching               exp_ps .. ->   10.4 millions of vector evaluations/second ->  24 cycles/value on a 1000MHz computer
benching               log_ps .. ->    9.5 millions of vector evaluations/second ->  26 cycles/value on a 1000MHz computer
```

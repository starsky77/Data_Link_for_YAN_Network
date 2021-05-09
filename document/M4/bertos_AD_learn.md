Bertos的解调有三种形式的滤波来实现：

afsk.h

```c
/**
 * \name Afsk filter types.
 * $WIZ$ afsk_filter_list = "AFSK_BUTTERWORTH", "AFSK_CHEBYSHEV"
 * \{
 */
#define AFSK_BUTTERWORTH  0
#define AFSK_CHEBYSHEV    1
#define AFSK_FIR          2
```

代码量上看主要使用的是第三种方式：

```c
#if (CONFIG_AFSK_FILTER == AFSK_FIR)
static int8_t fir_filter(int8_t s, enum fir_filters f)
{
	int8_t Q = fir_table[f].taps - 1;
	int8_t *B = fir_table[f].coef;
	int16_t *Bmem = fir_table[f].mem;

	int8_t i;
	int16_t y;

	Bmem[0] = s;
	y = 0;

	for (i = Q; i >= 0; i--)
	{
		y += Bmem[i] * B[i];
		Bmem[i + 1] = Bmem[i];
	}

	return (int8_t) (y / 128);
}
#endif
```

这一方式应该是Finite impulse response,一种数字信号处理中的滤波的方式，维基资料[Finite impulse response - Wikipedia](https://en.wikipedia.org/wiki/Finite_impulse_response)



解调的过程见函数`void afsk_adc_isr(Afsk *af, int8_t curr_sample)`。

先不说这个滤波的过程是怎么样的，我很好奇这个工程是怎么表示采样的数据的`int8_t curr_sample`，是怎么得到的？
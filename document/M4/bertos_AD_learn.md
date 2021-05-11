# Bertos demodulation 

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







# About DFT(FFT)

看FFT一脸懵逼，DFT相对还能理解（自动忽略数学推导过程）



**DFT：**

输入：N个采样点对应的数值；

输出：一组复数$X[k_c]$;

通过一个公式可以找到对应的频率在采样值中的分量:
$$
f=k_cf_s/N
$$
其中$f$为对应的频率，$f_s$是我们采样的频率，N是采样点数量；因此根据需要的$f$反求出$K_c$带入$X[k_c]$去找对应的实部的值（虚部会在计算中被消掉，这就是数学问题了，总之频率分量不考虑实部）。



推荐相关资料：

[用离散傅里叶变换解调数字调制信号 - 百度文库 (baidu.com)](https://wenku.baidu.com/view/014de46627d3240c8447ef3e.html)

[傅里叶变换学习心得 - 知乎 (zhihu.com)](https://zhuanlan.zhihu.com/p/66117227)


/* tests an issue in lcc spilling of floating point registers */

#include <stdio.h>
#include <assert.h>

void stub(double x) {}

int main(void)
{
	int a, b;
	double c, d;

	a=2, b=8;

	c = ((double)a) / ((double)b) * .1;
	stub(c);

	d = ((double)a) / ((double)b);
	d = d * .1;

	if (c!=d) {
		printf("%f != %f\n", c, d);
		printf("float spill failed.\n");
	}
	else {
		printf("%f == %f\n", c, d);
		printf("float spill passed.\n");
	}

	return 0;
}

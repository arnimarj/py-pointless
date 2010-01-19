#include <pointless/custom_sort.h>

static int cmp_adapter(int a, int b, qsort_cmp_ cmp, void* user, int* is_error)
{
	int c = 0;

	if (!(*cmp)(a, b, &c, user))
		*is_error = 1;

	return c;
}

static int med3(int a, int b, int c, qsort_cmp_ cmp, void* user)
{
	int is_error = 0;

	int v = cmp_adapter(a, b, cmp, user, &is_error) < 0
		? (cmp_adapter(b, c, cmp, user, &is_error) < 0 ? b : cmp_adapter(a, c, cmp, user, &is_error) < 0 ? c : a)
		: (cmp_adapter(b, c, cmp, user, &is_error) > 0 ? b : cmp_adapter(a, c, cmp, user, &is_error) < 0 ? a : c)
	;

	if (is_error)
		return -1;

	return v;
}

static int bentley_qsort_priv(int a, int n, qsort_cmp_ cmp_x, qsort_swap_ swap, void* user)
{
	int pm, pl, pn, pa, pb, pc, pd, i, d, r, presorted, is_error;

loop:
	is_error = 0;

	if (n < 7) {
		for (pm = a + 1; pm < a + n; pm += 1) {
			for (pl = pm; pl > a && cmp_adapter(pl - 1, pl, cmp_x, user, &is_error) > 0; pl -= 1) {
				if (is_error)
					return 0;

				(*swap)(pl, pl - 1, user);
			}
		}

		return 1;
	}

	presorted = 1;

	for (pm = a + 1; pm < a + n; pm += 1) {
		if (cmp_adapter(pm - 1, pm, cmp_x, user, &is_error) > 0) {
			if (is_error)
				return 0;

			presorted = 0;
			break;
		}
	}

	if (presorted)
		return 1;

	pm = a + (n / 2);

	if (n > 7) {
		pl = a;
		pn = a + (n - 1);

		if (n > 40) {
			d = (n / 8);
			pl = med3(pl, pl + d, pl + 2 * d, cmp_x, user);
			pm = med3(pm - d, pm, pm + d, cmp_x, user);
			pn = med3(pn - 2 * d, pn - d, pn, cmp_x, user);

			if (pl == -1 || pm == -1 || pn == -1)
				return 0;
		}

		pm = med3(pl, pm, pn, cmp_x, user);

		if (pm == -1)
			return 0;
	}

	(*swap)(a, pm, user);

	pa = pb = a + 1;
	pc = pd = a + (n - 1);

	for (;;) {
		while (pb <= pc && (r = cmp_adapter(pb, a, cmp_x, user, &is_error)) <= 0) {
			if (is_error)
				return 0;

			if (r == 0) {
				(*swap) (pa, pb, user);
				pa += 1;
			}

			pb += 1;
		}

		while (pb <= pc && (r = cmp_adapter(pc, a, cmp_x, user, &is_error)) >= 0) {
			if (is_error)
				return 0;

			if (r == 0) {
				(*swap) (pc, pd, user);
				pd -= 1;
			}

			pc -= 1;
		}

		if (pb > pc)
			break;

		(*swap)(pb, pc, user);
		pb += 1;
		pc -= 1;
	}

	pn = a + n;
	r = SIMPLE_MIN(pa - a, pb - pa);

	// vecswap(a, pb - r, r);
	for (i = 0; i < r; i++)
		(*swap)(a + i, pb - r + i, user);

	r = SIMPLE_MIN(pd - pc, pn - pd - 1);

	// vecswap(pb, pn - r, r);
	for (i = 0; i < r; i++)
		(*swap)(pb + i, pn - r + i, user);

	if ((r = pb - pa) > 1) {
		if (!bentley_qsort_priv(a, r, cmp_x, swap, user))
			return 0;
	}

	if ((r = pd - pc) > 1) {
		a = pn - r;
		n = r;
		goto loop;
	}

	return 1;
}

int bentley_sort_(int n, qsort_cmp_ cmp, qsort_swap_ swap, void* user)
{
	return bentley_qsort_priv(0, n, cmp, swap, user);
}

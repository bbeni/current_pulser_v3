// Copyright (C) 2026 Benjamin Froelich
// This file is part of https://github.com/bbeni/impedancer
// For conditions of distribution and use, see copyright notice in project root.

#include "mma.h"
#include "math.h"
#include "assert.h"
#include "stdio.h"
#include "stdlib.h"
#include "stdint.h"


void mma_clampf(float* v, float lower, float upper) {
	if (*v < lower) {
		*v = lower;
		return;
	}

	if (*v > upper) {
		*v = upper;
	}
}

void mma_clampi(int* v, int lower, int upper) {
	if (*v < lower) {
		*v = lower;
		return;
	}

	if (*v > upper) {
		*v = upper;
	}
}


float mma_lerpf(float lower, float upper, float t) {
	return lower * (1.0f - t) + upper * t;
}


void mma_move_towardsf(float* x, float target, float speed, float dt) {
	if (*x == target) {
		return;
	}
	if (*x > target) {
		*x -= speed * dt;
		if (*x < target) {
			*x = target;
		}
	}
	else {
		*x += speed * dt;
		if (*x > target) {
			*x = target;
		}
	}
}

void mma_move_towardsv3f(struct Vec3f* vec, struct Vec3f target, float speed, float dt) {
	mma_move_towardsf(&vec->x, target.x, speed, dt);
	mma_move_towardsf(&vec->y, target.y, speed, dt);
	mma_move_towardsf(&vec->z, target.z, speed, dt);
}

// move an angle to a range [-pi, pi) closer to the mapped target angle the shorter way around the circle
// @Bug somehow there is and edge case where the target is PI and we go to -PI and never set to PI but it should flip it here
// for now just assume the user of this function is not dependent on the wrap around behaviour
void mma_move_towards_on_circlef(float* angle, float target,  float speed, float dt) {

	//*angle = fmod(*angle + M_PI, 2 * M_PI) - M_PI; // map to [-pi, pi)
	// we should use fmod.. but for now thats ok what is done here
	// maybe I am using fmod the wrong way.. the asserts fired using fmod..
	while (*angle >= M_PI) {
		*angle -= 2 * M_PI;
	}

	while (*angle < -M_PI) {
		*angle += 2 * M_PI;
	}

	assert(*angle < M_PI);
	assert(*angle >= -M_PI);

	if (*angle + M_PI < target) {
		mma_move_towardsf(angle, target - 2*M_PI, speed, dt);
	} else if (*angle - M_PI > target) {
		mma_move_towardsf(angle, target + 2*M_PI, speed, dt);
	}
	else {
		mma_move_towardsf(angle, target, speed, dt);
	}
}

//thomas algorithm O(n) see https://en.wikipedia.org/wiki/Tridiagonal_matrix_algorithm
void mma_solve_tridiagonal_matrix(const size_t N, const double *a, const double *b,
            const double *c, double *x_out, double *scratch_buffer) {
    /*
     solves Ax = d, where A is a tridiagonal matrix consisting of vectors a, b, c
     N = number of equations
     x[] = initially contains the input, d, and returns x. indexed from [0, ..., N - 1]
     a[] = subdiagonal, indexed from [1, ..., N - 1]
     b[] = main diagonal, indexed from [0, ..., N - 1]
     c[] = superdiagonal, indexed from [0, ..., N - 2]
     scratch_buffer[] = scratch_buffer space of length N, provided by caller, allowing a, b, c to be const
     not performed in this example: manual expensive common subexpression elimination
     */
    scratch_buffer[0] = c[0] / b[0];
    x_out[0] = x_out[0] / b[0];

    /* loop from 1 to N - 1 inclusive */
    for (size_t ix = 1; ix < N; ix++) {
        if (ix < N-1){
        scratch_buffer[ix] = c[ix] / (b[ix] - a[ix] * scratch_buffer[ix - 1]);
        }
        x_out[ix] = (x_out[ix] - a[ix] * x_out[ix - 1]) / (b[ix] - a[ix] * scratch_buffer[ix - 1]);
    }

    /* loop from N - 2 to 0 inclusive */
    for (size_t ix = N - 1; ix > 0; ix--)
        x_out[ix-1] -= scratch_buffer[ix-1] * x_out[ix];
}

// S''(x0) = S''(x(n-1)) = 0 natural condition
// make sure there is enough space in y_out
// general function that accepts and x_resamples array (needs to be in ascending order!)
void mma_spline_cubic_natural(const double *x, const double *y, size_t n_in, double *y_out, double* x_resamples, size_t n_out) {
	assert(n_out >= 2);

	mma_temp_set_restore_point();
	double *a = mma_temp_alloc(sizeof(double) * (n_in - 1));
	double *b = mma_temp_alloc(sizeof(double) * (n_in - 1));
	mma_spline_cubic_natural_ab(x, y, n_in, a, b);
	// a_i b_i are used to derive the spline and are related for example to
	//        q_i(x) = (1-t) y_(i-1) + t y_i + t (t-1) ((1-t)a_i + tb_i)

	size_t j = 0;
	for (size_t i = 0; i < n_out; i ++) {
		double x_now = x_resamples[i];

		// we increase j until x is in [x[j], x[j+1]]
		while (j + 1 < n_in && x_now > x[j + 1]) {
			j++;
		}

		double t = (x_now - x[j]) / (x[j + 1] - x[j]);
		y_out[i] = (1 - t) * y[j] + t * y[j + 1] + t * (1 - t) * ((1 - t) * a[j] + t * b[j]);
	}

	mma_temp_restore();
}

// S''(x0) = S''(x(n-1)) = 0 natural condition
// make sure there is enough space in y_out
// general function that accepts and x_resamples array (needs to be in ascending order!)
void mma_spline_cubic_natural_complex(const double *x, const struct Complex *z, size_t n_in, struct Complex *z_out, double *x_resamples, size_t n_out) {
	assert(n_out >= 2);

	mma_temp_set_restore_point();
	struct Complex *a = mma_temp_alloc(sizeof(struct Complex) * (n_in - 1));
	struct Complex *b = mma_temp_alloc(sizeof(struct Complex) * (n_in - 1));
	mma_spline_cubic_natural_ab_complex(x, z, n_in, a, b);
	// a_i b_i are used to derive the spline and are related for example to
	//        q_i(x) = (1-t) y_(i-1) + t y_i + t (t-1) ((1-t)a_i + tb_i)

	size_t j = 0;
	for (size_t i = 0; i < n_out ; i ++) {
		double x_now = x_resamples[i];
		// we increase j until x is in [x[j], x[j+1]]
		while (j + 1 < n_in - 1 && x_now > x[j + 1]) {
			j++;
		}

		double t = (x_now - x[j]) / (x[j + 1] - x[j]);

		z_out[i].r = (1 - t) * z[j].r + t * z[j + 1].r + t * (1 - t) * ((1 - t) * a[j].r + t * b[j].r);
		z_out[i].i = (1 - t) * z[j].i + t * z[j + 1].i + t * (1 - t) * ((1 - t) * a[j].i + t * b[j].i);
	}

	mma_temp_restore();
}

// S''(x0) = S''(x(n-1)) = 0 natural condition
// make sure there is enough space in y_out
// general function that accepts and x_resamples array (needs to be in ascending order!)
void mma_spline_cubic_natural_complex_2(const double *x, const struct Complex *z, size_t n_in, double *real_out, double *imaginary_out, double *x_resamples, size_t n_out) {
	assert(n_out >= 2);

	mma_temp_set_restore_point();
	struct Complex *a = mma_temp_alloc(sizeof(struct Complex) * (n_in - 1));
	struct Complex *b = mma_temp_alloc(sizeof(struct Complex) * (n_in - 1));
	mma_spline_cubic_natural_ab_complex(x, z, n_in, a, b);
	// a_i b_i are used to derive the spline and are related for example to
	//        q_i(x) = (1-t) y_(i-1) + t y_i + t (t-1) ((1-t)a_i + tb_i)

	size_t j = 0;
	for (size_t i = 0; i < n_out ; i ++) {
		double x_now = x_resamples[i];
		// we increase j until x is in [x[j], x[j+1]]
		while (j + 1 < n_in - 1 && x_now > x[j + 1]) {
			j++;
		}

		double t = (x_now - x[j]) / (x[j + 1] - x[j]);

		real_out[i] = (1 - t) * z[j].r + t * z[j + 1].r + t * (1 - t) * ((1 - t) * a[j].r + t * b[j].r);
		imaginary_out[i] = (1 - t) * z[j].i + t * z[j + 1].i + t * (1 - t) * ((1 - t) * a[j].i + t * b[j].i);
	}

	mma_temp_restore();
}


// S''(x0) = S''(x(n-1)) = 0
// make sure there is enough space in y_out
void mma_spline_cubic_natural_linear(const double *x, const double *y, size_t n_in, double *y_out, size_t n_out, double x_min, double x_max) {
	assert(n_out >= 2);
	assert(x_min >= x[0]);
	assert(x_max <= x[n_in - 1]);
	assert(x_max >= x_min);

	double dx = (x_max - x_min) / (n_out - 1);

	mma_temp_set_restore_point();
	double *a = mma_temp_alloc(sizeof(double) * (n_in - 1));
	double *b = mma_temp_alloc(sizeof(double) * (n_in - 1));
	mma_spline_cubic_natural_ab(x, y, n_in, a, b);
	// a_i b_i are used to derive the spline and are related for example to
	//        q_i(x) = (1-t) y_(i-1) + t y_i + t (t-1) ((1-t)a_i + tb_i)

	size_t j = 0;
	for (size_t i = 0; i < n_out; i ++) {
		double x_now = i * dx + x_min;
		// we increase j until x is in [x[j], x[j+1]]
		while (j + 1 < n_in - 1 && x_now > x[j + 1]) {
			j++;
		}
		double dx = x[j + 1] - x[j];
		assert(dx > 0);
		double t = (x_now - x[j]) / dx;
		assert(t >= 0);
		y_out[i] = (1 - t) * y[j] + t * y[j + 1] + t * (1 - t) * ((1 - t) * a[j] + t * b[j]);
	}

	mma_temp_restore();

}

void mma_spline_cubic_natural_linear_complex(const double *x, const struct Complex *z, size_t n_in, struct Complex *z_out, size_t n_out, double x_min, double x_max) {
	assert(n_out >= 2);
	assert(x_min >= x[0]);
	assert(x_max <= x[n_in - 1]);
	assert(x_max >= x_min);

	double dx = (x_max - x_min) / (n_out - 1);
	mma_temp_set_restore_point();
	struct Complex *a = mma_temp_alloc(sizeof(struct Complex) * (n_in - 1));
	struct Complex *b = mma_temp_alloc(sizeof(struct Complex) * (n_in - 1));
	mma_spline_cubic_natural_ab_complex(x, z, n_in, a, b);
	// a_i b_i are used to derive the spline and are related for example to
	//        q_i(x) = (1-t) y_(i-1) + t y_i + t (t-1) ((1-t)a_i + tb_i)

	size_t j = 0;
	for (size_t i = 0; i < n_out ; i ++) {
		double x_now = i * dx + x_min;
		// we increase j until x is in [x[j], x[j+1]]
		while (j + 1 < n_in - 1 && x_now > x[j + 1]) {
			j++;
		}

		double t = (x_now - x[j]) / (x[j + 1] - x[j]);
		assert(t >= 0);
		z_out[i].r = (1 - t) * z[j].r + t * z[j + 1].r + t * (1 - t) * ((1 - t) * a[j].r + t * b[j].r);
		z_out[i].i = (1 - t) * z[j].i + t * z[j + 1].i + t * (1 - t) * ((1 - t) * a[j].i + t * b[j].i);
	}

	mma_temp_restore();
}

// see https://en.wikipedia.org/wiki/Spline_interpolation
// make sure there is enough space (n_in - 1) in a_out, b_out
// a_i b_i are used to derive the spline and are related for example to
//        q_i(x) = (1-t) y_(i-1) + t y_i + t (t-1) ((1-t)a_i + tb_i)
//
void mma_spline_cubic_natural_ab(const double *x, const double *y, size_t n_in, double *a_out, double *b_out) {
	assert(n_in >= 2);
	mma_temp_set_restore_point();
	double *a_sub = mma_temp_alloc(n_in*sizeof(double));
	double *a = mma_temp_alloc(n_in*sizeof(double));
	double *a_sup = mma_temp_alloc(n_in*sizeof(double));
	double *k = mma_temp_alloc(n_in*sizeof(double)); // out k, initally holds b
	// diagonal a_ii
	a[0] = 2.0 / (x[1] - x[0]);
	a[n_in - 1] = 2.0 / (x[n_in - 1] - x[n_in - 2]);
	for (size_t i = 1; i < n_in-1; i ++) {
		a[i] = 2.0 * (1.0 / (x[i] - x[i - 1]) + 1.0/ (x[i + 1] - x[i]));
	}

	// sub and super diagonal elements
	for (size_t i = 1; i < n_in; i ++) {
		a_sub[i] = 1.0 / (x[i] - x[i - 1]);
		a_sup[i - 1] = 1.0 / (x[i] - x[i - 1]);
	}

	// b vector is stored in k
	k[0] = 3.0 * (y[1] - y[0]) / ((x[1] - x[0]) * (x[1] - x[0]));
	k[n_in - 1] = 3.0 * (y[n_in - 1] - y[n_in - 2]) / ((x[n_in - 1] - x[n_in - 2]) * (x[n_in - 1] - x[n_in - 2]));
	for (size_t i = 1; i < n_in - 1; i++) {
		k[i] = 3.0 * ((y[i] - y[i - 1]) / ((x[i] - x[i - 1]) * (x[i] - x[i - 1]))  + (y[i + 1] - y[i]) / ((x[i + 1] - x[i]) * (x[i + 1] - x[i])));
	}

	double *scratch = mma_temp_alloc(n_in*sizeof(double));
	mma_solve_tridiagonal_matrix(n_in, a_sub, a, a_sup, k, scratch);

	// calc a_out b_out
	for (size_t i = 0; i < n_in - 1; i++) {
		a_out[i] = k[i] * (x[i+1] - x[i]) - (y[i+1] - y[i]);
		b_out[i] = -k[i+1] * (x[i+1] - x[i]) + (y[i+1] - y[i]);
	}

	mma_temp_restore();
}

// see https://en.wikipedia.org/wiki/Spline_interpolation
// make sure there is enough space (n_in - 1) in a_out, b_out
// a_i b_i are used to derive the spline and are related for example to
//        q_i(x) = (1-t) y_(i-1) + t y_i + t (t-1) ((1-t)a_i + tb_i)
//
void mma_spline_cubic_natural_ab_complex(const double *x, const struct Complex *z, size_t n_in, struct Complex *a_out, struct Complex *b_out) {
	assert(n_in >= 2);
	mma_temp_set_restore_point();
	double *a_sub = mma_temp_alloc(n_in*sizeof(double));
	double *a = mma_temp_alloc(n_in*sizeof(double));
	double *a_sup = mma_temp_alloc(n_in*sizeof(double));
	double *k = mma_temp_alloc(n_in*sizeof(double)); // out k, initally holds b
	double *ia_sub = mma_temp_alloc(n_in*sizeof(double));
	double *ia = mma_temp_alloc(n_in*sizeof(double));
	double *ia_sup = mma_temp_alloc(n_in*sizeof(double));
	double *ik = mma_temp_alloc(n_in*sizeof(double)); // out k, initally holds b


	// diagonal a_ii
	a[0] = 2.0 / (x[1] - x[0]);
	a[n_in - 1] = 2.0 / (x[n_in - 1] - x[n_in - 2]);
	for (size_t i = 1; i < n_in-1; i ++) {
		a[i] = 2.0 * (1.0 / (x[i] - x[i - 1]) + 1.0/ (x[i + 1] - x[i]));
	}

	// sub and super diagonal elements
	for (size_t i = 1; i < n_in; i ++) {
		a_sub[i] = 1.0 / (x[i] - x[i - 1]);
		a_sup[i - 1] = 1.0 / (x[i] - x[i - 1]);
	}

	// b vector is stored in k
	k[0] = 3.0 * (z[1].r - z[0].r) / ((x[1] - x[0]) * (x[1] - x[0]));
	k[n_in - 1] = 3.0 * (z[n_in - 1].r - z[n_in - 2].r) / ((x[n_in - 1] - x[n_in - 2]) * (x[n_in - 1] - x[n_in - 2]));
	for (size_t i = 1; i < n_in - 1; i++) {
		k[i] = 3.0 * ((z[i].r - z[i - 1].r) / ((x[i] - x[i - 1]) * (x[i] - x[i - 1]))  + (z[i + 1].r - z[i].r) / ((x[i + 1] - x[i]) * (x[i + 1] - x[i])));
	}

	// imag part
	// diagonal a_ii
	ia[0] = 2.0 / (x[1] - x[0]);
	ia[n_in - 1] = 2.0 / (x[n_in - 1] - x[n_in - 2]);
	for (size_t i = 1; i < n_in-1; i ++) {
		ia[i] = 2.0 * (1.0 / (x[i] - x[i - 1]) + 1.0/ (x[i + 1] - x[i]));
	}

	// sub and super diagonal elements
	for (size_t i = 1; i < n_in; i ++) {
		ia_sub[i] = 1.0 / (x[i] - x[i - 1]);
		ia_sup[i - 1] = 1.0 / (x[i] - x[i - 1]);
	}

	// b vector is stored in k
	ik[0] = 3.0 * (z[1].i - z[0].i) / ((x[1] - x[0]) * (x[1] - x[0]));
	ik[n_in - 1] = 3.0 * (z[n_in - 1].i - z[n_in - 2].i) / ((x[n_in - 1] - x[n_in - 2]) * (x[n_in - 1] - x[n_in - 2]));
	for (size_t i = 1; i < n_in - 1; i++) {
		ik[i] = 3.0 * ((z[i].i - z[i - 1].i) / ((x[i] - x[i - 1]) * (x[i] - x[i - 1]))  + (z[i + 1].i - z[i].i) / ((x[i + 1] - x[i]) * (x[i + 1] - x[i])));
	}


	double *scratch = mma_temp_alloc(n_in*sizeof(double));
	mma_solve_tridiagonal_matrix(n_in, a_sub, a, a_sup, k, scratch);
	mma_solve_tridiagonal_matrix(n_in, ia_sub, ia, ia_sup, ik, scratch);

	// calc a_out b_out
	for (size_t i = 0; i < n_in - 1; i++) {
		a_out[i].r = k[i] * (x[i+1] - x[i]) - (z[i+1].r - z[i].r);
		b_out[i].r = -k[i+1] * (x[i+1] - x[i]) + (z[i+1].r - z[i].r);
		a_out[i].i = ik[i] * (x[i+1] - x[i]) - (z[i+1].i - z[i].i);
		b_out[i].i = -ik[i+1] * (x[i+1] - x[i]) + (z[i+1].i - z[i].i);
	}

	mma_temp_restore();
}


// S'(x0) = y0', S'(x(n-1)) = y(n-1)'
// make sure there is enough space in y_out
//void mma_spline_cubic_clamped(double *x, double *y, size_t n_in, double *y_out, size_t n_out) {
//	TODO
//}


float mma_next_multiple_of(float start, float step) {
    float x = ceilf(start / step) * step;
    if (x == start) {
        x += step;
    }
    return x;
}


float mma_angle_betweenv2f(struct Vec2f a, struct Vec2f b) {
	float det = a.x * b.y - a.y * b.x;
	float dot = a.x * b.x + a.y * b.y;
	return atan2f(det, dot);
}

void mma_normalize_or_y_axisv2f(struct Vec2f* v) {
	float denom = sqrtf(v->x * v->x + v->y * v->y);
	if (denom == 0.0f) {
		v->x = 0.0f;
		v->y = 1.0f;
		return;
	}
	float factor = 1 / denom;
	v->x *= factor;
	v->y *= factor;
}

void mma_normalize_or_zerov2f(struct Vec2f* v) {
	float denom = sqrtf(v->x * v->x + v->y * v->y);
	if (denom == 0.0f) {
		v->x = 0.0f;
		v->y = 1.0f;
		return;
	}
	float factor = 1 / denom;
	v->x *= factor;
	v->y *= factor;
}


float mma_dotv3f(struct Vec3f a, struct Vec3f b) {
	return a.x * b.x + a.y * b.y + a.z * b.z;
}

float mma_dotv2f(struct Vec2f a, struct Vec2f b) {
	return a.x * b.x + a.y * b.y;
}

float mma_normv3f(struct Vec3f vec) {
	return sqrtf(mma_dotv3f(vec, vec));
}

float mma_normv2f(struct Vec2f vec) {
	return sqrtf(mma_dotv2f(vec, vec));
}

void mma_normalize_or_z_axisv3f(struct Vec3f* v) {
	float denom = sqrtf(v->x * v->x + v->y * v->y + v->z * v->z);
	if (denom == 0.0f) {
		v->x = 0.0f;
		v->y = 0.0f;
		v->z = 1.0f;
		return;
	}
	float factor = 1 / denom;
	v->x *= factor;
	v->y *= factor;
	v->z *= factor;
}

struct Vec3f mma_crossv3f(struct Vec3f a, struct Vec3f b) {
	struct Vec3f v;
	v.x = a.y * b.z - a.z * b.y;
	v.y = a.z * b.x - a.x * b.z;
	v.z = a.x * b.y - a.y * b.x;
	return v;
}

struct Vec3f mma_addv3f(struct Vec3f a, struct Vec3f b) {
	struct Vec3f v;
	v.x = a.x + b.x;
	v.y = a.y + b.y;
	v.z = a.z + b.z;
	return v;
}

struct Vec3f mma_subtractv3f(struct Vec3f a, struct Vec3f b) {
	struct Vec3f v;
	v.x = a.x - b.x;
	v.y = a.y - b.y;
	v.z = a.z - b.z;
	return v;
}

struct Vec3f mma_multiplyfv3f(struct Vec3f a, float f) {
	return (struct Vec3f) { f * a.x, f * a.y, f * a.z };
}

bool mma_not_equalsv3f(struct Vec3f a, struct Vec3f b) {
	return a.x != b.x || a.y != b.y || a.z != b.z;
}

bool mma_equalsv3f(struct Vec3f a, struct Vec3f b) {
	return a.x == b.x && a.y == b.y && a.z == b.z;
}


void mma_add_inplacev3f(struct Vec3f a, struct Vec3f b)
{
	a.x += b.x;
	a.y += b.y;
	a.z += b.z;
}

void mma_subtract_inplacev3f(struct Vec3f a, struct Vec3f b)
{
	a.x -= b.x;
	a.y -= b.y;
	a.z -= b.z;
}

struct Vec2f mma_addv2f(struct Vec2f a, struct Vec2f b) {
	struct Vec2f v;
	v.x = a.x + b.x;
	v.y = a.y + b.y;
	return v;
}

struct Vec2f mma_subtractv2f(struct Vec2f a, struct Vec2f b) {
	struct Vec2f v;
	v.x = a.x - b.x;
	v.y = a.y - b.y;
	return v;
}

struct Vec2f mma_mutliplyv2ff(struct Vec2f a, float f) {
	return (struct Vec2f){ f * a.x, f * a.y};
}

void mma_add_inplacev2f(struct Vec2f a, struct Vec2f b) {
	a.x += b.x;
	a.y += b.y;
}

void mma_subtract_inplacev2f(struct Vec2f a, struct Vec2f b) {
	a.x -= b.x;
	a.y -= b.y;
}

bool mma_not_equalsv2f(struct Vec2f a, struct Vec2f b) {
	return a.x != b.x || a.y != b.y;
}

bool mma_equalsv2f(struct Vec2f a, struct Vec2f b) {
	return a.x == b.x && a.y == b.y;
}

struct Complex mma_complex(double r, double i) {
	struct Complex c;
	c.r = r;
	c.i = i;
	return c;
}

struct Complex mma_complex_conjugate(struct Complex c) {
	struct Complex c1;
	c1.r = c.r;
	c1.i = -c.i;
	return c1;
}

double mma_complex_absolute(struct Complex c) {
	return sqrt(c.r * c.r + c.i * c.i);
}

double mma_complex_absolute_squared(struct Complex c) {
	return c.r * c.r + c.i * c.i;
}

struct Complex mma_complex_negate(struct Complex c1) {
	struct Complex c2;
	c2.r = -c1.r;
	c2.i = -c1.i;
	return c2;
}

struct Complex mma_complex_add(struct Complex a, struct Complex b) {
	return (struct Complex) {
		.r = a.r + b.r,
		.i = a.i + b.i
	};
}

struct Complex mma_complex_subtract(struct Complex a, struct Complex b) {
	return (struct Complex) {
		.r = a.r - b.r,
		.i = a.i - b.i
	};
}

struct Complex mma_complex_mult(struct Complex a, struct Complex b) {
	return (struct Complex) {
		.r = a.r*b.r - a.i*b.i,
		.i = a.r*b.i + b.r*a.i
	};
}


struct Complex mma_complex_divide_or_zero(struct Complex a, struct Complex b) {
	if (b.i == 0.0 && b.r == 0.0) {
		return (struct Complex) {0.0, 0.0};
	}

	double denom = b.r*b.r + b.i*b.i;

	return (struct Complex) {
		.r = (a.r*b.r + a.i*b.i) / denom,
		.i = (a.i*b.r - a.r*b.i) / denom
	};
}



const struct Mat4f mma_unit_mat4f =
{
	1.0f, 0, 0, 0,
	0, 1.0f, 0, 0,
	0, 0, 1.0f, 0,
	0, 0, 0, 1.0f,
};

struct Mat4f mma_matrix_multiplyv4f(struct Mat4f *a, struct Mat4f *b) {
	struct Mat4f m;

	m.u11 = a->u11 * b->u11 + a->u12 * b->u21 + a->u13 * b->u31 + a->u14 * b->u41;
	m.u12 = a->u11 * b->u12 + a->u12 * b->u22 + a->u13 * b->u32 + a->u14 * b->u42;
	m.u13 = a->u11 * b->u13 + a->u12 * b->u23 + a->u13 * b->u33 + a->u14 * b->u43;
	m.u14 = a->u11 * b->u14 + a->u12 * b->u24 + a->u13 * b->u34 + a->u14 * b->u44;

	m.u21 = a->u21 * b->u11 + a->u22 * b->u21 + a->u23 * b->u31 + a->u24 * b->u41;
	m.u22 = a->u21 * b->u12 + a->u22 * b->u22 + a->u23 * b->u32 + a->u24 * b->u42;
	m.u23 = a->u21 * b->u13 + a->u22 * b->u23 + a->u23 * b->u33 + a->u24 * b->u43;
	m.u24 = a->u21 * b->u14 + a->u22 * b->u24 + a->u23 * b->u34 + a->u24 * b->u44;

	m.u31 = a->u31 * b->u11 + a->u32 * b->u21 + a->u33 * b->u31 + a->u34 * b->u41;
	m.u32 = a->u31 * b->u12 + a->u32 * b->u22 + a->u33 * b->u32 + a->u34 * b->u42;
	m.u33 = a->u31 * b->u13 + a->u32 * b->u23 + a->u33 * b->u33 + a->u34 * b->u43;
	m.u34 = a->u31 * b->u14 + a->u32 * b->u24 + a->u33 * b->u34 + a->u34 * b->u44;

	m.u41 = a->u41 * b->u11 + a->u42 * b->u21 + a->u43 * b->u31 + a->u44 * b->u41;
	m.u42 = a->u41 * b->u12 + a->u42 * b->u22 + a->u43 * b->u32 + a->u44 * b->u42;
	m.u43 = a->u41 * b->u13 + a->u42 * b->u23 + a->u43 * b->u33 + a->u44 * b->u43;
	m.u44 = a->u41 * b->u14 + a->u42 * b->u24 + a->u43 * b->u34 + a->u44 * b->u44;

	return m;
}

struct Vec4f mma_matrix_multiply_rightmat4fv4f(struct Mat4f *m, struct Vec4f v)  {
	struct Vec4f out;
	out.x = m->u11 * v.x + m->u12 * v.y + m->u13 * v.z + m->u14 * v.w;
	out.y = m->u21 * v.x + m->u22 * v.y + m->u23 * v.z + m->u24 * v.w;
	out.z = m->u31 * v.x + m->u32 * v.y + m->u33 * v.z + m->u34 * v.w;
	out.w = m->u41 * v.x + m->u42 * v.y + m->u43 * v.z + m->u44 * v.w;
	return out;
}

struct Vec3f mma_matrix_multiply_rightmat4fv3f(struct Mat4f *m, struct Vec3f v)  {
	struct Vec4f out;
	out.x = v.x;
	out.y = v.y;
	out.z = v.z;
	out.w = 1;
	out = mma_matrix_multiply_rightmat4fv4f(m, out);
	return (struct Vec3f){out.x, out.y, out.z};
}


float mma_matrix_detmat4f(struct Mat4f *m) {
	// leibnitz fomula
	float result =
		m->u11 * m->u22 * m->u33 * m->u44 + m->u11 * m->u32 * m->u43 * m->u24 + m->u11 * m->u42 * m->u23 * m->u34 +
		m->u21 * m->u12 * m->u43 * m->u34 + m->u21 * m->u32 * m->u13 * m->u44 + m->u21 * m->u42 * m->u33 * m->u14 +
		m->u31 * m->u12 * m->u23 * m->u44 + m->u31 * m->u22 * m->u43 * m->u14 + m->u31 * m->u42 * m->u13 * m->u24 +
		m->u41 * m->u12 * m->u33 * m->u24 + m->u41 * m->u22 * m->u13 * m->u34 + m->u41 * m->u32 * m->u23 * m->u14 -
		m->u11 * m->u22 * m->u43 * m->u34 - m->u11 * m->u32 * m->u23 * m->u44 - m->u11 * m->u42 * m->u33 * m->u24 -
		m->u21 * m->u12 * m->u33 * m->u44 - m->u21 * m->u32 * m->u43 * m->u14 - m->u21 * m->u42 * m->u13 * m->u34 -
		m->u31 * m->u12 * m->u43 * m->u24 - m->u31 * m->u22 * m->u13 * m->u44 - m->u31 * m->u42 * m->u23 * m->u14 -
		m->u41 * m->u12 * m->u23 * m->u34 - m->u41 * m->u22 * m->u33 * m->u14 - m->u41 * m->u32 * m->u13 * m->u24;
	return result;
}

/* TODO: port to C

Mat4 matrix_transposed(const Mat4& mat) {
	Mat4 m;
	m.u11 = mat.u11;
	m.u12 = mat.u21;
	m.u13 = mat.u31;
	m.u14 = mat.u41;

	m.u21 = mat.u12;
	m.u22 = mat.u22;
	m.u23 = mat.u32;
	m.u24 = mat.u42;

	m.u31 = mat.u13;
	m.u32 = mat.u23;
	m.u33 = mat.u33;
	m.u34 = mat.u43;

	m.u41 = mat.u14;
	m.u42 = mat.u24;
	m.u43 = mat.u34;
	m.u44 = mat.u44;
	return m;
}

Mat4 matrix_from_basis_vectors(Vec3 x, Vec3 y, Vec3 z) {
	Mat4 result;
	result.u44 = 1.0f;

	result.u11 = x.x;
	result.u21 = x.y;
	result.u31 = x.z;

	result.u12 = y.x;
	result.u22 = y.y;
	result.u32 = y.z;

	result.u13 = z.x;
	result.u23 = z.y;
	result.u33 = z.z;

	return result;
}


// roatation around x-axis, y-axis then z-axis (alpha, beta, gamma).
Mat4 matrix_rotation_euler(float alpha, float beta, float gamma) {
	Mat4 m;

	float sa = sinf(alpha);
	float ca = cosf(alpha);

	float sb = sinf(beta);
	float cb = cosf(beta);

	float sc = sinf(gamma);
	float cc = cosf(gamma);

	m.u11 = cb * cc;
	m.u12 = sa * sb * cc - ca * sc;
	m.u13 = ca * sb * cc + sa * sc;

	m.u21 = cb * sc;
	m.u22 = sa * sb * sc + ca * cc;
	m.u23 = ca * sb * sc - sa * cc;

	m.u31 = -sb;
	m.u32 = sa * cb;
	m.u33 = ca * cb;

	m.u44 = 1.0f;

	return m;
}

Mat4 matrix_translation(const Vec3& translation) {
	Mat4 m;
	m.u11 = 1.0f;
	m.u22 = 1.0f;
	m.u33 = 1.0f;
	m.u14 = translation.x;
	m.u24 = translation.y;
	m.u34 = translation.z;
	m.u44 = 1.0f;
	return m;
}

Mat4 matrix_scale(const Vec3& scales) {
	Mat4 m;
	m.u11 = scales.x;
	m.u22 = scales.y;
	m.u33 = scales.z;
	m.u44 = 1.0f;
	return m;
}

Mat4 matrix_scale(float scale) {
	Vec3 s;
	s.x = scale;
	s.y = scale;
	s.z = scale;
	return matrix_scale(s);
}


Mat4 matrix_unit() {
	Mat4 m;
	m.u11 = 1.0f;
	m.u22 = 1.0f;
	m.u33 = 1.0f;
	m.u44 = 1.0f;
	return m;
}


Mat4 matrix_camera(Vec3 pos, Vec3 looking_direction, Vec3 up) {
	return matrix_look_at(pos, pos + looking_direction, up);
}

Mat4 matrix_look_at(Vec3 eye, Vec3 target, Vec3 up) {

	Vec3 forward = target - eye;
	normalize_or_z_axis(&forward);
	Vec3 side = cross(forward, up);
	normalize_or_z_axis(&side);
	up = cross(side, forward);
	normalize_or_z_axis(&up);

	Mat4 result;

	result.u11 = side.x;
	result.u12 = side.y;
	result.u13 = side.z;

	result.u21 = up.x;
	result.u22 = up.y;
	result.u23 = up.z;

	result.u31 = -forward.x;
	result.u32 = -forward.y;
	result.u33 = -forward.z;

	result.u14 = -dot(side, eye);
	result.u24 = -dot(up, eye);
	result.u34 = dot(forward, eye);

	result.u44 = 1.0f;

	return result;
}

// https://www.songho.ca/opengl/gl_projectionmatrix.html
// vertical_fov in degrees
Mat4 matrix_perspective_projection(float vertical_fov, float aspect, float near, float far) {

	Mat4 mat;

	float tan_half_fov = tanf(vertical_fov * 0.5f * DEG_TO_RAD);
	float top = near * tan_half_fov;
	float right = top * aspect;
	float inv_dist = 1.0f / (far - near);

	mat.u11 = near / right;
	mat.u22 = near / top;
	mat.u33 = -(near + far) * inv_dist;
	mat.u43 = -1.0f; // w = -z
	mat.u34 = -(2 * near * far) * inv_dist;
	mat.u44 = 0.0f;

	return mat;
}

// -1 to 1
Mat4 matrix_orthographic_projection(float left, float right, float top, float bottom, float near, float far) {
	Mat4 m;

	m.u11 = 2.0f / (right - left);
	m.u22 = 2.0f / (top - bottom);
	m.u33 = -2.0f / (far - near);

	m.u14 = -(right + left) / (right - left);
	m.u24 = -(top + bottom) / (top - bottom);
	m.u34 = -(far + near) / (far - near);

	m.u44 = 1.0f;

	return m;
}


// convenience wrappers
Mat4 model_rotation_0() {
	return matrix_from_basis_vectors({ 1,0,0 }, { 0,1,0 }, { 0,0,1 });
}

Mat4 model_rotation_90() {
	return matrix_from_basis_vectors({ 0,1,0 }, { -1,0,0 }, { 0,0,1 });
}

Mat4 model_rotation_180() {
	return matrix_from_basis_vectors({ -1,0,0 }, { 0,-1,0 }, { 0,0,1 });
}

Mat4 model_rotation_270() {
	return matrix_from_basis_vectors({ 0,-1,0 }, { 1,0,0 }, { 0,0,1 });
}

*/

// TEMP buffer
static char mma_temp_buffer_internal[MMA_TEMP_BUFFER_CAP_INTERNAL];
static size_t mma_temp_size_internal = 0;
void mma_temp_reset(void){ mma_temp_size_internal = 0; }
void* mma_temp_alloc(size_t requested_size) {
    size_t word_size = sizeof(uintptr_t);
    size_t size = (requested_size + word_size - 1)/word_size*word_size;
    if (mma_temp_size_internal + size > MMA_TEMP_BUFFER_CAP_INTERNAL) {
		printf("ERROR: mma_temp_alloc(): the buffer requested_size is too large for it to fit in %d kB\n", MMA_TEMP_BUFFER_CAP_INTERNAL/1024);
		return 0;
	}
    void *result = &mma_temp_buffer_internal[mma_temp_size_internal];
    mma_temp_size_internal += size;
    return result;
};

#define MMA_RESTORE_STACK_SIZE 1024
static size_t mma_restore_points_stack_internal[MMA_RESTORE_STACK_SIZE];
static size_t mma_restore_points_stack_pointer_internal = 0;

void mma_temp_set_restore_point() {
	assert(mma_restore_points_stack_pointer_internal < MMA_RESTORE_STACK_SIZE-1);
	mma_restore_points_stack_internal[mma_restore_points_stack_pointer_internal++] = mma_temp_size_internal;
}
void mma_temp_restore() {
	if (mma_restore_points_stack_pointer_internal > 0) {
		mma_restore_points_stack_pointer_internal--;
		mma_temp_size_internal = mma_restore_points_stack_internal[mma_restore_points_stack_pointer_internal];
	}
}


// end temp allocator


//
// electronics stuff
//

struct Mma_Complex_2x2 calc_t_from_s(struct Complex s[2][2]) {
	struct Mma_Complex_2x2 t_out;
	t_out.v[0][0] = mma_complex_divide_or_zero(mma_complex(1.0, 0), s[1][0]);
    t_out.v[0][1] = mma_complex_divide_or_zero(mma_complex_negate(s[1][1]), s[1][0]);
	t_out.v[1][0] = mma_complex_divide_or_zero(s[0][0], s[1][0]);
    t_out.v[1][1] = mma_complex_subtract(s[0][1], mma_complex_divide_or_zero(mma_complex_mult(s[0][0], s[1][1]), s[1][0]));
	return t_out;
}

struct Mma_Complex_2x2 calc_s_from_t(struct Complex t[2][2]) {
	struct Mma_Complex_2x2 s_out;
    s_out.v[0][0] = mma_complex_divide_or_zero(t[1][0], t[0][0]);
    s_out.v[0][1] = mma_complex_subtract(t[1][1], mma_complex_divide_or_zero(mma_complex_mult(t[0][1], t[1][0]), t[0][0]));
    s_out.v[1][0] = mma_complex_divide_or_zero(mma_complex(1.0, 0), t[0][0]);
    s_out.v[1][1] = mma_complex_divide_or_zero(mma_complex_negate(t[0][1]), t[0][0]);
	return s_out;
}

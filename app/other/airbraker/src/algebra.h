#include <math.h>
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

struct vec2 {
    float s[2];
};
struct vec2T {
    float s[2];
};

struct vec2T transpose_vec2(const struct vec2 *v) { return (struct vec2T) {.s = {v->s[0], v->s[1]}}; }

struct vec4 {
    float s[4];
};
struct vec4T {
    float s[4];
};
struct vec4T transpose_vec4(const struct vec4 *v) { return (struct vec4T) {.s = {v->s[0], v->s[1], v->s[2], v->s[3]}}; }

struct mat4 {
    struct vec4 vs[4];
};
struct mat2 {
    struct vec2 vs[2];
};

struct mat2x4 {
    struct vec4 vs[2];
};
struct mat4x2 {
    struct vec2 vs[4];
};
struct mat4x2 transpose_mat4_2(const struct mat2x4 *v) { 
    return (struct mat4x2) {.vs = {
        {v->vs[0].s[0], v->vs[1].s[0]},
        {v->vs[0].s[1], v->vs[1].s[1]},
        {v->vs[0].s[2], v->vs[1].s[2]},
        {v->vs[0].s[3], v->vs[1].s[3]},
    }}; 
}


struct mat2 mul_s_m2(float scalar, const struct mat2 *mat) {
    struct mat2 out = {0};
    out.vs[0].s[0] = mat->vs[0].s[0] * scalar;
    out.vs[0].s[1] = mat->vs[0].s[1] * scalar;
    out.vs[1].s[0] = mat->vs[1].s[0] * scalar;
    out.vs[1].s[1] = mat->vs[1].s[1] * scalar;
    return out;
}

struct mat4 mul_m4_m4(const struct mat4 *a, const struct mat4 *b) {
    struct mat4 out = {0};
    for (size_t i = 0; i < 4; i++) {
        for (size_t j = 0; j < 4; j++) {
            for (size_t k = 0; k < 4; k++) {
                out.vs[i].s[j] += a->vs[i].s[k] * b->vs[k].s[j];
            }
        }
    }
    return out;
}

struct vec4 mul_m4_v4(const struct mat4 *m, const struct vec4 *v) {
    struct vec4 out = {0};
    for (size_t i = 0; i < 4; i++) {
        for (size_t j = 0; j < 4; j++) {
            out.s[i] += m->vs[i].s[j] *  v->s[j];
        }
    }
    return out;
}

struct mat4 transpose_m4(const struct mat4 *inp){
    struct mat4 out = {0};
    for (size_t i = 0; i < 4; i++){
        for (size_t j = 0; j < 4; j++){
            out.vs[i].s[j] = inp->vs[j].s[i];
        }
    }
    return out;
}

bool inv2x2(const struct mat2 *in, struct mat2 *out) {
    float det = (*in).vs[0].s[0] * (*in).vs[1].s[1] - (*in).vs[0].s[1] * (*in).vs[1].s[0]; // ad -bc;
    if (det == 0) {
        return false;
    }
    struct mat2 temp = {0};
    temp.vs[0].s[0] = in->vs[1].s[1];
    temp.vs[1].s[1] = in->vs[0].s[0];
    temp.vs[0].s[1] = -in->vs[0].s[1];
    temp.vs[1].s[0] = -in->vs[1].s[0];

    *out = mul_s_m2((1 / det), &temp);
    return true;
}
/**
 * clay_kernel.c — Clay CAD native kernel
 *
 * Single kernel for the Clay module, compiled to both:
 * - Native code (via clang++, for LLVM backend)
 * - WebAssembly (via emcc, for WASM backend)
 *
 * Wraps OpenCascade (OCCT) via the C API in occt_wrapper.h.
 * Pure C — all C++ is isolated in occt_wrapper.cpp.
 *
 * Architecture:
 * - Stone records (StoneRecord*) carry geometry as opaque _handle fields
 * - _handle stores an integer body/query/sketch/profile handle
 * - Handle tables (static arrays) map handles to OcctShape pointers
 * - Primitives create OCCT shapes, store in handle table, wrap in Feature records
 * - Booleans combine OCCT shapes
 * - Queries enumerate/filter topology
 * - get_brep/get_mesh extract tessellated mesh data into Stone arrays
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <stdarg.h>

#include "occt_wrapper.h"

/* Stone runtime types — shared header with stone_record_abi.c / stone_runtime.c */
#include "stone_record_abi.h"

/* ============================================================================
 * Error Reporting
 * ============================================================================ */

#define CLAY_ERROR_BUF_SIZE 512
static char g_clay_error[CLAY_ERROR_BUF_SIZE] = {0};
static int g_clay_has_error = 0;

static void clay_set_error(const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    vsnprintf(g_clay_error, CLAY_ERROR_BUF_SIZE, fmt, args);
    va_end(args);
    g_clay_has_error = 1;
    fprintf(stderr, "%s\n", g_clay_error);
}

/* Called by the bridge after each operation to check for errors */
const char* stone_clay_get_error(void) {
    if (!g_clay_has_error) return NULL;
    g_clay_has_error = 0;
    return g_clay_error;
}

/* ============================================================================
 * Handle Tables
 * ============================================================================ */

#define MAX_BODIES   4096
#define MAX_QUERIES  8192
#define MAX_PROFILES 2048

typedef struct {
    OcctShape shape;  /* heap-allocated TopoDS_Shape* via occt_wrapper */
    int valid;
} BodyEntry;

/* Query filter types */
#define FILTER_NORMAL     0
#define FILTER_CONVEXITY  1
#define FILTER_EDGE_TYPE  2
#define FILTER_AXIS       3
#define FILTER_GEOMETRIC  4
#define FILTER_PLANE      5
#define FILTER_DISTANCE   6

typedef struct {
    int type;
    double params[12]; /* type-specific params */
} FilterSpec;

#define MAX_FILTERS 16

typedef struct {
    int body_handle;
    int kind;                 /* 0=faces, 1=edges, 2=vertices */
    int filter_count;
    FilterSpec filters[MAX_FILTERS];
    int selector;             /* 0=none, 1=first, 2=last */
    int combined_op;          /* 0=none, 1=union, 2=subtract, 3=intersect */
    int combined_a, combined_b;
    int derived_kind;         /* 0=none, 1=edges_of, 2=faces_of */
    int parent_query;
    int valid;
    /* Resolved indices cache — avoids re-computing expensive OCCT queries */
    int* cached_indices;
    int cached_count;
} QueryEntry;

typedef struct {
    OcctShape face_shape;     /* 2D face for extrusion */
    int valid;
} ProfileEntry;

static BodyEntry   g_bodies[MAX_BODIES];
static QueryEntry  g_queries[MAX_QUERIES];
static ProfileEntry g_profiles[MAX_PROFILES];
static int g_next_body    = 1;
static int g_next_query   = 1;
static int g_next_profile = 1;

static int store_body(OcctShape shape) {
    if (g_next_body >= MAX_BODIES) {
        clay_set_error("clay: body handle table full");
        return 0;
    }
    int h = g_next_body++;
    g_bodies[h].shape = shape;
    g_bodies[h].valid = 1;
    return h;
}

static OcctShape get_body_shape(int handle) {
    if (handle < 1 || handle >= MAX_BODIES || !g_bodies[handle].valid) return NULL;
    return g_bodies[handle].shape;
}

/**
 * Check if two QueryEntries are structurally identical (same body, kind,
 * filters, selector, combined ops, derived kind, parent).  Reusing an
 * existing handle lets the per-entry index cache avoid redundant OCCT work
 * when the same query chain is built multiple times (e.g. eight calls to
 * `edges(bracket) |> convex_edges` in a single script).
 */
static int queries_equal(const QueryEntry* a, const QueryEntry* b) {
    if (a->body_handle != b->body_handle) return 0;
    if (a->kind != b->kind) return 0;
    if (a->filter_count != b->filter_count) return 0;
    if (a->selector != b->selector) return 0;
    if (a->combined_op != b->combined_op) return 0;
    if (a->combined_a != b->combined_a) return 0;
    if (a->combined_b != b->combined_b) return 0;
    if (a->derived_kind != b->derived_kind) return 0;
    if (a->parent_query != b->parent_query) return 0;
    for (int i = 0; i < a->filter_count; i++) {
        if (a->filters[i].type != b->filters[i].type) return 0;
        if (memcmp(a->filters[i].params, b->filters[i].params, sizeof(a->filters[i].params)) != 0) return 0;
    }
    return 1;
}

static int store_query(QueryEntry q) {
    /* Deduplicate: return existing handle if an identical query already exists */
    for (int i = 1; i < g_next_query; i++) {
        if (g_queries[i].valid && queries_equal(&g_queries[i], &q)) {
            return i;
        }
    }

    if (g_next_query >= MAX_QUERIES) {
        clay_set_error("clay: query handle table full");
        return 0;
    }
    int h = g_next_query++;
    q.valid = 1;
    g_queries[h] = q;
    return h;
}

static QueryEntry* get_query(int handle) {
    if (handle < 1 || handle >= MAX_QUERIES || !g_queries[handle].valid) return NULL;
    return &g_queries[handle];
}

static int store_profile(OcctShape face) {
    if (g_next_profile >= MAX_PROFILES) {
        clay_set_error("clay: profile handle table full");
        return 0;
    }
    int h = g_next_profile++;
    g_profiles[h].face_shape = face;
    g_profiles[h].valid = 1;
    return h;
}

static OcctShape get_profile_shape(int handle) {
    if (handle < 1 || handle >= MAX_PROFILES || !g_profiles[handle].valid) return NULL;
    return g_profiles[handle].face_shape;
}

/* ============================================================================
 * Stone FFI Helpers
 * ============================================================================ */

static double get_field(StoneRecord* spec, const char* key, double def) {
    if (!spec) return def;
    for (int i = 0; i < spec->count; i++) {
        if (spec->keys[i] && strcmp(spec->keys[i], key) == 0)
            return spec->values[i];
    }
    return def;
}

static StoneArray* get_array_field(StoneRecord* spec, const char* key) {
    if (!spec) return NULL;
    return (StoneArray*)stone_record_get_ptr(spec, key);
}

static StoneRecord* get_record_field(StoneRecord* spec, const char* key) {
    if (!spec) return NULL;
    return (StoneRecord*)stone_record_get_ptr(spec, key);
}

static void get_xyz(StoneArray* arr, double* x, double* y, double* z) {
    *x = 0; *y = 0; *z = 0;
    if (!arr) return;
    if (arr->length >= 1) *x = arr->data[0];
    if (arr->length >= 2) *y = arr->data[1];
    if (arr->length >= 3) *z = arr->data[2];
}

static void set_ptr_field(StoneRecord* r, const char* key, void* ptr) {
    stone_record_set_ptr(r, key, ptr);
}

/* ============================================================================
 * Feature / Body / Query Record Construction
 * ============================================================================ */

static StoneRecord* make_body_record(int body_handle) {
    StoneRecord* body = stone_record_new(3);
    stone_record_set_ptr(body, "_type", (void*)"Body");
    stone_record_set(body, "_handle", (double)body_handle);
    return body;
}

static StoneRecord* make_feature(OcctShape shape) {
    int bh = store_body(shape);
    StoneRecord* feature = stone_record_new(4);
    stone_record_set_ptr(feature, "_type", (void*)"Feature");
    StoneRecord* body = make_body_record(bh);
    set_ptr_field(feature, "body", body);
    StoneRecord* selections = stone_record_new(0);
    set_ptr_field(feature, "selections", selections);
    return feature;
}

static StoneRecord* make_query_record(int query_handle, const char* kind) {
    StoneRecord* q = stone_record_new(3);
    stone_record_set_ptr(q, "_type", (void*)"Query");
    stone_record_set(q, "_handle", (double)query_handle);
    stone_record_set_ptr(q, "kind", (void*)kind);
    return q;
}

static StoneRecord* make_profile_record(int profile_handle) {
    StoneRecord* p = stone_record_new(2);
    stone_record_set_ptr(p, "_type", (void*)"Profile");
    stone_record_set(p, "_handle", (double)profile_handle);
    return p;
}

static int get_body_handle(StoneRecord* feature) {
    if (!feature) return 0;
    StoneRecord* body = get_record_field(feature, "body");
    if (!body) return 0;
    return (int)stone_record_get(body, "_handle");
}

static OcctShape get_shape_from_feature(StoneRecord* feature) {
    return get_body_shape(get_body_handle(feature));
}

static OcctShape get_shape_from_body(StoneRecord* body) {
    if (!body) return NULL;
    int h = (int)stone_record_get(body, "_handle");
    return get_body_shape(h);
}

/* ============================================================================
 * PRIMITIVE CONSTRUCTORS
 * ============================================================================ */

StoneRecord* stone_clay_box(StoneRecord* spec) {
    StoneArray* size_arr = get_array_field(spec, "size");
    StoneArray* center_arr = get_array_field(spec, "center");

    double sx, sy, sz;
    get_xyz(size_arr, &sx, &sy, &sz);
    if (sx <= 0) sx = 1; if (sy <= 0) sy = 1; if (sz <= 0) sz = 1;

    double cx, cy, cz;
    get_xyz(center_arr, &cx, &cy, &cz);

    OcctShape shape = occt_make_box(sx, sy, sz, cx, cy, cz);
    return make_feature(shape);
}

StoneRecord* stone_clay_cylinder(StoneRecord* spec) {
    double radius = get_field(spec, "radius", 1.0);
    double height = get_field(spec, "height", 1.0);
    StoneArray* center_arr = get_array_field(spec, "center");

    double cx, cy, cz;
    get_xyz(center_arr, &cx, &cy, &cz);

    OcctShape shape = occt_make_cylinder(radius, height, cx, cy, cz);
    return make_feature(shape);
}

StoneRecord* stone_clay_sphere(StoneRecord* spec) {
    double radius = get_field(spec, "radius", 1.0);
    StoneArray* center_arr = get_array_field(spec, "center");

    double cx, cy, cz;
    get_xyz(center_arr, &cx, &cy, &cz);

    OcctShape shape = occt_make_sphere(radius, cx, cy, cz);
    return make_feature(shape);
}

StoneRecord* stone_clay_cone(StoneRecord* spec) {
    double radius1 = get_field(spec, "radius1", 1.0);
    double radius2 = get_field(spec, "radius2", 0.0);
    double height = get_field(spec, "height", 1.0);
    StoneArray* center_arr = get_array_field(spec, "center");

    double cx, cy, cz;
    get_xyz(center_arr, &cx, &cy, &cz);

    OcctShape shape = occt_make_cone(radius1, radius2, height, cx, cy, cz);
    return make_feature(shape);
}

StoneRecord* stone_clay_torus(StoneRecord* spec) {
    double major_r = get_field(spec, "major_radius", 10.0);
    double minor_r = get_field(spec, "minor_radius", 3.0);
    StoneArray* center_arr = get_array_field(spec, "center");

    double cx, cy, cz;
    get_xyz(center_arr, &cx, &cy, &cz);

    OcctShape shape = occt_make_torus(major_r, minor_r, cx, cy, cz);
    return make_feature(shape);
}

/* ============================================================================
 * BOOLEAN OPERATIONS
 * ============================================================================ */

StoneRecord* stone_clay_union(StoneRecord* target, StoneRecord* tool) {
    OcctShape a = get_shape_from_feature(target);
    OcctShape b = get_shape_from_feature(tool);
    if (!a || !b) {
        clay_set_error("clay: union() received null shape — check that both arguments are valid Features");
        return target;
    }
    OcctShape result = occt_fuse(a, b);
    if (!result) { clay_set_error("clay: union() failed — OCCT boolean operation error"); return target; }
    return make_feature(result);
}

StoneRecord* stone_clay_subtract(StoneRecord* target, StoneRecord* tool) {
    OcctShape a = get_shape_from_feature(target);
    OcctShape b = get_shape_from_feature(tool);
    if (!a || !b) {
        clay_set_error("clay: subtract() received null shape — check that both arguments are valid Features");
        return target;
    }
    OcctShape result = occt_cut(a, b);
    if (!result) { clay_set_error("clay: subtract() failed — OCCT boolean operation error"); return target; }
    return make_feature(result);
}

StoneRecord* stone_clay_intersect(StoneRecord* target, StoneRecord* tool) {
    OcctShape a = get_shape_from_feature(target);
    OcctShape b = get_shape_from_feature(tool);
    if (!a || !b) {
        clay_set_error("clay: intersect() received null shape — check that both arguments are valid Features");
        return target;
    }
    OcctShape result = occt_common(a, b);
    if (!result) { clay_set_error("clay: intersect() failed — OCCT boolean operation error"); return target; }
    return make_feature(result);
}

/* ============================================================================
 * BATCH BOOLEAN OPERATIONS (multi-tool, single OCCT pass)
 * ============================================================================ */

#define MAX_BATCH_TOOLS 256

static int extract_tool_shapes(StoneArray* tools, OcctShape* out) {
    long long n = stone_array_len(tools);
    if (n > MAX_BATCH_TOOLS) n = MAX_BATCH_TOOLS;
    int count = 0;
    for (long long i = 0; i < n; i++) {
        double dval = stone_array_get(tools, i);
        // Array elements store pointers as (double)(intptr_t)ptr — use reverse conversion
        // (NOT memcpy — that reinterprets IEEE 754 bits, but we need float→int conversion)
        StoneRecord* rec = (StoneRecord*)(intptr_t)dval;
        OcctShape s = get_shape_from_feature(rec);
        if (s) out[count++] = s;
    }
    return count;
}

StoneRecord* stone_clay_union_all(StoneRecord* target, StoneArray* tools) {
    OcctShape base = get_shape_from_feature(target);
    if (!base) return target;
    OcctShape tool_shapes[MAX_BATCH_TOOLS];
    int n = extract_tool_shapes(tools, tool_shapes);
    if (n == 0) return target;
    OcctShape result = occt_fuse_multi(base, tool_shapes, n);
    if (!result) { clay_set_error("clay: union() with array failed — OCCT batch boolean error"); return target; }
    return make_feature(result);
}

StoneRecord* stone_clay_subtract_all(StoneRecord* target, StoneArray* tools) {
    OcctShape base = get_shape_from_feature(target);
    if (!base) return target;
    OcctShape tool_shapes[MAX_BATCH_TOOLS];
    int n = extract_tool_shapes(tools, tool_shapes);
    if (n == 0) return target;
    OcctShape result = occt_cut_multi(base, tool_shapes, n);
    if (!result) { clay_set_error("clay: subtract() with array failed — OCCT batch boolean error"); return target; }
    return make_feature(result);
}

StoneRecord* stone_clay_intersect_all(StoneRecord* target, StoneArray* tools) {
    OcctShape base = get_shape_from_feature(target);
    if (!base) return target;
    OcctShape tool_shapes[MAX_BATCH_TOOLS];
    int n = extract_tool_shapes(tools, tool_shapes);
    if (n == 0) return target;
    OcctShape result = occt_common_multi(base, tool_shapes, n);
    if (!result) { clay_set_error("clay: intersect() with array failed — OCCT batch boolean error"); return target; }
    return make_feature(result);
}

/* ============================================================================
 * TRANSFORMS
 * ============================================================================ */

StoneRecord* stone_clay_translate(StoneRecord* target, StoneRecord* spec) {
    OcctShape s = get_shape_from_feature(target);
    if (!s) return target;

    StoneArray* offset = get_array_field(spec, "offset");
    if (!offset) offset = get_array_field(spec, "vector");
    double dx, dy, dz;
    get_xyz(offset, &dx, &dy, &dz);

    OcctShape result = occt_translate(s, dx, dy, dz);
    return make_feature(result);
}

StoneRecord* stone_clay_rotate(StoneRecord* target, StoneRecord* spec) {
    OcctShape s = get_shape_from_feature(target);
    if (!s) return target;

    double angle = get_field(spec, "angle", 0.0);
    StoneArray* axis_arr = get_array_field(spec, "axis");
    StoneArray* center_arr = get_array_field(spec, "center");

    double ax, ay, az;
    get_xyz(axis_arr, &ax, &ay, &az);
    if (ax == 0 && ay == 0 && az == 0) az = 1; /* default Z axis */

    double cx, cy, cz;
    get_xyz(center_arr, &cx, &cy, &cz);

    OcctShape result = occt_rotate(s, angle, cx, cy, cz, ax, ay, az);
    return make_feature(result);
}

StoneRecord* stone_clay_scale(StoneRecord* target, StoneRecord* spec) {
    OcctShape s = get_shape_from_feature(target);
    if (!s) return target;

    double factor = get_field(spec, "factor", 1.0);
    StoneArray* center_arr = get_array_field(spec, "center");
    double cx, cy, cz;
    get_xyz(center_arr, &cx, &cy, &cz);

    OcctShape result = occt_scale(s, factor, cx, cy, cz);
    return make_feature(result);
}

StoneRecord* stone_clay_mirror(StoneRecord* target, StoneRecord* spec) {
    OcctShape s = get_shape_from_feature(target);
    if (!s) return target;

    StoneArray* normal_arr = get_array_field(spec, "normal");
    StoneArray* origin_arr = get_array_field(spec, "origin");

    double nx, ny, nz;
    get_xyz(normal_arr, &nx, &ny, &nz);
    if (nx == 0 && ny == 0 && nz == 0) nz = 1;

    double ox, oy, oz;
    get_xyz(origin_arr, &ox, &oy, &oz);

    OcctShape mirrored = occt_mirror(s, nx, ny, nz, ox, oy, oz);

    /* Check keep_original flag */
    double keep = get_field(spec, "keep_original", 0.0);
    if (keep != 0.0) {
        OcctShape combined = occt_fuse(s, mirrored);
        return make_feature(combined);
    }
    return make_feature(mirrored);
}

StoneRecord* stone_clay_linear_pattern(StoneRecord* target, StoneRecord* spec) {
    OcctShape s = get_shape_from_feature(target);
    if (!s) return target;

    StoneArray* dir_arr = get_array_field(spec, "direction");
    double dx, dy, dz;
    get_xyz(dir_arr, &dx, &dy, &dz);

    double spacing = get_field(spec, "spacing", 10.0);
    int count = (int)get_field(spec, "count", 2.0);
    if (count < 1) count = 1;
    if (count > 100) count = 100;

    /* Normalize direction */
    double len = sqrt(dx*dx + dy*dy + dz*dz);
    if (len < 1e-12) return make_feature(s);
    dx /= len; dy /= len; dz /= len;

    /* Start with original shape, fuse translated copies */
    OcctShape result = s;
    for (int i = 1; i < count; i++) {
        double offset = spacing * i;
        OcctShape copy = occt_translate(s, dx * offset, dy * offset, dz * offset);
        OcctShape combined = occt_fuse(result, copy);
        if (combined) result = combined;
    }
    return make_feature(result);
}

StoneRecord* stone_clay_circular_pattern(StoneRecord* target, StoneRecord* spec) {
    OcctShape s = get_shape_from_feature(target);
    if (!s) return target;

    StoneArray* axis_arr = get_array_field(spec, "axis");
    StoneArray* center_arr = get_array_field(spec, "center");
    double ax, ay, az;
    get_xyz(axis_arr, &ax, &ay, &az);
    if (ax == 0 && ay == 0 && az == 0) az = 1;

    double cx, cy, cz;
    get_xyz(center_arr, &cx, &cy, &cz);

    int count = (int)get_field(spec, "count", 4.0);
    double total_angle = get_field(spec, "angle", 360.0);
    if (count < 1) count = 1;
    if (count > 100) count = 100;

    double step = total_angle / (double)count;

    OcctShape result = s;
    for (int i = 1; i < count; i++) {
        OcctShape copy = occt_rotate(s, step * i, cx, cy, cz, ax, ay, az);
        OcctShape combined = occt_fuse(result, copy);
        if (combined) result = combined;
    }
    return make_feature(result);
}

/* ============================================================================
 * QUERY SYSTEM
 * ============================================================================ */

static const char* kind_string(int kind) {
    switch (kind) {
        case 0: return "faces";
        case 1: return "edges";
        case 2: return "vertices";
        default: return "faces";
    }
}

StoneRecord* stone_clay_query_faces(StoneRecord* body_rec) {
    int bh = (int)stone_record_get(body_rec, "_handle");
    QueryEntry q = {0};
    q.body_handle = bh;
    q.kind = 0; /* faces */
    int qh = store_query(q);
    return make_query_record(qh, "faces");
}

StoneRecord* stone_clay_query_edges(StoneRecord* body_rec) {
    int bh = (int)stone_record_get(body_rec, "_handle");
    QueryEntry q = {0};
    q.body_handle = bh;
    q.kind = 1; /* edges */
    int qh = store_query(q);
    return make_query_record(qh, "edges");
}

StoneRecord* stone_clay_query_vertices(StoneRecord* body_rec) {
    int bh = (int)stone_record_get(body_rec, "_handle");
    QueryEntry q = {0};
    q.body_handle = bh;
    q.kind = 2; /* vertices */
    int qh = store_query(q);
    return make_query_record(qh, "vertices");
}

StoneRecord* stone_clay_query_edges_of(StoneRecord* face_query_rec) {
    int parent_qh = (int)stone_record_get(face_query_rec, "_handle");
    QueryEntry* parent = get_query(parent_qh);
    if (!parent) return make_query_record(0, "edges");

    QueryEntry q = {0};
    q.body_handle = parent->body_handle;
    q.kind = 1; /* edges */
    q.derived_kind = 1; /* edges_of */
    q.parent_query = parent_qh;
    int qh = store_query(q);
    return make_query_record(qh, "edges");
}

StoneRecord* stone_clay_query_faces_of(StoneRecord* edge_query_rec) {
    int parent_qh = (int)stone_record_get(edge_query_rec, "_handle");
    QueryEntry* parent = get_query(parent_qh);
    if (!parent) return make_query_record(0, "faces");

    QueryEntry q = {0};
    q.body_handle = parent->body_handle;
    q.kind = 0; /* faces */
    q.derived_kind = 2; /* faces_of */
    q.parent_query = parent_qh;
    int qh = store_query(q);
    return make_query_record(qh, "faces");
}

StoneRecord* stone_clay_filter_query(StoneRecord* query_rec, StoneRecord* filter_spec) {
    int qh = (int)stone_record_get(query_rec, "_handle");
    QueryEntry* src = get_query(qh);
    if (!src) return query_rec;

    /* Create child query that references parent — resolution will resolve
       parent first (using cache), then apply only the new filter. */
    QueryEntry q = {0};
    q.body_handle = src->body_handle;
    q.kind = src->kind;
    q.parent_query = qh;
    q.filter_count = 1;
    {
        FilterSpec* f = &q.filters[0];
        memset(f, 0, sizeof(FilterSpec));

        /* Parse filter type from spec record */
        /* The filter_spec is a Stone record with a "type" string field */
        /* For now, store the filter params generically */
        StoneArray* dir = get_array_field(filter_spec, "direction");
        StoneArray* axis = get_array_field(filter_spec, "axis");
        double convex = get_field(filter_spec, "convex", -1.0);

        if (dir) {
            f->type = FILTER_NORMAL;
            get_xyz(dir, &f->params[0], &f->params[1], &f->params[2]);
        } else if (convex >= 0) {
            f->type = FILTER_CONVEXITY;
            f->params[0] = convex;
        } else if (axis) {
            f->type = FILTER_AXIS;
            get_xyz(axis, &f->params[0], &f->params[1], &f->params[2]);
        } else {
            /* Check for plane filter (at_x, at_y, at_z) */
            StoneRecord* plane_rec = get_record_field(filter_spec, "plane");
            if (plane_rec) {
                f->type = FILTER_PLANE;
                StoneArray* origin = get_array_field(plane_rec, "origin");
                StoneArray* normal = get_array_field(plane_rec, "normal");
                get_xyz(origin, &f->params[0], &f->params[1], &f->params[2]);
                get_xyz(normal, &f->params[3], &f->params[4], &f->params[5]);
                f->params[6] = 0.1; /* tolerance */
            } else {
                /* Check for geometric filter (planar_faces, cylindrical_faces) */
                const char* prop = (const char*)stone_record_get_ptr(filter_spec, "property");
                if (prop) {
                    f->type = FILTER_GEOMETRIC;
                    if (strcmp(prop, "planar") == 0) f->params[0] = 0;
                    else if (strcmp(prop, "cylindrical") == 0) f->params[0] = 1;
                    else if (strcmp(prop, "spherical") == 0) f->params[0] = 2;
                    else if (strcmp(prop, "conical") == 0) f->params[0] = 3;
                    else if (strcmp(prop, "toroidal") == 0) f->params[0] = 4;
                    else f->params[0] = -1;
                }
            }
        }

    }
    int new_qh = store_query(q);
    return make_query_record(new_qh, kind_string(q.kind));
}

StoneRecord* stone_clay_query_combine(StoneRecord* qa_rec, StoneRecord* qb_rec, StoneRecord* op_rec) {
    int qa_h = (int)stone_record_get(qa_rec, "_handle");
    int qb_h = (int)stone_record_get(qb_rec, "_handle");
    QueryEntry* qa = get_query(qa_h);
    if (!qa) return qa_rec;

    double op_val = get_field(op_rec, "op", 1.0);

    QueryEntry q = {0};
    q.body_handle = qa->body_handle;
    q.kind = qa->kind;
    q.combined_op = (int)op_val; /* 1=union, 2=subtract, 3=intersect */
    q.combined_a = qa_h;
    q.combined_b = qb_h;
    int qh = store_query(q);
    return make_query_record(qh, kind_string(q.kind));
}

/* Forward declaration — defined below after query_first/query_last */
static int* resolve_query_indices(int query_handle, int* out_count);

double stone_clay_query_count(StoneRecord* query_rec) {
    int qh = (int)stone_record_get(query_rec, "_handle");
    int n = 0;
    int* indices = resolve_query_indices(qh, &n);
    if (indices) free(indices);
    return (double)n;
}

StoneRecord* stone_clay_query_first(StoneRecord* query_rec) {
    int qh = (int)stone_record_get(query_rec, "_handle");
    QueryEntry* src = get_query(qh);
    if (!src) return query_rec;

    QueryEntry q = *src;
    q.selector = 1; /* first */
    int new_qh = store_query(q);
    return make_query_record(new_qh, kind_string(q.kind));
}

StoneRecord* stone_clay_query_last(StoneRecord* query_rec) {
    int qh = (int)stone_record_get(query_rec, "_handle");
    QueryEntry* src = get_query(qh);
    if (!src) return query_rec;

    QueryEntry q = *src;
    q.selector = 2; /* last */
    int new_qh = store_query(q);
    return make_query_record(new_qh, kind_string(q.kind));
}

/* ============================================================================
 * FEATURE OPERATIONS (fillet, chamfer, shell)
 * ============================================================================ */

/**
 * Resolve a query to a list of entity indices.
 * Returns allocated array of 0-based indices. Caller must free.
 * Sets *out_count to the number of indices.
 */
static int* resolve_query_indices(int query_handle, int* out_count) {
    *out_count = 0;
    QueryEntry* q = get_query(query_handle);
    if (!q) return NULL;

    /* Return cached result if available */
    if (q->cached_indices) {
        *out_count = q->cached_count;
        int* copy = (int*)malloc(q->cached_count * sizeof(int));
        memcpy(copy, q->cached_indices, q->cached_count * sizeof(int));
        return copy;
    }

    OcctShape s = get_body_shape(q->body_handle);
    if (!s) return NULL;

    /* Handle combined queries (combine/exclude/overlap) */
    if (q->combined_op > 0) {
        int na = 0, nb = 0;
        int* ia = resolve_query_indices(q->combined_a, &na);
        int* ib = resolve_query_indices(q->combined_b, &nb);

        int* result = NULL;
        int rc = 0;

        switch (q->combined_op) {
            case 1: { /* union: merge, deduplicate */
                result = (int*)malloc((na + nb) * sizeof(int));
                for (int i = 0; i < na; i++) result[rc++] = ia[i];
                for (int i = 0; i < nb; i++) {
                    int dup = 0;
                    for (int j = 0; j < na; j++) {
                        if (ib[i] == ia[j]) { dup = 1; break; }
                    }
                    if (!dup) result[rc++] = ib[i];
                }
                break;
            }
            case 2: { /* subtract: in A but not in B */
                result = (int*)malloc(na * sizeof(int));
                for (int i = 0; i < na; i++) {
                    int found = 0;
                    for (int j = 0; j < nb; j++) {
                        if (ia[i] == ib[j]) { found = 1; break; }
                    }
                    if (!found) result[rc++] = ia[i];
                }
                break;
            }
            case 3: { /* intersect: in both A and B */
                result = (int*)malloc(na * sizeof(int));
                for (int i = 0; i < na; i++) {
                    int found = 0;
                    for (int j = 0; j < nb; j++) {
                        if (ia[i] == ib[j]) { found = 1; break; }
                    }
                    if (found) result[rc++] = ia[i];
                }
                break;
            }
        }

        if (ia) free(ia);
        if (ib) free(ib);
        /* Cache the resolved result */
        if (rc > 0 && result) {
            q->cached_indices = (int*)malloc(rc * sizeof(int));
            memcpy(q->cached_indices, result, rc * sizeof(int));
            q->cached_count = rc;
        }
        *out_count = rc;
        return result;
    }

    /* Determine starting set of indices */
    int* indices = NULL;
    int count = 0;

    if (q->parent_query > 0 && q->derived_kind == 0) {
        /* Filter child: resolve parent (hits cache if already resolved),
           then apply only this query's filters to the parent's result set. */
        int pc = 0;
        int* parent_indices = resolve_query_indices(q->parent_query, &pc);
        if (pc == 0) {
            if (parent_indices) free(parent_indices);
            return NULL;
        }
        indices = parent_indices;
        count = pc;
    } else if (q->derived_kind == 1) {
        /* edges_of: get edges of the faces selected by parent query */
        int nf = 0;
        int* face_idx = resolve_query_indices(q->parent_query, &nf);
        int total_edges = occt_count_edges(s);
        int* seen = (int*)calloc(total_edges, sizeof(int));
        int* buf = (int*)malloc(total_edges * sizeof(int));
        int bc = 0;

        for (int i = 0; i < nf; i++) {
            int edge_buf[256];
            int ne = occt_edges_of_face(s, face_idx[i], edge_buf, 256);
            for (int j = 0; j < ne; j++) {
                if (!seen[edge_buf[j]]) {
                    seen[edge_buf[j]] = 1;
                    buf[bc++] = edge_buf[j];
                }
            }
        }
        if (face_idx) free(face_idx);
        free(seen);
        indices = buf;
        count = bc;
    } else if (q->derived_kind == 2) {
        /* faces_of: get faces adjacent to edges selected by parent query */
        int ne = 0;
        int* edge_idx = resolve_query_indices(q->parent_query, &ne);
        int total_faces = occt_count_faces(s);
        int* seen = (int*)calloc(total_faces, sizeof(int));
        int* buf = (int*)malloc(total_faces * sizeof(int));
        int bc = 0;

        for (int i = 0; i < ne; i++) {
            int face_buf[16];
            int nf = occt_faces_of_edge(s, edge_idx[i], face_buf, 16);
            for (int j = 0; j < nf; j++) {
                if (!seen[face_buf[j]]) {
                    seen[face_buf[j]] = 1;
                    buf[bc++] = face_buf[j];
                }
            }
        }
        if (edge_idx) free(edge_idx);
        free(seen);
        indices = buf;
        count = bc;
    } else {
        /* Standard: start with all entities, excluding seam edges */
        int total = 0;
        switch (q->kind) {
            case 0: total = occt_count_faces(s); break;
            case 1: total = occt_count_edges(s); break;
            case 2: total = occt_count_vertices(s); break;
        }
        if (total == 0) return NULL;
        indices = (int*)malloc(total * sizeof(int));
        count = 0;
        for (int i = 0; i < total; i++) {
            /* Skip seam edges (same face on both sides — parametric surface wrap) */
            if (q->kind == 1 && occt_is_seam_edge(s, i)) continue;
            indices[count++] = i;
        }
    }

    if (count == 0) {
        if (indices) free(indices);
        return NULL;
    }

    /* Apply filters */
    for (int fi = 0; fi < q->filter_count; fi++) {
        FilterSpec* f = &q->filters[fi];
        int* filtered = (int*)malloc(count * sizeof(int));
        int fc = 0;

        for (int j = 0; j < count; j++) {
            int idx = indices[j];
            int keep = 0;

            switch (f->type) {
                case FILTER_NORMAL: {
                    if (q->kind != 0) { keep = 1; break; }
                    double nx, ny, nz;
                    if (occt_face_normal(s, idx, &nx, &ny, &nz) == 0) {
                        double dot = nx * f->params[0] + ny * f->params[1] + nz * f->params[2];
                        keep = (dot > 0.9);
                    }
                    break;
                }
                case FILTER_CONVEXITY: {
                    if (q->kind != 1) { keep = 1; break; }
                    int conv = occt_edge_convexity(s, idx);
                    int want_convex = (int)f->params[0];
                    keep = want_convex ? (conv == 1) : (conv == -1);
                    break;
                }
                case FILTER_GEOMETRIC: {
                    if (q->kind != 0) { keep = 1; break; }
                    int gtype = occt_face_geom_type(s, idx);
                    keep = (gtype == (int)f->params[0]);
                    break;
                }
                case FILTER_AXIS: {
                    if (q->kind != 1) { keep = 1; break; }
                    double tx, ty, tz;
                    if (occt_edge_tangent_at(s, idx, 0.5, &tx, &ty, &tz) == 0) {
                        double dot = tx * f->params[0] + ty * f->params[1] + tz * f->params[2];
                        keep = (fabs(dot) > 0.99);
                    }
                    break;
                }
                case FILTER_PLANE: {
                    /* params[0..2] = origin, params[3..5] = normal, params[6] = tolerance */
                    double px, py, pz;
                    int ok = -1;
                    if (q->kind == 0) {
                        ok = occt_face_center(s, idx, &px, &py, &pz);
                    } else if (q->kind == 1) {
                        ok = occt_edge_point_at(s, idx, 0.5, &px, &py, &pz);
                    } else if (q->kind == 2) {
                        ok = occt_vertex_position(s, idx, &px, &py, &pz);
                    }
                    if (ok == 0) {
                        double dx = px - f->params[0];
                        double dy = py - f->params[1];
                        double dz = pz - f->params[2];
                        double dist = dx * f->params[3] + dy * f->params[4] + dz * f->params[5];
                        keep = (fabs(dist) < f->params[6]);
                    }
                    break;
                }
                default:
                    keep = 1; /* unknown filter: keep all */
                    break;
            }

            if (keep) filtered[fc++] = idx;
        }

        free(indices);
        indices = filtered;
        count = fc;
    }

    /* Apply selector */
    if (q->selector == 1 && count > 0) {
        count = 1;
    } else if (q->selector == 2 && count > 0) {
        indices[0] = indices[count - 1];
        count = 1;
    }

    /* Cache the resolved result */
    if (count > 0 && indices) {
        q->cached_indices = (int*)malloc(count * sizeof(int));
        memcpy(q->cached_indices, indices, count * sizeof(int));
        q->cached_count = count;
    }

    *out_count = count;
    return indices;
}

StoneRecord* stone_clay_fillet(StoneRecord* target, StoneRecord* spec) {
    OcctShape s = get_shape_from_feature(target);
    if (!s) return target;

    double radius = get_field(spec, "radius", 1.0);

    /* Get query from spec */
    StoneRecord* query_rec = get_record_field(spec, "edges");
    if (!query_rec) query_rec = get_record_field(spec, "query");
    if (!query_rec) {
        clay_set_error("clay: fillet() requires an edges query — use fillet({edges = ..., radius = ...})");
        return target;
    }

    int qh = (int)stone_record_get(query_rec, "_handle");
    int n_edges;
    int* edge_indices = resolve_query_indices(qh, &n_edges);
    if (!edge_indices || n_edges == 0) {
        if (edge_indices) free(edge_indices);
        return target;
    }

    OcctShape result = occt_fillet(s, edge_indices, n_edges, radius);
    free(edge_indices);

    if (!result) {
        clay_set_error("clay: fillet() failed — radius %.2f too large for %d selected edges", radius, n_edges);
        return target;
    }
    if (result == s) {
        /* Shape unchanged — all edges were either filtered or failed */
        clay_set_error("clay: fillet(radius=%.2f) had no effect — %d edges selected but none could be filleted (radius may be too large for the geometry)", radius, n_edges);
    }
    return make_feature(result);
}

StoneRecord* stone_clay_chamfer(StoneRecord* target, StoneRecord* spec) {
    OcctShape s = get_shape_from_feature(target);
    if (!s) return target;

    double distance = get_field(spec, "distance", 1.0);

    StoneRecord* query_rec = get_record_field(spec, "edges");
    if (!query_rec) query_rec = get_record_field(spec, "query");
    if (!query_rec) return target;

    int qh = (int)stone_record_get(query_rec, "_handle");
    int n_edges;
    int* edge_indices = resolve_query_indices(qh, &n_edges);
    if (!edge_indices || n_edges == 0) {
        if (edge_indices) free(edge_indices);
        return target;
    }

    OcctShape result = occt_chamfer(s, edge_indices, n_edges, distance);
    free(edge_indices);

    if (!result) return target;
    return make_feature(result);
}

StoneRecord* stone_clay_shell(StoneRecord* target, StoneRecord* spec) {
    OcctShape s = get_shape_from_feature(target);
    if (!s) return target;

    double thickness = get_field(spec, "thickness", 1.0);

    StoneRecord* query_rec = get_record_field(spec, "faces");
    if (!query_rec) query_rec = get_record_field(spec, "query");
    if (!query_rec) return target;

    int qh = (int)stone_record_get(query_rec, "_handle");
    int n_faces;
    int* face_indices = resolve_query_indices(qh, &n_faces);
    if (!face_indices || n_faces == 0) {
        if (face_indices) free(face_indices);
        return target;
    }

    OcctShape result = occt_shell(s, face_indices, n_faces, thickness);
    free(face_indices);

    if (!result) return target;
    return make_feature(result);
}

/* ============================================================================
 * PROFILE CONSTRUCTORS
 * ============================================================================ */

StoneRecord* stone_clay_profile_rect(StoneRecord* spec) {
    double w = get_field(spec, "width", 10.0);
    double h = get_field(spec, "height", 10.0);
    StoneArray* center_arr = get_array_field(spec, "center");
    double cx = 0, cy = 0, dummy;
    if (center_arr) {
        get_xyz(center_arr, &cx, &cy, &dummy);
    }
    OcctShape face = occt_profile_rect(w, h, cx, cy);
    return make_profile_record(store_profile(face));
}

StoneRecord* stone_clay_profile_circle(StoneRecord* spec) {
    double r = get_field(spec, "radius", 5.0);
    StoneArray* center_arr = get_array_field(spec, "center");
    double cx = 0, cy = 0, dummy;
    if (center_arr) get_xyz(center_arr, &cx, &cy, &dummy);
    OcctShape face = occt_profile_circle(r, cx, cy);
    return make_profile_record(store_profile(face));
}

StoneRecord* stone_clay_profile_ellipse(StoneRecord* spec) {
    double rx = get_field(spec, "rx", 5.0);
    double ry = get_field(spec, "ry", 3.0);
    StoneArray* center_arr = get_array_field(spec, "center");
    double cx = 0, cy = 0, dummy;
    if (center_arr) get_xyz(center_arr, &cx, &cy, &dummy);
    OcctShape face = occt_profile_ellipse(rx, ry, cx, cy);
    return make_profile_record(store_profile(face));
}

StoneRecord* stone_clay_profile_polygon(StoneRecord* spec) {
    StoneArray* pts = get_array_field(spec, "points");
    if (!pts) return make_profile_record(0);

    int n = (int)(pts->length / 2);
    double* flat = (double*)malloc(n * 2 * sizeof(double));
    for (int i = 0; i < n * 2; i++) flat[i] = pts->data[i];

    OcctShape face = occt_profile_polygon(flat, n);
    free(flat);
    return make_profile_record(store_profile(face));
}

StoneRecord* stone_clay_profile_slot(StoneRecord* spec) {
    StoneArray* start_arr = get_array_field(spec, "start");
    StoneArray* end_arr = get_array_field(spec, "end");
    double width = get_field(spec, "width", 5.0);

    double sx = 0, sy = 0, ex = 10, ey = 0, dummy;
    if (start_arr) get_xyz(start_arr, &sx, &sy, &dummy);
    if (end_arr) get_xyz(end_arr, &ex, &ey, &dummy);

    OcctShape face = occt_profile_slot(sx, sy, ex, ey, width);
    return make_profile_record(store_profile(face));
}

StoneRecord* stone_clay_profile_union(StoneRecord* a_rec, StoneRecord* b_rec) {
    int ah = (int)stone_record_get(a_rec, "_handle");
    int bh = (int)stone_record_get(b_rec, "_handle");
    OcctShape a = get_profile_shape(ah);
    OcctShape b = get_profile_shape(bh);
    if (!a || !b) return a_rec;
    OcctShape result = occt_profile_fuse(a, b);
    return make_profile_record(store_profile(result));
}

StoneRecord* stone_clay_profile_subtract(StoneRecord* a_rec, StoneRecord* b_rec) {
    int ah = (int)stone_record_get(a_rec, "_handle");
    int bh = (int)stone_record_get(b_rec, "_handle");
    OcctShape a = get_profile_shape(ah);
    OcctShape b = get_profile_shape(bh);
    if (!a || !b) return a_rec;
    OcctShape result = occt_profile_cut(a, b);
    return make_profile_record(store_profile(result));
}

StoneRecord* stone_clay_profile_intersect(StoneRecord* a_rec, StoneRecord* b_rec) {
    int ah = (int)stone_record_get(a_rec, "_handle");
    int bh = (int)stone_record_get(b_rec, "_handle");
    OcctShape a = get_profile_shape(ah);
    OcctShape b = get_profile_shape(bh);
    if (!a || !b) return a_rec;
    OcctShape result = occt_profile_common(a, b);
    return make_profile_record(store_profile(result));
}

/* ============================================================================
 * EXTRUDE / REVOLVE
 * ============================================================================ */

StoneRecord* stone_clay_extrude(StoneRecord* target, StoneRecord* spec) {
    /* Can extrude a profile or a feature */
    StoneRecord* profile_rec = get_record_field(spec, "profile");
    double distance = get_field(spec, "distance", 10.0);
    StoneArray* dir_arr = get_array_field(spec, "direction");

    double dx = 0, dy = 0, dz = 1;
    if (dir_arr) get_xyz(dir_arr, &dx, &dy, &dz);

    /* Normalize and scale by distance */
    double len = sqrt(dx*dx + dy*dy + dz*dz);
    if (len > 1e-12) { dx /= len; dy /= len; dz /= len; }
    dx *= distance; dy *= distance; dz *= distance;

    OcctShape face = NULL;
    if (profile_rec) {
        int ph = (int)stone_record_get(profile_rec, "_handle");
        face = get_profile_shape(ph);
    }

    if (!face) return target;

    OcctShape solid = occt_extrude(face, dx, dy, dz);
    if (!solid) return target;

    /* If target has existing body, perform boolean (add/cut) */
    OcctShape existing = get_shape_from_feature(target);
    if (existing) {
        double op = get_field(spec, "operation", 0.0);
        if (op == 1.0) {
            /* cut */
            OcctShape result = occt_cut(existing, solid);
            if (result) return make_feature(result);
        } else {
            /* add (default) */
            OcctShape result = occt_fuse(existing, solid);
            if (result) return make_feature(result);
        }
    }

    return make_feature(solid);
}

StoneRecord* stone_clay_revolve(StoneRecord* target, StoneRecord* spec) {
    StoneRecord* profile_rec = get_record_field(spec, "profile");
    double angle = get_field(spec, "angle", 360.0);

    StoneArray* axis_origin_arr = get_array_field(spec, "axis_origin");
    StoneArray* axis_dir_arr = get_array_field(spec, "axis_direction");

    double ax_ox = 0, ax_oy = 0, ax_oz = 0;
    double ax_dx = 0, ax_dy = 0, ax_dz = 1;
    if (axis_origin_arr) get_xyz(axis_origin_arr, &ax_ox, &ax_oy, &ax_oz);
    if (axis_dir_arr) get_xyz(axis_dir_arr, &ax_dx, &ax_dy, &ax_dz);

    OcctShape face = NULL;
    if (profile_rec) {
        int ph = (int)stone_record_get(profile_rec, "_handle");
        face = get_profile_shape(ph);
    }
    if (!face) return target;

    OcctShape solid = occt_revolve(face, ax_ox, ax_oy, ax_oz, ax_dx, ax_dy, ax_dz, angle);
    if (!solid) return target;

    return make_feature(solid);
}

/* ============================================================================
 * SKETCH SYSTEM (simplified — data accumulator matching JS kernel)
 * ============================================================================ */

/* Sketches are simplified: create profiles directly from sketch operations */

StoneRecord* stone_clay_sketch_new(StoneRecord* plane_rec) {
    StoneRecord* sketch = stone_record_new(3);
    stone_record_set_ptr(sketch, "_type", (void*)"Sketch");
    static int sketch_counter = 1;
    stone_record_set(sketch, "_handle", (double)(sketch_counter++));
    set_ptr_field(sketch, "plane", plane_rec);
    return sketch;
}

StoneRecord* stone_clay_sketch_on_face(StoneRecord* face_query_rec) {
    /* Create sketch on a face — extract face plane from query */
    StoneRecord* sketch = stone_record_new(3);
    stone_record_set_ptr(sketch, "_type", (void*)"Sketch");
    static int sketch_on_face_counter = 10000;
    stone_record_set(sketch, "_handle", (double)(sketch_on_face_counter++));
    return sketch;
}

/* Sketch geometry operations return {sketch, entities} */
static StoneRecord* sketch_geom_result(StoneRecord* sketch) {
    StoneRecord* result = stone_record_new(2);
    set_ptr_field(result, "sketch", sketch);
    StoneArray* entities = stone_array_new(0);
    set_ptr_field(result, "entities", entities);
    return result;
}

StoneRecord* stone_clay_sketch_rect(StoneRecord* sketch, StoneRecord* spec) {
    return sketch_geom_result(sketch);
}

StoneRecord* stone_clay_sketch_circle(StoneRecord* sketch, StoneRecord* spec) {
    return sketch_geom_result(sketch);
}

StoneRecord* stone_clay_sketch_ellipse(StoneRecord* sketch, StoneRecord* spec) {
    return sketch_geom_result(sketch);
}

StoneRecord* stone_clay_sketch_line(StoneRecord* sketch, StoneRecord* start, StoneRecord* end) {
    return sketch_geom_result(sketch);
}

StoneRecord* stone_clay_sketch_arc(StoneRecord* sketch, StoneRecord* spec) {
    return sketch_geom_result(sketch);
}

StoneRecord* stone_clay_sketch_polyline(StoneRecord* sketch, StoneRecord* points, StoneRecord* closed) {
    return sketch_geom_result(sketch);
}

StoneRecord* stone_clay_sketch_polygon(StoneRecord* sketch, StoneRecord* spec) {
    return sketch_geom_result(sketch);
}

StoneRecord* stone_clay_sketch_slot(StoneRecord* sketch, StoneRecord* spec) {
    return sketch_geom_result(sketch);
}

StoneRecord* stone_clay_sketch_constrain(StoneRecord* sketch, StoneRecord* constraint) {
    return sketch; /* mock solver — just return sketch */
}

StoneRecord* stone_clay_sketch_solve(StoneRecord* sketch) {
    return sketch; /* mock solver */
}

StoneRecord* stone_clay_sketch_close(StoneRecord* sketch) {
    return sketch;
}

StoneRecord* stone_clay_sketch_profile(StoneRecord* sketch, StoneRecord* selector) {
    /* Return a profile record */
    return make_profile_record(0); /* placeholder */
}

/* Profile-to-3D bridge */
StoneRecord* stone_clay_sketch_from_profile(StoneRecord* plane_rec, StoneRecord* profile_rec) {
    StoneRecord* sketch = stone_clay_sketch_new(plane_rec);
    return sketch;
}

StoneRecord* stone_clay_extrude_sketch(StoneRecord* sketch, StoneRecord* spec) {
    double distance = get_field(spec, "distance", 10.0);

    /* For now, extrude along Z axis */
    StoneRecord* feature = stone_record_new(4);
    stone_record_set_ptr(feature, "_type", (void*)"Feature");
    return feature;
}

/* ============================================================================
 * LEGACY STUBS
 * ============================================================================ */

StoneRecord* stone_clay_init_manifold(void) { return NULL; }
StoneRecord* stone_clay_is_manifold_ready(void) { return NULL; }
StoneRecord* stone_clay_wait_for_manifold(void) { return NULL; }

/* ============================================================================
 * MESH EXTRACTION (get_brep)
 * ============================================================================ */

StoneRecord* stone_clay_get_brep(StoneRecord* body) {
    OcctShape s = get_shape_from_body(body);
    if (!s) {
        clay_set_error("clay: get_brep() received null shape — the Feature has no geometry");
        StoneRecord* out = stone_record_new(5);
        stone_record_set_ptr(out, "type", (void*)"brep_mesh");
        StoneArray* empty = stone_array_new(0);
        set_ptr_field(out, "vertices", empty);
        set_ptr_field(out, "triangles", empty);
        set_ptr_field(out, "normals", empty);
        set_ptr_field(out, "edge_vertices", empty);
        return out;
    }

    OcctMeshResult mesh = occt_mesh(s, 0.1, 20.0);

    /* Build Stone arrays from mesh result */
    StoneArray* vertices = stone_array_new((long long)(mesh.n_verts * 3));
    for (int i = 0; i < mesh.n_verts * 3; i++) {
        stone_array_set(vertices, (long long)i, (double)mesh.vertices[i]);
    }

    StoneArray* triangles = stone_array_new((long long)(mesh.n_tris * 3));
    for (int i = 0; i < mesh.n_tris * 3; i++) {
        stone_array_set(triangles, (long long)i, (double)mesh.triangles[i]);
    }

    StoneArray* normals = stone_array_new((long long)(mesh.n_verts * 3));
    for (int i = 0; i < mesh.n_verts * 3; i++) {
        stone_array_set(normals, (long long)i, (double)mesh.normals[i]);
    }

    StoneArray* edge_vertices = stone_array_new((long long)(mesh.n_edge_pts * 3));
    for (int i = 0; i < mesh.n_edge_pts * 3; i++) {
        stone_array_set(edge_vertices, (long long)i, (double)mesh.edge_lines[i]);
    }

    occt_mesh_free(&mesh);

    StoneRecord* out = stone_record_new(6);
    stone_record_set_ptr(out, "type", (void*)"brep_mesh");
    set_ptr_field(out, "vertices", vertices);
    set_ptr_field(out, "triangles", triangles);
    set_ptr_field(out, "normals", normals);
    set_ptr_field(out, "edge_vertices", edge_vertices);

    return out;
}

/* ============================================================================
 * MESH EXTRACTION (get_mesh) — Plotly-compatible format
 * ============================================================================ */

StoneRecord* stone_clay_get_mesh(StoneRecord* body) {
    OcctShape s = get_shape_from_body(body);
    if (!s) {
        clay_set_error("clay: get_mesh() received null shape — the Feature has no geometry");
        StoneRecord* out = stone_record_new(7);
        stone_record_set_ptr(out, "type", (void*)"mesh");
        StoneArray* empty = stone_array_new(0);
        set_ptr_field(out, "x", empty);
        set_ptr_field(out, "y", empty);
        set_ptr_field(out, "z", empty);
        set_ptr_field(out, "i", empty);
        set_ptr_field(out, "j", empty);
        set_ptr_field(out, "k", empty);
        return out;
    }

    OcctMeshResult mesh = occt_mesh(s, 0.1, 20.0);

    StoneArray* x_arr = stone_array_new((long long)mesh.n_verts);
    StoneArray* y_arr = stone_array_new((long long)mesh.n_verts);
    StoneArray* z_arr = stone_array_new((long long)mesh.n_verts);
    for (int v = 0; v < mesh.n_verts; v++) {
        stone_array_set(x_arr, (long long)v, (double)mesh.vertices[v * 3 + 0]);
        stone_array_set(y_arr, (long long)v, (double)mesh.vertices[v * 3 + 1]);
        stone_array_set(z_arr, (long long)v, (double)mesh.vertices[v * 3 + 2]);
    }

    StoneArray* i_arr = stone_array_new((long long)mesh.n_tris);
    StoneArray* j_arr = stone_array_new((long long)mesh.n_tris);
    StoneArray* k_arr = stone_array_new((long long)mesh.n_tris);
    for (int t = 0; t < mesh.n_tris; t++) {
        stone_array_set(i_arr, (long long)t, (double)mesh.triangles[t * 3 + 0]);
        stone_array_set(j_arr, (long long)t, (double)mesh.triangles[t * 3 + 1]);
        stone_array_set(k_arr, (long long)t, (double)mesh.triangles[t * 3 + 2]);
    }

    occt_mesh_free(&mesh);

    StoneRecord* out = stone_record_new(7);
    stone_record_set_ptr(out, "type", (void*)"mesh");
    set_ptr_field(out, "x", x_arr);
    set_ptr_field(out, "y", y_arr);
    set_ptr_field(out, "z", z_arr);
    set_ptr_field(out, "i", i_arr);
    set_ptr_field(out, "j", j_arr);
    set_ptr_field(out, "k", k_arr);

    return out;
}

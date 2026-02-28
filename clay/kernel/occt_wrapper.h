/**
 * occt_wrapper.h — C API over OpenCascade C++ classes
 *
 * Provides extern "C" functions that clay_kernel.c can call without
 * needing C++ compilation. All OCCT types are hidden behind opaque pointers.
 */

#ifndef OCCT_WRAPPER_H
#define OCCT_WRAPPER_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Opaque handle — wraps a heap-allocated TopoDS_Shape */
typedef void* OcctShape;

/* Opaque handle — wraps a heap-allocated TopoDS_Wire */
typedef void* OcctWire;

/* ============================================================================
 * Lifecycle
 * ============================================================================ */

void occt_init(void);
void occt_cleanup(void);

/* Free a shape handle */
void occt_shape_free(OcctShape s);

/* ============================================================================
 * Primitives
 * ============================================================================ */

/* Box: create centered box then translate to center position */
OcctShape occt_make_box(double sx, double sy, double sz,
                        double cx, double cy, double cz);

/* Cylinder: along Z axis, centered vertically */
OcctShape occt_make_cylinder(double radius, double height,
                             double cx, double cy, double cz);

/* Sphere */
OcctShape occt_make_sphere(double radius,
                           double cx, double cy, double cz);

/* Cone / frustum */
OcctShape occt_make_cone(double radius1, double radius2, double height,
                         double cx, double cy, double cz);

/* Torus */
OcctShape occt_make_torus(double major_r, double minor_r,
                          double cx, double cy, double cz);

/* ============================================================================
 * Boolean Operations
 * ============================================================================ */

OcctShape occt_fuse(OcctShape a, OcctShape b);
OcctShape occt_cut(OcctShape a, OcctShape b);
OcctShape occt_common(OcctShape a, OcctShape b);

/** Multi-tool booleans — single OCCT pass, one topology rebuild */
OcctShape occt_fuse_multi(OcctShape base, OcctShape* tools, int count);
OcctShape occt_cut_multi(OcctShape base, OcctShape* tools, int count);
OcctShape occt_common_multi(OcctShape base, OcctShape* tools, int count);

/* ============================================================================
 * Transforms
 * ============================================================================ */

OcctShape occt_translate(OcctShape s, double dx, double dy, double dz);
OcctShape occt_rotate(OcctShape s, double angle_deg,
                      double cx, double cy, double cz,
                      double ax, double ay, double az);
OcctShape occt_scale(OcctShape s, double factor,
                     double cx, double cy, double cz);
OcctShape occt_mirror(OcctShape s,
                      double nx, double ny, double nz,
                      double ox, double oy, double oz);

/* ============================================================================
 * Feature Operations
 * ============================================================================ */

/**
 * Fillet edges. edge_indices are indices from occt_get_edges().
 * Returns new shape or NULL on failure.
 */
OcctShape occt_fillet(OcctShape s, int* edge_indices, int n_edges, double radius);

/**
 * Chamfer edges. Same indexing as fillet.
 */
OcctShape occt_chamfer(OcctShape s, int* edge_indices, int n_edges, double distance);

/**
 * Shell (hollow). face_indices are indices from occt_get_faces().
 * Positive thickness = outward, negative = inward.
 */
OcctShape occt_shell(OcctShape s, int* face_indices, int n_faces, double thickness);

/**
 * Extrude a 2D profile face along a direction.
 */
OcctShape occt_extrude(OcctShape face_shape, double dx, double dy, double dz);

/**
 * Revolve a 2D profile face around an axis.
 * axis_origin + axis_dir define the axis. angle_deg is revolution angle.
 */
OcctShape occt_revolve(OcctShape face_shape,
                       double ax_ox, double ax_oy, double ax_oz,
                       double ax_dx, double ax_dy, double ax_dz,
                       double angle_deg);

/* ============================================================================
 * Topology Queries
 * ============================================================================ */

/** Count faces/edges/vertices in shape */
int occt_count_faces(OcctShape s);
int occt_count_edges(OcctShape s);
int occt_count_vertices(OcctShape s);

/** Get face normal at center. Returns 0 on success, -1 on failure. */
int occt_face_normal(OcctShape s, int face_idx,
                     double* nx, double* ny, double* nz);

/** Get face center point. Returns 0 on success. */
int occt_face_center(OcctShape s, int face_idx,
                     double* cx, double* cy, double* cz);

/**
 * Get face geometry type.
 * Returns: 0=plane, 1=cylinder, 2=sphere, 3=cone, 4=torus, -1=other
 */
int occt_face_geom_type(OcctShape s, int face_idx);

/** Get edge tangent at parameter t (0.0-1.0). */
int occt_edge_tangent_at(OcctShape s, int edge_idx, double t,
                         double* tx, double* ty, double* tz);

/** Get edge point at parameter t (0.0-1.0). */
int occt_edge_point_at(OcctShape s, int edge_idx, double t,
                       double* px, double* py, double* pz);

/** Get vertex position. */
int occt_vertex_position(OcctShape s, int vert_idx,
                         double* px, double* py, double* pz);

/**
 * Determine edge convexity by examining dihedral angle between adjacent faces.
 * Returns: 1=convex (outside corner), -1=concave (inside corner), 0=flat/boundary
 */
int occt_edge_convexity(OcctShape s, int edge_idx);

/**
 * Get face indices adjacent to an edge.
 * Writes up to max_faces indices into out_face_indices.
 * Returns number of adjacent faces found.
 */
int occt_faces_of_edge(OcctShape s, int edge_idx,
                       int* out_face_indices, int max_faces);

/** Check if an edge is a seam (same face on both sides). Returns 1 if seam, 0 otherwise. */
int occt_is_seam_edge(OcctShape s, int edge_idx);

/**
 * Get edge indices bounding a face.
 * Writes up to max_edges indices into out_edge_indices.
 * Returns number of edges found.
 */
int occt_edges_of_face(OcctShape s, int face_idx,
                       int* out_edge_indices, int max_edges);

/* ============================================================================
 * 2D Profile Construction
 * ============================================================================ */

/** Create a rectangular wire */
OcctShape occt_profile_rect(double width, double height, double cx, double cy);

/** Create a circular wire */
OcctShape occt_profile_circle(double radius, double cx, double cy);

/** Create an elliptical wire */
OcctShape occt_profile_ellipse(double rx, double ry, double cx, double cy);

/** Create a polygon wire from points [x0,y0, x1,y1, ...] */
OcctShape occt_profile_polygon(double* points, int n_points);

/** Create a slot (stadium) wire */
OcctShape occt_profile_slot(double sx, double sy, double ex, double ey, double width);

/** Convert wire to face (fill) */
OcctShape occt_wire_to_face(OcctShape wire);

/** 2D boolean on faces */
OcctShape occt_profile_fuse(OcctShape a, OcctShape b);
OcctShape occt_profile_cut(OcctShape a, OcctShape b);
OcctShape occt_profile_common(OcctShape a, OcctShape b);

/* ============================================================================
 * Meshing
 * ============================================================================ */

typedef struct {
    float* vertices;     /* flat [x,y,z, x,y,z, ...] */
    int*   triangles;    /* flat [i,j,k, i,j,k, ...] */
    float* normals;      /* flat [nx,ny,nz, ...] per vertex */
    float* edge_lines;   /* flat [x,y,z, x,y,z, ...] line segment endpoints */
    int    n_verts;
    int    n_tris;
    int    n_edge_pts;   /* number of edge line points (pairs of 3D points) */
} OcctMeshResult;

/**
 * Tessellate shape to triangles + edge lines.
 * Caller must call occt_mesh_free() on the result.
 */
OcctMeshResult occt_mesh(OcctShape s, double linear_tol, double angular_tol_deg);

/** Free mesh result buffers */
void occt_mesh_free(OcctMeshResult* m);

#ifdef __cplusplus
}
#endif

#endif /* OCCT_WRAPPER_H */

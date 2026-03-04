/**
 * occt_wrapper.cpp — OpenCascade C++ implementations behind extern "C" API
 *
 * Each function creates OCCT objects, performs the operation, and returns
 * a heap-allocated TopoDS_Shape* cast to void* (OcctShape).
 *
 * Memory: All returned OcctShape pointers must be freed with occt_shape_free().
 * In practice, Stone's arena allocator handles lifetime — shapes live until
 * program exit. We use new/delete for TopoDS_Shape storage.
 */

#include "occt_wrapper.h"
#include <cstdio>
#include <stdexcept>

/* OCCT headers — Primitives */
#include <BRepPrimAPI_MakeBox.hxx>
#include <BRepPrimAPI_MakeCylinder.hxx>
#include <BRepPrimAPI_MakeSphere.hxx>
#include <BRepPrimAPI_MakeCone.hxx>
#include <BRepPrimAPI_MakeTorus.hxx>
#include <BRepPrimAPI_MakePrism.hxx>
#include <BRepPrimAPI_MakeRevol.hxx>

/* OCCT headers — Boolean operations */
#include <BRepAlgoAPI_BooleanOperation.hxx>
#include <BRepAlgoAPI_Fuse.hxx>
#include <BRepAlgoAPI_Cut.hxx>
#include <BRepAlgoAPI_Common.hxx>

/* OCCT headers — Transforms */
#include <BRepBuilderAPI_Transform.hxx>
#include <gp_Trsf.hxx>
#include <gp_Vec.hxx>
#include <gp_Pnt.hxx>
#include <gp_Dir.hxx>
#include <gp_Ax1.hxx>
#include <gp_Ax2.hxx>

/* OCCT headers — Features */
#include <BRepFilletAPI_MakeFillet.hxx>
#include <BRepFilletAPI_MakeChamfer.hxx>
#include <BRepOffsetAPI_MakeThickSolid.hxx>

/* OCCT headers — Topology exploration */
#include <TopExp.hxx>
#include <TopExp_Explorer.hxx>
#include <TopTools_IndexedMapOfShape.hxx>
#include <TopTools_IndexedDataMapOfShapeListOfShape.hxx>
#include <TopTools_ListOfShape.hxx>
#include <TopoDS.hxx>
#include <TopoDS_Shape.hxx>
#include <TopoDS_Face.hxx>
#include <TopoDS_Edge.hxx>
#include <TopoDS_Vertex.hxx>
#include <TopoDS_Wire.hxx>

/* OCCT headers — Curve adaptor (for edge midpoint queries) */
#include <BRepAdaptor_Curve.hxx>

/* OCCT headers — Geometry analysis */
#include <BRep_Tool.hxx>
#include <Geom_Surface.hxx>
#include <Geom_Plane.hxx>
#include <Geom_CylindricalSurface.hxx>
#include <Geom_SphericalSurface.hxx>
#include <Geom_ConicalSurface.hxx>
#include <Geom_ToroidalSurface.hxx>
#include <Geom_Curve.hxx>
#include <GeomLProp_SLProps.hxx>
#include <GeomAPI_ProjectPointOnSurf.hxx>
#include <BRepGProp.hxx>
#include <GProp_GProps.hxx>
#include <ShapeAnalysis_Surface.hxx>
#include <BRepTools.hxx>

/* OCCT headers — 2D geometry (for edge convexity via parametric curves) */
#include <Geom2d_Curve.hxx>
#include <gp_Vec2d.hxx>
#include <gp_Pnt2d.hxx>

/* OCCT headers — Error handling */
#include <Standard_Failure.hxx>

/* OCCT headers — Meshing */
#include <BRepMesh_IncrementalMesh.hxx>
#include <Poly_Triangulation.hxx>
#include <BRep_Tool.hxx>
#include <TopLoc_Location.hxx>

/* OCCT headers — 2D profiles */
#include <BRepBuilderAPI_MakeEdge.hxx>
#include <BRepBuilderAPI_MakeWire.hxx>
#include <BRepBuilderAPI_MakeFace.hxx>
#include <GC_MakeCircle.hxx>
#include <GC_MakeArcOfCircle.hxx>
#include <Geom_Circle.hxx>
#include <Geom_Ellipse.hxx>
#include <gp_Circ.hxx>
#include <gp_Elips.hxx>

#include <Message_Report.hxx>
#include <Message_Alert.hxx>
#include <Message_ListOfAlert.hxx>

#include <cmath>
#include <cstdlib>
#include <cstring>
#include <vector>

/* ============================================================================
 * Internal helpers
 * ============================================================================ */

/* Wrap a TopoDS_Shape on the heap and return as opaque pointer */
static OcctShape wrap_shape(const TopoDS_Shape& shape) {
    TopoDS_Shape* p = new TopoDS_Shape(shape);
    return static_cast<OcctShape>(p);
}

/* Unwrap opaque pointer to TopoDS_Shape reference */
static TopoDS_Shape& unwrap(OcctShape s) {
    return *static_cast<TopoDS_Shape*>(s);
}

/* Build indexed maps for topology queries */
static void build_face_map(const TopoDS_Shape& shape, TopTools_IndexedMapOfShape& map) {
    TopExp::MapShapes(shape, TopAbs_FACE, map);
}

static void build_edge_map(const TopoDS_Shape& shape, TopTools_IndexedMapOfShape& map) {
    TopExp::MapShapes(shape, TopAbs_EDGE, map);
}

static void build_vertex_map(const TopoDS_Shape& shape, TopTools_IndexedMapOfShape& map) {
    TopExp::MapShapes(shape, TopAbs_VERTEX, map);
}

/* ============================================================================
 * Lifecycle
 * ============================================================================ */

extern "C" void occt_init(void) {
    /* OCCT doesn't require explicit init for basic modeling */
}

extern "C" void occt_cleanup(void) {
    /* No global state to clean up */
}

extern "C" void occt_shape_free(OcctShape s) {
    if (s) delete static_cast<TopoDS_Shape*>(s);
}

/* ============================================================================
 * Primitives
 * ============================================================================ */

extern "C" OcctShape occt_make_box(double sx, double sy, double sz,
                                   double cx, double cy, double cz) {
    /* Create box centered at origin, then translate */
    gp_Pnt corner(-sx / 2.0, -sy / 2.0, -sz / 2.0);
    BRepPrimAPI_MakeBox maker(corner, sx, sy, sz);
    TopoDS_Shape box = maker.Shape();

    if (cx != 0.0 || cy != 0.0 || cz != 0.0) {
        gp_Trsf trsf;
        trsf.SetTranslation(gp_Vec(cx, cy, cz));
        BRepBuilderAPI_Transform xform(box, trsf, Standard_True);
        box = xform.Shape();
    }

    return wrap_shape(box);
}

extern "C" OcctShape occt_make_cylinder(double radius, double height,
                                        double cx, double cy, double cz) {
    /* Cylinder along Z axis, centered vertically at (cx, cy, cz) */
    gp_Ax2 axis(gp_Pnt(cx, cy, cz - height / 2.0), gp_Dir(0, 0, 1));
    BRepPrimAPI_MakeCylinder maker(axis, radius, height);
    return wrap_shape(maker.Shape());
}

extern "C" OcctShape occt_make_sphere(double radius,
                                      double cx, double cy, double cz) {
    gp_Pnt center(cx, cy, cz);
    BRepPrimAPI_MakeSphere maker(center, radius);
    return wrap_shape(maker.Shape());
}

extern "C" OcctShape occt_make_cone(double radius1, double radius2, double height,
                                    double cx, double cy, double cz) {
    /* Cone along Z axis. OCCT MakeCone wants R1 > R2 (swap if needed) */
    gp_Ax2 axis(gp_Pnt(cx, cy, cz - height / 2.0), gp_Dir(0, 0, 1));
    BRepPrimAPI_MakeCone maker(axis, radius1, radius2, height);
    return wrap_shape(maker.Shape());
}

extern "C" OcctShape occt_make_torus(double major_r, double minor_r,
                                     double cx, double cy, double cz) {
    gp_Ax2 axis(gp_Pnt(cx, cy, cz), gp_Dir(0, 0, 1));
    BRepPrimAPI_MakeTorus maker(axis, major_r, minor_r);
    return wrap_shape(maker.Shape());
}

/* ============================================================================
 * Boolean Operations
 * ============================================================================ */

static void dump_bool_errors(const char* op, BRepAlgoAPI_BooleanOperation& algo) {
    fprintf(stderr, "[%s] IsDone=false, HasErrors=%d, HasWarnings=%d\n",
            op, algo.HasErrors() ? 1 : 0, algo.HasWarnings() ? 1 : 0);
    const Handle(Message_Report)& report = algo.GetReport();
    if (!report.IsNull()) {
        const Message_ListOfAlert& alerts = report->GetAlerts(Message_Fail);
        for (auto it = alerts.cbegin(); it != alerts.cend(); ++it) {
            fprintf(stderr, "  Alert: %s\n", (*it)->DynamicType()->Name());
        }
    }
}

extern "C" OcctShape occt_fuse(OcctShape a, OcctShape b) {
    try {
        if (!a || !b) return nullptr;
        BRepAlgoAPI_Fuse fuser(unwrap(a), unwrap(b));
        if (!fuser.IsDone()) {
            dump_bool_errors("occt_fuse", fuser);
            return nullptr;
        }
        return wrap_shape(fuser.Shape());
    } catch (Standard_Failure const& e) {
        fprintf(stderr, "[occt_fuse] %s\n", e.GetMessageString());
        return nullptr;
    } catch (...) {
        return nullptr;
    }
}

extern "C" OcctShape occt_cut(OcctShape a, OcctShape b) {
    try {
        if (!a || !b) return nullptr;
        BRepAlgoAPI_Cut cutter(unwrap(a), unwrap(b));
        if (!cutter.IsDone()) {
            dump_bool_errors("occt_cut", cutter);
            return nullptr;
        }
        return wrap_shape(cutter.Shape());
    } catch (Standard_Failure const& e) {
        fprintf(stderr, "[occt_cut] %s\n", e.GetMessageString());
        return nullptr;
    } catch (...) {
        return nullptr;
    }
}

extern "C" OcctShape occt_common(OcctShape a, OcctShape b) {
    try {
        if (!a || !b) return nullptr;
        BRepAlgoAPI_Common sectioner(unwrap(a), unwrap(b));
        if (!sectioner.IsDone()) {
            dump_bool_errors("occt_common", sectioner);
            return nullptr;
        }
        return wrap_shape(sectioner.Shape());
    } catch (Standard_Failure const& e) {
        fprintf(stderr, "[occt_common] %s\n", e.GetMessageString());
        return nullptr;
    } catch (...) {
        return nullptr;
    }
}

/* ============================================================================
 * Multi-tool Boolean Operations
 * ============================================================================ */

static TopTools_ListOfShape build_tool_list(OcctShape* tools, int count) {
    TopTools_ListOfShape list;
    for (int i = 0; i < count; i++) {
        if (tools[i]) list.Append(unwrap(tools[i]));
    }
    return list;
}

extern "C" OcctShape occt_fuse_multi(OcctShape base, OcctShape* tools, int count) {
    try {
        if (!base || count == 0) return base;
        TopTools_ListOfShape args, toolList;
        args.Append(unwrap(base));
        toolList = build_tool_list(tools, count);
        BRepAlgoAPI_Fuse fuser;
        fuser.SetArguments(args);
        fuser.SetTools(toolList);
        fuser.Build();
        if (!fuser.IsDone()) {
            dump_bool_errors("occt_fuse_multi", fuser);
            return nullptr;
        }
        return wrap_shape(fuser.Shape());
    } catch (Standard_Failure const& e) {
        fprintf(stderr, "[occt_fuse_multi] %s\n", e.GetMessageString());
        return nullptr;
    } catch (...) {
        return nullptr;
    }
}

extern "C" OcctShape occt_cut_multi(OcctShape base, OcctShape* tools, int count) {
    try {
        if (!base || count == 0) return base;
        TopTools_ListOfShape args, toolList;
        args.Append(unwrap(base));
        toolList = build_tool_list(tools, count);
        BRepAlgoAPI_Cut cutter;
        cutter.SetArguments(args);
        cutter.SetTools(toolList);
        cutter.Build();
        if (!cutter.IsDone()) {
            dump_bool_errors("occt_cut_multi", cutter);
            return nullptr;
        }
        return wrap_shape(cutter.Shape());
    } catch (Standard_Failure const& e) {
        fprintf(stderr, "[occt_cut_multi] %s\n", e.GetMessageString());
        return nullptr;
    } catch (...) {
        return nullptr;
    }
}

extern "C" OcctShape occt_common_multi(OcctShape base, OcctShape* tools, int count) {
    try {
        if (!base || count == 0) return base;
        TopTools_ListOfShape args, toolList;
        args.Append(unwrap(base));
        toolList = build_tool_list(tools, count);
        BRepAlgoAPI_Common sectioner;
        sectioner.SetArguments(args);
        sectioner.SetTools(toolList);
        sectioner.Build();
        if (!sectioner.IsDone()) {
            dump_bool_errors("occt_common_multi", sectioner);
            return nullptr;
        }
        return wrap_shape(sectioner.Shape());
    } catch (Standard_Failure const& e) {
        fprintf(stderr, "[occt_common_multi] %s\n", e.GetMessageString());
        return nullptr;
    } catch (...) {
        return nullptr;
    }
}

/* ============================================================================
 * Transforms
 * ============================================================================ */

extern "C" OcctShape occt_translate(OcctShape s, double dx, double dy, double dz) {
    gp_Trsf trsf;
    trsf.SetTranslation(gp_Vec(dx, dy, dz));
    BRepBuilderAPI_Transform xform(unwrap(s), trsf, Standard_True);
    return wrap_shape(xform.Shape());
}

extern "C" OcctShape occt_rotate(OcctShape s, double angle_deg,
                                 double cx, double cy, double cz,
                                 double ax, double ay, double az) {
    gp_Ax1 axis(gp_Pnt(cx, cy, cz), gp_Dir(ax, ay, az));
    gp_Trsf trsf;
    trsf.SetRotation(axis, angle_deg * M_PI / 180.0);
    BRepBuilderAPI_Transform xform(unwrap(s), trsf, Standard_True);
    return wrap_shape(xform.Shape());
}

extern "C" OcctShape occt_scale(OcctShape s, double factor,
                                double cx, double cy, double cz) {
    gp_Trsf trsf;
    trsf.SetScale(gp_Pnt(cx, cy, cz), factor);
    BRepBuilderAPI_Transform xform(unwrap(s), trsf, Standard_True);
    return wrap_shape(xform.Shape());
}

extern "C" OcctShape occt_mirror(OcctShape s,
                                 double nx, double ny, double nz,
                                 double ox, double oy, double oz) {
    gp_Ax2 mirror_plane(gp_Pnt(ox, oy, oz), gp_Dir(nx, ny, nz));
    gp_Trsf trsf;
    trsf.SetMirror(mirror_plane);
    BRepBuilderAPI_Transform xform(unwrap(s), trsf, Standard_True);
    return wrap_shape(xform.Shape());
}

/* ============================================================================
 * Feature Operations
 * ============================================================================ */

extern "C" OcctShape occt_fillet(OcctShape s, int* edge_indices, int n_edges, double radius) {
    const TopoDS_Shape& shape = unwrap(s);
    TopTools_IndexedMapOfShape edgeMap;
    build_edge_map(shape, edgeMap);

    /* Pre-filter: skip edges shorter than 2*radius (fillet can't fit) */
    int* validEdges = (int*)malloc(n_edges * sizeof(int));
    int n_valid_edges = 0;
    for (int i = 0; i < n_edges; i++) {
        int idx = edge_indices[i] + 1; /* OCCT maps are 1-based */
        if (idx < 1 || idx > edgeMap.Extent()) continue;
        try {
            GProp_GProps props;
            BRepGProp::LinearProperties(TopoDS::Edge(edgeMap(idx)), props);
            if (props.Mass() <= 2.0 * radius + 0.01) continue; /* edge too short for this radius */
        } catch (...) { continue; }
        validEdges[n_valid_edges++] = idx;
    }

    if (n_valid_edges == 0) { free(validEdges); return s; /* return original = no change */ }

    /* Try batch fillet with pre-filtered edges — fastest path */
    BRepFilletAPI_MakeFillet fillet(shape);
    for (int i = 0; i < n_valid_edges; i++) {
        fillet.Add(radius, TopoDS::Edge(edgeMap(validEdges[i])));
    }
    free(validEdges);

    try {
        fillet.Build();
        if (fillet.IsDone()) return wrap_shape(fillet.Shape());
    } catch (...) {}

    /* Pre-compute midpoints of all selected edges in the original shape */
    struct EdgeMid { double x, y, z; };
    EdgeMid* mids = (EdgeMid*)malloc(n_edges * sizeof(EdgeMid));
    int n_valid = 0;

    for (int i = 0; i < n_edges; i++) {
        int idx = edge_indices[i] + 1;
        if (idx < 1 || idx > edgeMap.Extent()) continue;
        const TopoDS_Edge& edge = TopoDS::Edge(edgeMap(idx));
        BRepAdaptor_Curve curve(edge);
        double t = (curve.FirstParameter() + curve.LastParameter()) / 2.0;
        gp_Pnt mid = curve.Value(t);
        mids[n_valid].x = mid.X();
        mids[n_valid].y = mid.Y();
        mids[n_valid].z = mid.Z();
        n_valid++;
    }

    TopoDS_Shape current = shape;
    int applied = 0;
    int skipped = 0;

    for (int vi = 0; vi < n_valid; vi++) {
        /* Find the edge in the current shape closest to saved midpoint */
        TopTools_IndexedMapOfShape curEdgeMap;
        build_edge_map(current, curEdgeMap);

        double bestDist = 1e30;
        int bestIdx = -1;
        for (int j = 1; j <= curEdgeMap.Extent(); j++) {
            const TopoDS_Edge& e = TopoDS::Edge(curEdgeMap(j));
            try {
                BRepAdaptor_Curve c(e);
                double t = (c.FirstParameter() + c.LastParameter()) / 2.0;
                gp_Pnt p = c.Value(t);
                double dx = p.X() - mids[vi].x;
                double dy = p.Y() - mids[vi].y;
                double dz = p.Z() - mids[vi].z;
                double dist = dx*dx + dy*dy + dz*dz;
                if (dist < bestDist) { bestDist = dist; bestIdx = j; }
            } catch (...) {}
        }

        if (bestIdx < 0 || bestDist > 1.0) { skipped++; continue; }

        BRepFilletAPI_MakeFillet single(current);
        single.Add(radius, TopoDS::Edge(curEdgeMap(bestIdx)));

        try {
            single.Build();
            if (single.IsDone()) {
                current = single.Shape();
                applied++;
            } else {
                skipped++;
            }
        } catch (...) {
            skipped++;
        }
    }

    free(mids);

    if (skipped > 0) {
        fprintf(stderr, "clay fillet: per-edge fallback applied %d/%d edges (R=%.2f, %d skipped)\n",
                applied, n_valid, radius, skipped);
    }

    if (applied == 0) return nullptr;
    return wrap_shape(current);
}

extern "C" OcctShape occt_chamfer(OcctShape s, int* edge_indices, int n_edges, double distance) {
    const TopoDS_Shape& shape = unwrap(s);
    TopTools_IndexedMapOfShape edgeMap;
    build_edge_map(shape, edgeMap);

    /* Chamfer also needs the face-edge map */
    TopTools_IndexedDataMapOfShapeListOfShape edgeFaceMap;
    TopExp::MapShapesAndAncestors(shape, TopAbs_EDGE, TopAbs_FACE, edgeFaceMap);

    BRepFilletAPI_MakeChamfer chamfer(shape);
    for (int i = 0; i < n_edges; i++) {
        int idx = edge_indices[i] + 1;
        if (idx >= 1 && idx <= edgeMap.Extent()) {
            const TopoDS_Edge& edge = TopoDS::Edge(edgeMap(idx));
            /* Find adjacent face for chamfer */
            if (edgeFaceMap.Contains(edge)) {
                const TopTools_ListOfShape& faces = edgeFaceMap.FindFromKey(edge);
                if (!faces.IsEmpty()) {
                    chamfer.Add(distance, distance, edge, TopoDS::Face(faces.First()));
                }
            }
        }
    }

    try {
        chamfer.Build();
        if (!chamfer.IsDone()) return nullptr;
        return wrap_shape(chamfer.Shape());
    } catch (...) {
        return nullptr;
    }
}

extern "C" OcctShape occt_shell(OcctShape s, int* face_indices, int n_faces, double thickness) {
    const TopoDS_Shape& shape = unwrap(s);
    TopTools_IndexedMapOfShape faceMap;
    build_face_map(shape, faceMap);

    TopTools_ListOfShape facesToRemove;
    for (int i = 0; i < n_faces; i++) {
        int idx = face_indices[i] + 1;
        if (idx >= 1 && idx <= faceMap.Extent()) {
            facesToRemove.Append(faceMap(idx));
        }
    }

    BRepOffsetAPI_MakeThickSolid hollower;
    hollower.MakeThickSolidByJoin(shape, facesToRemove, thickness, 1e-3);

    try {
        hollower.Build();
        if (!hollower.IsDone()) return nullptr;
        return wrap_shape(hollower.Shape());
    } catch (...) {
        return nullptr;
    }
}

extern "C" OcctShape occt_extrude(OcctShape face_shape, double dx, double dy, double dz) {
    gp_Vec direction(dx, dy, dz);
    BRepPrimAPI_MakePrism prism(unwrap(face_shape), direction);
    if (!prism.IsDone()) return nullptr;
    return wrap_shape(prism.Shape());
}

extern "C" OcctShape occt_revolve(OcctShape face_shape,
                                  double ax_ox, double ax_oy, double ax_oz,
                                  double ax_dx, double ax_dy, double ax_dz,
                                  double angle_deg) {
    gp_Ax1 axis(gp_Pnt(ax_ox, ax_oy, ax_oz), gp_Dir(ax_dx, ax_dy, ax_dz));
    double angle_rad = angle_deg * M_PI / 180.0;
    BRepPrimAPI_MakeRevol revol(unwrap(face_shape), axis, angle_rad);
    if (!revol.IsDone()) return nullptr;
    return wrap_shape(revol.Shape());
}

/* ============================================================================
 * Topology Queries
 * ============================================================================ */

extern "C" int occt_count_faces(OcctShape s) {
    TopTools_IndexedMapOfShape map;
    build_face_map(unwrap(s), map);
    return map.Extent();
}

extern "C" int occt_count_edges(OcctShape s) {
    TopTools_IndexedMapOfShape map;
    build_edge_map(unwrap(s), map);
    return map.Extent();
}

extern "C" int occt_count_vertices(OcctShape s) {
    TopTools_IndexedMapOfShape map;
    build_vertex_map(unwrap(s), map);
    return map.Extent();
}

extern "C" int occt_face_normal(OcctShape s, int face_idx,
                                double* nx, double* ny, double* nz) {
    TopTools_IndexedMapOfShape map;
    build_face_map(unwrap(s), map);
    int idx = face_idx + 1;
    if (idx < 1 || idx > map.Extent()) return -1;

    const TopoDS_Face& face = TopoDS::Face(map(idx));
    Handle(Geom_Surface) surf = BRep_Tool::Surface(face);
    if (surf.IsNull()) return -1;

    /* Get UV bounds and evaluate at center */
    double u1, u2, v1, v2;
    BRepTools::UVBounds(face, u1, u2, v1, v2);
    double u_mid = (u1 + u2) / 2.0;
    double v_mid = (v1 + v2) / 2.0;

    GeomLProp_SLProps props(surf, u_mid, v_mid, 1, 1e-6);
    if (!props.IsNormalDefined()) return -1;

    gp_Dir normal = props.Normal();
    /* Reverse if face orientation is reversed */
    if (face.Orientation() == TopAbs_REVERSED) normal.Reverse();

    *nx = normal.X();
    *ny = normal.Y();
    *nz = normal.Z();
    return 0;
}

extern "C" int occt_face_center(OcctShape s, int face_idx,
                                double* cx, double* cy, double* cz) {
    TopTools_IndexedMapOfShape map;
    build_face_map(unwrap(s), map);
    int idx = face_idx + 1;
    if (idx < 1 || idx > map.Extent()) return -1;

    const TopoDS_Face& face = TopoDS::Face(map(idx));

    /* Use mass properties to find centroid */
    GProp_GProps props;
    BRepGProp::SurfaceProperties(face, props);
    gp_Pnt center = props.CentreOfMass();

    *cx = center.X();
    *cy = center.Y();
    *cz = center.Z();
    return 0;
}

extern "C" int occt_face_geom_type(OcctShape s, int face_idx) {
    TopTools_IndexedMapOfShape map;
    build_face_map(unwrap(s), map);
    int idx = face_idx + 1;
    if (idx < 1 || idx > map.Extent()) return -1;

    const TopoDS_Face& face = TopoDS::Face(map(idx));
    Handle(Geom_Surface) surf = BRep_Tool::Surface(face);
    if (surf.IsNull()) return -1;

    if (!Handle(Geom_Plane)::DownCast(surf).IsNull()) return 0;
    if (!Handle(Geom_CylindricalSurface)::DownCast(surf).IsNull()) return 1;
    if (!Handle(Geom_SphericalSurface)::DownCast(surf).IsNull()) return 2;
    if (!Handle(Geom_ConicalSurface)::DownCast(surf).IsNull()) return 3;
    if (!Handle(Geom_ToroidalSurface)::DownCast(surf).IsNull()) return 4;
    return -1; /* other */
}

extern "C" int occt_edge_tangent_at(OcctShape s, int edge_idx, double t,
                                    double* tx, double* ty, double* tz) {
    TopTools_IndexedMapOfShape map;
    build_edge_map(unwrap(s), map);
    int idx = edge_idx + 1;
    if (idx < 1 || idx > map.Extent()) return -1;

    const TopoDS_Edge& edge = TopoDS::Edge(map(idx));
    double first, last;
    Handle(Geom_Curve) curve = BRep_Tool::Curve(edge, first, last);
    if (curve.IsNull()) return -1;

    double param = first + t * (last - first);
    gp_Pnt p;
    gp_Vec tangent;
    curve->D1(param, p, tangent);

    if (tangent.Magnitude() > 1e-12) tangent.Normalize();

    *tx = tangent.X();
    *ty = tangent.Y();
    *tz = tangent.Z();
    return 0;
}

extern "C" int occt_edge_point_at(OcctShape s, int edge_idx, double t,
                                  double* px, double* py, double* pz) {
    TopTools_IndexedMapOfShape map;
    build_edge_map(unwrap(s), map);
    int idx = edge_idx + 1;
    if (idx < 1 || idx > map.Extent()) return -1;

    const TopoDS_Edge& edge = TopoDS::Edge(map(idx));
    double first, last;
    Handle(Geom_Curve) curve = BRep_Tool::Curve(edge, first, last);
    if (curve.IsNull()) return -1;

    double param = first + t * (last - first);
    gp_Pnt p = curve->Value(param);

    *px = p.X();
    *py = p.Y();
    *pz = p.Z();
    return 0;
}

extern "C" int occt_vertex_position(OcctShape s, int vert_idx,
                                    double* px, double* py, double* pz) {
    TopTools_IndexedMapOfShape map;
    build_vertex_map(unwrap(s), map);
    int idx = vert_idx + 1;
    if (idx < 1 || idx > map.Extent()) return -1;

    const TopoDS_Vertex& vertex = TopoDS::Vertex(map(idx));
    gp_Pnt p = BRep_Tool::Pnt(vertex);

    *px = p.X();
    *py = p.Y();
    *pz = p.Z();
    return 0;
}

extern "C" int occt_edge_convexity(OcctShape s, int edge_idx) {
    const TopoDS_Shape& shape = unwrap(s);
    TopTools_IndexedMapOfShape edgeMap;
    build_edge_map(shape, edgeMap);

    int idx = edge_idx + 1;
    if (idx < 1 || idx > edgeMap.Extent()) return 0;
    const TopoDS_Edge& edge = TopoDS::Edge(edgeMap(idx));

    /* Get the two faces sharing this edge */
    TopTools_IndexedDataMapOfShapeListOfShape efMap;
    TopExp::MapShapesAndAncestors(shape, TopAbs_EDGE, TopAbs_FACE, efMap);
    if (!efMap.Contains(edge)) return 0;

    const TopTools_ListOfShape& faces = efMap.FindFromKey(edge);
    if (faces.Extent() < 2) return 0; /* boundary edge */

    TopTools_ListIteratorOfListOfShape it(faces);
    const TopoDS_Face& face1 = TopoDS::Face(it.Value()); it.Next();
    const TopoDS_Face& face2 = TopoDS::Face(it.Value());
    if (face1.IsSame(face2)) return 0; /* seam edge */

    /* Get 3D edge curve and tangent at midpoint */
    double eFirst, eLast;
    Handle(Geom_Curve) curve = BRep_Tool::Curve(edge, eFirst, eLast);
    if (curve.IsNull()) return 0;
    double eMid = (eFirst + eLast) / 2.0;
    gp_Pnt edgePt;
    gp_Vec edgeTan;
    curve->D1(eMid, edgePt, edgeTan);
    if (edgeTan.Magnitude() < 1e-10) return 0;
    edgeTan.Normalize();

    /* Compute the "inward" direction on each face at the edge midpoint.
     * Uses the edge's 2D parametric curve on each face to find the direction
     * perpendicular to the edge that points INTO the face interior (toward material).
     * This is exact for all surface types (planar, cylindrical, etc.). */
    auto computeInwardDir = [&](const TopoDS_Face& face) -> gp_Vec {
        /* Get 2D parametric curve of the edge on this face */
        double f2d, l2d;
        Handle(Geom2d_Curve) c2d = BRep_Tool::CurveOnSurface(edge, face, f2d, l2d);
        if (c2d.IsNull()) return gp_Vec(0, 0, 0);

        double mid2d = (f2d + l2d) / 2.0;
        gp_Pnt2d uv;
        gp_Vec2d tan2d;
        c2d->D1(mid2d, uv, tan2d);
        if (tan2d.Magnitude() < 1e-10) return gp_Vec(0, 0, 0);
        tan2d.Normalize();

        /* Determine edge orientation on this face by finding it in the face's topology */
        bool edgeReversed = false;
        for (TopExp_Explorer exp(face, TopAbs_EDGE); exp.More(); exp.Next()) {
            if (exp.Current().IsSame(edge)) {
                edgeReversed = (exp.Current().Orientation() == TopAbs_REVERSED);
                break;
            }
        }

        /* Boundary traversal direction: flip tangent if edge is reversed on this face */
        gp_Vec2d bTan = edgeReversed ? gp_Vec2d(-tan2d.X(), -tan2d.Y()) : tan2d;

        /* Inward direction in 2D (perpendicular to boundary, pointing into face interior).
         * For FORWARD face: interior is LEFT of boundary → rotate 90° CCW = (-y, x)
         * For REVERSED face: boundary traversal effectively reverses → rotate 90° CW = (y, -x) */
        gp_Vec2d inward2d;
        if (face.Orientation() != TopAbs_REVERSED) {
            inward2d = gp_Vec2d(-bTan.Y(), bTan.X());
        } else {
            inward2d = gp_Vec2d(bTan.Y(), -bTan.X());
        }

        /* Map 2D inward direction to 3D using surface partial derivatives */
        Handle(Geom_Surface) surf = BRep_Tool::Surface(face);
        if (surf.IsNull()) return gp_Vec(0, 0, 0);
        gp_Pnt p;
        gp_Vec du, dv;
        surf->D1(uv.X(), uv.Y(), p, du, dv);

        return du * inward2d.X() + dv * inward2d.Y();
    };

    gp_Vec in1 = computeInwardDir(face1);
    gp_Vec in2 = computeInwardDir(face2);

    if (in1.Magnitude() < 1e-10 || in2.Magnitude() < 1e-10) return 0;
    in1.Normalize();
    in2.Normalize();

    /* Classify using the signed dihedral angle:
     * (inward1 × inward2) · edgeTangent encodes the dihedral angle sign.
     * Negative → convex (outside corner, box edges)
     * Positive → concave (inside corner, hole edges) */
    gp_Vec cross = in1.Crossed(in2);
    double det = cross.Dot(edgeTan);

    if (det < -1e-6) return 1;   /* convex */
    if (det > 1e-6) return -1;   /* concave */
    return 0;                     /* flat / tangent */
}

extern "C" int occt_faces_of_edge(OcctShape s, int edge_idx,
                                  int* out_face_indices, int max_faces) {
    const TopoDS_Shape& shape = unwrap(s);
    TopTools_IndexedMapOfShape edgeMap, faceMap;
    build_edge_map(shape, edgeMap);
    build_face_map(shape, faceMap);

    int idx = edge_idx + 1;
    if (idx < 1 || idx > edgeMap.Extent()) return 0;

    TopTools_IndexedDataMapOfShapeListOfShape edgeFaceMap;
    TopExp::MapShapesAndAncestors(shape, TopAbs_EDGE, TopAbs_FACE, edgeFaceMap);

    const TopoDS_Edge& edge = TopoDS::Edge(edgeMap(idx));
    if (!edgeFaceMap.Contains(edge)) return 0;

    const TopTools_ListOfShape& faces = edgeFaceMap.FindFromKey(edge);
    int count = 0;
    for (auto it = faces.cbegin(); it != faces.cend() && count < max_faces; ++it) {
        int fi = faceMap.FindIndex(*it);
        if (fi > 0) {
            out_face_indices[count++] = fi - 1; /* convert to 0-based */
        }
    }
    return count;
}

extern "C" int occt_edges_of_face(OcctShape s, int face_idx,
                                  int* out_edge_indices, int max_edges) {
    const TopoDS_Shape& shape = unwrap(s);
    TopTools_IndexedMapOfShape faceMap, edgeMap;
    build_face_map(shape, faceMap);
    build_edge_map(shape, edgeMap);

    int idx = face_idx + 1;
    if (idx < 1 || idx > faceMap.Extent()) return 0;

    const TopoDS_Face& face = TopoDS::Face(faceMap(idx));
    int count = 0;
    for (TopExp_Explorer exp(face, TopAbs_EDGE); exp.More() && count < max_edges; exp.Next()) {
        int ei = edgeMap.FindIndex(exp.Current());
        if (ei > 0) {
            out_edge_indices[count++] = ei - 1; /* convert to 0-based */
        }
    }
    return count;
}

/* Check if an edge is a seam (same face on both sides — parametric surface wrap) */
extern "C" int occt_is_seam_edge(OcctShape s, int edge_idx) {
    int face_indices[2];
    int nf = occt_faces_of_edge(s, edge_idx, face_indices, 2);
    if (nf < 2) return 0;
    return face_indices[0] == face_indices[1] ? 1 : 0;
}

/* ============================================================================
 * 2D Profile Construction
 * ============================================================================ */

extern "C" OcctShape occt_profile_rect(double width, double height, double cx, double cy) {
    double hw = width / 2.0, hh = height / 2.0;
    gp_Pnt p1(cx - hw, cy - hh, 0);
    gp_Pnt p2(cx + hw, cy - hh, 0);
    gp_Pnt p3(cx + hw, cy + hh, 0);
    gp_Pnt p4(cx - hw, cy + hh, 0);

    TopoDS_Edge e1 = BRepBuilderAPI_MakeEdge(p1, p2);
    TopoDS_Edge e2 = BRepBuilderAPI_MakeEdge(p2, p3);
    TopoDS_Edge e3 = BRepBuilderAPI_MakeEdge(p3, p4);
    TopoDS_Edge e4 = BRepBuilderAPI_MakeEdge(p4, p1);

    BRepBuilderAPI_MakeWire wire;
    wire.Add(e1); wire.Add(e2); wire.Add(e3); wire.Add(e4);

    BRepBuilderAPI_MakeFace face(wire.Wire());
    return wrap_shape(face.Shape());
}

extern "C" OcctShape occt_profile_circle(double radius, double cx, double cy) {
    gp_Ax2 axis(gp_Pnt(cx, cy, 0), gp_Dir(0, 0, 1));
    gp_Circ circ(axis, radius);
    TopoDS_Edge edge = BRepBuilderAPI_MakeEdge(circ);
    TopoDS_Wire wire = BRepBuilderAPI_MakeWire(edge);
    BRepBuilderAPI_MakeFace face(wire);
    return wrap_shape(face.Shape());
}

extern "C" OcctShape occt_profile_ellipse(double rx, double ry, double cx, double cy) {
    gp_Ax2 axis(gp_Pnt(cx, cy, 0), gp_Dir(0, 0, 1));
    /* gp_Elips requires major >= minor. Swap if needed. */
    double major = rx >= ry ? rx : ry;
    double minor = rx >= ry ? ry : rx;
    gp_Elips elips(axis, major, minor);
    TopoDS_Edge edge = BRepBuilderAPI_MakeEdge(elips);
    TopoDS_Wire wire = BRepBuilderAPI_MakeWire(edge);
    BRepBuilderAPI_MakeFace face(wire);
    return wrap_shape(face.Shape());
}

extern "C" OcctShape occt_profile_polygon(double* points, int n_points) {
    if (n_points < 3) return nullptr;

    BRepBuilderAPI_MakeWire wire;
    for (int i = 0; i < n_points; i++) {
        int j = (i + 1) % n_points;
        gp_Pnt p1(points[i * 2], points[i * 2 + 1], 0);
        gp_Pnt p2(points[j * 2], points[j * 2 + 1], 0);
        wire.Add(BRepBuilderAPI_MakeEdge(p1, p2));
    }

    BRepBuilderAPI_MakeFace face(wire.Wire());
    return wrap_shape(face.Shape());
}

extern "C" OcctShape occt_profile_slot(double sx, double sy, double ex, double ey, double width) {
    /* Slot = rectangle with semicircular ends (stadium shape) */
    double dx = ex - sx, dy = ey - sy;
    double len = std::sqrt(dx * dx + dy * dy);
    if (len < 1e-12) return nullptr;

    double r = width / 2.0;
    /* Normal perpendicular to slot axis */
    double nx = -dy / len * r;
    double ny = dx / len * r;

    gp_Pnt p1(sx + nx, sy + ny, 0);
    gp_Pnt p2(ex + nx, ey + ny, 0);
    gp_Pnt p3(ex - nx, ey - ny, 0);
    gp_Pnt p4(sx - nx, sy - ny, 0);

    /* Two straight edges + two semicircular arcs */
    TopoDS_Edge e_top = BRepBuilderAPI_MakeEdge(p1, p2);
    TopoDS_Edge e_bottom = BRepBuilderAPI_MakeEdge(p3, p4);

    /* Arc at end (ex, ey): from p2 to p3 around (ex, ey) */
    gp_Ax2 ax_end(gp_Pnt(ex, ey, 0), gp_Dir(0, 0, 1));
    gp_Circ circ_end(ax_end, r);
    Handle(Geom_Circle) gc_end = new Geom_Circle(circ_end);
    TopoDS_Edge e_arc_end = BRepBuilderAPI_MakeEdge(gc_end, p2, p3);

    /* Arc at start (sx, sy): from p4 to p1 around (sx, sy) */
    gp_Ax2 ax_start(gp_Pnt(sx, sy, 0), gp_Dir(0, 0, 1));
    gp_Circ circ_start(ax_start, r);
    Handle(Geom_Circle) gc_start = new Geom_Circle(circ_start);
    TopoDS_Edge e_arc_start = BRepBuilderAPI_MakeEdge(gc_start, p4, p1);

    BRepBuilderAPI_MakeWire wire;
    wire.Add(e_top);
    wire.Add(e_arc_end);
    wire.Add(e_bottom);
    wire.Add(e_arc_start);

    BRepBuilderAPI_MakeFace face(wire.Wire());
    return wrap_shape(face.Shape());
}

extern "C" OcctShape occt_wire_to_face(OcctShape wire_shape) {
    /* If it's already a face, return it */
    const TopoDS_Shape& shape = unwrap(wire_shape);
    if (shape.ShapeType() == TopAbs_FACE) return wrap_shape(shape);

    /* Extract first wire and make face */
    TopoDS_Wire wire;
    for (TopExp_Explorer exp(shape, TopAbs_WIRE); exp.More(); exp.Next()) {
        wire = TopoDS::Wire(exp.Current());
        break;
    }
    if (wire.IsNull()) return nullptr;

    BRepBuilderAPI_MakeFace face(wire);
    if (!face.IsDone()) return nullptr;
    return wrap_shape(face.Shape());
}

extern "C" OcctShape occt_profile_fuse(OcctShape a, OcctShape b) {
    return occt_fuse(a, b);
}

extern "C" OcctShape occt_profile_cut(OcctShape a, OcctShape b) {
    return occt_cut(a, b);
}

extern "C" OcctShape occt_profile_common(OcctShape a, OcctShape b) {
    return occt_common(a, b);
}

/* ============================================================================
 * Meshing
 * ============================================================================ */

extern "C" OcctMeshResult occt_mesh(OcctShape s, double linear_tol, double angular_tol_deg) {
    OcctMeshResult result = {};

    const TopoDS_Shape& shape = unwrap(s);

    /* Tessellate */
    double angular_tol_rad = angular_tol_deg * M_PI / 180.0;
    if (angular_tol_rad < 0.01) angular_tol_rad = 0.5; /* default ~30 deg */
    BRepMesh_IncrementalMesh mesher(shape, linear_tol, Standard_False, angular_tol_rad);
    mesher.Perform();

    /* Count total vertices and triangles across all faces */
    int total_verts = 0;
    int total_tris = 0;

    TopTools_IndexedMapOfShape faceMap;
    build_face_map(shape, faceMap);

    /* First pass: count */
    for (int fi = 1; fi <= faceMap.Extent(); fi++) {
        const TopoDS_Face& face = TopoDS::Face(faceMap(fi));
        TopLoc_Location loc;
        Handle(Poly_Triangulation) tri = BRep_Tool::Triangulation(face, loc);
        if (tri.IsNull()) continue;
        total_verts += tri->NbNodes();
        total_tris += tri->NbTriangles();
    }

    /* Allocate output */
    result.vertices = (float*)malloc(total_verts * 3 * sizeof(float));
    result.triangles = (int*)malloc(total_tris * 3 * sizeof(int));
    result.normals = (float*)malloc(total_verts * 3 * sizeof(float));
    result.n_verts = total_verts;
    result.n_tris = total_tris;

    /* Second pass: fill */
    int vert_offset = 0;
    int tri_offset = 0;

    for (int fi = 1; fi <= faceMap.Extent(); fi++) {
        const TopoDS_Face& face = TopoDS::Face(faceMap(fi));
        TopLoc_Location loc;
        Handle(Poly_Triangulation) tri = BRep_Tool::Triangulation(face, loc);
        if (tri.IsNull()) continue;

        gp_Trsf trsf = loc.Transformation();
        bool reversed = (face.Orientation() == TopAbs_REVERSED);

        int nv = tri->NbNodes();
        int nt = tri->NbTriangles();

        /* Copy vertices (transformed) */
        for (int vi = 1; vi <= nv; vi++) {
            gp_Pnt p = tri->Node(vi).Transformed(trsf);
            int base = (vert_offset + vi - 1) * 3;
            result.vertices[base + 0] = (float)p.X();
            result.vertices[base + 1] = (float)p.Y();
            result.vertices[base + 2] = (float)p.Z();
        }

        /* Copy normals if available, otherwise compute from triangles */
        if (tri->HasNormals()) {
            for (int vi = 1; vi <= nv; vi++) {
                gp_Dir n = tri->Normal(vi);
                n = n.Transformed(trsf);
                if (reversed) n.Reverse();
                int base = (vert_offset + vi - 1) * 3;
                result.normals[base + 0] = (float)n.X();
                result.normals[base + 1] = (float)n.Y();
                result.normals[base + 2] = (float)n.Z();
            }
        } else {
            /* OCCT didn't generate normals — compute from triangle geometry.
             * Accumulate cross-product normals at each vertex, then normalize.
             * Since vertices are per-face (not shared between faces), this gives
             * correct per-face normals that maintain sharp edges at face boundaries. */
            memset(result.normals + vert_offset * 3, 0, nv * 3 * sizeof(float));

            for (int ti = 1; ti <= nt; ti++) {
                int i1, i2, i3;
                tri->Triangle(ti).Get(i1, i2, i3);
                gp_Pnt p1 = tri->Node(i1).Transformed(trsf);
                gp_Pnt p2 = tri->Node(i2).Transformed(trsf);
                gp_Pnt p3 = tri->Node(i3).Transformed(trsf);

                /* Cross product (p2-p1) x (p3-p1) */
                double ex1 = p2.X()-p1.X(), ey1 = p2.Y()-p1.Y(), ez1 = p2.Z()-p1.Z();
                double ex2 = p3.X()-p1.X(), ey2 = p3.Y()-p1.Y(), ez2 = p3.Z()-p1.Z();
                float nx = (float)(ey1*ez2 - ez1*ey2);
                float ny = (float)(ez1*ex2 - ex1*ez2);
                float nz = (float)(ex1*ey2 - ey1*ex2);

                /* Accumulate at each vertex of this triangle */
                int verts[3] = { i1, i2, i3 };
                for (int k = 0; k < 3; k++) {
                    int base = (vert_offset + verts[k] - 1) * 3;
                    result.normals[base + 0] += nx;
                    result.normals[base + 1] += ny;
                    result.normals[base + 2] += nz;
                }
            }

            /* Normalize and flip for reversed faces */
            for (int vi = 1; vi <= nv; vi++) {
                int base = (vert_offset + vi - 1) * 3;
                float len = sqrtf(result.normals[base]*result.normals[base] +
                                  result.normals[base+1]*result.normals[base+1] +
                                  result.normals[base+2]*result.normals[base+2]);
                if (len > 1e-8f) {
                    result.normals[base+0] /= len;
                    result.normals[base+1] /= len;
                    result.normals[base+2] /= len;
                }
                if (reversed) {
                    result.normals[base+0] = -result.normals[base+0];
                    result.normals[base+1] = -result.normals[base+1];
                    result.normals[base+2] = -result.normals[base+2];
                }
            }
        }

        /* Copy triangles (with vertex offset) */
        for (int ti = 1; ti <= nt; ti++) {
            int n1, n2, n3;
            tri->Triangle(ti).Get(n1, n2, n3);
            /* OCCT triangles are 1-based */
            int base = (tri_offset + ti - 1) * 3;
            if (reversed) {
                result.triangles[base + 0] = vert_offset + n1 - 1;
                result.triangles[base + 1] = vert_offset + n3 - 1;
                result.triangles[base + 2] = vert_offset + n2 - 1;
            } else {
                result.triangles[base + 0] = vert_offset + n1 - 1;
                result.triangles[base + 1] = vert_offset + n2 - 1;
                result.triangles[base + 2] = vert_offset + n3 - 1;
            }
        }

        vert_offset += nv;
        tri_offset += nt;
    }

    /* Edge extraction: collect all edge polylines, excluding seam edges */
    TopTools_IndexedMapOfShape edgeMap;
    build_edge_map(shape, edgeMap);

    /* Build edge-face adjacency map to detect seams */
    TopTools_IndexedDataMapOfShapeListOfShape edgeFaceAdj;
    TopExp::MapShapesAndAncestors(shape, TopAbs_EDGE, TopAbs_FACE, edgeFaceAdj);

    std::vector<float> edgePts;
    for (int ei = 1; ei <= edgeMap.Extent(); ei++) {
        const TopoDS_Edge& edge = TopoDS::Edge(edgeMap(ei));

        /* Skip seam edges: both adjacent faces are the same */
        if (edgeFaceAdj.Contains(edge)) {
            const TopTools_ListOfShape& faces = edgeFaceAdj.FindFromKey(edge);
            if (faces.Extent() == 2) {
                TopTools_ListIteratorOfListOfShape it(faces);
                const TopoDS_Shape& f1 = it.Value(); it.Next();
                const TopoDS_Shape& f2 = it.Value();
                if (f1.IsSame(f2)) continue;
            }
        }
        TopLoc_Location loc;
        Handle(Poly_PolygonOnTriangulation) polyOnTri;
        Handle(Poly_Triangulation) tri;

        /* Try to get polygon on triangulation */
        BRep_Tool::PolygonOnTriangulation(edge, polyOnTri, tri, loc);
        if (!polyOnTri.IsNull() && !tri.IsNull()) {
            gp_Trsf trsf = loc.Transformation();
            const TColStd_Array1OfInteger& nodes = polyOnTri->Nodes();
            for (int i = nodes.Lower(); i < nodes.Upper(); i++) {
                gp_Pnt p1 = tri->Node(nodes(i)).Transformed(trsf);
                gp_Pnt p2 = tri->Node(nodes(i + 1)).Transformed(trsf);
                edgePts.push_back((float)p1.X());
                edgePts.push_back((float)p1.Y());
                edgePts.push_back((float)p1.Z());
                edgePts.push_back((float)p2.X());
                edgePts.push_back((float)p2.Y());
                edgePts.push_back((float)p2.Z());
            }
        }
    }

    if (!edgePts.empty()) {
        result.n_edge_pts = (int)edgePts.size() / 3;
        result.edge_lines = (float*)malloc(edgePts.size() * sizeof(float));
        memcpy(result.edge_lines, edgePts.data(), edgePts.size() * sizeof(float));
    } else {
        result.n_edge_pts = 0;
        result.edge_lines = nullptr;
    }

    return result;
}

extern "C" void occt_mesh_free(OcctMeshResult* m) {
    if (!m) return;
    free(m->vertices);
    free(m->triangles);
    free(m->normals);
    free(m->edge_lines);
    m->vertices = nullptr;
    m->triangles = nullptr;
    m->normals = nullptr;
    m->edge_lines = nullptr;
}

#ifndef CONVERT_H
#define CONVERT_H

#include <osl/osl.h>
#include <osl/relation.h>

#include <isl/ctx.h>
#include <isl/space.h>
#include <isl/constraint.h>
#include <isl/set.h>
#include <isl/map.h>
#include <isl/union_set.h>
#include <isl/union_map.h>

__isl_give isl_union_map* osl_relation_to_isl_union_map(isl_ctx* ctx, osl_relation_p relation);
osl_relation_p isl_basic_map_to_osl_relation(__isl_take isl_basic_map* bmap);
osl_relation_p isl_union_map_to_osl_relation(__isl_take isl_union_map* umap);

#endif // CONVERT_H

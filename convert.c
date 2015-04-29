#include "convert.h"

__isl_give isl_basic_map* osl_relation_part_to_isl_basic_map(isl_ctx* ctx, osl_relation_p relation) {
  isl_space* space;
  isl_mat* eq_mat;
  isl_mat* ineq_mat;
  size_t nb_equations = 0;
  int i, j;
  int eq_row = 0, ineq_row = 0;

  space = isl_space_alloc(ctx, relation->nb_parameters, relation->nb_input_dims, relation->nb_output_dims);

  for (i = 0; i < relation->nb_rows; i++) {
    if (osl_int_zero(osl_util_get_precision(), relation->m[i][0])) {
      nb_equations++;
    }
  }

  eq_mat = isl_mat_alloc(ctx, nb_equations, relation->nb_columns - 1);
  ineq_mat = isl_mat_alloc(ctx, relation->nb_rows - nb_equations, relation->nb_columns - 1);

  for (i = 0; i < relation->nb_rows; i++) {
    int is_eq_row = osl_int_zero(osl_util_get_precision(), relation->m[i][0]);
    for (j = 1; j < relation->nb_columns; j++) {
      int value = osl_int_get_si(osl_util_get_precision(), relation->m[i][j]);
      if (is_eq_row)
        isl_mat_set_element_si(eq_mat, eq_row, j - 1, value);
      else
        isl_mat_set_element_si(ineq_mat, ineq_row, j - 1, value);
    }
    if (is_eq_row)
      eq_row++;
    else
      ineq_row++;
  }

  return isl_basic_map_from_constraint_matrices(space, eq_mat, ineq_mat, isl_dim_out, isl_dim_in, isl_dim_div, isl_dim_param, isl_dim_cst);
}

__isl_give isl_union_map* osl_relation_to_isl_union_map(isl_ctx* ctx, osl_relation_p relation) {
  isl_map *map = NULL;
  isl_union_map* umap = NULL;
  for ( ; relation != NULL; relation = relation->next) {
    isl_basic_map* bmap = osl_relation_part_to_isl_basic_map(ctx, relation);
    if (map == NULL) {
      map = isl_map_from_basic_map(bmap);
    } else {
      isl_space* space1 = isl_map_get_space(map);
      isl_space* space2 = isl_basic_map_get_space(bmap);
      if (isl_space_is_equal(space1, space2)) {
        isl_map* bmap_wrapped = isl_map_from_basic_map(bmap);
        map = isl_map_union(map, bmap_wrapped);
      } else {
        if (umap == NULL) {
          umap = isl_union_map_from_map(map);
        } else {
          umap = isl_union_map_add_map(umap, map);
        }
        map = isl_map_from_basic_map(bmap);
      }
      isl_space_free(space1);
      isl_space_free(space2);
    }
  }

  if (map != NULL) {
    if (umap == NULL) {
      umap = isl_union_map_from_map(map);
    } else {
      isl_union_map* map_wrapped = isl_union_map_from_map(map);
      umap = isl_union_map_union(umap, map_wrapped);
    }
  }
  return umap;
}

osl_relation_p isl_basic_map_to_osl_relation(__isl_take isl_basic_map* bmap) {
  int i, j, eq_mat_rows;
  isl_mat* eq_mat = isl_basic_map_equalities_matrix(bmap, isl_dim_out, isl_dim_in, isl_dim_div, isl_dim_param, isl_dim_cst);
  isl_mat* ineq_mat = isl_basic_map_inequalities_matrix(bmap, isl_dim_out, isl_dim_in, isl_dim_div, isl_dim_param, isl_dim_cst);
  isl_local_space* space = isl_basic_map_get_local_space(bmap);

  eq_mat_rows = isl_mat_rows(eq_mat);
  osl_relation_p relation = osl_relation_malloc(isl_mat_rows(eq_mat) + isl_mat_rows(ineq_mat),
                                                isl_mat_cols(eq_mat) + 1);
  relation->next = NULL;

  for (i = 0; i < isl_mat_rows(eq_mat); i++) {
    osl_int_set_si(osl_util_get_precision(), &relation->m[i][0], 0);
    for (j = 0; j < isl_mat_cols(eq_mat); j++) {
      isl_val* val = isl_mat_get_element_val(eq_mat, i, j);
      int value = isl_val_get_num_si(val) / isl_val_get_den_si(val);
      osl_int_set_si(osl_util_get_precision(), &relation->m[i][1 + j], value);
      isl_val_free(val);
    }
  }
  for (i = 0; i < isl_mat_rows(ineq_mat); i++) {
    osl_int_set_si(osl_util_get_precision(), &relation->m[i][0], 1);
    for (j = 0; j < isl_mat_cols(ineq_mat); j++) {
      isl_val* val = isl_mat_get_element_val(ineq_mat, i, j);
      int value = isl_val_get_num_si(val) / isl_val_get_den_si(val);
      osl_int_set_si(osl_util_get_precision(), &relation->m[eq_mat_rows + i][1 + j], value);
      isl_val_free(val);
    }
  }
  osl_relation_set_attributes(relation,
                              isl_local_space_dim(space, isl_dim_out),
                              isl_local_space_dim(space, isl_dim_in),
                              isl_local_space_dim(space, isl_dim_div),
                              isl_local_space_dim(space, isl_dim_param));

  isl_local_space_free(space);
  isl_mat_free(eq_mat);
  isl_mat_free(ineq_mat);
  isl_basic_map_free(bmap);
  return relation;
}

int isl_basic_map_to_osl_relation_helper(__isl_take isl_basic_map* bmap, void* usr) {
  osl_relation_p* relation_ptr = (osl_relation_p*) usr;
  osl_relation_p r = isl_basic_map_to_osl_relation(bmap);
  if (*relation_ptr == NULL) {
    *relation_ptr = r;
  } else {
    r->next = *relation_ptr;
    *relation_ptr = r;
  }
  return 1;
}

int isl_map_to_osl_relation_helper(__isl_take isl_map* map, void *usr) {
  isl_map_foreach_basic_map(map, &isl_basic_map_to_osl_relation_helper, usr);
  isl_map_free(map);
  return 1;
}

osl_relation_p isl_union_map_to_osl_relation(__isl_take isl_union_map* umap) {
  osl_relation_p relation = NULL;
  isl_union_map_foreach_map(umap, &isl_map_to_osl_relation_helper, &relation);
  isl_union_map_free(umap);
  return relation;
}

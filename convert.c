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

static int id_list_index_of(__isl_keep struct isl_id_list* id_list, __isl_keep isl_id* id) {
  int i, n = isl_id_list_n_id(id_list);
  for (i = 0; i < n; ++i) {
    isl_id *lid = isl_id_list_get_id(id_list, i);
    if (lid == id) {
      isl_id_free(lid);
      return i;
    }
    isl_id_free(lid);
  }
  return -1;
}

struct collect_accesses_data {
  osl_scop_p scop;
  osl_statement_p stmt;
  isl_id_list* id_list;
};

static isl_stat collect_accesses(__isl_take isl_map* m, void* user) {
  osl_relation_p relation;
  osl_relation_list_p node;
  isl_id* array_id;
  int array_index;
  struct collect_accesses_data* data = (struct collect_accesses_data*) user;

  array_id = isl_map_get_tuple_id(m, isl_dim_out);
  array_index = id_list_index_of(data->id_list, array_id);
  if (array_index == -1) {
    array_index = isl_id_list_n_id(data->id_list);
    data->id_list = isl_id_list_add(data->id_list, array_id);
  } else {
    isl_id_free(array_id);
  }

  relation = isl_union_map_to_osl_relation(isl_union_map_from_map(m));
  osl_relation_insert_blank_column(relation, 1);
  osl_relation_insert_blank_row(relation, 0);
  relation->nb_output_dims += 1;
  osl_int_set_si(relation->precision, &relation->m[0][1], -1);
  osl_int_set_si(relation->precision, &relation->m[0][relation->nb_columns - 1], array_index + 1);
  node = osl_relation_list_node(relation);
  osl_relation_list_add(&data->stmt->access, node);
  return isl_stat_ok;
}

struct collect_stmts_data {
  isl_union_map* schedule_map;
  isl_union_map* access;
  osl_scop_p scop;
  isl_id_list* array_ids;
};

static isl_stat collect_stmts(__isl_take isl_set* s, void* user) {
  isl_space *space;
  isl_map *domain_map;
  isl_union_map *umap;
  osl_statement_p stmt;
  struct collect_stmts_data* data = (struct collect_stmts_data*) user;
  struct collect_accesses_data access_data;
  osl_scop_p scop = data->scop;

  stmt = osl_statement_malloc();
  osl_statement_add(&scop->statement, stmt);

  space = isl_set_get_space(s);
  space = isl_space_params(space);
  domain_map = isl_map_from_domain_and_range(isl_set_empty(isl_space_copy(space)), isl_set_copy(s));
  stmt->domain = isl_union_map_to_osl_relation(isl_union_map_from_map(domain_map));

  umap = isl_union_map_intersect_domain(isl_union_map_copy(data->schedule_map),
                                        isl_union_set_from_set(isl_set_copy(s)));
  stmt->scattering = isl_union_map_to_osl_relation(umap);

  umap = isl_union_map_intersect_domain(isl_union_map_copy(data->access),
                                        isl_union_set_from_set(isl_set_copy(s)));
  umap = isl_union_map_gist(umap,
      isl_union_map_from_domain_and_range(isl_union_set_empty(space), isl_union_set_from_set(s)));

  access_data.scop = scop;
  access_data.stmt = stmt;
  access_data.id_list = data->array_ids;
  isl_union_map_foreach_map(umap, &collect_accesses, &access_data);
  isl_union_map_free(umap);
  return isl_stat_ok;
}

osl_scop_p isl_to_osl_scop(__isl_take isl_union_set* context, __isl_take isl_union_set* domain, __isl_take isl_union_map* schedule_map,
    __isl_take isl_union_map* access) {
  osl_scop_p scop;
  isl_union_set *empty_set;
  isl_union_map *umap;
  isl_space *space;
  isl_ctx* ctx;
  isl_id_list* array_ids;
  int n_arrays_estimate, i, n;
  osl_arrays_p arrays;

  ctx = isl_union_set_get_ctx(context);
  space = isl_union_set_get_space(context);
  empty_set = isl_union_set_empty(space);

  scop = osl_scop_malloc();
  umap = isl_union_map_from_domain_and_range(isl_union_set_copy(empty_set), context);
  scop->context = isl_union_map_to_osl_relation(umap);

  n_arrays_estimate = isl_union_map_n_map(access) / isl_union_set_n_set(domain) + 1;
  array_ids = isl_id_list_alloc(ctx, n_arrays_estimate);

  struct collect_stmts_data collect_data = { schedule_map, access, scop, array_ids };
  isl_union_set_foreach_set(domain, &collect_stmts, &collect_data);

  arrays = osl_arrays_malloc();
  n = isl_id_list_n_id(array_ids);
  for (i = 0; i < n; ++i) {
    isl_id* id = isl_id_list_get_id(array_ids, i);
    osl_arrays_add(arrays, i + 1, (char *) isl_id_get_name(id));
    isl_id_free(id);
  }
  isl_id_list_free(array_ids);
  osl_generic_add(&scop->extension, osl_generic_shell(arrays, osl_arrays_interface()));
  return scop;
}

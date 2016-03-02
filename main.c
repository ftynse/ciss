#include <osl/osl.h>
#include <osl/extensions/dependence.h>

#include <candl/candl.h>
#include <candl/dependence.h>

#include <isl/ctx.h>
#include <isl/set.h>
#include <isl/map.h>

#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "convert.h"
#include "graph.h"
#include "linked_list.h"
#include "path.h"

ciss_kleene_element** build_kleene(ciss_graph* graph) {
  size_t nb_nodes;
  size_t i, j, k;
  ciss_graph_node* node_i;
  ciss_graph_node* node_j;
  ciss_graph_arc* arc;
  LL_SIZE(ciss_graph_node, graph->nodes, nb_nodes);

  ciss_kleene_element** previous_step = (ciss_kleene_element**) malloc(sizeof(ciss_kleene_element*) * nb_nodes * nb_nodes);
  ciss_kleene_element** current_step = (ciss_kleene_element**) malloc(sizeof(ciss_kleene_element*) * nb_nodes * nb_nodes);

  // Initialize.
  node_i = graph->nodes;
  for (i = 0; i < nb_nodes; i++, node_i = node_i->next) {
    node_j = graph->nodes;
    for (j = 0; j < nb_nodes; j++, node_j = node_j->next) {
      ciss_kleene_element_list* list = NULL;
      for (arc = node_i->outgoing; arc != NULL; arc = arc->next) {
        if (arc->target == node_j) {
          list = ciss_kleene_element_list_append(list, ciss_kleene_element_create_single(arc));
        }
      }
      if (i == j) {
        list = ciss_kleene_element_list_append(list, ciss_kleene_element_create_epsilon(node_i));
      }
      if (list == NULL) {
        list = ciss_kleene_element_list_append(list, ciss_kleene_element_create_empty());
      }

      ciss_kleene_element* element = ciss_kleene_element_create_list(list, LIST_ALTERNATIVES);
      previous_step[i * nb_nodes + j] = element;
    }
  }

  // Main iteration.
  for (k = 0; k < nb_nodes; k++) {
    node_i = graph->nodes;
    for (i = 0; i < nb_nodes; i++, node_i = node_i->next) {
      node_j = graph->nodes;
      for (j = 0; j < nb_nodes; j++, node_j = node_j->next) {
        ciss_kleene_element* star_element = ciss_kleene_element_create_star(
              ciss_kleene_element_clone(previous_step[k * nb_nodes + k]));

        ciss_kleene_element_list* list_1 = (ciss_kleene_element_list*) malloc(sizeof(ciss_kleene_element_list));
        ciss_kleene_element_list* list_2 = (ciss_kleene_element_list*) malloc(sizeof(ciss_kleene_element_list));
        ciss_kleene_element_list* list_3 = (ciss_kleene_element_list*) malloc(sizeof(ciss_kleene_element_list));
        list_1->next = list_2;
        list_2->next = list_3;
        list_3->next = 0;
        list_1->element = ciss_kleene_element_clone(previous_step[i * nb_nodes + k]);
        list_2->element = star_element;
        list_3->element = ciss_kleene_element_clone(previous_step[k * nb_nodes + j]);

        ciss_kleene_element* seq_element = ciss_kleene_element_create_list(list_1, LIST_SEQUENCE);

        ciss_kleene_element_list* alist_1 = (ciss_kleene_element_list*) malloc(sizeof(ciss_kleene_element_list));
        ciss_kleene_element_list* alist_2 = (ciss_kleene_element_list*) malloc(sizeof(ciss_kleene_element_list));
        alist_1->next = alist_2;
        alist_1->element = seq_element;
        alist_2->element = ciss_kleene_element_clone(previous_step[i * nb_nodes + j]);
        alist_2->next = 0;

        ciss_kleene_element* element = ciss_kleene_element_create_list(alist_1, LIST_ALTERNATIVES);
        ciss_kleene_element_simplify(element);
        current_step[i * nb_nodes + j] = element;
      }
    }
    // clean all the elements of the previous step (they were already cloned wherever necessary)
    for (i = 0; i < nb_nodes; ++i) {
      for (j = 0; j < nb_nodes; ++j) {
        ciss_kleene_element_free(previous_step[i * nb_nodes + j]);
      }
    }
    memcpy(previous_step, current_step, sizeof(ciss_kleene_element*) * nb_nodes * nb_nodes);
  }

  free(current_step);
  return previous_step;
}

// isl relation processing
isl_union_map* ciss_relation_compose_list_isl(ciss_graph_path* path, isl_ctx *ctx) {
  isl_union_map* composed_umap = NULL;

  for ( ; path != NULL; path = path->next) {
    osl_dependence_p dependence = path->arc->dependence;
    isl_union_map* dependence_umap = osl_relation_to_isl_union_map(ctx, dependence->domain);
    // XXX: assuming dependence domain is not a union of anything (true with Candl not supporting unions)
    // otherwise, we would need to access target_nb_output_dims_domain for each part of the union
    // since it may be different.
    isl_map* dependence_map = isl_map_from_union_map(dependence_umap);
    dependence_map = isl_map_project_out(dependence_map,
                                         isl_dim_in,
                                         dependence->target_nb_output_dims_domain,
                                         dependence->target_nb_output_dims_access);
    dependence_map = isl_map_project_out(dependence_map,
                                         isl_dim_out,
                                         dependence->source_nb_output_dims_domain,
                                         dependence->source_nb_output_dims_access);
    dependence_umap = isl_union_map_from_map(dependence_map);

    if (composed_umap == NULL) {
      composed_umap = dependence_umap;
    } else {
      composed_umap = isl_union_map_apply_range(composed_umap, dependence_umap);
    }
  }

  return composed_umap;
}

osl_relation_p ciss_relation_compose_list(ciss_graph_path* path) {
  osl_relation_p relation;
  isl_ctx *ctx = isl_ctx_alloc();

  isl_union_map* composed_umap = ciss_relation_compose_list_isl(path, ctx);

  relation = isl_union_map_to_osl_relation(composed_umap);
  isl_ctx_free(ctx);
  return relation;
}

isl_union_map* ciss_relation_compose_kleene_recurse(isl_ctx* ctx, ciss_kleene_element* head) {
  isl_union_map* composed_umap = NULL;
  int exact;
  ciss_kleene_element_list* list_element;

  switch (head->type) {
  case SINGLE:
    composed_umap = osl_relation_to_isl_union_map(ctx, head->arc->dependence->domain);
    break;
  case LIST_SEQUENCE:
    for (list_element = head->list; list_element != NULL; list_element = list_element->next) {
      isl_union_map* recurse_map = ciss_relation_compose_kleene_recurse(ctx, list_element->element);
      if (recurse_map == NULL) {
        composed_umap = NULL;
        break;
      }
      if (composed_umap == NULL)
        composed_umap = recurse_map;
      else
        composed_umap = isl_union_map_apply_range(composed_umap, recurse_map);
    }
    break;
  case LIST_ALTERNATIVES:
    for (list_element = head->list; list_element != NULL; list_element = list_element->next) {
      isl_union_map* recurse_map= ciss_relation_compose_kleene_recurse(ctx, list_element->element);
      if (recurse_map == NULL)
        continue;
      if (composed_umap == NULL)
        composed_umap = recurse_map;
      else
        composed_umap = isl_union_map_union(composed_umap, recurse_map);
    }
    break;
  case STAR:
    composed_umap = ciss_relation_compose_kleene_recurse(ctx, head->star);
    if (composed_umap != NULL) {
      composed_umap = isl_union_map_transitive_closure(composed_umap, &exact);
      if (!exact) {
        fprintf(stderr, "[ciss] Could not construct exact transitive closure.\n");
      }
    }
    break;
  case EMPTY:
    composed_umap = NULL;
    break;
  case EPSILON:
    composed_umap = osl_relation_to_isl_union_map(ctx, head->node->domain_ptr);
    break;
  default:
    break;
  }
  return composed_umap;
}

osl_relation_p ciss_relation_compose_kleene(ciss_kleene_element* head) {
  isl_ctx *ctx = isl_ctx_alloc();
  osl_relation_p relation =
      isl_union_map_to_osl_relation(ciss_relation_compose_kleene_recurse(ctx, head));
  isl_ctx_free(ctx);
  return relation;
}

// Complex algorithms on graphs
void ciss_dfs_pu_recurse(ciss_graph_node* node,
                         ciss_graph_path* path,
                         void (*callback)(ciss_graph_path*, void*),
                         void* param) {
  ciss_graph_arc* arc;
  ciss_graph_path* newpath;
  size_t arc_repetitions;

  if (node == NULL)
    return;

  if (callback != NULL && path != NULL)
    callback(path, param);

  for (arc = node->outgoing; arc != NULL; arc = arc->next) {
    arc_repetitions = ciss_graph_path_count_arcs(path, arc);
    if (arc_repetitions == 1)
      continue;

    newpath = ciss_graph_path_clone(path);
    newpath = ciss_graph_path_append(newpath, arc);
    ciss_dfs_pu_recurse(arc->target, newpath, callback, param);
    ciss_graph_path_destroy(newpath);
  }
}

// path-unique DFS
void ciss_dfs_pu(ciss_graph_node* node,
                 void (*callback)(ciss_graph_path*, void*),
                 void* param) {
  ciss_dfs_pu_recurse(node, NULL, callback, param);
}

ciss_graph* ciss_graph_construct(osl_dependence_p dependence) {
  ciss_graph* dependence_graph = ciss_graph_create();
  for ( ; dependence != NULL; dependence = dependence->next) {
    ciss_graph_node* source = ciss_graph_ensure_node(dependence_graph, dependence->label_source);
    ciss_graph_node* target = ciss_graph_ensure_node(dependence_graph, dependence->label_target);
    ciss_graph_connect_nodes(source, target, dependence);
//    ciss_graph_arc* arc = ciss_graph_arc_create(source, target, dependence);
//    ciss_graph_node_append_arc(source, arc);
  }
  return dependence_graph;
}

void ciss_collect_all_paths(ciss_graph_path_point* path, void* path_list) {
  ciss_graph_path_list** list = (ciss_graph_path_list**) path_list;
  *list = ciss_graph_path_list_append(*list, ciss_graph_path_clone(path));
}

ciss_graph_path_list* ciss_graph_all_paths(ciss_graph* graph) {
  ciss_graph_path_list* list = NULL;
  ciss_graph_node* node;
  for (node = graph->nodes; node != NULL; node = node->next) {
    ciss_dfs_pu(node, &ciss_collect_all_paths, &list);
  }
  return list;
}

void ciss_graph_path_list_print(ciss_graph_path_list* list) {
  ciss_graph_path_list* l;
  for (l = list; l != NULL; l = l->next) {
    ciss_graph_path* path = l->path;
    for (path = l->path; path != NULL; path = path->next) {
      ciss_graph_arc* arc = path->arc;
      printf("(%d -> %d)", arc->source->label, arc->target->label);
    }
    printf("\n");
  }
}

osl_relation_p ciss_process_dependence(osl_relation_p target_domain, osl_relation_p source_domain,
                                       isl_union_map* dependence_umap, isl_ctx* ctx) {
  isl_union_map* source_domain_umap = osl_relation_to_isl_union_map(ctx, source_domain);
  dependence_umap = isl_union_map_apply_range(source_domain_umap, dependence_umap);

  // at this point, dependence_umap should be a set.
  isl_space* space = isl_union_map_get_space(dependence_umap);
  assert(isl_space_dim(space, isl_dim_in) == 0);
  isl_space_free(space);

  isl_union_set* dependence_uset = isl_union_map_range(dependence_umap);
  isl_union_map* target_domain_umap = osl_relation_to_isl_union_map(ctx, target_domain);
  isl_union_set* target_domain_uset = isl_union_map_range(target_domain_umap);
  isl_union_set* intersection = isl_union_set_intersect(dependence_uset, isl_union_set_copy(target_domain_uset));
  isl_union_set* complement = isl_union_set_subtract(target_domain_uset, isl_union_set_copy(intersection));

  osl_relation_p first = isl_union_map_to_osl_relation(isl_union_map_from_range(intersection));
  osl_relation_p second = isl_union_map_to_osl_relation(isl_union_map_from_range(complement));
  LL_APPEND(osl_relation_t, first, second);

  return first;
}

osl_relation_p ciss_split_by_path(osl_relation_p source_domain, osl_relation_p target_domain, ciss_graph_path* path) { // relation = scattered domain or domain?
  // we need to work on scattered domains to check for chunks in a transformed scop, but modify the original domain.
  isl_ctx* ctx = isl_ctx_alloc();
  isl_union_map* dependence_umap = ciss_relation_compose_list_isl(path, ctx);
  osl_relation_p first = ciss_process_dependence(target_domain, source_domain, dependence_umap, ctx);

  isl_ctx_free(ctx);
  return first;
}

osl_relation_p ciss_split_by_kleene(osl_relation_p source_domain, osl_relation_p target_domain, ciss_kleene_element* head) {
  isl_ctx* ctx = isl_ctx_alloc();
  isl_union_map* dependence_umap = ciss_relation_compose_kleene_recurse(ctx, head);
  osl_relation_p first = ciss_process_dependence(target_domain, source_domain, dependence_umap, ctx);

  isl_ctx_free(ctx);
  return first;
}

typedef struct ciss_labeled_domain {
  int label;
  osl_relation_p domain;
  osl_statement_p stmt_ptr;
  struct ciss_labeled_domain* next;
} ciss_labeled_domain;

ciss_labeled_domain* ciss_labeled_domain_find(ciss_labeled_domain* head, int label) {
  for ( ; head != NULL; head = head->next) {
    if (head->label == label)
      break;
  }
  return head;
}

osl_statement_p ciss_osl_statement_find_label(osl_statement_p stmt, int label) {
  candl_statement_usr_p stmt_usr;
  for ( ; stmt != NULL; stmt = stmt->next) {
    stmt_usr = (candl_statement_usr_p) stmt->usr;
    if (stmt_usr && stmt_usr->label == label)
      break;
  }
  return stmt;
}

ciss_labeled_domain* ciss_labeled_domain_list(osl_scop_p scop) {
  ciss_labeled_domain* domains = NULL;
  ciss_labeled_domain* domains_ptr;
  osl_statement_p stmt;
  for (stmt = scop->statement; stmt != NULL; stmt = stmt->next) {
    candl_statement_usr_p stmt_usr = (candl_statement_usr_p) stmt->usr;
    ciss_labeled_domain* domain = (ciss_labeled_domain*) malloc(sizeof(ciss_labeled_domain));
    domain->label = stmt_usr->label;
    domain->domain = osl_relation_clone(stmt->domain);
    domain->stmt_ptr = stmt;

    if (domains == NULL) {
      domains = domain;
      domains_ptr = domain;
    } else {
      domains_ptr->next = domain;
      domains_ptr = domain;
    }
  }

  return domains;
}

void ciss_path(osl_scop_p scop) {
  candl_options_p options = candl_options_malloc();
  options->fullcheck = 1;
  candl_scop_usr_init(scop);

  ciss_labeled_domain* domains = ciss_labeled_domain_list(scop);

  osl_dependence_p dependence = candl_dependence(scop, options);
  ciss_graph* graph = ciss_graph_construct(dependence);
  ciss_graph_path_list* list = ciss_graph_all_paths(graph);

  ciss_graph_path_list* l;
  ciss_graph_path_point* p;
  for (l = list; l != NULL; l = l->next) {
    if (!l->path)
      continue;
    for (p = l->path; p->next != NULL; p = p->next)
      ;
    ciss_graph_node* source = p->arc->source;
    ciss_graph_node* target = p->arc->target;
    ciss_labeled_domain* target_labeled_domain = ciss_labeled_domain_find(domains, target->label);
    osl_relation_p source_domain = ciss_osl_statement_find_label(scop->statement, source->label)->domain;
    target_labeled_domain->domain = ciss_split_by_path(source_domain, target_labeled_domain->domain, l->path);
  }

  osl_relation_print(stdout, domains->domain);

  candl_scop_usr_cleanup(scop);
  candl_options_free(options);
}

void ciss_kleene_path(osl_scop_p scop) {
  candl_options_p options = candl_options_malloc();
  options->fullcheck = 1;
  candl_scop_usr_init(scop);

  ciss_labeled_domain* domains = ciss_labeled_domain_list(scop);
  osl_dependence_p dependence = candl_dependence(scop, options);
  ciss_graph* graph = ciss_graph_construct(dependence);
  ciss_kleene_element** kleene_table = build_kleene(graph);

  size_t nb_nodes, i, j;
  LL_SIZE(ciss_graph_node, graph->nodes, nb_nodes);
  for (i = 0; i < nb_nodes; i++) {
    for (j = 0; j < nb_nodes; j++) {
      ciss_kleene_element* dependence_chain;
      dependence_chain = kleene_table[i * nb_nodes + j];
      if (dependence_chain) {
        printf("%zu -> %zu: ", i, j);
        ciss_kleene_element_print(stdout, dependence_chain);
        printf("\n");

        ciss_relation_compose_kleene(dependence_chain);
        ciss_labeled_domain* target_labeled_domain = ciss_labeled_domain_find(domains,
                                                                              ciss_graph_nth_node(graph, j)->label);
        osl_relation_p source_domain = ciss_osl_statement_find_label(scop->statement,
                                                                     ciss_graph_nth_node(graph, i)->label)->domain;
        target_labeled_domain->domain = ciss_split_by_kleene(source_domain, target_labeled_domain->domain, dependence_chain);
      }
    }
  }
  for (i = 0; i < nb_nodes; ++i) {
    for (j = 0; j < nb_nodes; ++j) {
      ciss_kleene_element_free(kleene_table[i * nb_nodes + j]);
    }
  }

  candl_scop_usr_cleanup(scop);
  candl_options_free(options);
}

void test() {
  ciss_kleene_element* empty = ciss_kleene_element_create_empty();
  ciss_kleene_element_list* inner_list = ciss_kleene_element_list_create(empty);
  ciss_kleene_element* inner = ciss_kleene_element_create_list(inner_list, LIST_ALTERNATIVES);
  ciss_kleene_element_list* outer_list = ciss_kleene_element_list_create(inner);
  ciss_kleene_element* outer = ciss_kleene_element_create_list(outer_list, LIST_ALTERNATIVES);

  ciss_kleene_element_simplify(outer);

  ciss_kleene_element_print(stdout, outer);
  fprintf(stdout, "\n");
  fflush(stdout);

}

int main() {
//  test();
//  return 0;

  osl_scop_p scop = osl_scop_read(stdin);
  ciss_kleene_path(scop);
//  ciss_path(scop);

//  candl_options_p options = candl_options_malloc();
//  options->fullcheck = 1;
//  candl_scop_usr_init(scop);
//  osl_dependence_p dependence = candl_dependence(scop, options);

//  ciss_graph* graph = ciss_graph_construct(dependence);
//  ciss_graph_path_list* list = ciss_graph_all_paths(graph);

//  ciss_graph_path_list_print(list);

//  candl_scop_usr_cleanup(scop);
//  candl_options_free(options);
  osl_scop_free(scop);
  return 0;
}


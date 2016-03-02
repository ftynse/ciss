#ifndef PATH_H
#define PATH_H

#include <osl/extensions/dependence.h>

#include <candl/dependence.h>

#include "graph.h"

// Graph path point does not have ownership of nodes (weak observer ptr).
typedef struct ciss_graph_path_point {
  struct ciss_graph_arc* arc;
  struct ciss_graph_path_point* next;
} ciss_graph_path_point;

typedef struct ciss_graph_path_point ciss_graph_path;

// List has ownership of all paths in it.
typedef struct ciss_graph_path_list {
  ciss_graph_path_point* path;
  struct ciss_graph_path_list* next;
} ciss_graph_path_list;

struct ciss_kleene_element;

typedef struct ciss_kleene_element_list {
  struct ciss_kleene_element*      element;
  struct ciss_kleene_element_list* next;
} ciss_kleene_element_list;

typedef struct ciss_kleene_element {
  union {
    struct ciss_graph_arc*           arc;
    struct ciss_kleene_element_list* list;
    struct ciss_kleene_element*      star;
    struct ciss_graph_node*          node;
  };

  enum {
    SINGLE,
    LIST_SEQUENCE,
    LIST_ALTERNATIVES,
    STAR,
    EMPTY,
    EPSILON
  } type;
} ciss_kleene_element;

ciss_kleene_element* ciss_kleene_element_create_single(ciss_graph_arc* arc);
ciss_kleene_element* ciss_kleene_element_create_list(ciss_kleene_element_list* list, int type);
ciss_kleene_element* ciss_kleene_element_create_star(ciss_kleene_element* star);
ciss_kleene_element* ciss_kleene_element_create_epsilon(ciss_graph_node* node);
ciss_kleene_element* ciss_kleene_element_create_empty();
void                 ciss_kleene_element_free(ciss_kleene_element* element);
void                 ciss_kleene_element_simplify(ciss_kleene_element* head);
int                  ciss_kleene_element_equal(ciss_kleene_element* first, ciss_kleene_element* second);
void                 ciss_kleene_element_print(FILE* file, ciss_kleene_element* head);
void                 ciss_kleene_element_copy(ciss_kleene_element* target, ciss_kleene_element* source);
ciss_kleene_element* ciss_kleene_element_clone(ciss_kleene_element* element);

ciss_kleene_element_list* ciss_kleene_element_list_create(ciss_kleene_element* cke);
ciss_kleene_element_list* ciss_kleene_element_list_append(ciss_kleene_element_list* list,
                                                          ciss_kleene_element* cke);
void                      ciss_kleene_element_list_remove(ciss_kleene_element_list** head,
                                                          ciss_kleene_element_list* element);

//+//////// graph path-related
size_t ciss_graph_path_count_arcs(ciss_graph_path*, ciss_graph_arc*);
ciss_graph_path_point* ciss_graph_path_point_create(ciss_graph_arc*);
ciss_graph_path_point* ciss_graph_path_point_malloc();
ciss_graph_path_point* ciss_graph_path_clone();
void ciss_graph_path_destroy(ciss_graph_path_point*);

ciss_graph_path_point* ciss_graph_path_append(ciss_graph_path_point*, ciss_graph_arc*);

//+//////// graph path list-related
ciss_graph_path_list* ciss_graph_path_list_create();
void ciss_graph_path_list_destroy(ciss_graph_path_list*);
void ciss_graph_path_list_clear(ciss_graph_path_list*);
ciss_graph_path_list* ciss_graph_path_list_append(ciss_graph_path_list*,
                                                  ciss_graph_path_point*);

void check_loop(ciss_kleene_element*);
#endif // PATH_H

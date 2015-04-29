#include "graph.h"
#include "linked_list.h"
#include "path.h"

#include <stdlib.h>

ciss_kleene_element* ciss_kleene_element_create_single(ciss_graph_arc *arc) {
  ciss_kleene_element* arc_element = (ciss_kleene_element*) malloc(sizeof(ciss_kleene_element));
  arc_element->type = SINGLE;
  arc_element->arc = arc;
  return arc_element;
}

ciss_kleene_element* ciss_kleene_element_create_list(ciss_kleene_element_list *list, int type) {
  ciss_kleene_element* element = (ciss_kleene_element*) malloc(sizeof(ciss_kleene_element));
  element->type = type;
  element->list = list;
  return element;
}

ciss_kleene_element* ciss_kleene_element_create_star(ciss_kleene_element *star) {
  ciss_kleene_element* element = (ciss_kleene_element*) malloc(sizeof(ciss_kleene_element));
  element->type = STAR;
  element->star = star;
  return element;
}

ciss_kleene_element* ciss_kleene_element_create_epsilon(ciss_graph_node *node) {
  ciss_kleene_element* element = (ciss_kleene_element*) malloc(sizeof(ciss_kleene_element));
  element->node = node;
  element->type = EPSILON;
  return element;
}

ciss_kleene_element* ciss_kleene_element_create_empty() {
  ciss_kleene_element* element = (ciss_kleene_element*) malloc(sizeof(ciss_kleene_element));
  element->type = EMPTY;
  return element;
}

ciss_kleene_element_list* ciss_kleene_element_list_create(ciss_kleene_element* cke) {
  ciss_kleene_element_list* list_element = (ciss_kleene_element_list*) malloc(sizeof(ciss_kleene_element_list));
  list_element->element = cke;
  list_element->next = NULL;
  return list_element;
}

ciss_kleene_element_list* ciss_kleene_element_list_append(ciss_kleene_element_list* list,
                                                          ciss_kleene_element* cke) {
  ciss_kleene_element_list* list_element = ciss_kleene_element_list_create(cke);
  LL_APPEND(ciss_kleene_element_list, list, list_element);
  return list;
}

//int ciss_graph_path_contains_node(ciss_graph_path_point* start, ciss_graph_node* node) {
//  ciss_graph_path_point* point;
//  if (start == NULL || node == NULL)
//    return 0;

//  for (point = start; point != NULL; point = point->next) {
//    if (point->arc->target == node)
//      return 1;
//  }
//  return 0;
//}

size_t ciss_graph_path_count_arcs(ciss_graph_path* path, ciss_graph_arc* arc) {
  ciss_graph_path_point* point;
  size_t result = 0;
  if (path == NULL || arc == NULL)
    return 0;

  for (point = path; point != NULL; point = point->next) {
    if (ciss_graph_arc_equal(point->arc, arc)) {
      result++;
    }
  }
  return result;
}

ciss_graph_path_point* ciss_graph_path_point_create(ciss_graph_arc* arc) {
  ciss_graph_path_point* point = (ciss_graph_path_point*) malloc(sizeof(ciss_graph_path_point));
  point->next = NULL;
  point->arc = arc;
  return point;
}

ciss_graph_path_point* ciss_graph_path_append(ciss_graph_path_point* start, ciss_graph_arc* arc) {
  ciss_graph_path_point* newpoint;
  if (arc == NULL)
    return start;

  newpoint = ciss_graph_path_point_create(arc);
  LL_APPEND(ciss_graph_path_point, start, newpoint);

  return start;
}

ciss_graph_path_point* ciss_graph_path_point_malloc() {
  return ciss_graph_path_point_create(NULL);
}

ciss_graph_path_point* ciss_graph_path_clone(ciss_graph_path_point* start) {
  ciss_graph_path_point* point;
  ciss_graph_path_point* result = NULL;
  ciss_graph_path_point* previous = NULL;

  for ( ; start != NULL; start = start->next) {
    point = ciss_graph_path_point_create(start->arc);
    if (result == NULL) {
      result = point;
      previous = point;
    } else {
      previous->next = point;
      previous = point;
    }
  }
  return result;
}

void ciss_graph_path_destroy(ciss_graph_path_point* start) {
  LL_FREE(ciss_graph_path_point, start);
}

///////////////

ciss_graph_path_list* ciss_graph_path_list_create() {
  // I hate boilerplate code in C...
  ciss_graph_path_list* list = (ciss_graph_path_list*) malloc(sizeof(ciss_graph_path_list));
  list->next = NULL;
  list->path = NULL;
  return list;
}

void ciss_graph_path_list_destroy(ciss_graph_path_list* list) {
  ciss_graph_path_list* element;
  if (list == NULL)
    return;

  for (element = list; element != NULL; element = list->next) {
    ciss_graph_path_destroy(element->path);
  }
  free(list);
}

// Clears the path list without destroying the paths
void ciss_graph_path_list_clear(ciss_graph_path_list* list) {
  if (list != NULL) {
    list->next = NULL;
    list->path = NULL;
  }
}

ciss_graph_path_list* ciss_graph_path_list_append(ciss_graph_path_list* list,
                                                  ciss_graph_path_point* path) {
  ciss_graph_path_list* ptr;
  if (list == NULL) {
    list = ciss_graph_path_list_create();
    list->path = path;
    return list;
  }

  for (ptr = list; ptr->next != NULL; ptr = ptr->next)
    ;
  ptr->next = ciss_graph_path_list_create();
  ptr->next->path = path;
  return list;
}

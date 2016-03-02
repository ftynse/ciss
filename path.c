#include "graph.h"
#include "linked_list.h"
#include "path.h"

#include <stdlib.h>
#include <string.h>
#include <assert.h>

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

void check_loop(ciss_kleene_element *target) {
  if (target->type == LIST_ALTERNATIVES || target->type == LIST_SEQUENCE) {
    for (ciss_kleene_element_list* l = target->list; l != NULL; l = l->next) {
      assert(l->element != target && "gotcha 1");
      assert(l->next != target->list && "loop");
    }
  }
}

void ciss_kleene_element_copy(ciss_kleene_element *target, ciss_kleene_element *source) {
  // memcopy works just fine here, preserves type and pointer
  memcpy(target, source, sizeof(ciss_kleene_element));
}

ciss_kleene_element* ciss_kleene_element_clone(ciss_kleene_element* element) {
  if (!element)
    return NULL;

  switch (element->type) {
  case EMPTY:
    return ciss_kleene_element_create_empty();

  case EPSILON:
    return ciss_kleene_element_create_epsilon(element->node);

  case STAR:
    return ciss_kleene_element_create_star(ciss_kleene_element_clone(element->star));

  case SINGLE:
    return ciss_kleene_element_create_single(element->arc);

  case LIST_SEQUENCE:
  case LIST_ALTERNATIVES:
  {
    ciss_kleene_element_list* l;
    ciss_kleene_element_list* list = NULL;
    for (l = element->list; l != NULL; l = l->next) {
      list = ciss_kleene_element_list_append(list, ciss_kleene_element_clone(l->element));
    }
    return ciss_kleene_element_create_list(list, element->type);
  }
  }
}

void ciss_kleene_element_free(ciss_kleene_element *element) {
  if (element->type == STAR) {
    ciss_kleene_element_free(element->star);
  } else if (element->type == LIST_ALTERNATIVES || element->type == LIST_SEQUENCE) {
    ciss_kleene_element_list* l;
    for (l = element->list; l != NULL; l = l->next) {
      ciss_kleene_element_free(l->element);
    }
    free(element->list);
  }
  free(element);
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

void ciss_kleene_element_list_remove(ciss_kleene_element_list** head,
                                     ciss_kleene_element_list* element) {
  ciss_kleene_element_list* previous;
  ciss_kleene_element_list* current;

  if (head == NULL || *head == NULL)
    return;

  if (*head == element) {
    *head = (*head)->next;
    free(element);
    return;
  }

  previous = *head;
  for (current = (*head)->next; current != NULL && current != element; current = current->next)
    previous = current;
  if (current == NULL)
    return;

  previous->next = current->next;
//  free(current);
}

int ciss_kleene_element_equal(ciss_kleene_element* first,
                              ciss_kleene_element* second) {
  if (first == NULL && second == NULL)
    return 1;
  if (first == NULL || second == NULL)
    return 0;
  if (first->type != second->type)
    return 0;

  if (first->type == EPSILON ||
      first->type == EMPTY)     // types are known to match at this point, value-less types are always equal
    return 1;

  if (first->type == STAR) {
    return ciss_kleene_element_equal(first->star, second->star);
  }

  if (first->type == SINGLE) {
    return osl_dependence_equal(first->arc->dependence, second->arc->dependence);
  }

  // Order matters.
  if (first->type == LIST_SEQUENCE) {
    ciss_kleene_element_list* l1;
    ciss_kleene_element_list* l2;

    l1 = first->list;
    l2 = second->list;
    for (; l1 != NULL && l2 != NULL; l1 = l1->next, l2 = l2->next) {
      if (!ciss_kleene_element_equal(l1->element, l2->element))
        return 0;
    }
    if (l1 != NULL || l2 != NULL)
      return 0;
    return 1;
  }

  // Order does not matter, nor does repetition.
  if (first->type == LIST_ALTERNATIVES) {
    size_t matches, size1, size2, i;
    ciss_kleene_element_list* l1;
    ciss_kleene_element_list* l2;
    ciss_kleene_element_list** matched_elems;

    LL_SIZE(ciss_kleene_element_list, first->list, size1);
    LL_SIZE(ciss_kleene_element_list, second->list, size2);

    if (size1 != size2)
      return 0;

    matches = 0;
    matched_elems = (ciss_kleene_element_list**) malloc(size1 * sizeof(ciss_kleene_element_list*));
    for (l1 = first->list; l1 != NULL; l1 = l1->next) {
      for (l2 = second->list; l2 != NULL; l2 = l2->next) {
        // Skip already matched elements.
        for (i = 0; i < matches; ++i)
          if (matched_elems[i] == l2)
            continue;

        if (ciss_kleene_element_equal(l1->element, l2->element)) {
          matched_elems[matches] = l2;
          ++matches;
          break;
        }
      }
    }
    return matches == size2; // everything matched.
  }

  return 0; // something weird happened.
}

void ciss_kleene_element_list_lift(ciss_kleene_element* head, int type) {
  ciss_kleene_element_list* previous;
  ciss_kleene_element_list* l;
  previous = head->list;

  if (previous != NULL) {
    if (previous->element->type == type &&
        previous->element->list != NULL) {
      for (l = previous->element->list; l->next != NULL; l = l->next)
        ;
      l->next = previous->next;
      head->list = previous->element->list;
      previous = previous->element->list;
    }
  }

  while (previous != NULL) {
    ciss_kleene_element_list* current = previous->next;
    if (current == NULL) {
      break;
    } else {
      if (current->element->type == type &&
          current->element->list != NULL) {
        previous->next = current->element->list;
        check_loop(current->element);
        for (l = current->element->list; l->next != NULL; l = l->next)
          ;
        l->next = current->next;
        current->next = NULL;
      }
    }

    previous = previous->next;
  }
}

void ciss_kleene_element_simplify(ciss_kleene_element* head) {
  // Recurse first.
  if (head->type == EMPTY || head->type == EPSILON || head->type == SINGLE) {
    return; // single elements are not simplifiable
  }
  else if (head->type == LIST_SEQUENCE || head->type == LIST_ALTERNATIVES) {
    // TODO: Perform distribution a(b+c) = ab + ac ?
    // actually the inverse may be interesting for 1+xx* or 1+x*x inside the parthentheses that simplify to x*

    ciss_kleene_element_list* l;
    for (l = head->list; l != NULL; l = l->next) {
      ciss_kleene_element_simplify(l->element);
    }
  } else if (head->type == STAR) {
    ciss_kleene_element_simplify(head->star);
  }

  // Simplify current.
  if (head->type == LIST_SEQUENCE) {
    // Lift sequence (associativity).
    ciss_kleene_element_list_lift(head, LIST_SEQUENCE);

    assert(head->list && "Empty list");

    // Identity (a . EPSILON == a) => remove epsilons, except if it's the only element.
    while (1) {
      int epsilon_found = 0;
      int nb_elems;
      LL_SIZE(ciss_kleene_element_list, head->list, nb_elems);
      if (nb_elems == 1)
        break;

      for (ciss_kleene_element_list *l = head->list; l != NULL; l = l->next) {
        if (l->element->type == EPSILON) {
          epsilon_found = 1;
          ciss_kleene_element_list_remove(&head->list, l);
          break;
        }
      }
      if (!epsilon_found)
        break;
    }

    // Zero (a . EMPTY == EMPTY) => if at least one empty, replace by empty
    int replace = 0;
    for (ciss_kleene_element_list *l = head->list; l != NULL; l = l->next) {
      if (l->element->type == EMPTY) {
        replace = 1;
        break;
      }
    }
    if (replace) {
      head->type = EMPTY;
      LL_FREE(ciss_kleene_element_list, head->list);
      head->list = NULL;
      return;
    }

    // Sequenced stars ((a)*(a)* == (a)*)
    ciss_kleene_element_list* previous = head->list;
    while (previous != NULL) {
      ciss_kleene_element_list* current = previous->next;
      if (current == NULL)
        break;
      if (current->element->type == STAR &&
          previous->element->type == STAR &&
          ciss_kleene_element_equal(current->element->star, previous->element->star)) {
        // Brutal removal, no need to call the function as we already maintain previous pointer.
        previous->next = current->next;
        free(current);
      }
      previous = previous->next;
    }

    // Remove one-element sequences.
    if (head->list->next == NULL) {
      ciss_kleene_element* child = head->list->element;
      ciss_kleene_element_copy(head, child);
    }

  } else if (head->type == LIST_ALTERNATIVES) {
    // Lift alternatives (associativity).
    ciss_kleene_element_list_lift(head, LIST_ALTERNATIVES);

    assert(head->list && "Empty list");

    // Idempotence (a + a == a) => remove duplicates.
    ciss_kleene_element_list* l1;
    for (l1 = head->list; l1 != NULL; l1 = l1->next) {
      ciss_kleene_element_list* previous = l1;
      while (previous != NULL) {
        ciss_kleene_element_list* current = previous->next;
        if (current == NULL)
          break;
        if (ciss_kleene_element_equal(l1->element, current->element)) {
          // Brutally remove current, no need to call _remove as we already maintain previous pointer.
          previous->next = current->next;
          free(current);
        }
        previous = previous->next;
      }
    }

    // At this point, there can be only one empty element, others are removed as its duplicates.
    // Identity (a + EMPTY == a) => remove empties.
    // If the only element is left, lift it.
    if (head->list->next == NULL) {
      ciss_kleene_element* child = head->list->element;
      ciss_kleene_element_copy(head, child);
      return;
    }
    // If there are two elements, and one of the is EMPTY, lift non-Empty immediately
    else if (head->list->next && head->list->next->next == NULL) {
      if (head->list->element->type == EMPTY) {
        ciss_kleene_element_copy(head, head->list->next->element);
        return;
      } else if (head->list->next->element->type == EMPTY) {
        ciss_kleene_element_copy(head, head->list->element);
        return;
      }
    }
    // Otherwise remove the only empty element if present.
    ciss_kleene_element_list *l;
    for (l = head->list; l != NULL; l = l->next) {
      if (l->element->type == EMPTY) {
        ciss_kleene_element_list_remove(&head->list, l);
        break;
      }
    }

  } else if (head->type == STAR) {
    // Nested stars ((a*)* == a*).
    if (head->star->type == STAR) {
      ciss_kleene_element* old_element = head->star;
      head->star = head->star->star;
      free(old_element);
    }

    // Empty star ((EMPTY)* == EPSILON).
    // Epsilon star ((EPSILON)* == EPSILON).
    if (head->star->type == EMPTY || head->star->type == EPSILON) {
      head->type = EPSILON;
      head->node = head->star->node;
      head->star = NULL;
    }

    // TODO: complex optimization
    // property (x+y)*=x*(yx*)* is interesting in cases where x=1
    // (1+y)*=1*(y1*)*=1(y1)*=1(y)*=y*
    // (x+1)*=(1+x)*=x* (due to commutativity of +)
  }
}

void ciss_kleene_element_print(FILE *file, ciss_kleene_element *head) {
  if (head->type == SINGLE) {
    fprintf(file, "{%p:S%d->S%d}", head->arc->dependence, head->arc->source->label, head->arc->target->label);
  } else if (head->type == STAR) {
    fprintf(file, "(");
    ciss_kleene_element_print(file, head->star);
    fprintf(file, ")*");
  } else if (head->type == LIST_SEQUENCE) {
    ciss_kleene_element_list* l;
    for (l = head->list; l != NULL; l = l->next) {
      ciss_kleene_element_print(file, l->element);
    }
  } else if (head->type == LIST_ALTERNATIVES) {
    ciss_kleene_element_list* l;
    fprintf(file, "[");
    for (l = head->list; l != NULL; l = l->next) {
      if (l != head->list) {
        fprintf(file, "|");
      }
      ciss_kleene_element_print(file, l->element);
    }
    fprintf(file, "]");
  } else if (head->type == EMPTY) {
    fprintf(file, "0");
  } else if (head->type == EPSILON) {
    fprintf(file, "1");
    if (head->node) {
      fprintf(file, ":%d", head->node->label);
    }
  }
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

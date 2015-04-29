#include <osl/extensions/dependence.h>

#include <candl/candl.h>
#include <candl/dependence.h>

#include "graph.h"
#include "linked_list.h"

#include <stdlib.h>
#include <limits.h>

//+/////////////// node-related
ciss_graph_node* ciss_graph_node_create(int label) {
  ciss_graph_node* node;
  node = (ciss_graph_node*) malloc(sizeof(ciss_graph_node));
  node->label = label;
  node->outgoing = NULL;
  node->incoming = NULL;
  node->next = NULL;
  return node;
}

ciss_graph_node* ciss_graph_node_malloc() {
  return ciss_graph_node_create(INT_MAX);
}

void ciss_graph_node_append_arc(ciss_graph_node* node,
                                ciss_graph_arc* arc) {
  LL_APPEND(ciss_graph_arc, node->outgoing, arc);
}

void ciss_graph_connect_nodes(ciss_graph_node* source,
                              ciss_graph_node* target,
                              osl_dependence_p dep) {
  ciss_graph_arc* outgoing_arc = ciss_graph_arc_create(source, target, dep);
  ciss_graph_arc* incoming_arc = ciss_graph_arc_create(source, target, dep);
  LL_APPEND(ciss_graph_arc, source->outgoing, outgoing_arc);
  LL_APPEND(ciss_graph_arc, target->incoming, incoming_arc);
}

//+/////////////// arc-related
ciss_graph_arc* ciss_graph_arc_create(ciss_graph_node* source,
                                      ciss_graph_node* target,
                                      osl_dependence_p dep) {
  ciss_graph_arc* arc = (ciss_graph_arc*) malloc(sizeof(ciss_graph_arc));
  arc->source = source;
  arc->target = target;
  arc->dependence = dep;
  arc->next = NULL;
  return arc;
}

ciss_graph_arc* ciss_graph_arc_malloc() {
  return ciss_graph_arc_create(NULL, NULL, NULL);
}

void ciss_graph_arc_free(ciss_graph_arc* arc) {
  free(arc);
}

void ciss_graph_arc_destroy(ciss_graph_arc* arc) {
  if (arc == NULL)
    return;
  osl_dependence_free(arc->dependence);
  ciss_graph_arc_free(arc);
}

int ciss_graph_arc_equal(ciss_graph_arc* a1, ciss_graph_arc* a2) {
  if (a1 == a2)
    return 1;
  if (a1->source == a2->source &&
      a1->target == a2->target &&
      osl_dependence_equal(a1->dependence, a2->dependence)) {
    return 1;
  }
  return 0;
}

//+//////////////// graph-related
ciss_graph* ciss_graph_create() {
  ciss_graph* graph = (ciss_graph*) malloc(sizeof(ciss_graph));
  graph->nodes = NULL;
  return graph;
}

ciss_graph* ciss_graph_malloc() {
  return ciss_graph_create();
}

ciss_graph_node* ciss_graph_find_node(ciss_graph* graph, int label) {
  ciss_graph_node* node;
  for (node = graph->nodes; node != NULL; node = node->next) {
    if (node->label == label)
      return node;
  }
  return NULL;
}

void ciss_graph_append_node(ciss_graph* graph, ciss_graph_node* node) {
  LL_APPEND(ciss_graph_node, graph->nodes, node);
}

size_t ciss_graph_node_number(ciss_graph* graph) {
  size_t result;
  LL_SIZE(ciss_graph_node, graph->nodes, result);
  return result;
}

ciss_graph_node* ciss_graph_ensure_node(ciss_graph* graph, int label) {
  ciss_graph_node* node;

  node = ciss_graph_find_node(graph, label);
  if (node != NULL)
    return node;

  node = ciss_graph_node_create(label);
  ciss_graph_append_node(graph, node);
  return node;
}

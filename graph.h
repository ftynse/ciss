#ifndef GRAPH_H
#define GRAPH_H

#include <stdlib.h>

struct ciss_graph_arc;
struct osl_dependence;
struct osl_relation;

// Graph node has ownership of its children.
// Graph node has ownership of dependence stored.
typedef struct ciss_graph_node {
  int label;
  struct ciss_graph_arc* outgoing;
  struct ciss_graph_arc* incoming;
  struct osl_relation* domain_ptr;
  struct ciss_graph_node* next;
} ciss_graph_node;

typedef struct ciss_graph_arc {
  struct ciss_graph_node* source;
  struct ciss_graph_node* target;
  struct osl_dependence* dependence;
  struct ciss_graph_arc* next;
} ciss_graph_arc;

typedef struct ciss_graph {
  struct ciss_graph_node* nodes;
} ciss_graph;

//+/// node-related functions
ciss_graph_node* ciss_graph_node_create(int label);
ciss_graph_node* ciss_graph_node_malloc();

void ciss_graph_node_append_arc(ciss_graph_node*, ciss_graph_arc*);

//+/// arc-related functions
ciss_graph_arc* ciss_graph_arc_create(ciss_graph_node*, ciss_graph_node*, struct osl_dependence*);
ciss_graph_arc* ciss_graph_arc_malloc();
void ciss_graph_arc_destroy(ciss_graph_arc*);
void ciss_graph_arc_free(ciss_graph_arc*);
int ciss_graph_arc_equal(ciss_graph_arc*, ciss_graph_arc*);

//+/// graph-related functions
ciss_graph* ciss_graph_create();
ciss_graph* ciss_graph_malloc();

ciss_graph_node* ciss_graph_find_node(ciss_graph*, int);
void ciss_graph_append_node(ciss_graph*, ciss_graph_node*);
size_t ciss_graph_node_number(ciss_graph*);
ciss_graph_node* ciss_graph_ensure_node(ciss_graph*, int);

void ciss_graph_connect_nodes(ciss_graph_node* source,
                              ciss_graph_node* target,
                              struct osl_dependence* dep);

#endif // GRAPH_H

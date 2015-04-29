#include "../convert.h"

int main() {
  osl_scop_p scop = osl_scop_read(stdin);
  isl_ctx* ctx = isl_ctx_alloc();
//  osl_dependence_p dependence = (osl_dependence_p) osl_generic_lookup(scop->extension, OSL_URI_DEPENDENCE);
  isl_union_map* umap = osl_relation_to_isl_union_map(ctx, scop->statement->scattering);
  isl_printer* prn = isl_printer_to_file(ctx, stdout);
  prn = isl_printer_print_union_map(prn, umap);
  prn = isl_printer_print_str(prn, "\n");
  osl_relation_p relation = isl_union_map_to_osl_relation(umap);
  osl_relation_print(stdout, relation);
  prn = isl_printer_flush(prn);
  isl_printer_free(prn);
  isl_ctx_free(ctx);
  return 0;
}

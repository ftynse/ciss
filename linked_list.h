#ifndef LINKED_LIST_H
#define LINKED_LIST_H

#define LL_APPEND(type, name, value) \
  do { \
    type* iter; \
    if (name == NULL) { \
      name = value; \
    } else { \
      for (iter = name; iter->next != NULL; iter = iter->next) \
        ; \
      iter->next = value; \
    } \
  } while (0)

#define LL_SIZE(type, list, result) \
  do { \
    type* iter; \
    size_t size = 0; \
    if (list == NULL) { \
      result = 0; \
    } else { \
      for (iter = list; iter != NULL; iter = iter->next) \
        ++size; \
      result = size;\
    } \
  } while(0)

#define LL_FREE(type, list) \
  do { \
    type* iter; \
    while (list != NULL) { \
      iter = list->next; \
      free(list); \
      list = iter; \
    } \
  } while (0)

#endif // LINKED_LIST_H

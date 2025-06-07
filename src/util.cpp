#include "util.hpp"

wl_list *wl_list_get_last_item(wl_list *list) {
    int size = wl_list_length(list);
    wl_list *tail = list;
    for (int i = 0; i < size; i++) {
        tail = tail->next;
    }

    return tail;
}

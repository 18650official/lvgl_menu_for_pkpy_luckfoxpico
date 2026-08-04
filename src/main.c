#define _DEFAULT_SOURCE
#include <unistd.h>
#include "ui/ui.h"

int main(void)
{
    ui_backend_init();
    ui_init();

    while (1) {
        lv_timer_handler();
        usleep(5000);
    }

    return 0;
}

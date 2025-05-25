#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <cairo/cairo.h>
#include <lv2/core/lv2.h>
#include <lv2/ui/ui.h>

#include "xputty.h"
#include "xadjustment.h"
#include "xwidget.h"
#include "xwidgets.h"

#define RADIOFX_UI_URI "https://github.com/rbmannchued/radiofx#ui"
#define RADIOFX_URI "https://github.com/rbmannchued/radiofx"

#define CUTOFF_PORT 2
#define BYPASS_PORT 3
#define QUALITY_PORT 4
#define BANDGAIN_PORT 5

typedef struct {
    Xputty main;
    Widget_t *win;
    Widget_t *cutoff;
    Widget_t *bypass;
    Widget_t *quality;
    Widget_t *bandgain;
    LV2UI_Write_Function write;
    LV2UI_Controller controller;
}RadioFxUI;

static void value_changed_callback(void *widget_ptr, void *user_data) {
    Widget_t *widget = (Widget_t*)widget_ptr;
    RadioFxUI *ui = (RadioFxUI*)widget->parent_struct;
    if (!ui) return;

    float value = adj_get_value(widget->adj);
    uint32_t port = (uint32_t)(uintptr_t)widget->data;
    ui->write(ui->controller, port, sizeof(float), 0, &value);
}


void exposeCallback (void* obj, void* data)
{
    Widget_t *widget = (Widget_t*)obj;
    cairo_set_source_surface (widget->crb, widget->image, 0, 0);
    cairo_paint (widget->crb);
}

static LV2UI_Handle instantiate(const LV2UI_Descriptor* descriptor,
                                const char* plugin_uri,
                                const char* bundle_path,
                                LV2UI_Write_Function write_func,
                                LV2UI_Controller controller,
                                LV2UI_Widget* widget,
                                const LV2_Feature* const* features) {
    if (strcmp(plugin_uri, RADIOFX_URI)) return NULL;

    /* LV2UI_Resize* resize = NULL; */
    /* for (int i = 0; features[i]; ++i) { */
    /*     if (!strcmp(features[i]->URI, LV2_UI__parent)) { */
    /*         *widget = (LV2UI_Widget)features[i]->data; */
    /*     } else if (!strcmp(features[i]->URI, LV2_UI__resize)) { */
    /*         resize = (LV2UI_Resize*)features[i]->data; */
    /*     } */
    /* } */
 	
    RadioFxUI *ui = calloc(1, sizeof(RadioFxUI));
    if (!ui) return NULL;

    ui->write = write_func;
    ui->controller = controller;
//    DefaultRootWindow(NULL), 0
    main_init(&ui->main);
    ui->win = create_window(&ui->main, os_get_root_window(&ui->main, IS_WINDOW), 0, 0, 500, 500);
    ui->win->parent_struct = ui;
    ui->win->label = "RadioFX";

    ui->win->func.expose_callback = exposeCallback;

    // Knob: Cutoff
    ui->cutoff = add_knob(ui->win, "Cutoff", 10, 20, 60, 60);
    ui->cutoff->parent_struct = ui;
    ui->cutoff->data = CUTOFF_PORT;
    ui->cutoff->func.value_changed_callback = value_changed_callback;
    set_adjustment(ui->cutoff->adj, 1000.0, 1000.0, 20.0, 20000.0, 1.0, CL_CONTINUOS);

    // Knob: Quality
    ui->quality = add_knob(ui->win, "Q", 80, 10, 60, 60);
    ui->quality->parent_struct = ui;
    ui->quality->data = QUALITY_PORT;
    ui->quality->func.value_changed_callback = value_changed_callback;
    set_adjustment(ui->quality->adj, 0.707, 0.707, 0.1, 10.0, 0.01, CL_CONTINUOS);

    // Knob: Bandgain
    ui->bandgain = add_knob(ui->win, "Gain", 150, 10, 60, 60);
    ui->bandgain->parent_struct = ui;
    ui->bandgain->data = BANDGAIN_PORT;
    ui->bandgain->func.value_changed_callback = value_changed_callback;
    set_adjustment(ui->bandgain->adj, 0.0, 0.0, -24.0, 24.0, 0.1, CL_CONTINUOS);

    // Toggle: Bypass
    ui->bypass = add_toggle_button(ui->win, "Bypass", 220, 25, 60, 60);
    ui->bypass->parent_struct = ui;
    ui->bypass->data = BYPASS_PORT;
    ui->bypass->func.value_changed_callback = value_changed_callback;
    set_adjustment(ui->bypass->adj, 0.0, 0.0, 0.0, 1.0, 1.0, CL_TOGGLE);

    /* if (resize) { */
    /*     resize->ui_resize(resize->handle, 350, 150); */
    /* } */

    widget_show_all(ui->win);
    //widget = (LV2UI_Widget)ui->win->widget;

    return (LV2UI_Handle)ui;
}

static void cleanup(LV2UI_Handle handle) {
    RadioFxUI* ui = (RadioFxUI*)handle;
    main_quit(&ui->main);
    free(ui);
}

static void port_event(LV2UI_Handle handle,
                      uint32_t port_index,
                      uint32_t buffer_size,
                      uint32_t format,
                      const void* buffer) {
    RadioFxUI* ui = (RadioFxUI*)handle;
    if (format != 0 || buffer_size != sizeof(float)) return 0;

    float val = *(const float*)buffer;
    switch (port_index) {
        case CUTOFF_PORT:
            adj_set_value(ui->cutoff->adj, val);
            break;
        case QUALITY_PORT:
            adj_set_value(ui->quality->adj, val);
            break;
        case BANDGAIN_PORT:
            adj_set_value(ui->bandgain->adj, val);
            break;
        case BYPASS_PORT:
            adj_set_value(ui->bypass->adj, val);
            break;
        default:
            break;
    }
    return 0;
}
static int ui_idle(LV2UI_Handle ui) {
    RadioFxUI* myui = (RadioFxUI*)ui;
    if (!myui) return 0;
    
    // Adicione logs para depuração
    fprintf(stderr, "Running UI idle\n");
    
    // Certifique-se que está passando o ponteiro correto
    run_embedded(&myui->main);
    return 0;
}
static const void* extension_data(const char* uri) {
    static const LV2UI_Idle_Interface idle = { ui_idle };
    if (strcmp (uri, LV2_UI__idleInterface) == 0) return &idle;
    return NULL;
}

static const LV2UI_Descriptor descriptor = {
    RADIOFX_UI_URI,
    instantiate,
    cleanup,
    port_event,
    extension_data
};

LV2_SYMBOL_EXPORT
const LV2UI_Descriptor* lv2ui_descriptor(uint32_t index) {
    return (index == 0) ? &descriptor : NULL;
}

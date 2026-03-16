#include <stddef.h>
#include <math.h>

/*
    User Info
*/

/*
    Before including the header you have to define thing's injected into the library below.

    Note header defines shall be the same for all affinelation units 
    - it is recommended to wrap ui.h inclusion with your own header with the defintions.

    Also note, you can implement ui_draw functions in the same affinelation unit as the
    ui.h implementation by first including ui.h without UI_IMPL (for ui_transform definition),
    and later with UI_IMPL (for implementation)

    Render coordinate system:
             (0,  1)
    (-1, 0)           (1, 0)
             (0, -1)
*/

#ifdef UI_IMPL

void ui_draw_box(const ui_transform* matrix, const void* data, void* ctx);
void ui_draw_img(const ui_transform* matrix, const void* data, void* ctx); 
void ui_draw_txt(const ui_transform* matrix, const void* data, void* ctx);
void ui_draw_geo(const ui_transform* matrix, const void* data, void* ctx);

#endif

/*
    Header
*/

#ifndef UI_H
#define UI_H

// ===========================
// Typedefs

typedef unsigned short ui_proportion;

// structure representing ui elements tranformations (matrix 2x3)
typedef struct ui_transform {
    float m00, m01, tx;
    float m10, m11, ty;
} ui_transform;

// Default ui element transform
// offset: (0, 0), scale: (1, 1), rotation: (0)
static const ui_transform ui_default_trans = {
    1, 0, 0,
    0, 1, 0
};

typedef enum ui_node_type : char {
    // Instancing

    // unimplemented!
    ui_node_instance,         // offsets data reads by own data

    // Render
    ui_node_render_transform, // render transform

    // unimplemented!
    ui_node_render_clip,      // clipping space

    // Primitives

    ui_node_box,       // box    render primitive
    ui_node_img,       // image  render primitive
    ui_node_txt,       // text   render primitive
    ui_node_geo,       // custom render primitive

    ui_node_spacer,    // spacer  layout primitive

    // unimplemented!
    ui_node_padding,   // padding layout primitive

    // Layouts
    ui_node_row,       // row    layout
    ui_node_column,    // column layout
} ui_node_type;

typedef enum ui_node_flag : char {
    ui_flag_none       = 0,

    // unimplemented!
    ui_flag_instanced  = 1, // whether data is instanced (last ui_node_instance data is added to data value)
    ui_flag_dispatched = 2  // whether data is searched at *node.data or **node.data
} ui_node_flag;

typedef struct ui_node {
    ui_node_type    type;
    ui_node_flag    flags;
    size_t          child_count;
    struct ui_node* children;
    void*           data;
} ui_node;

// Spacer and Padding

// precent - precent of raw current layout space
typedef struct ui_spacer_data {
    float precent;
} ui_spacer_data;

// all cords = precent of raw curent layout space
typedef struct ui_padding_data {
    float left;
    float right;
    float top;
    float bottom;
} ui_padding_data;

// Canvas

typedef struct ui_canvas_data {

} ui_canvas_data;

// Row

typedef enum ui_row_allign {
    ui_row_allign_left, 
    ui_row_allign_center, 
    ui_row_allign_right
} ui_row_allign;

typedef struct ui_row_data {
    // spacing between elements
    // precent of total layout space
    float spacing;

    // proportion of received space between elements, 
    // ignored for spacers (shall be 0 for correct sum)
    // optional may be 0x0 (equal proportion)
    ui_proportion* proportions;

    // allignation
    ui_row_allign  allign;
} ui_row_data;

// Column

typedef enum ui_column_allign {
    ui_column_allign_top, 
    ui_column_allign_center, 
    ui_column_allign_bottom
} ui_column_allign;

typedef struct ui_column_data {
    // spacing between elements
    // precent of total layout space
    float spacing;

    // proportion of received space between elements, 
    // ignored for spacers (shall be 0 for correct sum)
    // optional may be 0x0 (equal proportion)
    ui_proportion*   proportions;

    // allignation
    ui_column_allign allign;
} ui_column_data;

// ===========================
// Draw

void ui_draw(const ui_node* node, void* ctx);

// ===========================
// Runtime Transformation

static inline ui_transform ui_trans(float offx, float offy, float scalex, float scaley, float deg_cw) {
    float rad = deg_cw * (3.14159265358979323846f / 180.0f);

    #ifdef UI_IMPL_INVERT_ROTATION
        float cosr = cosf(rad);
        float sinr = -sinf(rad); // invert rotation
    #else
        float cosr = cosf(rad);
        float sinr = sinf(rad);  // CW rotation by default
    #endif

    return (ui_transform){
        .m00 = cosr * scalex,
        .m01 = sinr * scaley,
        .tx  = offx,
        .m10 = -sinr * scalex,
        .m11 = cosr * scaley,
        .ty  = offy
    };
}

static inline ui_transform ui_off(ui_transform m, float dx, float dy) {
    m.tx += dx * m.m00 + dy * m.m01;
    m.ty += dx * m.m10 + dy * m.m11;
    return m;
}

static inline ui_transform ui_sca(ui_transform m, float sx, float sy) {
    m.m00 *= sx;  m.m01 *= sy;
    m.m10 *= sx;  m.m11 *= sy;
    return m;
}

static inline ui_transform ui_rot(ui_transform m, float deg_cw) {
    float rad = deg_cw * (3.14159265358979323846f / 180.0f);

#ifdef UI_IMPL_INVERT_ROTATION
    float cosr = cosf(rad);
    float sinr = sinf(rad);
#else
    float cosr = cosf(rad);
    float sinr = -sinf(rad);
#endif

    float m00 = m.m00 * cosr + m.m01 * sinr;
    float m01 = -m.m00 * sinr + m.m01 * cosr;
    float m10 = m.m10 * cosr + m.m11 * sinr;
    float m11 = -m.m10 * sinr + m.m11 * cosr;

    m.m00 = m00; m.m01 = m01;
    m.m10 = m10; m.m11 = m11;
    return m;
}

static inline ui_transform ui_mul(ui_transform p, ui_transform c) {
    ui_transform r;

    r.m00 = p.m00 * c.m00 + p.m01 * c.m10;
    r.m01 = p.m00 * c.m01 + p.m01 * c.m11;
    r.tx  = p.m00 * c.tx  + p.m01 * c.ty + p.tx;

    r.m10 = p.m10 * c.m00 + p.m11 * c.m10;
    r.m11 = p.m10 * c.m01 + p.m11 * c.m11;
    r.ty  = p.m10 * c.tx  + p.m11 * c.ty + p.ty;

    return r;
}

// ===========================
// Compile Time Transformation

// compile time fmod function
#define UI_FMOD_APPROX(x, m) \
    ( ((x) >= 0.0f) ? ((x) - (m) * (int)((x)/(m))) : ((x) - (m) * ((int)((x)/(m)) - 1)) )

#ifdef UI_IMPL_INVERT_ROTATION
// compile time deg to rad conversion
#define DEG_TO_RAD(deg_cw) \
    ((((UI_FMOD_APPROX((deg_cw), 360.0f) * 0.017453292519943295f) > 3.14159265f) ? \
        ((UI_FMOD_APPROX((deg_cw), 360.0f) * 0.017453292519943295f) - 6.28318531f) : \
        (((UI_FMOD_APPROX((deg_cw), 360.0f) * 0.017453292519943295f) < -3.14159265f) ? \
            ((UI_FMOD_APPROX((deg_cw), 360.0f) * 0.017453292519943295f) + 6.28318531f) : \
            (UI_FMOD_APPROX((deg_cw), 360.0f) * 0.017453292519943295f))))
#else
// compile time deg to rad conversion
#define UI_DEG_TO_RAD(deg_cw) \
    (((( -UI_FMOD_APPROX((deg_cw), 360.0f) * 0.017453292519943295f) > 3.14159265f) ? \
        ((-UI_FMOD_APPROX((deg_cw), 360.0f) * 0.017453292519943295f) - 6.28318531f) : \
        (((-UI_FMOD_APPROX((deg_cw), 360.0f) * 0.017453292519943295f) < -3.14159265f) ? \
            ((-UI_FMOD_APPROX((deg_cw), 360.0f) * 0.017453292519943295f) + 6.28318531f) : \
            (-UI_FMOD_APPROX((deg_cw), 360.0f) * 0.017453292519943295f))))
#endif

// compile time sin approx (x - rad)
#define UI_SIN_APPROX(x) ((x) \
  - ((x)*(x)*(x))/6.0 \
  + ((x)*(x)*(x)*(x)*(x))/120.0 \
  - ((x)*(x)*(x)*(x)*(x)*(x)*(x))/5040.0 \
  + ((x)*(x)*(x)*(x)*(x)*(x)*(x)*(x)*(x))/362880.0 \
  - ((x)*(x)*(x)*(x)*(x)*(x)*(x)*(x)*(x)*(x)*(x))/39916800.0)

// compile time cos approx (x - rad)
#define UI_COS_APPROX(x) (1.0 \
  - ((x)*(x))/2.0 \
  + ((x)*(x)*(x)*(x))/24.0 \
  - ((x)*(x)*(x)*(x)*(x)*(x))/720.0 \
  + ((x)*(x)*(x)*(x)*(x)*(x)*(x)*(x))/40320.0 \
  - ((x)*(x)*(x)*(x)*(x)*(x)*(x)*(x)*(x)*(x))/3628800.0)

// compile time tranformation matrix builder
// transformation order: rotate, scale, translate
#define UI_TRANS(offx, offy, scax, scay, deg_cw) \
    (ui_transform){ \
        .m00 = UI_COS_APPROX(UI_DEG_TO_RAD(deg_cw)) * (scax), \
        .m01 = -UI_SIN_APPROX(UI_DEG_TO_RAD(deg_cw)) * (scay), \
        .tx  = (offx), \
        .m10 = UI_SIN_APPROX(UI_DEG_TO_RAD(deg_cw)) * (scax), \
        .m11 = UI_COS_APPROX(UI_DEG_TO_RAD(deg_cw)) * (scay), \
        .ty  = (offy) \
    }

#endif

/*
    Implementation
*/

#ifdef UI_IMPL

static inline void calculate_spacer_space(
    size_t         children_count,
    const ui_node* children,
    float*         total_spacer_space, 
    size_t*        flexible_count
) {
    float  spacer_width = 0.0f;
    size_t flexibles    = 0;

    for (size_t i = 0; i < children_count; i++) {
        const ui_node* child = &children[i];
        if (child->type == ui_node_spacer) spacer_width += (((ui_spacer_data*)(child->data))->precent * 2.0f);
        else flexibles++;
    }

    *total_spacer_space = spacer_width;
    *flexible_count     = flexibles;
}

static inline size_t calculate_proportion_sum(
    size_t               children_count,
    const ui_proportion* proportions
) {
    size_t sum = 0;
    for (size_t i = 0; i < children_count; i++) sum += proportions[i];
    return sum;
}

static const ui_row_data row_data_default = {
    .allign      = ui_row_allign_left,
    .proportions = 0x0,
    .spacing     = 0
};

static void draw_dispatch(const ui_node* node, ui_transform world, void* ctx);

static inline void draw_row(const ui_node* node, ui_transform world, void* ctx) {
    const ui_row_data* data = node->data ? node->data : &row_data_default;

    float x_left  = -1.0f;
    float x_right =  1.0f;
    float remaining_space = (x_right - x_left);

    // Exclude spacer space
    float  total_spacer; size_t flexible_count;
    calculate_spacer_space(node->child_count, node->children, &total_spacer, &flexible_count);
    remaining_space -= total_spacer;

    // Exclue spacing space
    float spacing_total = (flexible_count == 0) ? 0.0f : data->spacing * (flexible_count - 1) * 2.0f;
    remaining_space -= spacing_total;

    // Calculate proportions sum
    size_t proportion_sum = data->proportions ? 
        calculate_proportion_sum(node->child_count, data->proportions) : 
        flexible_count;

    // Alignment offset
    float content_width = remaining_space + spacing_total;
    float align_offset = 0.0f;

    switch (data->allign) {
        case ui_row_allign_center:
            align_offset = ((x_right - x_left) - total_spacer - content_width) * 0.5f;
            break;

        case ui_row_allign_right:
            align_offset = ((x_right - x_left) - total_spacer - content_width);
            break;

        default: break;
    }

    x_left += align_offset;

    // Layout children
    size_t flex_index = 0;
    for (size_t i = 0; i < node->child_count; i++) {
        const ui_node* child = &node->children[i];

        if (child->type == ui_node_spacer) {
            x_left += ((ui_spacer_data*)(child->data))->precent * 2.0f;
            continue;
        }

        float child_width;
        if (data->proportions) child_width = remaining_space * ((float)data->proportions[flex_index] / proportion_sum);
        else                   child_width = remaining_space / flexible_count;

        float child_center = x_left + child_width * 0.5f;

        ui_transform local = ui_default_trans;
        local = ui_off(local, child_center, 0.0f);
        local = ui_sca(local, child_width * 0.5f, 1.0f);

        draw_dispatch(child, ui_mul(world, local), ctx);

        x_left += child_width + data->spacing * 2.0f;
        flex_index++;
    } 
}

static const ui_column_data column_data_default = {
    .allign      = ui_column_allign_top,
    .proportions = 0x0,
    .spacing     = 0
};

static inline void draw_column(const ui_node* node, ui_transform world, void* ctx) {
    const ui_column_data* data = node->data ? node->data : &column_data_default;

    float y_top    =  1.0f;
    float y_bottom = -1.0f;
    float remaining_space = (y_top - y_bottom);

    // Exclude spacer space
    float total_spacer; size_t flexible_count;
    calculate_spacer_space(node->child_count, node->children, &total_spacer, &flexible_count);
    remaining_space -= total_spacer;

    // Exclude spacing space
    float spacing_total = (flexible_count == 0) ? 0.0f : data->spacing * (flexible_count - 1) * 2.0f;
    remaining_space -= spacing_total;

    // Calculate proportions sum
    size_t proportion_sum = data->proportions ?
        calculate_proportion_sum(node->child_count, data->proportions) :
        flexible_count;

    // Alignment offset
    float content_height = remaining_space + spacing_total;
    float align_offset = 0.0f;

    switch (data->allign) {
        case ui_column_allign_center:
            align_offset = ((y_top - y_bottom) - total_spacer - content_height) * 0.5f;
            break;

        case ui_column_allign_bottom:
            align_offset = ((y_top - y_bottom) - total_spacer - content_height);
            break;

        default: break;
    }

    y_top -= align_offset;

    // Layout children
    size_t flex_index = 0;

    for (size_t i = 0; i < node->child_count; i++) {
        const ui_node* child = &node->children[i];

        if (child->type == ui_node_spacer) {
            y_top -= ((ui_spacer_data*)(child->data))->precent * 2.0f;
            continue;
        }

        float child_height;
        if (data->proportions)
            child_height = remaining_space * ((float)data->proportions[flex_index] / proportion_sum);
        else
            child_height = remaining_space / flexible_count;

        float child_center = y_top - child_height * 0.5f;

        ui_transform local = ui_default_trans;
        local = ui_off(local, 0.0f, child_center);
        local = ui_sca(local, 1.0f, child_height * 0.5f);

        draw_dispatch(child, ui_mul(world, local), ctx);

        y_top -= child_height + data->spacing * 2.0f;
        flex_index++;
    }
}

static void draw_dispatch(const ui_node* node, ui_transform world, void* ctx) {
    switch (node->type) {
        // Transfrom
        case ui_node_render_transform: {
            ui_transform new_world = ui_mul(world, *(ui_transform*)(node->data));
            for (size_t i = 0; i < node->child_count; i++) {
                const ui_node* child = &node->children[i];
                draw_dispatch(child, new_world, ctx);
            }
        } break;

        // Primitives
        case ui_node_box: ui_draw_box(&world, node->data, ctx); goto draw_primitive_children;   
        case ui_node_img: ui_draw_img(&world, node->data, ctx); goto draw_primitive_children;
        case ui_node_txt: ui_draw_txt(&world, node->data, ctx); goto draw_primitive_children;
        case ui_node_geo: ui_draw_geo(&world, node->data, ctx); goto draw_primitive_children;

        // draw all children with same transformation
        draw_primitive_children:
            for (size_t i = 0; i < node->child_count; i++) {
                const ui_node* child = &node->children[i];
                draw_dispatch(child, world, ctx);
            }
        break;

        // Panels
        case ui_node_row:    draw_row   (node, world, ctx); break;
        case ui_node_column: draw_column(node, world, ctx); break;
    }
}

void ui_draw(const ui_node* node, void* ctx) {
    draw_dispatch(node, ui_default_trans, ctx); // begin rendering with full screen
}

#endif
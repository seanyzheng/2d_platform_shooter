#include "list.h"
#include "vector.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

typedef struct list {
  void **array;
  size_t capacity;
  size_t size;
  free_func_t free_func;
} list_t;

typedef void (*free_func_t)(void *);

list_t *list_init(size_t initial_size, free_func_t freer) {
  list_t *list = malloc(sizeof(list_t));
  assert(list != NULL);
  list->free_func = freer;
  if (initial_size == 0) {
    initial_size = 1;
  }
  list->capacity = initial_size;
  list->size = 0;
  list->array = malloc(list->capacity * sizeof(void **));
  assert(list->array != NULL);
  return list;
}

list_t *rect_init(double width, double height) {
  vector_t half_width = {.x = width / 2, .y = 0.0},
           half_height = {.x = 0.0, .y = height / 2};
  list_t *rect = list_init(4, free);
  vector_t *v = malloc(sizeof(*v));
  *v = vec_subtract(half_height, half_width);
  list_add(rect, v);
  v = malloc(sizeof(*v));
  *v = vec_subtract(vec_negate(half_width), half_height);
  list_add(rect, v);
  v = malloc(sizeof(*v));
  *v = vec_subtract(half_width, half_height);
  list_add(rect, v);
  v = malloc(sizeof(*v));
  *v = vec_add(half_width, half_height);
  list_add(rect, v);
  return rect;
}

list_t *circle_init(double radius, size_t points) {
  list_t *circle = list_init(points, free);
  double arc_angle = 2 * M_PI / points;
  vector_t point = {.x = radius, .y = 0.0};
  for (size_t i = 0; i < points; i++) {
    vector_t *v = malloc(sizeof(*v));
    *v = point;
    list_add(circle, v);
    point = vec_rotate(point, arc_angle);
  }
  return circle;
}

list_t *polygon_init(double radius, size_t num_of_points) {
  list_t *polygon = list_init(num_of_points, free);
  double arc_angle = 2 * M_PI / num_of_points;
  vector_t point = {.x = radius, .y = 0.0};
  for (size_t i = 0; i < num_of_points; i++) {
    vector_t *v = malloc(sizeof(*v));
    *v = point;
    list_add(polygon, v);
    point = vec_rotate(point, arc_angle);
  }
  return polygon;
}

void list_free(list_t *list) {
  if (list->free_func != NULL) {
    for (size_t i = 0; i < list->size; i += 1) {
      list->free_func(list->array[i]);
    }
  }
  free(list->array);
  free(list);
}

void list_resize(list_t *list) {
  if (list->size >= list->capacity) {

    void **new_array = malloc(list->capacity * 2 * sizeof(void *));
    for (size_t i = 0; i < list->size; i += 1) {
      void *old = list_get(list, i);
      new_array[i] = old;
    }

    free(list->array);
    list->array = new_array;
    list->capacity *= 2;
  }
}

size_t list_size(list_t *list) { return list->size; }

void *list_get(list_t *list, size_t index) {
  assert(index < list->size);
  return list->array[index];
}

void *list_remove(list_t *list, size_t index) {
  assert(list->size > 0);
  assert(index < list->size);

  void *temp = list->array[index];

  for (size_t i = index; i < list->size - 1; i++) {
    list->array[i] = list->array[i + 1];
  }
  list->size--;
  return temp;
}

void list_add(list_t *list, void *value) {
  assert(value != NULL);
  list_resize(list);
  list->size++;
  list->array[list->size - 1] = value;
}

void list_printer(list_t *list) {
  printf("|| %zu, %zu || \n", list->size, list_size(list));

  for (size_t i = 0; i < list->size; i++) {
    double x = ((vector_t *)list->array[i])->x;
    double y = ((vector_t *)list->array[i])->y;
    printf("(%zu) x: %f, y: %f\n", i + 1, x, y);
  }
  printf("---------------------------------\n");
}
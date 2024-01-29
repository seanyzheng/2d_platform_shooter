#include "collision.h"
#include <math.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

collision_info_t find_collision(list_t *shape1, list_t *shape2) {

  collision_info_t collision = {false};
  double min_overlap = INFINITY;

  // looping through all edges of shape 1
  for (size_t i = 0; i < list_size(shape1); i++) {
    size_t point_idx = i;
    size_t next_point_idx = (i + 1) % list_size(shape1);
    vector_t edge = vec_subtract(*(vector_t *)list_get(shape1, point_idx),
                                 *(vector_t *)list_get(shape1, next_point_idx));
    vector_t perpendicular_axis = {edge.y, -edge.x};
    perpendicular_axis = vec_unit_vector(perpendicular_axis);

    // calculate projection for shape1
    double min1 = INFINITY;
    double max1 = -INFINITY;
    for (size_t j = 0; j < list_size(shape1); j++) {
      double point =
          vec_dot(*(vector_t *)list_get(shape1, j), perpendicular_axis);
      if (point < min1) {
        min1 = point;
      }
      if (point > max1) {
        max1 = point;
      }
    }

    // calculate projection for shape2
    double min2 = INFINITY;
    double max2 = -INFINITY;
    for (size_t j = 0; j < list_size(shape2); j++) {
      double point =
          vec_dot(*(vector_t *)list_get(shape2, j), perpendicular_axis);
      if (point < min2) {
        min2 = point;
      }
      if (point > max2) {
        max2 = point;
      }
    }

    if ((min1 > max2 || min2 > max1)) { // No collision
      return collision;
    }

    double overlap = fmin(max2 - min1, max1 - min2);

    if (overlap < min_overlap) {
      collision.axis = perpendicular_axis;
      min_overlap = overlap;
    }
  }

  // looping through all edges of shape 2
  for (size_t i = 0; i < list_size(shape2); i++) {
    size_t point_idx = i;
    size_t next_point_idx = (i + 1) % list_size(shape2);
    vector_t edge = vec_subtract(*(vector_t *)list_get(shape2, point_idx),
                                 *(vector_t *)list_get(shape2, next_point_idx));
    vector_t perpendicular_axis = {edge.y, -edge.x};
    perpendicular_axis = vec_unit_vector(perpendicular_axis);

    // calculate projection for shape1
    double min1 = INFINITY;
    double max1 = -INFINITY;
    for (size_t j = 0; j < list_size(shape1); j++) {
      double point =
          vec_dot(*(vector_t *)list_get(shape1, j), perpendicular_axis);
      if (point < min1) {
        min1 = point;
      }
      if (point > max1) {
        max1 = point;
      }
    }

    double min2 = INFINITY;
    double max2 = -INFINITY;
    for (size_t j = 0; j < list_size(shape2); j++) {
      double point =
          vec_dot(*(vector_t *)list_get(shape2, j), perpendicular_axis);
      if (point < min2) {
        min2 = point;
      }
      if (point > max2) {
        max2 = point;
      }
    }

    if ((min1 > max2 || min2 > max1)) { // No collision
      return collision;
    }

    double overlap = fmin(max2 - min1, max1 - min2);

    if (overlap < min_overlap) {
      collision.axis = perpendicular_axis;
      min_overlap = overlap;
    }
  }

  collision.collided = true;
  return collision;
}
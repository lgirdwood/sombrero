/*
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 *  Copyright (C) 2013 Liam Girdwood
 */

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <math.h>

#include "sombrero.h"
#include "local.h"

/*! \cond */
struct stack {
	unsigned int *data;
	unsigned int pos;
};

struct structure_info {
	struct smbrr_image *simage; /* structure significant image */
	struct smbrr_image *wimage; /* structure wavelet coefficients */

	struct stack *stack;
	struct structure *structure;

	unsigned int id;
	unsigned int pixel;
	float value;
};
/*! \cond */

static int create_object(struct smbrr_wavelet *w, unsigned int scale,
	struct structure *structure);

static inline int stack_pop(struct stack *s, unsigned int *data)
{
	if (!s->pos)
		return 0;

	*data = s->data[--s->pos];
	return 1;
}

static inline void stack_push(struct stack *s, unsigned int data)
{
	s->data[s->pos++] = data;
}

static int stack_init(struct stack *s, int size)
{
	s->data = calloc(sizeof(unsigned int), size);
	if (s->data == NULL)
		return -ENOMEM;

	s->pos = 0;
	return 0;
}

static inline void stack_free(struct stack *s)
{
	free(s->data);
}

static inline int stack_not_empty(struct stack *s)
{
	return s->pos;
}

static inline void structure_add_pixel(struct structure_info *info, int pixel)
{
	struct smbrr_image *simage = info->simage;
	struct smbrr_image *wimage = info->wimage;

	simage->s[pixel] = info->id;
	info->structure->size++;

	if (wimage->adu[pixel] > info->structure->max_value) {
		info->structure->max_value = wimage->adu[pixel];
		info->structure->max_pixel = pixel;
	}
}

/* check south pixels */
static int structure_detect_south(struct structure_info *info, int pixel,
		int new)
{
	struct smbrr_image *simage = info->simage;
	unsigned int y = pixel / simage->width;

	/* get minimum Y coord for structure */
	if (info->structure->minxY.y > y) {
		info->structure->minxY.y = y;
		info->structure->minxY.x = pixel % simage->width;
	}

	if (y > 0 && simage->s[pixel - simage->width] == 1) {

		if (!new)
			stack_push(info->stack, pixel - simage->width);

		return 1;
	}

	return 0;
}

/* check north pixels */
static int structure_detect_north(struct structure_info *info, int pixel,
		int new)
{
	struct smbrr_image *simage = info->simage;
	unsigned int y = pixel / simage->width;

	/* get maximum Y coord for structure */
	if (info->structure->maxxY.y < y) {
		info->structure->maxxY.y = y;
		info->structure->maxxY.x = pixel % simage->width;
	}

	if (y < simage->height - 1 &&
		simage->s[pixel + simage->width] == 1) {

		if (!new)
			stack_push(info->stack, pixel + simage->width);

		return 1;
	}

	return 0;
}

/* fill line with current structure ID */
static void structure_scan_line(struct structure_info *info, int pixel)
{
	struct smbrr_image *simage = info->simage;
	int x, y, start, end, line_pixel, news, newn;

	/* calculate line limits */
	x = pixel % simage->width;
	y = pixel / simage->width;
	start = pixel - x;
	end = start + simage->width;

	/* get initial y values */
	if (info->structure->maxxY.y < y) {
		info->structure->maxxY.y = y;
		info->structure->maxxY.x = x;
	}
	if (info->structure->minxY.y > y) {
		info->structure->minxY.y = y;
		info->structure->minxY.x = x;
	}

	/* scan west */
	news = newn = 0;
	for (line_pixel = pixel - 1; line_pixel >= start; line_pixel--) {

		if (simage->s[line_pixel] != 1)
			break;

		structure_add_pixel(info, line_pixel);
		news = structure_detect_south(info, line_pixel, news);
		newn = structure_detect_north(info, line_pixel, newn);
	}

	/* get minimum X coord for structure */
	x = line_pixel % simage->width;
	if (info->structure->minXy.x > x) {
		info->structure->minXy.x = x;
		info->structure->minXy.y = y;
	}

	/* scan east */
	news = newn = 0;
	for (line_pixel = pixel + 1; line_pixel < end; line_pixel++) {

		if (simage->s[line_pixel] != 1)
					break;

		structure_add_pixel(info, line_pixel);
		news = structure_detect_south(info, line_pixel, news);
		newn = structure_detect_north(info, line_pixel, newn);
	}

	/* get maximum X coord for structure */
	x = line_pixel % simage->width;
	if (info->structure->maxXy.x < x) {
		info->structure->maxXy.x = x;
		info->structure->maxXy.y = y;
	}

}

static void structure_detect_pixels(struct structure_info *info)
{
	struct stack *stack = info->stack;
	unsigned int pixel;

	stack_push(stack, info->pixel);

	/* add pixels from stack to structure */
	while (stack_not_empty(stack)) {

		stack_pop(stack, &pixel);

		structure_add_pixel(info, pixel);
		structure_scan_line(info, pixel);
	}
}

/*! \fn int smbrr_wavelet_structure_find(struct smbrr_wavelet *w,
	unsigned int scale)
* \param w Wavelet
* \param scale Scale to be searched.
* \return Number of structures found within this wavelet scale.
*
* Search this wavelet scale for any structures that could be part of an object.
*/
int smbrr_wavelet_structure_find(struct smbrr_wavelet *w, unsigned int scale)
{
	struct structure_info info;
	struct stack stack;
	int size, err;

	info.wimage = w->w[scale];
	info.simage = w->s[scale];

	size = info.wimage->size;
	err = stack_init(&stack, size);
	if (err < 0)
		return -ENOMEM;

	info.stack = &stack;

	w->num_structures[scale] = 1;
	info.id = 1;

	/* check pixel by pixel */
	for (info.pixel = 0; info.pixel < size; info.pixel++) {

		/* is pixel significant */
		if (info.simage->s[info.pixel] == 1) {
			info.id++;

			/* new structure detected */
			w->structure[scale] = realloc(w->structure[scale],
					w->num_structures[scale] * sizeof(struct structure));
			if (w->structure[scale] == NULL)
				goto err;

			/* fill structure pixels with ID */
			info.structure =
				w->structure[scale] + w->num_structures[scale] - 1;
			memset(info.structure, 0, sizeof(struct structure));

			info.structure->scale = scale;
			info.structure->id = info.id - 2;
			info.structure->minxY.y = w->height;
			info.structure->minXy.x = w->width;

			structure_detect_pixels(&info);
			w->num_structures[scale]++;
		}
	}

	stack_free(&stack);

	w->num_structures[scale] -= 1;
	return w->num_structures[scale];

err:
	return -ENOMEM;
}

/* find structure at pixel on scale */
static struct structure *find_root_structure(struct smbrr_wavelet *w,
		unsigned int root_scale, unsigned int pixel)
{
	struct smbrr_image *simage = w->s[root_scale];
	struct structure *s = w->structure[root_scale];
	int id = simage->s[pixel];

	/* is there any structure at this pixel ? */
	if (id < 2)
		return NULL;

	/* return structure at this pixel - IDs start at 1 */
	return s + id - 2;
}

/* prune structures within 8 pixels (scale 3) of image edges to
 * remove unwanted edge artifacts */
static void prune_structure(struct smbrr_wavelet *w,
	unsigned int scale, struct structure *structure)
{
	int x, y, pixel;

	pixel = structure->max_pixel;
	x = pixel % w->width;
	y = pixel / w->width;

	if (x < 8 || x > w->width - 8 || y < 8 || y > w->height - 8)
		structure->pruned = 1;
}

/* connect a single structure to a struct on scale + 1 */
static int connect_structure_to_root(struct smbrr_wavelet *w,
	unsigned int root_scale, struct structure *structure)
{
	struct structure *root;

	/* ignore any pruned structures */
	if (structure->pruned)
		return 0;

	/* find connected structure at new scale */
	root = find_root_structure(w, root_scale, structure->max_pixel);
	if (root == NULL)
		return 0;

	/* ignore any pruned structures */
	if (root->pruned)
		return 0;

	/* add branch and root structures */
	structure->root = root->id;
	structure->has_root = 1;

	root->num_branches++;
	root->branch = realloc(root->branch,
		root->num_branches * sizeof(unsigned int));
	if (root->branch == NULL)
		return -ENOMEM;

	root->branch[root->num_branches - 1] = structure->id;
	return 0;
}

static float structure_get_distance(struct smbrr_wavelet *w,
		struct structure *structure1,
		struct structure *structure2)
{
	struct smbrr_image *image1, *image2;
	float x, y;

	image1 = w->s[structure1->scale];
	image2 = w->s[structure2->scale];

	x = image_get_x(image1, structure1->max_pixel) -
			image_get_x(image1, structure2->max_pixel);
	y = image_get_y(image2, structure1->max_pixel) -
			image_get_y(image2, structure2->max_pixel);

	x *= x;
	y *= y;

	return sqrtf(x + y);
}

/* get branch closest to structure max value pixel */
struct structure *structure_get_closest_branch(struct smbrr_wavelet *w,
	int scale, struct structure *structure)
{
	struct structure *s, *closest, *branch;
	float dist = 1.0e6, new_dist;
	int i;

	s = w->structure[scale - 1];
	closest = s + structure->branch[0];

	for (i = 0; i < structure->num_branches; i++) {

		branch = s + structure->branch[i];
		new_dist = structure_get_distance(w, structure, branch);

		if (new_dist < dist) {
			dist = new_dist;
			closest = branch;
		}
	}

	return closest;
}

static struct structure *structure_is_root(struct smbrr_wavelet *w,
	struct structure *structure, struct structure *root)
{
	struct smbrr_image *simage = w->s[structure->scale];
	struct smbrr_image *sroot = w->s[root->scale];
	unsigned int x, y, pixel, sid, rid;

	sid = structure->id + 2;
	rid = root->id + 2;

	for (x = structure->minXy.x; x <= structure->maxXy.x; x++) {
		for (y = structure->minxY.y; y <= structure->maxxY.y; y++) {

			pixel = simage->width * y + x;

			if (simage->s[pixel] == sid &&
					sroot->s[pixel] == rid) {
				if (pixel == root->max_pixel)
					return structure;
			}
		}
	}

	return NULL;
}

static int object_create_image(struct smbrr_wavelet *w, struct object *object)
{
	int width, height, ret;

	width = object->o.maxXy.x - object->o.minXy.x + 1;
	height = object->o.maxxY.y - object->o.minxY.y + 1;

	object->image = smbrr_image_new(SMBRR_IMAGE_FLOAT, width, height, 0,
		SMBRR_ADU_8, NULL);
	if (object->image == NULL)
		return -ENOMEM;

	ret = smbrr_wavelet_deconvolution_object(w, w->conv_type,
		w->mask_type, &object->o);

	return ret;
}

static void object_get_bounds(struct smbrr_wavelet *w,
	struct object *object)
{
	struct structure *structure;
	unsigned int minX = 2147483647, minY = 2147483647, maxX = 0, maxY = 0;
	int i;

	object->o.max_adu = 0.0;

	/* get peak object scale and object limits */
	for (i = object->start_scale; i <= object->end_scale; i++) {

		structure = w->structure[i] + object->structure[i];

		/* get peak scale and position of peak pixel */
		if (structure->max_value > object->o.max_adu) {
			object->o.max_adu = structure->max_value;
			object->o.scale = i;
		} else
			continue;

		/* get limits */
		if (minX > structure->minXy.x) {
			minX = structure->minXy.x;
			object->o.minXy = structure->minXy;
		}

		if (minY > structure->minxY.y) {
			minY = structure->minxY.y;
			object->o.minxY = structure->minxY;
		}

		if (maxX < structure->maxXy.x) {
			maxX = structure->maxXy.x;
			object->o.maxXy = structure->maxXy;
		}

		if (maxY < structure->maxxY.y) {
			maxY = structure->maxxY.y;
			object->o.maxxY = structure->maxxY;
		}
	}
}

static void object_get_position(struct smbrr_wavelet *w,
	struct object *object)
{
	struct structure *structure;
	struct smbrr_image *wimage;

	/* get object position */
	wimage = w->w[object->o.scale];
	structure = w->structure[object->o.scale] +
		object->structure[object->o.scale];

	object->o.pos.x = structure->max_pixel % wimage->width;
	object->o.pos.y = structure->max_pixel / wimage->width;
}

static void object_get_sigma(struct smbrr_wavelet *w,
	struct object *object)
{
	int i;
	float sigma = 0.0, t;

	/* get sigma */
	for (i = 0; i < object->image->size; i++) {
		t = object->image->adu[i] - object->o.mean_adu;
		t *= t;
		sigma += t;
	}

	sigma /= object->o.area;
	object->o.sigma_adu = sqrtf(sigma);
}

static int object_get_area(struct smbrr_wavelet *w,
	struct object *object)
{
	int err, i;

	/* create image for this object */
	err = object_create_image(w, object);
	if (err < 0)
		return err;

	/* calculate total ADU and area for object */
	for (i = 0; i < object->image->size; i++) {
		if (object->image->adu[i] != 0.0) {
			object->o.total_adu += object->image->adu[i];
			object->o.area++;
		}
	}

	/* TODO: calculate PA based on max/min coords */
	object->o.mean_adu = object->o.total_adu / object->o.area;

	return 0;
}

static int object_calc_data(struct smbrr_wavelet *w)
{
	struct object *object;
	int err, i;

	for (i = 0; i < w->num_objects; i++) {
		object = &w->objects[i];

		object_get_bounds(w, object);

		err = object_get_area(w, object);
		if (err < 0)
			return err;

		object_get_position(w, object);

		object_get_sigma(w, object);
	}

	return 0;
}

static void object_calc_mag_delta(struct smbrr_wavelet *w,
	struct object *object)
{
	/* get magnitude delta to brightest object detected */
	if (object == &w->objects[0])
		object->o.mag_delta = 0.0;
	else
		object->o.mag_delta = -2.5 *
			log10(object->o.total_adu / w->objects[0].o.total_adu);
}

static void object_calc_data2(struct smbrr_wavelet *w)
{
	struct object *object;
	int i;

	for (i = 0; i < w->num_objects; i++) {
		object = &w->objects[i];

		object_calc_mag_delta(w, object);
	}
}

static struct object *new_object(struct smbrr_wavelet *w,
	unsigned int scale, struct structure *structure)
{
	struct object *object;

	w->objects = realloc(w->objects, sizeof(*object) * ++w->num_objects);
	if (w->objects == NULL)
		return NULL;

	object = &w->objects[w->num_objects - 1];
	memset(object, 0, sizeof(*object));
	object->end_scale = scale;
	object->o.id = w->num_objects - 1;

	return object;
}

static int object_cmp(const void *o1, const void *o2)
{
	const struct object *object1 = o1, *object2 = o2;

	if (object1->o.total_adu < object2->o.total_adu)
		return 1;
	else if (object1->o.total_adu > object2->o.total_adu)
		return -1;
	else
		return 0;
}

/*
 * An object is a set of connected structures at different scales. The low
 * resolution scale structures that make an object are the roots, whilst the
 * higher resolution structures are the branches.  Each structure can have a
 * single low resolution root structure and several branches from higher
 * resolution scales. TODO: add more logic here to detect more object types.
 */
static int create_object(struct smbrr_wavelet *w, unsigned int scale,
	struct structure *structure)
{
	struct object *object;
	struct structure *root = NULL, *closest_branch, *branch;
	int i, err, bscale = scale - 1, rscale = scale + 1;

	/* make sure we are not part of an object already */
	if (structure->merged)
		return 0;

	/* ignore pruned structures */
	if (structure->pruned)
		return 0;

	/* disconnected structures do not form objects */
	if (structure->num_branches == 0 && !structure->has_root)
		return 0;

	/* do we need to allocate new object */
	if (structure->has_root) {
		root = w->structure[rscale] + structure->root;
		object = &w->objects[root->object_id];
	} else {
		object = new_object(w, scale, structure);
		if (object == NULL)
			return -EINVAL;
		object->end_scale = scale;
	}

	/* assign structure to object */
	object->structure[scale] = structure->id;
	structure->object_id = object->o.id;
	structure->merged = 1;
	object->start_scale = scale;

	/* is this the highest resolution for this structure and object ? */
	if (structure->num_branches == 0)
		return 0;

	/* Get the closest branch to this structure based on the distance between
	 * the maximum pixel positions. Then check if the closest branch contains
	 * the maximum pixel for root structure. */
	closest_branch = structure_get_closest_branch(w, scale, structure);
	if (closest_branch != NULL)
		closest_branch = structure_is_root(w, closest_branch, structure);

	/* determine if object terminates here of other branches */
	/* object spans to next scale with 1 or more branches */
	for (i = 0; i < structure->num_branches; i++) {
		branch = w->structure[bscale] + structure->branch[i];

		/* dont create new object if branch is closest */
		if (branch != closest_branch)
			branch->has_root = 0;

		/* create a new object with branch at new scale */
		err = create_object(w, bscale, branch);
		if (err < 0)
			return err;
	}

	return 0;
}

static int prune_objects(struct smbrr_wavelet *w)
{
	struct object *object, *base, *nobject;
	int i, count = 0;

	for (i = 0; i < w->num_objects; i++) {
		object = &w->objects[i];

		if (object->start_scale == object->end_scale) {
			object->pruned = 1;
			count++;
		}
	}

	if (count == 0)
		return 0;

	base = calloc(w->num_objects - count, sizeof(*object));
	if (base == NULL)
		return -ENOMEM;

	nobject = base;

	for (i = 0; i < w->num_objects; i++) {
			object = &w->objects[i];

			if (object->pruned)
				continue;

			memcpy(nobject, object, sizeof(*object));
			nobject++;
	}

	free(w->objects);
	w->objects = base;
	w->num_objects -= count;
	return 0;
}

/*! \fn int smbrr_wavelet_structure_connect(struct smbrr_wavelet *w,
		unsigned int start_scale, unsigned int end_scale)
* \param w Wavelet
* \param start_scale Starting wavelet scale.
* \param end_scale Last wavelet scale.
* \return Number of Objects found within this wavelet scale range.
*
* Search the wavelet scales for any objects.
*/
int smbrr_wavelet_structure_connect(struct smbrr_wavelet *w,
		unsigned int start_scale, unsigned int end_scale)
{
	struct structure *structure;
	int err = 0, scale, i, start = start_scale, end = end_scale, ret;

	/* make sure we dont check scales after last */
	if (end_scale > w->num_scales - 1)
		return 0;

	/* prune structures near edges */
	for (scale = start; scale <= end; scale++) {

		/* get first structure at this scale */
		structure = w->structure[scale];

		/* connect each structure */
		for (i = 0; i < w->num_structures[scale]; i++)
			prune_structure(w, scale, &structure[i]);
	}

	/* connect structures at each scale */
	for (scale = start; scale < end; scale++) {

		/* get first structure at this scale */
		structure = w->structure[scale];

		/* connect each structure */
		for (i = 0; i < w->num_structures[scale]; i++) {
			err = connect_structure_to_root(w, scale + 1, &structure[i]);
			if (err < 0)
				return err;
		}
	}

	/* create new objects and deblend connected structures */
	for (scale = end; scale >= start; scale--) {

			/* get first structure at this scale */
			structure = w->structure[scale];

			/* connect each structure */
			for (i = 0; i < w->num_structures[scale]; i++) {
				err = create_object(w, scale, structure + i);
				if (err < 0)
					return err;
			}
	}

	/* prune objects */
	ret = prune_objects(w);
	if (ret < 0)
		return ret;

	/* calculate total adu for each object */
	ret = object_calc_data(w);
	if (ret < 0)
		return ret;

	/* sort objects into ascending order of total adu */
	qsort(w->objects, w->num_objects,
			sizeof(struct object), object_cmp);

	/* calculate total adu for each object */
	object_calc_data2(w);

	return w->num_objects;
}

/*! \fn struct smbrr_onbject *smbrr_wavelet_object_get(struct smbrr_wavelet *w,
	unsigned int object_id)
* \param w Wavelet
* \param object_id ID of object to retreive.
* \return Object or NULL.
*
* Get wavelet object by ID. Objects are ordered on brightness.
*/
struct smbrr_object *smbrr_wavelet_object_get(struct smbrr_wavelet *w,
	unsigned int object_id)
{
	if (object_id >= w->num_objects)
		return NULL;

	return &w->objects[object_id].o;
}

/*! \fn void smbrr_wavelet_object_free_all(struct smbrr_wavelet *w)
* \param w Wavelet.
*
* Free all objects, object images and structures.
*/
void smbrr_wavelet_object_free_all(struct smbrr_wavelet *w)
{
	struct object *object;
	struct structure *structure;
	int i, j;

	for (i = 0; i < w->num_objects; i++) {
		object = &w->objects[i];
		smbrr_image_free(object->image);
	}
	free(&w->objects[0]);

	for (i = 0; i < w->num_scales - 1; i++) {
		for (j = 0; j < w->num_structures[i]; j++) {
			structure = w->structure[i] + j;
			free(structure->branch);
		}
		free(w->structure[i]);
	}
}

/*! \fn int smbrr_wavelet_object_get_image(struct smbrr_wavelet *w,
		struct smbrr_object *object, struct smbrr_image **image)
* \param w Wavelet.
*
* Free all objects, object images and structures.
*/
int smbrr_wavelet_object_get_image(struct smbrr_wavelet *w,
		struct smbrr_object *object, struct smbrr_image **image)
{
	struct object *o = (struct object *)object;
	int width, height, ret;

	if (o->image) {
		*image = o->image;
		return 0;
	}

	width = object->maxXy.x - object->minXy.x + 1;
	height = object->maxxY.y - object->minxY.y + 1;

	o->image = smbrr_image_new(SMBRR_IMAGE_FLOAT, width, height, 0,
		SMBRR_ADU_8, NULL);
	if (o->image == NULL)
		return -ENOMEM;

	ret = smbrr_wavelet_deconvolution_object(w, w->conv_type,
		w->mask_type, object);

	return ret;
}

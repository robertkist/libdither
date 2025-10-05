#include "tetrapal.h"
#include <math.h>

/* Allocator functions. */
#if defined(TETRAPAL_MALLOC) && defined(TETRAPAL_REALLOC) && defined(TETRAPAL_FREE)
	/* User has defined all of their own allocator functions. */
#elif !defined(TETRAPAL_MALLOC) && !defined(TETRAPAL_REALLOC) && !defined(TETRAPAL_FREE)
#include <stdlib.h> /* free(), malloc(), realloc(). */
#define TETRAPAL_MALLOC(size) malloc(size)
#define TETRAPAL_REALLOC(ptr, size) realloc(ptr, size)
#define TETRAPAL_FREE(ptr) free(ptr)
#else
#error "Either all or none of MALLOC, REALLOC, and FREE must be defined!"
#endif

/* Dev-only debug macro. */
//#define TETRAPAL_DEBUG
#ifdef TETRAPAL_DEBUG
#include <stdio.h>
#include <assert.h>
#define TETRAPAL_ASSERT(condition, message) assert(condition && message)
#else
#define TETRAPAL_ASSERT(condition, message)
#endif

#ifndef NULL
#define NULL ((void*)0)
#endif

/* Maximum value of any unsigned integer type. */
#define max_of_unsigned(T) ((T)(~(T)0)) 

/* True/false macro constants. */
#define true 1
#define false 0

/* Typedefs for internal use. */
typedef float coord_t; /* Type representing a floating-point coordinate. */
typedef signed long vertex_t; /* Type representing a global vertex index. */
typedef unsigned long simplex_t; /* Type representing the global simplex index. */
typedef unsigned char facet_t; /* Type representing the cavity facet global index. */
typedef unsigned char local_t; /* Type representing a local index. */
typedef signed long error_t; /* Type representing an error code. */
typedef unsigned char flags_t; /* Type representing a set of bit-flags. */
typedef unsigned long random_t; /* Type representing a random integer. */
typedef unsigned long long digit_t; /* Type representing a digit; used for exact airthmetic. */
typedef unsigned char bool; /* Type representing a boolean. */

/* Internal constants. */
static const vertex_t VERTEX_INFINITE = -1; /* Value representing the infinite vertex. */
static const simplex_t SIMPLEX_NULL = max_of_unsigned(simplex_t); /* Value representing a null or invalid simplex. */
static const facet_t FACET_NULL = max_of_unsigned(facet_t); /* Value representing a null or invalid facet. */
static const local_t LOCAL_NULL = max_of_unsigned(local_t); /* Value representing a null or invalid local index. */
static const random_t RANDOM_MAX = 0xffff; /* Maximum value of a randomly generated integer. */
static const double ARRAY_GROWTH_FACTOR = 1.618; /* Amount to resize arrays when capacity is reached. */
static const double CAVITY_TABLE_MAX_LOAD = 0.7; /* Maximum load factor of the cavity hash table. */
static const facet_t CAVITY_TABLE_FREE = max_of_unsigned(facet_t); /* Value representing a free element in the cavity hash table. */

/* Internal error codes. */
typedef enum
{
	ERROR_NONE,
	ERROR_OUT_OF_MEMORY,
	ERROR_INVALID_ARGUMENT,

} ErrorCode;

/* Internal hard-coded maximum value of a given coordinate.
	Input points are expected to be given in the range [0.0, 1.0], and are then scaled by this amount.
	This value has been chosen because it allows for accurate representation of linear sRGB values without loss of precision.
	Additionally, knowing the maximum possible bit length of a coordinate makes exact computation of geometric predicates simpler.
	Do NOT increase this value if you want the triangulation to remain robust. */
static const coord_t TETRAPAL_PRECISION = (1u << 16u) - 1u;

/********************************/
/*		Vector Maths			*/
/********************************/

/* Calculate the dot product of [a] and [b] in 3D. */
static inline coord_t dot_3d(const coord_t a[3], const coord_t b[3]);

/* Calculate the dot product of [a] and [b] in 2D. */
static inline coord_t dot_2d(const coord_t a[2], const coord_t b[2]);

/* Subtract [b] from [a] in 3D. */
static inline void sub_3d(const coord_t a[3], const coord_t b[3], coord_t result[3]);

/* Subtract [b] from [a] in 2D. */
static inline void sub_2d(const coord_t a[2], const coord_t b[2], coord_t result[2]);

/* Multiply the 3D vector [a] by the scalar [s]. */
static inline void mul_3d(const coord_t a[3], const coord_t s, coord_t result[3]);

/* Calculate the 3D cross product of [a] against [b]. */
static inline void cross_3d(const coord_t a[3], const coord_t b[3], coord_t result[3]);

/* Normalise [a] in 3D. */
static inline void normalise_3d(const coord_t a[3], coord_t result[3]);

/* Determine the circumcentre of the triangle [a, b, c] in 2D space. */
static void circumcentre_2d(const coord_t a[2], const coord_t b[2], const coord_t c[2], coord_t* result);

/* Determine the circumcentre of the tetrahedron [a, b, c, d]. */
static void circumcentre_3d(const coord_t a[3], const coord_t b[3], const coord_t c[3], const coord_t d[3], coord_t* result);

/* Get the midpoint between [a] and [b] in 2D space. */
static inline void midpoint_2d(const coord_t a[2], const coord_t b[2], coord_t result[2]);

/* Get the midpoint between [a] and [b] in 3D space. */
static inline void midpoint_3d(const coord_t a[3], const coord_t b[3], coord_t result[3]);

/* Calculate the squared distance between [a] and [b] in 1D space. */
static inline coord_t distance_squared_1d(const coord_t a[1], const coord_t b[1]);

/* Calculate the squared distance between [a] and [b] in 2D space. */
static inline coord_t distance_squared_2d(const coord_t a[2], const coord_t b[2]);

/* Calculate the squared distance between [a] and [b] in 3D space. */
static inline coord_t distance_squared_3d(const coord_t a[3], const coord_t b[3]);

/********************************/
/*		128-Bit Integer			*/
/********************************/

/* Simulation of a 128-bit signed integer type for higher precision integer arithmetic. */

typedef struct
{
	digit_t digits[2];
	char sign;

} int128_t;

/* Create a new zero-initialised int128. */
static inline int128_t int128_zero(void);

/* Create a new int128 from the product of two doubles. */
static inline int128_t int128_from_product(const double a, const double b);

/* Add two int128 types together. */
static inline int128_t int128_add(const int128_t a, const int128_t b);

/* Subtract two int128 types from each other. */
static inline int128_t int128_sub(const int128_t a, const int128_t b);

/* Return the absolute (positive) value of [a]. */
static inline int128_t int128_abs(const int128_t a);

/* Return the negative value of [a]. */
static inline int128_t int128_neg(const int128_t a);

/* Return the additive inverse of [a] (flip the sign if it is not zero). */
static inline int128_t int128_inv(const int128_t a);

/* Test whether the absolute value of [a] is less than the absolute value of [b]. */
static inline bool int128_lt_abs(const int128_t a, const int128_t b);

/********************************/
/*		Geometric Predicates	*/
/********************************/

/* 
	Robust geometric predicates are calculated by taking advantage of the fact that the
	input coordinates consist of integer values whose maximum representable bit length is 
	known in advance.

	Because of this, it is possible to determine conservative error bounds for each
	predicate before runtime, assuming arithmetic is performed using double precision floats.

	For most predicates the maximum error is 0, and no specialised exact arithmetic is ever needed.

	Otherwise, the error bounds acts as a static filter that only performs exact arithmetic
	when the magnitude of the approximate result exceeds the maximum error. 
*/

static const double MAX_ERROR_INCIRCLE = 73728.0;
static const double MAX_ERROR_INSPHERE = 51539607552.0;

/* Check if the 3D coordinates [a] and [b] are coincident. */
static inline bool is_coincident_3d(const coord_t a[3], const coord_t b[3]);

/* Check if the 3D coordinates [a], [b] and [c] are colinear. */
static bool is_colinear_3d(const coord_t a[3], const coord_t b[3], const coord_t c[3]);

/* Check if the 3D coordinates [a], [b], [c] and [d] are coplanar. */
static inline bool is_coplanar_3d(const coord_t a[3], const coord_t b[3], const coord_t c[3], const coord_t d[3]);

/* Evaluate the signed area of the triangle [a, b, c] in 2D space. */
static coord_t orient_2d(const coord_t a[2], const coord_t b[2], const coord_t c[2]);

/* Evaluate the signed volume of the tetrahedron [a, b, c, d] in 3D space. */
static coord_t orient_3d(const coord_t a[3], const coord_t b[3], const coord_t c[3], const coord_t d[3]);

/* Test whether the point [e] lies inside or outside the sphere circumscribing the positively oriented triangle. [a, b, c]. */
static coord_t incircle_2d(const coord_t a[2], const coord_t b[2], const coord_t c[2], const coord_t d[2]);

/* Test whether the point [d] lies inside or outside the sphere circumscribing the positively oriented tetrahedron. [a, b, c, d]. */
static coord_t insphere_3d(const coord_t a[3], const coord_t b[3], const coord_t c[3], const coord_t d[3], const coord_t e[3]);

#ifdef TETRAPAL_DEBUG
/* Return the bit-length of the absolute value of a. */
static inline size_t bit_length(const coord_t a);
#endif

/********************************/
/*		Stack					*/
/********************************/

typedef struct
{
	size_t count;
	size_t capacity;
	simplex_t* data;
} Stack;

/* Allocate and initialise stack data. */
static error_t stack_init(Stack* stack, size_t reserve);

/* Free all data allocated by the stack. */
static void stack_free(Stack* stack);

/* Clear all items from the stack. */
static void stack_clear(Stack* stack);

/* Insert an item in the stack. Returns non-zero on failure. */
static error_t stack_insert(Stack* stack, simplex_t t);

/* Check if the stack is at capacity, resizing if necessary. */
static error_t stack_check_capacity(Stack* stack);

/* Remove the item at the top of the stack. */
static void stack_pop(Stack* stack);

/* Get the item at the top of the stack. */
static simplex_t stack_top(const Stack* stack);

/* Check if the stack contains an item. */
static bool stack_contains(const Stack* stack, simplex_t t);

/* Check whether or not the stack is empty. */
static bool stack_is_empty(const Stack* stack);

/********************************/
/*		KD Tree					*/
/********************************/

/* Perform in-place balancing of a [k]-d tree given a buffer of vertex indices. */
static error_t kdtree_balance(Tetrapal* tetrapal, const size_t begin, const size_t end, const size_t depth);

/* Perform an approximate nearest-neighbour search for the given coordinate (i.e. return the first leaf node visited).
	This function returns the index of the node in the tree, NOT the vertex index itself! */
static size_t kdtree_find_approximate(const Tetrapal* tetrapal, const coord_t* p);

/* Return the vertex index at node [i] in the tree. */
static inline vertex_t kdtree_get_vertex(const Tetrapal* tetrapal, const size_t i);

/* Partially sort the buffer in the range [begin, end] inclusive so that the middle value is the median. */
static void kdtree_sort_median(Tetrapal* tetrapal, const size_t begin, const size_t end, const size_t depth);

#ifdef TETRAPAL_DEBUG
/* Print the KD Tree. */
static void kdtree_print(const Tetrapal* tetrapal);

/* Recursive helper function for printing the KD Tree. */
static void kdtree_print_recurse(const Tetrapal* tetrapal, size_t begin, size_t end, size_t depth);
#endif

/********************************/
/*		Cavity					*/
/********************************/

typedef struct
{
	struct
	{
		size_t count; /* Number of facets in the cavity. */
		size_t capacity; /* Capacity of the facet arrays. */
		vertex_t* incident_vertex; /* Facet index to incident vertex global index. */
		simplex_t* adjacent_simplex; /* Facet index to adjacent simplex global index. */
		local_t* boundary_facet; /* Facet index to adjacent simplex facet local index. */

	} facets;

	struct
	{
		size_t capacity; /* Capacity of the hash table. */
		size_t count; /* Number of elements in the table. */
		vertex_t* edge;
		facet_t* facet;
	} table;

} Cavity;

/* Allocate and initialise cavity data. */
static error_t cavity_init(Cavity* cavity, size_t reserve);

/* Free all data allocated by the cavity. */
static void cavity_free(Cavity* cavity);

/* Insert a facet [a, b, c] into the cavity adjacent to a boundary simplex [t] at local facet index [i]. */
static facet_t cavity_insert(Cavity* cavity, vertex_t a, vertex_t b, vertex_t c, simplex_t t, local_t i);

/* Check if the cavity is at capacity, resizing if necessary. */
static error_t cavity_check_capacity(Cavity* cavity);

/* Insert the directed edge [a, b] corresponding to facet [f] into the hash table.
	Returns non-zero on failure. */
static error_t cavity_insert_edge(Cavity* cavity, vertex_t a, vertex_t b, facet_t f);

/* Check if the cavity hash table is at capacity, resizing if necessary. */
static error_t cavity_table_check_capacity(Cavity* cavity);

/* Generate a hash given the directed egde [a, b]. */
static size_t cavity_edge_hash(vertex_t a, vertex_t b);

/* Reset the cavity data. */
static void cavity_clear(Cavity* cavity);

/* Return the facet keyed on the directed edge [a, b]. */
static facet_t cavity_find(Cavity* cavity, vertex_t a, vertex_t b);

/* Set [t] to be the simplex adjacent to the cavity facet [f]. */
static void cavity_set_adjacent_simplex(Cavity* cavity, facet_t f, simplex_t t);

/* Return the vertex incident to the facet [f] at local index [i]. */
static vertex_t cavity_get_incident_vertex(Cavity* cavity, facet_t f, local_t i);

/* Return the simplex adjacent to the facet [f]. */
static simplex_t cavity_get_adjacent_simplex(Cavity* cavity, facet_t f);

/* Get the local index of a facet [f] wrt the facet's adjacent boundary simplex. */
static local_t cavity_get_adjacent_simplex_facet(Cavity* cavity, facet_t f);

#ifdef TETRAPAL_DEBUG
/* Print all facet data. */
static void cavity_print_facet_data(Cavity* cavity);
#endif

/********************************/
/*		Tetrapal Core			*/
/********************************/

struct Tetrapal
{
	size_t dimensions; /* Number of dimensions spanned by the triangulation. */

	struct
	{
		size_t count; /* Number of vertices. */
		size_t capacity; /* Size of the vertex buffer. */
		coord_t basis[2][3]; /* 3D Coordinates representing the basis vectors for 2D and 1D embedded triangulations. */
		coord_t* coordinates; /* Vertex coordinates. */
		simplex_t* incident_simplex; /* Vertex global index to incident simplex global index. */
		vertex_t* tree; /* KD Tree of the coordinates in the triangulation. */

	} vertices;

	struct
	{
		size_t count; /* Number of simplices. */
		size_t capacity; /* Size of the simplex arrays. */
		vertex_t* incident_vertex; /* Simplex global index to incident vertex global index. */
		simplex_t* adjacent_simplex; /* Simplex global index to adjacent simplex global index.  */
		simplex_t last; /* The most recently created finite simplex. */

		/* Array of flags for every simplex. */
		union
		{
			flags_t all; /* Convenient union to clear all flags. */
			struct
			{
				flags_t
					is_free : 1, /* Whether the simplex has been freed/deleted. */
					is_infinite : 1; /* Whether the simplex is infinite. */
			} bit;
		} *flags;

		struct
		{
			size_t count; /* Number of deleted simplices. */
			size_t capacity; /* Size of the deleted simplices array. */
			simplex_t* simplices; /* Global indices of the deleted simplices. */
		} deleted;

	} simplices;

	Cavity cavity;
	Stack stack;
};

/* Log a new vertex in the triangulation. */
static vertex_t new_vertex(Tetrapal* tetrapal, const coord_t* p);

/* Frees an existing tetrahedron [t]. */
static error_t free_simplex(Tetrapal* tetrapal, simplex_t t);

/* Set the adjacent simplex [a] with respect to simplex [t] at local facet index [i]. */
static inline void set_adjacent_simplex(Tetrapal* tetrapal, simplex_t t, simplex_t a, local_t i);

/* Get the vertex incident to simplex [t] at local vertex index [i]. */
static inline vertex_t get_incident_vertex(const Tetrapal* tetrapal, simplex_t t, local_t i);

/* Get the simplex adjacent to simplex [t] at local facet index [i]. */
static inline simplex_t get_adjacent_simplex(const Tetrapal* tetrapal, simplex_t t, local_t i);

/* Get a simplex incident to vertex [v]. */
static inline simplex_t get_incident_simplex(const Tetrapal* tetrapal, vertex_t v);

/* Get the circumcentre for a given simplex [t]. */
static inline void get_circumcentre(const Tetrapal* tetrapal, simplex_t t, coord_t* result);

/* Get a const pointer to the coordinates of vertex [v]. */
static inline const coord_t* get_coordinates(const Tetrapal* tetrapal, vertex_t v);

/* Get the normal vector of the facet at [i] of simplex [t]. */
static void get_facet_normal(const Tetrapal* tetrapal, simplex_t t, local_t i, coord_t result[3]);

/* Get the local vertex index from [t] corresponding to the global vertex index [v]. */
static inline local_t find_vertex(const Tetrapal* tetrapal, simplex_t t, vertex_t v);

/* Get the local facet index from [t] that is shared by [adj]. */
static inline local_t find_adjacent(const Tetrapal* tetrapal, simplex_t t, simplex_t adj);

/* Get the local facet index from [t] via the directed edge [a, b]. */
static inline local_t find_facet_from_edge(const Tetrapal* tetrapal, simplex_t t, vertex_t a, vertex_t b);

/* Check whether or not a given simplex has an infinite vertex. */
static inline bool is_infinite_simplex(const Tetrapal* tetrapal, simplex_t t);

/* Check whether or not the simplex [t] has been freed/deleted. */
static inline bool is_free_simplex(const Tetrapal* tetrapal, simplex_t t);

/* Check whether a given point is coincident with one of the vertices of simplex [t]. */
static inline bool is_coincident_simplex(const Tetrapal* tetrapal, simplex_t t, const float point[3]);

/* Get the size of a simplex in the triangulation (dimensions + 1). */
static inline local_t simplex_size(const Tetrapal* tetrapal);

/* Check whether the simplex buffers need to be resized, reallocating if so.
	Returns non-zero on failure. */
static error_t check_simplices_capacity(Tetrapal* tetrapal);

/* Check whether the deleted simplex array needs to be resized, reallocating if so.
	Returns non-zero on failure. */
static error_t check_deleted_capacity(Tetrapal* tetrapal);

/* Find the first d-simplex to start the triangulation with. Returns the number of dimensions spanned by the point set. */
static size_t find_first_simplex(Tetrapal* tetrapal, const float* points, const int size, vertex_t v[4]);

/* Generate a random integer from 0 to RANDOM_MAX given a seed value. The seed will be progressed. */
static inline long xrandom(random_t* seed);

/* Get a random integer from 0 to (range - 1) inclusive. */
static inline random_t random_range(random_t* seed, random_t range);

/* Swap two vertex indices. */
static inline void swap_vertex(vertex_t* a, vertex_t* b);

/* Swap two local indices. */
static inline void swap_local(local_t* a, local_t* b);

#ifdef TETRAPAL_DEBUG
/* Check that simplex [t] encloses or touches the query point. */
static bool is_enclosing_simplex(Tetrapal* tetrapal, simplex_t t, const float point[3]);

/* Check combinatorial data for inconsistencies or corruption. */
static void check_combinatorics(Tetrapal* tetrapal);

/* Print all simplex data. */
static void print_simplex_data(Tetrapal* tetrapal);

/* Print all vertex data. */
static void print_vertex_data(Tetrapal* tetrapal);

/* Print the size in memory of all data. */
static void print_memory(Tetrapal* tetrapal);
#endif

/********************************/
/*		3D Triangulation		*/
/********************************/

/* Table of local vertex indices such that the facet [i][0], [i][1], [i][2] is opposite [i].
	[i], [i][0], [i][1], [i][2] always form a positively-oriented tetrahedron. */
static const local_t facet_opposite_vertex[4][3] =
{
	 {1, 2, 3},
	 {0, 3, 2},
	 {3, 0, 1},
	 {2, 1, 0}
};

/* Table of local facet indices such that the directed edge [i, j] belongs to the local facet at [i][j].
	Useful for visiting the ring of tetrahedra around an edge. */
static const local_t facet_from_edge[4][4] =
{
	{  (local_t)(-1),  2,  3,  1 },
	{  3, (local_t)(-1),  0,  2 },
	{  1,  3, (local_t)(-1),  0 },
	{  2,  0,  1, (local_t)(-1) }
};

/* Initialise the 3D triangulation, creating the first simplex and setting up internal state. */
static error_t triangulate_3d(Tetrapal* tetrapal, vertex_t v[4], const float* points, const int size);

/* Insert a vertex [v] into the 3D triangulation. */
static error_t insert_3d(Tetrapal* tetrapal, vertex_t v);

/* Locate the simplex enclosing the input point, or an infinite simplex if it is outside the convex hull.
	If it fails, returns a null simplex. */
static simplex_t locate_3d(const Tetrapal* tetrapal, const coord_t point[3]);

/* Determine the conflict zone of a given point via depth-first search from [t], which must enclose the vertex [v].
	Frees conflicting simplices and retriangulates the cavity.
	Returns a simplex that was created during triangulation, or a null simplex if it failed. */
static simplex_t stellate_3d(Tetrapal* tetrapal, vertex_t v, simplex_t t);

/* Check whether a given point is in conflict with the simplex [t]. */
static bool conflict_3d(const Tetrapal* tetrapal, simplex_t t, const coord_t point[3]);

/* Interpolate an input point as the weighted sum of up to four existing points in the triangulation. */
static size_t interpolate_3d(const Tetrapal* tetrapal, const coord_t point[3], int indices[4], float weights[4], simplex_t* t);

/* Return the nearest neighbour of an input point. */
static vertex_t nearest_3d(const Tetrapal* tetrapal, const coord_t point[3]);

/* Scale a given input point in the range [0.0, 1.0] by the value defined by TETRAPAL_PRECISION.
	Input points beyond the expected range will be clamped before being transformed. */
static inline void transform_3d(const float in[3], coord_t out[3]);

/* Create a new tetrahedron [a, b, c, d] with positive orientation. Returns the global index of the new tetrahedron. */
static simplex_t new_tetrahedron(Tetrapal* tetrapal, vertex_t a, vertex_t b, vertex_t c, vertex_t d);

/********************************/
/*		2D Triangulation		*/
/********************************/

/* Table of local edge indices such that the edge [i][0], [i][1] is opposite vertex [i].
	[i], [i][0], [i][1] always form a positively-oriented triangle. */
static const local_t edge_opposite_vertex[3][2] =
{
	 {1, 2},
	 {2, 0},
	 {0, 1},
};

/* Initialise the 2D triangulation, creating the first simplex and setting up internal state. */
static error_t triangulate_2d(Tetrapal* tetrapal, vertex_t v[3], const float* points, const int size);

/* Insert a vertex [v] into the 2D triangulation. */
static error_t insert_2d(Tetrapal* tetrapal, vertex_t v);

/* Locate the simplex enclosing the input point, or an infinite simplex if it is outside the convex hull.
	If it fails, returns a null simplex. */
static simplex_t locate_2d(const Tetrapal* tetrapal, const coord_t point[2]);

/* Determine the conflict zone of a given point via depth-first search from [t], which must enclose the vertex [v].
	Frees conflicting simplices and retriangulates the cavity.
	Returns a simplex that was created during triangulation, or a null simplex if it failed. */
static simplex_t stellate_2d(Tetrapal* tetrapal, vertex_t v, simplex_t t);

/* Check whether a given point is in conflict with the simplex [t]. */
static bool conflict_2d(const Tetrapal* tetrapal, simplex_t t, const coord_t point[2]);

/* Interpolate an input point as the weighted sum of up to three existing points in the triangulation. */
static size_t interpolate_2d(const Tetrapal* tetrapal, const coord_t point[2], int indices[3], float weights[3], simplex_t* t);

/* Return the nearest neighbour of an input point. */
static vertex_t nearest_2d(const Tetrapal* tetrapal, const coord_t point[2]);

/* Transform 3D coordinates in the range [0.0, 1.0] to the local 2D coordinate system of the triangulation. */
static inline void transform_2d(const Tetrapal* tetrapal, const float point[3], coord_t out[2]);

/* Create a new triangle [a, b, c] with positive orientation. Returns the global index of the new triangle. */
static simplex_t new_triangle(Tetrapal* tetrapal, vertex_t a, vertex_t b, vertex_t c);

/********************************************/
/*		1D Triangulation (Binary Tree)		*/
/********************************************/

/* Initialise the 1D triangulation, i.e. build the binary tree. */
static error_t triangulate_1d(Tetrapal* tetrapal, const float* points, const int size);

/* Transform 3D coordinates in the range [0.0, 1.0] to the local 1D coordinate system of the triangulation. */
static inline void transform_1d(const Tetrapal* tetrapal, const float point[3], coord_t out[1]);

/* Interpolate an input point as the weighted sum of up to two existing points in the triangulation. */
static size_t interpolate_1d(const Tetrapal* tetrapal, const coord_t point[1], int indices[2], float weights[2]);

/* Return the nearest neighbour of an input point. */
static vertex_t nearest_1d(const Tetrapal* tetrapal, const coord_t point[1]);

/********************************************/
/*		0D Triangulation (Single Vertex)	*/
/********************************************/

/* Initialise the 0D triangulation. */
static error_t triangulate_0d(Tetrapal* tetrapal);

/* 'Interpolate' an input point in a 0d triangulation (returns the 0 vertex). */
static size_t interpolate_0d(int indices[1], float weights[1]);

/* Return the 0 vertex. */
static vertex_t nearest_0d(void);

/********************************************/
/*		Natural Neighbour Interpolation		*/
/********************************************/

/* Helper function to accumulate the weight for a given vertex index. */
static inline error_t natural_neighbour_accumulate(vertex_t index, coord_t weight, int* indices, float* weights, int size, size_t* count);

/* Get the natural neighbour coordinats of an input point within a 2D triangulation. */
static size_t natural_neighbour_2d(const Tetrapal* tetrapal, coord_t point[2], int* indices, float* weights, int size);

/* Get the natural neighbour coordinats of an input point within a 3D triangulation. */
static size_t natural_neighbour_3d(const Tetrapal* tetrapal, coord_t point[3], int* indices, float* weights, int size);

/********************************************************************************************/
/********************************************************************************************/
/*		IMPLEMENTATION																		*/
/********************************************************************************************/
/********************************************************************************************/

/********************************/
/*		Vector Maths			*/
/********************************/

static inline coord_t dot_3d(const coord_t a[3], const coord_t b[3])
{
	return a[0] * b[0] + a[1] * b[1] + a[2] * b[2];
}

static inline coord_t dot_2d(const coord_t a[2], const coord_t b[2])
{
	return a[0] * b[0] + a[1] * b[1];
}

static inline void sub_3d(const coord_t a[3], const coord_t b[3], coord_t result[3])
{
	result[0] = a[0] - b[0];
	result[1] = a[1] - b[1];
	result[2] = a[2] - b[2];
}

static inline void sub_2d(const coord_t a[2], const coord_t b[2], coord_t result[2])
{
	result[0] = a[0] - b[0];
	result[1] = a[1] - b[1];
}

static inline void mul_3d(const coord_t a[3], const coord_t s, coord_t result[3])
{
	result[0] = a[0] * s;
	result[1] = a[1] * s;
	result[2] = a[2] * s;
}

static inline void cross_3d(const coord_t a[3], const coord_t b[3], coord_t result[3])
{
	result[0] = a[1] * b[2] - a[2] * b[1];
	result[1] = a[2] * b[0] - a[0] * b[2];
	result[2] = a[0] * b[1] - a[1] * b[0];
}

static inline void normalise_3d(const coord_t a[3], coord_t result[3])
{
	coord_t length = sqrtf(a[0] * a[0] + a[1] * a[1] + a[2] * a[2]);
	result[0] = a[0] / length;
	result[1] = a[1] / length;
	result[2] = a[2] / length;
}

static void circumcentre_2d(const coord_t a[2], const coord_t b[2], const coord_t c[2], coord_t* result)
{
	/* Adapted from Shewchuk via https://ics.uci.edu/~eppstein/junkyard/circumcenter.html */

	/* Use coordinates relative to point `a' of the triangle. */
	double ab[2] = { (double)b[0] - (double)a[0], (double)b[1] - (double)a[1] };
	double ac[2] = { (double)c[0] - (double)a[0], (double)c[1] - (double)a[1] };

	/* Squares of lengths of the edges incident to `a'. */
	double ab_len = ab[0] * ab[0] + ab[1] * ab[1];
	double ac_len = ac[0] * ac[0] + ac[1] * ac[1];

	/* Calculate the denominator of the formulae. */
	double area = ab[0] * ac[1] - ab[1] * ac[0];
	double denominator = 0.5 / area;

	/* Calculate offset (from `a') of circumcenter. */
	double offset[2] =
	{
		(ac[1] * ab_len - ab[1] * ac_len) * denominator,
		(ab[0] * ac_len - ac[0] * ab_len) * denominator
	};

	result[0] = (coord_t)offset[0] + a[0];
	result[1] = (coord_t)offset[1] + a[1];
}

static void circumcentre_3d(const coord_t a[3], const coord_t b[3], const coord_t c[3], const coord_t d[3], coord_t* result)
{
	/* Adapted from Shewchuk via https://ics.uci.edu/~eppstein/junkyard/circumcenter.html */

	/* Use coordinates relative to point `a' of the tetrahedron. */
	double ab[3] = { (double)b[0] - (double)a[0], (double)b[1] - (double)a[1], (double)b[2] - (double)a[2] };
	double ac[3] = { (double)c[0] - (double)a[0], (double)c[1] - (double)a[1], (double)c[2] - (double)a[2] };
	double ad[3] = { (double)d[0] - (double)a[0], (double)d[1] - (double)a[1], (double)d[2] - (double)a[2] };

	/* Squares of lengths of the edges incident to `a'. */
	double ab_len = ab[0] * ab[0] + ab[1] * ab[1] + ab[2] * ab[2];
	double ac_len = ac[0] * ac[0] + ac[1] * ac[1] + ac[2] * ac[2];
	double ad_len = ad[0] * ad[0] + ad[1] * ad[1] + ad[2] * ad[2];

	/* Cross products of these edges. */
	double acxad[3] = { ac[1] * ad[2] - ac[2] * ad[1], ac[2] * ad[0] - ac[0] * ad[2], ac[0] * ad[1] - ac[1] * ad[0] };
	double adxab[3] = { ad[1] * ab[2] - ad[2] * ab[1], ad[2] * ab[0] - ad[0] * ab[2], ad[0] * ab[1] - ad[1] * ab[0] };
	double abxac[3] = { ab[1] * ac[2] - ab[2] * ac[1], ab[2] * ac[0] - ab[0] * ac[2], ab[0] * ac[1] - ab[1] * ac[0] };

	/* Calculate the denominator of the formulae. */
	double area = ab[0] * acxad[0] + ab[1] * acxad[1] + ab[2] * acxad[2];
	double denominator = 0.5 / area;

	/* Calculate offset (from `a') of circumcenter. */
	double offset[3] =
	{
		(ab_len * acxad[0] + ac_len * adxab[0] + ad_len * abxac[0]) * denominator,
		(ab_len * acxad[1] + ac_len * adxab[1] + ad_len * abxac[1]) * denominator,
		(ab_len * acxad[2] + ac_len * adxab[2] + ad_len * abxac[2]) * denominator
	};

	result[0] = (coord_t)offset[0] + a[0];
	result[1] = (coord_t)offset[1] + a[1];
	result[2] = (coord_t)offset[2] + a[2];
}

static inline void midpoint_2d(const coord_t a[2], const coord_t b[2], coord_t result[2])
{
	result[0] = (a[0] + b[0]) / 2;
	result[1] = (a[1] + b[1]) / 2;
}

static inline void midpoint_3d(const coord_t a[3], const coord_t b[3], coord_t result[3])
{
	result[0] = (a[0] + b[0]) / 2;
	result[1] = (a[1] + b[1]) / 2;
	result[2] = (a[2] + b[2]) / 2;
}

static inline coord_t distance_squared_1d(const coord_t a[1], const coord_t b[1])
{
	coord_t ab = b[0] - a[0];
	return ab * ab;
}

static inline coord_t distance_squared_2d(const coord_t a[2], const coord_t b[2])
{
	coord_t ab[2] = { b[0] - a[0], b[1] - a[1] };
	return ab[0] * ab[0] + ab[1] * ab[1];
}

static inline coord_t distance_squared_3d(const coord_t a[3], const coord_t b[3])
{
	coord_t ab[3] = { b[0] - a[0], b[1] - a[1], b[2] - a[2] };
	return ab[0] * ab[0] + ab[1] * ab[1] + ab[2] * ab[2];
}

/********************************/
/*		128-Bit Integer			*/
/********************************/

static inline int128_t int128_zero(void)
{
	int128_t result;
	result.digits[0] = result.digits[1] = 0;
	result.sign = 0;
	return result;
}

static inline int128_t int128_from_product(const double a, const double b)
{
	int128_t result = int128_zero();
	digit_t mask = (1uLL << 32) - 1;
	digit_t tmp[3];

	/* Exit early if any of the operands are zero. */
	if (a == 0 || b == 0)
		return result;

	/* Get the sign of the product. */
	result.sign = (char)((a < 0) == (b < 0) ? 1 : -1);

	/* Get the magnitude of the two doubles and seperate into higher and lower parts. */
	digit_t a_digit = (digit_t)fabs(a);
	digit_t b_digit = (digit_t)fabs(b);
	digit_t a_hi = a_digit >> 32;
	digit_t a_lo = a_digit & mask;
	digit_t b_hi = b_digit >> 32;
	digit_t b_lo = b_digit & mask;

	/* Get products. */
	result.digits[0] = a_hi * b_hi;
	result.digits[1] = a_lo * b_lo;
	tmp[0] = a_hi * b_lo;
	tmp[1] = a_lo * b_hi;

	/* Add upper products. */
	result.digits[0] += tmp[0] >> 32;
	result.digits[0] += tmp[1] >> 32;

	/* Add lower products. */
	tmp[0] = (tmp[0] & mask) + (tmp[1] & mask);
	tmp[1] = (tmp[0] & mask) << 32;
	result.digits[1] += tmp[1];
	result.digits[0] += tmp[0] >> 32;
	result.digits[0] += result.digits[1] < tmp[1];

	return result;
}

static inline int128_t int128_add(const int128_t a, const int128_t b)
{
	/* Test edge cases. */

	if (a.sign == 0) /* [a] is zero. */
		return b;

	if (b.sign == 0) /* [b] is zero. */
		return a;

	if (a.sign < b.sign) /* [a] is negative and [b] is positive. */
		return int128_sub(b, int128_abs(a));

	if (a.sign > b.sign) /* [a] is positive and [b] is negative.*/
		return int128_sub(a, int128_abs(b));

	/* Otherwise, add as normal, retaining the sign. */
	int128_t result = int128_zero();

	result.digits[1] = a.digits[1] + b.digits[1];
	result.digits[0] = a.digits[0] + b.digits[0];
	result.digits[0] += result.digits[1] < a.digits[1]; /* Add carry.*/

	TETRAPAL_ASSERT(result.digits[0] >= a.digits[0], "Addition overflow!\n");

	result.sign = a.sign;

	return result;
}

static inline int128_t int128_sub(const int128_t a, const int128_t b)
{
	/* Test edge cases. */

	if (a.sign == 0) /* [a] is zero.*/
		return int128_inv(b);

	if (b.sign == 0) /* [b] is zero.*/
		return a;

	if (a.sign < b.sign) /* [a] is negative and [b] is positive. */
		return int128_neg(int128_add(b, int128_abs(a)));

	if (a.sign > b.sign) /* [a] is positive and [b] is negative. */
		return int128_add(a, int128_abs(b));

	if (a.sign < 0 && b.sign < 0) /* [a] and [b] are both negative. */
		return int128_add(a, int128_abs(b));

	if (a.digits[0] == b.digits[0] && a.digits[1] == b.digits[1]) /* [a] and [b] are equal. */
		return int128_zero();

	if (int128_lt_abs(a, b) == true) /* [a] is less than [b]. */
		return int128_neg(int128_sub(b, a));

	/* Subtract. */
	int128_t result = int128_zero();

	result.digits[1] = a.digits[1] - b.digits[1];
	result.digits[0] = a.digits[0] - b.digits[0];
	result.digits[0] -= result.digits[1] > a.digits[1]; /* Subtract borrow.*/

	TETRAPAL_ASSERT(result.digits[0] <= a.digits[0], "Subtraction underflow!\n");

	result.sign = a.sign;

	return result;
}

static inline int128_t int128_abs(const int128_t a)
{
	int128_t result = a;
	result.sign = a.sign != 0 ? 1 : 0;
	return result;
}

static inline int128_t int128_neg(const int128_t a)
{
	int128_t result = a;
	result.sign = (char)(a.sign != 0 ? -1 : 0);
	return result;
}

static inline int128_t int128_inv(const int128_t a)
{
	int128_t result = a;
	result.sign = (char)(a.sign < 0 ? 1 : (a.sign > 0 ? -1 : 0));
	return result;
}

static inline bool int128_lt_abs(const int128_t a, const int128_t b)
{
	return
		a.digits[0] < b.digits[0] ? true :
		a.digits[0] > b.digits[0] ? false :
		a.digits[1] < b.digits[1] ? true : false;
}

/********************************/
/*		Geometric Predicates	*/
/********************************/

static inline bool is_coincident_3d(const coord_t a[3], const coord_t b[3])
{
	return (a[0] == b[0] && a[1] == b[1] && a[2] == b[2]) ? true : false;
}

static bool is_colinear_3d(const coord_t a[3], const coord_t b[3], const coord_t c[3])
{
	/* Assuming a maximum bit length of 16 for each coordinate, the result can be computed exactly with doubles. */

	/* Bit length here is still 16, because the minimum value of an input coordinate is always 0. */
	double ab[3] = { (double)b[0] - (double)a[0], (double)b[1] - (double)a[1], (double)b[2] - (double)a[2] };
	double ac[3] = { (double)c[0] - (double)a[0], (double)c[1] - (double)a[1], (double)c[2] - (double)a[2] };

	/* Bit length of cross product is at most 16 + 16 + 1 = 33. */
	double cross[3] =
	{
		ab[1] * ac[2] - ab[2] * ac[1],
		ab[2] * ac[0] - ab[0] * ac[2],
		ab[0] * ac[1] - ab[1] * ac[0]
	};

	/* Cross product is guaranteed to be exact, so simply check that all components are zero. */
	return (cross[0] == 0 && cross[1] == 0 && cross[2] == 0) ? true : false;
}

static inline bool is_coplanar_3d(const coord_t a[3], const coord_t b[3], const coord_t c[3], const coord_t d[3])
{
	return (orient_3d(a, b, c, d) == 0) ? true : false;
}

static coord_t orient_2d(const coord_t a[2], const coord_t b[2], const coord_t c[2])
{
	/* Bit length here is still 16, because the absolute difference between any two 2D coordinates is <= 16 bits. */
	double ab[2] = { (double)b[0] - (double)a[0], (double)b[1] - (double)a[1] };
	double ac[2] = { (double)c[0] - (double)a[0], (double)c[1] - (double)a[1] };

	/* Bit length of determinant is at most 16 * 2 + 1 = 33. */
	double det = ab[0] * ac[1] - ab[1] * ac[0];

	return (coord_t)det;
}

static coord_t orient_3d(const coord_t a[3], const coord_t b[3], const coord_t c[3], const coord_t d[3])
{
	/* Assuming a maximum bit length of 16 for each coordinate, the result can be computed exactly with doubles. */

	/* Bit length here is still 16, because the minimum value of an input coordinate is always 0. */
	double bc[3] = { (double)c[0] - (double)b[0], (double)c[1] - (double)b[1], (double)c[2] - (double)b[2] };
	double bd[3] = { (double)d[0] - (double)b[0], (double)d[1] - (double)b[1], (double)d[2] - (double)b[2] };
	double ba[3] = { (double)a[0] - (double)b[0], (double)a[1] - (double)b[1], (double)a[2] - (double)b[2] };

	/* Bit length of cross product is at most 16 + 16 + 1 = 33. */
	double cross[3] =
	{
		bc[1] * bd[2] - bc[2] * bd[1],
		bc[2] * bd[0] - bc[0] * bd[2],
		bc[0] * bd[1] - bc[1] * bd[0]
	};

	/* Bit length of determinant is at most 33 + 16 + 2 = 51. */
	double det = cross[0] * ba[0] + cross[1] * ba[1] + cross[2] * ba[2];

	/* 51 <= 53, so no extended precision is required. */
	return (coord_t)det;
}

static coord_t incircle_2d(const coord_t a[2], const coord_t b[2], const coord_t c[2], const coord_t d[2])
{
	/* maxbitlen = 16. */
	double da[2] = { (double)a[0] - (double)d[0], (double)a[1] - (double)d[1] };
	double db[2] = { (double)b[0] - (double)d[0], (double)b[1] - (double)d[1] };
	double dc[2] = { (double)c[0] - (double)d[0], (double)c[1] - (double)d[1] };

	/* maxbitlen = 33. */
	double abdet = da[0] * db[1] - db[0] * da[1];
	double bcdet = db[0] * dc[1] - dc[0] * db[1];
	double cadet = dc[0] * da[1] - da[0] * dc[1];

	double alift = da[0] * da[0] + da[1] * da[1];
	double blift = db[0] * db[0] + db[1] * db[1];
	double clift = dc[0] * dc[0] + dc[1] * dc[1];

	/* maxbitlen = 68. */
	double det = alift * bcdet + blift * cadet + clift * abdet;

	/* If the absolute value exceeds the maximum error, we can be sure that the sign is accurate. */
	if (fabs(det) > MAX_ERROR_INCIRCLE)
		return (coord_t)det;

	/* Otherwise, we resort to exact arithmetic. */
	int128_t x = int128_from_product(alift, bcdet);
	int128_t y = int128_from_product(blift, cadet);
	int128_t z = int128_from_product(clift, abdet);
	int128_t det_exact = int128_add(int128_add(x, y), z);

	return (coord_t)det_exact.sign;
}

static coord_t insphere_3d(const coord_t a[3], const coord_t b[3], const coord_t c[3], const coord_t d[3], const coord_t e[3])
{
	/* maxbitlen = 16. */
	double ea[3] = { (double)a[0] - (double)e[0], (double)a[1] - (double)e[1], (double)a[2] - (double)e[2] };
	double eb[3] = { (double)b[0] - (double)e[0], (double)b[1] - (double)e[1], (double)b[2] - (double)e[2] };
	double ec[3] = { (double)c[0] - (double)e[0], (double)c[1] - (double)e[1], (double)c[2] - (double)e[2] };
	double ed[3] = { (double)d[0] - (double)e[0], (double)d[1] - (double)e[1], (double)d[2] - (double)e[2] };

	/* maxbitlen = 33. */
	double ab = ea[0] * eb[1] - eb[0] * ea[1];
	double bc = eb[0] * ec[1] - ec[0] * eb[1];
	double cd = ec[0] * ed[1] - ed[0] * ec[1];
	double da = ed[0] * ea[1] - ea[0] * ed[1];
	double ac = ea[0] * ec[1] - ec[0] * ea[1];
	double bd = eb[0] * ed[1] - ed[0] * eb[1];

	/* maxbitlen = 51. */
	double abc = ea[2] * bc - eb[2] * ac + ec[2] * ab;
	double bcd = eb[2] * cd - ec[2] * bd + ed[2] * bc;
	double cda = ec[2] * da + ed[2] * ac + ea[2] * cd;
	double dab = ed[2] * ab + ea[2] * bd + eb[2] * da;

	/* maxbitlen = 34. */
	double alift = ea[0] * ea[0] + ea[1] * ea[1] + ea[2] * ea[2];
	double blift = eb[0] * eb[0] + eb[1] * eb[1] + eb[2] * eb[2];
	double clift = ec[0] * ec[0] + ec[1] * ec[1] + ec[2] * ec[2];
	double dlift = ed[0] * ed[0] + ed[1] * ed[1] + ed[2] * ed[2];

	/* maxbitlen = 87. */
	double det = (dlift * abc - clift * dab) + (blift * cda - alift * bcd);

	/* If the absolute value exceeds the maximum error, we can be sure that the sign is accurate. */
	if (fabs(det) > MAX_ERROR_INSPHERE)
		return (coord_t)det;

	/* Otherwise, we resort to exact arithmetic. */
	int128_t x = int128_sub(int128_from_product(dlift, abc), int128_from_product(clift, dab));
	int128_t y = int128_sub(int128_from_product(blift, cda), int128_from_product(alift, bcd));
	int128_t det_exact = int128_add(x, y);

	return (coord_t)det_exact.sign;
}

#ifdef TETRAPAL_DEBUG
static inline size_t bit_length(const coord_t a)
{
	size_t bits;
	size_t var = (size_t)fabs(a);

	for (bits = 0; var != 0; bits++)
		var >>= 1;

	return bits;
}
#endif

/********************************/
/*		Stack					*/
/********************************/

static error_t stack_init(Stack* stack, size_t reserve)
{
	stack->count = 0;
	stack->capacity = reserve;
	stack->data = TETRAPAL_MALLOC(sizeof(*stack->data) * reserve);

	if (stack->data == NULL)
		return ERROR_OUT_OF_MEMORY;

	return ERROR_NONE;
}

static void stack_free(Stack* stack)
{
	TETRAPAL_FREE(stack->data);
	stack->data = NULL;
}

static void stack_clear(Stack* stack)
{
	stack->count = 0;
}

static error_t stack_insert(Stack* stack, simplex_t t)
{
	if (stack_check_capacity(stack) != ERROR_NONE)
		return ERROR_OUT_OF_MEMORY;

	stack->data[stack->count] = t;
	stack->count += 1;

	return ERROR_NONE;
}

static error_t stack_check_capacity(Stack* stack)
{
	if (stack->count < stack->capacity)
		return ERROR_NONE;

	/* Stack is at capacity; resize. */
	size_t new_capacity = (stack->capacity * (size_t)ARRAY_GROWTH_FACTOR) + 1;
	void* new_data = TETRAPAL_REALLOC(stack->data, sizeof(*stack->data) * new_capacity);

	if (new_data == NULL)
		return ERROR_OUT_OF_MEMORY;

	stack->capacity = new_capacity;
	stack->data = new_data;

	return ERROR_NONE;
}

static void stack_pop(Stack* stack)
{
	stack->count -= 1;
}

static simplex_t stack_top(const Stack* stack)
{
	return stack->data[stack->count - 1];
}

static bool stack_contains(const Stack* stack, simplex_t t)
{
	for (size_t i = 0; i < stack->count; i++)
	{
		if (stack->data[i] == t)
			return true;
	}

	return false;
}

static bool stack_is_empty(const Stack* stack)
{
	return (stack->count == 0);
}

/********************************/
/*		KD Tree					*/
/********************************/

static error_t kdtree_balance(Tetrapal* tetrapal, const size_t begin, const size_t end, const size_t depth)
{
	/* Building a KD Tree has about O(logN) complexity, so a recursive solution shouldn't be an issue. */

	// Partition along current axis and recurse
	size_t median = (begin + end) / 2;
	kdtree_sort_median(tetrapal, begin, end, depth);

	if (median > begin) kdtree_balance(tetrapal, begin, median - 1, (depth + 1) % tetrapal->dimensions);
	if (median < end) kdtree_balance(tetrapal, median + 1, end, (depth + 1) % tetrapal->dimensions);

	return ERROR_NONE;
}

static size_t kdtree_find_approximate(const Tetrapal* tetrapal, const coord_t* p)
{
	vertex_t* tree = tetrapal->vertices.tree;
	coord_t* coordinates = tetrapal->vertices.coordinates;
	size_t k = tetrapal->dimensions;

	size_t begin = 0;
	size_t end = tetrapal->vertices.count - 1;
	size_t depth = 0;

	while (true)
	{
		size_t median = (begin + end) / 2;
		coord_t orientation = p[depth] - coordinates[(size_t)tree[median] * k + depth];

		if (orientation < 0) /* Go left. */
		{
			if (median > begin)
			{
				end = median - 1;
				depth = (depth + 1) % k;
				continue;
			}
		}
		else if (median < end) /* Go right.*/
		{
			begin = median + 1;
			depth = (depth + 1) % k;
			continue;
		}

		/* Reached the leaf node; return this node. */
		return median;
	}
}

static inline vertex_t kdtree_get_vertex(const Tetrapal* tetrapal, const size_t i)
{
	return tetrapal->vertices.tree[i];
}

static void kdtree_sort_median(Tetrapal* tetrapal, const size_t begin, const size_t end, const size_t depth)
{
	vertex_t* tree = tetrapal->vertices.tree;
	coord_t* coordinates = tetrapal->vertices.coordinates;
	size_t k = tetrapal->dimensions;

	// Using Hoare's Partition Scheme
	size_t lo = begin;
	size_t hi = end + 1;
	size_t median = (begin + end) / 2;

	while (true)
	{
		do
			lo++;
		while
			(lo <= end && (coordinates[(size_t)tree[lo] * k + depth] < coordinates[(size_t)tree[begin] * k + depth]));

		do
			hi--;
		while
			(coordinates[(size_t)tree[hi] * k + depth] > coordinates[(size_t)tree[begin] * k + depth]);

		if (lo >= hi)
			break;
		else
			swap_vertex(&tree[lo], &tree[hi]);
	}

	swap_vertex(&tree[begin], &tree[hi]);

	/* Return or recurse. */
	if (hi == median)
		return;

	if (hi < median)
		kdtree_sort_median(tetrapal, hi + 1, end, depth);
	else
		kdtree_sort_median(tetrapal, begin, hi - 1, depth);
}

#ifdef TETRAPAL_DEBUG
static void kdtree_print(const Tetrapal* tetrapal)
{
	printf("** PRINTING KD TREE DATA **\n");

	printf("\n");
	kdtree_print_recurse(tetrapal, 0, tetrapal->vertices.count - 1, 0);
	printf("\n");

	return;
}

static void kdtree_print_recurse(const Tetrapal* tetrapal, size_t begin, size_t end, size_t depth)
{
	size_t stride = tetrapal->dimensions;
	size_t i = (begin + end) / 2;

	printf("IDX: [%li] COORDS: [ ", tetrapal->vertices.tree[i]);

	for (size_t j = 0; j < stride; j++)
	{
		printf("%.3f ", tetrapal->vertices.coordinates[tetrapal->vertices.tree[i] * stride + j]);
	}

	printf("]\n");

	if (i > begin) /* Go left.*/
	{
		for (size_t j = 0; j < depth + 1; j++)
			printf("  ");

		printf("[LEFT] -> ");

		kdtree_print_recurse(tetrapal, begin, i - 1, depth + 1);
	}

	if (i < end) /* Go right.*/
	{
		for (size_t j = 0; j < depth + 1; j++)
			printf("  ");

		printf("[RIGHT] -> ");

		kdtree_print_recurse(tetrapal, i + 1, end, depth + 1);
	}
}
#endif

/********************************/
/*		Cavity					*/
/********************************/

static error_t cavity_init(Cavity* cavity, size_t reserve)
{
	cavity->facets.count = 0;
	cavity->facets.capacity = reserve;

	cavity->facets.incident_vertex = TETRAPAL_MALLOC(sizeof(*cavity->facets.incident_vertex) * reserve * 3);
	cavity->facets.adjacent_simplex = TETRAPAL_MALLOC(sizeof(*cavity->facets.adjacent_simplex) * reserve);
	cavity->facets.boundary_facet = TETRAPAL_MALLOC(sizeof(*cavity->facets.boundary_facet) * reserve);

	size_t table_capacity = reserve * 4;

	cavity->table.capacity = table_capacity;
	cavity->table.count = 0;

	cavity->table.edge = TETRAPAL_MALLOC(sizeof(*cavity->table.edge) * table_capacity * 2);
	cavity->table.facet = TETRAPAL_MALLOC(sizeof(*cavity->table.facet) * table_capacity);

	if (cavity->facets.adjacent_simplex == NULL ||
		cavity->facets.incident_vertex == NULL ||
		cavity->facets.boundary_facet == NULL ||
		cavity->table.edge == NULL ||
		cavity->table.facet == NULL)
	{
		cavity_free(cavity);
		return ERROR_OUT_OF_MEMORY;
	}

	return ERROR_NONE;
}

static void cavity_free(Cavity* cavity)
{
	TETRAPAL_FREE(cavity->facets.incident_vertex);
	TETRAPAL_FREE(cavity->facets.adjacent_simplex);
	TETRAPAL_FREE(cavity->facets.boundary_facet);
	TETRAPAL_FREE(cavity->table.edge);
	TETRAPAL_FREE(cavity->table.facet);

	cavity->facets.incident_vertex = NULL;
	cavity->facets.adjacent_simplex = NULL;
	cavity->facets.boundary_facet = NULL;
	cavity->table.edge = NULL;
	cavity->table.facet = NULL;
}

static facet_t cavity_insert(Cavity* cavity, vertex_t a, vertex_t b, vertex_t c, simplex_t t, local_t i)
{
	error_t error = cavity_check_capacity(cavity);

	if (error)
		return FACET_NULL;

	const facet_t f = (facet_t)cavity->facets.count;

	/* Create facet relations. */
	cavity->facets.incident_vertex[f * 3 + 0] = a;
	cavity->facets.incident_vertex[f * 3 + 1] = b;
	cavity->facets.incident_vertex[f * 3 + 2] = c;
	cavity->facets.adjacent_simplex[f] = t;
	cavity->facets.boundary_facet[f] = i;

	/* Update adjacency table. */
	error = 0;
	error += cavity_insert_edge(cavity, a, b, f);
	error += cavity_insert_edge(cavity, b, c, f);
	error += cavity_insert_edge(cavity, c, a, f);

	if (error)
		return FACET_NULL;

	cavity->facets.count += 1;
	return f;
}

static error_t cavity_check_capacity(Cavity* cavity)
{
	if (cavity->facets.count < cavity->facets.capacity)
		return ERROR_NONE;

	size_t new_capacity = (cavity->facets.capacity * (size_t)ARRAY_GROWTH_FACTOR) + 1;
	void* new_incident = TETRAPAL_REALLOC(cavity->facets.incident_vertex, sizeof(*cavity->facets.incident_vertex) * new_capacity * 3);
	void* new_adjacent = TETRAPAL_REALLOC(cavity->facets.adjacent_simplex, sizeof(*cavity->facets.adjacent_simplex) * new_capacity);
	void* new_boundary = TETRAPAL_REALLOC(cavity->facets.boundary_facet, sizeof(*cavity->facets.boundary_facet) * new_capacity);

	if (new_incident == NULL ||
		new_adjacent == NULL ||
		new_boundary == NULL)
	{
		TETRAPAL_FREE(new_incident);
		TETRAPAL_FREE(new_adjacent);
		TETRAPAL_FREE(new_boundary);
		return ERROR_OUT_OF_MEMORY;
	}

	cavity->facets.capacity = new_capacity;
	cavity->facets.incident_vertex = new_incident;
	cavity->facets.adjacent_simplex = new_adjacent;
	cavity->facets.boundary_facet = new_boundary;

	return ERROR_NONE;
}

static error_t cavity_insert_edge(Cavity* cavity, vertex_t a, vertex_t b, facet_t f)
{
	error_t error = cavity_table_check_capacity(cavity);

	if (error)
		return error;

	size_t hash = cavity_edge_hash(a, b) % cavity->table.capacity;

	while (cavity->table.facet[hash] != CAVITY_TABLE_FREE)
	{
		hash = (hash + 1) % cavity->table.capacity;
	}

	cavity->table.edge[hash * 2 + 0] = a;
	cavity->table.edge[hash * 2 + 1] = b;
	cavity->table.facet[hash] = f;
	cavity->table.count += 1;

	return ERROR_NONE;
}

static error_t cavity_table_check_capacity(Cavity* cavity)
{
	if ((double)cavity->table.count / (double)cavity->table.capacity < CAVITY_TABLE_MAX_LOAD)
		return ERROR_NONE;

	/* Stack is at capacity; resize. */
	size_t old_capacity = cavity->table.capacity;
	vertex_t* old_edge = cavity->table.edge;
	facet_t* old_facet = cavity->table.facet;

	size_t new_capacity = (cavity->table.capacity * (size_t)ARRAY_GROWTH_FACTOR) + 1;
	vertex_t* new_edge = TETRAPAL_MALLOC(sizeof(*cavity->table.edge) * new_capacity * 3);
	facet_t* new_facet = TETRAPAL_MALLOC(sizeof(*cavity->table.facet) * new_capacity);

	if (new_edge == NULL ||
		new_facet == NULL)
	{
		TETRAPAL_FREE(new_edge);
		TETRAPAL_FREE(new_facet);
		return ERROR_OUT_OF_MEMORY;
	}

	cavity->table.count = 0;
	cavity->table.capacity = new_capacity;
	cavity->table.edge = new_edge;
	cavity->table.facet = new_facet;

	/* Rehash all elements. */
	for (size_t i = 0; i < cavity->table.capacity; i++)
		cavity->table.facet[i] = CAVITY_TABLE_FREE;

	for (size_t i = 0; i < old_capacity; i++)
	{
		facet_t f = old_facet[i];

		if (f == CAVITY_TABLE_FREE)
			continue;

		vertex_t a = old_edge[i * 2 + 0];
		vertex_t b = old_edge[i * 2 + 1];

		cavity_insert_edge(cavity, a, b, f);
	}

	TETRAPAL_FREE(old_facet);
	TETRAPAL_FREE(old_edge);

	return ERROR_NONE;
}

static size_t cavity_edge_hash(vertex_t a, vertex_t b)
{
	const size_t x = (size_t)(a);
	const size_t y = (size_t)(b);
	return (x * 419) ^ (y * 31);
}

static void cavity_clear(Cavity* cavity)
{
	cavity->facets.count = 0;
	cavity->table.count = 0;

	/* Set all hash table elements to free. */
	for (size_t i = 0; i < cavity->table.capacity; i++)
		cavity->table.facet[i] = CAVITY_TABLE_FREE;
}

static facet_t cavity_find(Cavity* cavity, vertex_t a, vertex_t b)
{
	size_t hash = cavity_edge_hash(a, b) % cavity->table.capacity;

	while (cavity->table.facet[hash] != CAVITY_TABLE_FREE)
	{
		const vertex_t v[2] =
		{
			cavity->table.edge[hash * 2 + 0],
			cavity->table.edge[hash * 2 + 1],
		};

		if (a == v[0] && b == v[1])
			return cavity->table.facet[hash];

		hash = (hash + 1) % cavity->table.capacity;
	}

	/* Uh-oh. Something went wrong. */
	TETRAPAL_ASSERT(0, "Edge could not be found in the adjacency table!");
	return FACET_NULL;
}

static void cavity_set_adjacent_simplex(Cavity* cavity, facet_t f, simplex_t t)
{
	cavity->facets.adjacent_simplex[f] = t;
}

static vertex_t cavity_get_incident_vertex(Cavity* cavity, facet_t f, local_t i)
{
	return cavity->facets.incident_vertex[f * 3 + i];
}

static simplex_t cavity_get_adjacent_simplex(Cavity* cavity, facet_t f)
{
	return cavity->facets.adjacent_simplex[f];
}

static local_t cavity_get_adjacent_simplex_facet(Cavity* cavity, facet_t f)
{
	return cavity->facets.boundary_facet[f];
}

#ifdef TETRAPAL_DEBUG
/* Print all facet data. */
static void cavity_print_facet_data(Cavity* cavity)
{
	const size_t count = cavity->facets.count;

	printf("** PRINTING FACET DATA **\n");
	printf("Count: %zu\n", count);

	for (facet_t f = 0; (size_t)f < count; f++)
	{
		printf("IDX: [%u] V: [%lu, %lu, %lu] T: [%lu] F: [%i]\n",
			f,
			cavity_get_incident_vertex(cavity, f, 0),
			cavity_get_incident_vertex(cavity, f, 1),
			cavity_get_incident_vertex(cavity, f, 2),
			cavity_get_adjacent_simplex(cavity, f),
			cavity_get_adjacent_simplex_facet(cavity, f));
	}
}
#endif

/********************************/
/*		Tetrapal Core			*/
/********************************/

Tetrapal* tetrapal_new(const float* points, const int size)
{
	/* No points given. */
	if (size < 1)
		return NULL;

	Tetrapal* tetrapal = TETRAPAL_MALLOC(sizeof(*tetrapal));

	if (tetrapal == NULL)
		return NULL;

	/* Set all pointers to null. */
	tetrapal->vertices.coordinates = NULL;
	tetrapal->vertices.incident_simplex = NULL;
	tetrapal->vertices.tree = NULL;
	tetrapal->simplices.adjacent_simplex = NULL;
	tetrapal->simplices.incident_vertex = NULL;
	tetrapal->simplices.flags = NULL;
	tetrapal->simplices.deleted.simplices = NULL;
	tetrapal->stack.data = NULL;
	tetrapal->cavity.facets.incident_vertex = NULL;
	tetrapal->cavity.facets.adjacent_simplex = NULL;
	tetrapal->cavity.facets.boundary_facet = NULL;
	tetrapal->cavity.table.edge = NULL;
	tetrapal->cavity.table.facet = NULL;

	/* Find the first d-simplex and determine the number of dimensions of the point set. */
	vertex_t v[4];
	find_first_simplex(tetrapal, points, size, v);

	/* Choose the appropriate path based on the number of dimensions. */
	error_t error = ERROR_NONE;

	switch (tetrapal->dimensions)
	{
	case 0:
		error = triangulate_0d(tetrapal);
		break;

	case 1:
		error = triangulate_1d(tetrapal, points, size);
		break;

	case 2:
		error = triangulate_2d(tetrapal, v, points, size);
		break;

	case 3:
		error = triangulate_3d(tetrapal, v, points, size);
		break;
	}

	/* Initialisation failed for whatever reason; abort. */
	if (error != ERROR_NONE)
	{
		tetrapal_free(tetrapal);
		return NULL;
	}

	return tetrapal;
}

void tetrapal_free(Tetrapal* tetrapal)
{
	if (tetrapal == NULL)
		return;

	TETRAPAL_FREE(tetrapal->vertices.coordinates);
	TETRAPAL_FREE(tetrapal->vertices.incident_simplex);
	TETRAPAL_FREE(tetrapal->vertices.tree);
	TETRAPAL_FREE(tetrapal->simplices.incident_vertex);
	TETRAPAL_FREE(tetrapal->simplices.adjacent_simplex);
	TETRAPAL_FREE(tetrapal->simplices.flags);
	TETRAPAL_FREE(tetrapal->simplices.deleted.simplices);
	stack_free(&tetrapal->stack);
	cavity_free(&tetrapal->cavity);

	TETRAPAL_FREE(tetrapal);
}

int tetrapal_interpolate(const Tetrapal* tetrapal, const float point[3], int* indices, float* weights)
{
	if (tetrapal == NULL)
		return 0;

	coord_t p[3];
	simplex_t t; /* Enclosing simplex; unused. */

	switch (tetrapal->dimensions)
	{
	case 0:
		return (int)interpolate_0d(indices, weights);

	case 1:
		transform_1d(tetrapal, point, p);
		return (int)interpolate_1d(tetrapal, p, indices, weights);

	case 2:
		transform_2d(tetrapal, point, p);
		return (int)interpolate_2d(tetrapal, p, indices, weights, &t);

	case 3:
		transform_3d(point, p);
		return (int)interpolate_3d(tetrapal, p, indices, weights, &t);

	default:
		return 0;
	}
}

int tetrapal_natural_neighbour(const Tetrapal* tetrapal, const float point[3], int* indices, float* weights, const int size)
{
	if (tetrapal == NULL)
		return 0;

	coord_t p[3];

	switch (tetrapal->dimensions)
	{
	case 0:
		return size < 1 ? 0 : (int)interpolate_0d(indices, weights);

	case 1:
		transform_1d(tetrapal, point, p);
		return size < 2 ? 0 : (int)interpolate_1d(tetrapal, p, indices, weights);

	case 2:
		transform_2d(tetrapal, point, p);
		return (int)natural_neighbour_2d(tetrapal, p, indices, weights, size);

	case 3:
		transform_3d(point, p);
		return (int)natural_neighbour_3d(tetrapal, p, indices, weights, size);

	default:
		return 0;
	}
}

int tetrapal_nearest_neighbour(const Tetrapal* tetrapal, const float point[3])
{
	if (tetrapal == NULL)
		return -1;

	coord_t p[3];

	switch (tetrapal->dimensions)
	{
	case 0:
		return (int)nearest_0d();

	case 1:
		transform_1d(tetrapal, point, p);
		return (int)nearest_1d(tetrapal, p);

	case 2:
		transform_2d(tetrapal, point, p);
		return (int)nearest_2d(tetrapal, p);

	case 3:
		transform_3d(point, p);
		return (int)nearest_3d(tetrapal, p);

	default:
		return -1;
	}
}

int tetrapal_number_of_elements(const Tetrapal* tetrapal)
{
	if (tetrapal == NULL)
		return 0;

	int count = 0;

	switch (tetrapal->dimensions)
	{
	case 0:
		return 1;

	case 1:
		return (int)(tetrapal->vertices.count - 1);

	case 2:
	case 3:

		for (size_t i = 0; i < tetrapal->simplices.count; i++)
		{
			/* Skip infinite simplices. */
			if (is_infinite_simplex(tetrapal, (simplex_t)i) == true)
				continue;

			/* Skip free simplices. */
			if (is_free_simplex(tetrapal, (simplex_t)i) == true)
				continue;

			count += 1;
		}

		return count;

	default:
		return 0;
	}
}

int tetrapal_number_of_dimensions(const Tetrapal* tetrapal)
{
	if (tetrapal == NULL)
		return -1;

	return (int)tetrapal->dimensions;
}

int tetrapal_element_size(const Tetrapal* tetrapal)
{
	return simplex_size(tetrapal);
}

int tetrapal_get_elements(const Tetrapal* tetrapal, int* buffer)
{
	if (tetrapal == NULL)
		return ERROR_INVALID_ARGUMENT;

	const int stride = tetrapal_element_size(tetrapal);
	int count = 0;

	switch (tetrapal->dimensions)
	{
	case 0:
		buffer[0] = 0;
		return ERROR_NONE;

	case 1:

		for (size_t i = 0; i < tetrapal->vertices.count - 1; i++)
		{
			buffer[count * stride + 0] = (int)tetrapal->vertices.tree[i + 0];
			buffer[count * stride + 1] = (int)tetrapal->vertices.tree[i + 1];
			count += 1;
		}

		return ERROR_NONE;

	case 2:
	case 3:

		for (size_t i = 0; i < tetrapal->simplices.count; i++)
		{
			/* Skip infinite simplices. */
			if (is_infinite_simplex(tetrapal, (simplex_t)i) == true)
				continue;

			/* Skip free simplices. */
			if (is_free_simplex(tetrapal, (simplex_t)i) == true)
				continue;

			for (int j = 0; j < stride; j++)
			{
				buffer[count * stride + j] = (int)tetrapal->simplices.incident_vertex[i * (size_t)stride + (size_t)j];
			}

			count += 1;
		}

		return ERROR_NONE;
	}

	return ERROR_INVALID_ARGUMENT;
}

/* Static (internal) functions. */

static vertex_t new_vertex(Tetrapal* tetrapal, const coord_t* p)
{
	const vertex_t v = (vertex_t)tetrapal->vertices.count;

	for (local_t i = 0; i < tetrapal->dimensions; i++)
		tetrapal->vertices.coordinates[(size_t)v * tetrapal->dimensions + i] = p[i];

	tetrapal->vertices.count += 1;

	return v;
}

static error_t free_simplex(Tetrapal* tetrapal, simplex_t t)
{
	TETRAPAL_ASSERT(tetrapal->simplices.count > 0, "No more simplices left to free!");

	if (check_deleted_capacity(tetrapal) != ERROR_NONE)
		return ERROR_OUT_OF_MEMORY;

	const size_t num_deleted = tetrapal->simplices.deleted.count;
	tetrapal->simplices.deleted.simplices[num_deleted] = t;
	tetrapal->simplices.flags[t].bit.is_free = true;

	tetrapal->simplices.deleted.count += 1;
	tetrapal->simplices.count -= 1;

	return ERROR_NONE;
}

static inline void set_adjacent_simplex(Tetrapal* tetrapal, simplex_t t, simplex_t a, local_t i)
{
	tetrapal->simplices.adjacent_simplex[t * simplex_size(tetrapal) + i] = a;
}

static inline vertex_t get_incident_vertex(const Tetrapal* tetrapal, simplex_t t, local_t i)
{
	return tetrapal->simplices.incident_vertex[t * simplex_size(tetrapal) + i];
}

static inline simplex_t get_adjacent_simplex(const Tetrapal* tetrapal, simplex_t t, local_t i)
{
	return tetrapal->simplices.adjacent_simplex[t * simplex_size(tetrapal) + i];
}

static inline simplex_t get_incident_simplex(const Tetrapal* tetrapal, vertex_t v)
{
	return tetrapal->vertices.incident_simplex[v];
}

static inline void get_circumcentre(const Tetrapal* tetrapal, simplex_t t, coord_t* result)
{
	switch (tetrapal->dimensions)
	{
	case 2:
		circumcentre_2d(
			&tetrapal->vertices.coordinates[tetrapal->simplices.incident_vertex[t * 3 + 0] * 2],
			&tetrapal->vertices.coordinates[tetrapal->simplices.incident_vertex[t * 3 + 1] * 2],
			&tetrapal->vertices.coordinates[tetrapal->simplices.incident_vertex[t * 3 + 2] * 2],
			result);
		return;

	case 3:
		circumcentre_3d(
			&tetrapal->vertices.coordinates[tetrapal->simplices.incident_vertex[t * 4 + 0] * 3],
			&tetrapal->vertices.coordinates[tetrapal->simplices.incident_vertex[t * 4 + 1] * 3],
			&tetrapal->vertices.coordinates[tetrapal->simplices.incident_vertex[t * 4 + 2] * 3],
			&tetrapal->vertices.coordinates[tetrapal->simplices.incident_vertex[t * 4 + 3] * 3],
			result);
		return;

	default:
		return;
	}
}

static inline const coord_t* get_coordinates(const Tetrapal* tetrapal, vertex_t v)
{
	TETRAPAL_ASSERT(v != VERTEX_INFINITE, "Attempted to get the coordinates of an infinite vertex!");

	return &tetrapal->vertices.coordinates[(size_t)v * tetrapal->dimensions];
}

static void get_facet_normal(const Tetrapal* tetrapal, simplex_t t, local_t i, coord_t result[3])
{
	/* Get the coordinates of the facet. */
	const vertex_t v[3] =
	{
		get_incident_vertex(tetrapal, t, facet_opposite_vertex[i][0]),
		get_incident_vertex(tetrapal, t, facet_opposite_vertex[i][1]),
		get_incident_vertex(tetrapal, t, facet_opposite_vertex[i][2])
	};

	TETRAPAL_ASSERT(
		v[0] != VERTEX_INFINITE &&
		v[1] != VERTEX_INFINITE &&
		v[2] != VERTEX_INFINITE,
		"Attempted to get the normal of an infinite facet!");

	const coord_t* p[3] =
	{
		get_coordinates(tetrapal, v[0]),
		get_coordinates(tetrapal, v[1]),
		get_coordinates(tetrapal, v[2]),
	};

	/* Calculate normal. */
	coord_t ab[3], ac[3], cross[3];
	sub_3d(p[1], p[0], ab);
	sub_3d(p[2], p[0], ac);
	cross_3d(ab, ac, cross);
	normalise_3d(cross, result);
}

static inline local_t find_vertex(const Tetrapal* tetrapal, simplex_t t, vertex_t v)
{
	const vertex_t* vi = &tetrapal->simplices.incident_vertex[t * simplex_size(tetrapal)];

	/* Fast branchless check borrowed from Geogram. */
	local_t i = 0;

	switch (tetrapal->dimensions)
	{
	case 2:
		i = (local_t)((vi[1] == v) | ((vi[2] == v) * 2));
		break;

	case 3:
		i = (local_t)((vi[1] == v) | ((vi[2] == v) * 2) | ((vi[3] == v) * 3));
		break;
	}

	TETRAPAL_ASSERT(get_incident_vertex(tetrapal, t, i) == v, "Could not find vertex in simplex!");

	return i;
}

static inline local_t find_adjacent(const Tetrapal* tetrapal, simplex_t t, simplex_t adj)
{
	const simplex_t* ta = &tetrapal->simplices.adjacent_simplex[t * simplex_size(tetrapal)];

	/* Fast branchless check borrowed from Geogram. */
	local_t i = 0;

	switch (tetrapal->dimensions)
	{
	case 2:
		i = (local_t)((ta[1] == adj) | ((ta[2] == adj) * 2));
		break;

	case 3:
		i = (local_t)((ta[1] == adj) | ((ta[2] == adj) * 2) | ((ta[3] == adj) * 3));
		break;
	}

	return i;
}

static inline local_t find_facet_from_edge(const Tetrapal* tetrapal, simplex_t t, vertex_t a, vertex_t b)
{
	const vertex_t* v = &tetrapal->simplices.incident_vertex[t * simplex_size(tetrapal)];

	/* Fast branchless check borrowed from Geogram. */
	const local_t i = (local_t)((v[1] == a) | ((v[2] == a) * 2) | ((v[3] == a) * 3));
	const local_t j = (local_t)((v[1] == b) | ((v[2] == b) * 2) | ((v[3] == b) * 3));

	TETRAPAL_ASSERT(get_incident_vertex(tetrapal, t, i) == a, "Could not find vertex [a] from edge in simplex!");
	TETRAPAL_ASSERT(get_incident_vertex(tetrapal, t, j) == b, "Could not find vertex [b] from edge in simplex!");

	return facet_from_edge[i][j];
}

static inline bool is_infinite_simplex(const Tetrapal* tetrapal, simplex_t t)
{
	TETRAPAL_ASSERT(t < (tetrapal->simplices.count + tetrapal->simplices.deleted.count), "Simplex index out of range!");

	return (bool)tetrapal->simplices.flags[t].bit.is_infinite;
}

static inline bool is_free_simplex(const Tetrapal* tetrapal, simplex_t t)
{
	TETRAPAL_ASSERT(t < (tetrapal->simplices.count + tetrapal->simplices.deleted.count), "Simplex index out of range!");

	return (bool)tetrapal->simplices.flags[t].bit.is_free;
}

static inline bool is_coincident_simplex(const Tetrapal* tetrapal, simplex_t t, const float point[3])
{
	/* Check whether the query point is coincident with a vertex. */
	for (local_t i = 0; i < simplex_size(tetrapal); i++)
	{
		const vertex_t v = get_incident_vertex(tetrapal, t, i);

		if (v == VERTEX_INFINITE)
			continue;

		if (is_coincident_3d(point, get_coordinates(tetrapal, v)) == true)
			return true;
	}

	return false;
}

static inline local_t simplex_size(const Tetrapal* tetrapal)
{
	return (local_t)(tetrapal->dimensions + 1);
}

static error_t check_simplices_capacity(Tetrapal* tetrapal)
{
	if (tetrapal->simplices.count + tetrapal->simplices.deleted.count < tetrapal->simplices.capacity)
		return ERROR_NONE;

	/* Arrays are at capacity; resize. */
	size_t new_capacity = (tetrapal->simplices.capacity * (size_t)ARRAY_GROWTH_FACTOR) + 1;
	void* new_incident = TETRAPAL_REALLOC(tetrapal->simplices.incident_vertex, sizeof(*tetrapal->simplices.incident_vertex) * new_capacity * simplex_size(tetrapal));
	void* new_adjacent = TETRAPAL_REALLOC(tetrapal->simplices.adjacent_simplex, sizeof(*tetrapal->simplices.adjacent_simplex) * new_capacity * simplex_size(tetrapal));
	void* new_flags = TETRAPAL_REALLOC(tetrapal->simplices.flags, sizeof(*tetrapal->simplices.flags) * new_capacity);

	if (new_incident == NULL || new_adjacent == NULL || new_flags == NULL)
	{
		TETRAPAL_FREE(new_incident);
		TETRAPAL_FREE(new_adjacent);
		TETRAPAL_FREE(new_flags);
		return ERROR_OUT_OF_MEMORY;
	}

	tetrapal->simplices.capacity = new_capacity;
	tetrapal->simplices.incident_vertex = new_incident;
	tetrapal->simplices.adjacent_simplex = new_adjacent;
	tetrapal->simplices.flags = new_flags;

	return ERROR_NONE;
}

static error_t check_deleted_capacity(Tetrapal* tetrapal)
{
	if (tetrapal->simplices.deleted.count < tetrapal->simplices.deleted.capacity)
		return ERROR_NONE;

	/* Arrays are at capacity; resize. */
	size_t new_capacity = (tetrapal->simplices.deleted.capacity * (size_t)ARRAY_GROWTH_FACTOR) + 1;
	void* new_data = TETRAPAL_REALLOC(tetrapal->simplices.deleted.simplices, sizeof(*tetrapal->simplices.deleted.simplices) * new_capacity);

	if (new_data == NULL)
		return ERROR_OUT_OF_MEMORY;

	tetrapal->simplices.deleted.capacity = new_capacity;
	tetrapal->simplices.deleted.simplices = new_data;

	return ERROR_NONE;
}

static size_t find_first_simplex(Tetrapal* tetrapal, const float* points, const int size, vertex_t v[4])
{
	size_t num_dimensions = 0;
	coord_t p[4][3] = { 0 };

	/* Get the first coordinate (i.e. the first vertex in the triangulation). */
	v[0] = 0;
	transform_3d(&points[v[0] * 3], p[0]);

	/* Iterate over all the points to determine the affine span of the set. */
	for (vertex_t i = 1; i < (vertex_t)size; i++)
	{
		switch (num_dimensions)
		{
		case 0:

			v[1] = i;
			transform_3d(&points[v[1] * 3], p[1]);

			if (is_coincident_3d(p[0], p[1]) == false)
				num_dimensions = 1;

			break;

		case 1:

			v[2] = i;
			transform_3d(&points[v[2] * 3], p[2]);

			if (is_colinear_3d(p[0], p[1], p[2]) == false)
				num_dimensions = 2;

			break;

		case 2:

			v[3] = i;
			transform_3d(&points[v[3] * 3], p[3]);

			if (is_coplanar_3d(p[0], p[1], p[2], p[3]) == false)
				num_dimensions = 3;

			break;
		}

		if (num_dimensions == 3)
			break;
	}

	/* Set the number of dimensions internally. */
	tetrapal->dimensions = num_dimensions;
	return num_dimensions;
}

static inline long xrandom(random_t* seed)
{
	*seed = 214013u * *seed + 2531011u;
	return (long int)((*seed >> 16) & RANDOM_MAX);
}

static inline random_t random_range(random_t* seed, random_t range)
{
	return (random_t)((size_t)xrandom(seed) / (RANDOM_MAX / range + 1));
}

static inline void swap_vertex(vertex_t* a, vertex_t* b)
{
	vertex_t t = *a;
	*a = *b;
	*b = t;
}

static inline void swap_local(local_t* a, local_t* b)
{
	local_t t = *a;
	*a = *b;
	*b = t;
}

#ifdef TETRAPAL_DEBUG
static bool is_enclosing_simplex(Tetrapal* tetrapal, simplex_t t, const float point[3])
{
	/* Get the vertices' coordinates. */
	const coord_t* p[4];

	for (local_t i = 0; i < simplex_size(tetrapal); i++)
	{
		const vertex_t v = get_incident_vertex(tetrapal, t, i);

		/* We represent the coordinates of an infinite vertex as a NULL pointer. */
		p[i] = (v == VERTEX_INFINITE) ? NULL : get_coordinates(tetrapal, v);
	}

	/* If [t] is an infinite simplex, check if the point lies on the positive side of the finite facet. */
	if (tetrapal->dimensions == 3)
	{
		for (local_t i = 0; i < 4; i++)
		{
			if (p[i] != NULL)
				continue;

			const local_t a = facet_opposite_vertex[i][0];
			const local_t b = facet_opposite_vertex[i][1];
			const local_t c = facet_opposite_vertex[i][2];

			if (orient_3d(point, p[a], p[b], p[c]) >= 0)
				return true;
			else
				return false;
		}

		/* Its not an infinite simplex, so check the orientation against each face. */
		if (orient_3d(point, p[1], p[2], p[3]) >= 0 &&
			orient_3d(p[0], point, p[2], p[3]) >= 0 &&
			orient_3d(p[0], p[1], point, p[3]) >= 0 &&
			orient_3d(p[0], p[1], p[2], point) >= 0)
			return true;
		else
			return false;
	}
	else if (tetrapal->dimensions == 2)
	{
		for (local_t i = 0; i < 3; i++)
		{
			if (p[i] != NULL)
				continue;

			const local_t a = edge_opposite_vertex[i][0];
			const local_t b = edge_opposite_vertex[i][1];

			if (orient_2d(point, p[a], p[b]) >= 0)
				return true;
			else
				return false;
		}

		/* Its not an infinite simplex, so check the orientation against each face. */
		if (orient_2d(point, p[1], p[2]) >= 0 &&
			orient_2d(p[0], point, p[2]) >= 0 &&
			orient_2d(p[0], p[1], point) >= 0)
			return true;
		else
			return false;
	}
	else return false;
}

static void check_combinatorics(Tetrapal* tetrapal)
{
	int error = 0;
	size_t count = tetrapal->simplices.count;
	size_t freed = tetrapal->simplices.deleted.count;

	for (simplex_t t = 0; t < (simplex_t)(count + freed); t++)
	{
		if (is_free_simplex(tetrapal, t))
			continue;

		/* Check adjacencies. */
		for (local_t i = 0; i < simplex_size(tetrapal); i++)
		{
			const simplex_t adj = get_adjacent_simplex(tetrapal, t, i);

			/* Check that adjacencies are set. */
			if (adj == SIMPLEX_NULL)
			{
				printf("Simplex [%lu] is adjacent to a null simplex at [%i]!\n", t, i);
				error++;
				continue;
			}

			/* Check that there are no relations to free simplices. */
			if (is_free_simplex(tetrapal, adj))
			{
				printf("Simplex [%lu] is adjacent to a free simplex [%lu] at [%i]!\n", t, adj, i);
				error++;
			}

			/* Check that there are no self-relations. */
			if (adj == t)
			{
				printf("Simplex [%lu] is adjacent to itself at [%i]!\n", t, i);
				error++;
			}

			/* An adjacent simplex opposite the infinite vertex must be finite. */
			if (is_infinite_simplex(tetrapal, adj) && get_incident_vertex(tetrapal, t, i) == VERTEX_INFINITE)
			{
				printf("Simplex [%lu] has an adjacent infinite simplex [%lu] opposite an infinite vertex at [%i]!\n", t, adj, i);
				error++;
			}

			/* Check that relations are bi-directional. */
			if (get_adjacent_simplex(tetrapal, adj, find_adjacent(tetrapal, adj, t)) != t)
			{
				printf("Simplex [%lu] lacks bi-directional adjacency with [%lu] at [%i]!\n", t, adj, i);
				error++;
			}
		}

		/* Check vertex incidence. */
		for (local_t i = 0; i < simplex_size(tetrapal); i++)
		{
			/* Check that there are no duplicate vertices. */
			for (local_t j = i + 1; j < simplex_size(tetrapal); j++)
			{
				const vertex_t v[2] =
				{
					get_incident_vertex(tetrapal, t, i),
					get_incident_vertex(tetrapal, t, j)
				};

				if (v[0] == v[1])
				{
					printf("Simplex [%lu] has duplicate vertex [%li]\n", t, v[0]);
					error++;
				}
			}
		}
	}

	TETRAPAL_ASSERT(error == 0, "Combinatorial data is corrupted!");
}

static void print_simplex_data(Tetrapal* tetrapal)
{
	const size_t count = tetrapal->simplices.count;
	const size_t capacity = tetrapal->simplices.capacity;
	const size_t freed = tetrapal->simplices.deleted.count;

	printf("** PRINTING SIMPLEX DATA **\n");
	printf("Count: %zu\n", count);
	printf("Capacity: %zu\n", capacity);
	printf("Free: %zu\n", freed);

	for (simplex_t t = 0; t < count + freed; t++)
	{
		if (is_free_simplex(tetrapal, t))
		{
			printf("[FREE] ");
		}

		printf("IDX: [%lu] V: [ ", t);

		for (local_t i = 0; i < simplex_size(tetrapal); i++)
		{
			printf("%li ", get_incident_vertex(tetrapal, t, i));
		}

		printf("] T: [ ");

		for (local_t i = 0; i < simplex_size(tetrapal); i++)
		{
			printf("%li ", get_adjacent_simplex(tetrapal, t, i));
		}

		printf("]\n");
	}
}

static void print_vertex_data(Tetrapal* tetrapal)
{
	const size_t count = tetrapal->vertices.count;
	const size_t capacity = tetrapal->vertices.capacity;

	printf("** PRINTING VERTEX DATA **\n");
	printf("Count: %zu\n", count);
	printf("Capacity: %zu\n", capacity);

	for (size_t i = 0; i < count; i++)
	{
		printf("IDX: [%zu] COORDS: [ ", i);

		for (local_t j = 0; j < tetrapal->dimensions; j++)
		{
			printf("%.5f ", (float)tetrapal->vertices.coordinates[i * tetrapal->dimensions + j]);
		}

		printf("]\n");
	}
}

static void print_memory(Tetrapal* tetrapal)
{
	printf("** PRINTING MEMORY DATA **\n");

	size_t memory_struct = sizeof(*tetrapal);

	size_t memory_simplices =
		sizeof(*tetrapal->simplices.adjacent_simplex) * tetrapal->simplices.capacity * simplex_size(tetrapal) +
		sizeof(*tetrapal->simplices.incident_vertex) * tetrapal->simplices.capacity * simplex_size(tetrapal) +
		sizeof(*tetrapal->simplices.flags) * tetrapal->simplices.capacity;

	size_t memory_vertices =
		sizeof(*tetrapal->vertices.coordinates) * tetrapal->vertices.count * tetrapal->dimensions +
		sizeof(*tetrapal->vertices.incident_simplex) * tetrapal->vertices.count +
		sizeof(*tetrapal->vertices.tree) * tetrapal->vertices.count;

	size_t memory_total = memory_struct + memory_vertices + memory_simplices;

	printf("MEMORY (STRUCT): %zu KB\n", memory_struct / 1000);
	printf("MEMORY (VERTICES): %zu KB\n", memory_vertices / 1000);
	printf("MEMORY (ADJACENCY): %zu KB\n", memory_simplices / 1000);
	printf("TOTAL MEMORY: %zu KB\n", memory_total / 1000);
}
#endif

/********************************/
/*		3D Triangulation		*/
/********************************/

static error_t triangulate_3d(Tetrapal* tetrapal, vertex_t v[4], const float* points, const int size)
{
	/* Allocate memory. */
	size_t estimated_num_elements = (size_t)size * 7;

	tetrapal->vertices.capacity = (size_t)size;
	tetrapal->vertices.coordinates = TETRAPAL_MALLOC(sizeof(*tetrapal->vertices.coordinates) * (size_t)size * 3);
	tetrapal->vertices.incident_simplex = TETRAPAL_MALLOC(sizeof(*tetrapal->vertices.incident_simplex) * (size_t)size);
	tetrapal->vertices.tree = TETRAPAL_MALLOC(sizeof(*tetrapal->vertices.tree) * (size_t)size);

	tetrapal->simplices.capacity = estimated_num_elements;
	tetrapal->simplices.deleted.capacity = (size_t)size;
	tetrapal->simplices.incident_vertex = TETRAPAL_MALLOC(sizeof(*tetrapal->simplices.incident_vertex) * estimated_num_elements * 4);
	tetrapal->simplices.adjacent_simplex = TETRAPAL_MALLOC(sizeof(*tetrapal->simplices.adjacent_simplex) * estimated_num_elements * 4);
	tetrapal->simplices.flags = TETRAPAL_MALLOC(sizeof(*tetrapal->simplices.flags) * estimated_num_elements);
	tetrapal->simplices.deleted.simplices = TETRAPAL_MALLOC(sizeof(*tetrapal->simplices.deleted.simplices) * (size_t)size);

	/* Memory allocation failed? */
	if (tetrapal->vertices.coordinates == NULL ||
		tetrapal->vertices.incident_simplex == NULL ||
		tetrapal->vertices.tree == NULL ||
		tetrapal->simplices.incident_vertex == NULL ||
		tetrapal->simplices.adjacent_simplex == NULL ||
		tetrapal->simplices.flags == NULL ||
		tetrapal->simplices.deleted.simplices == NULL)
		return ERROR_OUT_OF_MEMORY;

	/* Initialise secondary structures. */
	if (stack_init(&tetrapal->stack, 32) != ERROR_NONE)
		return ERROR_OUT_OF_MEMORY;

	if (cavity_init(&tetrapal->cavity, (size_t)size) != ERROR_NONE)
		return ERROR_OUT_OF_MEMORY;

	tetrapal->vertices.count = 0;
	tetrapal->simplices.count = 0;
	tetrapal->simplices.deleted.count = 0;

	/* Add all points to the vertex array. */
	for (int i = 0; i < size; i++)
	{
		coord_t tmp[3];
		transform_3d(&points[i * 3], tmp);
		new_vertex(tetrapal, tmp);
	}

	/* Inistialise and  build the KD Tree */
	for (vertex_t i = 0; i < (vertex_t)size; i++)
		tetrapal->vertices.tree[i] = i;

	if (kdtree_balance(tetrapal, 0, (size_t)size - 1, 0) != ERROR_NONE)
		return ERROR_OUT_OF_MEMORY;

	/* Get the coordinates of the first simplex. */
	const coord_t* p[4];

	for (local_t i = 0; i < 4; i++)
		p[i] = get_coordinates(tetrapal, v[i]);

	/* Ensure positive orientation. */
	if (orient_3d(p[0], p[1], p[2], p[3]) < 0)
		swap_vertex(&v[0], &v[1]);

	/* Create the first tetrahedron. */
	simplex_t t = new_tetrahedron(tetrapal, v[0], v[1], v[2], v[3]);

	if (t == SIMPLEX_NULL)
		return ERROR_OUT_OF_MEMORY;

	/* Create the infinite tetrahedra. */
	simplex_t inf[4];

	for (local_t i = 0; i < 4; i++)
	{
		/* Get the facet indices opposite [t]'s vertex [i], enumerate backwards so that it is from pov of inf[i]. */
		vertex_t a = get_incident_vertex(tetrapal, t, facet_opposite_vertex[i][2]);
		vertex_t b = get_incident_vertex(tetrapal, t, facet_opposite_vertex[i][1]);
		vertex_t c = get_incident_vertex(tetrapal, t, facet_opposite_vertex[i][0]);
		inf[i] = new_tetrahedron(tetrapal, VERTEX_INFINITE, a, b, c);

		if (inf[i] == SIMPLEX_NULL)
			return ERROR_OUT_OF_MEMORY;

		/* Set adjacencies across the finite tetrahedron. */
		set_adjacent_simplex(tetrapal, t, inf[i], i);
		set_adjacent_simplex(tetrapal, inf[i], t, 0);
	}

	/* Set adjacencies across infinite tetrahedra. */
	for (local_t i = 0; i < 4; i++)
	{
		set_adjacent_simplex(tetrapal, inf[i], inf[facet_opposite_vertex[i][2]], 1);
		set_adjacent_simplex(tetrapal, inf[i], inf[facet_opposite_vertex[i][1]], 2);
		set_adjacent_simplex(tetrapal, inf[i], inf[facet_opposite_vertex[i][0]], 3);
	}

	/* Insert vertices one-by-one. */
	for (vertex_t i = 0; i < (vertex_t)size; i++)
	{
		/* Ensure we don't re-insert a vertex from the starting simplex. */
		if (i == v[0] || i == v[1] || i == v[2] || i == v[3])
			continue;

		if (insert_3d(tetrapal, i) != ERROR_NONE)
			return ERROR_OUT_OF_MEMORY;
	}

	/* Set vertex-to-simplex incidence. */
	for (vertex_t i = 0; i < (vertex_t)size; i++)
	{
		t = locate_3d(tetrapal, &tetrapal->vertices.coordinates[i * 3]);
		tetrapal->vertices.incident_simplex[i] = t;

		TETRAPAL_ASSERT(is_infinite_simplex(tetrapal, t) == false, "Incident simplex was infinite!\n");
	}

	/* Free intermediate structures. */
	TETRAPAL_FREE(tetrapal->simplices.deleted.simplices);
	tetrapal->simplices.deleted.simplices = NULL;
	stack_free(&tetrapal->stack);
	cavity_free(&tetrapal->cavity);

	return ERROR_NONE;
}

static error_t insert_3d(Tetrapal* tetrapal, vertex_t v)
{
	/* Add this point to the vertex array. */
	const coord_t* p = get_coordinates(tetrapal, v);

	/* Find the enclosing simplex of the input point. */
	simplex_t t = locate_3d(tetrapal, p);

	/* Check whether the input point is coincident with a vertex of the enclosing simplex.
		We still leave the vertex in the structure for consistency, but we don't triangulate it. */
	if (is_coincident_simplex(tetrapal, t, p) == true)
		return ERROR_NONE;

	/* Remove conflict simplices and triangulate the cavity. */
	t = stellate_3d(tetrapal, v, t);

	if (t == SIMPLEX_NULL)
		return ERROR_OUT_OF_MEMORY;

	/* Success! */
	return ERROR_NONE;
}

static simplex_t locate_3d(const Tetrapal* tetrapal, const coord_t point[3])
{
	/* Start from the last finite simplex. */
	simplex_t t = tetrapal->simplices.last;
	simplex_t t_prev = SIMPLEX_NULL; /* The simplex we just walked from. */

	/* Local seed for rng. */
	random_t seed = (random_t)t;

	/* Walk the triangulation until an enclosing simplex is found. */
WALK:
	{
		/* Check if we are at an infinite simplex. */
		if (is_infinite_simplex(tetrapal, t))
			return t;

		/* Get the vertices of the current simplex. */
		const vertex_t v[4] =
		{
			get_incident_vertex(tetrapal, t, 0),
			get_incident_vertex(tetrapal, t, 1),
			get_incident_vertex(tetrapal, t, 2),
			get_incident_vertex(tetrapal, t, 3)
		};

		/* Get the coordinates of each vertex for the current simplex. */
		const coord_t* p[4] =
		{
			get_coordinates(tetrapal, v[0]),
			get_coordinates(tetrapal, v[1]),
			get_coordinates(tetrapal, v[2]),
			get_coordinates(tetrapal, v[3])
		};

		/* Start from a random facet (stochastic walk). */
		const random_t r = random_range(&seed, 4);

		/* Test the orientation against every facet until a negative one is found. */
		for (int j = 0; j < 4; j++)
		{
			const local_t f = (local_t)((size_t)j + r) % 4;
			const simplex_t ta = get_adjacent_simplex(tetrapal, t, f);

			/* If we just came from this simplex, skip it. */
			if (ta == t_prev)
				continue;

			const local_t a = facet_opposite_vertex[f][0];
			const local_t b = facet_opposite_vertex[f][1];
			const local_t c = facet_opposite_vertex[f][2];

			/* If the orientation is negative, we should move towards the adjacent simplex. */
			if (orient_3d(point, p[a], p[b], p[c]) < 0)
			{
				t_prev = t;
				t = ta;
				goto WALK;
			}
		}

		return t;
	}
}

static simplex_t stellate_3d(Tetrapal* tetrapal, vertex_t v, simplex_t t)
{
	TETRAPAL_ASSERT(is_enclosing_simplex(tetrapal, t, get_coordinates(tetrapal, v)) == true, "Starting simplex does not enclose the point!\n");

	/* Reset the cavity struct. */
	Cavity* cavity = &tetrapal->cavity;
	cavity_clear(cavity);

	/* Insert the first conflict simplex [t] into the stack. */
	Stack* stack = &tetrapal->stack;
	stack_clear(stack);

	if (stack_insert(stack, t) != ERROR_NONE)
		return SIMPLEX_NULL;

	/* Mark the first simplex as free, since we know it should already be in conflict. */
	if (free_simplex(tetrapal, t) != ERROR_NONE)
		return SIMPLEX_NULL;

	/* Get the coordinates of the vertex. */
	const coord_t* p = get_coordinates(tetrapal, v);

	/* Perform depth-search traversal of conflict zone until there are no more simplices to check.*/
	while (stack_is_empty(stack) == false)
	{
		/* Get and pop the simplex at the top of the stack. */
		t = stack_top(stack);
		stack_pop(stack);

		/* Check every adjacent tetrahedron for conflict. */
		for (local_t i = 0; i < 4; i++)
		{
			simplex_t adj = get_adjacent_simplex(tetrapal, t, i);

			/* If the simplex is free, then it has already been processed. */
			if (is_free_simplex(tetrapal, adj) == true)
				continue;

			/* If it is in conflict, free the simplex and add it to the stack. */
			if (conflict_3d(tetrapal, adj, p) == true)
			{
				if (stack_insert(stack, adj) != ERROR_NONE)
					return SIMPLEX_NULL;

				if (free_simplex(tetrapal, adj) != ERROR_NONE)
					return SIMPLEX_NULL;

				continue;
			}

			/* It is not in conflict, so add the shared facet to the cavity. */
			const local_t a = facet_opposite_vertex[i][0];
			const local_t b = facet_opposite_vertex[i][1];
			const local_t c = facet_opposite_vertex[i][2];

			/* Facet vertices are given in positive orientation wrt the inside of the cavity. */
			const vertex_t vf[3] =
			{
				get_incident_vertex(tetrapal, t, a),
				get_incident_vertex(tetrapal, t, b),
				get_incident_vertex(tetrapal, t, c)
			};

			if (cavity_insert(cavity, vf[0], vf[1], vf[2], adj, find_adjacent(tetrapal, adj, t)) == FACET_NULL)
				return SIMPLEX_NULL;
		}
	}

	/* Now stellate (triangulate) the cavity by connecting every facet to [v]. */
	for (facet_t f = 0; (size_t)f < cavity->facets.count; f++)
	{
		/* Facet vertices are in positive orientation wrt [v]. */
		const vertex_t vf[3] =
		{
			cavity_get_incident_vertex(cavity, f, 0),
			cavity_get_incident_vertex(cavity, f, 1),
			cavity_get_incident_vertex(cavity, f, 2)
		};

		/* Create a new simplex from the facet. */
		t = new_tetrahedron(tetrapal, v, vf[0], vf[1], vf[2]);

		/* Check if we failed to create a new simplex. */
		if (t == SIMPLEX_NULL)
			return SIMPLEX_NULL;

		/* Connect the new simplex to the boundary simplex. */
		const simplex_t adj = cavity_get_adjacent_simplex(cavity, f);
		set_adjacent_simplex(tetrapal, t, adj, 0);
		set_adjacent_simplex(tetrapal, adj, t, cavity_get_adjacent_simplex_facet(cavity, f));

		/* Update the facet's adjacent simplex such that it now represents the new cavity simplex. */
		cavity_set_adjacent_simplex(cavity, f, t);
	}

	/* Repair adjacency relationships across the new simplices. */
	for (facet_t f = 0; (size_t)f < cavity->facets.count; f++)
	{
		/* Get the cavity simplex associated with this facet. */
		t = cavity_get_adjacent_simplex(cavity, f);

		/* Get the facet's incident vertices. */
		const vertex_t vf[3] =
		{
			cavity_get_incident_vertex(cavity, f, 0),
			cavity_get_incident_vertex(cavity, f, 1),
			cavity_get_incident_vertex(cavity, f, 2)
		};

		/* Get the facets neighbouring each edge. */
		const facet_t fa[3] =
		{
			cavity_find(cavity, vf[1], vf[0]),
			cavity_find(cavity, vf[2], vf[1]),
			cavity_find(cavity, vf[0], vf[2])
		};

		/* Get the simplices adjacent to each neighbouring facet. */
		const simplex_t ta[3] =
		{
			cavity_get_adjacent_simplex(cavity, fa[0]),
			cavity_get_adjacent_simplex(cavity, fa[1]),
			cavity_get_adjacent_simplex(cavity, fa[2])
		};

		/* Link the current simplex to each neighbouring simplex. */
		set_adjacent_simplex(tetrapal, t, ta[0], 3);
		set_adjacent_simplex(tetrapal, t, ta[1], 1);
		set_adjacent_simplex(tetrapal, t, ta[2], 2);
	}

	return t;
}

static bool conflict_3d(const Tetrapal* tetrapal, simplex_t t, const coord_t point[3])
{
	TETRAPAL_ASSERT(t < tetrapal->simplices.count + tetrapal->simplices.deleted.count, "Simplex index out of range!");

	/* Get the coordinates of each vertex for the current simplex. */
	const coord_t* p[4];

	for (local_t i = 0; i < 4; i++)
	{
		const vertex_t v = get_incident_vertex(tetrapal, t, i);

		/* We represent the coordinates of an infinite vertex as a NULL pointer. This idea is borrowed from Geogram. */
		p[i] = (v == VERTEX_INFINITE) ? NULL : get_coordinates(tetrapal, v);
	}

	/* The rules on testing the conflict zone for finite and infinite simplices are slightly different.
		For finite simplices, we can do a simple insphere/incircle test as normal.
		For infinite simplices, we must perform an orientation test on the finite facet.
		If the point lies on the inner side of the plane defined by the finite facet, then it is in conflict.
		If the point is directly on the plane however, we must do another test to check whether it is on the facet's circumcircle (i.e. in conflict). */

		/* Look for the infinite vertex if there is one. */
	for (local_t i = 0; i < 4; i++)
	{
		/* Ignore finite vertices. */
		if (p[i] != NULL)
			continue;

		const local_t a = facet_opposite_vertex[i][0];
		const local_t b = facet_opposite_vertex[i][1];
		const local_t c = facet_opposite_vertex[i][2];

		/* Get the orientation wrt the finite facet.*/
		const coord_t orientation = orient_3d(point, p[a], p[b], p[c]);

		/* If the orientation is positive, then this simplex is in conflict. If negative, then it is not. */
		if (orientation > 0)
			return true;

		if (orientation < 0)
			return false;

		/* If the orientation is exactly 0, then we need to check whether it lies on the facet's circumcircle.
			This is equivalent to checking whether the adjacent finite simplex is in conflict with the point. */
		const simplex_t adj = get_adjacent_simplex(tetrapal, t, i);
		TETRAPAL_ASSERT(is_infinite_simplex(tetrapal, adj) == false, "Adjacent simplex to infinite vertex should be finite!");

		/* Because we stellate depth-first, the simplex would have already been freed if it was in conflict.
			This saves us an insphere/incircle test. */
		if (is_free_simplex(tetrapal, adj))
			return true;
		else
			return (conflict_3d(tetrapal, adj, point));
	}

	/* The simplex is finite, so we do a regular insphere/incircle test. */
	if (insphere_3d(p[0], p[1], p[2], p[3], point) > 0)
		return true;
	else
		return false;
}

static size_t interpolate_3d(const Tetrapal* tetrapal, const coord_t point[3], int indices[4], float weights[4], simplex_t* t)
{
	vertex_t v[4]; /* Current simplex vertex indices. */
	const coord_t* p[4]; /* Current simplex vertex coordinates. */
	coord_t orient[4]; /* Orientation for each face. */
	simplex_t t_prev = SIMPLEX_NULL; /* The simplex we just walked from. */

	/* Find an appropriate starting simplex. */
	v[0] = kdtree_get_vertex(tetrapal, kdtree_find_approximate(tetrapal, point));
	*t = get_incident_simplex(tetrapal, v[0]);
	random_t seed = (random_t)*t; /* Local seed for rng. */

	/* Walk the triangulation until an enclosing simplex is found. */
WALK_START:
	{
		/* Check if we are at an infinite simplex. */
		if (is_infinite_simplex(tetrapal, *t))
			goto WALK_FACET;

		/* Get the vertex data for the current simplex. */
		for (local_t i = 0; i < 4; i++)
		{
			v[i] = get_incident_vertex(tetrapal, *t, i);
			p[i] = get_coordinates(tetrapal, v[i]);
		}

		/* Start from a random facet (stochastic walk). */
		const random_t r = random_range(&seed, 4);

		/* Test the orientation against every facet until a negative one is found. */
		for (int i = 0; i < 4; i++)
		{
			local_t f = (local_t)((size_t)i + r) % 4;
			local_t a = facet_opposite_vertex[f][0];
			local_t b = facet_opposite_vertex[f][1];
			local_t c = facet_opposite_vertex[f][2];
			simplex_t adj = get_adjacent_simplex(tetrapal, *t, f);

			/* Get the orientation. */
			orient[f] = orient_3d(point, p[a], p[b], p[c]);

			/* If the orientation is negative, move towards the adjacent simplex. */
			if (orient[f] < 0 && adj != t_prev)
			{
				t_prev = *t;
				*t = adj;
				goto WALK_START;
			}
		}

		/* This is the enclosing simplex. Calculate barycentric weights from the orientation results. */
		const float total = (float)(orient[0] + orient[1] + orient[2] + orient[3]);
		const float inverse = 1.0f / total;

		indices[0] = (int)v[0];
		indices[1] = (int)v[1];
		indices[2] = (int)v[2];
		indices[3] = (int)v[3];
		weights[0] = orient[0] * inverse;
		weights[1] = orient[1] * inverse;
		weights[2] = orient[2] * inverse;
		weights[3] = 1.0f - (weights[0] + weights[1] + weights[2]);

		return 4;
	}

	coord_t n[3]; /* Normal for the current facet. */

WALK_FACET:
	{
		/* Get the vertex data. */
		const local_t f = find_vertex(tetrapal, *t, VERTEX_INFINITE);
		get_facet_normal(tetrapal, *t, f, n);

		for (local_t i = 0; i < 3; i++)
		{
			v[i] = get_incident_vertex(tetrapal, *t, facet_opposite_vertex[f][i]);
			p[i] = get_coordinates(tetrapal, v[i]);
		}

		/* Start from a random edge (stochastic walk). */
		const random_t r = random_range(&seed, 3);

		/* Test the orientation against every edge until a negative one is found. */
		for (int i = 0; i < 3; i++)
		{
			const local_t c = (local_t)((size_t)i + r) % 3;
			const local_t a = edge_opposite_vertex[c][0];
			const local_t b = edge_opposite_vertex[c][1];
			const simplex_t ta = get_adjacent_simplex(tetrapal, *t, facet_opposite_vertex[f][c]);

			/* Get the orientation. */
			coord_t ab[3], ap[3], abp[3];
			sub_3d(p[b], p[a], ab);
			sub_3d(point, p[a], ap);
			cross_3d(ab, ap, abp);
			orient[c] = dot_3d(abp, n);

			/* If the orientation is negative, test the edge. */
			if (orient[c] < 0 && ta != t_prev)
			{
				/* Test the vertex regions. */
				orient[b] = dot_3d(ab, ap);

				/* If negative, jump to vertex region [a]. */
				if (orient[b] < 0)
				{
					v[0] = v[a];
					p[0] = p[a];
					goto WALK_VERTEX;
				}

				const coord_t total = dot_3d(ab, ab);
				orient[a] = total - orient[b];

				/* If negative, jump to vertex region [b]. */
				if (orient[a] < 0)
				{
					v[0] = v[b];
					p[0] = p[b];
					goto WALK_VERTEX;
				}

				/* Test the adjacent facet. */
				get_facet_normal(tetrapal, ta, find_vertex(tetrapal, ta, VERTEX_INFINITE), n);
				orient[c] = dot_3d(abp, n);

				/* If the orientation is negative, jump to this facet. */
				if (orient[c] < 0)
				{
					t_prev = *t;
					*t = ta;
					goto WALK_FACET;
				}

				/* Otherwise, point lies within this region. */
				*t = get_adjacent_simplex(tetrapal, *t, f);
				indices[0] = (int)v[a];
				indices[1] = (int)v[b];
				weights[0] = orient[a] / total;
				weights[1] = 1.0f - weights[0];

				return 2;
			}
		}

		/* This is the enclosing facet. Calculate barycentric weights from the orientation results. */
		const float total = (float)(orient[0] + orient[1] + orient[2]);
		const float inverse = 1.0f / total;

		indices[0] = (int)v[0];
		indices[1] = (int)v[1];
		indices[2] = (int)v[2];
		weights[0] = orient[0] * inverse;
		weights[1] = orient[1] * inverse;
		weights[2] = 1.0f - (weights[0] + weights[1]);

		return 3;
	}

WALK_VERTEX:
	{
		/* Rotate around the vertex. */
		simplex_t t_first = *t;
		local_t f;

		do
		{
			/* Find the pivot and the current edge. */
			f = find_vertex(tetrapal, *t, VERTEX_INFINITE);
			local_t a = find_vertex(tetrapal, *t, v[0]);
			local_t b = facet_from_edge[f][a];

			/* Get vertex info. */
			v[1] = get_incident_vertex(tetrapal, *t, b);
			p[1] = get_coordinates(tetrapal, v[1]);

			/* Test the edge region. */
			coord_t ab[3], ap[3];
			sub_3d(p[1], p[0], ab);
			sub_3d(point, p[0], ap);
			coord_t wb = dot_3d(ab, ap);

			/* If positive, jump to this edge region. */
			if (wb > 0)
			{
				/* Test the other vertex region. */
				coord_t total = dot_3d(ab, ab);
				coord_t wa = total - wb;

				/* If negative, jump to vertex region [b]. */
				if (wa < 0)
				{
					v[0] = v[1];
					p[0] = p[1];
					goto WALK_VERTEX;
				}

				/* Test the current facet. */
				coord_t abp[3];
				cross_3d(ab, ap, abp);
				get_facet_normal(tetrapal, *t, f, n);
				orient[0] = dot_3d(abp, n);

				/* If the orientation is positive, jump to this facet. */
				if (orient[0] > 0)
				{
					goto WALK_FACET;
				}

				/* Test adjacent facet. */
				simplex_t ta = get_adjacent_simplex(tetrapal, *t, facet_from_edge[a][f]);
				get_facet_normal(tetrapal, ta, find_vertex(tetrapal, ta, VERTEX_INFINITE), n);
				orient[0] = dot_3d(abp, n);

				/* If the orientation is negative, jump to this facet. */
				if (orient[0] < 0)
				{
					/* We already tested the current facet, so set it as the previous. */
					t_prev = *t;
					*t = ta;
					goto WALK_FACET;
				}

				/* Point lies within this edge region. */
				indices[0] = (int)v[0];
				indices[1] = (int)v[1];
				weights[0] = wa / total;
				weights[1] = 1.0f - weights[0];

				return 2;
			}

			/* Otherwise continue rotating. */
			*t = get_adjacent_simplex(tetrapal, *t, b);

		} while (*t != t_first);

		/* Point lies within this vertex region. */
		*t = get_adjacent_simplex(tetrapal, *t, f);
		indices[0] = (int)v[0];
		weights[0] = 1.0f;

		return 1;
	}
}

static vertex_t nearest_3d(const Tetrapal* tetrapal, const coord_t point[3])
{
	vertex_t v[4]; /* Current simplex vertex indices. */
	const coord_t* p[4]; /* Current simplex vertex coordinates. */
	coord_t orient[4]; /* Orientation for each face. */
	simplex_t t[3]; /* Simplex indices. */

	/* Find an appropriate starting simplex. */
	v[0] = kdtree_get_vertex(tetrapal, kdtree_find_approximate(tetrapal, point));
	t[0] = get_incident_simplex(tetrapal, v[0]);
	random_t seed = (random_t)t[0]; /* Local seed for rng. */

	/* The previously visited simplex. */
	t[2] = SIMPLEX_NULL;

	/* Walk the triangulation until an enclosing simplex is found. */
WALK_START:
	{
		/* Check if we are at an infinite simplex. */
		if (is_infinite_simplex(tetrapal, t[0]))
		{
			/* Set an arbitrary finite vertex as the starting hull vertex. */
			local_t f = find_vertex(tetrapal, t[0], VERTEX_INFINITE);

			v[0] = get_incident_vertex(tetrapal, t[0], facet_from_edge[f][0]);
			p[0] = get_coordinates(tetrapal, v[0]);

			goto WALK_HULL;
		}

		/* Get the vertex data for the current simplex. */
		for (local_t i = 0; i < 4; i++)
		{
			v[i] = get_incident_vertex(tetrapal, t[0], i);
			p[i] = get_coordinates(tetrapal, v[i]);
		}

		/* Start from a random facet (stochastic walk). */
		const random_t r = random_range(&seed, 4);

		/* Test the orientation against every facet until a negative one is found. */
		for (local_t i = 0; i < 4; i++)
		{
			local_t f = (local_t)(i + r) % 4;
			local_t a = facet_opposite_vertex[f][0];
			local_t b = facet_opposite_vertex[f][1];
			local_t c = facet_opposite_vertex[f][2];
			t[1] = get_adjacent_simplex(tetrapal, t[0], f);

			/* Get the orientation. */
			orient[f] = orient_3d(point, p[a], p[b], p[c]);

			/* If the orientation is negative, move towards the adjacent simplex. */
			if (orient[f] < 0 && t[1] != t[2])
			{
				t[2] = t[0];
				t[0] = t[1];
				goto WALK_START;
			}
		}

		/* This is the enclosing simplex. Find the closest vertex. */
		local_t i[4] = { 0, 1, 2, 3 };
		if (orient[i[0]] < orient[i[1]]) swap_local(&i[0], &i[1]);
		if (orient[i[2]] < orient[i[3]]) swap_local(&i[2], &i[3]);
		if (orient[i[0]] < orient[i[2]]) swap_local(&i[0], &i[2]);
		return v[i[0]];
	}

	/* Rotate around the vertex, testing the distance to every other vertex. */
	/* v[0] is the vertex we're rotating around. */
WALK_HULL:
	{
		/* Get the distance to v[0]. */
		coord_t distance_a = distance_squared_3d(point, p[0]);
		t[2] = t[0]; /* Record the simplex we started from. */

		do
		{
			/* Find the pivot and the current edge. */
			local_t f = find_vertex(tetrapal, t[0], VERTEX_INFINITE);
			local_t a = find_vertex(tetrapal, t[0], v[0]); /* Current vertex. */
			local_t b = facet_from_edge[f][a]; /* The vertex we're testing. */

			/* Get vertex info. */
			v[1] = get_incident_vertex(tetrapal, t[0], b);
			p[1] = get_coordinates(tetrapal, v[1]);

			/* Test the distance to this vertex */
			coord_t distance_b = distance_squared_3d(point, p[1]);

			/* If this vertex is closer, jump to it and rotate again. */
			if (distance_b < distance_a)
			{
				v[0] = v[1];
				p[0] = p[1];
				goto WALK_HULL;
			}

			/* Otherwise continue rotating. */
			t[0] = get_adjacent_simplex(tetrapal, t[0], b);

		} while (t[0] != t[2]);

		/* Point is closest to this vertex. */
		return v[0];
	}
}

static inline void transform_3d(const float in[3], coord_t out[3])
{
	float clamped[3] =
	{
		in[0] > 1.0f ? 1.0f : (in[0] < 0.0f ? 0.0f : in[0]),
		in[1] > 1.0f ? 1.0f : (in[1] < 0.0f ? 0.0f : in[1]),
		in[2] > 1.0f ? 1.0f : (in[2] < 0.0f ? 0.0f : in[2]),
	};

	out[0] = (coord_t)roundf(clamped[0] * TETRAPAL_PRECISION);
	out[1] = (coord_t)roundf(clamped[1] * TETRAPAL_PRECISION);
	out[2] = (coord_t)roundf(clamped[2] * TETRAPAL_PRECISION);
}

static simplex_t new_tetrahedron(Tetrapal* tetrapal, vertex_t a, vertex_t b, vertex_t c, vertex_t d)
{
	simplex_t t = SIMPLEX_NULL;

	/* Check if there are any free simplices. */
	const size_t num_deleted = tetrapal->simplices.deleted.count;

	if (num_deleted > 0)
	{
		t = tetrapal->simplices.deleted.simplices[num_deleted - 1];
		tetrapal->simplices.deleted.count -= 1;
	}
	else /* No free simplices; add a new index. */
	{
		const error_t error = check_simplices_capacity(tetrapal);

		/* Could not reallocate the simplex array. */
		if (error)
			return SIMPLEX_NULL;

		t = (simplex_t)tetrapal->simplices.count;
	}

	/* Set vertex incidence. */
	tetrapal->simplices.incident_vertex[t * 4 + 0] = a;
	tetrapal->simplices.incident_vertex[t * 4 + 1] = b;
	tetrapal->simplices.incident_vertex[t * 4 + 2] = c;
	tetrapal->simplices.incident_vertex[t * 4 + 3] = d;
	tetrapal->simplices.flags[t].all = false;

	/* Is it an infinite simplex? */
	if (a == VERTEX_INFINITE ||
		b == VERTEX_INFINITE ||
		c == VERTEX_INFINITE ||
		d == VERTEX_INFINITE)
	{
		tetrapal->simplices.flags[t].bit.is_infinite = true;
	}
	else
	{
		tetrapal->simplices.flags[t].bit.is_infinite = false;
		tetrapal->simplices.last = t;
	}

#ifdef TETRAPAL_DEBUG
	/* Set adjacencies to null. */
	tetrapal->simplices.adjacent_simplex[t * 4 + 0] = SIMPLEX_NULL;
	tetrapal->simplices.adjacent_simplex[t * 4 + 1] = SIMPLEX_NULL;
	tetrapal->simplices.adjacent_simplex[t * 4 + 2] = SIMPLEX_NULL;
	tetrapal->simplices.adjacent_simplex[t * 4 + 3] = SIMPLEX_NULL;
#endif

	tetrapal->simplices.count += 1;

	return t;
}

/********************************/
/*		2D Triangulation		*/
/********************************/

static error_t triangulate_2d(Tetrapal* tetrapal, vertex_t v[3], const float* points, const int size)
{
	/* Allocate memory. */
	size_t estimated_num_elements = (size_t)size * 2;

	tetrapal->vertices.capacity = (size_t)size;
	tetrapal->vertices.coordinates = TETRAPAL_MALLOC(sizeof(*tetrapal->vertices.coordinates) * (size_t)size * 2);
	tetrapal->vertices.incident_simplex = TETRAPAL_MALLOC(sizeof(*tetrapal->vertices.incident_simplex) * (size_t)size);
	tetrapal->vertices.tree = TETRAPAL_MALLOC(sizeof(*tetrapal->vertices.tree) * (size_t)size);

	tetrapal->simplices.capacity = estimated_num_elements;
	tetrapal->simplices.deleted.capacity = (size_t)size;
	tetrapal->simplices.incident_vertex = TETRAPAL_MALLOC(sizeof(*tetrapal->simplices.incident_vertex) * estimated_num_elements * 3);
	tetrapal->simplices.adjacent_simplex = TETRAPAL_MALLOC(sizeof(*tetrapal->simplices.adjacent_simplex) * estimated_num_elements * 3);
	tetrapal->simplices.flags = TETRAPAL_MALLOC(sizeof(*tetrapal->simplices.flags) * estimated_num_elements);
	tetrapal->simplices.deleted.simplices = TETRAPAL_MALLOC(sizeof(*tetrapal->simplices.deleted.simplices) * (size_t)size);

	/* Memory allocation failed? */
	if (tetrapal->vertices.coordinates == NULL ||
		tetrapal->vertices.incident_simplex == NULL ||
		tetrapal->vertices.tree == NULL ||
		tetrapal->simplices.incident_vertex == NULL ||
		tetrapal->simplices.adjacent_simplex == NULL ||
		tetrapal->simplices.flags == NULL ||
		tetrapal->simplices.deleted.simplices == NULL)
		return ERROR_OUT_OF_MEMORY;

	/* Initialise secondary structures. */
	if (stack_init(&tetrapal->stack, 32) != ERROR_NONE)
		return ERROR_OUT_OF_MEMORY;

	tetrapal->vertices.count = 0;
	tetrapal->simplices.count = 0;
	tetrapal->simplices.deleted.count = 0;

	/* Get the coords of the first simplex in 3D space. */
	const coord_t* p[3] =
	{
		&points[0 * 3],
		&points[1 * 3],
		&points[2 * 3]
	};

	/* Create the 3D basis vectors defining the coordinate system for the 2D triangulation. */
	coord_t x[3], y[3], up[3];
	sub_3d(p[1], p[0], x);
	sub_3d(p[2], p[0], y);
	cross_3d(x, y, up);
	cross_3d(x, up, y);

	/* Normalise the basis vectors with respect to the unit cube. */
	normalise_3d(x, x);
	normalise_3d(y, y);
	mul_3d(x, 1.0f / sqrtf(3.0f), tetrapal->vertices.basis[0]);
	mul_3d(y, 1.0f / sqrtf(3.0f), tetrapal->vertices.basis[1]);

	/* Add all points to the vertex array, converting coords to 2D space. */
	for (int i = 0; i < size; i++)
	{
		coord_t tmp[2];
		transform_2d(tetrapal, &points[i * 3], tmp);
		new_vertex(tetrapal, tmp);
	}

	/* Inistialise and  build the KD Tree */
	for (vertex_t i = 0; i < (vertex_t)size; i++)
		tetrapal->vertices.tree[i] = i;

	if (kdtree_balance(tetrapal, 0, (size_t)size - 1, 0) != ERROR_NONE)
		return ERROR_OUT_OF_MEMORY;

	/* Get the coordinates of the first simplex. */
	for (local_t i = 0; i < 3; i++)
		p[i] = get_coordinates(tetrapal, v[i]);

	/* Ensure positive orientation. */
	if (orient_2d(p[0], p[1], p[2]) < 0)
		swap_vertex(&v[0], &v[1]);

	/* Create the first triangle. */
	simplex_t t = new_triangle(tetrapal, v[0], v[1], v[2]);

	if (t == SIMPLEX_NULL)
		return ERROR_OUT_OF_MEMORY;

	/* Create the infinite triangles. */
	simplex_t inf[3];

	for (local_t i = 0; i < 3; i++)
	{
		/* Get the edge indices opposite [t]'s vertex [i], enumerate backwards so that it is from pov of inf[i]. */
		vertex_t a = get_incident_vertex(tetrapal, t, edge_opposite_vertex[i][1]);
		vertex_t b = get_incident_vertex(tetrapal, t, edge_opposite_vertex[i][0]);
		inf[i] = new_triangle(tetrapal, VERTEX_INFINITE, a, b);

		if (inf[i] == SIMPLEX_NULL)
			return ERROR_OUT_OF_MEMORY;

		/* Set adjacencies across the finite triangle. */
		set_adjacent_simplex(tetrapal, t, inf[i], i);
		set_adjacent_simplex(tetrapal, inf[i], t, 0);
	}

	/* Set adjacencies across infinite triangles. */
	for (local_t i = 0; i < 3; i++)
	{
		set_adjacent_simplex(tetrapal, inf[i], inf[edge_opposite_vertex[i][1]], 1);
		set_adjacent_simplex(tetrapal, inf[i], inf[edge_opposite_vertex[i][0]], 2);
	}

	/* Insert vertices one-by-one. */
	for (vertex_t i = 0; i < (vertex_t)size; i++)
	{
		/* Ensure we don't re-insert a vertex from the starting simplex. */
		if (i == v[0] || i == v[1] || i == v[2])
			continue;

		if (insert_2d(tetrapal, i) != ERROR_NONE)
			return ERROR_OUT_OF_MEMORY;
	}

	/* Set vertex-to-simplex incidence. */
		/* Set vertex-to-simplex incidence. */
	for (vertex_t i = 0; i < (vertex_t)size; i++)
	{
		t = locate_2d(tetrapal, &tetrapal->vertices.coordinates[i * 3]);
		tetrapal->vertices.incident_simplex[i] = t;

		TETRAPAL_ASSERT(is_infinite_simplex(tetrapal, t) == false, "Incident simplex was infinite!\n");
	}

	/* Free intermediate structures. */
	TETRAPAL_FREE(tetrapal->simplices.deleted.simplices);
	tetrapal->simplices.deleted.simplices = NULL;
	stack_free(&tetrapal->stack);

	return ERROR_NONE;
}

static error_t insert_2d(Tetrapal* tetrapal, vertex_t v)
{
	/* Add this point to the vertex array. */
	const coord_t* p = get_coordinates(tetrapal, v);

	/* Find the enclosing simplex of the input point. */
	simplex_t t = locate_2d(tetrapal, p);

	/* Check whether the input point is coincident with a vertex of the enclosing simplex.
		We still leave the vertex in the structure for consistency, but we don't triangulate it. */
	if (is_coincident_simplex(tetrapal, t, p) == true)
		return ERROR_NONE;

	/* Remove conflict simplices and triangulate the cavity. */
	t = stellate_2d(tetrapal, v, t);

	if (t == SIMPLEX_NULL)
		return ERROR_OUT_OF_MEMORY;

	/* Success! */
	return ERROR_NONE;
}

static simplex_t locate_2d(const Tetrapal* tetrapal, const coord_t point[2])
{
	/* Start from the last finite simplex. */
	simplex_t t = tetrapal->simplices.last;
	simplex_t t_prev = SIMPLEX_NULL; /* The simplex we just walked from. */

	/* Local seed for rng. */
	random_t seed = (random_t)t;

	/* Walk the triangulation until an enclosing simplex is found. */
WALK:
	{
		/* Check if we are at an infinite simplex. */
		if (is_infinite_simplex(tetrapal, t))
			return t;

		/* Get the vertices of the current simplex. */
		const vertex_t v[3] =
		{
			get_incident_vertex(tetrapal, t, 0),
			get_incident_vertex(tetrapal, t, 1),
			get_incident_vertex(tetrapal, t, 2)
		};

		/* Get the coordinates of each vertex for the current simplex. */
		const coord_t* p[3] =
		{
			get_coordinates(tetrapal, v[0]),
			get_coordinates(tetrapal, v[1]),
			get_coordinates(tetrapal, v[2])
		};

		/* Start from a random edge (stochastic walk). */
		const random_t r = random_range(&seed, 3);

		/* Test the orientation against every edge until a negative one is found. */
		for (int j = 0; j < 3; j++)
		{
			const local_t f = (local_t)((size_t)j + r) % 3;
			const simplex_t ta = get_adjacent_simplex(tetrapal, t, f);

			/* If we just came from this simplex, skip it. */
			if (ta == t_prev)
				continue;

			const local_t a = edge_opposite_vertex[f][0];
			const local_t b = edge_opposite_vertex[f][1];

			/* If the orientation is negative, we should move towards the adjacent simplex. */
			if (orient_2d(point, p[a], p[b]) < 0)
			{
				t_prev = t;
				t = ta;
				goto WALK;
			}
		}

		return t;
	}
}

static simplex_t stellate_2d(Tetrapal* tetrapal, vertex_t v, simplex_t t)
{
	TETRAPAL_ASSERT(is_enclosing_simplex(tetrapal, t, get_coordinates(tetrapal, v)) == true, "Starting simplex does not enclose the point!\n");

	/* Reference to a non-conflict triangle whose edge is on the boundary. */
	simplex_t t_boundary = SIMPLEX_NULL;
	local_t e_boundary = LOCAL_NULL;
	size_t count = 0;

	/* Insert the first conflict simplex [t] into the stack. */
	Stack* stack = &tetrapal->stack;
	stack_clear(stack);

	if (stack_insert(stack, t) != ERROR_NONE)
		return SIMPLEX_NULL;

	/* Mark the first simplex as free, since we know it should already be in conflict. */
	if (free_simplex(tetrapal, t) != ERROR_NONE)
		return SIMPLEX_NULL;

	/* Get the coordinates of the vertex. */
	const coord_t* p = get_coordinates(tetrapal, v);

	/* Perform depth-search traversal of conflict zone until there are no more simplices to check.*/
	while (stack_is_empty(stack) == false)
	{
		/* Get and pop the simplex at the top of the stack. */
		t = stack_top(stack);
		stack_pop(stack);

		/* Check every adjacent triangle for conflict. */
		for (local_t i = 0; i < 3; i++)
		{
			simplex_t adj = get_adjacent_simplex(tetrapal, t, i);

			/* If the simplex is free, then it has already been processed. */
			if (is_free_simplex(tetrapal, adj) == true)
				continue;

			/* If it is in conflict, free the simplex and add it to the stack. */
			if (conflict_2d(tetrapal, adj, p) == true)
			{
				if (stack_insert(stack, adj) != ERROR_NONE)
					return SIMPLEX_NULL;

				if (free_simplex(tetrapal, adj) != ERROR_NONE)
					return SIMPLEX_NULL;

				continue;
			}

			/* It is not in conflict. Keep a reference to the boundary triangle. */
			t_boundary = adj;
			e_boundary = find_adjacent(tetrapal, adj, t);
			count += 1;

			/* Set the adjacent simplex on the boundary edge to null so we can identify it later. */
			set_adjacent_simplex(tetrapal, t_boundary, SIMPLEX_NULL, e_boundary);
		}
	}

	TETRAPAL_ASSERT(t_boundary != SIMPLEX_NULL && e_boundary != LOCAL_NULL, "Boundary was ill-formed!\n");

	/* Rotate around the boundary, stellating the cavity. */
	simplex_t t_prev = SIMPLEX_NULL;
	simplex_t t_first = SIMPLEX_NULL;

	do
	{
		/* Get the boundary vertices. */
		local_t a = edge_opposite_vertex[e_boundary][0];
		local_t b = edge_opposite_vertex[e_boundary][1];
		vertex_t va = get_incident_vertex(tetrapal, t_boundary, a);
		vertex_t vb = get_incident_vertex(tetrapal, t_boundary, b);

		/* Create new triangle and set adjacencies wrt the boundary triangle. */
		t = new_triangle(tetrapal, v, vb, va);

		/* Check if we failed to create a new triangle. */
		if (t == SIMPLEX_NULL)
			return SIMPLEX_NULL;

		set_adjacent_simplex(tetrapal, t, t_boundary, 0);
		set_adjacent_simplex(tetrapal, t_boundary, t, e_boundary);

		/* Set adjacency to the previous triangle. */
		if (t_prev != SIMPLEX_NULL)
		{
			set_adjacent_simplex(tetrapal, t, t_prev, 1);
			set_adjacent_simplex(tetrapal, t_prev, t, 2);
		}
		else
		{
			t_first = t;
		}

		t_prev = t;
		count -= 1;

		/* Leave if we visited all the boundary edges. */
		if (count == 0)
			break;

		/* Find the next boundary triangle by rotating around the boundary vertex. */
		vertex_t pivot = vb;
		e_boundary = a;

		while (get_adjacent_simplex(tetrapal, t_boundary, e_boundary) != SIMPLEX_NULL)
		{
			t_boundary = get_adjacent_simplex(tetrapal, t_boundary, e_boundary);
			e_boundary = edge_opposite_vertex[find_vertex(tetrapal, t_boundary, pivot)][1];
		}

	} while (count != 0);

	/* Connect first and last triangles. */
	set_adjacent_simplex(tetrapal, t_first, t, 1);
	set_adjacent_simplex(tetrapal, t, t_first, 2);

	return t;
}

static bool conflict_2d(const Tetrapal* tetrapal, simplex_t t, const coord_t point[2])
{
	TETRAPAL_ASSERT(t < tetrapal->simplices.count + tetrapal->simplices.deleted.count, "Simplex index out of range!");

	/* Get the coordinates of each vertex for the current simplex. */
	const coord_t* p[3];

	for (local_t i = 0; i < 3; i++)
	{
		const vertex_t v = get_incident_vertex(tetrapal, t, i);

		/* We represent the coordinates of an infinite vertex as a NULL pointer. This idea is borrowed from Geogram. */
		p[i] = (v == VERTEX_INFINITE) ? NULL : get_coordinates(tetrapal, v);
	}

	/* The rules on testing the conflict zone for finite and infinite simplices are slightly different.
		For finite simplices, we can do a simple insphere/incircle test as normal.
		For infinite simplices, we must perform an orientation test on the finite facet.
		If the point lies on the inner side of the plane defined by the finite facet, then it is in conflict.
		If the point is directly on the plane however, we must do another test to check whether it is on the facet's circumcircle (i.e. in conflict). */

		/* Look for the infinite vertex if there is one. */
	for (local_t i = 0; i < 3; i++)
	{
		/* Ignore finite vertices. */
		if (p[i] != NULL)
			continue;

		const local_t a = edge_opposite_vertex[i][0];
		const local_t b = edge_opposite_vertex[i][1];

		/* Get the orientation wrt the finite facet.*/
		const coord_t orientation = orient_2d(point, p[a], p[b]);

		/* If the orientation is positive, then this simplex is in conflict. If negative, then it is not. */
		if (orientation > 0)
			return true;

		if (orientation < 0)
			return false;

		/* If the orientation is exactly 0, then we need to check whether it lies on the facet's circumcircle.
			This is equivalent to checking whether the adjacent finite simplex is in conflict with the point. */
		const simplex_t adj = get_adjacent_simplex(tetrapal, t, i);
		TETRAPAL_ASSERT(is_infinite_simplex(tetrapal, adj) == false, "Adjacent simplex to infinite vertex should be finite!");

		/* Because we stellate depth-first, the simplex would have already been freed if it was in conflict.
			This saves us an insphere/incircle test. */
		if (is_free_simplex(tetrapal, adj))
			return true;
		else
			return (conflict_2d(tetrapal, adj, point));
	}

	/* The simplex is finite, so we do a regular insphere/incircle test. */
	if (incircle_2d(p[0], p[1], p[2], point) > 0)
		return true;
	else
		return false;
}

static size_t interpolate_2d(const Tetrapal* tetrapal, const coord_t point[2], int indices[3], float weights[3], simplex_t* t)
{
	vertex_t v[3];
	const coord_t* p[3];
	coord_t orient[3]; /* Orientation for each edge. */
	simplex_t t_prev = SIMPLEX_NULL; /* The simplex we just walked from. */

	/* Find an appropriate starting simplex. */
	v[0] = kdtree_get_vertex(tetrapal, kdtree_find_approximate(tetrapal, point));
	*t = get_incident_simplex(tetrapal, v[0]);
	random_t seed = (random_t)(*t); /* Local seed for rng. */

	/* Start walking from within the triangulation. */
WALK_START:
	{
		/* Check if we are at an infinite simplex. */
		if (is_infinite_simplex(tetrapal, *t))
			goto WALK_HULL; /* Perform a hull walk. */

		/* Get the vertex data for the current simplex. */
		for (local_t i = 0; i < 3; i++)
		{
			v[i] = get_incident_vertex(tetrapal, *t, i);
			p[i] = get_coordinates(tetrapal, v[i]);
		}

		/* Start from a random edge (stochastic walk). */
		random_t r = random_range(&seed, 3);

		/* Test the orientation against every edge until a negative one is found. */
		for (local_t i = 0; i < 3; i++)
		{
			local_t e = (local_t)(i + r) % 3;
			local_t a = edge_opposite_vertex[e][0];
			local_t b = edge_opposite_vertex[e][1];
			simplex_t adj = get_adjacent_simplex(tetrapal, *t, e);

			/* Get the orientation. */
			orient[e] = orient_2d(point, p[a], p[b]);

			/* If the orientation is negative, move towards the adjacent simplex. */
			if (orient[e] < 0 && adj != t_prev)
			{
				t_prev = *t;
				*t = adj;
				goto WALK_START;
			}
		}

		/* This is the enclosing simplex. Calculate barycentric weights from the orientation results. */
		const float total = (float)(orient[0] + orient[1] + orient[2]);
		const float inverse = 1.0f / total;

		indices[0] = (int)v[0];
		indices[1] = (int)v[1];
		indices[2] = (int)v[2];
		weights[0] = orient[0] * inverse;
		weights[1] = orient[1] * inverse;
		weights[2] = 1.0f - (weights[0] + weights[1]);

		return 3;
	}

	/* Walk the hull. */
WALK_HULL:
	{
		/* Get the current edge on the hull. */
		local_t e = find_vertex(tetrapal, *t, VERTEX_INFINITE);

		v[0] = get_incident_vertex(tetrapal, *t, edge_opposite_vertex[e][0]);
		v[1] = get_incident_vertex(tetrapal, *t, edge_opposite_vertex[e][1]);
		p[0] = get_coordinates(tetrapal, v[0]);
		p[1] = get_coordinates(tetrapal, v[1]);

		for (local_t a = 0; a < 2; a++)
		{
			local_t b = (local_t)(a + 1) % 2;

			/* Is the point before [a]? */
			coord_t ab[2], ap[2];
			sub_2d(p[b], p[a], ab);
			sub_2d(point, p[a], ap);
			orient[b] = dot_2d(ab, ap);

			if (orient[b] < 0)
			{
				simplex_t adj = get_adjacent_simplex(tetrapal, *t, edge_opposite_vertex[e][b]);

				/* Did we just come from this edge? If so, we are in a vertex region. */
				if (adj == t_prev)
				{
					*t = get_adjacent_simplex(tetrapal, *t, e);
					indices[0] = (int)v[a];
					weights[0] = 1.0f;

					return 1;
				}

				/* Otherwise jump to the next edge. */
				t_prev = *t;
				*t = adj;
				goto WALK_HULL;
			}
		}

		/* Point is in this edge region. */
		*t = get_adjacent_simplex(tetrapal, *t, e);
		indices[0] = (int)v[0];
		indices[1] = (int)v[1];
		weights[0] = (float)(orient[0] / (orient[0] + orient[1]));
		weights[1] = 1.0f - weights[0];

		return 2;
	}
}

static vertex_t nearest_2d(const Tetrapal* tetrapal, const coord_t point[2])
{
	vertex_t v[2];
	const coord_t* p[2];
	simplex_t t[2];

	/* Find an appropriate starting simplex. */
	v[0] = kdtree_get_vertex(tetrapal, kdtree_find_approximate(tetrapal, point));
	t[0] = get_incident_simplex(tetrapal, v[0]);
	p[0] = get_coordinates(tetrapal, v[0]);
	coord_t distance_a = distance_squared_2d(point, p[0]);

	/* Walk the graph by rotating around each vertex until we find the true closest. */
WALK_GRAPH:
	{
		/* Set [t1] as the simplex we start rotating from. */
		t[1] = t[0];

		do
		{
			local_t a = find_vertex(tetrapal, t[0], v[0]);
			local_t b = edge_opposite_vertex[a][0];
			v[1] = get_incident_vertex(tetrapal, t[0], b);

			/* Don't test infinite vertices. */
			if (v[1] != VERTEX_INFINITE)
			{
				p[1] = get_coordinates(tetrapal, v[1]);
				coord_t distance_b = distance_squared_2d(point, p[1]);

				if (distance_b < distance_a)
				{
					v[0] = v[1];
					p[0] = p[1];
					distance_a = distance_b;
					goto WALK_GRAPH;
				}
			}

			/* Get the next simplex around the current edge. */
			t[0] = get_adjacent_simplex(tetrapal, t[0], b);

		} while (t[0] != t[1]);

		/* We found the closest vertex. */
		return v[0];
	}
}

static inline void transform_2d(const Tetrapal* tetrapal, const float point[3], coord_t out[2])
{
	coord_t p[3] = { (coord_t)point[0], (coord_t)point[1], (coord_t)point[2] };

	/* Clamp values. */
	out[0] = out[0] > 1.0f ? 1.0f : (out[0] < 0.0f ? 0.0f : out[0]);
	out[1] = out[1] > 1.0f ? 1.0f : (out[1] < 0.0f ? 0.0f : out[1]);

	/* Project onto basis vectors. */
	out[0] = dot_3d(p, tetrapal->vertices.basis[0]);
	out[1] = dot_3d(p, tetrapal->vertices.basis[1]);

	/* Scale coordinates. */
	out[0] = (coord_t)roundf(out[0] * TETRAPAL_PRECISION);
	out[1] = (coord_t)roundf(out[1] * TETRAPAL_PRECISION);
}

static simplex_t new_triangle(Tetrapal* tetrapal, vertex_t a, vertex_t b, vertex_t c)
{
	simplex_t t = SIMPLEX_NULL;

	/* Check if there are any free simplices. */
	const size_t num_deleted = tetrapal->simplices.deleted.count;

	if (num_deleted > 0)
	{
		t = tetrapal->simplices.deleted.simplices[num_deleted - 1];
		tetrapal->simplices.deleted.count -= 1;
	}
	else /* No free simplices; add a new index. */
	{
		const error_t error = check_simplices_capacity(tetrapal);

		/* Could not reallocate the simplex array. */
		if (error)
			return SIMPLEX_NULL;

		t = (simplex_t)tetrapal->simplices.count;
	}

	/* Set vertex incidence. */
	tetrapal->simplices.incident_vertex[t * 3 + 0] = a;
	tetrapal->simplices.incident_vertex[t * 3 + 1] = b;
	tetrapal->simplices.incident_vertex[t * 3 + 2] = c;
	tetrapal->simplices.flags[t].all = false;

	/* Is it an infinite simplex? */
	if (a == VERTEX_INFINITE ||
		b == VERTEX_INFINITE ||
		c == VERTEX_INFINITE)
	{
		tetrapal->simplices.flags[t].bit.is_infinite = true;
	}
	else
	{
		tetrapal->simplices.flags[t].bit.is_infinite = false;
		tetrapal->simplices.last = t;
	}

#ifdef TETRAPAL_DEBUG
	/* Set adjacencies to null. */
	tetrapal->simplices.adjacent_simplex[t * 3 + 0] = SIMPLEX_NULL;
	tetrapal->simplices.adjacent_simplex[t * 3 + 1] = SIMPLEX_NULL;
	tetrapal->simplices.adjacent_simplex[t * 3 + 2] = SIMPLEX_NULL;
#endif

	tetrapal->simplices.count += 1;

	return t;
}

/********************************************/
/*		1D Triangulation (Binary Tree)		*/
/********************************************/

static error_t triangulate_1d(Tetrapal* tetrapal, const float* points, const int size)
{
	/* Allocate memory. */
	tetrapal->vertices.capacity = (size_t)size;
	tetrapal->vertices.coordinates = TETRAPAL_MALLOC(sizeof(*tetrapal->vertices.coordinates) * (size_t)size);
	tetrapal->vertices.tree = TETRAPAL_MALLOC(sizeof(*tetrapal->vertices.tree) * (size_t)size);

	/* Memory allocation failed? */
	if (tetrapal->vertices.coordinates == NULL ||
		tetrapal->vertices.tree == NULL)
		return ERROR_OUT_OF_MEMORY;

	tetrapal->vertices.count = 0;

	/* Get the coords of the first simplex in 3D space. */
	const coord_t* p[2] =
	{
		&points[0 * 3],
		&points[1 * 3]
	};

	/* Create the 3D basis vector defining the coordinate system for the 1D triangulation. */
	coord_t x[3];
	sub_3d(p[1], p[0], x);
	normalise_3d(x, x);
	mul_3d(x, 1.0f / sqrtf(3.0f), tetrapal->vertices.basis[0]);

	/* Add all points to the vertex array, converting coords to 1D space. */
	for (int i = 0; i < size; i++)
	{
		coord_t tmp;
		transform_1d(tetrapal, &points[i * 3], &tmp);
		new_vertex(tetrapal, &tmp);
	}

	/* Inistialise and build the binary tree */
	for (vertex_t i = 0; i < (vertex_t)size; i++)
		tetrapal->vertices.tree[i] = i;

	if (kdtree_balance(tetrapal, 0, (size_t)size - 1, 0) != ERROR_NONE)
		return ERROR_OUT_OF_MEMORY;

	/* 'Triangulation' is done! */
	return ERROR_NONE;
}

static inline void transform_1d(const Tetrapal* tetrapal, const float point[3], coord_t out[1])
{
	coord_t p[3] = { (coord_t)point[0], (coord_t)point[1], (coord_t)point[2] };

	/* Clam, project onto basis vector, and scale. */
	out[0] = out[0] > 1.0f ? 1.0f : (out[0] < 0.0f ? 0.0f : out[0]);
	out[0] = dot_3d(p, tetrapal->vertices.basis[0]);
	out[0] = (coord_t)roundf(out[0] * TETRAPAL_PRECISION);
}

static size_t interpolate_1d(const Tetrapal* tetrapal, const coord_t point[1], int indices[2], float weights[2])
{
	vertex_t v[2];
	const coord_t* p[2];

	/* Find the approximate closest vertex. */
	size_t index = kdtree_find_approximate(tetrapal, point);
	v[0] = kdtree_get_vertex(tetrapal, index);
	p[0] = get_coordinates(tetrapal, v[0]);

	/* Is the query point before or after this vertex? */
	if (point[0] < p[0][0])
	{
		if (index == 0) /* No points before it. */
		{
			indices[0] = (int)v[0];
			weights[0] = 1.0f;
			return 1;
		}
		else /* Interpolate on this edge. */
		{
			v[1] = kdtree_get_vertex(tetrapal, index - 1);
			p[1] = get_coordinates(tetrapal, v[1]);

			indices[0] = (int)v[0];
			indices[1] = (int)v[1];
			weights[0] = (point[0] - p[1][0]) / (p[0][0] - p[1][0]);
			weights[1] = 1.0f - weights[0];
			return 2;
		}
	}
	else if (point[0] > p[0][0]) /* After the vertex. */
	{
		if (index == tetrapal->vertices.count - 1) /* No points after it. */
		{
			indices[0] = (int)v[0];
			weights[0] = 1.0f;
			return 1;
		}
		else /* Interpolate on this edge. */
		{
			v[1] = kdtree_get_vertex(tetrapal, index + 1);
			p[1] = get_coordinates(tetrapal, v[1]);

			indices[1] = (int)v[1];
			indices[0] = (int)v[0];
			weights[1] = (point[0] - p[0][0]) / (p[1][0] - p[0][0]);
			weights[0] = 1.0f - weights[1];
			return 2;
		}
	}

	/* Point lies exactly on this vertex. */
	indices[0] = (int)v[0];
	weights[0] = 1.0f;
	return 1;
}

static vertex_t nearest_1d(const Tetrapal* tetrapal, const coord_t point[1])
{
	vertex_t v[2];
	const coord_t* p[2];

	/* Find the approximate closest vertex. */
	size_t index = kdtree_find_approximate(tetrapal, point);
	v[0] = kdtree_get_vertex(tetrapal, index);
	p[0] = get_coordinates(tetrapal, v[0]);

	/* Is the query point before or after this vertex? */
	if (point[0] < p[0][0])
	{
		if (index == 0) /* No points before it. */
			return v[0];

		/* Get the closest point. */
		v[1] = kdtree_get_vertex(tetrapal, index - 1);
		p[1] = get_coordinates(tetrapal, v[1]);

		coord_t distance_a = distance_squared_1d(point, p[0]);
		coord_t distance_b = distance_squared_1d(point, p[1]);

		return (distance_a < distance_b) ? v[0] : v[1];
	}
	else if (point[0] > p[0][0]) /* After the vertex. */
	{
		if (index == tetrapal->vertices.count - 1) /* No points after it. */
			return v[0];

		/* Get the closest point. */
		v[1] = kdtree_get_vertex(tetrapal, index + 1);
		p[1] = get_coordinates(tetrapal, v[1]);

		coord_t distance_a = distance_squared_1d(point, p[0]);
		coord_t distance_b = distance_squared_1d(point, p[1]);

		return (distance_a < distance_b) ? v[0] : v[1];
	}

	/* On the vertex. */
	return v[0];
}

/********************************************/
/*		0D Triangulation (Single Vertex)	*/
/********************************************/

static error_t triangulate_0d(Tetrapal* tetrapal)
{
	tetrapal->vertices.capacity = 1;
	tetrapal->vertices.count = 1;

	/* 'Triangulation' is done! */
	return ERROR_NONE;
}

static size_t interpolate_0d(int indices[1], float weights[1])
{
	indices[0] = 0;
	weights[0] = 1;
	return 1;
}

static vertex_t nearest_0d(void)
{
	return 0;
}

/********************************************/
/*		Natural Neighbour Interpolation		*/
/********************************************/

static inline error_t natural_neighbour_accumulate(vertex_t index, coord_t weight, int* indices, float* weights, int size, size_t* count)
{
	/* Find the index. */
	for (size_t i = 0; i < *count; i++)
	{
		if (indices[i] == index)
		{
			weights[i] += (float)weight;
			return ERROR_NONE;
		}
	}

	/* Reached the buffer limit. */
	if (*count == (size_t)size)
		return ERROR_OUT_OF_MEMORY;

	/* Add this vertex to the output array. */
	indices[*count] = (int)index;
	weights[*count] = (float)weight;
	*count += 1;

	return ERROR_NONE;
}

static size_t natural_neighbour_2d(const Tetrapal* tetrapal, coord_t point[2], int* indices, float* weights, int size)
{
	vertex_t v[3]; /* Vertex global indices. */
	const coord_t* p[3]; /* Vertex coordinates. */
	coord_t m[3][2]; /* Mid-points between [point] and each vertex. */
	coord_t c[2][2]; /* Circumcentres. */
	simplex_t t[3]; /* Simplex indices. */
	size_t n = 0; /* Number of natural neighbours. */

	/* Struct to hold enclosing simplex information. */
	struct
	{
		size_t count;
		int indices[3];
		float weights[3];

	} enclosing;

	/* Struct representing the pending and previously visited simplices. */
	struct
	{
		Stack pending;
		Stack previous;

	} stack;

	stack.pending.data = NULL;
	stack.previous.data = NULL;

	/* Locate the enclosing simplex, projecting [p] onto the hull if it is outside. */
	enclosing.count = interpolate_2d(tetrapal, point, enclosing.indices, enclosing.weights, &t[0]);

	/* Point is outside the convex hull.*/
	if (enclosing.count < 3)
	{
		/* Not enough space in array to hold result. */
		if (size < (int)enclosing.count)
			return 0;

		for (size_t i = 0; i < enclosing.count; i++)
		{
			indices[i] = enclosing.indices[i];
			weights[i] = enclosing.weights[i];
		}

		return enclosing.count;
	}

	/* Allocate memory for stacks. */
	if (stack_init(&stack.pending, 32) != ERROR_NONE)
		goto EXIT_ON_ERROR;

	if (stack_init(&stack.previous, 32) != ERROR_NONE)
		goto EXIT_ON_ERROR;

	/* The enclosing simplex is necessarily in conflict. Add it to the pending stack. */
	if (stack_insert(&stack.pending, t[0]) != ERROR_NONE)
		goto EXIT_ON_ERROR;

	if (stack_insert(&stack.previous, SIMPLEX_NULL) != ERROR_NONE)
		goto EXIT_ON_ERROR;

	/* Perform depth-first traversal of the conflict-zone. */
	while (stack_is_empty(&stack.pending) == false)
	{
		/* Take [t0] from the top of the pending stack. */
		t[0] = stack_top(&stack.pending);
		stack_pop(&stack.pending);

		/* Take the [t2] as the simplex we came from. */
		t[2] = stack_top(&stack.previous);
		stack_pop(&stack.previous);

		/* Get vertex data. */
		for (local_t i = 0; i < 3; i++)
		{
			v[i] = get_incident_vertex(tetrapal, t[0], i);
			p[i] = get_coordinates(tetrapal, v[i]);
			midpoint_2d(point, p[i], m[i]); /* Midpoint between [p] and each vertex of [t0]. */
		}

		/* Get the circumcentre of [t0]. */
		circumcentre_2d(p[0], p[1], p[2], c[0]);

		/* Check every adjacent simplex [t1] of [t0]. */
		for (local_t e = 0; e < 3; e++)
		{
			t[1] = get_adjacent_simplex(tetrapal, t[0], e);

			/* If we have already visited the adjacent simplex, skip it. */
			if (t[1] == t[2])
				continue;

			/* Adjacent simplex is in conflict zone. */
			if (is_infinite_simplex(tetrapal, t[1]) == false &&
				conflict_2d(tetrapal, t[1], point) == true)
			{
				/* Get the circumcentre of [t1]. */
				get_circumcentre(tetrapal, t[1], c[1]);

				/* Add [t1] to the pending stack. */
				if (stack_insert(&stack.pending, t[1]) != ERROR_NONE)
					goto EXIT_ON_ERROR;

				if (stack_insert(&stack.previous, t[0]) != ERROR_NONE)
					goto EXIT_ON_ERROR;
			}
			else /* Adjacent simplex is outside conflict zone.*/
			{
				/* Get the circumcentre of the simplex formed by [p] and the boundary vertices. */
				local_t a = edge_opposite_vertex[e][0];
				local_t b = edge_opposite_vertex[e][1];
				circumcentre_2d(point, p[a], p[b], c[1]);
			}

			/* For every vertex shared by [t0] and [t1], accumulate the area of the triangle formed by the mid-point and the two circumcentres. */
			for (local_t i = 0; i < 2; i++)
			{
				local_t l = edge_opposite_vertex[e][i]; /* Local vertex index. */
				coord_t area = orient_2d(m[l], c[i], c[1 - i]);

				if (natural_neighbour_accumulate(v[l], area, indices, weights, size, &n) != ERROR_NONE)
					goto EXIT_ON_ERROR;
			}
		}
		/* Repeat until we have visited all conflict simplices. */
	}

	/* Free the stacks. */
	stack_free(&stack.pending);
	stack_free(&stack.previous);

	/* Get the total weight. */
	coord_t total = 0.0f;

	for (size_t i = 0; i < n; i++)
		total += weights[i];

	/* Normalise. */
	coord_t inverse = 1.0f / total;

	for (size_t i = 0; i < n; i++)
		weights[i] *= inverse;

	return n;

	/* We failed because we hit some kind of memory limit. */
EXIT_ON_ERROR:
	stack_free(&stack.pending);
	stack_free(&stack.previous);
	return 0;
}

static size_t natural_neighbour_3d(const Tetrapal* tetrapal, coord_t point[3], int* indices, float* weights, int size)
{
	vertex_t v[4]; /* Vertex global indices. */
	const coord_t* p[4]; /* Vertex coordinates. */
	coord_t m[5][3]; /* Mid-points. */
	coord_t c[4][3]; /* Circumcentres. */
	simplex_t t[3]; /* Current simplices. */
	size_t n = 0; /* Number of natural neighbours. */

	/* Struct to hold enclosing simplex information. */
	struct
	{
		size_t count;
		int indices[4];
		float weights[4];

	} enclosing;

	/* Stacks for holding the pending simplices and conflict simplices. */
	struct
	{
		Stack pending;
		Stack conflict;

	} stack;

	stack.pending.data = NULL;
	stack.conflict.data = NULL;

	/* Locate the enclosing simplex, projecting [p] onto the hull if it is outside. */
	enclosing.count = interpolate_3d(tetrapal, point, enclosing.indices, enclosing.weights, &t[0]);

	/* If the point is outside the convex hull, project it and fall back to linear interpolation. */
	if (enclosing.count < 4)
	{
		if (size < (int)enclosing.count)
			return 0;

		for (size_t i = 0; i < enclosing.count; i++)
		{
			indices[i] = enclosing.indices[i];
			weights[i] = enclosing.weights[i];
		}

		return enclosing.count;
	}

	/* Allocate memory for stacks. */
	if (stack_init(&stack.pending, 32) != ERROR_NONE)
		goto EXIT_ON_ERROR;

	if (stack_init(&stack.conflict, 32) != ERROR_NONE)
		goto EXIT_ON_ERROR;

	/* The enclosing simplex is necessarily in conflict. Add it to the pending stack. */
	if (stack_insert(&stack.pending, t[0]) != ERROR_NONE)
		goto EXIT_ON_ERROR;

	/* Perform depth-first traversal of the conflict-zone. */
	while (stack_is_empty(&stack.pending) == false)
	{
		/* Take [t0] from the top of the pending stack. */
		t[0] = stack_top(&stack.pending);
		stack_pop(&stack.pending);

		/* If we have already visited the simplex, skip it. */
		if (stack_contains(&stack.conflict, t[0]) == true)
			continue;

		/* Check every adjacent simplex [t1] of [t0]. */
		for (local_t f = 0; f < 4; f++)
		{
			t[1] = get_adjacent_simplex(tetrapal, t[0], f);

			/* Is in conflict zone. Make sure we don't add any infinite conflict simplices. */
			if (is_infinite_simplex(tetrapal, t[1]) == false &&
				conflict_3d(tetrapal, t[1], point) == true)
			{
				/* Add [t1] to the pending stack. */
				if (stack_insert(&stack.pending, t[1]) != ERROR_NONE)
					goto EXIT_ON_ERROR;

				continue;
			}
		}

		/* Mark [t] as visited. */
		if (stack_insert(&stack.conflict, t[0]) != ERROR_NONE)
			goto EXIT_ON_ERROR;
	}

	/* Visit each conflict simplex [t0]. */
	for (size_t i = 0; i < stack.conflict.count; i++)
	{
		t[0] = stack.conflict.data[i];

		/* Get the data for this simplex. */
		for (local_t f = 0; f < 4; f++)
		{
			/* Get vertex data. */
			v[f] = get_incident_vertex(tetrapal, t[0], f);
			p[f] = get_coordinates(tetrapal, v[f]);

			/* Get the mid-point between [p] and each vertex. */
			midpoint_3d(point, p[f], m[f]);
		}

		/* Get the circumcentre [c0] of [t0]. */
		circumcentre_3d(p[0], p[1], p[2], p[3], c[0]);

		/* Check every adjacent simplex [t1] of [t0]. */
		for (local_t f = 0; f < 4; f++)
		{
			t[1] = get_adjacent_simplex(tetrapal, t[0], f);

			/* [t1] is a conflict simplex. */
			if (stack_contains(&stack.conflict, t[1]) == true)
			{
				/* Get the circumcentre [c1] of [t1]. */
				get_circumcentre(tetrapal, t[1], c[1]);

				/* For each vertex of the internal facet. */
				for (local_t j = 0; j < 3; j++)
				{
					/* [a, b] is the current directed edge of the facet wrt [t0]. */
					local_t a = facet_opposite_vertex[f][(j + 0) % 3];
					local_t b = facet_opposite_vertex[f][(j + 1) % 3];

					/* Get the mid-point of this edge. */
					midpoint_3d(p[a], p[b], m[4]);

					/* Accumulate the volume for [a]. */
					coord_t volume = orient_3d(c[0], c[1], m[a], m[4]);

					if (natural_neighbour_accumulate(v[a], volume, indices, weights, size, &n) != ERROR_NONE)
						goto EXIT_ON_ERROR;
				}
			}
			else /* [t1] is a boundary simplex. */
			{
				/* Get the circumcentre [c1] of the tetrahedron formed by [p] and the boundary facet. */
				circumcentre_3d(
					point,
					p[facet_opposite_vertex[f][0]],
					p[facet_opposite_vertex[f][1]],
					p[facet_opposite_vertex[f][2]],
					c[1]);

				/* For each vertex of the internal facet. */
				for (local_t j = 0; j < 3; j++)
				{
					/* [a, b] is the current directed edge of the facet wrt [t0]. */
					local_t a = facet_opposite_vertex[f][(j + 0) % 3];
					local_t b = facet_opposite_vertex[f][(j + 1) % 3];

					/* Get the mid-point of this edge. */
					midpoint_3d(p[a], p[b], m[4]);

					/* Get the next simplex [t2] around this edge towards the conflict zone. */
					t[1] = t[0];
					local_t f2;

					/* Rotate around [a, b] until we reach another boundary facet. */
					while (true)
					{
						/* Get the next simplex [t2] around this edge towards the conflict zone. */
						f2 = find_facet_from_edge(tetrapal, t[1], v[b], v[a]);
						t[2] = get_adjacent_simplex(tetrapal, t[1], f2);

						/* Determine whether [t2] is a boundary simplex.*/
						if (stack_contains(&stack.conflict, t[2]) == false)
							break;

						/* If we didn't, continue rotating. */
						t[1] = t[2];
					}

					/* Get the circumcentres [c2] and [c3]. */
					get_circumcentre(tetrapal, t[1], c[2]);

					circumcentre_3d(
						point,
						get_coordinates(tetrapal, get_incident_vertex(tetrapal, t[1], facet_opposite_vertex[f2][0])),
						get_coordinates(tetrapal, get_incident_vertex(tetrapal, t[1], facet_opposite_vertex[f2][1])),
						get_coordinates(tetrapal, get_incident_vertex(tetrapal, t[1], facet_opposite_vertex[f2][2])),
						c[3]);

					/* Accumulate the volume for [a]. */
					coord_t volume = 0;
					volume += orient_3d(c[1], c[0], m[4], m[a]);
					volume += orient_3d(c[2], c[3], m[4], m[a]);
					volume += orient_3d(c[1], c[3], m[a], m[4]);

					if (natural_neighbour_accumulate(v[a], volume, indices, weights, size, &n) != ERROR_NONE)
						goto EXIT_ON_ERROR;
				}
			}
		}
	}

	/* Free the stacks. */
	stack_free(&stack.pending);
	stack_free(&stack.conflict);

	/* Normalise. */
	coord_t total = 0;

	for (size_t i = 0; i < n; i++)
		total += weights[i];

	coord_t inverse = 1.0f / total;

	for (size_t i = 0; i < n; i++)
		weights[i] *= inverse;

	return n;

	/* We failed because we hit some kind of memory limit. */
EXIT_ON_ERROR:
	stack_free(&stack.pending);
	stack_free(&stack.conflict);
	return 0;
}

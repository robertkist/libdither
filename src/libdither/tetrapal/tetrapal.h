#ifndef TETRAPAL_H
#define TETRAPAL_H

#ifdef __cplusplus
extern "C" {
#endif
	
	/* Opaque data structure. */
	typedef struct Tetrapal Tetrapal;

	/* 
		Create a new triangulation from the given points. 

		*points: pointer to an array of 3D float coordinates in the range [0.0, 1.0] and format [x0, y0, z0, x1, y1, z1...].
		size: specifies how many points are represented in the array.

		Returns NULL on failure.
	*/
	Tetrapal* tetrapal_new(const float* points, const int size);

	/* 
		Free the triangulation. 
	*/
	void tetrapal_free(Tetrapal* tetrapal);

	/* 
		Interpolate an input point as the weighted sum of up to four existing points in the triangulation. 

		point[3]: the point to be interpolated.
		*indices: array where indices are written to. Should be at least as large as 'tetrapal_element_size'.
		*weights: array where weights are written to. Should be at least as large as 'tetrapal_element_size'.
		
		Returns an int from 1 to 4 depending on the number of points contributing to the interpolant.
	*/
	int tetrapal_interpolate(const Tetrapal* tetrapal, const float point[3], int* indices, float* weights);

	/* 
		Calculate the natural neighbour coordinates of an input point. 

		point[3]: the point to be interpolated.
		*indices: array where indices are written to.
		*weights: array where weights are written to.
		size: specifies the size of the output arrays.
		
		Returns an int specifying the number of points contributing to the interpolant, or 0 on failure.
	*/
	int tetrapal_natural_neighbour(const Tetrapal* tetrapal, const float point[3], int* indices, float* weights, const int size);

	/* 
		Find the nearest neigbour of a given query point. 

		point[3]: the query point.
	*/
	int tetrapal_nearest_neighbour(const Tetrapal* tetrapal, const float point[3]);

	/* 
		Get the number of (finite) simplices in the triangulation. 
	*/
	int tetrapal_number_of_elements(const Tetrapal* tetrapal);

	/* 
		Get the number of dimensions spanned by the triangulation. 
	*/
	int tetrapal_number_of_dimensions(const Tetrapal* tetrapal);

	/* 
		Get the size of an element, i.e. the number of vertices defining a simplex within the triangulation. 
	*/
	int tetrapal_element_size(const Tetrapal* tetrapal);

	/* 
		Fill a contiguous buffer with finite element indices. 
		Buffer size should be 'tetrapal_number_of_elements' * 'tetrapal_element_size' at least.

		Returns non-zero on failure.
	*/
	int tetrapal_get_elements(const Tetrapal* tetrapal, int* buffer);

#ifdef __cplusplus
}
#endif

#endif // !TETRAPAL_H

/*
	MIT License

	Copyright (c) 2023 matejlou

	Permission is hereby granted, free of charge, to any person obtaining a copy
	of this software and associated documentation files (the "Software"), to deal
	in the Software without restriction, including without limitation the rights
	to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
	copies of the Software, and to permit persons to whom the Software is
	furnished to do so, subject to the following conditions:

	The above copyright notice and this permission notice shall be included in all
	copies or substantial portions of the Software.

	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
	IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
	FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
	AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
	LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
	OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
	SOFTWARE.
*/

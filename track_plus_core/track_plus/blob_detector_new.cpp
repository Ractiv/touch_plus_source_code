/*
 * Touch+ Software
 * Copyright (C) 2015
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the Aladdin Free Public License as
 * published by the Aladdin Enterprises, either version 9 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * Aladdin Free Public License for more details.
 *
 * You should have received a copy of the Aladdin Free Public License
 * along with this program.  If not, see <http://ghostscript.com/doc/8.54/Public.htm>.
 */

#include "blob_detector_new.h"

void BlobDetectorNew::compute(Mat image_in, uchar gray_in, int x_min_in, int x_max_in, int y_min_in, int y_max_in, bool shallow, bool octal)
{
	x_min_result = 9999;
	x_max_result = 0;
	y_min_result = 9999;
	y_max_result = 0;

	image_atlas = Mat::zeros(image_in.rows, image_in.cols, CV_16UC1);
	Mat image_clone = image_in;

	delete blobs;
	blobs = new vector<BlobNew>();
	blob_max_size_actual = BlobNew();
	blob_max_size = &blob_max_size_actual;

	const int j_min = y_min_in < 1 ? 1 : y_min_in;
	const int j_max = y_max_in > image_in.rows - 2 ? image_in.rows - 2 : y_max_in;

	const int i_min = x_min_in < 1 ? 1 : x_min_in;
	const int i_max = x_max_in > image_in.cols - 2 ? image_in.cols - 2 : x_max_in;

	const int y_min = 0;
	const int y_max = image_in.rows - 1;

	const int x_min = 0;
	const int x_max = image_in.cols - 1;

	for (int j = j_min; j < j_max; ++j)
		for (int i = i_min; i < i_max; ++i)
		{
			uchar* pix_ptr = &image_clone.ptr<uchar>(j, i)[0];

			if (*pix_ptr != gray_in)
				continue;

			blobs->push_back(BlobNew(image_atlas, blobs->size() + 1));
			BlobNew* blob = &((*blobs)[blobs->size() - 1]);
			blob->add(i, j);
			*pix_ptr = 255;

			for (int k = 0; k < blob->data.size(); ++k)
			{
				const int pt_x = blob->data[k].x;
				const int pt_y = blob->data[k].y;

				if (pt_x <= x_min || pt_x >= x_max || pt_y <= y_min || pt_y >= y_max)
					continue;

				const int pt_x0 = pt_x - 1;
				const int pt_y0 = pt_y - 1;
				const int pt_x1 = pt_x + 1;
				const int pt_y1 = pt_y + 1;

				pix_ptr = &image_clone.ptr<uchar>(pt_y, pt_x0)[0];
				if (*pix_ptr == gray_in)
				{
					blob->add(pt_x0, pt_y);
					*pix_ptr = 255;
				}
				pix_ptr = &image_clone.ptr<uchar>(pt_y, pt_x1)[0];
				if (*pix_ptr == gray_in)
				{
					blob->add(pt_x1, pt_y);
					*pix_ptr = 255;
				}
				pix_ptr = &image_clone.ptr<uchar>(pt_y0, pt_x)[0];
				if (*pix_ptr == gray_in)
				{
					blob->add(pt_x, pt_y0);
					*pix_ptr = 255;
				}
				pix_ptr = &image_clone.ptr<uchar>(pt_y1, pt_x)[0];
				if (*pix_ptr == gray_in)
				{
					blob->add(pt_x, pt_y1);
					*pix_ptr = 255;
				}

				if (octal)
				{
					pix_ptr = &image_clone.ptr<uchar>(pt_y0, pt_x0)[0];
					if (*pix_ptr == gray_in)
					{
						blob->add(pt_x0, pt_y0);
						*pix_ptr = 255;
					}
					pix_ptr = &image_clone.ptr<uchar>(pt_y1, pt_x1)[0];
					if (*pix_ptr == gray_in)
					{
						blob->add(pt_x1, pt_y1);
						*pix_ptr = 255;
					}
					pix_ptr = &image_clone.ptr<uchar>(pt_y0, pt_x1)[0];
					if (*pix_ptr == gray_in)
					{
						blob->add(pt_x1, pt_y0);
						*pix_ptr = 255;
					}
					pix_ptr = &image_clone.ptr<uchar>(pt_y1, pt_x0)[0];
					if (*pix_ptr == gray_in)
					{
						blob->add(pt_x0, pt_y1);
						*pix_ptr = 255;
					}
				}
			}

			blob->compute();

			if (blob->data.size() > blob_max_size->data.size())
			{
				blob_max_size_actual = *blob;
				blob_max_size = &blob_max_size_actual;
			}

			if (blob->x_min < x_min_result)
				x_min_result = blob->x_min;
			if (blob->x_max > x_max_result)
				x_max_result = blob->x_max;
			if (blob->y_min < y_min_result)
				y_min_result = blob->y_min;
			if (blob->y_max > y_max_result)
			{
				y_max_result = blob->y_max;
				pt_y_max_result = blob->pt_y_max;
			}
		}

	if (!shallow)
		for (BlobNew& blob : *blobs)
			blob.fill(image_clone, gray_in);
}

void BlobDetectorNew::compute_region(Mat image_in, uchar gray_in, vector<Point>& region_vec, bool shallow, bool octal)
{
	x_min_result = 9999;
	x_max_result = 0;
	y_min_result = 9999;
	y_max_result = 0;

	image_atlas = Mat::zeros(image_in.rows, image_in.cols, CV_16UC1);
	Mat image_clone = image_in;

	delete blobs;
	blobs = new vector<BlobNew>();
	blob_max_size_actual = BlobNew();
	blob_max_size = &blob_max_size_actual;

	for (Point& pt_region : region_vec)
	{
		int i = pt_region.x;
		int j = pt_region.y;
		
		uchar* pix_ptr = &image_clone.ptr<uchar>(j, i)[0];

		if (*pix_ptr != gray_in)
			continue;

		blobs->push_back(BlobNew(image_atlas, blobs->size() + 1));
		BlobNew* blob = &((*blobs)[blobs->size() - 1]);
		blob->add(i, j);
		*pix_ptr = 255;

		for (int k = 0; k < blob->data.size(); ++k)
		{
			const int pt_x = blob->data[k].x;
			const int pt_y = blob->data[k].y;

			const int pt_x0 = pt_x - 1;
			const int pt_y0 = pt_y - 1;
			const int pt_x1 = pt_x + 1;
			const int pt_y1 = pt_y + 1;

			pix_ptr = &image_clone.ptr<uchar>(pt_y, pt_x0)[0];
			if (*pix_ptr == gray_in)
			{
				blob->add(pt_x0, pt_y);
				*pix_ptr = 255;
			}
			pix_ptr = &image_clone.ptr<uchar>(pt_y, pt_x1)[0];
			if (*pix_ptr == gray_in)
			{
				blob->add(pt_x1, pt_y);
				*pix_ptr = 255;
			}
			pix_ptr = &image_clone.ptr<uchar>(pt_y0, pt_x)[0];
			if (*pix_ptr == gray_in)
			{
				blob->add(pt_x, pt_y0);
				*pix_ptr = 255;
			}
			pix_ptr = &image_clone.ptr<uchar>(pt_y1, pt_x)[0];
			if (*pix_ptr == gray_in)
			{
				blob->add(pt_x, pt_y1);
				*pix_ptr = 255;
			}

			if (octal)
			{
				pix_ptr = &image_clone.ptr<uchar>(pt_y0, pt_x0)[0];
				if (*pix_ptr == gray_in)
				{
					blob->add(pt_x0, pt_y0);
					*pix_ptr = 255;
				}
				pix_ptr = &image_clone.ptr<uchar>(pt_y1, pt_x1)[0];
				if (*pix_ptr == gray_in)
				{
					blob->add(pt_x1, pt_y1);
					*pix_ptr = 255;
				}
				pix_ptr = &image_clone.ptr<uchar>(pt_y0, pt_x1)[0];
				if (*pix_ptr == gray_in)
				{
					blob->add(pt_x1, pt_y0);
					*pix_ptr = 255;
				}
				pix_ptr = &image_clone.ptr<uchar>(pt_y1, pt_x0)[0];
				if (*pix_ptr == gray_in)
				{
					blob->add(pt_x0, pt_y1);
					*pix_ptr = 255;
				}
			}
		}

		blob->compute();

		if (blob->data.size() > blob_max_size->data.size())
		{
			blob_max_size_actual = *blob;
			blob_max_size = &blob_max_size_actual;
		}

		if (blob->x_min < x_min_result)
			x_min_result = blob->x_min;
		if (blob->x_max > x_max_result)
			x_max_result = blob->x_max;
		if (blob->y_min < y_min_result)
			y_min_result = blob->y_min;
		if (blob->y_max > y_max_result)
		{
			y_max_result = blob->y_max;
			pt_y_max_result = blob->pt_y_max;
		}
	}

	if (!shallow)
		for (BlobNew& blob : *blobs)
			blob.fill(image_clone, gray_in);
}

void BlobDetectorNew::compute_location(Mat image_in, const uchar gray_in, const int i, const int j, bool shallow, bool in_process, bool octal)
{
	if (in_process == false)
	{
		x_min_result = 9999;
		x_max_result = 0;
		y_min_result = 9999;
		y_max_result = 0;

		image_atlas = Mat::zeros(image_in.rows, image_in.cols, CV_16UC1);
		
		delete blobs;
		blobs = new vector<BlobNew>();
		blob_max_size_actual = BlobNew();
		blob_max_size = &blob_max_size_actual;
	}

	Mat image_clone = image_in;

	const int j_max = image_in.rows - 2;
	const int i_max = image_in.cols - 2;
	const int y_max = image_in.rows - 1;
	const int x_max = image_in.cols - 1;

	uchar* pix_ptr = &image_clone.ptr<uchar>(j, i)[0];

	if (*pix_ptr != gray_in)
		return;

	blobs->push_back(BlobNew(image_atlas, blobs->size() + 1));
	BlobNew* blob = &((*blobs)[blobs->size() - 1]);
	blob->add(i, j);
	*pix_ptr = 255;

	for (int k = 0; k < blob->data.size(); ++k)
	{
		const int pt_x = blob->data[k].x;
		const int pt_y = blob->data[k].y;

		if (pt_x <= 0 || pt_x >= x_max || pt_y <= 0 || pt_y >= y_max)
			continue;

		const int pt_x0 = pt_x - 1;
		const int pt_y0 = pt_y - 1;
		const int pt_x1 = pt_x + 1;
		const int pt_y1 = pt_y + 1;

		pix_ptr = &image_clone.ptr<uchar>(pt_y, pt_x0)[0];
		if (*pix_ptr == gray_in)
		{
			blob->add(pt_x0, pt_y);
			*pix_ptr = 255;
		}
		pix_ptr = &image_clone.ptr<uchar>(pt_y, pt_x1)[0];
		if (*pix_ptr == gray_in)
		{
			blob->add(pt_x1, pt_y);
			*pix_ptr = 255;
		}
		pix_ptr = &image_clone.ptr<uchar>(pt_y0, pt_x)[0];
		if (*pix_ptr == gray_in)
		{
			blob->add(pt_x, pt_y0);
			*pix_ptr = 255;
		}
		pix_ptr = &image_clone.ptr<uchar>(pt_y1, pt_x)[0];
		if (*pix_ptr == gray_in)
		{
			blob->add(pt_x, pt_y1);
			*pix_ptr = 255;
		}

		if (octal)
		{
			pix_ptr = &image_clone.ptr<uchar>(pt_y0, pt_x0)[0];
			if (*pix_ptr == gray_in)
			{
				blob->add(pt_x0, pt_y0);
				*pix_ptr = 255;
			}
			pix_ptr = &image_clone.ptr<uchar>(pt_y1, pt_x1)[0];
			if (*pix_ptr == gray_in)
			{
				blob->add(pt_x1, pt_y1);
				*pix_ptr = 255;
			}
			pix_ptr = &image_clone.ptr<uchar>(pt_y0, pt_x1)[0];
			if (*pix_ptr == gray_in)
			{
				blob->add(pt_x1, pt_y0);
				*pix_ptr = 255;
			}
			pix_ptr = &image_clone.ptr<uchar>(pt_y1, pt_x0)[0];
			if (*pix_ptr == gray_in)
			{
				blob->add(pt_x0, pt_y1);
				*pix_ptr = 255;
			}
		}
	}

	blob->compute();

	if (blob->data.size() > blob_max_size->data.size())
	{
		blob_max_size_actual = *blob;
		blob_max_size = &blob_max_size_actual;
	}

	if (blob->x_min < x_min_result)
		x_min_result = blob->x_min;
	if (blob->x_max > x_max_result)
		x_max_result = blob->x_max;
	if (blob->y_min < y_min_result)
		y_min_result = blob->y_min;
	if (blob->y_max > y_max_result)
	{
		y_max_result = blob->y_max;
		pt_y_max_result = blob->pt_y_max;
	}

	if (!shallow)
		for (BlobNew& blob : *blobs)
			blob.fill(image_clone, gray_in);
}

void BlobDetectorNew::compute_all(Mat image_in, bool octal)
{
	x_min_result = 9999;
	x_max_result = 0;
	y_min_result = 9999;
	y_max_result = 0;

	image_atlas = Mat::zeros(image_in.rows, image_in.cols, CV_16UC1);
	
	delete blobs;
	blobs = new vector<BlobNew>();
	blob_max_size_actual = BlobNew();
	blob_max_size = &blob_max_size_actual;
	
	const int width_const = image_in.cols;
	const int height_const = image_in.rows;

	for (int i = 0; i < width_const; ++i)
		for (int j = 0; j < height_const; ++j)
		{
			const uchar gray = image_in.ptr<uchar>(j, i)[0];
			if (gray < 255)
				compute_location(image_in, gray, i, j, true, true, octal);
		}
}

struct compare_blob_count
{
	bool operator() (const BlobNew& blob0, const BlobNew& blob1)
	{
		return (blob0.count > blob1.count);
	}
};

struct compare_blob_angle
{
	Point pivot;

	compare_blob_angle(Point& pivot_in)
	{
		pivot = pivot_in;
	}

	bool operator() (const BlobNew& blob0, const BlobNew& blob1)
	{
		float theta0 = get_angle(blob0.x, blob0.y, pivot.x, pivot.y);
		float theta1 = get_angle(blob1.x, blob1.y, pivot.x, pivot.y);

		return theta0 > theta1;
	}
};

struct compare_blob_x
{
	bool operator() (const BlobNew& blob0, const BlobNew& blob1)
	{
		return (blob0.x < blob1.x);
	}
};

struct compare_blob_y_max
{
	bool operator() (const BlobNew& blob0, const BlobNew& blob1)
	{
		return (blob0.y_max > blob1.y_max);
	}
};

struct compare_blob_x_min
{
	bool operator() (const BlobNew& blob0, const BlobNew& blob1)
	{
		return (blob0.x_min < blob1.x_min);
	}
};

void BlobDetectorNew::sort_blobs_by_count()
{
	sort(blobs->begin(), blobs->end(), compare_blob_count());
}

void BlobDetectorNew::sort_blobs_by_angle(Point& pivot)
{
	sort(blobs->begin(), blobs->end(), compare_blob_angle(pivot));
}

void BlobDetectorNew::sort_blobs_by_x()
{
	sort(blobs->begin(), blobs->end(), compare_blob_x());
}

void BlobDetectorNew::sort_blobs_by_y_max()
{
	sort(blobs->begin(), blobs->end(), compare_blob_y_max());
}

void BlobDetectorNew::sort_blobs_by_x_min()
{
	sort(blobs->begin(), blobs->end(), compare_blob_x_min());
}
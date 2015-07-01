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

#include "blob_tracker.h"

void BlobTracker::compute(vector<BlobNew>& blobs_in)
{
	merge = false;
	split = false;

	blobs_new = blobs_in;

	if (history.size() > 0)
	{
		vector<BlobNew>* blobs_old = &(history[history.size() - 1]);
		vector<CollectionInfo> collection_info_vec;

		const int blobs_new_size = blobs_new.size();
		const int blobs_old_size = blobs_old->size();
		for (int i = 0; i < blobs_new_size; ++i)
			for (int j = 0; j < blobs_old_size; ++j)
			{
				BlobNew* blob_new = &blobs_new[i];
				BlobNew* blob_old = &(*blobs_old)[j];

				CollectionInfo info;

				info.position0 = i;
				info.position1 = j;

				info.index0 = blob_new->index;
				info.index1 = blob_old->index;

				info.atlas_id0 = blob_new->atlas_id;
				info.atlas_id1 = blob_old->atlas_id;

				info.overlap_count = blob_new->compute_overlap(*blob_old);

				info.overlap_ratio0 = (float)info.overlap_count / blob_new->count;
				info.overlap_ratio1 = (float)info.overlap_count / blob_old->count;
				info.overlap_ratio_max = max(info.overlap_ratio0, info.overlap_ratio1);

				if (info.overlap_count > 0)
					collection_info_vec.push_back(info);
			}
		sort(collection_info_vec.begin(), collection_info_vec.end(), compare_overlap_count());

		vector<BlobNew*> blobs_inherited;

		for (CollectionInfo& info : collection_info_vec)
		{
			BlobNew* blob_new = &blobs_new[info.position0];
			BlobNew* blob_old = &(*blobs_old)[info.position1];

			if (blob_new->active && blob_old->active)
			{
				//inherit
				info.active = true;

				BlobNew* blob_ptr = &(blobs_in[info.position0]);
				blob_ptr->index = blob_old->index;
				blob_ptr->index_vec = blob_old->index_vec;
				blob_ptr->id = blob_old->id;

				blob_new->active = false;
				blob_old->active = false;

				blobs_inherited.push_back(blob_ptr);
			}
		}
		for (CollectionInfo& info : collection_info_vec)
		{
			BlobNew* blob_new = &blobs_new[info.position0];
			BlobNew* blob_old = &(*blobs_old)[info.position1];
			BlobNew* blob_ptr = &(blobs_in[info.position0]);

			if (!info.active && info.overlap_ratio_max > 0.25)
				if (info.overlap_ratio0 < info.overlap_ratio1)
				{
					merge = true;

					vector<int>* index_vec0 = &(blob_ptr->index_vec);
					vector<int>* index_vec1 = &(blob_old->index_vec);

					index_vec0->push_back(info.index1);

					for (int& index1 : *index_vec1)
					{
						bool found = false;
						for (int& index0 : *index_vec0)
							if (index1 == index0)
							{
								found = true;
								break;
							}
						if (!found)
							index_vec0->push_back(index1);
					}
					sort(index_vec0->begin(), index_vec0->end());

					blob_ptr->id = (*index_vec0)[0];
					blob_ptr->merge = true;
				}
				else if (blob_old->index_vec.size() > 1)
				{
					split = true;

					blob_ptr->index = blob_old->index_vec[1];
					blob_ptr->index_vec.push_back(blob_ptr->index);
					blob_ptr->id = blob_ptr->index;
					blob_new->active = false;

					for (BlobNew*& blob_inherited : blobs_inherited)
					{
						bool break_now = false;

						for (int i = 0; i < blob_inherited->index_vec.size(); ++i)
							if (blob_inherited->index_vec[i] == blob_ptr->index)
							{
								blob_inherited->index_vec.erase(blob_inherited->index_vec.begin() + i);
								break_now = true;
								--i;
							}

						if (break_now)
							break;
					}
				}
		}
	}

	const int blobs_new_size = blobs_new.size();
	for (int i = 0; i < blobs_new_size; ++i)
		if (blobs_new[i].active)
		{
			//new
			BlobNew* blob_ptr = &(blobs_in[i]);
			blob_ptr->index = index;
			blob_ptr->index_vec.push_back(index);
			blob_ptr->id = index;
			++index;
		}

	if (history.size() > 50)
	{
		vector<vector<BlobNew>> history_temp;
		for (int i = 45; i <= 50; ++i)
			history_temp.push_back(history[i]);

		history = history_temp;
	}

	history.push_back(blobs_in);
}

bool BlobTracker::find_in_history(const int id, const int frames_back, BlobNew*& result)
{
	if (history.size() < 2)
		return false;

	bool b = false;

	vector<BlobNew>* frame = &(history[history.size() - 1 - frames_back]);
	for (BlobNew& blob : *frame)
		if (blob.id == id)
		{
			b = true;
			result = &blob;
			break;
		}
		
	return b;
}

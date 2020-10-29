#pragma once

#include <vector>

#include "GeoPoint.h"
#include "Utility.h"

namespace GeoProc
{
	// A Face of a 3D Polygon
	class GeoFace
	{
		// Vertices in one face of the 3D polygon
		std::vector<GeoPoint> _pts;

		// Vertices Index
		std::vector<int> _idx;

		// Number of vertices
		int _n;

	public:
		
		__declspec(dllexport) GeoFace(void);
		
		__declspec(dllexport) ~GeoFace(void)
		{
			// free memory
			Utility::FreeVectorMemory(this->_pts);
			Utility::FreeVectorMemory(this->_idx);
		}
			
		__declspec(dllexport) GeoFace(std::vector<GeoPoint> pts, std::vector<int> idx)
		{
			this->_n = pts.size();
		
			for(int i=0;i<_n;i++)
			{
				this->_pts.push_back(pts[i]);
				this->_idx.push_back(idx[i]);
			}
		
		}

		__declspec(dllexport) int GetN()
		{
			return this->_n;
		}

		__declspec(dllexport) std::vector<int> GetI()
		{
			return this->_idx;
		}

		__declspec(dllexport) std::vector<GeoPoint> GetV()
		{
			return this->_pts;
		}
	};
}

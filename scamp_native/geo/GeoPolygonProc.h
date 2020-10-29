#pragma once

#include <vector>

#include "GeoPoint.h"
#include "GeoVector.h"
#include "GeoFace.h"
#include "GeoPlane.h"
#include "GeoPolygon.h"
#include "Utility.h"

namespace GeoProc
{
	// 3D Polygon Process 
	class GeoPolygonProc
	{
		// Polygon
		GeoPolygon _polygon;

		// Polygon Boundary
		double _x0, _x1, _y0, _y1, _z0, _z1;	

		// Polygon faces
		std::vector<GeoFace> _Faces;

		// Polygon face planes
		std::vector<GeoPlane> _FacePlanes;

		// Number of faces
		int _NumberOfFaces;	

		// Maximum point to face plane distance error, point is considered in the face plane if its distance is less than this error
		double _MaxDisError;

		__declspec(dllexport) void GeoPolygonProc::Set3DPolygonBoundary();
	
		__declspec(dllexport) void GeoPolygonProc::Set3DPolygonUnitError();
	
		__declspec(dllexport) void GeoPolygonProc::SetConvex3DFaces();

	public:

		__declspec(dllexport) GeoPolygonProc(void) {}
		__declspec(dllexport) ~GeoPolygonProc(void) {}
			
		__declspec(dllexport) GeoPolygonProc(GeoPolygon polygon)
		{
			this->_polygon = polygon;

			Set3DPolygonBoundary();
	
			Set3DPolygonUnitError();
	
			SetConvex3DFaces();
		}

		__declspec(dllexport) GeoPolygon GetPolygon() { return _polygon; }

		__declspec(dllexport) double GetX0() { return this->_x0; }
		__declspec(dllexport) double GetX1() { return this->_x1; }
		__declspec(dllexport) double GetY0() { return this->_y0; }
		__declspec(dllexport) double GetY1() { return this->_y1; }
		__declspec(dllexport) double GetZ0() { return this->_z0; }
		__declspec(dllexport) double GetZ1() { return this->_z1; }

		__declspec(dllexport) std::vector<GeoFace> GetFaces() { return this->_Faces; }
		__declspec(dllexport) std::vector<GeoPlane> GetFacePlanes() { return this->_FacePlanes; }
		__declspec(dllexport) int GetNumberOfFaces() { return this->_NumberOfFaces; }

		__declspec(dllexport) double GetMaxDisError() { return this->_MaxDisError; }
		__declspec(dllexport) void SetMaxDisError(double value) { this->_MaxDisError = value; }		
						
		__declspec(dllexport) bool GeoPolygonProc::PointInside3DPolygon(double x, double y, double z);

	};

}
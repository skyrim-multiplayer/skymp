#pragma once

#include "GeoPoint.h"

namespace GeoProc
{
	// A 3D Geometry Vector
	class GeoVector
	{
		GeoPoint _p0; // vector begin point
		GeoPoint _p1; // vector end point

		double _x; // vector x axis projection value
		double _y; // vector y axis projection value
		double _z; // vector z axis projection value 

	public:

		__declspec(dllexport) GeoVector(void);
		__declspec(dllexport) ~GeoVector(void);
	
		__declspec(dllexport) GeoVector(GeoPoint p0, GeoPoint p1)
		{
			this->_p0 = p0;
			this->_p1 = p1;
			this->_x = p1.x - p0.x;
			this->_y = p1.y - p0.y;
			this->_z = p1.z - p0.z;
		}		

		__declspec(dllexport) friend GeoVector operator*(const GeoVector& u, const GeoVector& v) 
		{			
			double x = u._y * v._z - u._z * v._y;
			double y = u._z * v._x - u._x * v._z;
			double z = u._x * v._y - u._y * v._x;
			
			GeoPoint p0 = v._p0;
			GeoPoint p1 = p0 + GeoPoint(x, y, z);

			return GeoVector(p0, p1);
		}

		__declspec(dllexport) GeoPoint GetP0(){return this->_p0;}
		__declspec(dllexport) GeoPoint GetP1(){return this->_p1;}
		__declspec(dllexport) double GetX(){return this->_x;}
		__declspec(dllexport) double GetY(){return this->_y;}
		__declspec(dllexport) double GetZ(){return this->_z;}

	};

}
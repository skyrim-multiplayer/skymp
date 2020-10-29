#pragma once

namespace GeoProc
{
	// A 3D Geometry Point 
	class GeoPoint
	{			

	public:
	
		double x;		
		double y;
		double z;	

		__declspec(dllexport) GeoPoint(void);
		__declspec(dllexport) ~GeoPoint(void);

		__declspec(dllexport) GeoPoint(double x, double y, double z)		
		{
			this->x = x;
			this->y = y;
			this->z = z;	
		}	

		__declspec(dllexport) friend GeoPoint operator+(const GeoPoint& p0, const GeoPoint& p1) 
		{
			return GeoPoint(p0.x + p1.x, p0.y + p1.y, p0.z + p1.z);
		}
	
	};

}
#include "math.h"

#include "GeoPolygonProc.h"

double MaxUnitMeasureError = 0.001;

namespace GeoProc {

void GeoPolygonProc::Set3DPolygonBoundary()
{
  std::vector<GeoPoint> vertices = _polygon.GetV();

  int n = _polygon.GetN();

  this->_x0 = vertices[0].x;
  this->_x1 = vertices[0].x;
  this->_y0 = vertices[0].y;
  this->_y1 = vertices[0].y;
  this->_z0 = vertices[0].z;
  this->_z1 = vertices[0].z;

  for (int i = 1; i < n; i++) {
    if (vertices[i].x < _x0)
      this->_x0 = vertices[i].x;
    if (vertices[i].y < _y0)
      this->_y0 = vertices[i].y;
    if (vertices[i].z < _z0)
      this->_z0 = vertices[i].z;
    if (vertices[i].x > _x1)
      this->_x1 = vertices[i].x;
    if (vertices[i].y > _y1)
      this->_y1 = vertices[i].y;
    if (vertices[i].z > _z1)
      this->_z1 = vertices[i].z;
  }
}

void GeoPolygonProc::Set3DPolygonUnitError()
{
  this->_MaxDisError =
    ((fabs(_x0) + fabs(_x1) + fabs(_y0) + fabs(_y1) + fabs(_z0) + fabs(_z1)) /
     6 * MaxUnitMeasureError);
}

void GeoPolygonProc::SetConvex3DFaces()
{
  // get polygon vertices
  std::vector<GeoPoint> vertices = this->_polygon.GetV();

  // get number of polygon vertices
  int n = this->_polygon.GetN();

  // face planes
  std::vector<GeoPlane> fpOutward;

  // 2d vertices indexes, first dimension is face index,
  // second dimension is vertice indexes in one face
  std::vector<std::vector<int>> faceVerticeIndex;

  for (int i = 0; i < n; i++) {
    // triangle point 1
    GeoPoint p0 = vertices[i];

    for (int j = i + 1; j < n; j++) {
      // triangle point 2
      GeoPoint p1 = vertices[j];

      for (int k = j + 1; k < n; k++) {
        // triangle point 3
        GeoPoint p2 = vertices[k];

        GeoPlane trianglePlane = GeoPlane(p0, p1, p2);

        int onLeftCount = 0;
        int onRightCount = 0;

        int m = 0;

        std::vector<int> pointInSamePlaneIndex;

        for (int l = 0; l < n; l++) {
          if (l != i && l != j &&
              l != k) // any point other than the 3 triangle points
          {
            GeoPoint pt = vertices[l];

            double dis = trianglePlane * pt;

            if (fabs(dis) <
                this->_MaxDisError) // next point is in the triangle plane
            {
              pointInSamePlaneIndex.push_back(l);
            } else {
              if (dis < 0) {
                onLeftCount++;
              } else {
                onRightCount++;
              }
            }
          }
        }

        // This is a face for a CONVEX 3d polygon.
        // For a CONCAVE 3d polygon, this maybe not a face.
        if (onLeftCount == 0 || onRightCount == 0) {
          std::vector<int> faceVerticeIndexInOneFace;

          // add 3 triangle vertices to the triangle plane
          faceVerticeIndexInOneFace.push_back(i);
          faceVerticeIndexInOneFace.push_back(j);
          faceVerticeIndexInOneFace.push_back(k);

          // add other same plane vetirces in this triangle plane
          for (int p = 0; p < pointInSamePlaneIndex.size(); p++) {
            faceVerticeIndexInOneFace.push_back(pointInSamePlaneIndex[p]);
          }

          // check if it is a new face
          if (!Utility::ContainsVector(faceVerticeIndex,
                                       faceVerticeIndexInOneFace)) {
            // add the new face
            faceVerticeIndex.push_back(faceVerticeIndexInOneFace);

            // add the new face plane
            if (onRightCount == 0) {
              fpOutward.push_back(trianglePlane);
            } else if (onLeftCount == 0) {
              fpOutward.push_back(-trianglePlane);
            }
          }

          // free local memory
          Utility::FreeVectorMemory(faceVerticeIndexInOneFace);

        } else {
          // possible reasons:
          // 1. the plane is not a face of a convex 3d polygon,
          //    it is a plane crossing the convex 3d polygon.
          // 2. the plane is a face of a concave 3d polygon
        }

        // free local memory
        Utility::FreeVectorMemory(pointInSamePlaneIndex);

      } // k loop
    }   // j loop
  }     // i loop

  // set number of faces
  this->_NumberOfFaces = faceVerticeIndex.size();

  for (int i = 0; i < this->_NumberOfFaces; i++) {

    // set face planes
    this->_FacePlanes.emplace_back(fpOutward[i].a, fpOutward[i].b,
                                   fpOutward[i].c, fpOutward[i].d);

    // face vertices
    std::vector<GeoPoint> v;

    // face vertices indexes
    std::vector<int> idx;

    // number of vertices of the face
    int count = faceVerticeIndex[i].size();

    for (int j = 0; j < count; j++) {
      idx.push_back(faceVerticeIndex[i][j]);

      v.emplace_back(vertices[idx[j]].x, vertices[idx[j]].y,
                     vertices[idx[j]].z);
    }

    // set faces
    this->_Faces.emplace_back(v, idx);
  }

  // free local memory
  Utility::FreeVectorMemory(fpOutward);
  Utility::FreeVectorListMemory<int>(faceVerticeIndex);
}

bool GeoPolygonProc::PointInside3DPolygon(double x, double y, double z)
{
  GeoPoint pt = GeoPoint(x, y, z);

  for (int i = 0; i < this->GetNumberOfFaces(); i++) {

    double dis = this->GetFacePlanes()[i] * pt;

    // If the point is in the same half space with normal vector for any face
    // of the 3D convex polygon, then it is outside of the 3D polygon
    if (dis > 0) {
      return false;
    }
  }

  // If the point is in the opposite half space with normal vector for all
  // faces, then it is inside of the 3D polygon
  return true;
}
}

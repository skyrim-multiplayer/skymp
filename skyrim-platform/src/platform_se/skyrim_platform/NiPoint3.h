// Modified version of
// https://github.com/himika/libSkyrim/blob/2559175f7f30189b7d3681d01b3e055505c3e0d7/Skyrim/include/Skyrim/NetImmerse/NiPoint3.h
#pragma once
#include <cmath>

class NiPoint3
{
public:
  NiPoint3() = default;
  NiPoint3(float X, float Y, float Z)
    : x(X)
    , y(Y)
    , z(Z){};

  float& operator[](std::size_t idx)
  {
    const float* arr = &x;
    return (float&)arr[idx];
  }
  const float& operator[](std::size_t idx) const
  {
    const float* arr = &x;
    return (float&)arr[idx];
  }

  bool operator==(const NiPoint3& rhs) const
  {
    return (x == rhs.x && y == rhs.y && z == rhs.z);
  }
  bool operator!=(const NiPoint3& rhs) const { return !operator==(rhs); }

  NiPoint3 operator+(const NiPoint3& rhs) const
  {
    return NiPoint3(x + rhs.x, y + rhs.y, z + rhs.z);
  }
  NiPoint3 operator-(const NiPoint3& rhs) const
  {
    return NiPoint3(x - rhs.x, y - rhs.y, z - rhs.z);
  }
  float operator*(const NiPoint3& rhs) const
  {
    return x * rhs.x + y * rhs.y + z * rhs.z;
  }
  NiPoint3 operator*(float scalar) const
  {
    return NiPoint3(x * scalar, y * scalar, z * scalar);
  }
  NiPoint3 operator/(float scalar) const { return (*this) * (1.0f / scalar); }
  NiPoint3 operator-() const { return NiPoint3(-x, -y, -z); }

  NiPoint3& operator+=(const NiPoint3& rhs)
  {
    x += rhs.x;
    y += rhs.y;
    z += rhs.z;
    return *this;
  }
  NiPoint3& operator-=(const NiPoint3& rhs)
  {
    x -= rhs.x;
    y -= rhs.y;
    z -= rhs.z;
    return *this;
  }
  NiPoint3& operator*=(float scalar)
  {
    x *= scalar;
    y *= scalar;
    z *= scalar;
    return *this;
  }
  NiPoint3& operator/=(float scalar) { return (*this) *= (1.0f / scalar); }

  float SqrLength() const { return x * x + y * y + z * z; }
  float Length() const { return std::sqrt(x * x + y * y + z * z); }
  float Unitize()
  {
    float length = Length();
    if (length > 1e-6f) {
      (*this) /= length;
    } else {
      x = 0.0f;
      y = 0.0f;
      z = 0.0f;
      length = 0.0f;
    }
    return length;
  }
  float QuickLength() const
  {
    const unsigned int* sqrtTable = GetSqrtTable();
    float length = SqrLength();
    unsigned int* pUIRep = (unsigned int*)&length;

    if (*pUIRep == 0) {
      length = 0.0f;
    } else {
      short exp = ((*pUIRep) >> 23) - 127;
      *pUIRep &= 0x007fffff;
      if (exp & 1)
        *pUIRep |= 0x00800000;
      exp >>= 1;
      *pUIRep = sqrtTable[(*pUIRep) >> 16] | ((exp + 127) << 23);
    }

    return length;
  }
  float QuickUnitize()
  {
    float length = QuickLength();
    if (length > 1e-6f) {
      (*this) /= length;
    } else {
      x = 0.0f;
      y = 0.0f;
      z = 0.0f;
      length = 0.0f;
    }
    return length;
  }

  NiPoint3 Cross(const NiPoint3& pt) const
  {
    return NiPoint3(y * pt.z - z * pt.y, z * pt.x - x * pt.z,
                    x * pt.y - y * pt.x);
  }
  NiPoint3 UnitCross(const NiPoint3& pt) const
  {
    NiPoint3 cross = Cross(pt);
    cross.Unitize();
    return cross;
  }

  static float VectorLength(const NiPoint3& v) { return v.QuickLength(); }

  static float UnitizeVector(NiPoint3& v) { return v.QuickUnitize(); }

  float x = 0;
  float y = 0;
  float z = 0;

private:
  static const unsigned int* GetSqrtTable()
  {
    static const unsigned int g_sqrtTable[256] = {
      0x00000000, 0x00007FC0, 0x0000FF02, 0x00017DC7, 0x0001FC10, 0x000279DF,
      0x0002F734, 0x00037413, 0x0003F07B, 0x00046C6F, 0x0004E7EE, 0x000562FC,
      0x0005DD98, 0x000657C5, 0x0006D182, 0x00074AD3, 0x0007C3B6, 0x00083C2F,
      0x0008B43D, 0x00092BE3, 0x0009A320, 0x000A19F6, 0x000A9067, 0x000B0672,
      0x000B7C1A, 0x000BF15E, 0x000C6641, 0x000CDAC3, 0x000D4EE4, 0x000DC2A7,
      0x000E360B, 0x000EA912, 0x000F1BBD, 0x000F8E0C, 0x00100000, 0x0010719A,
      0x0010E2DC, 0x001153C4, 0x0011C456, 0x00123491, 0x0012A476, 0x00131406,
      0x00138341, 0x0013F229, 0x001460BE, 0x0014CF01, 0x00153CF2, 0x0015AA92,
      0x001617E3, 0x001684E4, 0x0016F196, 0x00175DFA, 0x0017CA11, 0x001835DC,
      0x0018A15A, 0x00190C8C, 0x00197774, 0x0019E211, 0x001A4C65, 0x001AB66F,
      0x001B2032, 0x001B89AC, 0x001BF2DF, 0x001C5BCB, 0x001CC471, 0x001D2CD1,
      0x001D94EC, 0x001DFCC2, 0x001E6455, 0x001ECBA4, 0x001F32AF, 0x001F9979,
      0x00200000, 0x00206646, 0x0020CC4A, 0x0021320E, 0x00219792, 0x0021FCD7,
      0x002261DC, 0x0022C6A3, 0x00232B2B, 0x00238F75, 0x0023F383, 0x00245753,
      0x0024BAE7, 0x00251E3E, 0x0025815A, 0x0025E43B, 0x002646E1, 0x0026A94D,
      0x00270B7F, 0x00276D77, 0x0027CF36, 0x002830BC, 0x0028920A, 0x0028F31F,
      0x002953FD, 0x0029B4A4, 0x002A1514, 0x002A754D, 0x002AD550, 0x002B351D,
      0x002B94B5, 0x002BF417, 0x002C5345, 0x002CB23E, 0x002D1104, 0x002D6F95,
      0x002DCDF3, 0x002E2C1E, 0x002E8A16, 0x002EE7DB, 0x002F456F, 0x002FA2D0,
      0x00300000, 0x00305CFF, 0x0030B9CC, 0x0031166A, 0x003172D6, 0x0031CF13,
      0x00322B20, 0x003286FE, 0x0032E2AC, 0x00333E2C, 0x0033997C, 0x0033F49F,
      0x00344F93, 0x0034AA5A, 0x003504F3, 0x0035B99E, 0x00366D96, 0x003720DD,
      0x0037D375, 0x00388560, 0x003936A1, 0x0039E738, 0x003A9728, 0x003B4673,
      0x003BF51B, 0x003CA321, 0x003D5087, 0x003DFD4E, 0x003EA979, 0x003F5509,
      0x00400000, 0x0040AA5F, 0x00415428, 0x0041FD5C, 0x0042A5FE, 0x00434E0D,
      0x0043F58D, 0x00449C7E, 0x004542E1, 0x0045E8B9, 0x00468E06, 0x004732CA,
      0x0047D706, 0x00487ABC, 0x00491DEC, 0x0049C098, 0x004A62C2, 0x004B046A,
      0x004BA592, 0x004C463A, 0x004CE665, 0x004D8613, 0x004E2545, 0x004EC3FC,
      0x004F623A, 0x00500000, 0x00509D4E, 0x00513A26, 0x0051D689, 0x00527278,
      0x00530DF3, 0x0053A8FD, 0x00544395, 0x0054DDBC, 0x00557775, 0x005610BF,
      0x0056A99B, 0x0057420B, 0x0057DA10, 0x005871A9, 0x005908D9, 0x00599FA0,
      0x005A35FE, 0x005ACBF5, 0x005B6186, 0x005BF6B1, 0x005C8B77, 0x005D1FD9,
      0x005DB3D7, 0x005E4773, 0x005EDAAE, 0x005F6D87, 0x00600000, 0x00609219,
      0x006123D4, 0x0061B531, 0x00624630, 0x0062D6D3, 0x00636719, 0x0063F704,
      0x00648695, 0x006515CC, 0x0065A4A9, 0x0066332E, 0x0066C15A, 0x00674F2F,
      0x0067DCAE, 0x006869D6, 0x0068F6A9, 0x00698327, 0x006A0F50, 0x006A9B26,
      0x006B26A9, 0x006BB1D9, 0x006C3CB7, 0x006CC744, 0x006D517F, 0x006DDB6B,
      0x006E6507, 0x006EEE53, 0x006F7751, 0x00700000, 0x00708862, 0x00711076,
      0x0071983E, 0x00721FBA, 0x0072A6EA, 0x00732DCF, 0x0073B46A, 0x00743ABA,
      0x0074C0C0, 0x0075467E, 0x0075CBF2, 0x0076511E, 0x0076D603, 0x00775AA0,
      0x0077DEF6, 0x00786305, 0x0078E6CE, 0x00796A52, 0x0079ED91, 0x007A708B,
      0x007AF340, 0x007B75B1, 0x007BF7DF, 0x007C79CA, 0x007CFB72, 0x007D7CD8,
      0x007DFDFC, 0x007E7EDE, 0x007EFF7F, 0x007F7FE0
    };
    return g_sqrtTable;
  }
};

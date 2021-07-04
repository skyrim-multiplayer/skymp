#include "Utils.h"

#ifdef WIN32
#  include <cstring>
int Utils::stricmp(const char* s1, const char* s2)
{
  return ::stricmp(s1, s2);
}
#endif

#ifndef WIN32
#  include <strings.h>
int Utils::stricmp(const char* s1, const char* s2)
{
  return strcasecmp(s1, s2);
}
#endif
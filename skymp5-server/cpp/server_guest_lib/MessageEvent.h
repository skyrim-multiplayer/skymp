#pragma once
#include "RawMessageData.h"

template <class T>
struct MessageEvent
{
  RawMessageData rawMsgData;
  T message;
};

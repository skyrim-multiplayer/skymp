#pragma once

enum class FunctionType
{
  Method,         // 'callmethod' opcode
  GlobalFunction, // 'callstatic' opcode
};

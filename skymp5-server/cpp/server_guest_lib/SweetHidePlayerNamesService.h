#pragma once
class PartOne;

class SweetHidePlayerNamesService
{
public:
  explicit SweetHidePlayerNamesService(PartOne& partOne);

private:
  PartOne& partOne;
};

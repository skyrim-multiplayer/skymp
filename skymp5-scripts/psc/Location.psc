Scriptname Location extends Form Hidden

float Function GetKeywordData(Keyword akKeyword) native
int Function GetRefTypeAliveCount(LocationRefType akRefType) native
int Function GetRefTypeDeadCount(LocationRefType akRefType) native
bool Function HasCommonParent(Location akOther, Keyword akFilter = None) native
bool Function HasRefType(LocationRefType akRefType) native
bool Function IsCleared() native
bool Function IsChild(Location akOther) native
bool Function IsLoaded() native
bool Function IsSameLocation(Location akOtherLocation, Keyword akKeyword = None)
	bool bmatching = self == akOtherLocation
	if !bmatching && akKeyword
		bmatching = HasCommonParent(akOtherLocation, akKeyword)
		
		if !bmatching && akOtherLocation.HasKeyword(akKeyword)
			bmatching = akOtherLocation.IsChild(self) 
		elseif !bmatching && self.HasKeyword(akKeyword)
			bmatching = self.IsChild(akOtherLocation) 
		endif
		
	endif
  return bmatching
endFunction
Function SetKeywordData(Keyword akKeyword, float afData) native
Function SetCleared(bool abCleared = true) native
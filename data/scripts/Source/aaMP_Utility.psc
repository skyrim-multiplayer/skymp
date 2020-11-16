Scriptname aaMP_Utility   

; ======== Functions ========

;/
	Функция проверяет принадлежность к интервалу и возвращает bool значение.
	
	value					-	Проверяемое значение.
	moreThenValue			-	Более чем X.
	lessAndEqualThenValue	-	Менее или равно X.
/;
bool function comparePointInInterval(float value, float moreThenValue, float lessAndEqualThenValue) global
	return ((value > moreThenValue) && (value <= lessAndEqualThenValue))
endFunction
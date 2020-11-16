Scriptname aaMP_SatietySystemSatietyService

;/
	Скрипт, глобальные вспомогательные функции  и конфигурационные переменные которого используются в системе голода игрока.
/;


; ======== Config Variable ========


;/
	Возвращает значение осков сытости, которое будет уменьшаться за единицу времени.
/;
float function satietyPointRemovePerTimeUnit() global
	return 6.0
endFunction


;/
	Возвращает значение времени, раз в которое будет происходить уменьшение сытости игрока и обработка событий.
/;
float function satietyUpdateTimeUnit() global
	return 5.0
endFunction


;/
	Возвращает модификатор эффективности, зависящий от текущего значения сытости
	При значении 0.05 для лимита модификатора 80.0 функция #getAddSatietyModificator(...) вернет 1.0,
	для нижнего лимита 20.0 функция вернет #getAddSatietyModificator(...) 4.0.
/;
float function satietyModificatorPerValue() global
	return 0.05
endFunction


;/
	Возвращает лимит сытости, выше которого модификатор эффективности сытости не учитывается.
/;
float function satietyModificatorLimitValue() global
	return 80.0
endFunction


; ======== Functions ========

;/
	Функции определяет значение стадии сытости игрока на основании очков сытости,
	и заполняет переданный массив значениями нового интервала сытости и новой стадией.
	Работает на основе упрощенного поиска по бинарному древу.
	satietyValue		-	Значение сытости игрока.
	satietyStageValues	-	Массив, в котором:
	[0]					-	Значение проверки интервала более X
	[1]					-	Значение проверки интервала менее или равно X
	[2]					-	id стадии сытости
/;
function findActualSatietyStage(float satietyValue, float[] satietyStageValues) global
	if (aaMP_Utility.comparePointInInterval(satietyValue, -100.0, 60.0))
		if (aaMP_Utility.comparePointInInterval(satietyValue, -100.0, 20.0))
			if (aaMP_Utility.comparePointInInterval(satietyValue, -100.0, 0.0))
				satietyStageValues[0] = -100.0
				satietyStageValues[1] = 0.0
				satietyStageValues[2] = 0.0
			else
				satietyStageValues[0] = 0.0
				satietyStageValues[1] = 20.0
				satietyStageValues[2] = 1.0
			endIf
		else
			if (aaMP_Utility.comparePointInInterval(satietyValue, 20.0, 40.0))
				satietyStageValues[0] = 20.0
				satietyStageValues[1] = 40.0
				satietyStageValues[2] = 2.0
			else
				satietyStageValues[0] = 40.0
				satietyStageValues[1] = 60.0
				satietyStageValues[2] = 3.0
			endIf
		endIf
	else
		if (aaMP_Utility.comparePointInInterval(satietyValue, 60.0, 100.0))
			if (aaMP_Utility.comparePointInInterval(satietyValue, 60.0, 80.0))
				satietyStageValues[0] = 60.0
				satietyStageValues[1] = 80.0
				satietyStageValues[2] = 4.0
			else
				satietyStageValues[0] = 80.0
				satietyStageValues[1] = 100.0
				satietyStageValues[2] = 5.0
			endIf
		else
			if (aaMP_Utility.comparePointInInterval(satietyValue, 100.0, 120.0))
				satietyStageValues[0] = 100.0
				satietyStageValues[1] = 120.0
				satietyStageValues[2] = 6.0
			else
				satietyStageValues[0] = 120.0
				satietyStageValues[1] = 200.0
				satietyStageValues[2] = 7.0
			endIf
		endIf
	endIf
endFunction
ScriptName aaMp_SatietySystemPlayerController extends Actor  

; ======== Properties ========
Faction property satietyFaction Auto

; ======== Variables ========

float satietyValue = 110.0
float tempSatietyValue

;/
	ћассив, который хранит в себе значени€ текущего диапазона проверки сытости и стадию сытости.
	[0]	-	значение проверки интервала более X
	[1]	-	значение проверки интервала менее или равно X
	[2]	-	id стадии сытости
/;
float[] satietyValuesArray

; ======== Event ========

event OnInit()
	RegisterForSingleUpdate(aaMP_SatietySystemSatietyService.satietyUpdateTimeUnit())
	satietyValuesArray = new float[3]
	satietyValuesArray[0] = 100.0
	satietyValuesArray[1] = 120.0
	satietyValuesArray[2] = 6.0
	AddToFaction(satietyFaction)
endEvent

;/
	»вент, кототрый при обновлении уменьшает текущее значение сытости и пытаетс€ обновить стадию сытости.
/;
event onUpdate()
	if (!IsEnabled())
		return
	endIf
	
	removeSatietyPoint(aaMP_SatietySystemSatietyService.satietyPointRemovePerTimeUnit())
	checkAndHandleSturveDeath()
	updateCurrentSatietyStage()
	Debug.Notification("—ытость: " + satietyValue)
	RegisterForSingleUpdate(aaMP_SatietySystemSatietyService.satietyUpdateTimeUnit())
endEvent


; ======== Functions ========

;/
	‘ункци€ отнимает переданное число от текущего состо€ни€ сытости, если это возможно.
	inputSatietyValue	-	отнимаемое число сытости.

	!Ќевозможно уменьшить меньше 0.
/;
function removeSatietyPoint(float inputSatietyValue)
	tempSatietyValue = satietyValue - inputSatietyValue
	if (tempSatietyValue < 0.0)
		satietyValue = 0
	else
		satietyValue = tempSatietyValue
	endIf
endFunction


;/
	‘ункци€ провер€ет, равна ли сытость игрока 0.
	≈сли да - убивает игрока и восстановит ему сытость до 50 (времено, требует обсуждени€).
/;
function checkAndHandleSturveDeath()
	if (satietyValue == 0.0)
		Kill()
		satietyValue = 50.0
	endIf
endFunction


;/
	‘ункци€ добавл€ет переданное число к текущему состо€ни€ сытости с учетом эффективности модификатора сытости, если это возможно.
	inputSatietyValue	-	прибавл€емое число сытости.

	!Ќевозможно прибавить больше 120.
/;
function addSatietyPoint(float inputSatietyValue)
	tempSatietyValue = satietyValue + inputSatietyValue * getCalculateSatietyModificator()
	if (tempSatietyValue > 120.0)
		satietyValue = 120.0
	else 
		satietyValue = tempSatietyValue
	endIf
endFunction


;/
	‘ункци€ возвращаетрасчитанный модификатор эффективности насыщени€.
/;
float function getCalculateSatietyModificator()
	if(satietyValue >= aaMP_SatietySystemSatietyService.satietyModificatorLimitValue())
		return 1.0
	else
		return (1.0 + ((aaMP_SatietySystemSatietyService.satietyModificatorLimitValue() - satietyValue) * aaMP_SatietySystemSatietyService.satietyModificatorPerValue()))
	endif
		
endFunction


;/
	‘ункци€ провер€ет, находитс€ ли текущаю сытость в рамках текущего диапазона. 
	ѕри изменении диапазона инициализируетс€ смена стадии голода, 
	и на основании новой стадии мен€етс€ ранг во фракции голода.
/;
function updateCurrentSatietyStage()
	if (!aaMP_Utility.comparePointInInterval(satietyValue, satietyValuesArray[0], satietyValuesArray[1]))
		aaMP_SatietySystemSatietyService.findActualSatietyStage(satietyValue, satietyValuesArray)
		SetFactionRank(satietyFaction, (satietyValuesArray[2] as int))
	endIf
endFunction


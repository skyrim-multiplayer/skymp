Scriptname Math Hidden

; Calculates the absolute value of the passed in value - N for N, and N for (-N)
float Function abs(float afValue) global native

; Calculates the arccosine of the passed in value, returning degrees
float Function acos(float afValue) global native

; Calculates the arcsine of the passed in value, returning degrees
float Function asin(float afValue) global native

; Calculates the arctangent of the passed in value, returning degrees
float Function atan(float afValue) global native

; Calculates the ceiling of the passed in value - the smallest integer greater than or equal to the value
int Function Ceiling(float afValue) global native

; Calculates the cosine of the passed in value (in degrees)
float Function cos(float afValue) global native

; Converts degrees to radians
float Function DegreesToRadians(float afDegrees) global native

; Calculates the floor of the passed in value - the largest integer less than or equal to the value
int Function Floor(float afValue) global native

; Calculates x raised to the y power
float Function pow(float x, float y) global native

; Converts radians to degrees
float Function RadiansToDegrees(float afRadians) global native

; Calculates the sine of the passed in value (in degrees)
float Function sin(float afValue) global native

; Calculate the square root of the passed in value
float Function sqrt(float afValue) global native

; Calculates the tangent of the passed in value (in degrees)
float Function tan(float afValue) global native

; SKSE 64 additions built 2020-07-29 17:24:48.495000 UTC
int Function LeftShift(int value, int shiftBy) global native
int Function RightShift(int value, int shiftBy) global native
int Function LogicalAnd(int arg1, int arg2) global native
int Function LogicalOr(int arg1, int arg2) global native
int Function LogicalXor(int arg1, int arg2) global native
int Function LogicalNot(int arg1) global native
float Function Log(float arg1) global native

float Function min(float[] value) global native
float Function max(float[] value) global native

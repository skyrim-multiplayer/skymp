namespace FridaHooksUtils {
void* GetMenuByName(void* name);
bool SetMenuNumberVariable(void* fsName, const char* target, double value);
double GetMenuNumberVariable(void* fsName, const char* target);
void SaveCursorPosition();
float* GetCursorX();
float* GetCursorY();
int GetNthVtableElement(void* pointer, int pointerOffset, int elementIndex);
}

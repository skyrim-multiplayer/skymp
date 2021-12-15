namespace FridaHooksUtils {
void* GetMenuByName(void* name);
bool SetMenuNumberVariable(void* fsName, const char* target, double value);
double GetMenuNumberVariable(void* fsName, const char* target);
std::pair<double, double> GetCursorPosition();
float* GetCursorX();
float* GetCursorY();
}

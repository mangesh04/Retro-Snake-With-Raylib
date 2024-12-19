#ifndef listener_H
#define listener_H
int listenForShortCutKey();
void extra_features();
void installHook();
void wait();
void uninstallHook();
void wait();
void SetRoundedWindow(int cornerRadius);
extern bool minWin;
typedef void *HWND;
extern HWND hwnd;
typedef int WINBOOL;
WINBOOL IsIconic(HWND hwnd);
HWND GetActiveWindow();
bool isMin();
#endif
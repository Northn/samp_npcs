#include "hooks.h"

BOOL APIENTRY DllMain(HMODULE hModule, DWORD fdwReason, LPVOID lpReserved) {
  if (fdwReason == DLL_PROCESS_ATTACH) {
    npcs_module::hooks::install();
  } else if (fdwReason == DLL_PROCESS_DETACH) {
    npcs_module::destroy();
  }
  return TRUE;
}

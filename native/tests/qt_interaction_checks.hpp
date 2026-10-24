#pragma once
#include <functional>
namespace luma { class WorkbenchWindow; }
void interactionCheck(bool ok, const char* message);
void interactionSettle();
void checkMediaInteractions(luma::WorkbenchWindow&);
void checkProjectInteractions(luma::WorkbenchWindow&);
void checkControlInteractions(luma::WorkbenchWindow&);

/*
 *   This file is only there to link some functions into the executable,
 *   which are not linked in otherwise. This is, because XFree has some
 *   libraries only in static versions
 *   Since the function below is never called, we don't have to care about
 *   proper prototyping
 */

void XF86VidModeAddModeLine();
void XF86VidModeDeleteModeLine();
void XF86VidModeGetAllModeLines();
void XF86VidModeGetDotClocks();
void XF86VidModeGetGamma();
void XF86VidModeGetModeLine();
void XF86VidModeGetMonitor();
void XF86VidModeGetViewPort();
void XF86VidModeLockModeSwitch();
void XF86VidModeModModeLine();
void XF86VidModeQueryExtension();
void XF86VidModeQueryVersion();
void XF86VidModeSetClientVersion();
void XF86VidModeSetGamma();
void XF86VidModeSetViewPort();
void XF86VidModeSwitchMode();
void XF86VidModeSwitchToMode();
void XF86VidModeValidateModeLine();

static void Ignore_warning()
  {
  XF86VidModeAddModeLine();
  XF86VidModeDeleteModeLine();
  XF86VidModeGetAllModeLines();
  XF86VidModeGetDotClocks();
  XF86VidModeGetGamma();
  XF86VidModeGetModeLine();
  XF86VidModeGetMonitor();
  XF86VidModeGetViewPort();
  XF86VidModeLockModeSwitch();
  XF86VidModeModModeLine();
  XF86VidModeQueryExtension();
  XF86VidModeQueryVersion();
  XF86VidModeSetClientVersion();
  XF86VidModeSetGamma();
  XF86VidModeSetViewPort();
  XF86VidModeSwitchMode();
  XF86VidModeSwitchToMode();
  XF86VidModeValidateModeLine();
  }

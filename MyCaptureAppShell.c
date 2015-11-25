/*	File:		MyCaptureAppShell.c		Contains:	MyCaptureApp shell.	Written by:	John Wang	Copyright:	� 1994 by Apple Computer, Inc., all rights reserved.	Change History (most recent first):		<1>		04/04/94	JW		Created.	To Do:	*/#ifdef THINK_C#define		applec#endif#include	"MyHeaders"/*#include	<Types.h>#include	<Memory.h>#include	<QuickDraw.h>#include	<Palettes.h>#include	<QDOffscreen.h>#include	<Errors.h>#include	<Fonts.h>#include	<Dialogs.h>#include	<Windows.h>#include	<Menus.h>#include	<Events.h>#include	<Desk.h>#include	<DiskInit.h>#include	<OSUtils.h>#include	<Resources.h>#include	<ToolUtils.h>#include	<AppleEvents.h>#include	<EPPC.h>#include	<GestaltEqu.h>#include	<Processes.h>#include	<Balloons.h>#include	<Aliases.h>#include	<MixedMode.h>#include	<Scrap.h>#include	<LowMem.h>*/#include	"MyCaptureAppShell.h"#include	"MySGStuff.h"#include	"MyUtils.h"#ifdef powercQDGlobals		qd;#endifBoolean			gDoneFlag;			//	Set to true if you want to Application to kindly quit./* ------------------------------------------------------------------------- */void main(){	EventRecord 	myEvent;	long			yieldTime;	WindowPtr		foundWindow;	short			windowPart;	Boolean			isEvent;	GrafPtr			savePort;	GDHandle		saveGD;	PScrapStuff		myScrapStuff;	//	Initialize here.  Set the yield time too.	Initialize();	MyAdjustMenus();	yieldTime = MyYieldTime(suspendResumeMessage);	//	Event loop.	for ( ; ; ) {		//	Get the event.		isEvent = WaitNextEvent(everyEvent, &myEvent, yieldTime, nil);		//	If the event is unhandled by app specific event handling, then we proceed.		if ( isEvent ) {			switch ( myEvent.what ) {				case mouseDown:					//	Get current port and device.					GetPort(&savePort);					saveGD = GetGDevice();										//	Set the port and gdevice to the window if we own the window.					//	We can then assume anytime the event occured in one of our windows,					//	   that the port and gdevice are set correctly.					windowPart = FindWindow(myEvent.where, &foundWindow);					SetPort(foundWindow);					SetGDevice(GetMainDevice());										//	Handle the different mouse down events.					switch ( windowPart ) {						case inSysWindow:							SystemClick(&myEvent, foundWindow);							break;						case inMenuBar:							MyAdjustMenus();							DoCommand(MenuSelect(myEvent.where));							MyAdjustMenus();							break;						case inContent:							break;						case inDrag:							//	If dragging one of the application's windows, then handle it.							//	However, if we are dragging a zoomed window, we							//	must remember to save the new window location into the							//	zoomed rect in the data handle.  Otherwise, the event							//	manager will think that we are no longer zoomed.							{								WStateData	*zoomData;								Rect		windowRect;																//	Get window location before drag.								GetGlobalWindow(foundWindow, &windowRect);																//	Drag window.								MyDrag(foundWindow, myEvent.where);								//	If the windowRect in global coordinates matches the zoom rect,								//	then assume that we are dragging the zoomed window.  update								//	zoom rect.								zoomData = (WStateData *) *(((CWindowPeek) foundWindow)->dataHandle);								if ( EqualRect(&(zoomData->stdState), &windowRect) ) {									GetGlobalWindow(foundWindow, &windowRect);									zoomData = (WStateData *) *(((CWindowPeek) foundWindow)->dataHandle);									zoomData->stdState = windowRect;								}							}							break;						case inGrow:							break;						case inGoAway:							//	Handle clicking on the go away.  If it is the clip window,							//	then hide it.							if ( TrackGoAway (foundWindow, myEvent.where) ) {								BringToFront(foundWindow);								MyClose();							}							break;						case inZoomIn:						case inZoomOut:							break;						default:							break;					}										//	Restore port and device.					SetPort(savePort);					SetGDevice(saveGD);										break;				case keyDown:				case autoKey:					if ( myEvent.modifiers & cmdKey ) {						if ( myEvent.what == keyDown ) {							MyAdjustMenus();							DoCommand(MenuKey(myEvent.message & charCodeMask));							MyAdjustMenus();						}					}					break;				case updateEvt:					//	Handle update events for window and clip window.					foundWindow = (WindowPtr) myEvent.message;					GetPort(&savePort);					saveGD = GetGDevice();					SetPort(foundWindow);					SetGDevice(GetMainDevice());					BeginUpdate(foundWindow);					EndUpdate(foundWindow);					MyUpdate(foundWindow);					SetPort(savePort);					SetGDevice(saveGD);					break;				case diskEvt:					//	This handles a bad disk.  Otherwise the disk will not eject.					if ( myEvent.message >> 16 ) {						Point	tempPoint;						tempPoint.v = 50; tempPoint.h = 50;						DIBadMount(tempPoint, myEvent.message);					}					break;				case activateEvt:					break;				case app4Evt:					switch ( myEvent.message >> 24 ) {						case suspendResumeMessage:							yieldTime = MyYieldTime(myEvent.message & 0x01);							break;						default:							DebugStr("\pUnexpected suspend/resume message.");					}					break;				case kHighLevelEvent:					AEProcessAppleEvent(&myEvent);				default:					break;			}		}			MyIdle();		//	If DoneFlag set, then quit.		if ( gDoneFlag ) {			MyFinishup();			ExitToShell();		}	}}/* ------------------------------------------------------------------------- */void Initialize(){	long				err;	long				vers;	Handle				myMenu;	Boolean				hasAppleEvents;		//	Initialize Managaer.	MaxApplZone();	MoreMasters(); MoreMasters(); MoreMasters(); MoreMasters();	MoreMasters(); MoreMasters(); MoreMasters(); MoreMasters();	MoreMasters(); MoreMasters(); MoreMasters(); MoreMasters();	MoreMasters(); MoreMasters(); MoreMasters(); MoreMasters();	InitGraf(&qd.thePort);	FlushEvents(everyEvent, 0);	InitWindows();	InitDialogs(nil);	InitCursor();	//	Set up menus.	myMenu = GetNewMBar(kMENUBAR);	SetMenuBar(myMenu);	DisposHandle(myMenu);	AddResMenu(GetMHandle(kMENU_APPLEID), 'DRVR');	DrawMenuBar();		//	Require at least System 7.0 and Color QuickDraw.  We don't really need to be strict	//	about this.  So, this can be removed if necessary.  But, test thoroughly with	//	System 6 and non-color if you do.	Gestalt(gestaltSystemVersion, &vers);	vers = (vers >> 8) & 0x0f;	if ( vers < 7 )		ReportFatal("\pThis Application does not run under System 6!", 0);			Gestalt(gestaltQuickdrawVersion, &vers);	if ( vers < 0x100 )		ReportFatal("\pThis Application requires Color QuickDraw!", 0);		//	Initialize AppleEvents if available.	hasAppleEvents = (Gestalt(gestaltAppleEventsAttr, &vers) == noErr);	if ( !hasAppleEvents )		ReportFatal("\pThis Application requires AppleEvents!", 0);	err = AEInstallEventHandler(kCoreEventClass, kAEOpenApplication, NewAEEventHandlerProc(AEOpenHandler), 0, false);	if ( err != noErr )		ReportFatal("\pError installing AppleEvent handlers.", err);	err = AEInstallEventHandler(kCoreEventClass, kAEOpenDocuments, NewAEEventHandlerProc(AEOpenDocHandler), 0, false);	if ( err != noErr )		ReportFatal("\pError installing AppleEvent handlers.", err);	err = AEInstallEventHandler(kCoreEventClass, kAEQuitApplication, NewAEEventHandlerProc(AEQuitHandler), 0, false);	if ( err != noErr )		ReportFatal("\pError installing AppleEvent handlers.", err);	err = AEInstallEventHandler(kCoreEventClass, kAEPrintDocuments, NewAEEventHandlerProc(AEPrintHandler), 0, false);	if ( err != noErr )		ReportFatal("\pError installing AppleEvent handlers.", err);		//	Setup other globals.	gDoneFlag = false;	//	Call app specific Intialization.	err = MyInitialize();	if ( err != noErr )		ReportFatal("\pError returned from MyInitialize:", err);}void DoCommand(long mResult){	short 			theMenu, theItem;	Str255			myStr;	GrafPtr			savePort;	GDHandle		saveGD;	WindowPtr		foundWindow;		theItem = LoWord(mResult);	theMenu = HiWord(mResult);		if ( theItem != 0 || theMenu != 0 ) {		switch ( theMenu ) {			case kMENU_APPLEID:				if ( theItem == 1 ) {					ParamText("\pSample QuickTime Recording Application", "\pUses Sequence Grabber.", nil, nil);					Alert(kALERT_ABOUT, nil);				} else {					GetItem(GetMHandle(kMENU_APPLEID), theItem, myStr);					GetPort(&savePort);					saveGD = GetGDevice();					(void) OpenDeskAcc(myStr);					SetPort(savePort);					SetGDevice(saveGD);				}				break;				case kMENU_FILEID:				switch ( theItem ) {					case kMENU_FILENEW:						MyNew();						break;					case kMENU_FILECLOSE:						MyClose();						break;					case kMENU_FILEQUIT:						gDoneFlag = true;						break;					default:						ReportFatal("\pError in handling file menu:", theItem);				}				break;				case kMENU_SETTINGSID:				MySettings(theItem);				break;				case kMENU_RESIZEID:				MyResize(theItem);				break;				case kMENU_SPECIALID:				MySpecial(theItem);				break;				case kMENU_RECORDID:				MyRecord();				break;				default:				ReportFatal("\pError in handling menu:", theMenu);		}	}	HiliteMenu(0);}/* ------------------------------------------------------------------------- *///	AppleEvents handling (AEOpenHandler, AEOpenDocHandler, AEPrintHandler,//	   and AECloseHandler) :pascal OSErr AEOpenHandler(AppleEvent *messagein, AppleEvent *reply, long refIn){		MyNew();	MyAdjustMenus();	return ( noErr );}pascal OSErr AEOpenDocHandler(AppleEvent *messagein, AppleEvent *reply, long refIn){	return ( noErr );}pascal OSErr AEPrintHandler(AppleEvent *messagein, AppleEvent *reply, long refIn){	return ( noErr );}pascal OSErr AEQuitHandler(AppleEvent *messagein, AppleEvent *reply, long refIn){	gDoneFlag = true;		return ( noErr );}
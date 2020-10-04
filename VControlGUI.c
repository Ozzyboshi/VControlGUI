/*
Copyright (C) 2020-2021 Alessio Garzi <gun101@email.it>
This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the
License, or (at your option) any later version.
This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
General Public License for more details.
You should have received a copy of the GNU General Public
License along with this program; if not, write to the
Free Software Foundation, Inc., 59 Temple Place - Suite 330,
Boston, MA 02111-1307, USA.
*/

/****************************************************/
/*** VControlGUI                                  ***/
/*** Use vcontrol with your mouse                 ***/
/****************************************************/

#include <proto/intuition.h>
#include <proto/gadtools.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <intuition/intuition.h>
#include <libraries/gadtools.h>
#include <libraries/asl.h>

#include <proto/asl.h>
#include <stdio.h>
#include <string.h>


//#define DEBUG 0

#define WINDOW_WIDTH 640
#define WINDOW_TITLE (unsigned int)"VControlGUI"
#define KICKSTART_PATH (unsigned int)"Devs:Kickstarts"

#define MAXBUTTONS 13
#define MAXLABELS 9

#define VREG_FASTIDE    0xdd1020

// C Protos
int readTurtle();
int readSS();
void readStates();
void updateStrGad(struct Window *, struct Gadget *, UBYTE *);

// Asm protos
int v_cpu_cacr();
int v_cpu_pcr();
int v_fpu_isok();
int readFpu();

// Global variables
int closewin = FALSE;
struct Window *myWindow;
struct Gadget *myViewGadgets[MAXLABELS];
struct Gadget* myTxtGadgets[1];

void exitFunction()
{
  closewin = TRUE;
  return;
}

void turtleFunctionOn()
{
  Execute("Vcontrol TU=1", 0, 0);
#ifdef DEBUG
  printf("Turtlemode Activated\n");
#endif
  readStates();
}

void turtleFunctionOff()
{
  Execute("Vcontrol TU=0", 0, 0);
#ifdef DEBUG
  printf("Turtlemode Deactivated\n");
#endif
  readStates();
}

void superscalarFunctionOn()
{
  Execute("Vcontrol SS=1", 0, 0);
#ifdef DEBUG
  printf("Superscalarmode Activated\n");
#endif
  readStates();
}

void superscalarFunctionOff()
{
  Execute("Vcontrol SS=0", 0, 0);
#ifdef DEBUG
  printf("Superscalarmode Deactivated\n");
#endif
  readStates();
}

void fpuFunctionOn()
{
  Execute("Vcontrol FP=1", 0, 0);
#ifdef DEBUG
  printf("FPU Activated\n");
#endif
  readStates();
}

void fpuFunctionOff()
{
  Execute("Vcontrol FP=0", 0, 0);
#ifdef DEBUG
  printf("FPU Deactivated\n");
#endif
  readStates();
}

void ideFunctionSlow()
{
  Execute("Vcontrol ID=0", 0, 0);
  //*((volatile UWORD*)VREG_FASTIDE) = 0x0000;
#ifdef DEBUG
  printf("Ide set to SLOW\n");
#endif
  readStates();
}

void ideFunctionFast()
{
  Execute("Vcontrol ID=1", 0, 0);
#ifdef DEBUG
  printf("Ide set to FAST\n");
#endif
  readStates();
}

void ideFunctionFaster()
{
  Execute("Vcontrol ID=2", 0, 0);
#ifdef DEBUG
  printf("Ide set to Faster\n");
#endif
  readStates();
}

void ideFunctionFastest()
{
  Execute("Vcontrol ID=3", 0, 0);
#ifdef DEBUG
  printf("Ide set to Fastest\n");
#endif
  readStates();
}

void attnSet()
{
  Execute("forceattn_844f", 0, 0);
#ifdef DEBUG
  printf("forceattn_844f launched\n");
#endif
  readStates();
}

void kickstartSelector()
{
  struct FileRequester *request;
  UBYTE fname[255 * 10];

  /* Create a FileRequester structure */
  request = (struct FileRequester *)AllocAslRequestTags(ASL_FileRequest,
                                                        ASL_Hail, (unsigned int)"Open File", TAG_END);

  /* Now show file requester */
  if (AslRequestTags(request,
                     ASLFR_Window, (unsigned int)myWindow,
                     ASLFR_InitialLeftEdge, 20,
                     ASLFR_InitialTopEdge, 20,
                     ASLFR_InitialWidth, 300,
                     ASLFR_InitialHeight, 350,
                     ASLFR_InitialDrawer, KICKSTART_PATH,
                     ASLFR_InitialPattern, (unsigned int)"#?",
                     ASLFR_PositiveText, (unsigned int)"Open",
                     TAG_END))
  {

    /* Build filename */
    strcpy(fname, "Vcontrol MR ");
    strcat(fname, request->rf_Dir);
    if (fname[strlen(fname) - 1] != (UBYTE)58) /* Check for : */
      strcat(fname, "/");
    strcat(fname, request->rf_File);
#ifdef DEBUG
    printf("Selected ###%s###\n", fname);
#endif

    Execute(fname, 0, 0);
  }
}

// Setup functions associated with buttons
void (*sg_pBtnFunctions[MAXBUTTONS])() = {turtleFunctionOn, turtleFunctionOff, kickstartSelector, superscalarFunctionOn, superscalarFunctionOff, fpuFunctionOn, fpuFunctionOff, ideFunctionSlow, ideFunctionFast, ideFunctionFaster, ideFunctionFastest, attnSet, exitFunction};

APTR visual;

/* Type of gadgets to display */
ULONG Gadgetkinds[MAXBUTTONS] = {BUTTON_KIND, BUTTON_KIND, BUTTON_KIND, BUTTON_KIND, BUTTON_KIND, BUTTON_KIND, BUTTON_KIND, BUTTON_KIND, BUTTON_KIND, BUTTON_KIND, BUTTON_KIND, BUTTON_KIND, BUTTON_KIND};
ULONG GadgetViewkinds[MAXLABELS] = {STRING_KIND, STRING_KIND , STRING_KIND, STRING_KIND, STRING_KIND, STRING_KIND,  STRING_KIND, STRING_KIND, STRING_KIND};

struct TextAttr topaz8 = {
    (STRPTR) "topaz.font", 2, 0, 1};

/* Data for gadget structures */
struct NewGadget Gadgetdata[MAXBUTTONS] = {
    10, 30, 172, 13, (UBYTE *)"Enable Turtle mode", &topaz8, 1, PLACETEXT_IN, NULL, NULL,
    10, 50, 172, 13, (UBYTE *)"Disable Turtle mode", &topaz8, 2, PLACETEXT_IN, NULL, NULL,

    10, 70, 172, 13, (UBYTE *)"Kickstart", &topaz8, 3, PLACETEXT_IN, NULL, NULL,

    10, 90, 172, 13, (UBYTE *)"Superscalar ON", &topaz8, 4, PLACETEXT_IN, NULL, NULL,
    10, 110, 172, 13, (UBYTE *)"Superscalar OFF", &topaz8, 5, PLACETEXT_IN, NULL, NULL,

    10, 130, 172, 13, (UBYTE *)"Fpu ON", &topaz8, 6, PLACETEXT_IN, NULL, NULL,
    10, 150, 172, 13, (UBYTE *)"Fpu OFF", &topaz8, 7, PLACETEXT_IN, NULL, NULL,

    // Ide set sepeed
    10, 170, 130, 13, (UBYTE *)"Vamp IDE slow", &topaz8, 8, PLACETEXT_IN, NULL, NULL,
    140, 170, 130, 13, (UBYTE *)"Vamp IDE fast", &topaz8, 9, PLACETEXT_IN, NULL, NULL,
    270, 170, 130, 13, (UBYTE *)"Vamp IDE faster", &topaz8, 10, PLACETEXT_IN, NULL, NULL,
    400, 170, 130, 13, (UBYTE *)"Vamp IDE fastest", &topaz8, 11, PLACETEXT_IN, NULL, NULL,

    10, 190, 172, 13, (UBYTE *)"Launch ForceATTN_844F", &topaz8, 12, PLACETEXT_IN, NULL, NULL,

    WINDOW_WIDTH/2-54/2, 210, 54, 31, (UBYTE *)"Exit", &topaz8, 13, PLACETEXT_IN, NULL, NULL};

struct NewGadget GadgetViewdata[MAXLABELS] = {
    190,
    40,
    172,
    13,
    (UBYTE *)"",
    &topaz8,
    12,
    PLACETEXT_IN,
    NULL,
    NULL,

    190,
    100,
    172,
    13,
    (UBYTE *)"",
    &topaz8,
    13,
    PLACETEXT_IN,
    NULL,
    NULL,

    190,
    140,
    172,
    13,
    (UBYTE *)"",
    &topaz8,
    14,
    PLACETEXT_IN,
    NULL,
    NULL,

    380,
    40,
    200,
    13,
    (UBYTE *)"",
    &topaz8,
    15,
    PLACETEXT_LEFT,
    NULL,
    NULL,

    190,
    60,
    430,
    13,
    (UBYTE *)"",
    &topaz8,
    15,
    PLACETEXT_LEFT,
    NULL,
    NULL,

    190,
    80,
    430,
    13,
    (UBYTE *)"",
    &topaz8,
    15,
    PLACETEXT_LEFT,
    NULL,
    NULL,

    380,
    100,
    200,
    13,
    (UBYTE *)"",
    &topaz8,
    15,
    PLACETEXT_LEFT,
    NULL,
    NULL,

    380,
    120,
    200,
    13,
    (UBYTE *)"",
    &topaz8,
    15,
    PLACETEXT_LEFT,
    NULL,
    NULL,

    380,
    140,
    200,
    13,
    (UBYTE *)"",
    &topaz8,
    15,
    PLACETEXT_LEFT,
    NULL,
    NULL,
};

struct NewGadget GadgetTxtdata[1] = {
  390, 70, 172, 44, (UBYTE *)"Enable Turtle mode", &topaz8, 1, PLACETEXT_IN, NULL, NULL
};

/* Extra information for gadgets using Tags */
ULONG GadgetTags[] = {
    (GTST_MaxChars), 256, (TAG_DONE),
    (GTNM_Border), TRUE, (TAG_DONE),
    (TAG_DONE)};

ULONG GadgetViewTags[] = {
    (GTST_MaxChars), 256, (TAG_DONE),
    (GTNM_Border), TRUE, (TAG_DONE),
    (TAG_DONE)};

  
  ULONG txtTL[] = { GTTX_Border,TRUE,GTTX_Text,(ULONG)"lol",TAG_END};
  ULONG *ggTL [] = { NULL,NULL,NULL,NULL};


// Start main
int main(void)
{
  struct Screen *pubScreen;
  struct Gadget *myGadgets[3], *glist = NULL, *gad1;

  int i;
  struct IntuiMessage *msg;
  ULONG msgClass;

  /* Lock screen and get visual info for gadtools */
  if (pubScreen = LockPubScreen(NULL))
  {
    if (!(visual = GetVisualInfo(pubScreen, TAG_DONE)))
    {
      printf("Failed to get visual info.\n");
      return (5);
    }
  }
  else
  {
    printf("Failed to lock screen.\n");
    return (5);
  }
  /* Create the gadget list */
  if (!(gad1 = CreateContext(&glist)))
  {
    printf("Failed to create gadtools context.\n");
    return (5);
  }
  /* Create gadgets specify gadget kind, a Gadget, NewGadget data and extra tag info */
  for (i = 0; i < MAXBUTTONS; i++)
  {
    Gadgetdata[i].ng_VisualInfo = visual;
    if (myGadgets[i] = gad1 = CreateGadgetA(Gadgetkinds[i], gad1, &Gadgetdata[i], (struct TagItem *)&GadgetTags[0]))
    {
      #ifdef DEBUG
      printf("Gadget %d created.\n", i);
      #endif
    }
    else
    {
      #ifdef DEBUG
      printf("Failed to create gadget %d.\n", i);
      #endif
    }
  }

  for (i = 0; i < MAXLABELS; i++)
  {
    GadgetViewdata[i].ng_VisualInfo = visual;
    struct TagItem* tags = (struct TagItem*)txtTL[i];
    myViewGadgets[i] = gad1 = CreateGadgetA(GadgetViewkinds[i], gad1, &GadgetViewdata[i],NULL);
  }

  //myTxtGadgets[i] = gad1 = CreateGadgetA(STRING_KIND, gad1, &GadgetTxtdata[0], (struct TagItem *)&GadgetTags[0]);
  

  /* Open window and specify gadget list (glist) */
  myWindow = OpenWindowTags(NULL,
                            WA_Left, 10, WA_Top, 15,
                            WA_Width, WINDOW_WIDTH, WA_Height, 250,
                            WA_IDCMP, IDCMP_CLOSEWINDOW | IDCMP_GADGETUP,
                            WA_Flags, WFLG_SIZEGADGET | WFLG_DRAGBAR | WFLG_DEPTHGADGET | WFLG_CLOSEGADGET | WFLG_ACTIVATE | WFLG_SMART_REFRESH,
                            WA_Gadgets, (unsigned int)glist,
                            WA_Title, WINDOW_TITLE,
                            WA_PubScreenName, (unsigned int)"Workbench",
                            TAG_DONE);

  readStates();

  GT_RefreshWindow(myWindow, NULL); /* Update window */

  while (closewin == FALSE)
  {
    Wait(1L << myWindow->UserPort->mp_SigBit);
    msg = GT_GetIMsg(myWindow->UserPort);
    msgClass = msg->Class;
    GT_ReplyIMsg(msg);
    if (msgClass == IDCMP_CLOSEWINDOW)
    {
      closewin = TRUE;
    }
    if (msgClass == GADGETUP)
    {
      struct Gadget *gadAddr;
      UWORD gadgetid;
      gadAddr = (struct Gadget *)msg->IAddress;
      gadgetid = gadAddr->GadgetID;
#ifdef DEBUG
      printf("\n------\n pressed %d\n", gadgetid);
#endif
      (*sg_pBtnFunctions[gadgetid - 1])();
    }
  }
  if (myWindow)
    CloseWindow(myWindow);
  /* Free gadgets */
  if (glist)
    FreeGadgets(glist);

  if (visual)
    FreeVisualInfo(visual);
  if (pubScreen)
    UnlockPubScreen(NULL, pubScreen);
  return (0);
}

void readStates()
{

  int state = readTurtle();
  if (state)
    updateStrGad(myWindow, myViewGadgets[0], (UBYTE *)"Turtle disabled");
  else
    updateStrGad(myWindow, myViewGadgets[0], (UBYTE *)"Turtle enabled");

  state = readSS();
  if (state)
    updateStrGad(myWindow, myViewGadgets[1], (UBYTE *)"Sscalar enabled");
  else
    updateStrGad(myWindow, myViewGadgets[1], (UBYTE *)"Scalar disabled");

  state = readFpu();
  if (state)
    updateStrGad(myWindow, myViewGadgets[2], (UBYTE *)"Fpu enabled");
  else
    updateStrGad(myWindow, myViewGadgets[2], (UBYTE *)"Fpu disabled");

    char buf[300];
    BPTR cpuMsg=0;
    
    Execute("VControl cpu >RAM:cpuinfo",0,0);
    cpuMsg = Open("ram:cpuinfo",MODE_OLDFILE);
    memset(buf,0,300);
    Read( cpuMsg, buf, 200);
    //printf(" #####\n%s\n#####",buf);
    Close(cpuMsg);
    DeleteFile("RAM:cpuinfo");
    
    char* ptr = buf;
    while (*ptr && *ptr!='\n')
    {
      ptr++;
    }

    if (*ptr) ptr++;
    while (*ptr && *ptr!='\n')
    {
      ptr++;
    }

    if (*ptr) ptr++;
    char cpuinfo[100];
    strcpy(cpuinfo,"");
    while (*ptr && *ptr!='\n')
    {
      if (strlen(cpuinfo)<99) sprintf(cpuinfo,"%s%c",cpuinfo,*ptr);
      ptr++;
    }

    updateStrGad(myWindow, myViewGadgets[3], cpuinfo);

    if (*ptr) ptr++;
    char fpuinfo[100];
    strcpy(fpuinfo,"");
    while (*ptr && *ptr!='\n')
    {
      if (strlen(fpuinfo)<99) sprintf(fpuinfo,"%s%c",fpuinfo,*ptr);
      ptr++;
    }

    updateStrGad(myWindow, myViewGadgets[6], fpuinfo);

    if (*ptr) ptr++;
    char pcrinfo[100];
    strcpy(pcrinfo,"");
    while (*ptr && *ptr!='\n')
    {
      if (strlen(pcrinfo)<99) sprintf(pcrinfo,"%s%c",pcrinfo,*ptr);
      ptr++;
    }

    updateStrGad(myWindow, myViewGadgets[5], pcrinfo);

    if (*ptr) ptr++;
    char vbrinfo[100];
    strcpy(vbrinfo,"");
    while (*ptr && *ptr!='\n')
    {
      if (strlen(vbrinfo)<99) sprintf(vbrinfo,"%s%c",vbrinfo,*ptr);
      ptr++;
    }

    updateStrGad(myWindow, myViewGadgets[4], vbrinfo);

    if (*ptr) ptr++;
    char cacrinfo[100];
    strcpy(cacrinfo,"");
    while (*ptr && *ptr!='\n')
    {
      if (strlen(cacrinfo)<99) sprintf(cacrinfo,"%s%c",cacrinfo,*ptr);
      ptr++;
    }

    updateStrGad(myWindow, myViewGadgets[7], cacrinfo);

    Execute("VControl af >RAM:af",0,0);
    cpuMsg = Open("ram:af",MODE_OLDFILE);
    memset(buf,0,300);
    Read( cpuMsg, buf, 200);
    //printf(" #####\n%s\n#####",buf);
    Close(cpuMsg);
    DeleteFile("RAM:af");

    char attninfo[100];
    strcpy(attninfo,"");
    ptr=buf;
    while (*ptr && *ptr!='\n')
    {
      if (strlen(attninfo)<99) sprintf(attninfo,"%s%c",attninfo,*ptr);
      ptr++;
    }

    updateStrGad(myWindow, myViewGadgets[8], attninfo);
}

int readTurtle()
{
  return v_cpu_cacr() & (1 << 15);
}

int readSS()
{
  return v_cpu_pcr() & (1 << 0);
}

int readFpu()
{
  return v_fpu_isok();
}

/*
** Routine to update the value in the string gadget's buffer, then
** activate the gadget.
*/
VOID updateStrGad(struct Window *win, struct Gadget *gad, UBYTE *newstr)
{
  /* first, remove the gadget from the window.  this must be done before
** modifying any part of the gadget!!!
*/
  RemoveGList(win, gad, 1);

  /* For fun, change the value in the buffer, as well as the cursor and
** initial display position.
*/
  strcpy(((struct StringInfo *)(gad->SpecialInfo))->Buffer, newstr);
#if 1
  ((struct StringInfo *)(gad->SpecialInfo))->BufferPos = 0;
  ((struct StringInfo *)(gad->SpecialInfo))->DispPos = 0;

  /* Add the gadget back, placing it at the end of the list (~0)
** and refresh its imagery.
*/
  AddGList(win, gad, ~0, 1, NULL);
  RefreshGList(gad, win, NULL, 1);
#endif

  /* Activate the string gadget */
  ActivateGadget(gad, win, NULL);
}

#if 0


/* ASL File Request Example 
   by Peter Hutchison
*/
#include <proto/intuition.h>
#include <proto/gadtools.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/asl.h>
#include <intuition/intuition.h>
#include <string.h>
/* Answer goes into %s parameter */
   struct EasyStruct ResultReq = {
     sizeof(struct EasyStruct),
     0,
     "Answer",
     "%s",
     "OK"
   };
   struct FileRequester *request;
   UBYTE fname[255];
int main(void) {
    struct Window *myWindow;
   
    myWindow = OpenWindowTags(NULL,
      WA_Left, 20, WA_Top, 20,
      WA_Width, 200, WA_Height, 150,
      WA_IDCMP, IDCMP_CLOSEWINDOW,
      WA_Flags, WFLG_SIZEGADGET | WFLG_DRAGBAR | WFLG_DEPTHGADGET    | WFLG_CLOSEGADGET | WFLG_ACTIVATE,
      WA_Title, "My Window",
      WA_PubScreenName, "Workbench",
      TAG_DONE);

    /* Create a FileRequester structure */
    request = (struct FileRequester *)AllocAslRequestTags(ASL_FileRequest, 
      ASL_Hail, "Open File", (struct TagItem *)TAG_DONE);
   
    /* Now show file requester */
    if (0 && AslRequestTags(request,
      ASLFR_Window, myWindow, 
      ASLFR_InitialLeftEdge, 20,
      ASLFR_InitialTopEdge, 20,
      ASLFR_InitialWidth, 300,
      ASLFR_InitialHeight, 350,
      ASLFR_InitialDrawer, "Sys:Work",
      ASLFR_InitialPattern, "#?.txt",
      ASLFR_PositiveText, "Open",
      (struct TagItem *)TAG_DONE)) {
   
       /* Build filename */
   
      strcat( fname, request->rf_Dir);
      if (fname[strlen(fname)-1] != (UBYTE)58) /* Check for : */
         strcat(fname, "/");
      strcat(fname, request->rf_File);
   
      /* Display filename */
      //EasyRequest(myWindow, &ResultReq, NULL, fname);
      struct Gadget **glistptr;
      struct Gadget *gad;
      struct NewGadget ng;


      gad = CreateContext(glistptr);


      CreateGadget(BUTTON_KIND, gad, &ng,
                    GT_Underscore, '_',
                    TAG_END);
   }
   else {
        //EasyRequest(myWindow, &ResultReq, NULL, "File request cancelled");
   }
   /* Free requester */
   if (request) FreeAslRequest(request);
   if (myWindow) CloseWindow(myWindow);
   
   return(0);
   }
#endif

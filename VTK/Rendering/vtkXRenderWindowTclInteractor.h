/*=========================================================================

  Program:   Visualization Toolkit
  Module:    $RCSfile: vtkXRenderWindowTclInteractor.h,v $

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkXRenderWindowTclInteractor - a TCL event driven interface for a RenderWindow
// .SECTION Description
// vtkXRenderWindowTclInteractor is a convenience object that provides event
// bindings to common graphics functions. For example, camera and actor
// functions such as zoom-in/zoom-out, azimuth, roll, and pan. IT is one of
// the window system specific subclasses of vtkRenderWindowInteractor. Please
// see vtkRenderWindowInteractor documentation for event bindings.
//
// .SECTION see also
// vtkRenderWindowInteractor vtkXRenderWindow


#ifndef __vtkXRenderWindowTclInteractor_h
#define __vtkXRenderWindowTclInteractor_h

//===========================================================
// now we define the C++ class

#include "vtkRenderWindowInteractor.h"
#include <X11/StringDefs.h> // Needed for X types in public interface
#include <X11/Intrinsic.h> // Needed for X types in public interface

extern void vtkXRenderWindowTclInteractorCallback(Widget,XtPointer,
                                                  XEvent *,Boolean *);
extern void vtkXRenderWindowTclInteractorTimer(XtPointer,XtIntervalId *);

class VTK_RENDERING_EXPORT vtkXRenderWindowTclInteractor : public vtkRenderWindowInteractor
{
public:
  static vtkXRenderWindowTclInteractor *New();
  vtkTypeRevisionMacro(vtkXRenderWindowTclInteractor,vtkRenderWindowInteractor);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Initializes the event handlers without an XtAppContext.  This is
  // good for when you don't have a user interface, but you still
  // want to have mouse interaction.
  virtual void Initialize();

  // Description: 
  // Initializes the event handlers using an XtAppContext that you have
  // provided.  This assumes that you want to own the event loop.
  virtual void Initialize(XtAppContext app);
  vtkGetMacro( App, XtAppContext );
  
  // Description:
  // Enable/Disable interactions.  By default interactors are enabled when
  // initialized.  Initialize() must be called prior to enabling/disabling
  // interaction. These methods are used when a window/widget is being
  // shared by multiple renderers and interactors.  This allows a "modal"
  // display where one interactor is active when its data is to be displayed
  // and all other interactors associated with the widget are disabled
  // when their data is not displayed.
  virtual void Enable();
  virtual void Disable();

  // Description:
  // This will start up the X event loop and never return. If you
  // call this method it will loop processing X events until the
  // application is exited.
  virtual void Start();
  virtual void UpdateSize(int,int);

  // Description:
  // Specify the Xt widget to use for interaction. This method is
  // one of a couple steps that are required for setting up a
  // vtkRenderWindowInteractor as a widget inside of another user 
  // interface. You do not need to use this method if the render window
  // will be a stand-alone window. This is only used when you want the
  // render window to be a subwindow within a larger user interface.
  // In that case, you must tell the render window what X display id
  // to use, and then ask the render window what depth, visual and 
  // colormap it wants. Then, you must create an Xt TopLevelShell with
  // those settings. Then you can create the rest of your user interface
  // as a child of the TopLevelShell you created. Eventually, you will 
  // create a drawing area or some other widget to serve as the rendering
  // window. You must use the SetWidget method to tell this Interactor
  // about that widget. It's X and it's not terribly easy, but it looks cool.
  virtual void SetWidget(Widget);
  Widget GetWidget() 
    {return this->top;}

  // Description
  // This method will store the top level shell widget for the interactor.
  // This method and the method invocation sequence applies for:
  //     * vtkRenderWindow-Interactor pair in a nested widget hierarchy
  //     * multiple vtkRenderWindow-Interactor pairs in the same top 
  //       level shell
  //
  // It is not needed for:
  //     * vtkRenderWindow-Interactor pair as the direct child of a 
  //       top level shell,
  //     * multiple vtkRenderWindow-Interactor pairs, each in its own
  //       top level shell.
  //
  // This method, along with EnterNotify event, changes the keyboard focus
  // among the widgets/vtkRenderWindow(s) so the Interactor(s) can receive
  // the proper keyboard events. The following calls need to be made:
  //     * vtkRenderWindow's display ID need to be set to the top level 
  //       shell's display ID.
  //     * vtkXRenderWindowInteractor's Widget has to be set to the 
  //       vtkRenderWindow's container widget.
  //     * vtkXRenderWindowInteractor's TopLevel has to be set to the 
  //       top level shell widget.
  //
  // Note that the procedure for setting up render window in a widget needs
  // to be followed.  See vtkRenderWindowInteractor's SetWidget() method.
  //
  // If multiple vtkRenderWindow-Interactor pairs in SEPARATE windows are
  // desired, do not set the display ID (Interactor will create them as
  // needed.  Alternatively, create and set distinct DisplayID for each
  // vtkRenderWindow. Using the same display ID without setting the parent
  // widgets will cause the display to be reinitialized every time an
  // interactor is initialized), do not set the widgets (so the render
  // windows would be in their own windows), and do not set TopLevelShell
  // (each has its own top level shell already)
  virtual void SetTopLevelShell(Widget);
  Widget GetTopLevelShell() 
    {return this->TopLevelShell;}

  // Description:
  // X timer methods.
  int CreateTimer(int timertype);
  int DestroyTimer(void);

  // Description:
  // X Tcl specific application terminate. 
  void TerminateApp(void);

  // Description:
  // Set this flag to break the event loop.
  vtkGetMacro(BreakLoopFlag, int);
  vtkSetMacro(BreakLoopFlag, int);

  // Description:
  // Functions that are used internally.
  friend void vtkXRenderWindowTclInteractorCallback(Widget,XtPointer,
                                                    XEvent *,Boolean *);
  friend void vtkXRenderWindowTclInteractorTimer(XtPointer,XtIntervalId *);

protected:
  vtkXRenderWindowTclInteractor();
  ~vtkXRenderWindowTclInteractor();

  Widget TopLevelShell;

  Display *DisplayId;
  Window WindowId;
  Atom KillAtom;
  Widget top;
  Widget oldTop;
  XtAppContext App;
  int PositionBeforeStereo[2];

  int BreakLoopFlag;
private:
  vtkXRenderWindowTclInteractor(const vtkXRenderWindowTclInteractor&);  // Not implemented.
  void operator=(const vtkXRenderWindowTclInteractor&);  // Not implemented.
};

#endif

// MFCKit2004View.cpp : implementation of the CMFCKit2004View class
//

#include "stdafx.h"
#include "MFCKit2004.h"

#include "MFCKit2004Doc.h"
#include "MFCKit2004View.h"
#include "internal.h"
#include "propdlg.h"
#include "gl/gl.h"
#include "gl/glu.h"
#include "KVgui.h"
#include "NewSurfaceDlg.h"

#include <string>
#include <fstream>

#pragma warning (disable : 4800)
#pragma warning (disable : 4018)

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

double modelView[16];		// the modelview Matrix
int viewPort[4];			// viewport - actually [width, height, 0, 0]

extern SEGMENT *list;
extern GLint fuzziness;
extern GLdouble sensitive;
/////////////////////////////////////////////////////////////////////////////
// CMFCKit2004View

IMPLEMENT_DYNCREATE(CMFCKit2004View, CView)

BEGIN_MESSAGE_MAP(CMFCKit2004View, CView)
	//{{AFX_MSG_MAP(CMFCKit2004View)
	ON_WM_CHAR()
	ON_WM_LBUTTONUP()
	ON_WM_MOUSEMOVE()
	ON_WM_LBUTTONDOWN()
	ON_WM_RBUTTONDOWN()
	ON_WM_RBUTTONUP()
	ON_WM_TIMER()
	ON_WM_CREATE()
	ON_WM_SIZE()
	ON_WM_ERASEBKGND()
	ON_WM_PAINT()
	ON_WM_MOUSEWHEEL()
	//}}AFX_MSG_MAP
	ON_COMMAND(ID_FUZZINESS_MORE, OnFuzzinessMore)
	ON_COMMAND(ID_FUZZINESS_LESS, OnFuzzinessLess)
	ON_COMMAND(ID_OPTIONS_RESET, OnOptionsReset)
	ON_COMMAND(ID_ROTATION_MORE, OnRotationMore)
	ON_COMMAND(ID_ROTATION_LESS, OnRotationLess)
	ON_COMMAND(ID_TRANSLATION_MORE, OnTranslationMore)
	ON_COMMAND(ID_TRANSLATION_LESS, OnTranslationLess)
	ON_COMMAND(ID_SCALING_MORE, OnScalingMore)
	ON_COMMAND(ID_SCALING_LESS, OnScalingLess)
	ON_COMMAND(ID_FILE_NEW, OnFileNew)
	ON_COMMAND(ID_FILE_OPEN, OnFileOpen)

	ON_COMMAND(ID_CONTEXTBG_NEWBEZIERCURVE, &CMFCKit2004View::OnContextbgNewbeziercurve)
	ON_COMMAND(ID_CONTEXTBG_CLEARALL, &CMFCKit2004View::OnContextbgClearall)
	ON_COMMAND(ID_CONTEXTPOLYGON_INSERTPOINT, &CMFCKit2004View::OnContextpolygonInsertpoint)
	ON_COMMAND(ID_CONTEXTPOLYGON_APPENDPOINT, &CMFCKit2004View::OnContextpolygonAppendpoint)
	ON_COMMAND(ID_CONTEXTPOLYGON_SHOW, &CMFCKit2004View::OnContextpolygonShowHideControlPolygon)
	ON_COMMAND(ID_CONTEXTBG_NEWBSPLINECURVE, &CMFCKit2004View::OnContextbgNewbsplinecurve)
	ON_COMMAND(ID_CONTEXTPT_ADJUSTWEIGHT, &CMFCKit2004View::OnContextptAdjustweight)
	ON_UPDATE_COMMAND_UI(ID_CONTEXTPT_ADJUSTWEIGHT, &CMFCKit2004View::OnUpdateContextptAdjustweight)
	ON_COMMAND(ID_CONTEXTPOLYGON_RAISEDEGREE, &CMFCKit2004View::OnContextpolygonRaisedegree)
	ON_COMMAND(ID_CONTEXTPOLYGON_SUBDIVIDEATT, &CMFCKit2004View::OnContextpolygonSubdivide)
	ON_COMMAND(ID_CONTEXTPT_REMOVEPOINT, &CMFCKit2004View::OnContextptRemovepoint)
	ON_COMMAND(ID_OPTIONS_SHOWGRID, &CMFCKit2004View::OnOptionsShowgrid)
	ON_COMMAND(ID_FILE_SAVE32808, &CMFCKit2004View::OnFileSaveGeometry)
	ON_COMMAND(ID_FILE_SAVE, &CMFCKit2004View::OnFileSaveGeometry)
	ON_COMMAND(ID_CONTEXTBSPLINEPOLY_MODIFYKNOTVECTOR, &CMFCKit2004View::OnContextbsplinepolyModifyknotvector)
	ON_COMMAND(ID_CONNECTTOWITHCONTINUITY_G0, &CMFCKit2004View::OnConnecttowithcontinuityG0)
	ON_COMMAND(ID_CONNECTTOWITHCONTINUITY_G1, &CMFCKit2004View::OnConnecttowithcontinuityG1)
	ON_COMMAND(ID_CONNECTTOWITHCONTINUITY_C1, &CMFCKit2004View::OnConnecttowithcontinuityC1)
	ON_COMMAND(ID_KNOTGUI_REMOVEKNOT, &CMFCKit2004View::OnKnotguiRemoveknot)
	ON_COMMAND(ID_KNOTGUI_INSERTKNOT, &CMFCKit2004View::OnKnotguiInsertknot)
	ON_COMMAND(ID_CONTEXTBSPLINEPOLY_APPENDPOINT, &CMFCKit2004View::OnContextbsplinepolyAppendpoint)
	ON_COMMAND(ID_KNOTGUI_INSERTKNOT32820, &CMFCKit2004View::OnKnotguiInsertknotBoehm)
	ON_COMMAND(ID_MODIFYKNOTVECTOR_U, &CMFCKit2004View::OnModifyknotvectorU)
	ON_COMMAND(ID_MODIFYKNOTVECTOR_V, &CMFCKit2004View::OnModifyknotvectorV)
	ON_COMMAND(ID_CONTEXTBG_NEWSURFACE, &CMFCKit2004View::OnContextbgNewsurface)
	ON_COMMAND(ID_SURFACES_GLOBALS, &CMFCKit2004View::OnSurfacesGlobals)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CMFCKit2004View construction/destruction

CMFCKit2004View::CMFCKit2004View() {
	SSense = RSense = TSense = 0.5;
	RButtonDown = LButtonDown = false;
	CtrlKeyDown = false;
	selectedT = -1; // outside of bounds -> nothing drawn
	draggedPolyline = 0;		
	draggedPoint = -1;			
	draggedCircle = 0;			
	draggedCtlPt = NULL;	

	m_offsetD = 0.5;


	m_curveIdx = 0;
	m_lastCurvature = -1;
	m_lastTorsion = -1;
	
	m_showAxes = false;



	// bezier stuff below here
	m_state = StateIdle;
	m_currCurveIdx = -1;
	m_lastWeightControlStatus = false;
	m_bsplineDegree = 3;
	m_modifiedCurveIdx = -1;



	m_modifiedAxis = AxisUndef;


}

CMFCKit2004View::~CMFCKit2004View() {

}

BOOL CMFCKit2004View::PreCreateWindow(CREATESTRUCT& cs) {
	// TODO: Modify the Window class or styles here by modifying
	//  the CREATESTRUCT cs

	cs.style |= WS_CLIPSIBLINGS | WS_CLIPCHILDREN;
	return CView::PreCreateWindow(cs);
}
//  reset the scene, wither create new curve by specifying the params in the
// dialog box, or proceed to load a new curve from a file:
void CMFCKit2004View::OnFileNew() {
	// TODO: Add your command handler code here
}


#include <sstream>
//  load a new curve from a file
void CMFCKit2004View::OnFileOpen() 
{

	CFileDialog fileDialog(TRUE, ".dat", NULL, 0, NULL, NULL, 0);
	INT_PTR nResult = fileDialog.DoModal();
	if(IDCANCEL == nResult) 
	{
		Invalidate();
		return;
	}


	OnOptionsReset();

	m_parser2 = SurfaceFileParser();

	CString pathName = fileDialog.GetPathName();
	CString fileName = fileDialog.GetFileTitle();
	m_filename = std::string(pathName);
	bool parseRes = false;
	
	parseRes = m_parser2.parse_file(m_filename);


	if (parseRes)
	{
		
		m_bs = BsplineSurface(m_parser2.m_temp_surf);
		m_bs.Draw();
	}


	Invalidate();
}

// This method initialized the OpenGL system.
bool CMFCKit2004View::InitializeOpenGL()
{
    m_pDC = new CClientDC(this);

    if ( NULL == m_pDC ) { // failure to get DC
	    ::AfxMessageBox("Couldn't get a valid DC.");
        return FALSE;
	}

    if ( !SetupPixelFormat() ) {
		::AfxMessageBox("SetupPixelFormat failed.\n");
        return FALSE;
	}

    if ( 0 == (m_hRC = ::wglCreateContext( m_pDC->GetSafeHdc() ) ) ) {
		::AfxMessageBox("wglCreateContext failed.");
        return FALSE;
	}

    if ( FALSE == ::wglMakeCurrent( m_pDC->GetSafeHdc(), m_hRC ) ) {
	    ::AfxMessageBox("wglMakeCurrent failed.");
        return FALSE;
	}

    // specify black as clear color
    ::glClearColor( 0.75f, 0.75f, 0.75f, 0.0f );
    // specify the back of the buffer as clear depth (0 closest, 1 farmost)
    ::glClearDepth( 1.0f );
    // enable depth testing (Enable zbuffer - hidden surface removal)
    ::glEnable( GL_DEPTH_TEST );
	// Set default black as background color.
	::glClearColor(0.75, 0.75, 0.75, 1.0f);


    return TRUE;
}



bool CMFCKit2004View::SetupPixelFormat(PIXELFORMATDESCRIPTOR* pPFD)
{
    // default pixel format for a single-buffered,
    // OpenGL-supporting, hardware-accelerated, 
    // RGBA-mode format. Pass in a pointer to a different
    // pixel format if you want something else
    PIXELFORMATDESCRIPTOR pfd = {
        sizeof(PIXELFORMATDESCRIPTOR),// size of this pfd
        1,                      // version number
        PFD_DRAW_TO_WINDOW |    // support window
          PFD_SUPPORT_OPENGL |  // support OpenGL
		  PFD_DOUBLEBUFFER,     // double buffered
        PFD_TYPE_RGBA,          // RGBA type
        24,                     // 24-bit color depth
        0, 0, 0, 0, 0, 0,       // color bits ignored
        0,                      // no alpha buffer
        0,                      // shift bit ignored
        0,                      // no accumulation buffer
        0, 0, 0, 0,             // accum bits ignored
        16,                     // 16-bit z-buffer
        0,                      // no stencil buffer
        0,                      // no auxiliary buffer
        PFD_MAIN_PLANE,         // main layer
        0,                      // reserved
        0, 0, 0                 // layer masks ignored
	};

    int pixelformat;
    PIXELFORMATDESCRIPTOR* pPFDtoUse;

    // let the user override the default pixel format
    pPFDtoUse = (0 == pPFD)? &pfd : pPFD; 
 
    if ( 0 == (pixelformat = ::ChoosePixelFormat( m_pDC->GetSafeHdc(), pPFDtoUse )) ) {
	    ::AfxMessageBox("ChoosePixelFormat failed.");
	    return FALSE;
    }

    if ( FALSE == ::SetPixelFormat(m_pDC->GetSafeHdc(), pixelformat, pPFDtoUse ) ) {
	    ::AfxMessageBox("SetPixelFormat failed.");
        return FALSE;
    }

    return TRUE;
}


bool CMFCKit2004View::SetupViewingFrustum(void)
{
    // select a default perspective viewing volumn
    ::gluPerspective( 40.0f, m_AspectRatio, 0.1f, 20.0f );

    // NOTE: Other commands you could have here are
    // glFrustum, which gives you much more control over
    // the perspective view, or glOrtho which is used for
    // parallel projections. No matter what you use, check
    // the error status when you set the viewing frustum!

    // if ( GL_NO_ERROR != ::glGetError() ) {
	//   ::AfxMessageBox("Error while trying to set viewing frustum.");
	//    return FALSE;
    //}

    return TRUE;
}


bool CMFCKit2004View::SetupViewingOrtho(void)
{
    // select a default perspective viewing volumn
    ::glOrtho( -2, 2, -2, 2, -2, 2);
	// This gives a projection around the (-2,-2,-2), (2,2,2) coordinate.
	// The fifth and six parameter are near and far. Since it is used as
	// -near and -far, than this gives us the actual near (positive z) to be 2
	// and far (nefative z) to be -2.
	// Note that the view point is from (0,0,0) (yes, inside) towards the negative z.

    // NOTE: Other commands you could have here are
    // glFrustum, which gives you much more control over
    // the perspective view, or glOrtho which is used for
    // parallel projections. No matter what you use, check
    // the error status when you set the viewing frustum!

    //if ( GL_NO_ERROR != ::glGetError() ) {
	//    ::AfxMessageBox("Error while trying to set viewing frustum.");
	//    return FALSE;
    //}

    return TRUE;
}


// This viewing projection gives us a constant aspect ration. This is done by
// increasing the corresponding size of the ortho cube.
bool CMFCKit2004View::SetupViewingOrthoConstAspect(void)
{
	double windowSize = 200;	// the size of the window in GL coord system.


	if ( m_AspectRatio > 1 ) {	// x is bigger than y.
		// Maintain y size and increase x size (and z) accordingly
		// by MULTIPLYING by the aspect ration.
	    ::glOrtho(  -windowSize*m_AspectRatio/2.0, windowSize*m_AspectRatio/2.0,
					-windowSize/2.0, windowSize/2.0,
					-30*windowSize*m_AspectRatio/2.0, 30*windowSize*m_AspectRatio/2.0);
//					0, 30*windowSize*m_AspectRatio);
	} else {
		// Maintain x size and increase y size (and z) accordingly, 
		// by DIVIDING the aspect Ration (because it's smaller than 1).
	    ::glOrtho(  -windowSize/2.0, windowSize/2.0,
					-windowSize/m_AspectRatio/2.0, windowSize/m_AspectRatio/2.0,
//					-windowSize/m_AspectRatio/2.0, windowSize/m_AspectRatio/2.0);
					0, 3*windowSize/m_AspectRatio);
	}

//    if ( GL_NO_ERROR != ::glGetError() ) {
//	    ::AfxMessageBox("Error while trying to set viewing frustum.");
//	    return FALSE;
//	}

    return TRUE;
}


/////////////////////////////////////////////////////////////////////////////
// CMFCKit2004View drawing

void CMFCKit2004View::OnDraw(CDC* pDC) {
//	CMFCKit2004Doc* pDoc = GetDocument();
//	ASSERT_VALID(pDoc);
	drawSegments(GL_RENDER);
	glFlush();
	SwapBuffers(wglGetCurrentDC());

	// TODO: add draw code for native data here
}


void CMFCKit2004View::OnPaint() {
	CPaintDC dc(this); // device context for painting
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);		// clear screen and zbuffer
	drawSegments(GL_RENDER);

	glFlush();
	SwapBuffers(wglGetCurrentDC());

//	OnDraw(&dc);
//	drawSegments(GL_RENDER);
//	glFlush();
//	SwapBuffers(wglGetCurrentDC());
}
/////////////////////////////////////////////////////////////////////////////
// CMFCKit2004View diagnostics

#ifdef _DEBUG
void CMFCKit2004View::AssertValid() const {
	CView::AssertValid();
}

void CMFCKit2004View::Dump(CDumpContext& dc) const {
	CView::Dump(dc);
}

CMFCKit2004Doc* CMFCKit2004View::GetDocument() /* non-debug version is inline*/ {
	ASSERT(m_pDocument->IsKindOf(RUNTIME_CLASS(CMFCKit2004Doc)));
	return (CMFCKit2004Doc*)m_pDocument;
}
#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CMFCKit2004View message handlers

int CMFCKit2004View::OnCreate(LPCREATESTRUCT lpCreateStruct) 
{
	if (CView::OnCreate(lpCreateStruct) == -1)
		return -1;
	
	InitializeOpenGL();
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glGetDoublev(GL_MODELVIEW_MATRIX,modelView);
//	SetupViewingOrthoConstAspect();
	OnOptionsReset();
	
	//Scale(100.0);

	return 0;
}

BOOL CMFCKit2004View::OnEraseBkgnd(CDC* pDC) 
{
	// TODO: Add your message handler code here and/or call default
	//return CView::OnEraseBkgnd(pDC);
	return true;
}

void CMFCKit2004View::OnChar(UINT nChar, UINT nRepCnt, UINT nFlags) {
	if (nChar == 61) //+
	{
		Scale(2.0);
	}
	if (nChar == 45) //-
	{
		Scale(0.5);
	}
	Invalidate();
//	CView::OnChar(nChar, nRepCnt, nFlags);
}

void CMFCKit2004View::OnLButtonDown(UINT nFlags, CPoint point) {
	SetCapture();	// capture the mouse 'right button up' command
	m_prevMouseLocation = point;
	m_mouseDownLocation = point;
	LButtonDown = true;


	if(! ::GetAsyncKeyState(VK_CONTROL))
	{ 
		m_bs.OnLButtonDown(point.x, point.y);
	}	


	if(m_state == StateIdle)
	{
		bool found = m_kvmgr.attemptAnchor(point.x, point.y);
		if (found)
			return;
		m_weightCtrlAnchor = m_mgr.AttemptWeightAnchor(point.x, point.y);
		// look for the point within the curves
		m_draggedPt.m_pt = m_mgr.PickControlPoint(point.x, point.y);
		if(m_draggedPt.m_pt.IsValid())
		{
			// dragging stuff
			m_draggedPt.m_startPosScreen = CCagdPoint(point.x, point.y);
		}
	}
	

	/*int frameIdx = m_ffmgr.PickFrame(point.x, point.y);
	if(frameIdx >= 0) 
	{
		ASSERT(m_ffmgr.GetFrameCount() > frameIdx);
		m_curveIdx = frameIdx;
		DrawFrenetComponents(m_curveIdx);
	}*/

}

void CMFCKit2004View::OnLButtonUp(UINT nFlags, CPoint point) {
	if (GetCapture() == this) ::ReleaseCapture();
	LButtonDown = false;
	
	// release the dragged point
	m_draggedPt.m_pt.Invalidate();
	m_weightCtrlAnchor.Invalidate();
	m_kvmgr.dropAnchor();

	CCagdPoint cp[2];
	cagdToObject(point.x, point.y, cp);
	m_lastLbuttonUp = cp[0];
	m_lastLbuttonUp.z = 0.0;


	m_bs.OnLButtonUp();
	
	//======================================================
	// OLD BEZIER CODE
	//======================================================
	if(StateAddBezierPts == m_state)
	{
		CCagdPoint coord[2];
		cagdToObject(point.x, point.y, coord);
		coord[0].z = 0.0;
		m_mgr.AddLastCtrlPt(coord[0], 1, m_currCurveIdx);
		
	}
	else if(StateAddBSplinePts == m_state)
	{
		CCagdPoint coord[2];
		cagdToObject(point.x, point.y, coord);
		coord[0].z = 0.0;
		m_mgr.AddLastCtrlPt(coord[0], 1, m_currCurveIdx);
		if (m_modifiedCurveIdx == m_currCurveIdx)
		{
			vector<double> kv = m_mgr.GetKnotVector(m_currCurveIdx);
			m_kvmgr.setVector(kv);
			m_kvmgr.show();
		}
	}
	else if (m_state == StateConnectingCurvesG0 || m_state == StateConnectingCurvesG1 || m_state == StateConnectingCurvesC1)
	{
		int idx2 = m_mgr.getCurveIndexByPointOnPolygon(m_lastLbuttonUp);
		if (idx2 != -1)
		{
			if (m_state == StateConnectingCurvesG0)
				m_mgr.connectG0(m_currCurveIdx, idx2);
			else if (m_state == StateConnectingCurvesG1)
				m_mgr.connectG1(m_currCurveIdx, idx2);
			else if (m_state == StateConnectingCurvesC1)
				m_mgr.connectC1(m_currCurveIdx, idx2);

			m_state = StateIdle;
		}
	}

	Invalidate();					// redraw scene
}

void CMFCKit2004View::OnRButtonDown(UINT nFlags, CPoint point) {
	SetCapture();	// capture the mouse 'right button up' command
	m_prevMouseLocation = point;
	RButtonDown = true;

}

void CMFCKit2004View::OnRButtonUp(UINT nFlags, CPoint point) {
	if (GetCapture() == this) ::ReleaseCapture();
	RButtonDown = false;

	if (StateIdle != m_state)
	{
		m_state = StateIdle;
		return;
	}

	// this code pops up the menu called "IDR_POPUPMENU" on the screen at coordinate "point"
	CCagdPoint cp[2];
	cagdToObject(point.x, point.y, cp);
	m_lastRbuttonUp = cp[0];
	m_lastRbuttonUp.z = 0.0;

	CMenu popupMenu;
	popupMenu.LoadMenu(IDR_POPUPMENU);	

	CMenu* subMenu = popupMenu.GetSubMenu(0);
	ClientToScreen(&point);
	subMenu->TrackPopupMenu(0, point.x, point.y,
	   AfxGetMainWnd(), NULL);
	Invalidate();
}

void CMFCKit2004View::OnMouseMove(UINT nFlags, CPoint point) {

	if ((GetCapture() != this) || (!LButtonDown)) 		// 'this' has the mouse capture
	{ return; }

	if (m_state != StateIdle)
		return;

	




	if(::GetAsyncKeyState(VK_CONTROL))
	{ 
		// This is the movement ammount 
		int valuex(point.x - m_prevMouseLocation.x);
		int valuey(point.y - m_prevMouseLocation.y);	
		if (LButtonDown) // rotateXY
			RotateXY(valuey*RSense,valuex*RSense);
		else if (RButtonDown) // rotateZ
			RotateZ(valuex);
	}
	else if (::GetAsyncKeyState(VK_SHIFT))
	{
		int valuex(point.x - m_prevMouseLocation.x);
		int valuey(point.y - m_prevMouseLocation.y);	
		if (LButtonDown)  // translateXY
			translateXY(valuex*TSense,-valuey*TSense);

	}
	else
	{
		// dragging points of control mesh
		CCagdPoint p1[2];
		CCagdPoint p2[2];
		cagdToObject(m_prevMouseLocation.x, m_prevMouseLocation.y, p1);
		//cagdToObject(m_mouseDownLocation.x, m_mouseDownLocation.y, p1);
		cagdToObject(point.x, point.y, p2);
		CCagdPoint diff = p2[0] - p1[0];
		m_bs.OnMouseMove(diff);



		if (m_kvmgr.isAnchored())
		{
			m_kvmgr.updateLastAnchor(point.x, point.y);
			if (m_kvmgr.changedSinceLastGet(0.05))
			{
				vector<double> vec = m_kvmgr.getVector();
				if (m_modifiedAxis == AxisU)
					m_bs.SetKnotVectorU(vec);
				if (m_modifiedAxis == AxisV)
					m_bs.SetKnotVectorV(vec);
			}
		}
	}

	Invalidate();					// redraw scene
	m_prevMouseLocation = point;
}

BOOL CMFCKit2004View::OnMouseWheel(UINT nFlags, short zDelta, CPoint pt) {
	UINT stateCtrl	= ::GetAsyncKeyState(VK_CONTROL);
	UINT stateShift = ::GetAsyncKeyState(VK_SHIFT);
	if(stateCtrl)
	{
		// u
		DrawPt ptUV = m_bs.GetDrawPt();
		double newU = ptUV.m_u + double(zDelta)/5000.0;
		ptUV.m_u = newU;
		m_bs.SetDrawPt(ptUV);
		std::stringstream strm;
		strm << "U updated. Eval point now (" << ptUV.m_u << "," << ptUV.m_v << ")." << std::endl;
		::OutputDebugString((LPCSTR)strm.str().c_str());
		m_bs.Draw();
	}
	else if(stateShift)
	{
		// v
		DrawPt ptUV = m_bs.GetDrawPt();
		double newV = ptUV.m_v + double(zDelta)/5000.0;
		//newV = U::Clamp(newV, minV, maxV);
		ptUV.m_v = newV;
		m_bs.SetDrawPt(ptUV);	

		std::stringstream strm;
		strm << "V updated. Eval point now (" << ptUV.m_u << "," << ptUV.m_v << ")." << std::endl;
		::OutputDebugString((LPCSTR)strm.str().c_str());
		m_bs.Draw();
	}
	else
	{
		Scale(1 + double(zDelta)/300);
		m_kvmgr.setDimensions(m_WindowWidth, m_WindowHeight);
		m_kvmgr.show();
	}
	Invalidate();
	return true;
}

void CMFCKit2004View::OnTimer(UINT nIDEvent) {
	return;
}


void CMFCKit2004View::OnSize(UINT nType, int cx, int cy) 
{
	CView::OnSize(nType, cx, cy);
	
    if ( 0 >= cx || 0 >= cy ) {
        return;
	}

    // save the width and height of the current window
    m_WindowWidth = cx;
    m_WindowHeight = cy;
	viewPort[0] = viewPort[1] = 0;
	viewPort[2] = m_WindowWidth;
	viewPort[3] = m_WindowHeight;
    // compute the aspect ratio
    // this will keep all dimension scales equal
    m_AspectRatio = (GLdouble)m_WindowWidth/(GLdouble)m_WindowHeight;

    // Now, set up the viewing area-select the full client area
    ::glViewport(0, 0, m_WindowWidth, m_WindowHeight);

    //if ( GL_NO_ERROR != ::glGetError() ) {
	//	::AfxMessageBox("Error while trying to set viewport.");
    //}

    // select the projection matrix as the recipient of
    // matrix operations (there's three to choose from)
    ::glMatrixMode(GL_PROJECTION);
    // initialize the projection matrix to a pristine state
    ::glLoadIdentity();

    // select the viewing volume. You do it after you
    // get the aspect ratio and set the viewport
    //SetupViewingFrustum( );
	//SetupViewingOrtho();
	SetupViewingOrthoConstAspect();

    // now select the modelview matrix and clear it
    // this is the mode we do most of our calculations in
    // so we leave it as the default mode.
    ::glMatrixMode(GL_MODELVIEW);
//    ::glLoadIdentity();
	
}

// find a control point
CCagdPoint *CMFCKit2004View::findCtlPoint(int x, int y) {
	UINT id;
	for (cagdPick(x,y);id = cagdPickNext();) {
		if (cagdGetSegmentType(id) == CAGD_SEGMENT_CTLPLYGN) {
			draggedPolyline = id;
			draggedPoint = cagdGetNearestVertex(id,x,y)-1;
			CCagdPoint *ret;
			ret = cagdGetVertex(id,draggedPoint);
			int retx,rety;
			cagdToWindow(ret,&retx,&rety);
			if (abs(retx-x) > 5 || abs(rety-y) > 5)
				continue;
			return ret;
		}
	}
	draggedPoint = -1;
	draggedCtlPt = NULL;
	return NULL;
}

UINT CMFCKit2004View::findCircle(int x, int y) {
	UINT id;
	for (cagdPick(x,y);id = cagdPickNext();) {
		if (list[id].type == CAGD_SEGMENT_WCIRCLE) {
			draggedCircle = id;
			draggedCtlPt = list[id].ctlPt;
			return id;
		}
	}
	return -1;
}


void CMFCKit2004View::RotateXY(double valX, double valY) {
	glLoadIdentity();
	glRotated(valX,1.0, 0, 0);
	glRotated(valY,0, 1.0, 0);
	glMultMatrixd (modelView);
	glGetDoublev(GL_MODELVIEW_MATRIX, modelView);
}

void CMFCKit2004View::RotateZ (double val) {
	glLoadIdentity();
	glRotated(val,0, 0, 1.0);
	glMultMatrixd (modelView);
	glGetDoublev(GL_MODELVIEW_MATRIX, modelView);
}

void CMFCKit2004View::translateXY(double valX, double valY) {
	glLoadIdentity();
	glTranslated(valX, valY, 0);
	glMultMatrixd (modelView);
	glGetDoublev(GL_MODELVIEW_MATRIX, modelView);
}

void CMFCKit2004View::Scale (double val) {
	glLoadIdentity();
	glScaled(val, val, val);
	glMultMatrixd (modelView);
	glGetDoublev(GL_MODELVIEW_MATRIX, modelView);
}

void CMFCKit2004View::OnFuzzinessMore() {
	fuzziness *= 2;
	moreFuzziness();
}

void CMFCKit2004View::OnFuzzinessLess() {
	fuzziness /= 2;
	lessFuzziness();
}
void CMFCKit2004View::OnOptionsReset() {
	m_kvmgr.dismiss();
	m_modifiedAxis = AxisUndef;
	m_bs.invalidate();
	cagdFreeAllSegments();
	cagdReset();
	Invalidate();
}

void CMFCKit2004View::OnRotationMore() {
	RSense*= 1.05;
}

void CMFCKit2004View::OnRotationLess() {
	RSense*= 0.95;
}

void CMFCKit2004View::OnTranslationMore() {
	TSense*= 1.05;
}

void CMFCKit2004View::OnTranslationLess() {
	TSense*= 0.95;
}

void CMFCKit2004View::OnScalingMore() {
	SSense*= 1.05;
}

void CMFCKit2004View::OnScalingLess() {
	SSense*= 0.95;
}

void CMFCKit2004View::TestCagdMath()
{
	// test code
	CCagdPoint a(1.0, 2.0, 3.0);
	CCagdPoint b(3.5, 7.5, 4.0);
	CCagdPoint c = a + b;
	CCagdPoint d = b - CCagdPoint(0.5, 0.1, 0.4);
	CCagdPoint e = a * 5.0;
	CCagdPoint f = 3.0 * b;
	CCagdPoint g = a / 5.0;
	double h = dot(a, b);
	CCagdPoint k = cross(CCagdPoint(1,0,0), CCagdPoint(0,0,1));
	CCagdPoint l = length(b);
	CCagdPoint n = normalize(b);
}





void CMFCKit2004View::DrawAxes()
{
	CCagdPoint xAxis[2];
	CCagdPoint yAxis[2];
	CCagdPoint zAxis[2];

	double axisLength = 1000.0;
	xAxis[0] = yAxis[0] = zAxis[0] = CCagdPoint(0,0,0);
	xAxis[1] = CCagdPoint(1,0,0) * axisLength;
	yAxis[1] = CCagdPoint(0,1,0) * axisLength;
	zAxis[1] = CCagdPoint(0,0,1) * axisLength;
	cagdSetSegmentColor(cagdAddPolyline(xAxis, 2,  CAGD_SEGMENT_POLYLINE), 128,0,0);
	cagdSetSegmentColor(cagdAddPolyline(yAxis, 2,  CAGD_SEGMENT_POLYLINE), 0,128,0);
	cagdSetSegmentColor(cagdAddPolyline(zAxis, 2,  CAGD_SEGMENT_POLYLINE), 30,100,128);


}

void CMFCKit2004View::OnContextbgNewbeziercurve()
{
	m_state = StateAddBezierPts;
	m_currCurveIdx = m_mgr.NewBezierCurve();
	// change cursor
}

void CMFCKit2004View::OnContextbgNewbsplinecurve()
{
	m_state = StateAddBSplinePts;
	m_currCurveIdx = m_mgr.NewBsplineCurve(m_bsplineDegree);
}

void CMFCKit2004View::OnContextbgClearall()
{
	m_kvmgr.dismiss();
	cagdFreeAllSegments();
	Invalidate();
}

void CMFCKit2004View::OnContextpolygonInsertpoint()
{
	m_currCurveIdx = m_mgr.getCurveIndexByPointOnPolygon(m_lastRbuttonUp);
	m_mgr.AddCtrlPt(m_lastRbuttonUp, 1, m_currCurveIdx);
	if (m_modifiedCurveIdx == m_currCurveIdx)
	{
		vector<double> kv = m_mgr.GetKnotVector(m_currCurveIdx);
		m_kvmgr.setVector(kv);
		m_kvmgr.show();
	}
	Invalidate();
}

void CMFCKit2004View::OnContextpolygonAppendpoint()
{
	m_currCurveIdx = m_mgr.getCurveIndexByPointOnPolygon(m_lastRbuttonUp);
	if (-1 != m_currCurveIdx)
	{
		m_state = StateAddBezierPts;
	}
	Invalidate();
}

void CMFCKit2004View::OnContextpolygonShowHideControlPolygon()
{
	m_currCurveIdx = m_mgr.getCurveIndexByPointOnPolygon(m_lastRbuttonUp);
	m_mgr.ToggleShowPolygon(m_currCurveIdx);
	Invalidate();
}


void CMFCKit2004View::OnContextptAdjustweight()
{
	m_lastWeightControlStatus = m_mgr.ToggleWeightConrol(m_lastRbuttonUp);
	Invalidate();
}

void CMFCKit2004View::OnUpdateContextptAdjustweight(CCmdUI *pCmdUI)
{
	//pCmdUI->SetCheck(m_lastWeightControlStatus);
}


void CMFCKit2004View::OnContextpolygonRaisedegree()
{
	m_currCurveIdx = m_mgr.getCurveIndexByPointOnPolygon(m_lastRbuttonUp);
	if (-1 != m_currCurveIdx)
	{
		m_mgr.RaiseDegree(m_currCurveIdx);
	}
	Invalidate();
}


void CMFCKit2004View::OnContextpolygonSubdivide()
{
	m_currCurveIdx = m_mgr.getCurveIndexByPointOnPolygon(m_lastRbuttonUp);
	if (-1 != m_currCurveIdx)
	{
		m_mgr.Subdivide(m_currCurveIdx);
	}
	Invalidate();
}

void CMFCKit2004View::OnContextptRemovepoint()
{
	m_mgr.RemoveCtrlPt(m_lastRbuttonUp);
	m_currCurveIdx = m_mgr.getCurveIndexByPointOnPolygon(m_lastRbuttonUp);
	if (m_modifiedCurveIdx == m_currCurveIdx)
	{
		vector<double> kv = m_mgr.GetKnotVector(m_currCurveIdx);
		m_kvmgr.setVector(kv);
		m_kvmgr.show();
	}
	Invalidate();
}


void CMFCKit2004View::OnOptionsShowgrid()
{
	m_mgr.showGrid();
	Invalidate();
}


void CMFCKit2004View::OnFileSaveGeometry()
{
	char strFilter[] = { "ITD Files (*.itd)|*.itd|DAT Diles (*.dat)|*.dat||" };
	CFileDialog fileDialog = CFileDialog(FALSE, "itd", NULL, OFN_OVERWRITEPROMPT, strFilter, NULL);
	INT_PTR nResult = fileDialog.DoModal();
	if(IDCANCEL == nResult) 
	{
		Invalidate();
		return;
	}
	CString pathName = fileDialog.GetPathName();
	CString fileName = fileDialog.GetFileTitle();
	string fileExt	 = std::string(fileDialog.GetFileExt());
	string filename = std::string(pathName);
	string res;
	if ("itd" == fileExt)
	{
		res = m_mgr.toIrit();
	}
	else if ("dat" == fileExt)
	{
		res = m_mgr.toDat();
	}
	else
	{
		Invalidate();
		return;
	}
	std::ofstream out(filename.c_str());
	out << res;
	out.close();
	Invalidate();	
}


void CMFCKit2004View::OnContextbsplinepolyModifyknotvector()
{
	m_currCurveIdx = m_mgr.getCurveIndexByPointOnPolygon(m_lastRbuttonUp);
	if (-1 != m_currCurveIdx)
	{
		//do stuff
		m_modifiedCurveIdx = m_currCurveIdx;
		vector<double> kv = m_mgr.GetKnotVector(m_currCurveIdx);
		m_kvmgr.setDimensions(m_WindowWidth, m_WindowHeight);
		m_kvmgr.setVector(kv);
		m_kvmgr.show();
	}
	Invalidate();
}


void CMFCKit2004View::OnConnecttowithcontinuityG0()
{
	m_currCurveIdx = m_mgr.getCurveIndexByPointOnPolygon(m_lastRbuttonUp);
	m_state = StateConnectingCurvesG0;
}


void CMFCKit2004View::OnConnecttowithcontinuityG1()
{
	m_currCurveIdx = m_mgr.getCurveIndexByPointOnPolygon(m_lastRbuttonUp);
	m_state = StateConnectingCurvesG1;
}


void CMFCKit2004View::OnConnecttowithcontinuityC1()
{
	m_currCurveIdx = m_mgr.getCurveIndexByPointOnPolygon(m_lastRbuttonUp);
	m_state = StateConnectingCurvesC1;
}


void CMFCKit2004View::OnKnotguiRemoveknot()
{
	vector<double> kv = m_kvmgr.getVector();
	int idx = m_kvmgr.idxAtPoint(m_lastRbuttonUp);
	vector<double> newKv;
	if (idx != -1)
	{
		for (int i = 0; i < kv.size(); i++)
		{
			if (i != idx)
				newKv.push_back(kv[i]);
		}
		m_mgr.SetKnotVector(m_modifiedCurveIdx, newKv);
		m_kvmgr.setVector(newKv);
		m_kvmgr.show();
	}
}


void CMFCKit2004View::OnKnotguiInsertknot()
{
	m_kvmgr.addKnotAtPoint(m_lastRbuttonUp);
	vector<double> kv = m_kvmgr.getVector(); // It sorts it inside
	m_mgr.SetKnotVector(m_modifiedCurveIdx, kv);
	m_kvmgr.show();
}


void CMFCKit2004View::OnContextbsplinepolyAppendpoint()
{
	m_currCurveIdx = m_mgr.getCurveIndexByPointOnPolygon(m_lastRbuttonUp);
	if (-1 != m_currCurveIdx)
	{
		m_state = StateAddBSplinePts;
	}
	Invalidate();
}


void CMFCKit2004View::OnKnotguiInsertknotBoehm()
{
	double knotval = m_kvmgr.guiXtoknot(m_lastRbuttonUp.x);
	if(m_mgr.InsertKnot(m_modifiedCurveIdx, knotval))
	{
		m_kvmgr.addKnotAtPoint(m_lastRbuttonUp);
	}
	//vector<double> kv = m_kvmgr.getVector(); // It sorts it inside
	//m_mgr.SetKnotVector(m_modifiedCurveIdx, kv);
}


void CMFCKit2004View::OnModifyknotvectorU()
{
	m_kvmgr.dismiss();
	m_modifiedAxis = AxisU;
	vector<double> kv = m_bs.KnotVectorU();
	m_kvmgr.setDimensions(m_WindowWidth, m_WindowHeight);
	m_kvmgr.setVector(kv);
	m_kvmgr.show();
	Invalidate();
}


void CMFCKit2004View::OnModifyknotvectorV()
{
	m_kvmgr.dismiss();
	m_modifiedAxis = AxisV;
	vector<double> kv = m_bs.KnotVectorU();
	m_kvmgr.setDimensions(m_WindowWidth, m_WindowHeight);
	m_kvmgr.setVector(kv);
	m_kvmgr.show();
	Invalidate();
}


void CMFCKit2004View::OnContextbgNewsurface()
{
	CNewSurfaceDlg newSurfDlg;
	newSurfDlg.SaveValues();
	if(IDOK == newSurfDlg.DoModal())
	{
		ParsedSurface psurf;
		// set the order
		psurf.m_order.m_u = newSurfDlg.m_orderU;
		psurf.m_order.m_v = newSurfDlg.m_orderV;
		
		// add points
		vector<CCagdPoint> newArr;
		for(int v=0; v<newSurfDlg.m_ctrlPtCountV; ++v)
		{
			psurf.m_points.push_back(newArr);
			for(int u=0; u<newSurfDlg.m_ctrlPtCountU; ++u)			
			{
				double x = -5.0 + 10.0*double(u)*(1.0/double(newSurfDlg.m_ctrlPtCountU-1));
				double y = -5.0 + 10.0*double(v)*(1.0/double(newSurfDlg.m_ctrlPtCountV-1));
				CCagdPoint newPt(x,y,0.0);
				psurf.m_points[v].push_back(newPt);
			}
			
		}

		// create the knot vector (parse)
		psurf.m_knots.m_u.clear();
		psurf.m_knots.m_v.clear();
		
		OnOptionsReset();
		m_bs = BsplineSurface(psurf);		

		m_bs.Draw();
		Invalidate();
	}
	else
	{
		newSurfDlg.RestoreSavedValues();
	}

}


void CMFCKit2004View::OnSurfacesGlobals()
{
	m_globalsDlg.m_orderU = m_bs.GetOrder().m_u;
	m_globalsDlg.m_orderV = m_bs.GetOrder().m_v;

	m_globalsDlg.m_isoCurvesU = m_bs.numberOfIsocurves().m_u;
	m_globalsDlg.m_isoCurvesV = m_bs.numberOfIsocurves().m_v;

	m_globalsDlg.m_samplesPerCurveU = m_bs.samplesPerCurve().m_u;
	m_globalsDlg.m_samplesPerCurveV = m_bs.samplesPerCurve().m_v;

	m_globalsDlg.m_knotsU = m_bs.KnotVectorU();
	m_globalsDlg.m_knotsV = m_bs.KnotVectorV();

	if(IDOK == m_globalsDlg.DoModal())
	{
		m_bs.SetOrder(Order(m_globalsDlg.m_orderU, m_globalsDlg.m_orderV));
		m_bs.samplesPerCurve(SamplingFreq(m_globalsDlg.m_samplesPerCurveU, m_globalsDlg.m_samplesPerCurveV));
		m_bs.numberOfIsocurves(IsocurvesNumber(m_globalsDlg.m_isoCurvesU, m_globalsDlg.m_isoCurvesV));
		m_bs.SetKnotVectorU(m_globalsDlg.m_knotsU);
		m_bs.SetKnotVectorV(m_globalsDlg.m_knotsV);

		m_bs.ClearSegments();
		cagdFreeAllSegments();

		m_bs.Draw();
		Invalidate();
	}
	
}

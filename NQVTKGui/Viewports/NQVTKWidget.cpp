#include "GLBlaat/GL.h"

#include "NQVTKWidget.h"
#include "NQVTKWidget.moc"

#include "NQVTK/Math/Vector3.h"

#include "NQVTK/Rendering/Renderer.h"
#include "NQVTK/Rendering/NestedRenderer.h"
#include "NQVTK/Rendering/Camera.h"
#include "NQVTK/Interactors/Interactor.h"

#include <QMouseEvent>
#include <QGLFormat>

#include <algorithm>
#include <cassert>

// ----------------------------------------------------------------------------
NQVTKWidget::NQVTKWidget(QWidget *parent, const QGLWidget *shareWidget)
: QGLWidget(QGLFormat(QGL::AlphaChannel), parent, shareWidget), 
renderer(0), interactor(0)
{
	setMouseTracking(true);
	crosshairOn = false;
}

// ----------------------------------------------------------------------------
NQVTKWidget::~NQVTKWidget()
{
	delete interactor;
	if (renderer) 
	{
		makeCurrent();
		delete renderer;
	}
}

// ----------------------------------------------------------------------------
void NQVTKWidget::SetRenderer(NQVTK::Renderer *renderer)
{
	assert(!this->renderer);
	this->renderer = renderer;
}

// ----------------------------------------------------------------------------
NQVTK::Renderer *NQVTKWidget::GetRenderer(bool getInner)
{
	NQVTK::Renderer *ren = renderer;
	if (getInner)
	{
		// Loop to remove all layers
		NQVTK::NestedRenderer *nren;
		do
		{
			nren = dynamic_cast<NQVTK::NestedRenderer*>(ren);
			if (nren) ren = nren->GetBaseRenderer();
		}
		while (nren);
	}

	return ren;
}

// ----------------------------------------------------------------------------
void NQVTKWidget::SetInteractor(NQVTK::Interactor *interactor)
{
	assert(!this->interactor);
	this->interactor = interactor;
	interactor->ResizeEvent(this->width(), this->height());
}

// ----------------------------------------------------------------------------
void NQVTKWidget::StartContinuousUpdate()
{
	// Render-on-idle timer
	startTimer(0);
	// FPS display timer
	fpsTimerId = startTimer(1000);
	frames = 0;
}

// ----------------------------------------------------------------------------
bool NQVTKWidget::Initialize()
{
	glInit();
	return isValid();
}

// ----------------------------------------------------------------------------
void NQVTKWidget::toggleCrosshair(bool on)
{
	crosshairOn = on; 
	updateGL();
}

// ----------------------------------------------------------------------------
void NQVTKWidget::setCrosshairPos(double x, double y)
{
	crosshairX = x; crosshairY = y;
	updateGL();
}

// ----------------------------------------------------------------------------
void NQVTKWidget::syncCamera(NQVTK::Camera *cam)
{
	if (!renderer) return;
	renderer->GetCamera()->position = cam->position;
	renderer->GetCamera()->focus = cam->focus;
	renderer->GetCamera()->up = cam->up;
	updateGL();
}

// ----------------------------------------------------------------------------
void NQVTKWidget::initializeGL()
{
	// TODO: add support for multiple contexts
	// GLBlaat should support this (on the GLEW side), as well as NQVTK. This 
	// involves splitting all GL-side resources and managing them separately, 
	// with multiple instances (one per context) per renderable, volume, ...
	if (glewInit() != GLEW_OK)
	{
		qDebug("Failed to initialize GLEW!");
		return;
	}

	qDebug("Initializing renderer...");
	if (renderer)
	{
		renderer->TryInitialize();
		if (!renderer->IsInitialized())
		{
			qDebug("Failed to initialize renderer...");
		}
	}
	else
	{
		qDebug("Renderer not set!");
	}
	qDebug("Init done!");
}

// ----------------------------------------------------------------------------
void NQVTKWidget::resizeGL(int w, int h)
{
	if (!renderer) return;
	renderer->Resize(w, h);
	if (interactor) interactor->ResizeEvent(w, h);
	if (renderer->IsInitialized())
	{
		emit cameraUpdated(GetRenderer()->GetCamera());
	}
}

// ----------------------------------------------------------------------------
void NQVTKWidget::paintGL()
{
	// Draw if we can and are visible, otherwise just clear the screen
	// NOTE: Call both Initialize and Resize before calling Draw on a Renderer
	// The second of these is only called when the widget is shown
	if (renderer != 0 && renderer->IsInitialized() && isVisible())
	{
		renderer->Draw();

		// Draw crosshairs
		if (crosshairOn)
		{
			double x = (crosshairX / renderer->GetCamera()->aspect);
			double y = -crosshairY;

			glMatrixMode(GL_PROJECTION);
			glLoadIdentity();
			glMatrixMode(GL_MODELVIEW);
			glLoadIdentity();

			glPushAttrib(GL_ALL_ATTRIB_BITS);

			glDisable(GL_DEPTH_TEST);
			glDisable(GL_LIGHTING);

			glBlendFunc(GL_SRC_ALPHA, GL_ONE);
			glEnable(GL_BLEND);

			glBegin(GL_LINES);
			glColor4d(1.0, 0.0, 0.0, 0.7);
			glVertex2d(x, -1.0);
			glVertex2d(x, 1.0);
			glVertex2d(-1.0, y);
			glVertex2d(1.0, y);
			glEnd();

			glPopAttrib();
		}
	}
	else
	{
		glPushAttrib(GL_ALL_ATTRIB_BITS);
		// Dark red shows something went wrong
		glClearColor(0.5, 0.0, 0.0, 0.0);
		glClear(GL_COLOR_BUFFER_BIT);
		glPopAttrib();
	}

	// Increase fps count
	++frames;
}

// ----------------------------------------------------------------------------
NQVTK::MouseEvent NQVTKWidget::translateMouseEvent(QMouseEvent *event)
{
	NQVTK::MouseEvent ev;
	// Translate button
	int button = 0;
	switch (event->button())
	{
	case Qt::LeftButton:
		button = NQVTK::MouseEvent::LeftButton;
		break;
	case Qt::RightButton:
		button = NQVTK::MouseEvent::RightButton;
		break;
	case Qt::MidButton:
		button = NQVTK::MouseEvent::MiddleButton;
		break;
	}
	// Translate buttons state
	int buttons = 0;
	if (event->buttons() & Qt::LeftButton) 
		buttons |= NQVTK::MouseEvent::LeftButton;
	if (event->buttons() & Qt::RightButton) 
		buttons |= NQVTK::MouseEvent::RightButton;
	if (event->buttons() & Qt::MidButton) 
		buttons |= NQVTK::MouseEvent::MiddleButton;
	// Build event
	ev.button = button;
	ev.buttons = buttons;
	ev.x = event->x();
	ev.y = event->y();
	ev.control = event->modifiers() & Qt::ControlModifier;
	ev.shift = event->modifiers() & Qt::ShiftModifier;
	ev.alt = event->modifiers() & Qt::AltModifier;
	return ev;
}

// ----------------------------------------------------------------------------
void NQVTKWidget::timerEvent(QTimerEvent *event)
{
	if (event->timerId() == fpsTimerId)
	{
		// Update FPS display
		emit fpsChanged(frames);
		frames = 0;
	}
	else
	{
		// Update on idle
		updateGL();
	}
}

// ----------------------------------------------------------------------------
void NQVTKWidget::mouseMoveEvent(QMouseEvent *event)
{
	// Ignore events if initialization failed
	if (renderer == 0 || !renderer->IsInitialized())
	{
		event->ignore();
		return;
	}

	// Pass the event to the interactor
	if (interactor)
	{
		if (interactor->MouseMoveEvent(translateMouseEvent(event)))
		{
			// TODO: this should probably not be emitted for each mouse event
			emit cameraUpdated(GetRenderer()->GetCamera());
			event->accept();
			updateGL();
		}
		else
		{
			event->ignore();
		}
	}

	// Compute projection-neutral coordinates
	double x = (2.0 * static_cast<double>(event->x()) / this->width() - 1.0) * 
		GetRenderer()->GetCamera()->aspect;
	double y = 2.0 * static_cast<double>(event->y()) / this->height() - 1.0;
	emit cursorPosChanged(x, y);
}

// ----------------------------------------------------------------------------
void NQVTKWidget::mousePressEvent(QMouseEvent *event)
{
	// Ignore events if initialization failed
	if (renderer == 0 || !renderer->IsInitialized())
	{
		event->ignore();
		return;
	}

	// Pass the event to the interactor
	if (interactor)
	{
		if (interactor->MousePressEvent(translateMouseEvent(event)))
		{
			event->accept();
			updateGL();
		}
		else
		{
			event->ignore();
		}
	}
}

// ----------------------------------------------------------------------------
void NQVTKWidget::mouseReleaseEvent(QMouseEvent *event)
{
	// Ignore events if initialization failed
	if (renderer == 0 || !renderer->IsInitialized())
	{
		event->ignore();
		return;
	}

	// Pass the event to the interactor
	if (interactor)
	{
		if (interactor->MouseReleaseEvent(translateMouseEvent(event)))
		{
			event->accept();
			updateGL();
		}
		else
		{
			event->ignore();
		}
	}
}

#pragma once

#include <QGLWidget>

#include "NQVTK/Interactors/MouseEvent.h"

namespace NQVTK
{
	class Renderer;
	class Camera;
	class Interactor;
}

class NQVTKWidget : public QGLWidget 
{
	Q_OBJECT

public:
	// TODO: add full QGLWidget constructors
	NQVTKWidget(QWidget *parent = 0);
	virtual ~NQVTKWidget();

	void SetRenderer(NQVTK::Renderer *renderer);
	NQVTK::Renderer *GetRenderer(bool getInner = false);

	void SetInteractor(NQVTK::Interactor *interactor);
	NQVTK::Interactor *GetInteractor() { return interactor; }

	void StartContinuousUpdate();

	bool Initialize();

	// Use this if the sharewidget is deleted or when using multiple contexts
	static void ResetShareWidget(NQVTKWidget *shareWidget = 0);

public slots:
	void toggleCrosshair(bool on);
	void setCrosshairPos(double x, double y);
	void syncCamera(NQVTK::Camera *cam);

signals:
	void fpsChanged(int fps);
	void cursorPosChanged(double x, double y);
	void cameraUpdated(NQVTK::Camera *cam);

protected:
	void initializeGL();
	void resizeGL(int w, int h);
	void paintGL();

	NQVTK::MouseEvent translateMouseEvent(QMouseEvent *event);

private:
	void timerEvent(QTimerEvent *event);
	void mouseMoveEvent(QMouseEvent *event);
	void mousePressEvent(QMouseEvent *event);
	void mouseReleaseEvent(QMouseEvent *event);

	// Singleton for sharing gl resources between all NQVTKWidgets
	static NQVTKWidget *shareWidget;

	// The renderer
	NQVTK::Renderer *renderer;

	// The interactor
	NQVTK::Interactor *interactor;

	// FPS measurement
	int frames;
	int fpsTimerId;

	// Crosshair
	bool crosshairOn;
	double crosshairX;
	double crosshairY;
};

#include "lightingwidget.h"
#include "ui_lightingwidget.h"

#include "glview.h"

#include <QAction>


// Slider lambda
auto sld = []( QSlider * slider, int min, int max, int val ) {
	slider->setSizePolicy( QSizePolicy::MinimumExpanding, QSizePolicy::Maximum );
	slider->setRange( min, max );
	slider->setSingleStep( max / 8 );
	slider->setTickInterval( max / 2 );
	slider->setTickPosition( QSlider::TicksBelow );
	slider->setValue( val );
};

LightingWidget::LightingWidget( GLView * ogl, QWidget * parent ) : QWidget(parent),
	ui(new Ui::LightingWidget)
{
	ui->setupUi(this);

	setDefaults();

	ui->sldDeclination->setDisabled( ui->btnFrontal->isChecked() );

	// Disable declination slider when Frontal (planar angle is still used to rotate the environment map instead)
	connect( ui->btnFrontal, &QToolButton::toggled, ui->sldDeclination, &QSlider::setDisabled );

	// Disable Frontal checkbox (and sliders) when no lighting
	connect( ui->btnLighting, &QToolButton::toggled, ui->btnFrontal, &QToolButton::setEnabled );
	connect( ui->btnLighting, &QToolButton::toggled, [&]( bool checked ) {
		if ( !ui->btnFrontal->isChecked() ) {
			// Don't enable the slider if Frontal is checked
			ui->sldDeclination->setEnabled( checked );
		}
		ui->sldPlanarAngle->setEnabled( checked );
	} );

	// Inform ogl of changes
	connect( ui->sldDirectional, &QSlider::valueChanged, ogl, &GLView::setBrightness );
	connect( ui->sldLightLevel, &QSlider::valueChanged, ogl, &GLView::setLightLevel );
	connect( ui->sldLightColor, &QSlider::valueChanged, ogl, &GLView::setLightColor );
	connect( ui->sldToneMapping, &QSlider::valueChanged, ogl, &GLView::setToneMapping );
	connect( ui->sldAmbient, &QSlider::valueChanged, ogl, &GLView::setAmbient );
	connect( ui->sldDeclination, &QSlider::valueChanged, ogl, &GLView::setDeclination );
	connect( ui->sldPlanarAngle, &QSlider::valueChanged, ogl, &GLView::setPlanarAngle );
	connect( ui->btnFrontal, &QToolButton::toggled, ogl, &GLView::setFrontalLight );
	connect( ui->btnLoadCubeMap, &QPushButton::clicked, ogl, &GLView::selectPBRCubeMap );
}

LightingWidget::~LightingWidget()
{
}

void LightingWidget::setDefaults()
{
	sld( ui->sldDirectional, DirMin, DirMax, DirDefault );
	sld( ui->sldAmbient, AmbientMin, AmbientMax, AmbientDefault );
	sld( ui->sldDeclination, DeclinationMin, DeclinationMax, DeclinationDefault );
	sld( ui->sldPlanarAngle, PlanarAngleMin, PlanarAngleMax, PlanarAngleDefault );
	sld( ui->sldLightLevel, LightLevelMin, LightLevelMax, LightLevelDefault );
	sld( ui->sldLightColor, LightColorMin, LightColorMax, LightColorDefault );
	ui->sldLightColor->setSingleStep( LightColorMax / 16 );
	ui->sldLightColor->setTickInterval( LightColorMax / 8 );
	sld( ui->sldToneMapping, ToneMappingMin, ToneMappingMax, ToneMappingDefault );
}

void LightingWidget::setActions( QVector<QAction *> atns )
{
	ui->btnLighting->setDefaultAction( atns.value(0) );
	ui->btnTextures->setDefaultAction( atns.value(1) );
	ui->btnVertexColors->setDefaultAction( atns.value(2) );
	ui->btnSpecular->setDefaultAction( atns.value(3) );
	ui->btnCubemap->setDefaultAction( atns.value(4) );
	ui->btnGlow->setDefaultAction( atns.value(5) );
	ui->btnLightingOnly->setDefaultAction( atns.value(6) );
	ui->btnSilhouette->setDefaultAction( atns.value(7) );

	connect( ui->btnLighting, &QToolButton::toggled, atns.value(3), &QAction::setEnabled );
}

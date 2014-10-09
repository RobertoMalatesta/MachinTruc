#include <QDebug>
#include "projectprofiledialog.h"


typedef struct AudioPreset_ {
	int samplerate;
	int layout;
} AudioPreset;

typedef struct VideoPreset_ {
	int width;
	int height;
	QString ratio;
	int framerate; /*fpsPresets index*/
	int scanning;
} VideoPreset;

static const int nfpspresets = 8;
static const double fpsPresets[ nfpspresets ] = {
	24000. / 1001.,
	24,
	25,
	30000. / 1001.,
	30,
	50,
	60000. / 1001.,
	60
};

static const int nvpresets = 20;
static const VideoPreset vPresets[ nvpresets ] = {
	{ 1920, 1080, "16:9", 0, 0 },
	{ 1920, 1080, "16:9", 1, 0 },
	{ 1920, 1080, "16:9", 2, 0 },
	{ 1920, 1080, "16:9", 3, 0 },
	{ 1920, 1080, "16:9", 4, 0 },
	{ 1920, 1080, "16:9", 5, 0 },
	{ 1920, 1080, "16:9", 6, 0 },
	{ 1920, 1080, "16:9", 7, 0 },
	
	{ 1280, 720, "16:9", 0, 0 },
	{ 1280, 720, "16:9", 1, 0 },
	{ 1280, 720, "16:9", 2, 0 },
	{ 1280, 720, "16:9", 3, 0 },
	{ 1280, 720, "16:9", 4, 0 },
	{ 1280, 720, "16:9", 5, 0 },
	{ 1280, 720, "16:9", 6, 0 },
	{ 1280, 720, "16:9", 7, 0 },
	
	{ 720, 576, "16:9", 2, 0 },
	{ 720, 576, "4:3", 2, 0 },
	{ 720, 480, "16:9", 3, 0 },
	{ 720, 480, "4:3", 3, 0 }
};



ProjectProfileDialog::ProjectProfileDialog( QWidget *parent, Profile &p, int warn ) : QDialog( parent )
{
	setupUi( this );
	
	QString info = "Select a profile for your project.";
	if ( warn == WARNCHANGE )
		info += "\nIt's not recommended to change the video settings when you have already put some clips in the timeline.";
	else if ( warn == WARNNEWCLIP )
		info += "\nThe default settings come from the clip you have dropped onto the timeline. If you don't know what to choose, just click ok and play the timeline; if you are not satisfied, remove the clip and drop a new one.";
	
	infoLab->setText( info );

	audioPresetLab->setEnabled( false );
	audioPresetCombo->setEnabled( false );
	layoutLab->setEnabled( false );
	layoutCombo->addItem( "Stereo" );
	layoutCombo->setEnabled( false );
	samplerateLab->setEnabled( false );
	samplerateSpinBox->setRange( 1000, 192000 );
	samplerateSpinBox->setSuffix( "Hz" );
	samplerateSpinBox->setValue( 48000 );
	samplerateSpinBox->setEnabled( false );

	videoPresetCombo->addItem( tr("Custom") );
	for ( int i = 0; i < nvpresets; ++i ) {
		videoPresetCombo->addItem( QString("%1x%2 - %3 - %4fps")
			.arg(vPresets[i].width)
			.arg(vPresets[i].height)
			.arg(vPresets[i].ratio)
			.arg( QString::number( fpsPresets[ vPresets[i].framerate ], 'f', 2 )  ) );
	}

	widthSpinBox->setRange( 64, 1920 );
	widthSpinBox->setSingleStep( 2 );
	heightSpinBox->setRange( 64, 1080 );
	heightSpinBox->setSingleStep( 2 );
	
	ratioCombo->addItem( "16:9" );
	ratioCombo->addItem( "4:3" );
	ratioCombo->addItem( "Size" );
	
	for ( int i = 0; i < nfpspresets; ++i )
		framerateCombo->addItem( QString::number( fpsPresets[i], 'f', 2 ) );
	
	scanningCombo->addItem( "Progressive" );
	scanningCombo->addItem( "Top Field First" );
	scanningCombo->addItem( "Bottom Field First" );
	
	widthSpinBox->setValue( p.getVideoWidth() );
	heightSpinBox->setValue( p.getVideoHeight() );

	double ar = p.getVideoWidth() * p.getVideoSAR() / p.getVideoHeight();
	if ( qAbs( ar - 16./9. ) < 1e-3 )
		ratioCombo->setCurrentIndex( 0 );
	else if ( qAbs( ar - 4./3. ) < 1e-3 )
		ratioCombo->setCurrentIndex( 1 );
	else
		ratioCombo->setCurrentIndex( 2 );
	
	for ( int i = 0; i < nfpspresets; ++i ) {
		if ( qAbs( p.getVideoFrameRate() - fpsPresets[i] ) < 1e-3 ) {
			framerateCombo->setCurrentIndex( i );
			break;
		}
	}
	
	scanningCombo->setEnabled( false );
	
	connect( videoPresetCombo, SIGNAL(currentIndexChanged(int)), this, SLOT(presetChanged(int)) );
}



void ProjectProfileDialog::presetChanged( int index )
{
	if ( index == 0 )
		return;
	
	widthSpinBox->setValue( vPresets[index - 1].width );
	heightSpinBox->setValue( vPresets[index - 1].height );
	
	if ( vPresets[index - 1].ratio == "16:9" )
		ratioCombo->setCurrentIndex( 0 );
	else if ( vPresets[index - 1].ratio == "4:3" )
		ratioCombo->setCurrentIndex( 1 );
	
	framerateCombo->setCurrentIndex( vPresets[index - 1].framerate );
}



Profile ProjectProfileDialog::getCurrentProfile()
{
	Profile p;
	p.setVideoWidth( widthSpinBox->value() );
	p.setVideoHeight( heightSpinBox->value() );
	
	double r = widthSpinBox->value() / heightSpinBox->value();
	if ( ratioCombo->currentText() == "16:9" )
		r = 16. / 9.;
	else if ( ratioCombo->currentText() == "4:3" )
		r = 4. / 3.;
	p.setVideoSAR( (double)p.getVideoHeight() * r / (double)p.getVideoWidth() );
	
	p.setVideoFrameRate( fpsPresets[ framerateCombo->currentIndex() ] );
	p.setVideoFrameDuration( MICROSECOND / p.getVideoFrameRate() );
	
	return p;
}

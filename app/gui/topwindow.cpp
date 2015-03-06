#include <QMessageBox>
#include <QFileDialog>
#include <QShortcut>

#include "gui/topwindow.h"
#include "renderingdialog.h"
#include "projectprofiledialog.h"

#define VIDEOCLEARDELAY 200



TopWindow::TopWindow()
	: openSourcesCounter( 0 ),
	projectLoader( NULL ),
	thumbnailer( new Thumbnailer() )
{
	setupUi( this );

	seekSlider = new SeekSlider( baseVideoWidget );
	seekSlider->setObjectName(QString::fromUtf8("seekSlider"));
	seekSlider->setFocusPolicy(Qt::NoFocus);
	seekSlider->setMaximum(999);
	seekSlider->setPageStep(10);
	seekSlider->setOrientation(Qt::Horizontal);
	seekSlider->setInvertedAppearance(false);
	seekSlider->setInvertedControls(false);
	seekSlider->setTickPosition( QSlider::NoTicks );
	seekSlider->setTickInterval( 0 );
	horizontalLayout_2->insertWidget( 9, seekSlider );
	
	sampler = new Sampler();
	timeline = new Timeline( this );

	timelineView = new TimelineGraphicsView( 0 );
	timelineView->setFocusPolicy( Qt::NoFocus );
	timelineView->setAlignment( Qt::AlignLeft | Qt::AlignTop );
	timelineView->setScene( timeline );
	timelineView->setAcceptDrops( true );
	timelineStackedWidget->addWidget( timelineView );
	
	connect( this, SIGNAL(setCursorPos(double)), timeline, SLOT(setCursorPos(double)) );
	connect( timeline, SIGNAL(seekTo(double)), this, SLOT(timelineSeek(double)) );
	connect( timeline, SIGNAL(ensureVisible(const QGraphicsItem*)), this, SLOT(ensureVisible(const QGraphicsItem*)) );
	connect( timeline, SIGNAL(centerOn(const QGraphicsItem*)), this, SLOT(centerOn(const QGraphicsItem*)) );
	connect( timeline, SIGNAL(updateFrame()), sampler, SLOT(updateFrame()) );
	connect( timeline, SIGNAL(clipAddedToTimeline(Profile)), this, SLOT(clipAddedToTimeline(Profile)) );
	connect( timeline, SIGNAL(trackRequest(bool,int)), this, SLOT(trackRequest(bool,int)) );
	connect( timelineView, SIGNAL(sizeChanged(const QSize&)), timeline, SLOT(viewSizeChanged(const QSize&)) );
	
	animEditor = new AnimEditor( 0 );
	connect( animEditor, SIGNAL(quitEditor()), this, SLOT(quitEditor()) );
	connect( animEditor, SIGNAL(updateFrame()), sampler, SLOT(updateFrame()) );
	timelineStackedWidget->addWidget( animEditor );
	
	sourcePage = new ProjectSourcesPage( sampler );
	connect( sourcePage, SIGNAL(sourceActivated()), this, SLOT(sourceActivated()) );
	connect( sourcePage, SIGNAL(openSourcesBtnClicked()), this, SLOT(openSources()) );
	
	fxPage = new FxPage();
	connect( timeline, SIGNAL(clipSelected(Clip*)), fxPage, SLOT(clipSelected(Clip*)) );
	connect( timeline, SIGNAL(showEffects()), this, SLOT(showFxPage()) );
	connect( fxPage, SIGNAL(filterDeleted(Clip*,QSharedPointer<Filter>)), timeline, SLOT(filterDeleted(Clip*,QSharedPointer<Filter>)) );
	connect( fxPage, SIGNAL(filterDeleted(Clip*,QSharedPointer<Filter>)), animEditor, SLOT(filterDeleted(Clip*,QSharedPointer<Filter>)) );
	connect( fxPage, SIGNAL(updateFrame()), sampler, SLOT(updateFrame()) );
	connect( fxPage, SIGNAL(editAnimation(FilterWidget*,ParameterWidget*,Parameter*)), this, SLOT(editAnimation(FilterWidget*,ParameterWidget*,Parameter*)) );
	
	fxSettingsPage = new FxSettingsPage();
	connect( timeline, SIGNAL(clipSelected(Clip*)), fxSettingsPage, SLOT(clipSelected(Clip*)) );
	connect( timeline, SIGNAL(showTransition()), this, SLOT(showFxSettingsPage()) );
	connect( fxSettingsPage, SIGNAL(updateFrame()), sampler, SLOT(updateFrame()) );
	
	stackedWidget->addWidget( sourcePage );
	stackedWidget->addWidget( fxPage );
	stackedWidget->addWidget( fxSettingsPage );

	QHBoxLayout *layout = new QHBoxLayout;
	layout->setContentsMargins( 0, 0, 0, 0 );
	vw = new VideoWidget( glWidget );
	layout->addWidget( vw );
	glWidget->setLayout( layout );

	connect( vw, SIGNAL(playPause()), this, SLOT(videoPlayPause()) );
	connect( vw, SIGNAL(wheelSeek(int)), sampler, SLOT(wheelSeek(int)) );
	connect( vw, SIGNAL(newSharedContext(QGLWidget*)), sampler, SLOT(setSharedContext(QGLWidget*)) );
	connect( vw, SIGNAL(newFencesContext(QGLWidget*)), sampler, SLOT(setFencesContext(QGLWidget*)) );
	connect( vw, SIGNAL(newThumbContext(QGLWidget*)), this, SLOT(setThumbContext(QGLWidget*)) );
	connect( sampler->getMetronom(), SIGNAL(newFrame(Frame*)), vw, SLOT(showFrame(Frame*)) );
	connect( sampler->getMetronom(), SIGNAL(currentFramePts(double)), this, SLOT(currentFramePts(double)) );
	connect( vw, SIGNAL(frameShown(Frame*)), sampler->getMetronom(), SLOT(setLastFrame(Frame*)) );
	connect( sampler, SIGNAL(newFrame(Frame*)), vw, SLOT(showFrame(Frame*)) );
	connect( sampler, SIGNAL(paused(bool)), this, SLOT(composerPaused(bool)) );
	connect( sampler, SIGNAL(modeSwitched()), this, SLOT(modeSwitched()) );
	
	connect( sampler->getMetronom(), SIGNAL(osdMessage(const QString&,int)), vw, SLOT(showOSDMessage(const QString&,int)) );
	connect( sampler->getMetronom(), SIGNAL(osdTimer(bool)), vw, SLOT(showOSDTimer(bool)) );
	connect( this, SIGNAL(startOSDTimer(bool)), vw, SLOT(showOSDTimer(bool)) );

	connect( clipsToolButton, SIGNAL(clicked()), this, SLOT(showProjectClipsPage()) );
	connect( fxToolButton, SIGNAL(clicked()), this, SLOT(showFxPage()) );
	connect( fxSettingsToolButton, SIGNAL(clicked()), this, SLOT(showFxSettingsPage()) );

	connect( playToolButton, SIGNAL(toggled(bool)), this, SLOT(playPause(bool)) );
	connect( previousToolButton, SIGNAL(clicked()), this, SLOT(seekPrevious()) );
	connect( nextToolButton, SIGNAL(clicked()), this, SLOT(seekNext()) );
	connect( backwardToolButton, SIGNAL(clicked()), this, SLOT(seekBackward()) );
	connect( forwardToolButton, SIGNAL(clicked()), this, SLOT(seekForward()) );

	connect( seekSlider, SIGNAL(valueChanged(int)), this, SLOT(seek(int)) );

	connect( inButton, SIGNAL(clicked()), this, SLOT(setInPoint()) );
	connect( outButton, SIGNAL(clicked()), this, SLOT(setOutPoint()) );

	connect( switchButton, SIGNAL(toggled(bool)), sampler, SLOT(switchMode(bool)) );

	connect( actionNewProject, SIGNAL(triggered()), this, SLOT(newProject()) );
	connect( actionSave, SIGNAL(triggered()), this, SLOT(saveProject()) );
	connect( actionOpen, SIGNAL(triggered()), this, SLOT(loadProject()) );
	connect( actionPlayPause, SIGNAL(triggered()), this, SLOT(videoPlayPause()) );
	connect( actionForward, SIGNAL(triggered()), this, SLOT(playForward()) );
	connect( actionBackward, SIGNAL(triggered()), this, SLOT(playBackward()) );
	connect( actionFaster, SIGNAL(triggered()), this, SLOT(playFaster()) );
	connect( actionFrameByFrame, SIGNAL(triggered()), this, SLOT(seekNext()) );
	connect( actionFrameByFrameBackward, SIGNAL(triggered()), this, SLOT(seekPrevious()) );
	connect( actionSlower, SIGNAL(triggered()), this, SLOT(playSlower()) );
	connect( actionProjectSettings, SIGNAL(triggered()), this, SLOT(menuProjectSettings()) );
	connect( actionDeleteClip, SIGNAL(triggered()), timeline, SLOT(deleteClip()) );
	connect( actionSplitCurrentClip, SIGNAL(triggered()), timeline, SLOT(splitCurrentClip()) );
	connect( actionSaveImage, SIGNAL(triggered()), vw, SLOT(shot()) );
	connect( actionRenderToFile, SIGNAL(triggered()), this, SLOT(renderDialog()) );
	
	new QShortcut( QKeySequence( "Ctrl+M" ), this, SLOT(showMemoryInfo()) );
	
	timeline->setScene( sampler->getCurrentScene() );
}



void TopWindow::showMemoryInfo()
{
	MemDialog *d = new MemDialog( this );
	d->setModal( false );
	d->show();
}



void TopWindow::renderDialog()
{
	if ( !sampler->currentTimelineSceneDuration() ) {
		QMessageBox::warning( this, tr("Sorry"), tr("The timeline is empty.") );
		return;
	}

	double playhead = 0;
	Frame *f = sampler->getMetronom()->getLastFrame();
	if ( f && !sampler->previewMode() )
		playhead = f->pts();
	else
		playhead = sampler->getCurrentScene()->currentPTS;		

	RenderingDialog *dlg = new RenderingDialog( this,
								sampler->getCurrentScene()->getProfile(),
								playhead,
								sampler->currentTimelineSceneDuration(),
								&sampler->getMetronom()->audioFrames,
								&sampler->getMetronom()->encodeVideoFrames );

	connect( dlg, SIGNAL(renderStarted(double)), this, SLOT(renderStart(double)) );
	connect( dlg, SIGNAL(renderFinished(double)), this, SLOT(renderFinished(double)) );
	connect( dlg, SIGNAL(showFrame(Frame*)), vw, SLOT(showFrame(Frame*)) );
	connect( this, SIGNAL(timelineReadyForEncode()), dlg, SLOT(timelineReady()) );
	dlg->exec();
	delete dlg;
}

void TopWindow::renderStart( double startPts )
{
	timelineSeek( startPts );
	vw->clear();
	sampler->getMetronom()->setRenderMode( true );
	playPause( true );
	emit timelineReadyForEncode();
}

void TopWindow::renderFinished( double pts )
{
	timelineSeek( pts );
	sampler->getMetronom()->setRenderMode( false );
	timelineSeek( pts );
}



void TopWindow::trackRequest( bool rm, int index )
{
	if ( rm ) {
		if ( !sampler->trackRequest( rm, index ) ) {
			QMessageBox::warning( this, tr("Error"), tr("Only empty track can be deleted.") );
		}
		else {
			timeline->trackRemoved( index );
		}
	}
	else {
		if ( !sampler->trackRequest( rm, index ) ) {
			QMessageBox::warning( this, tr("Error"), tr("Track could not be added.") );
		}
		else {
			timeline->trackAdded( index );
		}
	}
}



void TopWindow::clipAddedToTimeline( Profile prof )
{
	tempProfile = prof;
	QTimer::singleShot( 1, this, SLOT(projectSettings()) );
}



void TopWindow::newProject()
{
	if ( openSourcesCounter != 0 ) {
		QMessageBox msgBox;
		msgBox.setText( tr("Files are being loaded.\nWait until the operation has completed.") );
		msgBox.exec();
		return;
	}

	tempProfile = Profile();
	ProjectProfileDialog dlg( this, tempProfile, WARNNO );
	dlg.exec();
	if ( dlg.result() == QDialog::Accepted ) {
		showProjectClipsPage();
		sampler->newProject( dlg.getCurrentProfile() );
		timeline->setScene( sampler->getCurrentScene() );
		sourcePage->clearAllSources();
		timelineSeek( 0 );
		QTimer::singleShot( VIDEOCLEARDELAY, vw, SLOT(clear()) );
	}
}



void TopWindow::menuProjectSettings()
{
	tempProfile = sampler->getCurrentScene()->getProfile();
	int warn = WARNNO;
	if ( !sampler->isProjectEmpty() )
		warn = WARNCHANGE;
	projectSettings( warn );
}



void TopWindow::projectSettings( int warn )
{
	ProjectProfileDialog dlg( this, tempProfile, warn );
	dlg.exec();
	if ( dlg.result() == QDialog::Accepted ) {
		if ( !sampler->setProfile( dlg.getCurrentProfile() ) )
			QMessageBox::warning( this, tr("Error"), tr("Somme errors occured while changing profile.\nCheck clips alignement.") );
		timeline->setScene( sampler->getCurrentScene() );
	}
}



void TopWindow::editAnimation( FilterWidget* f, ParameterWidget *pw, Parameter *p )
{
	timelineStackedWidget->setCurrentIndex( 1 );
	animEditor->setCurrentParam( f, pw, p );
}



void TopWindow::quitEditor()
{
	timelineStackedWidget->setCurrentIndex( 0 );
}



void TopWindow::modeSwitched()
{
	if ( sourcePage->hasActiveSource() )
		sampler->setSource( sourcePage->sourceActiveSource(), sourcePage->currentPtsActiveSource() );
}



void TopWindow::setInPoint()
{
	// ATTENTION preview pts is now in timeline range (start at 0)
	if ( sourcePage->hasActiveSource() ) {
		//double pts;
		//QImage img = vw->getThumb( THUMBHEIGHT, &pts, false );
		//if ( !img.isNull() )
			//activeSource->setInPoint( img, pts );
	}
}



void TopWindow::setOutPoint()
{
	// ATTENTION preview pts is now in timeline range (start at 0)
	if ( sourcePage->hasActiveSource() ) {
		//double pts;
		//QImage img = vw->getThumb( THUMBHEIGHT, &pts, true );
		//if ( !img.isNull() ) {
			//activeSource->setOutPoint( img, pts );
			//sourcePage->newCut( activeSource );
		//}
	}
}



Source* TopWindow::getDroppedCut( int index, QString mime, QString filename, double &start, double &len )
{
	if ( mime == MIMETYPECUT ) {
		if ( !sourcePage->hasActiveSource() )
			return NULL;
		Cut *cut = sourcePage->activeSourceGetCut( index, filename );
		if ( !cut )
			return NULL;
		start = cut->getStart();
		len = cut->getLength();
		return cut->getSource();
	}
	if ( mime == MIMETYPESOURCE ) {
		Source *source = sourcePage->getSource( index, filename );
		start = source->getProfile().getStreamStartTime();
		len = source->getProfile().getStreamDuration();
		return source;
	}
	return NULL;
}



void TopWindow::ensureVisible( const QGraphicsItem *it )
{
	if ( playToolButton->isChecked() )
		timelineView->ensureVisible( it, 100, 100 );
}



void TopWindow::centerOn( const QGraphicsItem *it )
{
	timelineView->centerOn( it );
}



void TopWindow::sourceActivated()
{
	if ( sourcePage->hasActiveSource() ) {
		if ( switchButton->isChecked() )
			switchButton->toggle();
		else
			sampler->setSource( sourcePage->sourceActiveSource(), sourcePage->currentPtsActiveSource() );
	}
}



void TopWindow::showProjectClipsPage()
{
	stackedWidget->setCurrentIndex( 0 );
}

void TopWindow::showFxPage()
{
	stackedWidget->setCurrentIndex( 1 );
}

void TopWindow::showFxSettingsPage()
{
	stackedWidget->setCurrentIndex( 2 );
}



void TopWindow::currentFramePts( double d )
{
	if ( !switchButton->isChecked() ) {
		if ( !sourcePage->hasActiveSource() )
			return;
		sourcePage->activeSourceSetCurrentPts( d );
	}
	else {
		emit setCursorPos( d ); 
	}
	
	d *= seekSlider->maximum() / sampler->currentSceneDuration();
	seekSlider->blockSignals( true );
	seekSlider->setValue( d );
	seekSlider->blockSignals( false );
}



void TopWindow::composerPaused( bool b )
{
	// avoid toggled() to be emitted
	playToolButton->blockSignals( true );
	playToolButton->setChecked( !b );
	playToolButton->blockSignals( false );
}



void TopWindow::playPause( bool playing )
{
	sampler->play( playing );
}



void TopWindow::videoPlayPause()
{
	playToolButton->toggle();
}



void TopWindow::playForward()
{
	if ( sampler->play( true ) )
		composerPaused( false );
	else
		playFaster();
}



void TopWindow::playBackward()
{
	if ( sampler->play( true, true ) )
		composerPaused( false );
	else
		playFaster();
}



void TopWindow::playFaster()
{
	sampler->getMetronom()->changeSpeed( 1 );
}



void TopWindow::playSlower()
{
	sampler->getMetronom()->changeSpeed( -1 );
}



void TopWindow::seekPrevious()
{
	sampler->wheelSeek( -1 );
}



void TopWindow::seekNext()
{
	sampler->wheelSeek( 1 );
}



void TopWindow::seekBackward()
{
	sampler->wheelSeek( -10 );
}



void TopWindow::seekForward()
{
	sampler->wheelSeek( 10 );
}



void TopWindow::seek( int v )
{
	double value = v * sampler->currentSceneDuration() / seekSlider->maximum();
	sampler->slideSeek( value );
}



void TopWindow:: timelineSeek( double pts )
{
	if ( !switchButton->isChecked() )
		switchButton->toggle();
	
	sampler->slideSeek( pts );
}



void TopWindow::setThumbContext( QGLWidget *context )
{
	thumbnailer->setSharedContext( context );
	connect( thumbnailer, SIGNAL(resultReady()), thumbnailer, SLOT(gotResult()) );
	connect( thumbnailer, SIGNAL(thumbReady(ThumbRequest)), this, SLOT(thumbResultReady(ThumbRequest)) );
}



void TopWindow::clipThumbRequest( ThumbRequest request )
{
	thumbnailer->pushRequest( request );
}



void TopWindow::openSources()
{
	QStringList	list = QFileDialog::getOpenFileNames( this, tr("Open files"), openSourcesCurrentDir,
		"Videos(*.3gp *.dv *.m2t *.mts *.mkv *.mpg *.mpeg *.ts *.avi *.mov *.vob *.wmv *.mjpg *.mp4 *.ogg *.wav *.mp3 *.ac3 *.mp2 *.mpa *.mpc *.png *.jpg)" );

	if ( !list.isEmpty() ) {
		openSourcesCurrentDir = QFileInfo( list[0] ).absolutePath();
		
		for ( int i = 0; i < list.count(); ++i ) {
			if ( sourcePage->exists( list[i] ) )
				duplicateOpenSources.append( QFileInfo( list[i] ).fileName() );
			else {
				if ( thumbnailer->pushRequest( ThumbRequest( list[i] ) ) )
					++openSourcesCounter;
			}
		}
		if ( !openSourcesCounter )
			unsupportedDuplicateMessage();
	}
}



void TopWindow::unsupportedDuplicateMessage()
{
	QMessageBox msgBox;
	msgBox.setText( tr("Some files are unsupported or already part of the project.") );
	msgBox.setDetailedText( unsupportedOpenSources.join( "\n" ) + "\n" + duplicateOpenSources.join( "\n" ) );
	msgBox.exec();
	
	unsupportedOpenSources.clear();
	duplicateOpenSources.clear();
}

		

void TopWindow::thumbResultReady( ThumbRequest result )
{
	if ( result.typeOfRequest == ThumbRequest::THUMB ) {
		timeline->thumbResultReady( result );
		return;
	}

	if ( projectLoader ) {
		Source *source = NULL;
		for ( int i = 0; i < projectLoader->sourcesList.count(); ++i ) {
			if ( projectLoader->sourcesList[i]->getFileName() == result.filePath ) {
				source = projectLoader->sourcesList.takeAt( i );
				break;
			}
		}
		
		if ( !source ) // what happens???
			return;
		
		if ( !result.thumb.isNull() ) {
			source->setAfter( (InputBase::InputType)result.inputType, result.profile );
			sourcePage->addSource( QPixmap::fromImage( result.thumb ), source );
		}
		else {
			for ( int k = 0; k < projectLoader->sceneList.count(); ++k ) {
				Scene *scene = projectLoader->sceneList[k];
				for ( int i = 0; i < scene->tracks.count(); ++i ) {
					Track *t = scene->tracks[i];
					for ( int j = 0; j < t->clipCount(); ++j ) {
						Clip *c = t->clipAt( j );
						if ( c->sourcePath() == source->getFileName() ) {
							t->removeClip( j-- );
							delete c;
						}
					}
				}
			}
			unsupportedOpenSources.append( source->getFileName() );
			delete source;
		}
		
		if ( !projectLoader->sourcesList.count() ) {
			setEnabled( true );
			sampler->setSceneList( projectLoader->sceneList );
			timeline->setScene( sampler->getCurrentScene() );
			timelineSeek( 0 );
			emit startOSDTimer( false );
			
			if ( projectLoader->readError ) {
				QMessageBox msgBox;
				msgBox.setText( tr("Some errors occured while reading the project file.\nCheck the timeline.") );
				msgBox.exec();
			}
			else if ( unsupportedOpenSources.count() ) {
				QMessageBox msgBox;
				msgBox.setText( tr("Some sources are unsupported or missing.\nCorresponding clips have been removed.") );
				msgBox.setDetailedText( unsupportedOpenSources.join( "\n" ) );
				msgBox.exec();
			}
			delete projectLoader;
			projectLoader = NULL;
		}
	}
	else {
		if ( !result.thumb.isNull() ) {
			Source *source = new Source( (InputBase::InputType)result.inputType, result.filePath, result.profile );
			sourcePage->addSource( QPixmap::fromImage( result.thumb ), source );
		}
		else
			unsupportedOpenSources.append( QFileInfo( result.filePath ).fileName() );
		
		if ( --openSourcesCounter == 0 ) {
			if ( unsupportedOpenSources.count() || duplicateOpenSources.count() )
				unsupportedDuplicateMessage();
		}
	}
}



void TopWindow::saveProject()
{
	QString file = QFileDialog::getSaveFileName( this, tr("Save project"),
						openSourcesCurrentDir, "MachinTruc(*.mct)" );

	if ( file.isEmpty() )
		return;
	
	ProjectFile xml;
	xml.saveProject( sourcePage->getAllSources(), sampler, file );
}



void TopWindow::loadProject()
{
	if ( openSourcesCounter != 0 ) {
		QMessageBox msgBox;
		msgBox.setText( tr("You can't load a project while files are being loaded.\nWait until the operation has completed.") );
		msgBox.exec();
		return;
	}
	
	QString file = QFileDialog::getOpenFileName( this, tr("Open project"),
						openSourcesCurrentDir, "MachinTruc(*.mct)" );

	if ( file.isEmpty() )
		return;
	
	showProjectClipsPage();
	sampler->drainScenes();
	timeline->setScene( sampler->getCurrentScene() );
	sourcePage->clearAllSources();
	QTimer::singleShot( VIDEOCLEARDELAY, vw, SLOT(clear()) );

	projectLoader = new ProjectFile();

	if ( projectLoader->loadProject( file ) && projectLoader->sourcesList.count() ) {
		int i;
		for ( i = 0; i < projectLoader->sourcesList.count(); ++i ) {
			thumbnailer->pushRequest( ThumbRequest( projectLoader->sourcesList[i]->getFileName() ) );
		}
		if ( i > 0 ) {
			setEnabled( false );
			emit startOSDTimer( true );
		}
	}
	else {
		while ( projectLoader->sourcesList.count() )
			delete projectLoader->sourcesList.takeFirst();
		
		while ( projectLoader->sceneList.count() )
			delete projectLoader->sceneList.takeFirst();
			
		QMessageBox msgBox;
		msgBox.setText( tr("Project loading failed.\nThe file seems corrupted.") );
		msgBox.exec();
		delete projectLoader;
		projectLoader = NULL;
	}
}

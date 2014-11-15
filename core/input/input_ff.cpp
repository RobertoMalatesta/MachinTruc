// kate: tab-indent on; indent-width 4; mixedindent off; indent-mode cstyle; remove-trailing-space on;


#include "input/input_ff.h"



InputFF::InputFF() : InputBase(),
	decoder( new FFDecoder() ),
	backwardAudioSamples( 0 ),
	semaphore( new QSemaphore( 1 ) ),
	running( false ),
	seekAndPlayPTS( 0 ),
	seekAndPlay( false ),
	playBackward( false ),
	backwardPts( 0 ),
	eofVideo( false ),
	eofAudio( false )
{
	inputType = FFMPEG;

	for ( int i = 0; i < NUMINPUTFRAMES; ++i ) {
		freeAudioFrames.enqueue( new Frame( &freeAudioFrames ) );
		freeVideoFrames.enqueue( new Frame( &freeVideoFrames ) );
	}
}



InputFF::~InputFF()
{
	delete decoder;

	flush();

	Frame *f;
	while ( (f = freeAudioFrames.dequeue()) )
		delete f;
	while ( (f = freeVideoFrames.dequeue()) )
		delete f;
}



void InputFF::flush()
{
	Frame *f;
	while ( (f = videoFrames.dequeue()) )
		f->release();
	while ( (f = reorderedVideoFrames.dequeue()) )
		delete f;
	while ( !backwardVideoFrames.isEmpty() )
		delete backwardVideoFrames.takeFirst();

	while ( (f = audioFrames.dequeue()) )
		f->release();
	while ( !backwardAudioFrames.isEmpty() )
		delete backwardAudioFrames.takeFirst();
	backwardAudioSamples = 0;

	lastFrame.set( NULL );
	audioFrameList.reset( outProfile );
	videoResampler.reset( outProfile.getVideoFrameDuration() );
	eofVideo = eofAudio = false;
}




bool InputFF::open( QString fn )
{
	bool ok = decoder->open( fn );
	if ( ok )
		sourceName = fn;

	flush();

	return ok;
}



bool InputFF::probe( QString fn, Profile *prof )
{
	return decoder->probe( fn, prof );
}



double InputFF::seekTo( double p )
{
	flush();
	mmiSeek();

	if ( playBackward ) {
		backwardPts = p;
		Frame *f = new Frame( NULL );
		AudioFrame *af = new AudioFrame( audioFrameList.getBytesPerSample() );
		bool ok = decoder->seekTo( p - MICROSECOND, f, af );
		if ( af->buffer ) {
			backwardAudioFrames.append( af );
			backwardAudioSamples += af->available;
		}
		else
			delete af;
		if ( ok && f->getBuffer() ) {
			backwardVideoFrames.append( f );
			return f->pts();
		}
		else {
			delete f;
		}
	}
	else {
		Frame *f = freeVideoFrames.dequeue();
		if ( !f )
			return p;
		AudioFrame *af = new AudioFrame( audioFrameList.getBytesPerSample() );
		bool ok = decoder->seekTo( p, f, af );
		if ( af->buffer )
			audioFrameList.append( af );
		else
			delete af;
		if ( ok && f->getBuffer() ) {
			lastFrame.set( f );
			videoResampler.reset( outProfile.getVideoFrameDuration() );
			videoResampler.outputPts = f->pts();
			resample( f );
			return f->pts();
		}
		else {
			f->release();
		}
	}

	return p;
}



/* openSeekPlay is asynchronous and as such it could be called again before open and seek
 * have completed. So the need for a mutex that is also required in getVideoFrame/getAudioFrame
 * since we want to wait for seek completion before asking for a frame.
 * We use a semaphore here because a mutex must be locked and unlocked in the same thread
 * but we have to lock in openSeekPlay (main thread) and unlock in run (other thread).
 * This SemaphoreLocker used in getVideoFrame/getAudioFrame behaves like QMutexLocker.*/
class SemaphoreLocker
{
public:
	explicit SemaphoreLocker( QSemaphore *s ) {
		sem = s;
		sem->acquire();
	}
	~SemaphoreLocker() {
		sem->release();
	}

private:
	QSemaphore *sem;
};



void InputFF::openSeekPlay( QString fn, double p, bool backward )
{
	semaphore->acquire();
	play( false );
	seekAndPlayPTS = p;
	seekAndPlayPath = fn;
	seekAndPlay = true;
	playBackward = backward;
	start();
}



void InputFF::play( bool b )
{
	if ( !b ) {
		running = false;
		wait();
	}
	else {
		running = true;
		start();
	}
}



void InputFF::run()
{
	if ( seekAndPlay ) {
		if ( seekAndPlayPath != sourceName || outProfile.hasVideo() != decoder->haveVideo || outProfile.hasAudio() != decoder->haveAudio )
			open( seekAndPlayPath );
		seekTo( seekAndPlayPTS );
		seekAndPlay = false;
		running = true;
		semaphore->release();
	}

	// in case file has moved
	if ( !decoder->formatCtx ) {
		decoder->endOfFile = FFDecoder::EofPacket | FFDecoder::EofAudioPacket | FFDecoder::EofVideoPacket | FFDecoder::EofAudio | FFDecoder::EofVideo;
		running = false;
	}

	if ( playBackward )
		runBackward();
	else
		runForward();
}



void InputFF::runForward()
{
	int doWait;
	Frame *f;

	while ( running ) {
		doWait = 1;

		if ( decoder->haveVideo && !eofVideo ) {
			if ( (f = freeVideoFrames.dequeue()) ) {
				// resample if necessary
				if ( videoResampler.repeat && lastFrame.valid() ) {
					// duplicate previous frame
					lastFrame.get( f );
					videoResampler.duplicate( f );
					f->mmi = mmi;
					f->mmiProvider = mmiProvider;
					if ( !videoResampler.repeat )
						mmiIncrement();
					videoFrames.enqueue( f );
				}
				else {
					f->mmi = mmi;
					f->mmiProvider = mmiProvider;
					mmiIncrement();
					if ( decoder->decodeVideo( f ) ) {
						lastFrame.set( f );
						resample ( f );
					}
					else
						f->release();
				}

				if ( (decoder->endOfFile & FFDecoder::EofVideo) && !videoResampler.repeat )
					eofVideo = true;
				doWait = 0;
			}
		}

		if ( decoder->haveAudio && !eofAudio ) {
			if ( audioFrameList.writable() ) {
				AudioFrame *af = new AudioFrame( audioFrameList.getBytesPerSample() );
				decoder->decodeAudio( af );
				if ( af->buffer )
					audioFrameList.append( af );
				else
					delete af;

				if ( decoder->endOfFile & FFDecoder::EofAudio )
					eofAudio = true;
				doWait = 0;
			}
		}

		if ( decoder->haveVideo && decoder->haveAudio ) {
			if ( eofVideo && eofAudio ) {
				printf("ff.run break\n");
				break;
			}
		}
		else if ( decoder->haveVideo ) {
			if ( eofVideo ) {
				printf("ff.run break\n");
				break;
			}
		}
		else if ( decoder->haveAudio ) {
			if ( eofAudio ) {
				printf("ff.run break\n");
				break;
			}
		}

		if ( doWait ) {
			usleep( 1000 );
		}
	}
}



void InputFF::runBackward()
{
	int doWait;
	bool endVideoSequence = !decoder->haveVideo;
	bool endAudioSequence = !decoder->haveAudio;

	while ( running ) {
		doWait = 1;

		if ( !eofVideo && !endVideoSequence ) {
			Frame *f = new Frame( NULL );
			if ( decoder->decodeVideo( f ) ) {
				backwardVideoFrames.append( f );
				if ( backwardVideoFrames.count() == outProfile.getVideoFrameRate() )
					endVideoSequence = true;
			}
			else {
				delete f;
				endVideoSequence = true;
			}
			doWait = 0;
		}

		if ( !eofAudio && !endAudioSequence ) {
			AudioFrame *af = new AudioFrame( audioFrameList.getBytesPerSample() );
			decoder->decodeAudio( af );
			if ( af->buffer ) {
				if ( backwardAudioSamples + af->available >= outProfile.getAudioSampleRate() ) {
					af->available = outProfile.getAudioSampleRate() - backwardAudioSamples;
					endAudioSequence = true;
				}
				backwardAudioSamples += af->available;
				backwardAudioFrames.append( af );
			}
			else {
				delete af;
				endAudioSequence = true;
			}
			doWait = 0;
		}

		if ( endVideoSequence && endAudioSequence && !audioFrameList.readable( 2000 ) ) {
			bool eof = false;
			if ( backwardVideoFrames.count() ) {
				if ( backwardVideoFrames.first()->pts() == inProfile.getStreamStartTime() )
					eof = true;
			}
			else if ( backwardAudioFrames.count() ) {
				if ( backwardAudioFrames.first()->bufPts == inProfile.getStreamStartTime() )
					eof = true;
			}

			while ( !backwardVideoFrames.isEmpty() )
				reorderedVideoFrames.enqueue( backwardVideoFrames.takeLast() );
			while ( !backwardAudioFrames.isEmpty() ) {
				int bps = audioFrameList.getBytesPerSample();
				AudioFrame *af = backwardAudioFrames.takeLast();
				int size = af->available * bps;
				Buffer *buffer = BufferPool::globalInstance()->getBuffer( size );
				uint8_t *src = af->buffer->data() + af->bufOffset + size - bps;
				uint8_t *dst = buffer->data();
				af->bufSize = size;
				af->bufOffset = 0;
				while ( size ) {
					memcpy( dst, src, bps );
					src -= bps;
					dst += bps;
					size -= bps;
				}
				BufferPool::globalInstance()->releaseBuffer( af->buffer );
				af->buffer = buffer;

				audioFrameList.append( af );
			}

			if ( eof ) {
				eofVideo = eofAudio = true;
			}
			else {
				backwardAudioSamples = 0;
				backwardPts -= MICROSECOND;
				backwardPts -= inProfile.getVideoFrameDuration();
				Frame *f = new Frame( NULL );
				AudioFrame *af = new AudioFrame( audioFrameList.getBytesPerSample() );
				bool ok = decoder->seekTo( backwardPts - MICROSECOND, f, af );
				if ( af->buffer ) {
					backwardAudioSamples += af->available;
					backwardAudioFrames.append( af );
				}
				else
					delete af;
				if ( ok && f->getBuffer() )
					backwardVideoFrames.append( f );
				else
					delete f;
			}
			endVideoSequence = !decoder->haveVideo;
			endAudioSequence = !decoder->haveAudio;
		}

		if ( doWait ) {
			usleep( 1000 );
		}
	}
}



void InputFF::resample( Frame *f )
{
	double delta = ( f->pts() + f->profile.getVideoFrameDuration() ) - ( videoResampler.outputPts + videoResampler.outputDuration );
	if ( outProfile.getVideoFrameRate() == inProfile.getVideoFrameRate() ) { // no resampling
		videoFrames.enqueue( f );
	}
	else if ( delta >= videoResampler.outputDuration ) {
		// this frame will be duplicated (delta / videoResampler.outputDuration) times
		printf("duplicate, delta=%f, f->pts=%f, outputPts=%f\n", delta, f->pts(), videoResampler.outputPts);
		// add some to delta to prevent subduplicate
		videoResampler.setRepeat( f->pts(), (delta + 1) / videoResampler.outputDuration );
		videoFrames.enqueue( f );
		videoResampler.outputPts += videoResampler.outputDuration;
	}
	else if ( delta <= -videoResampler.outputDuration ) {
		// skip
		printf("skip frame delta=%f, f->pts=%f, outputPts=%f\n", delta, f->pts(), videoResampler.outputPts);
		f->release();
	}
	else {
		videoFrames.enqueue( f );
		videoResampler.outputPts += videoResampler.outputDuration;
	}
}



Frame* InputFF::getVideoFrame()
{
	SemaphoreLocker sem( semaphore );

	if ( !decoder->haveVideo ) {
		return NULL;
	}

	if ( playBackward ) {
		while ( freeVideoFrames.queueEmpty() )
			usleep( 1000 );
		while ( reorderedVideoFrames.queueEmpty() ) {
			if ( eofVideo )
				return NULL;
			usleep( 1000 );
		}
		Frame *rf = reorderedVideoFrames.dequeue();
		Frame *f = freeVideoFrames.dequeue();
		f->setSharedBuffer( rf->getBuffer() );
		f->setVideoFrame( (Frame::DataType)rf->type(), rf->profile.getVideoWidth(), rf->profile.getVideoHeight(),
						  rf->profile.getVideoSAR(), rf->profile.getVideoInterlaced(),
						  rf->profile.getVideoTopFieldFirst(), rf->pts(),
						  rf->profile.getVideoFrameDuration(),
						  rf->orientation() );
		f->profile = rf->profile;
		delete rf;
		return f;
	}

	while ( videoFrames.queueEmpty() ) {
		if ( eofVideo ) {
			if ( lastFrame.valid() ) {
				while ( freeVideoFrames.queueEmpty() )
					usleep( 1000 );
				Frame *f = freeVideoFrames.dequeue();
				lastFrame.get( f );
				return f;
			}
			return NULL;
		}
		usleep( 1000 );
	}

	Frame *f = videoFrames.dequeue();
	return f;
}



Frame* InputFF::getAudioFrame( int nSamples )
{
	SemaphoreLocker sem( semaphore );

	if ( !decoder->haveAudio ) {
		return NULL;
	}

	while ( freeAudioFrames.queueEmpty() ) {
		usleep( 1000 );
	}
	Frame *f = freeAudioFrames.dequeue();

	while ( !audioFrameList.readable( nSamples ) ) {
		if ( eofAudio ) {
			f->setAudioFrame( outProfile.getAudioChannels(), outProfile.getAudioSampleRate(), Profile::bytesPerChannel( &outProfile ), nSamples, audioFrameList.readPts() );
			int n = audioFrameList.read( f->data() );
			if ( !n ) {
				f->release();
				return NULL;
			}
			// complete with silence
			memset( f->data() + ( n * audioFrameList.getBytesPerSample() ), 0, (nSamples - n) * audioFrameList.getBytesPerSample() );
			return f;
		}
		usleep( 1000 );
	}

	f->setAudioFrame( outProfile.getAudioChannels(), outProfile.getAudioSampleRate(), Profile::bytesPerChannel( &outProfile ), nSamples, audioFrameList.readPts() );
	audioFrameList.read( f->data(), nSamples );

	return f;
}

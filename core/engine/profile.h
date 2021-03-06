#ifndef PROFILE_H
#define PROFILE_H

#include <QObject>
#include <QString>

#define MAXPROJECTWIDTH 3840
#define MAXPROJECTHEIGHT 2160

#define MILLISECOND 1000.0
#define MICROSECOND 1000000.0

#define DEFAULTSAMPLERATE 48000
#define DEFAULTCHANNELS 2
#define DEFAULTLAYOUT Profile::LAYOUT_STEREO
#define DEFAULTSAMPLEFORMAT Profile::SAMPLE_FMT_32F



class Profile
{
public:
	enum SampleFormat{ SAMPLE_FMT_NATIVE, SAMPLE_FMT_S16, SAMPLE_FMT_32F, SAMPLE_LAST };
	enum AudioLayout{ LAYOUT_NATIVE, LAYOUT_STEREO, LAYOUT_51, LAYOUT_LAST };
	enum ColorSpace{ SPC_UNDEF, SPC_709, SPC_601_625, SPC_601_525, SPC_SRGB, SPC_LAST };
	enum ColorPrimaries{ PRI_UNDEF, PRI_709, PRI_SRGB, PRI_601_625, PRI_601_525, PRI_LAST };
	enum ChromaLocation{ LOC_UNDEF, LOC_LEFT, LOC_CENTER, LOC_TOPLEFT, LOC_LAST };
	enum GammaCurve{ GAMMA_UNDEF, GAMMA_709, GAMMA_601, GAMMA_SRGB, GAMMA_LAST };

	Profile();
	static int bytesPerChannel( Profile *prof );
	QString colorPrimariesName();
	QString gammaCurveName();	
	QString colorSpaceName();

	void setVideoFrameRate( double fr ) { videoFrameRate = fr; }
	void setVideoFrameDuration( double d ) { videoFrameDuration = d; }
	void setVideoWidth( int w ) { videoWidth = w; }
	void setVideoHeight( int h ) { videoHeight = h; }
	void setVideoSAR( double sar ) { videoSAR = sar; }
	void setVideoInterlaced( bool il ) { videoInterlaced = il; }
	void setVideoTopFieldFirst( bool tff ) { videoTopFieldFirst = tff; }
	void setStreamStartTime( double st ) { streamStartTime = st; }
	void setStreamDuration( double d ) { streamDuration = d; }
	void setVideoColorSpace( int cs ) { videoColorSpace = cs; }
	void setVideoColorPrimaries( int cp ) { videoColorPrimaries = cp; }
	void setVideoColorFullRange( bool b ) { videoColorFullRange = b; }
	void setVideoChromaLocation( int cl ) { videoChromaLocation = cl; }
	void setVideoGammaCurve( int g ) { videoGammaCurve = g; }
	void setVideoCodecName( QString s ) { videoCodecName = s; }

	double getVideoFrameRate() const { return videoFrameRate; }
	double getVideoFrameDuration() const { return videoFrameDuration; }
	int getVideoWidth() const { return videoWidth; }
	int getVideoHeight() const { return videoHeight; }
	double getVideoSAR() const { return videoSAR; }
	bool getVideoInterlaced() const { return videoInterlaced; }
	bool getVideoTopFieldFirst() const { return videoTopFieldFirst; }
	double getStreamStartTime() const { return streamStartTime; }
	double getStreamDuration() const { return streamDuration; }
	int getVideoColorSpace() const { return videoColorSpace; }
	int getVideoColorPrimaries() const { return videoColorPrimaries; }
	bool getVideoColorFullRange() const { return videoColorFullRange; }
	int getVideoChromaLocation() const { return videoChromaLocation; }
	int getVideoGammaCurve() const { return videoGammaCurve; }
	const QString & getVideoCodecName() const { return videoCodecName; }

	void setAudioSampleRate( int r ) { audioSampleRate = r; }
	void setAudioChannels( int c ) { audioChannels = c; }
	void setAudioFormat( int f ) { audioFormat = f; }
	void setAudioLayout( int l ) { audioLayout = l; }
	void setAudioCodecName( QString s ) { audioCodecName = s; }
	void setAudioLayoutName( QString s ) { audioLayoutName = s; }

	int getAudioSampleRate() const { return audioSampleRate; }
	int getAudioChannels() const { return audioChannels; }
	int getAudioFormat() const { return audioFormat; }
	int getAudioLayout() const { return audioLayout; }
	const QString & getAudioCodecName() const { return audioCodecName; }
	const QString & getAudioLayoutName() const { return audioLayoutName; }

	bool hasVideo() const { return haveVideo; }
	bool hasAudio() const { return haveAudio; }
	void setHasVideo( bool b ) { haveVideo = b; }
	void setHasAudio( bool b ) { haveAudio = b; }

private:
	double videoFrameRate;
	double videoFrameDuration;
	int videoWidth;
	int videoHeight;
	double videoSAR;
	bool videoInterlaced;
	bool videoTopFieldFirst;
	int videoColorSpace;
	int videoColorPrimaries;
	int videoChromaLocation;
	int videoGammaCurve;
	bool videoColorFullRange;
	
	bool haveVideo, haveAudio;

	int audioSampleRate;
	int audioChannels;
	int audioFormat;
	int audioLayout;

	double streamStartTime;
	double streamDuration;
	
	QString audioLayoutName;
	QString audioCodecName;
	QString videoCodecName;
};
#endif //PROFILE_H

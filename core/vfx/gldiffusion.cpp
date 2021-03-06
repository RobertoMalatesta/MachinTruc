#include <movit/diffusion_effect.h>
#include "vfx/gldiffusion.h"



GLDiffusion::GLDiffusion( QString id, QString name ) : GLFilter( id, name )
{
	mixAmount = addParameter( "mixAmount", tr("Amount:"), Parameter::PDOUBLE, 0.3, 0.0, 1.0, true );
	blurRadius = addParameter( "blurRadius", tr("Radius:"), Parameter::PDOUBLE, 3.0, 0.0, 10.0, true );
}



GLDiffusion::~GLDiffusion()
{
}



bool GLDiffusion::process( const QList<Effect*> &el, double pts, Frame *src, Profile *p )
{
	Q_UNUSED( src );
	Q_UNUSED( p );
	return el.at(0)->set_float( "blurred_mix_amount", getParamValue( mixAmount, pts ).toFloat() )
		&& el.at(0)->set_float( "radius", getParamValue( blurRadius, pts ).toFloat() );
}



QList<Effect*> GLDiffusion::getMovitEffects()
{
	QList<Effect*> list;
	list.append( new DiffusionEffect() );
	return list;
}

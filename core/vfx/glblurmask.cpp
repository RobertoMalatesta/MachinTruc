#include <assert.h>
#include <vector>

#include <movit/effect_chain.h>

#include "vfx/glblurmask.h"



GLBlurmask::GLBlurmask( QString id, QString name ) : GLFilter( id, name )
{
	radius = 40.0;
}



GLBlurmask::~GLBlurmask()
{
}



bool GLBlurmask::process( const QList<Effect*> &el, double pts, Frame *src, Profile *p )
{
	Q_UNUSED( pts );
	Q_UNUSED( src );
	Q_UNUSED( p );
	return el.at(0)->set_float( "radius", radius );
}



QList<Effect*> GLBlurmask::getMovitEffects()
{
	QList<Effect*> list;
	list.append( new BlurEffectMask() );
	return list;
}



BlurEffectMask::BlurEffectMask()
	: blur(new BlurEffect),
	  maskfx(new MaskEffect)
{
}

void BlurEffectMask::rewrite_graph(EffectChain *graph, Node *self)
{
	assert(self->incoming_links.size() == 1);
	Node *input = self->incoming_links[0];

	Node *blur_node = graph->add_node(blur);
	Node *mask_node = graph->add_node(maskfx);
	graph->replace_receiver(self, mask_node);
	graph->connect_nodes(input, blur_node);
	graph->connect_nodes(blur_node, mask_node);
	graph->replace_sender(self, mask_node);

	self->disabled = true;
}

bool BlurEffectMask::set_float(const std::string &key, float value)
{
	return blur->set_float(key, value);
}

#ifndef ANIMITEM_H
#define ANIMITEM_H

#include "engine/parameter.h"
#include "engine/clip.h"
#include "gui/filter/filterwidget.h"
#include "keyitem.h"



class AnimItem : public QObject, public QGraphicsRectItem
{
	Q_OBJECT
public:
	AnimItem();
	
	void itemSelected( KeyItem *it );
	void itemMove( KeyItem *it, QPointF mouse, QPointF startPos, QPointF startMouse );
	void keyDoubleClicked( KeyItem *it );
	
	void paint( QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget );
	void setSize( const QSize &size );
	
	void setCurrentParam( FilterWidget *f, ParameterWidget *pw, Parameter *p );
	void removeGraph();
	bool filterDeleted( Clip *c, Filter *f );
	void quitEditor();
	
protected:
	void mouseDoubleClickEvent( QGraphicsSceneMouseEvent *event );
	
private:
	void reset();
	void sendValue( double val );
	void propagateConstant( int index );
	
	Parameter *currentParam;
	ParameterWidget *currentParamWidget;
	FilterWidget *currentFilterWidget;
	QList<KeyItem*> keys;
	int currentKeyIndex;
	
private slots:
	void keyValueChanged( Parameter *p, QVariant val );
	
signals:
	void updateFrame();
};

#endif // ANIMITEM_H
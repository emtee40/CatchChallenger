#include "CCDialogTitle.hpp"
#include <QPainter>
#include <QEvent>
#include <QMouseEvent>
#include <iostream>

CCDialogTitle::CCDialogTitle(QGraphicsItem *parent) :
    QGraphicsItem(parent)
{
    textPath=nullptr;

    font=new QFont();
    font->setFamily("Comic Sans MS");
    font->setPixelSize(25);
    font->setStyleHint(QFont::Monospace);
    font->setBold(true);
    font->setStyleStrategy(QFont::ForceOutline);

    lastwidth=0;
    lastheight=0;
    cache=nullptr;
}

CCDialogTitle::~CCDialogTitle()
{
    if(textPath!=nullptr)
    {
        delete textPath;
        textPath=nullptr;
    }
    if(font!=nullptr)
    {
        delete font;
        font=nullptr;
    }
    if(cache!=nullptr)
    {
        delete cache;
        cache=nullptr;
    }
}

void CCDialogTitle::paint(QPainter *painter, const QStyleOptionGraphicsItem *, QWidget *)
{
    if(cache!=nullptr && !cache->isNull() && cache->width()==m_boundingRect.width() && cache->height()==m_boundingRect.height())
    {
        painter->drawPixmap(m_boundingRect.x(),m_boundingRect.y()+m_boundingRect.height()*0.1,m_boundingRect.width(),m_boundingRect.height(),*cache);
        return;
    }
    if(m_boundingRect.isEmpty())
        return;
    if(cache!=nullptr)
        delete cache;
    cache=new QPixmap();

    QImage image(m_boundingRect.width(),m_boundingRect.height(),QImage::Format_ARGB32);
    image.fill(Qt::transparent);
    QPainter paint;
    if(image.isNull())
        abort();
    paint.begin(&image);
    if(lastwidth!=m_boundingRect.width() || lastheight!=m_boundingRect.height())
    {
        updateTextPath();
        lastwidth=m_boundingRect.width();
        lastheight=m_boundingRect.height();
    }

    if(textPath!=nullptr && paint.isActive())
    {
        paint.setRenderHint(QPainter::Antialiasing);
        qreal penWidth=2.0;
        if(font->pixelSize()<=12)
            penWidth=0.7;
        else if(font->pixelSize()<=18)
            penWidth=1;
        else if(font->pixelSize()<=24)
            penWidth=1.5;
        //paint.setPen(QPen(QColor(255,0,0), penWidth, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
        QLinearGradient gradient( 0, 0, 0, m_boundingRect.height() );
        gradient.setColorAt( 0.25, "#e69900");
        gradient.setColorAt( 0.75, "#ffffff");
        paint.setPen(QPen(gradient, penWidth, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
        paint.setBrush(QColor(64,28,2));
        paint.drawPath(*textPath);
    }

    *cache=QPixmap::fromImage(image);

    painter->drawPixmap(m_boundingRect.x(),m_boundingRect.y()+m_boundingRect.height()*0.1,m_boundingRect.width(),m_boundingRect.height(),*cache);
}

QRectF CCDialogTitle::boundingRect() const
{
    return m_boundingRect;
}

void CCDialogTitle::setText(const QString &text)
{
    if(this->text==text)
        return;
    this->text=text;
    updateTextPath();
}

bool CCDialogTitle::setPixelSize(uint8_t size)
{
    if(font->pixelSize()==size)
        return true;
    font->setPixelSize(size);
    updateTextPath();
    return true;
}

void CCDialogTitle::updateTextPath()
{
    const QString &text=this->text;
    if(textPath!=nullptr)
        delete textPath;
    QPainterPath tempPath;
    tempPath.addText(0, 20, *font, text);
    QRectF rect=tempPath.boundingRect();
    textPath=new QPainterPath();
    m_boundingRect=QRectF(0,0,rect.width()+20,rect.height()+20);
    int newHeight=m_boundingRect.height();
    const int p=font->pixelSize();
    const int tempHeight=newHeight/2+p/2;
    const Qt::Alignment a=Qt::AlignCenter;//alignment();
    if(a.testFlag(Qt::AlignLeft))
        textPath->addText(0, tempHeight-1, *font, text);
    else if(a.testFlag(Qt::AlignRight))
        textPath->addText(m_boundingRect.width()-rect.width(), tempHeight-1, *font, text);
    else
        textPath->addText(m_boundingRect.width()/2-rect.width()/2, tempHeight-1, *font, text);
}

void CCDialogTitle::setFont(const QFont &font)
{
    *this->font=font;
}

QFont CCDialogTitle::getFont() const
{
    return *font;
}
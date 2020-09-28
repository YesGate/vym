#include "imageobj.h"
#include "mapobj.h"

#include <QDebug>
#include <QPainter>
#include <QStyleOptionGraphicsItem>
#include <QSvgGenerator>

/////////////////////////////////////////////////////////////////
// ImageObj	
/////////////////////////////////////////////////////////////////
ImageObj::ImageObj()
{
    //qDebug() << "Const ImageObj ()  this=" << this;
    init();
}

ImageObj::ImageObj( QGraphicsItem *parent) : QGraphicsItem (parent )
{
    //qDebug() << "Const ImageObj  this=" << this << "  parent= " << parent ;
    init();
}

ImageObj::~ImageObj()
{
    //qDebug() << "Destr ImageObj  this=" << this << "  imageType = " << imageType ;
    switch (imageType)
    {
        case ImageObj::SVG:
            if (svgItem) delete (svgItem);
            break;
        case ImageObj::Pixmap:
            if (pixmapItem) delete (pixmapItem);
            break;
        case ImageObj::ModifiedPixmap:  
            if (pixmapItem) delete (pixmapItem);
            if (originalPixmap) delete (originalPixmap);
            break;
        default: 
            qDebug() << "ImgObj::copy other->imageType undefined";    
            break;
    }
}

void ImageObj::init()
{
    // qDebug() << "Const ImageObj (scene)";
    hide();

    imageType = ImageObj::Undefined;
    svgItem        = NULL;
    pixmapItem     = NULL;
    originalPixmap = NULL;
    scaleFactor    = 1;
}

void ImageObj::copy(ImageObj* other)    
{
    prepareGeometryChange();
    if (imageType != ImageObj::Undefined)
        qWarning() << "ImageObj::copy into existing image of type " << imageType;

    switch (other->imageType)
    {
        case ImageObj::SVG:
            if (!other->svgPath.isEmpty())
            {
                load(other->svgPath);
                svgItem->setVisible( isVisible());
                imageType = ImageObj::SVG;
            } else 
                qWarning() << "ImgObj::copy svg: no svgPath available.";
            break;
        case ImageObj::Pixmap:
            pixmapItem = new QGraphicsPixmapItem();
            pixmapItem->setPixmap (other->pixmapItem->pixmap());
            pixmapItem->setParentItem (parentItem() );
            pixmapItem->setVisible( isVisible());
            imageType = ImageObj::Pixmap;
            break;
        case ImageObj::ModifiedPixmap:
            // create new pixmap?
            pixmapItem->setPixmap (other->pixmapItem->pixmap());
            pixmapItem->setParentItem (parentItem());
            pixmapItem->setVisible( isVisible());
            imageType = ImageObj::Pixmap;
            break;
        default: 
            qWarning() << "ImgObj::copy other->imageType undefined";   
            return;
            break;
    }
}

void ImageObj::setPos(const QPointF &pos)
{
    switch (imageType)
    {
        case ImageObj::SVG:
            svgItem->setPos(pos);
            break;
        case ImageObj::Pixmap:
            pixmapItem->setPos(pos);
            break;
        case ImageObj::ModifiedPixmap:
            pixmapItem->setPos(pos);
            break;
        default: 
            break;
    }
}

void ImageObj::setPos(const qreal &x, const qreal &y)
{
    setPos (QPointF (x, y));
}

void ImageObj::setZValue (qreal z)
{
    switch (imageType)
    {
        case ImageObj::SVG:
            svgItem->setZValue(z);
            break;
        case ImageObj::Pixmap:
        case ImageObj::ModifiedPixmap:
            pixmapItem->setZValue(z);
            break;
        default: 
            break;
    }
}
    

void ImageObj::setVisibility (bool v)   
{
    switch (imageType)
    {
        case ImageObj::SVG:
            v ? svgItem->show() : svgItem->hide();
            break;
        case ImageObj::Pixmap:
        case ImageObj::ModifiedPixmap:
            v ? pixmapItem->show() : pixmapItem->hide();
            break;
        default: 
            break;
    }
}

void  ImageObj::setScaleFactor(qreal f) 
{
    scaleFactor = f;
    switch (imageType)
    {
        case ImageObj::SVG:
            svgItem->setScale (f);
            break;
        case ImageObj::Pixmap: 
            if (f != 1 )
            {
                // create ModifiedPixmap
                originalPixmap = new QPixmap (pixmapItem->pixmap());
                imageType = ModifiedPixmap;

                setScaleFactor (f);
            }
            break;
        case ImageObj::ModifiedPixmap:  
            if (!originalPixmap)
            {
                qWarning() << "ImageObj::setScaleFactor   no originalPixmap!";
                return;
            }
            pixmapItem->setPixmap(
                    originalPixmap->scaled( 
                        originalPixmap->width() * f, 
                        originalPixmap->height() * f));
            break;
        default: 
            break;
    }
}

qreal  ImageObj::getScaleFactor()
{
    return scaleFactor;
}

QRectF ImageObj::boundingRect() const   
{
    switch (imageType)
    {
        case ImageObj::SVG:
            return QRectF(0, 0, 
                    svgItem->boundingRect().width() * scaleFactor, 
                    svgItem->boundingRect().height() * scaleFactor);
        case ImageObj::Pixmap:
            return pixmapItem->boundingRect();
        case ImageObj::ModifiedPixmap:
            return pixmapItem->boundingRect();
        default: 
            break;
    }
    return QRectF();
}

void ImageObj::paint (QPainter *painter, const QStyleOptionGraphicsItem
*sogi, QWidget *widget)     // FIXME-1 modPixmap not used?
{
    switch (imageType)
    {
        case ImageObj::SVG:
            svgItem->paint(painter, sogi, widget);
            break;
        case ImageObj::Pixmap:
            qDebug() << "IO::paint pm this=" << this  << "  pmitem=" << pixmapItem;
            pixmapItem->paint(painter, sogi, widget);
            break;
        default: 
            break;
    }
}

bool ImageObj::shareCashed(const QString &fn)
{
    if (save(fn))
    {
        svgPath = fn;
        return true;
    }
    
    return false;
}

QString ImageObj::getCashPath()
{
    return svgPath;
}

bool ImageObj::load (const QString &fn) 
{
    //qDebug() << "IO::load "  << fn;
    if (imageType != ImageObj::Undefined)
        qWarning() << "ImageObj::load (" << fn << ") into existing image of type " << imageType;

    if (fn.toLower().endsWith(".svg"))
    {
        svgItem = new QGraphicsSvgItem(fn);
        imageType = ImageObj::SVG;
        if (scene() ) scene()->addItem (svgItem);

        return true;
    } else
    {
        QPixmap pm;
        if (pm.load (fn))
        {
            prepareGeometryChange();

            pixmapItem = new QGraphicsPixmapItem (this);    // FIXME-1 existing pmi? 
            pixmapItem->setPixmap (pm);
            pixmapItem->setParentItem(parentItem() );
            imageType = ImageObj::Pixmap;

            return true;
        }
    }
    
    return false;
}

bool ImageObj::save(const QString &fn) 
{
    switch (imageType)
    {
        case ImageObj::SVG:
            if (svgItem)
            {
                //qDebug() << "IO::save svg" << fn; 
                QSvgGenerator generator;
                generator.setFileName(fn);
                // generator.setTitle(originalFileName);
                generator.setDescription("An SVG drawing created by vym - view your mind");
                QStyleOptionGraphicsItem qsogi;
                QPainter painter;
                painter.begin(&generator);
                svgItem->paint(&painter, &qsogi, NULL);
                painter.end();
            }
            return true;
            break;
        case ImageObj::Pixmap:
            //qDebug() << "IO::save pixmap " << fn;
            return pixmapItem->pixmap().save (fn, "PNG", 100);
            break;
        case ImageObj::ModifiedPixmap:
            //qDebug() << "IO::save modified pixmap " << fn;
            return originalPixmap->save (fn, "PNG", 100);
            break;
        default:
            break;
    }
    return false;
}

QString ImageObj::getExtension()
{
    QString s;
    switch (imageType)
    {
        case ImageObj::SVG:
            s = ".svg";
            break;
        case ImageObj::Pixmap:
        case ImageObj::ModifiedPixmap:
            s = ".png";
            break;
        default:
            break;
    }
    return s;
}

ImageObj::ImageType ImageObj::getType()
{
    return imageType;
}

QIcon ImageObj::getIcon()
{
    switch (imageType)
    {
        case ImageObj::SVG:
            return QPixmap(getCashPath());
            break;
        case ImageObj::Pixmap:
        case ImageObj::ModifiedPixmap:
            return QIcon(pixmapItem->pixmap() );
            break;
        default:
            break;
    }
    return QIcon(); 
}

// FIXME-1 is originalPixmap used after all?

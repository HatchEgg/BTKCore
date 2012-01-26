/* 
 * The Biomechanical ToolKit
 * Copyright (c) 2009-2012, Arnaud Barré
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 
 *     * Redistributions of source code must retain the above
 *       copyright notice, this list of conditions and the following
 *       disclaimer.
 *     * Redistributions in binary form must reproduce the above
 *       copyright notice, this list of conditions and the following
 *       disclaimer in the documentation and/or other materials
 *       provided with the distribution.
 *     * Neither the name(s) of the copyright holders nor the names
 *       of its contributors may be used to endorse or promote products
 *       derived from this software without specific prior written
 *       permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include "VideoWidget.h"
#include "Acquisition.h"
#include "UserDefined.h"
#include "LoggerMessage.h"

#include <QTreeWidgetItem>
#include <QPainter>
#include <QPaintEvent>
#include <QMessageBox>
#include <QVBoxLayout>
#include <QApplication>
#include <QStackedLayout>

VideoWidget::VideoWidget(QWidget* parent)
: QWidget(parent), mp_CurrentFrameFunctor()
{
  // Member(s)
  this->mp_Acquisition = 0;
  this->mp_Delays = 0;
  this->m_VideoId = -1;
  this->mp_MediaObject = new Phonon::MediaObject(this);
  this->mp_Video = new Phonon::VideoWidget(this);
  this->mp_Overlay = new VideoOverlayWidget(this);
  // Drag and drop
  this->setAcceptDrops(true);
  
  // UI
  // - Background color
  QPalette p(this->palette());
  p.setColor(QPalette::Background, Qt::black);
  this->setAutoFillBackground(true);
  this->setPalette(p);
  // - Layout
  QStackedLayout* videoStack = new QStackedLayout;
  videoStack->addWidget(this->mp_Video);
  videoStack->addWidget(this->mp_Overlay);
  videoStack->setStackingMode(QStackedLayout::StackAll);
  QVBoxLayout* layout = new QVBoxLayout;
  layout->setContentsMargins(0,0,0,0);
  layout->addLayout(videoStack);
  this->setLayout(layout);
  this->setVideoVisible(false);
  
  // Connect the media nodes
  Phonon::createPath(this->mp_MediaObject, this->mp_Video);
};

void VideoWidget::render()
{
  if (this->mp_Video->isVisible())
  {
    int frame = (*this->mp_CurrentFrameFunctor)();
    qint64 time = static_cast<qint64>(static_cast<double>(frame - 1) / this->mp_Acquisition->pointFrequency() * 1000.0) - this->mp_Delays->operator[](this->m_VideoId);
    if (time >= 0)
      this->mp_MediaObject->seek(time);
  }
};

void VideoWidget::show(bool s)
{
  if (!s) // Hidden
  {
    this->mp_MediaObject->clear();
    this->setVideoVisible(false);
  }
};

void VideoWidget::copy(VideoWidget* source)
{
  this->mp_Acquisition = source->mp_Acquisition;
  this->mp_Delays = source->mp_Delays;
  this->mp_CurrentFrameFunctor = source->mp_CurrentFrameFunctor;
};

void VideoWidget::dragEnterEvent(QDragEnterEvent* event)
{
  this->setVideoVisible(false);
  event->ignore();
  if (event->mimeData()->hasFormat("application/x-qabstractitemmodeldatalist"))
  {
    QTreeWidget* treeWidget = qobject_cast<QTreeWidget*>(event->source());
    if (treeWidget)
    {
      QList<QTreeWidgetItem*> selectedItems = treeWidget->selectedItems();
      for (QList<QTreeWidgetItem*>::const_iterator it = selectedItems.begin() ; it != selectedItems.end() ; ++it)
      {
        if ((*it)->type() != VideoType)
          return;
      }
      event->setDropAction(Qt::CopyAction); // To have the cross (+) symbol
      event->accept();
    }
  }
};

void VideoWidget::dragLeaveEvent(QDragLeaveEvent* event)
{
  if ((this->mp_MediaObject->currentSource().type() != Phonon::MediaSource::Empty) && (this->mp_MediaObject->currentSource().type() != Phonon::MediaSource::Invalid))
    this->setVideoVisible(true);
  this->QWidget::dragLeaveEvent(event);
}

void VideoWidget::dropEvent(QDropEvent* event)
{
  QMessageBox error(QMessageBox::Warning, "Video player", "Error when loading the video.", QMessageBox::Ok , this);
  #ifdef Q_OS_MAC
    error.setWindowFlags(Qt::Sheet);
    error.setWindowModality(Qt::WindowModal);
  #endif

  event->setDropAction(Qt::IgnoreAction); // Only to know which Video IDs were dropped.
  event->accept();
  QTreeWidget* treeWidget = qobject_cast<QTreeWidget*>(event->source());
  QTreeWidgetItem* video = treeWidget->selectedItems().at(0);
  int id = video->data(0,VideoId).toInt();
  QString videoFilePath = this->mp_Acquisition->videoPath(id) + "/" + this->mp_Acquisition->videoFilename(id);
  if (this->mp_MediaObject->currentSource().fileName().compare(videoFilePath) == 0) // same file?
    return;
  else if (this->mp_Acquisition->videoFilename(id).isEmpty())
  {
    LOG_CRITICAL("Error when loading the video file: File not found.");
    error.setInformativeText("File not found.<br/>"
                             "<nobr>Video(s) must be in the same folder than the acquisition.</nobr>");
    error.exec();
    return;
  }
  LOG_INFO("Loading video: " + this->mp_Acquisition->videoFilename(id));
  QApplication::setOverrideCursor(Qt::WaitCursor);
  this->mp_MediaObject->setCurrentSource(videoFilePath);
  QApplication::restoreOverrideCursor();
  if (this->mp_MediaObject->state() == Phonon::ErrorState)
  {
    LOG_CRITICAL("Error when loading the video file: " + this->mp_MediaObject->errorString());
    error.setInformativeText("<nobr>Have you the right video codec installed?</nobr>");
    error.exec();
  }
  else
  {
    this->m_VideoId = id;
    this->setVideoVisible(true);
    this->render();
  }
};

void VideoWidget::paintEvent(QPaintEvent* event)
{
  if (!this->mp_Video->isVisible())
  {
    QString str = "Drop a video";
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setPen(Qt::lightGray);
    QFont f = this->font();
    f.setPointSize(32);
    painter.setFont(f);
    QFontMetrics fm = painter.fontMetrics();
    QRect rect = fm.boundingRect(str);
    QPoint center = this->geometry().center();
    painter.drawText(center - rect.center(), str);
    painter.setPen(QPen(Qt::lightGray, 1.75, Qt::DashLine));
    qreal side = static_cast<qreal>(qMin(this->width() / 2, this->height() / 2));
    side = qMax(side, rect.width() + 30.0); // 15 px on each side
    painter.drawRoundedRect(QRectF(center - QPointF(side, side) / 2.0, QSizeF(side, side)), 25.0, 25.0);
  }
  else
    this->QWidget::paintEvent(event);
};

void VideoWidget::setVideoVisible(bool v)
{
  this->mp_Video->setVisible(v);
  this->mp_Overlay->setVisible(v);
};

// ----------------------------------------------------------------------------

VideoOverlayWidget::VideoOverlayWidget(QWidget* parent)
: QWidget(parent)
{
  this->setAttribute(Qt::WA_NoSystemBackground);
};
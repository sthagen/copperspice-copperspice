/***********************************************************************
*
* Copyright (c) 2012-2025 Barbara Geller
* Copyright (c) 2012-2025 Ansel Sermersheim
*
* Copyright (c) 2015 The Qt Company Ltd.
* Copyright (c) 2012-2016 Digia Plc and/or its subsidiary(-ies).
* Copyright (c) 2008-2012 Nokia Corporation and/or its subsidiary(-ies).
*
* This file is part of CopperSpice.
*
* CopperSpice is free software. You can redistribute it and/or
* modify it under the terms of the GNU Lesser General Public License
* version 2.1 as published by the Free Software Foundation.
*
* CopperSpice is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
*
* https://www.gnu.org/licenses/
*
***********************************************************************/

#include <qstyleoption.h>

#include <qapplication.h>
#include <qdebug.h>
#include <qmath.h>

#ifdef Q_OS_DARWIN
# include <qmacstyle.h>
#endif

QStyleOption::QStyleOption(int version, int type)
   : m_styleVersion(version), m_styleType(type), state(QStyle::State_None),
     direction(QApplication::layoutDirection()), fontMetrics(QFont()), styleObject(nullptr)
{
}

QStyleOption::~QStyleOption()
{
}

void QStyleOption::initFrom(const QWidget *widget)
{
   QWidget *window = widget->window();
   state = QStyle::State_None;

   if (widget->isEnabled()) {
      state |= QStyle::State_Enabled;
   }

   if (widget->hasFocus()) {
      state |= QStyle::State_HasFocus;
   }

   if (window->testAttribute(Qt::WA_KeyboardFocusChange)) {
      state |= QStyle::State_KeyboardFocusChange;
   }

   if (widget->underMouse()) {
      state |= QStyle::State_MouseOver;
   }

   if (window->isActiveWindow()) {
      state |= QStyle::State_Active;
   }

   if (widget->isWindow()) {
      state |= QStyle::State_Window;
   }

#if defined(Q_OS_DARWIN) && ! defined(QT_NO_STYLE_MAC)
   switch (QMacStyle::widgetSizePolicy(widget)) {
      case QMacStyle::SizeSmall:
         state |= QStyle::State_Small;
         break;

      case QMacStyle::SizeMini:
         state |= QStyle::State_Mini;
         break;

      default:
         break;
   }
#endif

#ifdef QT_KEYPAD_NAVIGATION
   if (widget->hasEditFocus()) {
      state |= QStyle::State_HasEditFocus;
   }
#endif

   direction   = widget->layoutDirection();
   rect        = widget->rect();
   palette     = widget->palette();
   fontMetrics = widget->fontMetrics();
   styleObject = const_cast<QWidget *>(widget);
}

QStyleOption::QStyleOption(const QStyleOption &other)
   : m_styleVersion(Version), m_styleType(Type), state(other.state),
     direction(other.direction), rect(other.rect), fontMetrics(other.fontMetrics),
     palette(other.palette), styleObject(other.styleObject)
{
}

QStyleOption &QStyleOption::operator=(const QStyleOption &other)
{
   state       = other.state;
   direction   = other.direction;
   rect        = other.rect;
   fontMetrics = other.fontMetrics;
   palette     = other.palette;
   styleObject = other.styleObject;

   return *this;
}

QStyleOptionFocusRect::QStyleOptionFocusRect()
   : QStyleOption(Version, SO_FocusRect)
{
   state |= QStyle::State_KeyboardFocusChange; // assume we had one, will be corrected in initFrom()
}

QStyleOptionFocusRect::QStyleOptionFocusRect(int version)
   : QStyleOption(version, SO_FocusRect)
{
   state |= QStyle::State_KeyboardFocusChange;  // assume we had one, will be corrected in initFrom()
}

QStyleOptionFrame::QStyleOptionFrame()
   : QStyleOption(Version, SO_Frame), lineWidth(0), midLineWidth(0),
     features(None), frameShape(QFrame::NoFrame)
{
}
QStyleOptionFrame::QStyleOptionFrame(int version)
   : QStyleOption(version, SO_Frame), lineWidth(0), midLineWidth(0),
     features(None), frameShape(QFrame::NoFrame)
{
}

QStyleOptionGroupBox::QStyleOptionGroupBox()
   : QStyleOptionComplex(Version, Type), lineWidth(0), midLineWidth(0),
     textAlignment(Qt::AlignLeft), features(QStyleOptionFrame::None)
{
}

QStyleOptionGroupBox::QStyleOptionGroupBox(int version)
   : QStyleOptionComplex(version, Type), lineWidth(0), midLineWidth(0),
     textAlignment(Qt::AlignLeft),features(QStyleOptionFrame::None)

{
}

QStyleOptionHeader::QStyleOptionHeader()
   : QStyleOption(QStyleOptionHeader::Version, SO_Header), section(0),
     textAlignment(Qt::AlignLeft), iconAlignment(Qt::AlignLeft), orientation(Qt::Horizontal),
     position(QStyleOptionHeader::Beginning), selectedPosition(QStyleOptionHeader::NotAdjacent), sortIndicator(None)
{
}

QStyleOptionHeader::QStyleOptionHeader(int version)
   : QStyleOption(version, SO_Header), section(0),
     textAlignment(Qt::AlignLeft), iconAlignment(Qt::AlignLeft), orientation(Qt::Horizontal),
     position(QStyleOptionHeader::Beginning), selectedPosition(QStyleOptionHeader::NotAdjacent), sortIndicator(None)
{
}

QStyleOptionButton::QStyleOptionButton()
   : QStyleOption(QStyleOptionButton::Version, SO_Button), features(None)
{
}

QStyleOptionButton::QStyleOptionButton(int version)
   : QStyleOption(version, SO_Button), features(None)
{
}

#ifndef QT_NO_TOOLBAR

QStyleOptionToolBar::QStyleOptionToolBar()
   : QStyleOption(Version, SO_ToolBar), lineWidth(0), midLineWidth(0),
     positionOfLine(OnlyOne), positionWithinLine(OnlyOne), toolBarArea(Qt::TopToolBarArea),
     features(None)
{
}

QStyleOptionToolBar::QStyleOptionToolBar(int version)
   : QStyleOption(version, SO_ToolBar), lineWidth(0), midLineWidth(0),
     positionOfLine(OnlyOne), positionWithinLine(OnlyOne), toolBarArea(Qt::TopToolBarArea),
     features(None)
{
}

#endif

#ifndef QT_NO_TABBAR

QStyleOptionTab::QStyleOptionTab()
   : QStyleOption(QStyleOptionTab::Version, SO_Tab), row(0), documentMode(false),
     position(Beginning), selectedPosition(NotAdjacent), cornerWidgets(QStyleOptionTab::NoCornerWidgets),
     features(QStyleOptionTab::None), shape(QTabBar::RoundedNorth)
{
}

QStyleOptionTab::QStyleOptionTab(int version)
   : QStyleOption(version, SO_Tab), row(0), documentMode(false),
     position(Beginning), selectedPosition(NotAdjacent), cornerWidgets(QStyleOptionTab::NoCornerWidgets),
     features(QStyleOptionTab::None), shape(QTabBar::RoundedNorth)
{
}

#endif // QT_NO_TABBAR

QStyleOptionProgressBar::QStyleOptionProgressBar()
   : QStyleOption(QStyleOptionProgressBar::Version, SO_ProgressBar),
     minimum(0), maximum(0), progress(0),
     bottomToTop(false), invertedAppearance(false), textVisible(false),
     textAlignment(Qt::AlignLeft), orientation(Qt::Horizontal)
{
}

QStyleOptionProgressBar::QStyleOptionProgressBar(int version)
   : QStyleOption(version, SO_ProgressBar), minimum(0), maximum(0), progress(0),
     bottomToTop(false), invertedAppearance(false), textVisible(false),
     textAlignment(Qt::AlignLeft), orientation(Qt::Horizontal)
{
}

QStyleOptionMenuItem::QStyleOptionMenuItem()
   : QStyleOption(QStyleOptionMenuItem::Version, SO_MenuItem), maxIconWidth(0), tabWidth(0),
     checked(false), menuHasCheckableItems(true), menuItemType(Normal), checkType(NotCheckable)
{
}

QStyleOptionMenuItem::QStyleOptionMenuItem(int version)
   : QStyleOption(version, SO_MenuItem), maxIconWidth(0), tabWidth(0),
     checked(false), menuHasCheckableItems(true), menuItemType(Normal), checkType(NotCheckable)
{
}

QStyleOptionComplex::QStyleOptionComplex(int version, int type)
   : QStyleOption(version, type), subControls(QStyle::SC_All), activeSubControls(QStyle::SC_None)
{
}

#ifndef QT_NO_SLIDER

QStyleOptionSlider::QStyleOptionSlider()
   : QStyleOptionComplex(Version, SO_Slider), minimum(0), maximum(0), tickInterval(0),
     sliderPosition(0), sliderValue(0), singleStep(0), pageStep(0),
     upsideDown(false), dialWrapping(false), notchTarget(0.0),
     orientation(Qt::Horizontal), tickPosition(QSlider::NoTicks)
{
}

QStyleOptionSlider::QStyleOptionSlider(int version)
   : QStyleOptionComplex(version, SO_Slider), minimum(0), maximum(0), tickInterval(0),
     sliderPosition(0), sliderValue(0), singleStep(0), pageStep(0),
     upsideDown(false), dialWrapping(false), notchTarget(0.0),
      orientation(Qt::Horizontal), tickPosition(QSlider::NoTicks)
{
}

#endif // QT_NO_SLIDER

#ifndef QT_NO_SPINBOX

QStyleOptionSpinBox::QStyleOptionSpinBox()
   : QStyleOptionComplex(Version, SO_SpinBox), frame(false),
     buttonSymbols(QAbstractSpinBox::UpDownArrows), stepEnabled(QAbstractSpinBox::StepNone)
{
}

QStyleOptionSpinBox::QStyleOptionSpinBox(int version)
   : QStyleOptionComplex(version, SO_SpinBox), frame(false),
     buttonSymbols(QAbstractSpinBox::UpDownArrows), stepEnabled(QAbstractSpinBox::StepNone)
{
}

#endif

QStyleOptionDockWidget::QStyleOptionDockWidget()
   : QStyleOption(Version, SO_DockWidget), closable(false),
     movable(false), floatable(false), verticalTitleBar(false)
{
}

QStyleOptionDockWidget::QStyleOptionDockWidget(int version)
   : QStyleOption(version, SO_DockWidget), closable(false),
     movable(false), floatable(false), verticalTitleBar(false)
{
}

QStyleOptionToolButton::QStyleOptionToolButton()
   : QStyleOptionComplex(Version, SO_ToolButton), arrowType(Qt::DownArrow),
     toolButtonStyle(Qt::ToolButtonIconOnly), features(None)
{
}

QStyleOptionToolButton::QStyleOptionToolButton(int version)
   : QStyleOptionComplex(version, SO_ToolButton), arrowType(Qt::DownArrow),
     toolButtonStyle(Qt::ToolButtonIconOnly), features(None)
{
}

QStyleOptionComboBox::QStyleOptionComboBox()
   : QStyleOptionComplex(Version, SO_ComboBox), editable(false), frame(true)
{
}

QStyleOptionComboBox::QStyleOptionComboBox(int version)
   : QStyleOptionComplex(version, SO_ComboBox), editable(false), frame(true)
{
}

QStyleOptionToolBox::QStyleOptionToolBox()
   : QStyleOption(Version, SO_ToolBox), position(Beginning), selectedPosition(NotAdjacent)
{
}

QStyleOptionToolBox::QStyleOptionToolBox(int version)
   : QStyleOption(version, SO_ToolBox), position(Beginning), selectedPosition(NotAdjacent)
{
}

#ifndef QT_NO_RUBBERBAND

QStyleOptionRubberBand::QStyleOptionRubberBand()
   : QStyleOption(Version, SO_RubberBand), opaque(false), shape(QRubberBand::Line)
{
}

QStyleOptionRubberBand::QStyleOptionRubberBand(int version)
   : QStyleOption(version, SO_RubberBand), opaque(false), shape(QRubberBand::Line)
{
}

#endif

QStyleOptionTitleBar::QStyleOptionTitleBar()
   : QStyleOptionComplex(Version, SO_TitleBar), titleBarState(0), titleBarFlags(Qt::EmptyFlag)
{
}

QStyleOptionTitleBar::QStyleOptionTitleBar(int version)
   : QStyleOptionComplex(version, SO_TitleBar), titleBarState(0), titleBarFlags(Qt::EmptyFlag)
{
}

#ifndef QT_NO_ITEMVIEWS

QStyleOptionViewItem::QStyleOptionViewItem()
   : QStyleOption(Version, SO_ViewItem), showDecorationSelected(false),
     displayAlignment(Qt::AlignLeft), decorationAlignment(Qt::AlignLeft),
     textElideMode(Qt::ElideMiddle), checkState(Qt::Unchecked),
     decorationPosition(Left), features(None), viewItemPosition(QStyleOptionViewItem::Invalid), widget(nullptr)
{
}

QStyleOptionViewItem::QStyleOptionViewItem(int version)
   : QStyleOption(version, SO_ViewItem), showDecorationSelected(false),
     displayAlignment(Qt::AlignLeft), decorationAlignment(Qt::AlignLeft),
     textElideMode(Qt::ElideMiddle), checkState(Qt::Unchecked),
     decorationPosition(Left), features(None), viewItemPosition(QStyleOptionViewItem::Invalid), widget(nullptr)
{
}

#endif

#ifndef QT_NO_TABWIDGET

QStyleOptionTabWidgetFrame::QStyleOptionTabWidgetFrame()
   : QStyleOption(Version, SO_TabWidgetFrame), lineWidth(0), midLineWidth(0),
     shape(QTabBar::RoundedNorth)
{
}

QStyleOptionTabWidgetFrame::QStyleOptionTabWidgetFrame(int version)
   : QStyleOption(version, SO_TabWidgetFrame), lineWidth(0), midLineWidth(0),
     shape(QTabBar::RoundedNorth)
{
}

#endif

#ifndef QT_NO_TABBAR

QStyleOptionTabBarBase::QStyleOptionTabBarBase()
   : QStyleOption(Version, SO_TabBarBase), documentMode(false), shape(QTabBar::RoundedNorth)
{
}

QStyleOptionTabBarBase::QStyleOptionTabBarBase(int version)
   : QStyleOption(version, SO_TabBarBase), documentMode(false), shape(QTabBar::RoundedNorth)
{
}

#endif

#ifndef QT_NO_SIZEGRIP

QStyleOptionSizeGrip::QStyleOptionSizeGrip()
   : QStyleOptionComplex(Version, Type), corner(Qt::BottomRightCorner)
{
}

QStyleOptionSizeGrip::QStyleOptionSizeGrip(int version)
   : QStyleOptionComplex(version, Type), corner(Qt::BottomRightCorner)
{
}

#endif

QStyleOptionGraphicsItem::QStyleOptionGraphicsItem()
   : QStyleOption(Version, Type), levelOfDetail(1)
{
}

QStyleOptionGraphicsItem::QStyleOptionGraphicsItem(int version)
   : QStyleOption(version, Type), levelOfDetail(1)
{
}

qreal QStyleOptionGraphicsItem::levelOfDetailFromTransform(const QTransform &worldTransform)
{
   if (worldTransform.type() <= QTransform::TxTranslate) {
      return 1;   // Translation only? The LOD is 1.
   }

   // Two unit vectors.
   QLineF v1(0, 0, 1, 0);
   QLineF v2(0, 0, 0, 1);

   // LOD is the transformed area of a 1x1 rectangle.
   return qSqrt(worldTransform.map(v1).length() * worldTransform.map(v2).length());
}

QStyleHintReturn::QStyleHintReturn(int version, int type)
   : m_styleHintVersion(version), m_styleHintType(type)
{
}

QStyleHintReturn::~QStyleHintReturn()
{
}

QStyleHintReturnMask::QStyleHintReturnMask() : QStyleHintReturn(Version, Type)
{
}

QStyleHintReturnMask::~QStyleHintReturnMask()
{
}

QStyleHintReturnVariant::QStyleHintReturnVariant() : QStyleHintReturn(Version, Type)
{
}

QStyleHintReturnVariant::~QStyleHintReturnVariant()
{
}

QDebug operator<<(QDebug debug, const QStyleOption::OptionType &optionType)
{
#if defined(CS_SHOW_DEBUG_GUI_STYLES)
   switch (optionType) {
      case QStyleOption::SO_Default:
         debug << "SO_Default";
         break;

      case QStyleOption::SO_FocusRect:
         debug << "SO_FocusRect";
         break;

      case QStyleOption::SO_Button:
         debug << "SO_Button";
         break;

      case QStyleOption::SO_Tab:
         debug << "SO_Tab";
         break;

      case QStyleOption::SO_MenuItem:
         debug << "SO_MenuItem";
         break;

      case QStyleOption::SO_Frame:
         debug << "SO_Frame";
         break;

      case QStyleOption::SO_ProgressBar:
         debug << "SO_ProgressBar";
         break;

      case QStyleOption::SO_ToolBox:
         debug << "SO_ToolBox";
         break;

      case QStyleOption::SO_Header:
         debug << "SO_Header";
         break;

      case QStyleOption::SO_DockWidget:
         debug << "SO_DockWidget";
         break;

      case QStyleOption::SO_ViewItem:
         debug << "SO_ViewItem";
         break;

      case QStyleOption::SO_TabWidgetFrame:
         debug << "SO_TabWidgetFrame";
         break;

      case QStyleOption::SO_TabBarBase:
         debug << "SO_TabBarBase";
         break;

      case QStyleOption::SO_RubberBand:
         debug << "SO_RubberBand";
         break;

      case QStyleOption::SO_Complex:
         debug << "SO_Complex";
         break;

      case QStyleOption::SO_Slider:
         debug << "SO_Slider";
         break;

      case QStyleOption::SO_SpinBox:
         debug << "SO_SpinBox";
         break;

      case QStyleOption::SO_ToolButton:
         debug << "SO_ToolButton";
         break;

      case QStyleOption::SO_ComboBox:
         debug << "SO_ComboBox";
         break;

      case QStyleOption::SO_TitleBar:
         debug << "SO_TitleBar";
         break;

      case QStyleOption::SO_CustomBase:
         debug << "SO_CustomBase";
         break;

      case QStyleOption::SO_GroupBox:
         debug << "SO_GroupBox";
         break;

      case QStyleOption::SO_ToolBar:
         debug << "SO_ToolBar";
         break;

      case QStyleOption::SO_ComplexCustomBase:
         debug << "SO_ComplexCustomBase";
         break;

      case QStyleOption::SO_SizeGrip:
         debug << "SO_SizeGrip";
         break;

      case QStyleOption::SO_GraphicsItem:
         debug << "SO_GraphicsItem";
         break;
   }

#else
   (void) optionType;

#endif

   return debug;
}

QDebug operator<<(QDebug debug, const QStyleOption &option)
{
#if defined(CS_SHOW_DEBUG_GUI_STYLES)
   debug << "QStyleOption(";
   debug << QStyleOption::OptionType(option.m_styleType);
   debug << ',' << (option.direction == Qt::RightToLeft ? "RightToLeft" : "LeftToRight");
   debug << ',' << option.state;
   debug << ',' << option.rect;
   debug << ',' << option.styleObject;
   debug << ')';

#else
   (void) option;

#endif

   return debug;
}

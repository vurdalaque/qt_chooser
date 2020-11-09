// vim: ft=css
#include "helper.h"

const QString DesktopWidget::g_defaultStyleSheet = R"""(
* {
 	color: white;
	/* background-color: rgb(50, 50, 50); */
	background-color: rgb(34, 34, 34);
	border: none;
}

/* -------------------------------------------------------------------------------- */
/* Qt's symlink widget */

#LinkFrame {
	border: none;
	background-color: rgb(34, 34, 34);
	height: 50px;
}

#LinkFrame:disabled {
	border: none;
	background-color: rgb(50, 50, 50);
}

/* qlabel styling */

#LinkFrame > QLabel
{
	font-size: 8pt;
	color: white;
	background-color: transparent;
	border: none;
}

#LinkFrame > QLabel:disabled
{
	color: rgb(255, 230, 230);
	background-color: rgb(50, 50, 50);
}

/* combobox styling */

#LinkFrame > QComboBox
{
	border: 1px solid white;
	border-radius: 8px;
	background-color: transparent;
	color: white;
	selection-color: rgb(150, 150, 150);
	font-size: 8pt;
	height: 18px;
	padding-left: 10px;
}

#LinkFrame > QComboBox QAbstractItemView
{
	border: none;
	selection-background-color: rgb(130, 130, 130);
	selection-color: rgb(34, 34, 34);
	color: white;
	padding-left: 5px;
	padding-top: 2px;
	padding-bottom: 2px;
}

#LinkFrame > QComboBox::drop-down
{
	border:none;
	border-radius: 6px;
	background-color: transparent;
	margin-top: 3px;
	margin-right: 3px;
	margin-bottom: 3px;
}

#LinkFrame > QComboBox::down-arrow
{
	border: 1px solid white;
	border-radius: 6px;
	color: white;
}

#LinkFrame > QComboBox:disabled
{
	border: none;
	background-color: transparent;
}

#LinkFrame > QComboBox::down-arrow:disabled
{
	border: none;
	background-color: transparent;
}

/* -------------------------------------------------------------------------------- */
/* MoveArea styling */

#MoveFrame
{
	border: none;
	background-color: rgb(34, 34, 34);
	width: 9px;
	height: 100%;
}

#MoveFrame > #LineFrame
{
	border: none;
	background-color: transparent;
	padding-left: 1px;
}

#MoveFrame > #LineFrame > #line_1,#line_2,#line_3,#line_4,#line_5,#line_6
{
	width: 1px;
	background-color: rgb(40,40,40);
	margin-left: 1px;
	padding-right: 2px;
}

#MoveFrame > #LineFrame > #line_1:hover,#line_2:hover,#line_3:hover,#line_4:hover,#line_5:hover,#line_6:hover
{
	background-color: rgb(34,34,34);
}


#MoveFrame > QToolButton
{
	background-color: transparent;
	qproperty-text: '✕';
	padding-left: 1px;
	padding-bottom: 2px;
}

#MoveFrame > QToolButton:hover
{
	background-color: rgb(130,130,130);
}

/* -------------------------------------------------------------------------------- */
/* ServiceControl styling */

#MainServiceFrame
{
	background-color: rgb(34,34,34);
	width: 50px;
	max-width: 50px;
	height: 50px;
	max-height: 50px;
}

#ServiceFrame
{
	background-color: transparent;
}

StateLabel
{
	margin: 3px;
	min-width: 44px;
	min-height: 44px;
	border: 1px solid white;
	border-radius: 20px;
	font-size: 12pt;
	font-family: Consolas;
	font-weight: 600;
}

StateLabel > QToolTip
{
	background-color: rgb(34, 34, 34);
	border: 1px solid white;
}

/* not found\initialized */
StateLabel[state="0"]
{
	color: rgb(34, 34, 34);
	background-color:  qradialgradient(spread:pad, cx:0.5, cy:0.5, radius:0.5, fx:0.5, fy:0.5,stop:0.1 #323232, stop:0.8 #1e1e1e);
	border: 1px solid rgb(34, 34, 34);
}

/* not running */
StateLabel[state="1"]
{
	color: rgb(34, 34, 34);
	background-color:  qradialgradient(spread:pad, cx:0.5, cy:0.5, radius:0.5, fx:0.5, fy:0.5,stop:0.1 #903e3e, stop:0.8 #1e1e1e);
	border: 1px solid rgb(34, 34, 34);
}

/* running */
StateLabel[state="2"]
{
	color: white;
	background-color:  qradialgradient(spread:pad, cx:0.5, cy:0.5, radius:0.5, fx:0.5, fy:0.5,stop:0.0 #3e803e, stop:0.8 #1e1e1e);
	border: 1px solid rgb(34, 34, 34);
}

/* paused */
StateLabel[state="3"]
{
	color: white;
	background-color:  qradialgradient(spread:pad, cx:0.5, cy:0.5, radius:0.5, fx:0.5, fy:0.5,stop:0.1 #3e9ea0, stop:0.8 #1e1e1e);
	border: 1px solid rgb(34, 34, 34);
}

/* -------------------------------------------------------------------------------- */
/* Process Control styling */

#proctree
{
	border: none;
}

#proclist
{
	background-color: rgb(34, 34, 34);
	padding-top: 2px;
	padding-bottom: 2px;
	border: none;
}

QListWidget::item
{
	background-color: transparent;
	height: 24px;
}

#ProcessMainFrame
{
	background-color: rgb(34, 34, 34);
	border: 1px solid transparent;
}

#ProcessWidgetFrame
{
	background-color: transparent;
	padding-left: 4px;
	padding-right: 4px;
	border: 1px solid transparent;
}

#ProcessWidgetFrame > #processName
{
	background-color: transparent;
}

#ProcessWidgetFrame:hover
{
	border: 1px solid rgb(130, 130, 130);
}

#ProcessWidgetFrame > #ico
{
	background-color: transparent;
	margin-right: 6px;
}

#ProcessWidgetFrame > #closeProcess
{
	min-width: 16px;
	min-height: 16px;
	background-color: transparent;
	font-size: 12pt;
	qproperty-text: '卐';
	color: lightblue;
}

#ProcessWidgetFrame > #attachDebugger
{
	min-width: 16px;
	min-height: 16px;
	background-color: transparent;
	font-size: 12pt;
	qproperty-text: '☭';
	margin-right: 4px;
	color: red;
}

#ProcessWidget > #ProcessWidgetFrame > #closeProcess:hover
{
	background-color: rgb(130, 130, 130);
}

#ProcessWidget > #ProcessWidgetFrame > #attachDebugger:hover
{
	background-color: rgb(130, 130, 130);
}

MemoryWatchLabel
{
	background-color: transparent;
	color: rgb(130, 130, 130);
}

MemoryWatchLabel[state="0"]
{
	color: rgb(130, 130, 130);
}

MemoryWatchLabel[state="1"]
{
	color: green;
}

MemoryWatchLabel[state="-1"]
{
	color: red;
}

/* -------------------------------------------------------------------------------- */
/* ScrollBar Control styling */

QScrollBar {
	background: transparent;
	border: none;
}

QScrollBar:vertical {
	width: 4px;
	border: none;
	background: none;
}

QScrollBar::handle:vertical {
	background: rgb(34, 34, 34);
	border: none;
	/* border: 1px solid white; */
}

QScrollBar:up-arrow {
	image: none;
	background: none;
	background-color: transparent;
	border: none;
}

QScrollBar:down-arrow {
	image: none;
	background: none;
	background-color: transparent;
	border: none;
}

QScrollBar::sub-line {
	background-color: transparent;
	border: none;
}

QScrollBar::add-line {
	background-color: transparent;
	border: none;
}


)""";

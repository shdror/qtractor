// qtractorSpinBox.cpp
//
/****************************************************************************
   Copyright (C) 2005-2013, rncbc aka Rui Nuno Capela. All rights reserved.

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License
   as published by the Free Software Foundation; either version 2
   of the License, or (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License along
   with this program; if not, write to the Free Software Foundation, Inc.,
   51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.

*****************************************************************************/

#include "qtractorAbout.h"
#include "qtractorSpinBox.h"

#include <QLineEdit>
#include <QLocale>

#include <math.h>


//----------------------------------------------------------------------------
// qtractorTimeSpinBox -- A time-scale formatted spin-box widget.

// Constructor.
qtractorTimeSpinBox::qtractorTimeSpinBox ( QWidget *pParent )
	: QAbstractSpinBox(pParent)
{
	m_pTimeScale    = NULL;
	m_iValue        = 0;
	m_iMinimumValue = 0;
	m_iMaximumValue = 0;
	m_iDeltaValue   = 0;
	m_bDeltaValue   = false;

#if QT_VERSION >= 0x040200
	QAbstractSpinBox::setAccelerated(true);
#endif

	QObject::connect(this,
		SIGNAL(editingFinished()),
		SLOT(editingFinishedSlot()));
	QObject::connect(QAbstractSpinBox::lineEdit(),
		SIGNAL(textChanged(const QString&)),
		SLOT(valueChangedSlot(const QString&)));
}


// Destructor.
qtractorTimeSpinBox::~qtractorTimeSpinBox (void)
{
}


// Mark that we got actual value.
void qtractorTimeSpinBox::showEvent ( QShowEvent */*pShowEvent*/ )
{
	QLineEdit *pLineEdit = QAbstractSpinBox::lineEdit();
	bool bBlockSignals = pLineEdit->blockSignals(true);
	pLineEdit->setText(textFromValue(m_iValue));
	QAbstractSpinBox::interpretText();
	pLineEdit->blockSignals(bBlockSignals);
}


// Time-scale accessors.
void qtractorTimeSpinBox::setTimeScale ( qtractorTimeScale *pTimeScale )
{
	m_pTimeScale = pTimeScale;
}

qtractorTimeScale *qtractorTimeSpinBox::timeScale (void) const
{
	return m_pTimeScale;
}


// Display-format accessors.
qtractorTimeScale::DisplayFormat qtractorTimeSpinBox::displayFormat (void) const
{
	return (m_pTimeScale
		? m_pTimeScale->displayFormat()
		: qtractorTimeScale::Frames);
}

void qtractorTimeSpinBox::updateDisplayFormat (void)
{
	setValue(m_iValue);
}


// Nominal value (in frames) accessors.
void qtractorTimeSpinBox::setValue ( unsigned long iValue, bool bNotifyChange )
{
	QLineEdit *pLineEdit = QAbstractSpinBox::lineEdit();
	int iCursorPos = pLineEdit->cursorPosition();

	if (iValue < m_iMinimumValue)
		iValue = m_iMinimumValue;
	if (iValue > m_iMaximumValue && m_iMaximumValue > m_iMinimumValue)
		iValue = m_iMaximumValue;
	
	bool bValueChanged = (iValue != m_iValue);

	m_iValue = iValue;

	if (QAbstractSpinBox::isVisible()) {
		bool bBlockSignals = pLineEdit->blockSignals(true);
		pLineEdit->setText(textFromValue(iValue));
		QAbstractSpinBox::interpretText();
		if (bNotifyChange && bValueChanged)
			emit valueChanged(iValue);
		pLineEdit->blockSignals(bBlockSignals);
	}

	pLineEdit->setCursorPosition(iCursorPos);
}

unsigned long qtractorTimeSpinBox::value (void) const
{
	return m_iValue;
}


// Minimum value (in frames) accessors.
void qtractorTimeSpinBox::setMinimum ( unsigned long iMinimum )
{
	m_iMinimumValue = iMinimum;
}

unsigned long qtractorTimeSpinBox::minimum (void) const
{
	return m_iMinimumValue;
}


// Maximum value (in frames) accessors.
void qtractorTimeSpinBox::setMaximum ( unsigned long iMaximum )
{
	m_iMaximumValue = iMaximum;
}

unsigned long qtractorTimeSpinBox::maximum (void) const
{
	return m_iMaximumValue;
}


// Differential value mode (BBT format only) accessor.
void qtractorTimeSpinBox::setDeltaValue ( bool bDeltaValue, unsigned long iDeltaValue )
{
	m_bDeltaValue = bDeltaValue;
	m_iDeltaValue = iDeltaValue;
}

bool qtractorTimeSpinBox::isDeltaValue (void) const
{
	return m_bDeltaValue;
}

unsigned long qtractorTimeSpinBox::deltaValue (void) const
{
	return m_iDeltaValue;
}


// Inherited/override methods.
QValidator::State qtractorTimeSpinBox::validate ( QString& sText, int& iPos ) const
{
#ifdef CONFIG_DEBUG_0
	qDebug("qtractorTimeSpinBox[%p]::validate(\"%s\",%d)",
		this, sText.toUtf8().constData(), iPos);
#endif

	if (iPos == 0)
		return QValidator::Acceptable;

	const QChar& ch = sText[iPos - 1];
	if (m_pTimeScale) {
		switch (m_pTimeScale->displayFormat()) {
		case qtractorTimeScale::Time:
			if (ch == ':')
				return QValidator::Acceptable;
			// Fall thru.
		case qtractorTimeScale::BBT:
			if (ch == '.')
				return QValidator::Acceptable;
			// Fall thru.
		case qtractorTimeScale::Frames:
		default:
			if (ch.isDigit())
				return QValidator::Acceptable;
			break;
		}
	}

	return QValidator::Invalid;
}


void qtractorTimeSpinBox::fixup ( QString& sText ) const
{
#ifdef CONFIG_DEBUG_0
	qDebug("qtractorTimeSpinBox[%p]::fixup(\"%s\")",
		this, sText.toUtf8().constData());
#endif

	sText = textFromValue(m_iValue);
}


void qtractorTimeSpinBox::stepBy ( int iSteps )
{
#ifdef CONFIG_DEBUG_0
	qDebug("qtractorTimeSpinBox[%p]::stepBy(%d)", this, iSteps);
#endif

	QLineEdit *pLineEdit = QAbstractSpinBox::lineEdit();
	int iCursorPos = pLineEdit->cursorPosition();
	
	long iValue = long(value());

	if (m_pTimeScale) {
		switch (m_pTimeScale->displayFormat()) {
		case qtractorTimeScale::BBT: {
			qtractorTimeScale::Cursor cursor(m_pTimeScale);
			qtractorTimeScale::Node *pNode = cursor.seekFrame(iValue);
			unsigned long iFrame = pNode->frame;
			const QString& sText = pLineEdit->text();
			int iPos = sText.section('.', 0, 0).length() + 1;
			if (iCursorPos < iPos)
				iFrame = pNode->frameFromBar(pNode->bar + 1);
			else if (iCursorPos < iPos + sText.section('.', 1, 1).length() + 1)
				iFrame = pNode->frameFromBeat(pNode->beat + 1);
			else
				iFrame = pNode->frameFromTick(pNode->tick + 1);
			iSteps *= int(iFrame - pNode->frame);
			break;
		}
		case qtractorTimeScale::Time: {
			const QString& sText = pLineEdit->text();
			int iPos = sText.section(':', 0, 0).length() + 1;
			if (iCursorPos < iPos)
				iSteps *= int(3600 * m_pTimeScale->sampleRate());
			else if (iCursorPos < iPos + sText.section(':', 1, 1).length() + 1)
				iSteps *= int(60 * m_pTimeScale->sampleRate());
			else
				iSteps *= int(m_pTimeScale->sampleRate());
			break;
		}
		case qtractorTimeScale::Frames:
		default:
			break;
		}
	}

	iValue += iSteps;
	if (iValue < 0)
		iValue = 0;
	setValue(iValue);

	pLineEdit->setCursorPosition(iCursorPos);
}


QAbstractSpinBox::StepEnabled qtractorTimeSpinBox::stepEnabled (void) const
{
	StepEnabled flags = StepUpEnabled;
	if (value() > 0)
		flags |= StepDownEnabled;
	return flags;
}


// Value/text format converters.
unsigned long qtractorTimeSpinBox::valueFromText (void)
{
	return valueFromText(QAbstractSpinBox::text());
}

unsigned long qtractorTimeSpinBox::valueFromText ( const QString& sText ) const
{
	return (m_pTimeScale
		? m_pTimeScale->frameFromText(sText, m_bDeltaValue, m_iDeltaValue)
		: sText.toULong());
}

QString qtractorTimeSpinBox::textFromValue ( unsigned long iValue ) const
{
	return (m_pTimeScale
		? (m_bDeltaValue
			? m_pTimeScale->textFromFrame(m_iDeltaValue, true, iValue)
			: m_pTimeScale->textFromFrame(iValue))
		: QString::number(iValue));
}


// Pseudo-fixup slot.
void qtractorTimeSpinBox::editingFinishedSlot (void)
{
#ifdef CONFIG_DEBUG_0
	qDebug("qtractorTimeSpinBox[%p]::editingFinishedSlot()", this);
#endif

	// Kind of final fixup.
	setValue(valueFromText());
}


// Textual value change notification.
void qtractorTimeSpinBox::valueChangedSlot ( const QString& sText )
{
#ifdef CONFIG_DEBUG_0
	qDebug("qtractorTimeSpinBox[%p]::valueChangedSlot(\"%s\")",
		this, sText.toUtf8().constData());
#endif

	// Forward this...
	emit valueChanged(sText);
}


//----------------------------------------------------------------------------
// qtractorTempoSpinBox -- A time-scale formatted spin-box widget.

// Constructor.
qtractorTempoSpinBox::qtractorTempoSpinBox ( QWidget *pParent )
	: QAbstractSpinBox(pParent), m_fTempo(120.0f),
		m_iBeatsPerBar(4), m_iBeatDivisor(2), m_iValueChanged(0)
{
#if QT_VERSION >= 0x040200
	QAbstractSpinBox::setAccelerated(true);
#endif

	QObject::connect(this,
		SIGNAL(editingFinished()),
		SLOT(editingFinishedSlot()));
	QObject::connect(QAbstractSpinBox::lineEdit(),
		SIGNAL(textChanged(const QString&)),
		SLOT(valueChangedSlot(const QString&)));
}


// Destructor.
qtractorTempoSpinBox::~qtractorTempoSpinBox (void)
{
}


// Mark that we got actual value.
void qtractorTempoSpinBox::showEvent ( QShowEvent */*pShowEvent*/ )
{
	QLineEdit *pLineEdit = QAbstractSpinBox::lineEdit();
	bool bBlockSignals = pLineEdit->blockSignals(true);
	pLineEdit->setText(textFromValue(m_fTempo, m_iBeatsPerBar, m_iBeatDivisor));
	pLineEdit->blockSignals(bBlockSignals);
//	QAbstractSpinBox::interpretText();
}


// Nominal tempo value (BPM) accessors.
void qtractorTempoSpinBox::setTempo (
	float fTempo, bool bNotifyChange )
{
	if (updateValue(fTempo, m_iBeatsPerBar, m_iBeatDivisor, bNotifyChange))
		updateText();
}


float qtractorTempoSpinBox::tempo (void) const
{
	return m_fTempo;
}


// Nominal time-signature numerator (beats/bar) accessors.
void qtractorTempoSpinBox::setBeatsPerBar (
	unsigned short iBeatsPerBar, bool bNotifyChange )
{
	if (updateValue(m_fTempo, iBeatsPerBar, m_iBeatDivisor, bNotifyChange))
		updateText();
}


unsigned short qtractorTempoSpinBox::beatsPerBar (void) const
{
	return m_iBeatsPerBar;
}


// Nominal time-signature denominator (beat-divisor) accessors.
void qtractorTempoSpinBox::setBeatDivisor (
	unsigned short iBeatDivisor, bool bNotifyChange )
{
	if (updateValue(m_fTempo, m_iBeatsPerBar, iBeatDivisor, bNotifyChange))
		updateText();
}


unsigned short qtractorTempoSpinBox::beatDivisor (void) const
{
	return m_iBeatDivisor;
}


// Common value setler.
bool qtractorTempoSpinBox::updateValue ( float fTempo,
	unsigned short iBeatsPerBar, unsigned short iBeatDivisor,
	bool bNotifyChange )
{
	if (fTempo < 1.0f)
		fTempo = 1.0f;
	if (fTempo > 1000.0f)
		fTempo = 1000.0f;

	if (iBeatsPerBar < 2)
		iBeatsPerBar = 2;
	if (iBeatsPerBar > 128)
		iBeatsPerBar = 128;

	if (iBeatDivisor < 1)
		iBeatDivisor = 1;
	if (iBeatDivisor > 8)
		iBeatDivisor = 8;

	if (::fabs(m_fTempo - fTempo) > 0.05f) {
		m_fTempo = fTempo;
		++m_iValueChanged;
	}

	if (m_iBeatsPerBar != iBeatsPerBar) {
		m_iBeatsPerBar  = iBeatsPerBar;
		++m_iValueChanged;
	}
	if (m_iBeatDivisor != iBeatDivisor) {
		m_iBeatDivisor  = iBeatDivisor;
		++m_iValueChanged;
	}

	int iValueChanged = m_iValueChanged;

	if (bNotifyChange && m_iValueChanged > 0) {
		emit valueChanged(m_fTempo, m_iBeatsPerBar, m_iBeatDivisor);
		m_iValueChanged = 0;
	}

	return (iValueChanged > 0);
}


void qtractorTempoSpinBox::updateText (void)
{
	if (QAbstractSpinBox::isVisible()) {
		QLineEdit *pLineEdit = QAbstractSpinBox::lineEdit();
		bool bBlockSignals = pLineEdit->blockSignals(true);
		int iCursorPos = pLineEdit->cursorPosition();
		pLineEdit->setText(textFromValue(
			m_fTempo, m_iBeatsPerBar, m_iBeatDivisor));
	//	QAbstractSpinBox::interpretText();
		pLineEdit->setCursorPosition(iCursorPos);
		pLineEdit->blockSignals(bBlockSignals);
	}
}


// Inherited/override methods.
QValidator::State qtractorTempoSpinBox::validate ( QString& sText, int& iPos ) const
{
#ifdef CONFIG_DEBUG_0
	qDebug("qtractorTempoSpinBox[%p]::validate(\"%s\",%d)",
		this, sText.toUtf8().constData(), iPos);
#endif

	if (iPos == 0)
		return QValidator::Acceptable;

	const QChar& ch = sText[iPos - 1];
	const QChar& decp = QLocale().decimalPoint();
	if (ch == decp || ch == '/' || ch == ' ' || ch.isDigit())
		return QValidator::Acceptable;
	else
		return QValidator::Invalid;
}


void qtractorTempoSpinBox::fixup ( QString& sText ) const
{
#ifdef CONFIG_DEBUG_0
	qDebug("qtractorTempoSpinBox[%p]::fixup(\"%s\")",
		this, sText.toUtf8().constData());
#endif

	sText = textFromValue(m_fTempo, m_iBeatsPerBar, m_iBeatDivisor);
}


void qtractorTempoSpinBox::stepBy ( int iSteps )
{
#ifdef CONFIG_DEBUG_0
	qDebug("qtractorTempoSpinBox[%p]::stepBy(%d)", this, iSteps);
#endif

	QLineEdit *pLineEdit = QAbstractSpinBox::lineEdit();
	int iCursorPos = pLineEdit->cursorPosition();
	const QString& sText = pLineEdit->text();
	if (iCursorPos < sText.section(' ', 0, 0).length() + 1) {
		const QChar& decp = QLocale().decimalPoint();
		if (iCursorPos > sText.section(decp, 0, 0).length())
			setTempo(tempo() + 0.1f * float(iSteps));
		else
			setTempo(tempo() + float(iSteps));
	}
	else
	if (iCursorPos > sText.section('/', 0, 0).length())
		setBeatDivisor(int(beatDivisor()) + iSteps);
	else
		setBeatsPerBar(int(beatsPerBar()) + iSteps);
}


QAbstractSpinBox::StepEnabled qtractorTempoSpinBox::stepEnabled (void) const
{
	StepEnabled flags = StepNone;
	float fTempo = tempo();
	unsigned short iBeatsPerBar = beatsPerBar();
	unsigned short iBeatDivisor = beatDivisor();
	if (fTempo > 1.0f && iBeatsPerBar > 2 && iBeatDivisor > 1)
		flags |= StepDownEnabled;
	if (fTempo < 1000.0f && iBeatsPerBar < 128 && iBeatDivisor < 8)
		flags |= StepUpEnabled;
	return flags;
}


// Value/text format converters.
float qtractorTempoSpinBox::tempoFromText ( const QString& sText ) const
{
	float fTempo = sText.section(' ', 0, 0).toFloat();
	return (fTempo >= 1.0f ? fTempo : m_fTempo);
}


unsigned short qtractorTempoSpinBox::beatsPerBarFromText ( const QString& sText) const
{
	unsigned short iBeatsPerBar
		= sText.section(' ', 1, 1).section('/', 0, 0).toUShort();
	return (iBeatsPerBar >= 2 ? iBeatsPerBar : m_iBeatsPerBar);
}


unsigned short qtractorTempoSpinBox::beatDivisorFromText ( const QString& sText) const
{
	unsigned short iBeatDivisor = 0;
	unsigned short i = sText.section(' ', 1, 1).section('/', 1, 1).toUShort();
	while (i > 1) {	++iBeatDivisor;	i >>= 1; }
	return (iBeatDivisor >= 1 ? iBeatDivisor : m_iBeatDivisor);
}


QString qtractorTempoSpinBox::textFromValue ( float fTempo,
	unsigned short iBeatsPerBar, unsigned short iBeatDivisor) const
{
	return QString("%1 %2/%3")
		.arg(fTempo, 0, 'f', 1)
		.arg(iBeatsPerBar)
		.arg(1 << iBeatDivisor);
}


// Textual value change notification.
void qtractorTempoSpinBox::valueChangedSlot ( const QString& sText )
{
#ifdef CONFIG_DEBUG_0
	qDebug("qtractorTempoSpinBox[%p]::valueChangedSlot(\"%s\")",
		this, sText.toUtf8().constData());
#endif

	// Kind of interim fixup.
	if (updateValue(tempoFromText(sText),
			beatsPerBarFromText(sText),
			beatDivisorFromText(sText), false)) {
		// Just forward this one...
		emit valueChanged(sText);
	}
}


// Final pseudo-fixup slot.
void qtractorTempoSpinBox::editingFinishedSlot (void)
{
#ifdef CONFIG_DEBUG_0
	qDebug("qtractorTempoSpinBox[%p]::editingFinishedSlot()", this);
#endif

	const QString& sText = QAbstractSpinBox::text();

	// Kind of final fixup.
	if (updateValue(tempoFromText(sText),
			beatsPerBarFromText(sText),
			beatDivisorFromText(sText), true)) {
		// Rephrase text display...
		updateText();
	}
}


// end of qtractorTimeSpinBox.cpp

/***************************************************************************
 *   Copyright (C) 2008-2009 by Heiko Koehn                                     *
 *   KoehnHeiko@googlemail.com                                             *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/
//#define NDEBUG
#include <assert.h>
#include <QDebug>
#include <stdio.h>
#include <iostream>
#include <string>
#include <deque>
#include <queue>
#include "TBuffer.h"
#include "Host.h"
#include "TConsole.h"


using namespace std;

class Host;


TChar::TChar()
{
    fgR = 255;
    fgG = 255;
    fgB = 255;
    bgR = 0;
    bgG = 0;
    bgB = 0;
    italics = false;
    bold = false;
    underline = false;
}

TChar::TChar( int fR, int fG, int fB, int bR, int bG, int bB, bool b, bool i, bool u )
: fgR(fR)
, fgG(fG)
, fgB(fB)
, bgR(bR)
, bgG(bG)
, bgB(bB)
, bold(b)
, italics(i)
, underline(u)
{
}

TChar::TChar( Host * pH )
{
    if( pH )
    {
        fgR = pH->mFgColor.red();
        fgG = pH->mFgColor.green();
        fgB = pH->mFgColor.blue();
        bgR = pH->mBgColor.red();
        bgG = pH->mBgColor.green();
        bgB = pH->mBgColor.blue();
    }
    else
    {
        
        fgR = 255;
        fgG = 255;
        fgB = 255;
        bgR = 0;
        bgG = 0;
        bgB = 0;
    }
    italics = false;
    bold = false;
    underline = false;    
}

bool TChar::operator==( const TChar & c )
{
    if( fgR != c.fgR ) return false;
    if( fgG != c.fgG ) return false;
    if( fgB != c.fgB ) return false;
    if( bgR != c.bgR ) return false;
    if( bgB != c.bgB ) return false;
    if( bold != c.bold ) return false;
    if( italics != c.italics ) return false;
    if( underline != c.underline ) return false;
    return true;
}

TChar::TChar( const TChar & copy )
{
    fgR = copy.fgR;
    fgG = copy.fgG;
    fgB = copy.fgB;
    bgR = copy.bgR;
    bgG = copy.bgG;
    bgB = copy.bgB;
    italics = copy.italics;
    bold = copy.bold;
    underline = copy.underline;     
}


TBuffer::TBuffer( Host * pH )
: mLinesLimit( 10000 )
, mBatchDeleteSize( 100 )
, mpHost( pH )
, mCursorMoved( false )
, mWrapAt( 99999999 )
, mWrapIndent( 0 )
, mFormatSequenceRest( QString("") )
, mBold( false )
, mItalics( false )
, mUnderline( false )
, mFgColorCode( 0 )
, mBgColorCode( 0 )
, mBlack             ( pH->mBlack )
, mLightBlack        ( pH->mLightBlack )
, mRed               ( pH->mRed )
, mLightRed          ( pH->mLightRed )
, mLightGreen        ( pH->mLightGreen )
, mGreen             ( pH->mGreen )
, mLightBlue         ( pH->mLightBlue )
, mBlue              ( pH->mBlue )
, mLightYellow       ( pH->mLightYellow )
, mYellow            ( pH->mYellow )
, mLightCyan         ( pH->mLightCyan )
, mCyan              ( pH->mCyan )
, mLightMagenta      ( pH->mLightMagenta )
, mMagenta           ( pH->mMagenta )
, mLightWhite        ( pH->mLightWhite )
, mWhite             ( pH->mWhite )
, mFgColor           ( pH->mFgColor )
, mBgColor           ( pH->mBgColor )
, mUntriggered( 0 )
, gotESC( false )
, gotHeader( false )
, codeRet( 0 )
{   
    clear();
    newLines = 0;
    mLastLine = 0;
    updateColors();
}

void TBuffer::setBufferSize( int s, int batch )
{
    if( s < 100 ) s = 100;
    if( batch >= s ) batch = s/10;
    mLinesLimit = s;
    mBatchDeleteSize = batch;
}

void TBuffer::resetFontSpecs()
{
    fgColorR = mFgColorR;
    fgColorG = mFgColorG;
    fgColorB = mFgColorB;
    bgColorR = mBgColorR;
    bgColorG = mBgColorG;
    bgColorB = mBgColorB;
    mBold = false;
    mItalics = false;
    mUnderline = false;
}

void TBuffer::updateColors()
{
    Host * pH = mpHost;
    mBlack = pH->mBlack;
    mBlackR=mBlack.red();
    mBlackG=mBlack.green();
    mBlackB=mBlack.blue();
    mLightBlack = pH->mLightBlack;
    mLightBlackR = mLightBlack.red();
    mLightBlackG=mLightBlack.green();
    mLightBlackB=mLightBlack.blue();
    mRed = pH->mRed;
    mRedR=mRed.red();
    mRedG=mRed.green();
    mRedB=mRed.blue();
    mLightRed = pH->mLightRed;
    mLightRedR=mLightRed.red();
    mLightRedG=mLightRed.green();
    mLightRedB=mLightRed.blue();
    mLightGreen = pH->mLightGreen;
    mLightGreenR=mLightGreen.red();
    mLightGreenG=mLightGreen.green();
    mLightGreenB=mLightGreen.blue();
    mGreen = pH->mGreen;
    mGreenR=mGreen.red();
    mGreenG=mGreen.green();
    mGreenB=mGreen.blue();
    mLightBlue = pH->mLightBlue;
    mLightBlueR=mLightBlue.red();
    mLightBlueG=mLightBlue.green();
    mLightBlueB=mLightBlue.blue();
    mBlue = pH->mBlue;
    mBlueR=mBlue.red();
    mBlueG=mBlue.green();
    mBlueB=mBlue.blue();
    mLightYellow  = pH->mLightYellow;
    mLightYellowR=mLightYellow.red();
    mLightYellowG=mLightYellow.green();
    mLightYellowB=mLightYellow.blue();
    mYellow = pH->mYellow;
    mYellowR=mYellow.red();
    mYellowG=mYellow.green();
    mYellowB=mYellow.blue();
    mLightCyan = pH->mLightCyan;
    mLightCyanR=mLightCyan.red();
    mLightCyanG=mLightCyan.green();
    mLightCyanB=mLightCyan.blue();
    mCyan = pH->mCyan;
    mCyanR=mCyan.red();
    mCyanG=mCyan.green();
    mCyanB=mCyan.blue();
    mLightMagenta = pH->mLightMagenta;
    mLightMagentaR=mLightMagenta.red();
    mLightMagentaG=mLightMagenta.green();
    mLightMagentaB=mLightMagenta.blue();
    mMagenta = pH->mMagenta;
    mMagentaR=mMagenta.red();
    mMagentaG=mMagenta.green();
    mMagentaB=mMagenta.blue();
    mLightWhite = pH->mLightWhite;
    mLightWhiteR=mLightWhite.red();
    mLightWhiteG=mLightWhite.green();
    mLightWhiteB=mLightWhite.blue();
    mWhite = pH->mWhite;
    mWhiteR=mWhite.red();
    mWhiteG=mWhite.green();
    mWhiteB=mWhite.blue();
    mFgColor = pH->mFgColor;
    mBgColor = pH->mBgColor;
    mFgColorR = mFgColor.red();
    fgColorR = mFgColorR;
    fgColorLightR = fgColorR;
    mFgColorG = mFgColor.green();
    fgColorG = mFgColorG;
    fgColorLightG = fgColorG;
    mFgColorB = mFgColor.blue();
    fgColorB = mFgColorB;
    fgColorLightB = fgColorB;
    mBgColorR = mBgColor.red();
    bgColorR = mBgColorR;
    mBgColorG = mBgColor.green();
    bgColorG = mBgColorG;
    mBgColorB = mBgColor.blue();
    bgColorB = mBgColorB;

}

QPoint & TBuffer::getEndPos()
{
    int x = 0;
    int y = 0;
    if( buffer.size() > 0 )
        y = buffer.size()-1;
    if( buffer[y].size() > 0 )
        x = buffer[y].size()-1;
    QPoint P_end( x, y );
    return P_end;
}

int TBuffer::getLastLineNumber()
{
    if( static_cast<int>(buffer.size()) > 0 )
    {
        return static_cast<int>(buffer.size())-1;
    }
    else
    {
        return -1;
    }
}

int speedTP;

inline void TBuffer::set_text_properties(int tag)
{
    if( mWaitingForHighColorCode )
    {
        if( mHighColorModeForeground )
        {
            if( tag < 16 )
            {
                mHighColorModeForeground = false;
                mWaitingForHighColorCode = false;
                mIsHighColorMode = false;
                goto NORMAL_ANSI_COLOR_TAG;
            }
            if( tag < 232 )
            {
                tag-=16; // because color 1-15 behave like normal ANSI colors
                // 6x6 RGB color space
                int r = tag / 36;
                int g = (tag-(r*36)) / 6;
                int b = (tag-(r*36))-(g*6);
                fgColorR = r*42;
                fgColorG = g*42;
                fgColorB = b*42;
            }
            else
            {
                // black + 23 tone grayscale from dark to light gray
                tag -= 232;
                fgColorR = tag*10;
                fgColorG = tag*10;
                fgColorB = tag*10;
            }
            mHighColorModeForeground = false;
            mWaitingForHighColorCode = false;
            mIsHighColorMode = false;

            return;
        }
        if( mHighColorModeBackground )
        {
            if( tag < 16 )
            {
                mHighColorModeBackground = false;
                mWaitingForHighColorCode = false;
                mIsHighColorMode = false;
                goto NORMAL_ANSI_COLOR_TAG;
            }
            if( tag < 232 )
            {
                tag-=16;
                int r = tag / 36;
                int g = (tag-(r*36)) / 6;
                int b = (tag-(r*36))-(g*6);
                bgColorR = r*42;
                bgColorG = g*42;
                bgColorB = b*42;
            }
            else
            {
                // black + 23 tone grayscale from dark to light gray
                tag -= 232;
                fgColorR = tag*10;
                fgColorG = tag*10;
                fgColorB = tag*10;
            }
            mHighColorModeBackground = false;
            mWaitingForHighColorCode = false;
            mIsHighColorMode = false;

            return;
        }
    }

    if( tag == 38 )
    {
        mIsHighColorMode = true;
        mHighColorModeForeground = true;
        return;
    }
    if( tag == 48 )
    {
        mIsHighColorMode = true;
        mHighColorModeBackground = true;
    }
    if( ( mIsHighColorMode ) && ( tag == 5 ) )
    {
        mWaitingForHighColorCode = true;
        return;
    }

    // we are dealing with standard ANSI colors
NORMAL_ANSI_COLOR_TAG:

    switch( tag )
    {
    case 0:
        mHighColorModeForeground = false;
        mHighColorModeBackground = false;
        mWaitingForHighColorCode = false;
        mIsHighColorMode = false;
        mIsDefaultColor = true;
        resetFontSpecs();
        break;
    case 1:
        mBold = true;
        break;
    case 2:
        mBold = false;
        break;
    case 3:
        mItalics = true;
        break;
    case 4:
        mUnderline = true;
    case 5:
        break; //FIXME support blinking
    case 6:
        break; //FIXME support fast blinking
    case 7:
        break; //FIXME support inverse
    case 9:
        break; //FIXME support strikethrough
    case 22:
        mBold = false;
        break;
    case 23:
        mItalics = false;
        break;
    case 24:
        mUnderline = false;
        break;
    case 27:
        break; //FIXME inverse off
    case 29:
        break; //FIXME
    case 30:
        fgColorR = mBlackR;
        fgColorG = mBlackG;
        fgColorB = mBlackB;
        fgColorLightR = mLightBlackR;
        fgColorLightG = mLightBlackG;
        fgColorLightB = mLightBlackB;
        mIsDefaultColor = false;
        break;
    case 31:
        fgColorR = mRedR;
        fgColorG = mRedR;
        fgColorB = mRedB;
        fgColorLightR = mLightRedR;
        fgColorLightG = mLightRedG;
        fgColorLightB = mLightRedB;
        mIsDefaultColor = false;
        break;
    case 32:
        fgColorR = mGreenR;
        fgColorG = mGreenG;
        fgColorB = mGreenB;
        fgColorLightR = mLightGreenR;
        fgColorLightG = mLightGreenR;
        fgColorLightB = mLightGreenB;
        mIsDefaultColor = false;
        break;
    case 33:
        fgColorR = mYellowR;
        fgColorG = mYellowG;
        fgColorB = mYellowB;
        fgColorLightR = mLightYellowR;
        fgColorLightG = mLightYellowG;
        fgColorLightB = mLightYellowB;
        mIsDefaultColor = false;
        break;
    case 34:
        fgColorR = mBlueR;
        fgColorG = mBlueG;
        fgColorB = mBlueB;
        fgColorLightR = mLightBlueR;
        fgColorLightG = mLightBlueG;
        fgColorLightB = mLightBlueB;
        mIsDefaultColor = false;
        break;
    case 35:
        fgColorR = mMagentaR;
        fgColorG=mMagentaG;
        fgColorB=mMagentaB;
        fgColorLightR=mLightMagentaR;
        fgColorLightG=mLightMagentaG;
        fgColorLightB=mLightMagentaB;
        mIsDefaultColor = false;
        break;
    case 36:
        fgColorR = mCyanR;
        fgColorG = mCyanG;
        fgColorB = mCyanB;
        fgColorLightR = mLightCyanR;
        fgColorLightG = mLightCyanG;
        fgColorLightB = mLightCyanB;
        mIsDefaultColor = false;
        break;
    case 37:
        fgColorR = mWhiteR;
        fgColorG = mWhiteG;
        fgColorB = mWhiteB;
        fgColorLightR = mLightWhiteR;
        fgColorLightG = mLightWhiteG;
        fgColorLightB = mLightWhiteB;
        mIsDefaultColor = false;
        break;
    case 39:
        bgColorR = mBgColorR;
        bgColorG = mBgColorG;
        bgColorB = mBgColorB;
        break;
    case 40:
        bgColorR = mBlackR;
        bgColorG = mBlackG;
        bgColorB = mBlackB;
        break;
    case 41:
        bgColorR = mRedR;
        bgColorG = mRedG;
        bgColorB = mRedB;
        break;
    case 42:
        bgColorR = mGreenR;
        bgColorG = mGreenG;
        bgColorB = mGreenB;
        break;
    case 43:
        bgColorR = mYellowR;
        bgColorG = mYellowG;
        bgColorB = mYellowB;
        break;
    case 44:
        bgColorR = mBlueR;
        bgColorG = mBlueG;
        bgColorB = mBlueB;
        break;
    case 45:
        bgColorR = mMagentaR;
        bgColorG = mMagentaG;
        bgColorB = mMagentaB;
        break;
    case 46:
        bgColorR = mCyanR;
        bgColorG = mCyanG;
        bgColorB = mCyanB;
        break;
    case 47:
        bgColorR = mWhiteR;
        bgColorG = mWhiteG;
        bgColorB = mWhiteB;
        break;
    };
}


/* ANSI color codes: sequence = "ESCAPE + [ code_1; ... ; code_n m"
      -----------------------------------------
      0 reset
      1 intensity bold on
      2 intensity faint
      3 italics on
      4 underline on
      5 blink slow
      6 blink fast
      7 inverse on
      9 strikethrough
      22 intensity normal (not bold, not faint)
      23 italics off
      24 underline off
      27 inverse off
      28 strikethrough off
      30 fg black
      31 fg red
      32 fg green
      33 fg yellow
      34 fg blue
      35 fg magenta
      36 fg cyan
      37 fg white
      39 bg default white
      40 bg black
      41 bg red
      42 bg green
      43 bg yellow
      44 bg blue
      45 bg magenta
      46 bg cyan
      47 bg white
      49 bg black

      sequences for 256 Color support:
      38;5;0-256 foreground color
      48;5;0-256 background color */

int speedSequencer;
int speedAppend;

const QChar cESC = '\033';
const QString cDigit = "0123456789";
int msLength;
int msPos;
int mCode[3];

inline int TBuffer::lookupColor( QString & s, int pos )
{
    int ret = 0;
    QString code;

    msPos = pos;
    while( msPos < msLength )
    {
        int digit = cDigit.indexOf( s[msPos] );
        if( digit > -1 )
        {
            code.append( s[msPos] );
            msPos++;
            continue;
        }
        else if( s[msPos] == ';' )
        {
            ret++;
            mCode[ret] = code.toInt();
            msPos++;
            code.clear();
            continue;
        }
        else if( s[msPos] == 'm' )
        {
            ret++;
            mCode[ret] = code.toInt();
            msPos++;
            return ret;
        }
        else if( s[msPos] == '[' )
        {
            msPos++;
            continue;
        }
        else
        {
            msPos++;
            qDebug()<<"unrecognized sequence:<"<<s.mid(pos,msPos-pos)<<">";
            return 0; // unbekannte sequenz
        }
    }
    msPos = pos-1;
    return -1; // unbeendete sequenz
}


void TBuffer::translateToPlainText( std::string & s )
{
    speedAppend = 0;
    speedTP = 0;
    int numCodes=0;
    speedSequencer = 0;
    int last = buffer.size()-1;

    mUntriggered = lineBuffer.size()-1;
    msLength = s.length();
    mFormatSequenceRest="";
    int sequence_begin = 0;
    int sequence_end = 0;
    int msPos = 0;
    int highCode = 0;
    int numNull = 0;

    QString packetTime = (QTime::currentTime()).toString("hh:mm:ss.zzz") + "   ";
    bool firstChar = (lineBuffer.back().size() == 0); //FIXME
    if( msLength < 1 ) return;

    while( true )
    {
        DECODE:
        if( msPos >= msLength )
        {
            return;
        }
        const char & ch = s[msPos];
        if( ch == '\033' )
        {
            gotESC = true;
            msPos++;
            continue;
        }
        if( gotESC )
        {
            if( ch == '[' )
            {
                gotHeader = true;
                gotESC = false;
                msPos++;
                continue;
            }
        }

        if( gotHeader )
        {

            while( msPos < msLength )
            {
                QChar ch2 = s[msPos];
                int digit = cDigit.indexOf( ch2 );
                if( digit > -1 )
                {
                    code.append( ch2 );
                    msPos++;
                    continue;
                }
                else if( ch2 == ';' )
                {
                    codeRet++;
                    mCode[codeRet] = code.toInt();
                    code.clear();
                    msPos++;
                    continue;
                }
                else if( ch2 == 'm' )
                {
                    codeRet++;
                    mCode[codeRet] = code.toInt();
                    code.clear();
                    gotHeader = false;
                    msPos++;

                    numCodes += codeRet;
                    for( int i=1; i<codeRet+1; i++ )
                    {
                        int tag = mCode[i];
                        if( mWaitingForHighColorCode )
                        {
                            if( mHighColorModeForeground )
                            {
                                if( tag < 16 )
                                {
                                    mHighColorModeForeground = false;
                                    mWaitingForHighColorCode = false;
                                    mIsHighColorMode = false;
                                    goto NORMAL_ANSI_COLOR_TAG;
                                }
                                if( tag < 232 )
                                {
                                    tag-=16; // because color 1-15 behave like normal ANSI colors
                                    // 6x6 RGB color space
                                    int r = tag / 36;
                                    int g = (tag-(r*36)) / 6;
                                    int b = (tag-(r*36))-(g*6);
                                    fgColorR = r*42;
                                    fgColorG = g*42;
                                    fgColorB = b*42;
                                }
                                else
                                {
                                    // black + 23 tone grayscale from dark to light gray
                                    tag -= 232;
                                    fgColorR = tag*10;
                                    fgColorG = tag*10;
                                    fgColorB = tag*10;
                                }
                                mHighColorModeForeground = false;
                                mWaitingForHighColorCode = false;
                                mIsHighColorMode = false;
                                continue;
                            }
                            if( mHighColorModeBackground )
                            {
                                if( tag < 16 )
                                {
                                    mHighColorModeBackground = false;
                                    mWaitingForHighColorCode = false;
                                    mIsHighColorMode = false;
                                    goto NORMAL_ANSI_COLOR_TAG;
                                }
                                if( tag < 232 )
                                {
                                    tag-=16;
                                    int r = tag / 36;
                                    int g = (tag-(r*36)) / 6;
                                    int b = (tag-(r*36))-(g*6);
                                    bgColorR = r*42;
                                    bgColorG = g*42;
                                    bgColorB = b*42;
                                }
                                else
                                {
                                    // black + 23 tone grayscale from dark to light gray
                                    tag -= 232;
                                    fgColorR = tag*10;
                                    fgColorG = tag*10;
                                    fgColorB = tag*10;
                                }
                                mHighColorModeBackground = false;
                                mWaitingForHighColorCode = false;
                                mIsHighColorMode = false;
                                continue;
                            }
                        }

                        if( tag == 38 )
                        {
                            mIsHighColorMode = true;
                            mHighColorModeForeground = true;
                            continue;
                        }
                        if( tag == 48 )
                        {
                            mIsHighColorMode = true;
                            mHighColorModeBackground = true;
                        }
                        if( ( mIsHighColorMode ) && ( tag == 5 ) )
                        {
                            mWaitingForHighColorCode = true;
                            continue;
                        }

                        // we are dealing with standard ANSI colors
                    NORMAL_ANSI_COLOR_TAG:

                        switch( tag )
                        {
                        case 0:
                            mHighColorModeForeground = false;
                            mHighColorModeBackground = false;
                            mWaitingForHighColorCode = false;
                            mIsHighColorMode = false;
                            mIsDefaultColor = true;
                            fgColorR = mFgColorR;
                            fgColorG = mFgColorG;
                            fgColorB = mFgColorB;
                            bgColorR = mBgColorR;
                            bgColorG = mBgColorG;
                            bgColorB = mBgColorB;
                            mBold = false;
                            mItalics = false;
                            mUnderline = false;
                            break;
                        case 1:
                            mBold = true;
                            break;
                        case 2:
                            mBold = false;
                            break;
                        case 3:
                            mItalics = true;
                            break;
                        case 4:
                            mUnderline = true;
                        case 5:
                            break; //FIXME support blinking
                        case 6:
                            break; //FIXME support fast blinking
                        case 7:
                            break; //FIXME support inverse
                        case 9:
                            break; //FIXME support strikethrough
                        case 22:
                            mBold = false;
                            break;
                        case 23:
                            mItalics = false;
                            break;
                        case 24:
                            mUnderline = false;
                            break;
                        case 27:
                            break; //FIXME inverse off
                        case 29:
                            break; //FIXME
                        case 30:
                            fgColorR = mBlackR;
                            fgColorG = mBlackG;
                            fgColorB = mBlackB;
                            fgColorLightR = mLightBlackR;
                            fgColorLightG = mLightBlackG;
                            fgColorLightB = mLightBlackB;
                            mIsDefaultColor = false;
                            break;
                        case 31:
                            fgColorR = mRedR;
                            fgColorG = mRedG;
                            fgColorB = mRedB;
                            fgColorLightR = mLightRedR;
                            fgColorLightG = mLightRedG;
                            fgColorLightB = mLightRedB;
                            mIsDefaultColor = false;
                            break;
                        case 32:
                            fgColorR = mGreenR;
                            fgColorG = mGreenG;
                            fgColorB = mGreenB;
                            fgColorLightR = mLightGreenR;
                            fgColorLightG = mLightGreenG;
                            fgColorLightB = mLightGreenB;
                            mIsDefaultColor = false;
                            break;
                        case 33:
                            fgColorR = mYellowR;
                            fgColorG = mYellowG;
                            fgColorB = mYellowB;
                            fgColorLightR = mLightYellowR;
                            fgColorLightG = mLightYellowG;
                            fgColorLightB = mLightYellowB;
                            mIsDefaultColor = false;
                            break;
                        case 34:
                            fgColorR = mBlueR;
                            fgColorG = mBlueG;
                            fgColorB = mBlueB;
                            fgColorLightR = mLightBlueR;
                            fgColorLightG = mLightBlueG;
                            fgColorLightB = mLightBlueB;
                            mIsDefaultColor = false;
                            break;
                        case 35:
                            fgColorR = mMagentaR;
                            fgColorG = mMagentaG;
                            fgColorB = mMagentaB;
                            fgColorLightR = mLightMagentaR;
                            fgColorLightG = mLightMagentaG;
                            fgColorLightB = mLightMagentaB;
                            mIsDefaultColor = false;
                            break;
                        case 36:
                            fgColorR = mCyanR;
                            fgColorG = mCyanG;
                            fgColorB = mCyanB;
                            fgColorLightR = mLightCyanR;
                            fgColorLightG = mLightCyanG;
                            fgColorLightB = mLightCyanB;
                            mIsDefaultColor = false;
                            break;
                        case 37:
                            fgColorR = mWhiteR;
                            fgColorG = mWhiteG;
                            fgColorB = mWhiteB;
                            fgColorLightR = mLightWhiteR;
                            fgColorLightG = mLightWhiteG;
                            fgColorLightB = mLightWhiteB;
                            mIsDefaultColor = false;
                            break;
                        case 39:
                            bgColorR = mBgColorR;
                            bgColorG = mBgColorG;
                            bgColorB = mBgColorB;
                            break;
                        case 40:
                            bgColorR = mBlackR;
                            bgColorG = mBlackG;
                            bgColorB = mBlackB;
                            break;
                        case 41:
                            bgColorR = mRedR;
                            bgColorG = mRedG;
                            bgColorB = mRedB;
                            break;
                        case 42:
                            bgColorR = mGreenR;
                            bgColorG = mGreenG;
                            bgColorB = mGreenB;
                            break;
                        case 43:
                            bgColorR = mYellowR;
                            bgColorG = mYellowG;
                            bgColorB = mYellowB;
                            break;
                        case 44:
                            bgColorR = mBlueR;
                            bgColorG = mBlueG;
                            bgColorB = mBlueB;
                            break;
                        case 45:
                            bgColorR = mMagentaR;
                            bgColorG = mMagentaG;
                            bgColorB = mMagentaB;
                            break;
                        case 46:
                            bgColorR = mCyanR;
                            bgColorG = mCyanG;
                            bgColorB = mCyanB;
                            break;
                        case 47:
                            bgColorR = mWhiteR;
                            bgColorG = mWhiteG;
                            bgColorB = mWhiteB;
                            break;
                        default: qDebug()<<"code error:"<<tag;
                        };
                    }
                    codeRet = 0;
                    goto DECODE;
                }
                else
                {
                    msPos++;
                    gotHeader = false;
                    goto DECODE;
                    qDebug()<<"unrecognized sequence:<"<<code<<s[msPos]<<">";
                }
            }
            // sequenz ist im naechsten tcp paket keep decoder state
            return;
        }
        const QString nothing = "";
        TChar stdCh;
        if( ch == '\n' )
        {
            int line = lineBuffer.size()-1;
            mpHost->mpConsole->runTriggers( line );
	    if( lineBuffer.size()-1 == line )
            {
                if( lineBuffer[line] == "" )
		{
                    newLines++;
                }
                else
                    newLines += 1+wrap( line );
            }
	    else
                newLines += 1+wrap( line );

            std::deque<TChar> newLine;
            buffer.push_back( newLine );
            lineBuffer << nothing;
            QString time = "-----";
            timeBuffer << time;
            firstChar = true;
            msPos++;
            if( static_cast<int>(buffer.size()) > mLinesLimit )
            {
                shrinkBuffer();
            }
            continue;
        }

        lineBuffer.back().append( QChar(ch) );
        TChar c( ! mIsDefaultColor && mBold ? fgColorLightR : fgColorR,
                 ! mIsDefaultColor && mBold ? fgColorLightG : fgColorG,
                 ! mIsDefaultColor && mBold ? fgColorLightB : fgColorB,
                 bgColorR,
                 bgColorG,
                 bgColorB,
                 mIsDefaultColor ? mBold : false,
                 mItalics,
                 mUnderline );
        buffer.back().push_back( c );

        if( firstChar )
        {
            timeBuffer.back() = packetTime;//(QTime::currentTime()).toString("hh:mm:ss.zzz") + "   ";
            firstChar = false;
        }
        msPos++;
    }
}


void TBuffer::append( QString & text,
                             int sub_start,
                             int sub_end,
                             int fgColorR,
                             int fgColorG,
                             int fgColorB,
                             int bgColorR,
                             int bgColorG,
                             int bgColorB,
                             bool bold,
                             bool italics,
                             bool underline )
{
    if( static_cast<int>(buffer.size()) > mLinesLimit )
    {
        shrinkBuffer();
    }
    int last = buffer.size()-1;
    if( last < 0 )
    {
        std::deque<TChar> newLine;
        TChar c(fgColorR,fgColorG,fgColorB,bgColorR,bgColorG,bgColorB,bold,italics,underline);
        newLine.push_back( c );
        buffer.push_back( newLine );
        lineBuffer.push_back(QString());
        timeBuffer << (QTime::currentTime()).toString("hh:mm:ss.zzz") + "   ";
        last = 0;
    }
    bool firstChar = (lineBuffer.back().size() == 0);
    int length = text.size();
    if( length < 1 ) return;
    if( sub_end >= length ) sub_end = text.size()-1;

    for( int i=sub_start; i<=(sub_start+sub_end); i++ )
    {
        if( text.at(i) == '\n' )
        {
            std::deque<TChar> newLine;
            buffer.push_back( newLine );
            lineBuffer.push_back( QString() );
            QString time = "-----";
            timeBuffer << time;
            mLastLine++;
            newLines++;
            firstChar = true;
            continue;
        }
        if( lineBuffer.back().size() >= mWrapAt )
        {
            //assert(lineBuffer.back().size()==buffer.back().size());
            const QString lineBreaks = ",.- ";
            const QString nothing = "";
            for( int i=lineBuffer.back().size()-1; i>=0; i-- )
            {
                if( lineBreaks.indexOf( lineBuffer.back().at(i) ) > -1 )
                {
                    QString tmp = lineBuffer.back().mid(0,i+1);
                    QString lineRest = lineBuffer.back().mid(i+1);
                    lineBuffer.back() = tmp;
                    std::deque<TChar> newLine;

                    int k = lineRest.size();
                    if( k > 0 )
                    {
                        while( k > 0 )
                        {
                            newLine.push_front(buffer.back().back());
                            buffer.back().pop_back();
                            k--;
                        }
                    }

                    buffer.push_back( newLine );
                    if( lineRest.size() > 0 )
                        lineBuffer.append( lineRest );
                    else
                        lineBuffer.append( nothing );
                    QString time = "-----";
                    timeBuffer << time;
                    mLastLine++;
                    newLines++;
                    break;
                }
            }
        }
        lineBuffer.back().append( text.at( i ) );
        TChar c(fgColorR,fgColorG,fgColorB,bgColorR,bgColorG,bgColorB,bold,italics,underline);
        buffer.back().push_back( c );
        if( firstChar )
        {
            timeBuffer.back() = (QTime::currentTime()).toString("hh:mm:ss.zzz") + "   ";
        }
    }
}

void TBuffer::appendLine( QString & text,
                          int sub_start,
                          int sub_end,
                          int fgColorR,
                          int fgColorG,
                          int fgColorB,
                          int bgColorR,
                          int bgColorG,
                          int bgColorB,
                          bool bold,
                          bool italics,
                          bool underline )
{
    if( sub_end < 0 ) return;
    if( static_cast<int>(buffer.size()) > mLinesLimit )
    {
        while( static_cast<int>(buffer.size()) > mLinesLimit-mBatchDeleteSize )
        {
            deleteLine( 0 );
        }
    }
    int last = buffer.size()-1;
    if( last < 0 )
    {
        std::deque<TChar> newLine;
        TChar c(fgColorR,fgColorG,fgColorB,bgColorR,bgColorG,bgColorB,bold,italics,underline);
        newLine.push_back( c );
        buffer.push_back( newLine );
        lineBuffer.push_back(QString());
        timeBuffer << (QTime::currentTime()).toString("hh:mm:ss.zzz") + "   ";
        last = 0;
    }
    bool firstChar = (lineBuffer.back().size() == 0);
    int length = text.size();
    if( length < 1 ) return;
    if( sub_end >= length ) sub_end = text.size()-1;

    for( int i=sub_start; i<=(sub_start+sub_end); i++ )
    {
        lineBuffer.back().append( text.at( i ) );
        TChar c(fgColorR,fgColorG,fgColorB,bgColorR,bgColorG,bgColorB,bold,italics,underline);
        buffer.back().push_back( c );
        if( firstChar )
        {
            timeBuffer.back() = (QTime::currentTime()).toString("hh:mm:ss.zzz") + "   ";
        }
    }
}


void TBuffer::messen()
{
}

/*inline void TBuffer::append( QString & text,
                             int sub_start,
                             int sub_end,
                             int fgColorR,
                             int fgColorG,
                             int fgColorB,
                             int bgColorR,
                             int bgColorG,
                             int bgColorB,
                             bool bold,
                             bool italics,
                             bool underline )
{
    //QTime speed;
    //speed.start();

    if( static_cast<int>(buffer.size()) > mLinesLimit )
    {
        while( static_cast<int>(buffer.size()) > mLinesLimit-10000 )
        {
            deleteLine( 0 );
        }
    }

    int last = buffer.size()-1;
    if( last < 0 )
    {
        std::deque<TChar> newLine;
        TChar c(fgColorR,fgColorG,fgColorB,bgColorR,bgColorG,bgColorB,bold,italics,underline);
        newLine.push_back( c );
        buffer.push_back( newLine );
        lineBuffer << QChar( 0x21af );
        timeBuffer << (QTime::currentTime()).toString("hh:mm:ss.zzz") + "   ";
        last = 0;
    }
    bool firstChar = (lineBuffer[last].size() == 0);
    int length = text.size();
    if( length < 1 ) return;
    if( sub_end >= length ) sub_end = text.size()-1;

    for( int i=sub_start; i<sub_start+sub_end; i++ )
    {

        if( text.at(i) == QChar('\n') )
        {
            std::deque<TChar> newLine;
            buffer.push_back( newLine );
            lineBuffer << QString("");
            QString time = "-----";
            timeBuffer << time;
            mLastLine++;
            newLines++;
            mpHost->mpConsole->runTriggers( mUntriggered, last );
            mUntriggered = ++last;
            firstChar = true;
            continue;
        }
        lineBuffer[last].append( text.at( i ) );
        TChar c(fgColorR,fgColorG,fgColorB,bgColorR,bgColorG,bgColorB,bold,italics,underline);
        buffer[last].push_back( c );
        if( firstChar )
        {
            timeBuffer[last] = (QTime::currentTime()).toString("hh:mm:ss.zzz") + "   ";
        }
    }
    //speedAppend+=speed.elapsed();
}*/

/*inline void TBuffer::append( QString text, QColor & fgColor, QColor & bgColor, bool bold, bool italics, bool underline )
{
//    qDebug()<<"calling bad append text=<"<<text<<">";
    if( static_cast<int>(buffer.size()) > mLinesLimit )
    {
        while( static_cast<int>(buffer.size()) > mLinesLimit-10000 )
        {
            deleteLine( 0 );
        }
    }

    bool runOncePerLine = true;
    for( int i=0; i<text.size(); i++ )
    {
        int last = buffer.size()-1;
        if( last < 0 )
        {
            std::deque<TChar *> newLine;
            TChar * pC = new TChar;
            pC->fgColor = bgColor;    // make the <LF>-marker invisible
            pC->bgColor = bgColor;
            pC->italics = italics;
            pC->bold = bold;
            pC->underline = underline;
            newLine.push_back( pC );
            buffer.push_back( newLine );
            lineBuffer << QChar( 0x21af );
            timeBuffer << (QTime::currentTime()).toString("hh:mm:ss.zzz") + "   ";
            last = 0;
        }
        if( mCursorMoved ) 
        {
            if(lineBuffer[last].size() == 1) // <LF> at beginning of new line marker
            {
                if( lineBuffer[last].at( 0 ) == QChar( 0x21af ) )
                {
                    if( text.at( i ) != QChar( '\n' ) )
                    {
                        mCursorMoved = false;
                        lineBuffer[last].replace( 0, 1, text.at( i ) );
                        TChar * pC = buffer[last][0];
                        pC->fgColor = fgColor;
                        pC->bgColor = bgColor;
                        pC->italics = italics;
                        pC->bold = bold;
                        pC->underline = underline;
                        timeBuffer[last] = (QTime::currentTime()).toString("hh:mm:ss.zzz") + "   ";
                        continue;
                    }
                }
            }
        }
        else
        {
            lineBuffer[last].append( text.at( i ) );
            TChar * pC = new TChar;
            pC->fgColor = fgColor;
            pC->bgColor = bgColor;
            pC->italics = italics;
            pC->bold = bold;
            pC->underline = underline;
            buffer[last].push_back( pC );
        }
        if( text.at(i) == QChar('\n') )
        {
            std::deque<TChar *> newLine;
            TChar * pC = new TChar;
            pC->fgColor = bgColor;    // make the <LF>-marker invisible
            pC->bgColor = bgColor;
            pC->italics = italics;
            pC->bold = bold;
            pC->underline = underline;
            newLine.push_back( pC );
            buffer.push_back( newLine );
            lineBuffer << QChar( 0x21af );
            QString time = (QTime::currentTime()).toString("hh:mm:ss.zzz") + "   ";
            timeBuffer << time;
            mLastLine++;
            newLines++;
            mCursorMoved = true;
            runOncePerLine = true;
        }
    }
}*/

QPoint TBuffer::insert( QPoint & where, QString text, int fgColorR, int fgColorG, int fgColorB, int bgColorR, int bgColorG, int bgColorB, bool bold, bool italics, bool underline )
{
    QPoint P(-1, -1);
    
    int x = where.x();
    int y = where.y();
    
    if( y < 0 ) return P;
    if( y >= static_cast<int>(buffer.size()) ) return P;
    
    
    for( int i=0; i<text.size(); i++ )
    {
        if( text.at(i) == QChar('\n') )
        {
            std::deque<TChar> newLine;
            TChar c(fgColorR,fgColorG,fgColorB,bgColorR,bgColorG,bgColorB,bold,italics,underline);
            newLine.push_back( c );
            buffer.push_back( newLine );
            const QString nothing = "";
            lineBuffer.insert(y, nothing );
            timeBuffer << (QTime::currentTime()).toString("hh:mm:ss.zzz") + "-   ";
            mLastLine++;
            newLines++;
            x = 0;
            y++;
            continue;
        }
        lineBuffer[y].insert( x, text.at( i ) );
        TChar c(fgColorR,fgColorG,fgColorB,bgColorR,bgColorG,bgColorB,bold,italics,underline);
        typedef std::deque<TChar>::iterator IT;
        IT it = buffer[y].begin();
        buffer[y].insert( it+x, c );
    }
    P.setX( x );
    P.setY( y );
    return P;
}


bool TBuffer::insertInLine( QPoint & P, QString & text, TChar & format )
{
    if( text.size() < 1 ) return false;
    int x = P.x();
    int y = P.y();
    if( ( y >= 0 ) && ( y < static_cast<int>(buffer.size()) ) )
    {
        if( x < 0 )
        {
            return false;
        }
        if( x >= static_cast<int>(buffer[y].size()) )
        {
            TChar c;
            expandLine( y, x-buffer[y].size(), c );
        }
        for( int i=0; i<text.size(); i++ )
        {
            lineBuffer[y].insert( x+i, text.at( i ) );
            TChar c = format;
            typedef std::deque<TChar>::iterator IT;
            IT it = buffer[y].begin();
            buffer[y].insert( it+x+i, c );
        }   
    }
    else
    {
        appendLine( text, 0, text.size(), format.fgR, format.fgG, format.fgB, format.bgR, format.bgG, format.bgB, format.bold, format.italics, format.underline );
    }
    return true;
}

TBuffer TBuffer::copy( QPoint & P1, QPoint & P2 )
{
    TBuffer slice( mpHost );
    slice.clear();
    int y = P1.y();
    int x = P1.x();
    if( y < 0 || y >= static_cast<int>(buffer.size()) )
        return slice;
    
    if( ( x < 0 ) 
        || ( x >= static_cast<int>(buffer[y].size()) )
        || ( P2.x() < 0 ) 
        || ( P2.x() > static_cast<int>(buffer[y].size()) ) )
    {
        x=0;
    }
    for( ; x<P2.x(); x++ )
    {
        QString s(lineBuffer[y][x]);
        slice.append(s,
                     0,
                     1,
                     buffer[y][x].fgR,
                     buffer[y][x].fgG,
                     buffer[y][x].fgB,
                     buffer[y][x].bgR,
                     buffer[y][x].bgG,
                     buffer[y][x].bgB,
                     (buffer[y][x].bold == true),
                     (buffer[y][x].italics == true),
                     (buffer[y][x].underline == true) );
    }
    return slice;
}

TBuffer TBuffer::cut( QPoint & P1, QPoint & P2 )
{
    TBuffer slice = copy( P1, P2 );
    QString nothing = "";
    TChar format;
    replaceInLine( P1, P2, nothing, format );
    return slice;
}

void TBuffer::paste( QPoint & P, TBuffer chunk )
{
    bool needAppend = false;
    bool hasAppended = false;
    int y = P.y();
    int x = P.x();
    if( chunk.buffer.size() < 1 )
    {
        return;
    }
    if( y < 0 || y > getLastLineNumber() )
    {
        y = getLastLineNumber();
    }
    if( y == -1 )
    {
        needAppend = true;
    }
    else
    {
        if( x < 0 || x >= static_cast<int>(buffer[y].size()) )
        {
            return;
        }
    }
    for( int cx=0; cx<static_cast<int>(chunk.buffer[0].size()); cx++ )
    {
        QPoint P_current(cx, y);
        if( ( y < getLastLineNumber() ) && ( ! needAppend ) )
        {
            TChar & format = chunk.buffer[0][cx];
            QString s = QString(chunk.lineBuffer[0][cx]);
            insertInLine( P_current, s, format );
        }
        else
        {
            hasAppended = true;
            QString s(chunk.lineBuffer[0][cx]);
            append(s,
                   0,
                   1,
                   chunk.buffer[0][cx].fgR,
                   chunk.buffer[0][cx].fgG,
                   chunk.buffer[0][cx].fgB,
                   chunk.buffer[0][cx].bgR,
                   chunk.buffer[0][cx].bgG,
                   chunk.buffer[0][cx].bgB,
                   (chunk.buffer[0][cx].bold == true),
                   (chunk.buffer[0][cx].italics == true),
                   (chunk.buffer[0][cx].underline == true) );
        }
    }
    if( hasAppended )
    {
        TChar format;
        if( y == -1 )
        {
            wrap( y,mWrapAt, mWrapIndent, format );
        }
        else
        {
            wrapLine( y, mWrapAt, mWrapIndent, format );
        }
    }
}

void TBuffer::appendBuffer( TBuffer chunk )
{
    if( chunk.buffer.size() < 1 )
    {
        return;
    }
    for( int cx=0; cx<static_cast<int>(chunk.buffer[0].size()); cx++ )
    {
        QString s(chunk.lineBuffer[0][cx]);
        append(s,
               0,
               1,
               chunk.buffer[0][cx].fgR,
               chunk.buffer[0][cx].fgG,
               chunk.buffer[0][cx].fgB,
               chunk.buffer[0][cx].bgR,
               chunk.buffer[0][cx].bgG,
               chunk.buffer[0][cx].bgB,
               (chunk.buffer[0][cx].bold == true),
               (chunk.buffer[0][cx].italics == true),
               (chunk.buffer[0][cx].underline == true) );
    }
    QString lf = "\n";
    append( lf,
               0,
               1,
               0,
               0,
               0,
               0,
               0,
               0,
               false,
               false,
               false );
}

int TBuffer::calcWrapPos( int line, int begin, int end )
{
    const QString lineBreaks = ",.- ";
    if( lineBuffer.size() < line ) return -1;
    if( lineBuffer[line].size() < end )
    {
        end = lineBuffer[line].size()-1;
    }
    for( int i=end; i>=begin; i-- )
    {
        if( lineBreaks.indexOf(lineBuffer[line][i]) > -1 )
        {
            return i;
        }
    }
    return -1;
}

inline int TBuffer::wrap( int startLine )
{
    if( static_cast<int>(buffer.size()) < startLine ) return 0;
    std::queue<std::deque<TChar> > queue;
    QStringList tempList;
    QStringList timeList;
    int lineCount = 0;

    for( int i=startLine; i<static_cast<int>(buffer.size()); i++ )
    {
        assert( static_cast<int>(buffer[i].size()) == lineBuffer[i].size() );
        std::deque<TChar> newLine;
        QString lineText;
        QString time = timeBuffer[i];
        int indent = 0;
        if( static_cast<int>(buffer[i].size()) >= mWrapAt )
        {
            for( int i3=0; i3<mWrapIndent; i3++ )
            {
                TChar pSpace;
                newLine.push_back( pSpace );
                lineText.append( " " );
            }
            indent = mWrapIndent;
        }
        int lastSpace = -1;
        int wrapPos = -1;
        int length = buffer[i].size();
        for( int i2=0; i2<static_cast<int>(buffer[i].size());  )
        {
            if( length-i2 > mWrapAt-indent )
            {
                wrapPos = calcWrapPos( i, i2, i2+mWrapAt-indent );
                if( wrapPos > -1 )
                {
                    lastSpace = wrapPos;
                }
                else
                {
                    lastSpace = -1;
                }
            }
            else
            {
                lastSpace = -1;
            }
            for( int i3=0; i3<mWrapAt-indent; i3++ )
            {
                if( lastSpace > 0 )
                {
                    if( i2 >= lastSpace )
                    {
                        i2++;
                        break;
                    }
                }
                if( i2 >= static_cast<int>(buffer[i].size()) )
                {
                    break;
                }
                if( lineBuffer[i].at(i2) == '\n' )
                {
                    i2++;
                    break;
                }
                newLine.push_back( buffer[i][i2] );
                lineText.append( lineBuffer[i].at(i2) );
                i2++;
            }
            queue.push( newLine );
            tempList.append( lineText );
            timeList.append( time );
            newLine.clear();
            lineText.clear();
            indent = 0;
        }
        lineCount++;
    }

    for( int i=0; i<lineCount; i++ )
    {
        buffer.pop_back();
        lineBuffer.pop_back();
        timeBuffer.pop_back();
    }

    newLines -= lineCount;
    newLines += queue.size();
    int insertedLines = queue.size()-1;

    while( ! queue.empty() )
    {
        buffer.push_back( queue.front() );
        queue.pop();
    }

    for( int i=0; i<tempList.size(); i++ )
    {
        lineBuffer.append( tempList[i] );
        timeBuffer.append( timeList[i] );
    }
    return insertedLines > 0 ? insertedLines : 0;
}


// returns how many new lines have been inserted by the wrapping action
int TBuffer::wrap( int startLine, int screenWidth, int indentSize, TChar & format )
{
    return 0;

    if( startLine < 0 ) return 0;
    if( static_cast<int>(buffer.size()) < startLine ) return 0;
    std::queue<std::deque<TChar> > queue;
    QStringList tempList;
    QStringList timeList;
    int lineCount = 0;
    
    for( int i=startLine; i<static_cast<int>(buffer.size()); i++ )
    {
        assert( static_cast<int>(buffer[i].size()) == lineBuffer[i].size() );
        std::deque<TChar> newLine;
        QString lineText;
        QString time = timeBuffer[i];
        int indent = 0;
        if( static_cast<int>(buffer[i].size()) >= screenWidth )
        {
            for( int i3=0; i3<indentSize; i3++ )
            {
                TChar pSpace = format;
                newLine.push_back( pSpace );
                lineText.append( " " );
            }
            indent = indentSize;
        }
        int lastSpace = -1;
        int wrapPos = -1;
        int length = buffer[i].size();
        for( int i2=0; i2<static_cast<int>(buffer[i].size());  )
        {
            if( length-i2 > screenWidth-indent )
            {
                wrapPos = calcWrapPos( i, i2, i2+screenWidth-indent );
                if( wrapPos > -1 )
                {
                    lastSpace = wrapPos;
                }
                else
                {
                    lastSpace = -1;
                }
            }
            else
            {
                lastSpace = -1;
            }
            for( int i3=0; i3<screenWidth-indent; i3++ )
            {
                if( lastSpace > 0 )
                {
                    if( i2 >= lastSpace )
                    {
                        i2++;
                        break;
                    }
                }
                if( i2 >= static_cast<int>(buffer[i].size()) )
                {
                    break;
                }
                if( lineBuffer[i][i2] == QChar('\n') )
                {
                    i2++;
                    break;
                }
                newLine.push_back( buffer[i][i2] );
                lineText.append( lineBuffer[i].at(i2) );
                i2++;
            }
            queue.push( newLine );
            tempList.append( lineText );
            timeList.append( time );
            newLine.clear();
            lineText.clear();
            indent = 0;
        }
        lineCount++;
    }
    
    for( int i=0; i<lineCount; i++ )
    {
        buffer.pop_back();    
        lineBuffer.pop_back();
        timeBuffer.pop_back();
    }
    
    newLines -= lineCount;
    newLines += queue.size();
    int insertedLines = queue.size()-1;
    
    while( ! queue.empty() )
    {
        buffer.push_back( queue.front() );
        queue.pop();
    }
    
    for( int i=0; i<tempList.size(); i++ )
    {
        lineBuffer.append( tempList[i] );
        timeBuffer.append( timeList[i] );
    }
    return insertedLines > 0 ? insertedLines : 0;
}

// returns how many new lines have been inserted by the wrapping action
int TBuffer::wrapLine( int startLine, int screenWidth, int indentSize, TChar & format )
{
    if( startLine < 0 ) return 0;
    if( static_cast<int>(buffer.size()) <= startLine ) return 0;
    std::queue<std::deque<TChar> > queue;
    QStringList tempList;
    int lineCount = 0;

    for( int i=startLine; i<static_cast<int>(buffer.size()); i++ )
    {
        if( i > startLine ) break; //only wrap one line of text

        assert( static_cast<int>(buffer[i].size()) == lineBuffer[i].size() );
        std::deque<TChar> newLine;
        QString lineText;

        int indent = 0;
        if( static_cast<int>(buffer[i].size()) >= screenWidth )
        {
            for( int i3=0; i3<indentSize; i3++ )
            {
                TChar pSpace = format;
                newLine.push_back( pSpace );
                lineText.append( " " );
            }
            indent = indentSize;
        }
        int lastSpace = -1;
        int wrapPos = -1;
        int length = static_cast<int>(buffer[i].size());
        for( int i2=0; i2<static_cast<int>(buffer[i].size());  )
        {
            if( length-i2 > screenWidth-indent )
            {
                wrapPos = calcWrapPos( i, i2, i2+screenWidth-indent );
                if( wrapPos > -1 )
                {
                    lastSpace = wrapPos;
                }
                else
                {
                    lastSpace = -1;
                }
            }
            else
            {
                lastSpace = -1;
            }
            for( int i3=0; i3<screenWidth-indent; i3++ )
            {
                if( lastSpace > 0 )
                {
                    if( i2 >= lastSpace )
                    {
                        i2++;
                        break;
                    }
                }
                if( i2 >= static_cast<int>(buffer[i].size()) )
                {
                    break;
                }
                if( lineBuffer[i][i2] == QChar('\n') )
                {
                    i2++;
                    break;
                }
                newLine.push_back( buffer[i][i2] );
                lineText.append( lineBuffer[i].at(i2) );
                i2++;
            }
            queue.push( newLine );
            tempList.append( lineText );

            newLine.clear();
            lineText.clear();
            indent = 0;
        }
        lineCount++;
    }

    buffer.erase( buffer.begin()+startLine );
    lineBuffer.removeAt( startLine );
    timeBuffer.removeAt( startLine );

    int insertedLines = queue.size()-1;

    int i=0;
    while( ! queue.empty() )
    {
        buffer.insert(buffer.begin()+startLine+i, queue.front());
        queue.pop();
        i++;
    }

    for( int i=0; i<tempList.size(); i++ )
    {
        lineBuffer.insert( startLine+i, tempList[i] );
        timeBuffer.insert( startLine+i, QTime::currentTime().toString("hh:mm:ss.zzz")+"   " );
    }
    return insertedLines > 0 ? insertedLines : 0;
}


bool TBuffer::moveCursor( QPoint & where )
{
    int x = where.x();
    int y = where.y();
    if( y < 0 ) return false;
    if( y >= static_cast<int>(buffer.size()) ) return false;
    
    if( static_cast<int>(buffer[y].size())-1 >  x )
    {
        TChar c;
        expandLine( y, x-buffer[y].size()-1, c );
    }
    return true;
}

QString badLineError = QString("ERROR: invalid line number");


QString & TBuffer::line( int n )
{
    if( (n >= lineBuffer.size()) || (n<0) ) return badLineError;
    return lineBuffer[n];
}


int TBuffer::find( int line, QString what, int pos=0 )
{
    if( lineBuffer[line].size() >= pos ) return -1;
    if( pos < 0 ) return -1;
    if( ( line >= static_cast<int>(buffer.size()) ) || ( line < 0 ) ) return -1;
    return lineBuffer[line].indexOf( what, pos );
}


QStringList TBuffer::split( int line, QString splitter )
{
    if( ( line >= static_cast<int>(buffer.size()) ) || ( line < 0 ) ) return QStringList();
    return lineBuffer[line].split( splitter );
}


QStringList TBuffer::split( int line, QRegExp splitter )
{
    if( ( line >= static_cast<int>(buffer.size()) ) || ( line < 0 ) ) return QStringList();
    return lineBuffer[line].split( splitter );
}


void TBuffer::expandLine( int y, int count, TChar & pC )
{
    int size = buffer[y].size()-1;
    for( int i=size; i<size+count; i++ )
    {
        buffer[y].push_back( pC );
        lineBuffer[y].append( " " );
    }
}

bool TBuffer::replaceInLine( QPoint & P_begin,
                             QPoint & P_end,
                             QString & with,
                             TChar & format )
{
    int x1 = P_begin.x();
    int x2 = P_end.x();
    int y1 = P_begin.y();
    int y2 = P_end.y();
    if( ( y1 >= static_cast<int>(buffer.size()) ) || ( y2 >= static_cast<int>(buffer.size()) ) )
    {
        return false;
    }
    if( ( x2 > static_cast<int>(buffer[y2].size()) ) || ( x1 > static_cast<int>(buffer[y1].size()) ) )
    {
        return false;
    }
    if( x1 < 0 || x2 < 0 )
    {
        return false;
    }

    int xb,xe, yb, ye;
    if( y1 <= y2 )
    {
        yb = y1;
        ye = y2;
        xb = x1;
        xe = x2;
    }
    else
    {
        yb = y2;
        ye = y1;
        xb = x2;
        xe = x1;
    }

    for( int y=yb; y<=ye; y++ )
    {
        int x = 0;
        if( y == yb )
        {
            x = xb;
        }
        int x_end = buffer[y].size()-1;
        if( y == ye )
        {
            x_end = xe;
        }
        lineBuffer[y].remove( x, x_end-x );
        typedef std::deque<TChar>::iterator IT;
        IT it1 = buffer[y].begin()+x;
        IT it2 = buffer[y].begin()+x_end;
        buffer[y].erase( it1, it2 );
    }

    // insert replacement
    insertInLine( P_begin, with, format );
    return true;
}


bool TBuffer::replace( int line, QString what, QString with )
{
    if( ( line >= static_cast<int>(buffer.size()) ) || ( line < 0 ) )
        return false;
    lineBuffer[line].replace( what, with );
    
    // fix size of the corresponding format buffer
    
    int delta = lineBuffer[line].size() - static_cast<int>(buffer[line].size());
    
    if( delta > 0 )
    {
        for( int i=0; i<delta; i++ )
        {
            TChar c( mpHost ); // cloning default char format according to profile
                               // because a lookup would be too expensive as
                               // this is a very often used function and this standard
                               // behaviour is acceptable. If the user wants special colors
                               // he can apply format changes
            buffer[line].push_back( c );
        }
    }
    else if( delta < 0 )
    {
        for( int i=0; i<delta; i++ )
        {
            buffer[line].pop_back();
        }
    }
    return true;
}

void TBuffer::clear()
{
    while( buffer.size() > 0 )
    {
        if( ! deleteLines( 0, 0 ) )
        {
            break;
        }
    }
    std::deque<TChar> newLine;
    buffer.push_back( newLine );
    lineBuffer << QString();
    timeBuffer << "   ";
}

bool TBuffer::deleteLine( int y )
{ 
    return deleteLines( y, y );
}

void TBuffer::shrinkBuffer()
{
    for( int i=0; i < mBatchDeleteSize; i++ )
    {
        lineBuffer.pop_front();
        timeBuffer.pop_front();
        buffer.pop_front();
    }
}

bool TBuffer::deleteLines( int from, int to )
{
    if( ( from >= 0 ) 
     && ( from < static_cast<int>(buffer.size()) )
     && ( from <= to )   
     && ( to >=0 )
     && ( to < static_cast<int>(buffer.size()) ) )
    {
        int delta = to - from + 1;
        
        for( int i=from; i<from+delta; i++ )
        {
            lineBuffer.removeAt( i ); 
            timeBuffer.removeAt( i );
        }
        
        int i = (int)buffer.size();
        // we do reverse lookup as the wanted lines are usually at the end of the buffer
        // std::reverse_iterator is not defined for usage in erase()
        
        typedef std::deque<std::deque<TChar> >::iterator IT;
        for( IT it=buffer.end(); it!=buffer.begin(); )
        {
            it--;
            i--;
            if( i > to ) 
                continue;
            
            if( --delta >= 0 )
                buffer.erase( it );
            else
                break;
        }
        return true;
    }
    else 
    {
        return false;
    }
}


bool TBuffer::applyFormat( QPoint & P_begin, QPoint & P_end, TChar & format )
{
    if( ( P_begin.x() >= 0 ) 
        && ( ( P_end.y() < static_cast<int>(buffer.size()) ) && ( P_end.y() >= 0 ) )
        && ( ( P_end.x() > P_begin.x() ) || ( P_end.y() > P_begin.y() ) ) )
    {
        for( int y=P_begin.y(); y<=P_end.y(); y++ )
        {
            int x = 0;
            if( y == P_begin.y() )
            {
                x = P_begin.x();
            }
            while( x < static_cast<int>(buffer[y].size()) )
            {
                if( y >= P_end.y() )
                {
                    if( x >= P_end.x() )
                    {
                        return true;
                    }
                }
            
                buffer[y][x] = format;
                x++;
            }
        }
        return true;
    }
    else 
        return false;            
}

bool TBuffer::applyBold( QPoint & P_begin, QPoint & P_end, bool bold )
{
    if( ( P_begin.x() >= 0 )
        && ( ( P_end.y() < static_cast<int>(buffer.size()) ) && ( P_end.y() >= 0 ) )
        && ( ( P_end.x() > P_begin.x() ) || ( P_end.y() > P_begin.y() ) ) )
    {
        for( int y=P_begin.y(); y<=P_end.y(); y++ )
        {
            int x = 0;
            if( y == P_begin.y() )
            {
                x = P_begin.x();
            }
            while( x < static_cast<int>(buffer[y].size()) )
            {
                if( y >= P_end.y() )
                {
                    if( x >= P_end.x() )
                    {
                        return true;
                    }
                }

                buffer[y][x].bold = bold;
                x++;
            }
        }
        return true;
    }
    else
        return false;
}

bool TBuffer::applyItalics( QPoint & P_begin, QPoint & P_end, bool bold )
{
    if( ( P_begin.x() >= 0 )
        && ( ( P_end.y() < static_cast<int>(buffer.size()) ) && ( P_end.y() >= 0 ) )
        && ( ( P_end.x() > P_begin.x() ) || ( P_end.y() > P_begin.y() ) ) )
    {
        for( int y=P_begin.y(); y<=P_end.y(); y++ )
        {
            int x = 0;
            if( y == P_begin.y() )
            {
                x = P_begin.x();
            }
            while( x < static_cast<int>(buffer[y].size()) )
            {
                if( y >= P_end.y() )
                {
                    if( x >= P_end.x() )
                    {
                        return true;
                    }
                }

                buffer[y][x].italics = bold;
                x++;
            }
        }
        return true;
    }
    else
        return false;
}

bool TBuffer::applyUnderline( QPoint & P_begin, QPoint & P_end, bool bold )
{
    if( ( P_begin.x() >= 0 )
        && ( ( P_end.y() < static_cast<int>(buffer.size()) ) && ( P_end.y() >= 0 ) )
        && ( ( P_end.x() > P_begin.x() ) || ( P_end.y() > P_begin.y() ) ) )
    {
        for( int y=P_begin.y(); y<=P_end.y(); y++ )
        {
            int x = 0;
            if( y == P_begin.y() )
            {
                x = P_begin.x();
            }
            while( x < static_cast<int>(buffer[y].size()) )
            {
                if( y >= P_end.y() )
                {
                    if( x >= P_end.x() )
                    {
                        return true;
                    }
                }

                buffer[y][x].underline = bold;
                x++;
            }
        }
        return true;
    }
    else
        return false;
}

bool TBuffer::applyFgColor( QPoint & P_begin, QPoint & P_end, int fgColorR, int fgColorG, int fgColorB )
{
    if( ( P_begin.x() >= 0 )
        && ( ( P_end.y() < static_cast<int>(buffer.size()) ) && ( P_end.y() >= 0 ) )
        && ( ( P_end.x() > P_begin.x() ) || ( P_end.y() > P_begin.y() ) ) )
    {
        for( int y=P_begin.y(); y<=P_end.y(); y++ )
        {
            int x = 0;
            if( y == P_begin.y() )
            {
                x = P_begin.x();
            }
            while( x < static_cast<int>(buffer[y].size()) )
            {
                if( y >= P_end.y() )
                {
                    if( x >= P_end.x() )
                    {
                        return true;
                    }
                }

                buffer[y][x].fgR = fgColorR;
                buffer[y][x].fgG = fgColorG;
                buffer[y][x].fgB = fgColorB;
                x++;
            }
        }
        return true;
    }
    else
        return false;
}

bool TBuffer::applyBgColor( QPoint & P_begin, QPoint & P_end, int bgColorR, int bgColorG, int bgColorB )
{
    if( ( P_begin.x() >= 0 )
        && ( ( P_end.y() < static_cast<int>(buffer.size()) ) && ( P_end.y() >= 0 ) )
        && ( ( P_end.x() > P_begin.x() ) || ( P_end.y() > P_begin.y() ) ) )
    {
        for( int y=P_begin.y(); y<=P_end.y(); y++ )
        {
            int x = 0;
            if( y == P_begin.y() )
            {
                x = P_begin.x();
            }
            while( x < static_cast<int>(buffer[y].size()) )
            {
                if( y >= P_end.y() )
                {
                    if( x >= P_end.x() )
                    {
                        return true;
                    }
                }

                (buffer[y][x]).bgR = bgColorR;
                (buffer[y][x]).bgG = bgColorG;
                (buffer[y][x]).bgB = bgColorB;
                x++;
            }
        }
        return true;
    }
    else
        return false;
}

QStringList TBuffer::getEndLines( int n )
{
    QStringList linesList;
    for( int i=getLastLineNumber()-n; i<getLastLineNumber(); i++ )
    {
        linesList << line( i );
    }
    return linesList;
}





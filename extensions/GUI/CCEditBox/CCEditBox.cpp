/****************************************************************************
 Copyright (c) 2010-2012 cocos2d-x.org
 Copyright (c) 2012 James Chen
 
 http://www.cocos2d-x.org
 
 Permission is hereby granted, free of charge, to any person obtaining a copy
 of this software and associated documentation files (the "Software"), to deal
 in the Software without restriction, including without limitation the rights
 to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 copies of the Software, and to permit persons to whom the Software is
 furnished to do so, subject to the following conditions:
 
 The above copyright notice and this permission notice shall be included in
 all copies or substantial portions of the Software.
 
 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 THE SOFTWARE.
 ****************************************************************************/

#include "CCEditBox.h"
#include "CCEditBoxImpl.h"

NS_CC_EXT_BEGIN

// to save all CCEditBox instances
static std::vector<CCEditBox*>* sActiveEditBoxes = new std::vector<CCEditBox*>();

// find next edit box, or null if none
static CCEditBox* nextEditBox(CCEditBox* box) {
    for(std::vector<CCEditBox*>::iterator iter = sActiveEditBoxes->begin(); iter != sActiveEditBoxes->end(); iter++) {
        if(*iter == box) {
            iter++;
            if(iter != sActiveEditBoxes->end()) {
                CCEditBox* next = *iter;
                if(next->isRunning())
                    return next;
                else
                    return nullptr;
            } else {
                return nullptr;
            }
        }
    }
    
    return nullptr;
}

CCEditBox::CCEditBox()
: m_pEditBoxImpl(nullptr)
, m_pDelegate(nullptr)
, m_eEditBoxInputMode(kEditBoxInputModeSingleLine)
, m_eEditBoxInputFlag(kEditBoxInputFlagInitialCapsAllCharacters)
, m_eKeyboardReturnType(kKeyboardReturnTypeDefault)
, m_nFontSize(-1)
, m_nPlaceholderFontSize(-1)
, m_colText(ccWHITE)
, m_colPlaceHolder(ccGRAY)
, m_nMaxLength(0)
, m_fAdjustHeight(0.0f) {
    memset(&m_nScriptEditBoxHandler, 0, sizeof(ccScriptFunction));
    
    // register self
    sActiveEditBoxes->push_back(this);
}

CCEditBox::~CCEditBox() {
    // unregister self
    for(std::vector<CCEditBox*>::iterator iter = sActiveEditBoxes->begin(); iter != sActiveEditBoxes->end(); iter++) {
        if(*iter == this) {
            sActiveEditBoxes->erase(iter);
            break;
        }
    }
    
    // release others
    CC_SAFE_DELETE(m_pEditBoxImpl);
    unregisterScriptEditBoxHandler();
}

bool CCEditBox::moveToNext() {
    CCEditBox* next = nextEditBox(this);
    if(next) {
        next->m_pEditBoxImpl->openKeyboard();
        m_pEditBoxImpl->removeFromSuperview();
        return true;
    } else {
        return false;
    }
}

void CCEditBox::touchDownAction(CCObject *sender, CCControlEvent controlEvent)
{
    m_pEditBoxImpl->openKeyboard();
}

CCEditBox* CCEditBox::create(const CCSize& size, CCScale9Sprite* pNormal9SpriteBg, CCScale9Sprite* pPressed9SpriteBg/* = nullptr*/, CCScale9Sprite* pDisabled9SpriteBg/* = nullptr*/)
{
    CCEditBox* pRet = new CCEditBox();
    
    if (pRet != nullptr && pRet->initWithSizeAndBackgroundSprite(size, pNormal9SpriteBg))
    {
        if (pPressed9SpriteBg != nullptr)
        {
            pRet->setBackgroundSpriteForState(pPressed9SpriteBg, CCControlStateHighlighted);
        }
        
        if (pDisabled9SpriteBg != nullptr)
        {
            pRet->setBackgroundSpriteForState(pDisabled9SpriteBg, CCControlStateDisabled);
        }
        CC_SAFE_AUTORELEASE(pRet);
    }
    else
    {
        CC_SAFE_DELETE(pRet);
    }
    
    return pRet;
}

bool CCEditBox::initWithSizeAndBackgroundSprite(const CCSize& size, CCScale9Sprite* pPressed9SpriteBg)
{
    if (CCControlButton::initWithBackgroundSprite(pPressed9SpriteBg))
    {
        m_pEditBoxImpl = __createSystemEditBox(this);
        m_pEditBoxImpl->initWithSize(size);
        
        this->setZoomOnTouchDown(false);
        this->setPreferredSize(size);
        this->setPosition(ccp(0, 0));
        this->addTargetWithActionForControlEvent(this, cccontrol_selector(CCEditBox::touchDownAction), CCControlEventTouchUpInside);
        
        return true;
    }
    return false;
}

void CCEditBox::setDelegate(CCEditBoxDelegate* pDelegate)
{
    m_pDelegate = pDelegate;
    if (m_pEditBoxImpl != nullptr)
    {
        m_pEditBoxImpl->setDelegate(pDelegate);
    }
}

CCEditBoxDelegate* CCEditBox::getDelegate()
{
    return m_pDelegate;
}

void CCEditBox::setText(const char* pText)
{
    if (pText != nullptr)
    {
        m_strText = pText;
        if (m_pEditBoxImpl != nullptr)
        {
            m_pEditBoxImpl->setText(pText);
        }
    }
}

const char* CCEditBox::getText(void)
{
    if (m_pEditBoxImpl != nullptr)
    {
		const char* pText = m_pEditBoxImpl->getText();
		if(pText != nullptr)
			return pText;
    }
    
    return "";
}

void CCEditBox::setFont(const char* pFontName, int fontSize)
{
    m_strFontName = pFontName;
    m_nFontSize = fontSize;
    if (pFontName != nullptr)
    {
        if (m_pEditBoxImpl != nullptr)
        {
            m_pEditBoxImpl->setFont(pFontName, fontSize);
        }
    }
}

void CCEditBox::setFontName(const char* pFontName)
{
    m_strFontName = pFontName;
    if (m_pEditBoxImpl != nullptr && m_nFontSize != -1)
    {
        m_pEditBoxImpl->setFont(pFontName, m_nFontSize);
    }
}

void CCEditBox::setFontSize(int fontSize)
{
    m_nFontSize = fontSize;
    if (m_pEditBoxImpl != nullptr && m_strFontName.length() > 0)
    {
        m_pEditBoxImpl->setFont(m_strFontName.c_str(), m_nFontSize);
    }
}

void CCEditBox::setFontColor(const ccColor3B& color)
{
    m_colText = color;
    if (m_pEditBoxImpl != nullptr)
    {
        m_pEditBoxImpl->setFontColor(color);
    }
}

void CCEditBox::setPlaceholderFont(const char* pFontName, int fontSize)
{
    m_strPlaceholderFontName = pFontName;
    m_nPlaceholderFontSize = fontSize;
    if (pFontName != nullptr)
    {
        if (m_pEditBoxImpl != nullptr)
        {
            m_pEditBoxImpl->setPlaceholderFont(pFontName, fontSize);
        }
    }
}

void CCEditBox::setPlaceholderFontName(const char* pFontName)
{
    m_strPlaceholderFontName = pFontName;
    if (m_pEditBoxImpl != nullptr && m_nPlaceholderFontSize != -1)
    {
        m_pEditBoxImpl->setPlaceholderFont(pFontName, m_nFontSize);
    }
}

void CCEditBox::setPlaceholderFontSize(int fontSize)
{
    m_nPlaceholderFontSize = fontSize;
    if (m_pEditBoxImpl != nullptr && m_strPlaceholderFontName.length() > 0)
    {
        m_pEditBoxImpl->setPlaceholderFont(m_strPlaceholderFontName.c_str(), m_nFontSize);
    }
}

void CCEditBox::setPlaceholderFontColor(const ccColor3B& color)
{
    m_colText = color;
    if (m_pEditBoxImpl != nullptr)
    {
        m_pEditBoxImpl->setPlaceholderFontColor(color);
    }
}

void CCEditBox::setPlaceHolder(const char* pText)
{
    if (pText != nullptr)
    {
        m_strPlaceHolder = pText;
        if (m_pEditBoxImpl != nullptr)
        {
            m_pEditBoxImpl->setPlaceHolder(pText);
        }
    }
}

const char* CCEditBox::getPlaceHolder(void)
{
    return m_strPlaceHolder.c_str();
}

void CCEditBox::setInputMode(EditBoxInputMode inputMode)
{
    m_eEditBoxInputMode = inputMode;
    if (m_pEditBoxImpl != nullptr)
    {
        m_pEditBoxImpl->setInputMode(inputMode);
    }
}

void CCEditBox::setMaxLength(int maxLength)
{
    m_nMaxLength = maxLength;
    if (m_pEditBoxImpl != nullptr)
    {
        m_pEditBoxImpl->setMaxLength(maxLength);
    }
}


int CCEditBox::getMaxLength()
{
    return m_nMaxLength;
}

void CCEditBox::setInputFlag(EditBoxInputFlag inputFlag)
{
    m_eEditBoxInputFlag = inputFlag;
    if (m_pEditBoxImpl != nullptr)
    {
        m_pEditBoxImpl->setInputFlag(inputFlag);
    }
}

void CCEditBox::setReturnType(KeyboardReturnType returnType)
{
    if (m_pEditBoxImpl != nullptr)
    {
        m_pEditBoxImpl->setReturnType(returnType);
    }
}

/* override function */
void CCEditBox::setPosition(const CCPoint& pos)
{
    CCControlButton::setPosition(pos);
    if (m_pEditBoxImpl != nullptr)
    {
        m_pEditBoxImpl->setPosition(pos);
    }
}

void CCEditBox::setVisible(bool visible)
{
    CCControlButton::setVisible(visible);
    if (m_pEditBoxImpl != nullptr)
    {
        m_pEditBoxImpl->setVisible(visible);
    }
}

void CCEditBox::setContentSize(const CCSize& size)
{
    CCControlButton::setContentSize(size);
    if (m_pEditBoxImpl != nullptr)
    {
        m_pEditBoxImpl->setContentSize(size);
    }
}

void CCEditBox::setAnchorPoint(const CCPoint& anchorPoint)
{
    CCControlButton::setAnchorPoint(anchorPoint);
    if (m_pEditBoxImpl != nullptr)
    {
        m_pEditBoxImpl->setAnchorPoint(anchorPoint);
    }
}

void CCEditBox::visit(void)
{
    CCControlButton::visit();
    if (m_pEditBoxImpl != nullptr)
    {
        m_pEditBoxImpl->visit();
    }
}

void CCEditBox::onEnter(void)
{
    CCControlButton::onEnter();
    if (m_pEditBoxImpl != nullptr)
    {
        m_pEditBoxImpl->onEnter();
    }
}

void CCEditBox::onExit(void)
{
    CCControlButton::onExit();
    if (m_pEditBoxImpl != nullptr)
    {
        // remove system edit control
        m_pEditBoxImpl->closeKeyboard();
    }
}

static CCRect getRect(CCNode * pNode)
{
	CCSize contentSize = pNode->getContentSize();
	CCRect rect = CCRectMake(0, 0, contentSize.width, contentSize.height);
	return CCRectApplyAffineTransform(rect, pNode->nodeToWorldTransform());
}

void CCEditBox::keyboardWillShow(CCIMEKeyboardNotificationInfo& info)
{
    // CCLOG("CCEditBox::keyboardWillShow");
    CCRect rectTracked = getRect(this);
	// some adjustment for margin between the keyboard and the edit box.
	rectTracked.origin.y -= 4;

    // if the keyboard area doesn't intersect with the tracking node area, nothing needs to be done.
    if (!rectTracked.intersectsRect(info.end))
    {
        CCLOG("needn't to adjust view layout.");
        return;
    }
    
    // assume keyboard at the bottom of screen, calculate the vertical adjustment.
    m_fAdjustHeight = info.end.getMaxY() - rectTracked.getMinY();
    // CCLOG("CCEditBox:needAdjustVerticalPosition(%f)", m_fAdjustHeight);
    
    if (m_pEditBoxImpl != nullptr)
    {
        m_pEditBoxImpl->doAnimationWhenKeyboardMove(info.duration, m_fAdjustHeight);
    }
}

void CCEditBox::keyboardDidShow(CCIMEKeyboardNotificationInfo& info)
{
	
}

void CCEditBox::keyboardWillHide(CCIMEKeyboardNotificationInfo& info)
{
    // CCLOG("CCEditBox::keyboardWillHide");
    if (m_pEditBoxImpl != nullptr)
    {
        m_pEditBoxImpl->doAnimationWhenKeyboardMove(info.duration, -m_fAdjustHeight);
    }
}

void CCEditBox::keyboardDidHide(CCIMEKeyboardNotificationInfo& info)
{
	
}

void CCEditBox::registerScriptEditBoxHandler(ccScriptFunction handler)
{
    unregisterScriptEditBoxHandler();
    m_nScriptEditBoxHandler = handler;
}

void CCEditBox::unregisterScriptEditBoxHandler() {
    if (m_nScriptEditBoxHandler.handler) {
        CCScriptEngineManager::sharedManager()->getScriptEngine()->removeScriptHandler(m_nScriptEditBoxHandler.handler);
        m_nScriptEditBoxHandler.handler = 0;
    }
}


NS_CC_EXT_END

#include "atscreen.h"

cATScreen::cATScreen(atGlobal &global):m_global(global)
{
    m_screenState = SS_BOOT;
}

cATScreen::~cATScreen()
{

}

bool cATScreen::init()
{
    return true;
}
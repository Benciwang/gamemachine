#include "stdafx.h"
#include "effects.h"

void Demo_Effects::onActivate()
{
	// ʹ��Ԥ����Ч�ǳ��򵥣�ֻ��Ҫ���ô�״̬����
	// ������Զ�����Ч����Ҫ�����޸���ɫ��
	GMSetRenderState(EFFECTS, gm::GMEffects::Blur);
}
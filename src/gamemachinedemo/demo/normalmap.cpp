﻿#include "stdafx.h"
#include "normalmap.h"
#include <linearmath.h>
#include <gmcontrols.h>

GM_DEFINE_SIGNAL(Demo_NormalMap::rotateStateChanged);

void Demo_NormalMap::init()
{
	D(d);
	D_BASE(db, DemoHandler);
	Base::init();
	
	// 创建对象
	getDemoWorldReference() = new gm::GMDemoGameWorld(db->parentDemonstrationWorld->getContext());

	// 创建一个纹理
	struct _ShaderCb : public gm::IPrimitiveCreatorShaderCallback
	{
		gm::GMDemoGameWorld* world = nullptr;

		_ShaderCb(gm::GMDemoGameWorld* d) : world(d)
		{
		}

		virtual void onCreateShader(gm::GMShader& shader) override
		{
			shader.setCull(gm::GMS_Cull::CULL);
			shader.getMaterial().kd = GMVec3(.6f, .2f, .3f);
			shader.getMaterial().ks = GMVec3(.1f, .2f, .3f);
			shader.getMaterial().ka = GMVec3(1, 1, 1);
			shader.getMaterial().shininess = 20;

			auto pk = gm::GameMachine::instance().getGamePackageManager();
			gm::ITexture* tex = nullptr;
			gm::GMToolUtil::createTexture(world->getContext(), "bnp.png", &tex);
			gm::GMToolUtil::addTextureToShader(shader, tex, gm::GMTextureType::NormalMap);
			gm::GMToolUtil::addTextureToShader(shader, tex, gm::GMTextureType::Diffuse);
			world->getAssets().insertAsset(gm::GMAssetType::Texture, tex);
		}
	} cb(asDemoGameWorld(getDemoWorldReference()));

	// 创建一个带纹理的对象
	gm::GMfloat extents[] = { .5f, .5f, .5f };
	gm::GMfloat pos[] = { 0, 0, 1.f };
	gm::GMModel* model;
	gm::GMPrimitiveCreator::createQuad(extents, pos, &model, &cb);
	gm::GMAsset quadAsset = getDemoWorldReference()->getAssets().insertAsset(gm::GMAssetType::Model, model);
	d->gameObject = new gm::GMGameObject(quadAsset);
	asDemoGameWorld(getDemoWorldReference())->addObject("texture", d->gameObject);

	gm::GMWidget* widget = createDefaultWidget();
	auto top = getClientAreaTop();

	gm::GMControlLabel* stateLabel = nullptr;
	widget->addLabel(
		L"状态：旋转中",
		getLabelFontColor(),
		10,
		top,
		250,
		30,
		false,
		&stateLabel
	);
	gm::GMControlButton* button = nullptr;
	widget->addButton(
		L"暂停/继续旋转",
		10,
		top += 40,
		250,
		30,
		false,
		&button
	);

	connect(*button, GM_SIGNAL(gm::GMControlButton::click), [=](gm::GMObject* sender, gm::GMObject* receiver) {
		d->rotate = !d->rotate;
		emit(rotateStateChanged);
	});

	connect(*this, GM_SIGNAL(rotateStateChanged), [=](auto, auto) {
		if (d->rotate)
			stateLabel->setText(L"状态：旋转中");
		else
			stateLabel->setText(L"状态：已停止");
	});

	widget->setSize(widget->getSize().width, top + 40);
}

void Demo_NormalMap::event(gm::GameMachineHandlerEvent evt)
{
	D(d);
	D_BASE(db, Base);
	Base::event(evt);
	switch (evt)
	{
	case gm::GameMachineHandlerEvent::FrameStart:
		break;
	case gm::GameMachineHandlerEvent::FrameEnd:
		break;
	case gm::GameMachineHandlerEvent::Simulate:
	{
		if (d->rotate)
			d->angle += .01f;
		getDemoWorldReference()->simulateGameWorld();
		break;
	}
	case gm::GameMachineHandlerEvent::Render:
		d->rotation = Rotate(Identity<GMQuat>(), d->angle, (GMVec3(0, 0, 1)));
		d->gameObject->setRotation(d->rotation);
		getDemoWorldReference()->renderScene();

		break;
	case gm::GameMachineHandlerEvent::Activate:
	{
		gm::IInput* inputManager = getDemonstrationWorld()->getMainWindow()->getInputMananger();
		gm::IKeyboardState& kbState = inputManager->getKeyboardState();
		if (kbState.keyTriggered(gm::GM_keyFromASCII('0')))
			db->debugConfig.set(gm::GMDebugConfigs::FrameBufferIndex_I32, 0);

		for (gm::GMint i = 0; i < 8; ++i)
		{
			if (kbState.keyTriggered(gm::GM_keyFromASCII('1' + (gm::GMint)i)))
				db->debugConfig.set(gm::GMDebugConfigs::FrameBufferIndex_I32, (gm::GMint)i + 1);
		}
		break;
	}
	case gm::GameMachineHandlerEvent::Deactivate:
		break;
	case gm::GameMachineHandlerEvent::Terminate:
		break;
	default:
		break;
	}
}
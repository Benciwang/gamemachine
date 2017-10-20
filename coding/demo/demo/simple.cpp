#include "stdafx.h"
#include "simple.h"
#include <linearmath.h>

Demo_Simple::~Demo_Simple()
{
	D(d);
	gm::GM_delete(d->demoWorld);
}

void Demo_Simple::init()
{
	D(d);
	Base::init();
	// ��������
	d->demoWorld = new gm::GMDemoGameWorld();

	// ����һ������
	struct _ShaderCb : public gm::IPrimitiveCreatorShaderCallback
	{
		gm::GMDemoGameWorld* world = nullptr;

		_ShaderCb(gm::GMDemoGameWorld* d) : world(d)
		{
		}

		virtual void onCreateShader(gm::Shader& shader) override
		{
			shader.setCull(gm::GMS_Cull::CULL);
			shader.getMaterial().kd = gm::linear_math::Vector3(.6f, .2f, .3f);
			shader.getMaterial().ks = gm::linear_math::Vector3(.1f, .2f, .3f);
			shader.getMaterial().ka = gm::linear_math::Vector3(1, 1, 1);
			shader.getMaterial().shininess = 20;

			auto pk = gm::GameMachine::instance().getGamePackageManager();
			auto& container = world->getAssets();

			gm::GMImage* img = nullptr;
			gm::GMBuffer buf;
			pk->readFile(gm::GMPackageIndex::Textures, "bnp.png", &buf);
			gm::GMImageReader::load(buf.buffer, buf.size, &img);
			GM_ASSERT(img);

			gm::ITexture* tex = nullptr;
			GM.getFactory()->createTexture(img, &tex);
			GM_ASSERT(tex);
			// ��Ҫ�����ͷ�img
			delete img;

			world->getAssets().insertAsset(gm::GMAssetType::Texture, tex);
			{
				auto& frames = shader.getTexture().getTextureFrames(gm::GMTextureType::NORMALMAP, 0);
				frames.addFrame(tex);
			}

			{
				auto& frames = shader.getTexture().getTextureFrames(gm::GMTextureType::DIFFUSE, 0);
				frames.addFrame(tex);
			}

		}
	} cb(d->demoWorld);

	// ����һ��������Ķ���
	gm::GMfloat extents[] = { .5f, .5f, .5f };
	gm::GMfloat pos[] = { 0, 0, -1.f };
	gm::GMModel* coreObj;
	gm::GMPrimitiveCreator::createQuad(extents, pos, &coreObj, &cb);
	gm::GMAsset* quadAsset = d->demoWorld->getAssets().insertAsset(GM_ASSET_MODELS, "quad", gm::GMAssetType::Model, coreObj);
	gm::GMGameObject* obj = new gm::GMGameObject(quadAsset);
	d->demoWorld->appendObject("texture", obj);

	// ���õƹ�
	gm::GMLight light(gm::GMLightType::SPECULAR);
	gm::GMfloat lightPos[] = { 0, 0, .2f };
	light.setLightPosition(lightPos);
	gm::GMfloat color[] = { .7f, .7f, .7f };
	light.setLightColor(color);
	d->demoWorld->addLight(light);
}

void Demo_Simple::event(gm::GameMachineEvent evt)
{
	D(d);
	switch (evt)
	{
	case gm::GameMachineEvent::FrameStart:
		break;
	case gm::GameMachineEvent::FrameEnd:
		break;
	case gm::GameMachineEvent::Simulate:
		d->demoWorld->simulateGameWorld();
		d->demoWorld->notifyControls();
		break;
	case gm::GameMachineEvent::Render:
	{
		gm::IGraphicEngine* engine = GM.getGraphicEngine();
		engine->beginBlend();
		d->demoWorld->renderScene();
		engine->endBlend();
		break;
	}
	case gm::GameMachineEvent::Activate:
	{
		gm::IInput* inputManager = GM.getMainWindow()->getInputMananger();
		gm::IKeyboardState& kbState = inputManager->getKeyboardState();

		if (kbState.keydown('Q') || kbState.keydown(VK_ESCAPE))
			GM.postMessage({ gm::GameMachineMessageType::Quit });

		if (kbState.keydown('B'))
			GM.postMessage({ gm::GameMachineMessageType::Console });

		if (kbState.keyTriggered('I'))
			GMSetDebugState(RUN_PROFILE, !GMGetDebugState(RUN_PROFILE));
		break;
	}
	case gm::GameMachineEvent::Deactivate:
		break;
	case gm::GameMachineEvent::Terminate:
		break;
	default:
		break;
	}
}

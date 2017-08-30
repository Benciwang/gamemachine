﻿#include "stdafx.h"
#include "shader_constants.h"
#include <algorithm>
#include "gmglgraphic_engine.h"
#include "gmdatacore/gmmodel.h"
#include "gmgltexture.h"
#include "gmengine/gmgameobject.h"
#include "gmglmodelpainter.h"
#include "renders/gmgl_renders_object.h"
#include "renders/gmgl_renders_glyph.h"
#include "renders/gmgl_renders_particle.h"
#include "gmglgraphic_engine_default_shaders.h"
#include "foundation/gamemachine.h"
#include "foundation/gmconfig.h"

extern "C"
{
	GLenum s_glErrCode;
}

class GMEffectRenderer
{
public:
	GMEffectRenderer(GMGLFramebuffer& effectBuffer, GMEffects effects, GMGLShaderProgram* program)
		: m_effectBuffer(effectBuffer)
		, m_program(program)
	{
		if (!m_effectBuffer.hasBegun()) // 防止绘制嵌套
		{
			m_isHost = true;
			m_effectBuffer.beginDrawEffects(effects);
		}
	}

	~GMEffectRenderer()
	{
		if (m_isHost || !m_effectBuffer.hasBegun())
		{
			m_effectBuffer.endDrawEffects();
			m_effectBuffer.draw(m_program);
		}
	}

	GLuint framebuffer()
	{
		return m_effectBuffer.framebuffer();
	}

private:
	GMGLFramebuffer& m_effectBuffer;
	GMGLShaderProgram* m_program;
	bool m_isHost = false;
};

GMGLGraphicEngine::~GMGLGraphicEngine()
{
	D(d);
	disposeDeferredRenderQuad();

	for (auto iter : d->forwardRenderingShaders)
	{
		if (iter.second)
			delete iter.second;
	}

	for (auto iter : d->deferredCommonPassShaders)
	{
		if (iter.second)
			delete iter.second;
	}

	if (d->deferredLightPassShader)
		delete d->deferredLightPassShader;

	for (auto iter : d->allRenders)
	{
		if (iter.second)
			delete iter.second;
	}

	if (d->lightPassRender)
		delete d->lightPassRender;

	if (d->effectsShader)
		delete d->effectsShader;
}

void GMGLGraphicEngine::start()
{
	installShaders();
	glDepthFunc(GL_LEQUAL);
	createDeferredRenderQuad();
}

void GMGLGraphicEngine::newFrame()
{
	D(d);
	if (d->renderMode == GMGLRenderMode::DeferredRendering)
	{
		d->gbuffer.releaseBind();
		newFrameOnCurrentContext();
	}
	else
	{
		newFrameOnCurrentContext();
	}
}

void GMGLGraphicEngine::event(const GameMachineMessage& e)
{
	D(d);
	switch (e.msgType)
	{
	case GameMachineMessageType::OnWindowSizeChanged:
	{
		GMRect rect = GM.getMainWindow()->getClientRect();
		setViewport(rect);

		if (d->renderMode == GMGLRenderMode::DeferredRendering)
		{
			if (!refreshGBuffer())
			{
				gm_error("init gbuffer error");
				setRenderMode(GMGLRenderMode::ForwardRendering); // if error occurs, back into forward rendering
			}
		}

		if (!refreshFramebuffer())
			gm_error("init framebuffer error");
		break;
	}
	default:
		break;
	}
}

void GMGLGraphicEngine::drawObjects(GMGameObject *objects[], GMuint count)
{
	D(d);
	GM_PROFILE(drawObjects);
	if (d->renderMode == GMGLRenderMode::ForwardRendering)
	{
		refreshForwardRenderLights();

		{
			GMEffectRenderer effectRender(d->effectBuffer, d->effects, d->effectsShader);
			forwardRender(objects, count);
		}
	}
	else
	{
		ASSERT(getRenderMode() == GMGLRenderMode::DeferredRendering);
		// 把渲染图形分为两组，可延迟渲染组和不可延迟渲染组，先渲染可延迟渲染的图形
		groupGameObjects(objects, count);
		geometryPass(d->deferredRenderingGameObjects);
		newFrame();

		{
			GMEffectRenderer effectRender(d->effectBuffer, d->effects, d->effectsShader);

			lightPass();
			d->gbuffer.copyDepthBuffer(effectRender.framebuffer());
			setRenderMode(GMGLRenderMode::ForwardRendering);
			drawObjects(d->forwardRenderingGameObjects.data(), d->forwardRenderingGameObjects.size());
			setRenderMode(GMGLRenderMode::DeferredRendering);
		}

		viewFrameBuffer();
	}
}

void GMGLGraphicEngine::installShaders()
{
	D(d);
	// 按照Object顺序创建renders
	IRender* renders[] = {
		new GMGLRenders_Object(),
		new GMGLRenders_Glyph(),
		new GMGLRenders_Particle(),
	};

	GMGamePackage* package = GM.getGamePackageManager();

	GM_FOREACH_ENUM_CLASS(type, GMMeshType::MeshTypeBegin, GMMeshType::MeshTypeEnd)
	{
		GMGLShaderProgram* forwardShaderProgram = new GMGLShaderProgram();
		if (!d->shaderLoadCallback || (d->shaderLoadCallback && !d->shaderLoadCallback->onLoadForwardShader(type, *forwardShaderProgram)) )
		{
			if (!loadDefaultForwardShader(type, forwardShaderProgram))
			{
				delete forwardShaderProgram;
				forwardShaderProgram = nullptr;
			}
		}

		if (forwardShaderProgram)
			forwardShaderProgram->load();

		registerForwardRenderingShader(type, forwardShaderProgram);
		registerRender(type, renders[(GMint) type]);
	}

	{
		GMGLShaderProgram* deferredShaderLightPassProgram = new GMGLShaderProgram();
		if (!d->shaderLoadCallback || (d->shaderLoadCallback && !d->shaderLoadCallback->onLoadDeferredLightPassShader(*deferredShaderLightPassProgram)))
		{
			if (!loadDefaultDeferredLightPassShader(deferredShaderLightPassProgram))
			{
				delete deferredShaderLightPassProgram;
				deferredShaderLightPassProgram = nullptr;
			}
		}

		if (deferredShaderLightPassProgram)
			deferredShaderLightPassProgram->load();

		registerLightPassShader(deferredShaderLightPassProgram);
	}

	{
		GMGLShaderProgram* effectsProgram = new GMGLShaderProgram();
		if (!d->shaderLoadCallback || (d->shaderLoadCallback && !d->shaderLoadCallback->onLoadEffectsShader(*effectsProgram)))
		{
			//if (!loadDefaultDeferredLightPassShader(effectsProgram))
			//{
			//	delete effectsProgram;
			//	effectsProgram = nullptr;
			//}
		}

		if (effectsProgram)
			effectsProgram->load();

		registerEffectsShader(effectsProgram);
	}

	GM_FOREACH_ENUM_CLASS(state, GMGLDeferredRenderState::PassingGeometry, GMGLDeferredRenderState::EndOfRenderState)
	{
		GMGLShaderProgram* shaderProgram = new GMGLShaderProgram();
		if (!d->shaderLoadCallback || (d->shaderLoadCallback && !d->shaderLoadCallback->onLoadDeferredPassShader(state, *shaderProgram)))
		{
			if (!loadDefaultDeferredRenderShader(state, shaderProgram))
			{
				delete shaderProgram;
				shaderProgram = nullptr;
			}
		}

		if (shaderProgram)
			shaderProgram->load();

		registerCommonPassShader(state, shaderProgram);
	}

	d->lightPassRender = new GMGLRenders_LightPass();
}

bool GMGLGraphicEngine::loadDefaultForwardShader(const GMMeshType type, GMGLShaderProgram* shaderProgram)
{
	switch (type)
	{
	case GMMeshType::Model:
		shaderProgram->attachShader({ GL_VERTEX_SHADER, gmgl_shaders::object.vert });
		shaderProgram->attachShader({ GL_FRAGMENT_SHADER, gmgl_shaders::object.frag });
		break;
	case GMMeshType::Glyph:
		shaderProgram->attachShader({ GL_VERTEX_SHADER, gmgl_shaders::glyph.vert });
		shaderProgram->attachShader({ GL_FRAGMENT_SHADER, gmgl_shaders::glyph.frag });
		break;
	case GMMeshType::Particles:
		shaderProgram->attachShader({ GL_VERTEX_SHADER, gmgl_shaders::particles.vert });
		shaderProgram->attachShader({ GL_FRAGMENT_SHADER, gmgl_shaders::particles.frag });
		break;
	default:
		return false;
	}
	return true;
}

bool GMGLGraphicEngine::loadDefaultDeferredLightPassShader(GMGLShaderProgram* shaderProgram)
{
	shaderProgram->attachShader({ GL_VERTEX_SHADER, gmgl_shaders::deferred_light_pass.vert });
	shaderProgram->attachShader({ GL_FRAGMENT_SHADER, gmgl_shaders::deferred_light_pass.frag });
	return true;
}

bool GMGLGraphicEngine::loadDefaultDeferredRenderShader(GMGLDeferredRenderState state, GMGLShaderProgram* shaderProgram)
{
	switch (state)
	{
	case GMGLDeferredRenderState::PassingGeometry:
		shaderProgram->attachShader({ GL_VERTEX_SHADER, gmgl_shaders::deferred_geometry_pass.vert });
		shaderProgram->attachShader({ GL_FRAGMENT_SHADER, gmgl_shaders::deferred_geometry_pass.frag });
		break;
	case GMGLDeferredRenderState::PassingMaterial:
		shaderProgram->attachShader({ GL_VERTEX_SHADER, gmgl_shaders::deferred_material_pass.vert });
		shaderProgram->attachShader({ GL_FRAGMENT_SHADER, gmgl_shaders::deferred_material_pass.frag });
		break;
	default:
		return false;
	}
	return true;
}

void GMGLGraphicEngine::activateForwardRenderLight(const Vector<GMLight>& lights)
{
	D(d);
	GM_FOREACH_ENUM(type, GMMeshType::MeshTypeBegin, GMMeshType::MeshTypeEnd)
	{
		IRender* render = getRender(type);
		GMint lightId[(GMuint)GMLightType::COUNT] = { 0 };

		for (auto& light : lights)
		{
			GMint id = lightId[(GMuint)light.getType()]++;
			render->activateLight(light, id);
		}
	}
}

void GMGLGraphicEngine::activateLightPassLight(const Vector<GMLight>& lights)
{
	D(d);
	IRender* render = d->lightPassRender;
	GMint lightId[(GMuint)GMLightType::COUNT] = { 0 };

	for (auto& light : lights)
	{
		GMint id = lightId[(GMuint)light.getType()]++;
		render->activateLight(light, id);
	}
}

bool GMGLGraphicEngine::refreshGBuffer()
{
	D(d);
	GMRect rect = GM.getMainWindow()->getClientRect();
	if (rect.width <= 0 || rect.height <= 0)
		return true;

	d->gbuffer.dispose();
	return d->gbuffer.init(rect.width, rect.height);
}

bool GMGLGraphicEngine::refreshFramebuffer()
{
	D(d);
	GMRect rect = GM.getMainWindow()->getClientRect();
	if (rect.width <= 0 || rect.height <= 0)
		return true;

	d->effectBuffer.dispose();
	return d->effectBuffer.init(rect.width, rect.height);
}

void GMGLGraphicEngine::forwardRender(GMGameObject *objects[], GMuint count)
{
	D(d);
	for (GMuint i = 0; i < count; i++)
	{
		objects[i]->draw();
	}
}

void GMGLGraphicEngine::geometryPass(Vector<GMGameObject*>& objects)
{
	D(d);
	d->gbuffer.beginPass();
	do
	{
		d->gbuffer.newFrame();
		d->gbuffer.bindForWriting();
		GM_CHECK_GL_ERROR();

		for (auto object : objects)
		{
			object->draw();
			GM_CHECK_GL_ERROR();
		}
		d->gbuffer.releaseBind();
		GM_CHECK_GL_ERROR();
	} while (d->gbuffer.nextPass());
}

void GMGLGraphicEngine::lightPass()
{
	D(d);
	d->deferredLightPassShader->useProgram();
	refreshDeferredRenderLights();
	d->gbuffer.activateTextures(d->deferredLightPassShader);
	renderDeferredRenderQuad();
}

void GMGLGraphicEngine::updateVPMatrices(const CameraLookAt& lookAt)
{
	D(d);
	GMMesh dummy;
	GM_FOREACH_ENUM(i, GMMeshType::MeshTypeBegin, GMMeshType::MeshTypeEnd)
	{
		IRender* render = getRender(i);
		dummy.setType(i);

		render->begin(this, &dummy, nullptr);
		render->updateVPMatrices(d->projectionMatrix, d->viewMatrix, lookAt);
		render->end();
	}
}

void GMGLGraphicEngine::groupGameObjects(GMGameObject *objects[], GMuint count)
{
	D(d);
	d->deferredRenderingGameObjects.clear();
	d->deferredRenderingGameObjects.reserve(count);
	d->forwardRenderingGameObjects.clear();
	d->forwardRenderingGameObjects.reserve(count);

	for (GMuint i = 0; i < count; i++)
	{
		if (objects[i]->canDeferredRendering())
			d->deferredRenderingGameObjects.push_back(objects[i]);
		else
			d->forwardRenderingGameObjects.push_back(objects[i]);
	}
}

void GMGLGraphicEngine::viewFrameBuffer()
{
	D(d);
	GMint fbIdx = GMGetBuiltIn(FRAMEBUFFER_VIEWER_INDEX);
	if (fbIdx > 0)
	{
		glDisable(GL_DEPTH_TEST);
		GMint x = GMGetBuiltIn(FRAMEBUFFER_VIEWER_X),
			y = GMGetBuiltIn(FRAMEBUFFER_VIEWER_Y),
			width = GMGetBuiltIn(FRAMEBUFFER_VIEWER_WIDTH),
			height = GMGetBuiltIn(FRAMEBUFFER_VIEWER_HEIGHT);
		d->gbuffer.beginPass();
		d->gbuffer.bindForReading();
		d->gbuffer.setReadBuffer((GBufferGeometryType)(fbIdx - 1));
		glBlitFramebuffer(0, 0, d->gbuffer.getWidth(), d->gbuffer.getHeight(), x, y, width, height, GL_COLOR_BUFFER_BIT, GL_LINEAR);
		GM_CHECK_GL_ERROR();
		d->gbuffer.releaseBind();
	}
}

void GMGLGraphicEngine::updateCameraView(const CameraLookAt& lookAt)
{
	D(d);
	updateMatrices(lookAt);

	// 遍历每一种着色器
	auto renderModeCache = getRenderMode();
	auto renderStateCache = getRenderState();

	setRenderMode(GMGLRenderMode::ForwardRendering);
	updateVPMatrices(lookAt);

	// 更新material pass著色器
	setRenderMode(GMGLRenderMode::DeferredRendering);
	GM_FOREACH_ENUM_CLASS(state, GMGLDeferredRenderState::PassingGeometry, GMGLDeferredRenderState::EndOfRenderState)
	{
		setRenderState(state);
		updateVPMatrices(lookAt);
	}
	setRenderState(renderStateCache);
	setRenderMode(renderModeCache);

	if (getRenderMode() == GMGLRenderMode::DeferredRendering)
	{
		// 更新light pass著色器
		GMMesh dummy;
		IRender* render = d->lightPassRender;
		render->begin(this, &dummy, nullptr);
		render->updateVPMatrices(d->projectionMatrix, d->viewMatrix, lookAt);
		render->end();
	}
}

void GMGLGraphicEngine::updateMatrices(const CameraLookAt& lookAt)
{
	D(d);
	GMCamera& camera = GM.getCamera();

	d->projectionMatrix = camera.getFrustum().getPerspective();
	d->viewMatrix = getViewMatrix(lookAt);

	camera.getFrustum().updateViewMatrix(d->viewMatrix, d->projectionMatrix);
	camera.getFrustum().update();
}

void GMGLGraphicEngine::refreshForwardRenderLights()
{
	D(d);
	if (d->needRefreshLights)
	{
		d->needRefreshLights = false;
		IGraphicEngine* engine = GM.getGraphicEngine();
		activateForwardRenderLight(d->lights);
	}
}

void GMGLGraphicEngine::refreshDeferredRenderLights()
{
	D(d);
	if (d->needRefreshLights)
	{
		d->needRefreshLights = false;
		IGraphicEngine* engine = GM.getGraphicEngine();
		activateLightPassLight(d->lights);
	}
}

void GMGLGraphicEngine::createDeferredRenderQuad()
{
	D(d);
	if (d->quadVAO == 0)
	{
		static GLfloat quadVertices[] = {
			// Positions		// Texture Coords
			-1.0f, 1.0f, 0.0f, 0.0f, 1.0f,
			-1.0f, -1.0f, 0.0f, 0.0f, 0.0f,
			1.0f, 1.0f, 0.0f, 1.0f, 1.0f,
			1.0f, -1.0f, 0.0f, 1.0f, 0.0f,
		};

		glGenVertexArrays(1, &d->quadVAO);
		glBindVertexArray(d->quadVAO);
		glGenBuffers(1, &d->quadVBO);
		glBindBuffer(GL_ARRAY_BUFFER, d->quadVBO);
		glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), &quadVertices, GL_STATIC_DRAW);
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(GLfloat), (GLvoid*)0);
		glEnableVertexAttribArray(1);
		glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(GLfloat), (GLvoid*)(3 * sizeof(GLfloat)));
	}
}

void GMGLGraphicEngine::renderDeferredRenderQuad()
{
	D(d);
	glDisable(GL_CULL_FACE);
	glBindVertexArray(d->quadVAO);
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
	glBindVertexArray(0);

	GM_CHECK_GL_ERROR();
}

void GMGLGraphicEngine::disposeDeferredRenderQuad()
{
	D(d);
	glBindVertexArray(0);
	if (d->quadVAO)
		glDeleteVertexArrays(1, &d->quadVAO);

	if (d->quadVBO)
		glDeleteBuffers(1, &d->quadVBO);
}

void GMGLGraphicEngine::setViewport(const GMRect& rect)
{
	glViewport(rect.x, rect.y, rect.width, rect.height);
}

void GMGLGraphicEngine::registerForwardRenderingShader(GMMeshType objectType, AUTORELEASE GMGLShaderProgram* forwardShaderProgram)
{
	D(d);
	d->forwardRenderingShaders[objectType] = forwardShaderProgram;
}

void GMGLGraphicEngine::registerCommonPassShader(GMGLDeferredRenderState state, AUTORELEASE GMGLShaderProgram* shaderProgram)
{
	D(d);
	d->deferredCommonPassShaders[state] = shaderProgram;
}

void GMGLGraphicEngine::registerLightPassShader(AUTORELEASE GMGLShaderProgram* deferredLightPassProgram)
{
	D(d);
	d->deferredLightPassShader = deferredLightPassProgram;
}

void GMGLGraphicEngine::registerEffectsShader(AUTORELEASE GMGLShaderProgram* effectsShader)
{
	D(d);
	d->effectsShader = effectsShader;
}

GMGLShaderProgram* GMGLGraphicEngine::getShaders(GMMeshType objectType)
{
	D(d);
	GMGLShaderProgram* prog;
	if (d->renderMode == GMGLRenderMode::ForwardRendering)
	{
		prog = d->forwardRenderingShaders[objectType];
	}
	else
	{
		ASSERT(d->renderMode == GMGLRenderMode::DeferredRendering);
		prog = d->deferredCommonPassShaders[d->renderState];
	}

	ASSERT(prog);
	return prog;
}

void GMGLGraphicEngine::registerRender(GMMeshType objectType, AUTORELEASE IRender* render)
{
	D(d);
	d->allRenders[objectType] = render;
}

IRender* GMGLGraphicEngine::getRender(GMMeshType objectType)
{
	D(d);
	if (d->allRenders.find(objectType) == d->allRenders.end())
		return nullptr;

	return d->allRenders[objectType];
}

void GMGLGraphicEngine::setRenderMode(GMGLRenderMode mode)
{
	D(d);
	if (d->renderMode != mode)
		d->needRefreshLights = true;
	d->renderMode = mode;
}

void GMGLGraphicEngine::addLight(const GMLight& light)
{
	D(d);
	d->lights.push_back(light);
	d->needRefreshLights = true;
}

void GMGLGraphicEngine::beginCreateStencil()
{
	D(d);
	d->stencilRenderModeCache = getRenderMode();
	setRenderMode(GMGLRenderMode::ForwardRendering);
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_STENCIL_TEST);
	glClear(GL_STENCIL_BUFFER_BIT);
	glStencilMask(0xFF);
	glStencilFunc(GL_ALWAYS, 1, 0xFF);
	glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);
}

void GMGLGraphicEngine::endCreateStencil()
{
	D(d);
	glStencilMask(0x00);
	setRenderMode(d->stencilRenderModeCache);
}

void GMGLGraphicEngine::beginUseStencil(bool inverse)
{
	if (!inverse)
		glStencilFunc(GL_EQUAL, 1, 0xFF);
	else
		glStencilFunc(GL_NOTEQUAL, 1, 0xFF);
}

void GMGLGraphicEngine::endUseStencil()
{
	glDisable(GL_STENCIL_TEST);
}

void GMGLGraphicEngine::setEffects(GMEffects effects)
{
	D(d);
	d->effects = effects;
}

void GMGLGraphicEngine::newFrameOnCurrentContext()
{
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
}
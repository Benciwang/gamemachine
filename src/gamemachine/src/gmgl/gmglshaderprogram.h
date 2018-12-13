﻿#ifndef __SHADERS_H__
#define __SHADERS_H__
#include <gmcommon.h>
#include <gamemachine.h>
BEGIN_NS

struct GM_EXPORT GMGLShaderInfo
{
	GMuint32 type;
	GMString source;
	GMString filename;

	static GMuint32 toGLShaderType(GMShaderType type);
	static GMShaderType fromGLShaderType(GMuint32 type);
};

typedef AlignedVector<GMuint32> GMGLShaderIDList;
typedef AlignedVector<GMGLShaderInfo> GMGLShaderInfos;

GM_PRIVATE_OBJECT(GMGLShaderProgram)
{
	const IRenderContext* context = nullptr;
	GMGLShaderInfos shaderInfos;
	GMGLShaderIDList shaders;
	GMuint32 shaderProgram = 0;
	GMuint32 techniqueIndex = 0;
	HashMap<GMString, GMString, GMStringHashFunctor> aliasMap;
	static GMuint32 lastUsedProgram;
};

class GM_EXPORT GMGLShaderProgram : public GMObject, public IShaderProgram
{
	GM_DECLARE_PRIVATE(GMGLShaderProgram);

public:
	GMGLShaderProgram(const IRenderContext* context);
	~GMGLShaderProgram();

	bool load();
	GMuint32 getProgram() { D(d); return d->shaderProgram; }
	void attachShader(const GMGLShaderInfo& shaderCfgs);

public:
	virtual void useProgram();
	virtual GMint32 getIndex(const GMString& name) override;
	virtual void setMatrix4(GMint32 index, const GMMat4& value) override;
	virtual void setVec4(GMint32 index, const GMFloat4& value) override;
	virtual void setVec3(GMint32 index, const GMfloat value[3]) override;
	virtual void setInt(GMint32 index, GMint32 value) override;
	virtual void setFloat(GMint32 index, GMfloat value) override;
	virtual void setBool(GMint32 index, bool value) override;
	virtual bool setInterfaceInstance(const GMString& interfaceName, const GMString& instanceName, GMShaderType type) override;

private:
	void setProgram(GMuint32 program) { D(d); d->shaderProgram = program; }
	void removeShaders();
	void expandSource(REF GMGLShaderInfo& shaderInfo);
	GMString expandSource(const GMString& workingDir, const GMString& source);
	bool matchMacro(const GMString& source, const GMString& macro, REF GMString& result);
	void expandInclude(const GMString& workingDir, const GMString& fn, IN OUT GMString& source);
	void expandAlias(const GMString& alias, IN OUT GMString& source);
	GMString& replaceLine(IN OUT GMString& line);
	bool setSubrotinue(const GMString& interfaceName, const GMString& implement, GMuint32 shaderType);
	bool verify();

public:
	virtual bool getInterface(GameMachineInterfaceID id, void** out) override;
	virtual bool setInterface(GameMachineInterfaceID id, void* in) override;

public:
	static const GMString& techniqueName()
	{
		static GMString techName = L"GM_TechniqueEntrance";
		return techName;
	}
};

END_NS
#endif
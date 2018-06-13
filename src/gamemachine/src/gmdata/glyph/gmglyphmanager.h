﻿#ifndef __GLYPHMANAGER_H__
#define __GLYPHMANAGER_H__
#include <gmcommon.h>
#include <map>

BEGIN_NS

struct ITexture;

// 用于管理字形的类
struct GMGlyphInfo
{
	bool valid;
	GMfloat x, y; // 字形在纹理中的位置
	GMfloat width, height; // 字形在纹理中的大小
	GMfloat bearingX, bearingY;
	GMfloat advance;
};

struct GMGlyphBitmap
{
	GMbyte* buffer;
	GMuint width;
	GMuint rows;
};

typedef void* GMFontFace;
typedef GMsize_t GMFontHandle;

struct GMFont
{
	GMString fontPath;
	GMFontFace face;
};

typedef HashMap<GMFontHandle, HashMap<GMint, HashMap<GMwchar, GMGlyphInfo> > > CharList;

GM_PRIVATE_OBJECT(GMGlyphManager)
{
	const IRenderContext* context = nullptr;
	CharList chars;
	GMint cursor_u, cursor_v;
	GMfloat maxHeight;
	Vector<GMFont> fonts;

	GMFontHandle defaultFontSun;
	GMFontHandle defaultFontTimesNewRoman;
};

class GMGlyphManager : public GMObject
{
	GM_DECLARE_PRIVATE(GMGlyphManager);

public:
	// 文字纹理的大小
	enum
	{
		CANVAS_WIDTH = 1024,
		CANVAS_HEIGHT = 1024,
	};

	static const GMsize_t InvalidHandle;

public:
	GMGlyphManager(const IRenderContext* context);
	virtual ~GMGlyphManager();

public:
	const GMGlyphInfo& getChar(GMwchar c, GMFontSizePt fontSize, GMFontHandle font);
	GMFontHandle addFontByFileName(const GMString& fontFileName);
	GMFontHandle addFontByFullName(const GMString& fontFullName);

public:
	virtual ITexture* glyphTexture() = 0;

public:
	inline GMFontHandle getSimSun()
	{
		D(d);
		return d->defaultFontSun;
	}

	inline GMFontHandle getTimesNewRoman()
	{
		D(d);
		return d->defaultFontTimesNewRoman;
	}

private:
	virtual void updateTexture(const GMGlyphBitmap& bitmapGlyph, const GMGlyphInfo& glyphInfo) = 0;

private:
	const GMGlyphInfo& createChar(GMwchar c, GMFontSizePt fontSize, GMFontHandle font);
	GMFont* getFont(GMFontHandle);
	GMGlyphInfo& insertChar(GMFontSizePt fontSize, GMFontHandle font, GMwchar ch, const GMGlyphInfo& glyph);
	const GMGlyphInfo& getChar(GMFontSizePt fontSize, GMFontHandle font, GMwchar ch);
	const GMGlyphInfo& getCharRecursively(GMFontHandle fontStart, GMFontHandle fontSkip, GMFontSizePt fontSize, GMwchar ch);
};

END_NS
#endif

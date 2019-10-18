/****************************************************************************
 Copyright (c) 2013-2016 Chukong Technologies Inc.

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

#include <cocos/renderer/CCQuadCommand.h>

#include <cocos/base/ccMacros.h>
#include <cocos/base/ccTypes.h>
#include <cocos/math/Mat4.h>
#include <cocos/math/Vec2.h>
#include <cocos/platform/CCPlatformMacros.h>
#include <cocos/renderer/CCGLProgramState.h>
#include <cocos/renderer/CCTexture2D.h>
#include <cocos/renderer/CCTrianglesCommand.h>

#include <algorithm>
#include <limits>
#include <new>

NS_CC_BEGIN

std::size_t QuadCommand::__indexCapacity = static_cast<std::size_t>(0);
GLushort* QuadCommand::__indices = nullptr;

QuadCommand::~QuadCommand()
{
    for (auto& indices : _ownedIndices)
    {
        CC_SAFE_DELETE_ARRAY(indices);
    }
}

void QuadCommand::init(float globalOrder, GLuint textureID, GLProgramState* glProgramState, const BlendFunc& blendType, V3F_C4B_T2F_Quad* quads,
                       std::size_t quadCount, const Mat4& mv, uint32_t flags)
{
    CCASSERT(glProgramState, "Invalid GLProgramState");
    CCASSERT(glProgramState->getVertexAttribsFlags() == 0, "No custom attributes are supported in QuadCommand");

    if ((quadCount * static_cast<std::size_t>(6)) > _indexSize)
        reIndex(quadCount * static_cast<std::size_t>(6));

    Triangles triangles;
    triangles.verts = &quads->tl;
    triangles.vertCount = static_cast<int>(quadCount * static_cast<std::size_t>(4));
    triangles.indices = __indices;
    triangles.indexCount = static_cast<int>(quadCount * static_cast<std::size_t>(6));
    TrianglesCommand::init(globalOrder, textureID, glProgramState, blendType, triangles, mv, flags);
}

void QuadCommand::reIndex(std::size_t indicesCount)
{
    // first time init: create a decent buffer size for indices to prevent too much resizing
    if (__indexCapacity == static_cast<std::size_t>(0))
    {
        indicesCount = std::max(indicesCount, static_cast<std::size_t>(2048));
    }

    if (indicesCount > __indexCapacity)
    {
        // if resizing is needed, get needed size plus 25%, but not bigger that max size
        assert(indicesCount < std::numeric_limits<std::size_t>::max() / static_cast<std::size_t>(5));
        indicesCount = (indicesCount * static_cast<std::size_t>(5)) / static_cast<std::size_t>(4);

        CCLOG("cocos2d: QuadCommand: resizing index size from [%d] to [%d]", __indexCapacity, indicesCount);
        _ownedIndices.emplace_back(__indices);
        __indices = new (std::nothrow) GLushort[indicesCount];
        __indexCapacity = indicesCount;
    }

    for (int i = 0, max = static_cast<int>(__indexCapacity / static_cast<std::size_t>(6)); i < max; i++)
    {
        __indices[i * 6 + 0] = static_cast<GLushort>(i * 4 + 0);
        __indices[i * 6 + 1] = static_cast<GLushort>(i * 4 + 1);
        __indices[i * 6 + 2] = static_cast<GLushort>(i * 4 + 2);
        __indices[i * 6 + 3] = static_cast<GLushort>(i * 4 + 3);
        __indices[i * 6 + 4] = static_cast<GLushort>(i * 4 + 2);
        __indices[i * 6 + 5] = static_cast<GLushort>(i * 4 + 1);
    }

    _indexSize = indicesCount;
}

void QuadCommand::init(float globalOrder, GLuint textureID, GLProgramState* shader, const BlendFunc& blendType, V3F_C4B_T2F_Quad* quads, std::size_t quadCount,
                       const Mat4& mv)
{
    init(globalOrder, textureID, shader, blendType, quads, quadCount, mv, 0);
}

void QuadCommand::init(float globalOrder, Texture2D* texture, GLProgramState* glProgramState, const BlendFunc& blendType, V3F_C4B_T2F_Quad* quads,
                       std::size_t quadCount, const Mat4& mv, uint32_t flags)
{
    init(globalOrder, texture->getName(), glProgramState, blendType, quads, quadCount, mv, flags);
#ifdef DEBUG_TEXTURE_SIZE
    CC_ASSERT(texture != nullptr);
    _texSize = {static_cast<float>(texture->getPixelsWide()), static_cast<float>(texture->getPixelsHigh())};
#endif
    _alphaTextureID = texture->getAlphaTextureName();
}

NS_CC_END

/****************************************************************************
 Copyright (C) 2013 Henry van Merode. All rights reserved.
 Copyright (c) 2015 Chukong Technologies Inc.

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

#ifndef CC_EXTENSIONS_PARTICLE3D_PU_MANAGER_H
#define CC_EXTENSIONS_PARTICLE3D_PU_MANAGER_H

#include <cocos/base/CCRef.h>
#include <cocos/extensions/Particle3D/PU/CCPUAffector.h>
#include <cocos/extensions/Particle3D/PU/CCPUAlignAffectorTranslator.h>
#include <cocos/extensions/Particle3D/PU/CCPUBaseColliderTranslator.h>
#include <cocos/extensions/Particle3D/PU/CCPUBaseForceAffectorTranslator.h>
#include <cocos/extensions/Particle3D/PU/CCPUBoxColliderTranslator.h>
#include <cocos/extensions/Particle3D/PU/CCPUCollisionAvoidanceAffectorTranslator.h>
#include <cocos/extensions/Particle3D/PU/CCPUColorAffectorTranslator.h>
#include <cocos/extensions/Particle3D/PU/CCPUFlockCenteringAffectorTranslator.h>
#include <cocos/extensions/Particle3D/PU/CCPUForceFieldAffectorTranslator.h>
#include <cocos/extensions/Particle3D/PU/CCPUGeometryRotatorTranslator.h>
#include <cocos/extensions/Particle3D/PU/CCPUGravityAffectorTranslator.h>
#include <cocos/extensions/Particle3D/PU/CCPUInterParticleColliderTranslator.h>
#include <cocos/extensions/Particle3D/PU/CCPUJetAffectorTranslator.h>
#include <cocos/extensions/Particle3D/PU/CCPULineAffectorTranslator.h>
#include <cocos/extensions/Particle3D/PU/CCPULinearForceAffectorTranslator.h>
#include <cocos/extensions/Particle3D/PU/CCPUParticleFollowerTranslator.h>
#include <cocos/extensions/Particle3D/PU/CCPUPathFollowerTranslator.h>
#include <cocos/extensions/Particle3D/PU/CCPUPlaneColliderTranslator.h>
#include <cocos/extensions/Particle3D/PU/CCPURandomiserTranslator.h>
#include <cocos/extensions/Particle3D/PU/CCPUScaleAffectorTranslator.h>
#include <cocos/extensions/Particle3D/PU/CCPUScaleVelocityAffectorTranslator.h>
#include <cocos/extensions/Particle3D/PU/CCPUScriptTranslator.h>
#include <cocos/extensions/Particle3D/PU/CCPUSineForceAffectorTranslator.h>
#include <cocos/extensions/Particle3D/PU/CCPUSphereColliderTranslator.h>
#include <cocos/extensions/Particle3D/PU/CCPUTextureAnimatorTranslator.h>
#include <cocos/extensions/Particle3D/PU/CCPUTextureRotatorTranslator.h>
#include <cocos/extensions/Particle3D/PU/CCPUVelocityMatchingAffectorTranslator.h>
#include <cocos/extensions/Particle3D/PU/CCPUVortexAffectorTranslator.h>

NS_CC_BEGIN

class PUAffectorManager
{
public:
    static PUAffectorManager* Instance();

    /**
     */
    PUScriptTranslator* getTranslator(const std::string& type);
    PUAffector* createAffector(const std::string& type);

    CC_CONSTRUCTOR_ACCESS : PUAffectorManager();
    ~PUAffectorManager();

protected:
    PUAlignAffectorTranslator _alignAffectorTranslator;
    PUBaseColliderTranslator _baseColliderTranslator;
    PUBaseForceAffectorTranslator _baseForceAffectorTranslator;
    PUBoxColliderTranslator _boxColliderTranslator;
    PUCollisionAvoidanceAffectorTranslator _collisionAvoidanceAffectorTranstor;
    PUColorAffectorTranslator _colorAffectorTranslator;
    PUFlockCenteringAffectorTranslator _flockCenteringAffectorTranslator;
    PUForceFieldAffectorTranslator _forceFieldAffectorTranslator;
    PUGeometryRotatorTranslator _geometryRotatorTranslator;
    PUGravityAffectorTranslator _gravityAffectorTranslator;
    PUInterParticleColliderTranslator _interParticleColliderTranslator;
    PUJetAffectorTranslator _jetAffectorTranslator;
    PULineAffectorTranslator _lineAffectorTranslator;
    PULinearForceAffectorTranslator _linearForceAffectorTranslator;
    PUParticleFollowerTranslator _particleFollowerTranslator;
    PUPathFollowerTranslator _pathFollowerTranslator;
    PUPlaneColliderTranslator _planeColliderTranslator;
    PURandomiserTranslator _randomiserTranslator;
    PUScaleAffectorTranslator _scaleAffectorTranslator;
    PUScaleVelocityAffectorTranslator _scaleVelocityAffectorTranslator;
    PUSineForceAffectorTranslator _sineForceAffectorTranslator;
    PUSphereColliderTranslator _sphereColliderTranslator;
    PUTextureAnimatorTranslator _textureAnimatorTranslator;
    PUTextureRotatorTranslator _textureRotatorTranslator;
    PUVelocityMatchingAffectorTranslator _velocityMatchingAffectorTranslator;
    PUVortexAffectorTranslator _vortexAffectorTranslator;
};

NS_CC_END

#endif // CC_EXTENSIONS_PARTICLE3D_PU_MANAGER_H

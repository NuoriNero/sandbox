//
//  sb_clear_scene.cpp
//  YinYang
//
//  Created by Андрей Куницын on 2/15/12.
//  Copyright (c) 2012 AndryBlack. All rights reserved.
//

#include "sb_clear_scene.h"
#include "sb_graphics.h"

SB_META_DECLARE_OBJECT(Sandbox::ClearScene, Sandbox::SceneObject)

namespace Sandbox {
    
    
    void ClearScene::Draw(Sandbox::Graphics &g) const {
        g.Clear(m_color,m_depth);
    }
    
}
